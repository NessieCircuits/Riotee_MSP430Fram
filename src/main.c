#include <msp430.h>
#include <msp430fr5994.h>
#include <stdbool.h>
#include <stdint.h>

#include "printf.h"
#include "uart.h"

/*

LPM4: 600nA (SVS), 400nA (no SVS), wakeup time <10us
LPM4.5: 250nA (SVS), 45 nA (no SVS), wakeup time 250us (SVS), 400us (no SVS)

*/

void delay_cycles(long unsigned int cycles) {
  for (long unsigned int i = 0; i < cycles; i++)
    __no_operation();
}

void spi_init() {
  /* Put to reset */
  UCA3CTLW0 |= UCSWRST;

  /* Configure for 3-pin SPI slave */
  UCA3CTLW0 = UCCKPH | UCMSB | UCSYNC | UCMODE_0;

  /* UCA3CLK on P6.2 */
  P6SEL1 &= ~BIT2;
  P6SEL0 |= BIT2;

  /* UCA3MOSI on P6.0 and UCA3MISO on P6.1 */
  P6SEL0 |= (BIT0 | BIT1);
  P6SEL1 &= ~(BIT0 | BIT1);

  /* Release reset */
  UCA3CTLW0 &= ~UCSWRST;
}

void dma_init(void) {
  /* Reset DMA channels */
  DMA3CTL &= ~DMAEN;
  DMA4CTL &= ~DMAEN;
  DMA5CTL &= ~DMAEN;

  /* DMA3 reads the first three command bytes into memory */
  DMA3CTL = DMASRCBYTE | DMADSTBYTE | DMADSTINCR_3 | DMADT_0;
  /* trigger 16 is USCIA0RXIFG*/
  DMACTL1 |= DMA3TSEL_16;
  DMA3SA = (uint16_t)&UCA3RXBUF;
  DMA3SZ = 3;

  /* DMA4 handles 'write' requests by shoveling data into memory */
  DMA4SZ = 0xFFFF;
  DMA4CTL &= ~DMAEN;
  DMA4CTL = (DMASRCBYTE | DMADSTBYTE | DMADSTINCR_3 | DMADT_0);
  DMACTL2 |= DMA4TSEL_16;
  DMA4SA = (uint32_t)&UCA3RXBUF;

  /* DMA5 handles 'read' requests by shoveling data out of memory */
  DMA5SZ = 0xFFFF;
  DMA5CTL &= ~DMAEN;
  DMA5CTL = (DMASRCBYTE | DMADSTBYTE | DMASRCINCR_3 | DMADT_0);
  DMACTL2 |= DMA5TSEL_17;
  DMA5DA = (uint32_t)&UCA3TXBUF;
}

extern __int20__ cs_handler(uint8_t *data_buf);

__attribute__((interrupt(PORT6_VECTOR))) void PORT6_ISR(void) {
  P6IFG &= ~BIT3;
  LPM4_EXIT;
}

int main(void) {

  /* Stop watchdog */
  WDTCTL = WDTPW | WDTHOLD | WDTCNTCL;

  /* Apply the GPIO configuration */
  PM5CTL0 &= ~LOCKLPM5;

  // Configure GPIO
  P1OUT = 0;
  P1DIR = 0xFF;
  P1SEL0 = 0x0;
  P1SEL1 = 0x0;

  P2OUT = 0;
  P2DIR = 0xFF;
  P2SEL0 = 0x0;
  P2SEL1 = 0x0;

  P3OUT = 0;
  P3DIR = 0xFF;
  P2SEL0 = 0x0;
  P2SEL1 = 0x0;

  P4OUT = 0;
  P4DIR = 0xFF;
  P2SEL0 = 0x0;
  P2SEL1 = 0x0;

  P5OUT = 0;
  P5DIR = 0xFF;
  P2SEL0 = 0x0;
  P2SEL1 = 0x0;

  P6DIR = ~(BIT0 | BIT2 | BIT3);
  P6OUT = 0;
  P2SEL0 = 0x0;
  P2SEL1 = 0x0;

  P7OUT = 0;
  P7DIR = 0xFF;
  P2SEL0 = 0x0;
  P2SEL1 = 0x0;

  P8OUT = 0;
  P8DIR = 0xFF;
  P2SEL0 = 0x0;
  P2SEL1 = 0x0;

  PJOUT = 0;
  PJDIR = 0xFFFF;
  P2SEL0 = 0x0;
  P2SEL1 = 0x0;

  /* Use password and add wait states */
  FRCTL0 = FRCTLPW | NWAITS_1;

  /* Unlock clock system registers */
  CSCTL0 = 0xA5 << 8;

  /* Set to 16MHz */
  CSCTL1 = (DCORSEL | DCOFSEL_4);

  /* No divider */
  CSCTL3 = DIVS_0 | DIVM_0 | DIVA_0;

  CSCTL2 = SELM__DCOCLK | SELS__DCOCLK | SELA__LFXTCLK;

  /* Clear pending interrupt */
  P6IFG &= ~BIT3;
  /* Input direction for CS */
  P6DIR &= ~BIT3;
  /* Enable interrupt */
  P6IE |= BIT3;
  /* Configure rising edge interrupt */
  P6IES &= ~BIT3;

  /* Wait in LPM4 until CS is high */
  __bis_SR_register(GIE + LPM4_bits);

  // uart_init();
  spi_init();
  dma_init();

  P4DIR |= BIT1 | BIT2;
  P4OUT |= BIT2;
  P4OUT &= ~BIT2;

  volatile __int20__ tmp;

  while (1) {
    /* Configure falling edge */
    P6IES |= BIT3;
    /* Clear pending interrupt */
    P6IFG &= ~BIT3;

    /* Stop watchdog */
    WDTCTL = WDTPW | WDTHOLD;

    /* Go into LPM4 and wakeup on GPIO edge */
    __bis_SR_register(LPM4_bits);

    /* Start watchdog with 250ms period */
    WDTCTL = WDTPW | WDTCNTCL | WDTSSEL__ACLK | WDTIS_5;

    tmp = cs_handler((uint8_t *)0x10000);

    /* Configure rising edge interrupt */
    P6IES &= ~BIT3;
    /* Clear pending interrupt */
    P6IFG &= ~BIT3;

    /* Go into LPM0 to still allow DMA to move data */
    __bis_SR_register(LPM0_bits);
    /* Reset DMA channels */
    DMA3CTL &= ~DMAEN;
    DMA4CTL &= ~DMAEN;
    DMA5CTL &= ~DMAEN;

    DMA4SZ = 0xFFFF;
    DMA5SZ = 0xFFFF;
    DMA3SZ = 3;

    /* Reset SPI */
    UCA3CTLW0 |= UCSWRST;
    UCA3TXBUF = 0x0;
    UCA3CTLW0 &= ~UCSWRST;
  };
}
