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
  UCA0CTLW0 |= UCSWRST;

  /* Configure for 3-pin SPI slave */
  UCA0CTLW0 = UCCKPH | UCMSB | UCSYNC | UCMODE_0;

  /* UCA0CLK on P1.5 */
  P1SEL0 &= ~BIT5;
  P1SEL1 |= BIT5;

  /* UCA0MOSI on P2.0 and UCA0MISO on P2.1 */
  P2SEL0 &= ~(BIT0 | BIT1);
  P2SEL1 |= (BIT0 | BIT1);

  /* Release reset */
  UCA0CTLW0 &= ~UCSWRST;
}

static uint8_t dma_cmd_buf[4];

void dma_init(void) {
  /* Reset DMA channels */
  DMA0CTL &= ~DMAEN;
  DMA1CTL &= ~DMAEN;
  DMA2CTL &= ~DMAEN;

  /* DMA0 reads the first three command bytes into memory */
  DMA0CTL = DMASRCBYTE | DMADSTBYTE | DMADSTINCR_3 | DMADT_0;
  /* trigger 14 is USCIA0RXIFG*/
  DMACTL0 |= DMA0TSEL_14;
  DMA0SA = (uintptr_t)&UCA0RXBUF;
  DMA0DA = (uintptr_t)dma_cmd_buf;
  DMA0SZ = 3;

  /* DMA1 handles 'write' requests by shoveling data into memory */
  DMA1CTL = (DMASRCBYTE | DMADSTBYTE | DMADSTINCR_3 | DMADT_0);
  DMACTL0 |= DMA1TSEL_14;
  DMA1SA = (uintptr_t)&UCA0RXBUF;
  DMA1SZ = 0xFFFF;

  /* DMA2 handles 'read' requests by shoveling data out of memory */
  DMA2CTL = (DMASRCBYTE | DMADSTBYTE | DMASRCINCR_3 | DMADT_0);
  DMACTL1 |= DMA2TSEL_15;
  DMA2DA = (uintptr_t)&UCA0TXBUF;
  DMA2SZ = 0xFFFF;
}

int gpio_init(void) {

  /* To save energy, all non-shared GPIOs are put to a defined state */
  P1OUT = 0x0;
  P1DIR = BIT0 | BIT1 | BIT2 | BIT6 | BIT7;
  P1SEL0 = 0x0;
  P1SEL1 = 0x0;

  P2OUT = 0;
  P2DIR = BIT2 | BIT7;
  P2SEL0 = 0x0;
  P2SEL1 = 0x0;

  P3OUT = 0;
  P3DIR = BIT0 | BIT1 | BIT2 | BIT4 | BIT5 | BIT7;
  P3SEL0 = 0x0;
  P3SEL1 = 0x0;

  P4OUT = 0;
  P4DIR = BIT0 | BIT1 | BIT2 | BIT3 | BIT4 | BIT5 | BIT7;
  P4SEL0 = 0x0;
  P4SEL1 = 0x0;

  P5OUT = 0;
  P5DIR = BIT6 | BIT7;
  P5SEL0 = 0x0;
  P5SEL1 = 0x0;

  P6OUT = 0;
  P6DIR = BIT0 | BIT1 | BIT3 | BIT6 | BIT7;
  P6SEL0 = 0x0;
  P6SEL1 = 0x0;

  P7OUT = 0;
  P7DIR = BIT1 | BIT2 | BIT4 | BIT6 | BIT7;
  /* This is configured as ADC pin to reduce leakage */
  P7SEL0 = 0x0;
  P7SEL1 = 0x0;

  P8OUT = 0;
  P8DIR = BIT0 | BIT1 | BIT2 | BIT3;
  P8SEL0 = 0x0;
  P8SEL1 = 0x0;

  PJOUT = 0;
  PJDIR = BIT3 | BIT7;
  PJSEL0 = 0x0;
  PJSEL1 = 0x0;

  return 0;
}

__attribute__((interrupt(PORT1_VECTOR))) void PORT1_ISR(void) {
  P1IFG &= ~BIT4;
  LPM4_EXIT;
}

int setup_transfer(uint8_t *base_addr) {

  /* Clear RX and DMA interrupt */
  UCA0IFG &= ~UCRXIFG;
  /* Enable DMA0 */
  DMA0CTL |= DMAEN;
  /* Wait for DMA transfer to finish */
  while ((DMA0CTL & DMAIFG) == 0) {
  };
  DMA0CTL &= ~DMAIFG;

  uint8_t *addr = base_addr + *((uintptr_t *)dma_cmd_buf);

  /* Write request */
  if (dma_cmd_buf[2] & 0x80) {
    _data16_write_addr(&DMA1DA, addr);
    UCA0IFG &= ~UCRXIFG;
    DMA1CTL |= DMAEN;
    /* Read request */
  } else {
    UCA0TXBUF = *addr;
    _data16_write_addr(&DMA2SA, addr + 1);
    DMA2CTL |= DMAEN;
  }

  return 0;
}

int main(void) {

  /* Stop watchdog */
  WDTCTL = WDTPW | WDTHOLD | WDTCNTCL;

  /* Apply the GPIO configuration */
  PM5CTL0 &= ~LOCKLPM5;

  gpio_init();

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
  P1IFG &= ~BIT4;
  /* Input direction for CS */
  P1DIR &= ~BIT4;
  /* Enable interrupt */
  P1IE |= BIT4;
  /* Configure rising edge interrupt */
  P1IES &= ~BIT4;

  /* Wait in LPM4 until CS is high */
  __bis_SR_register(GIE + LPM4_bits);

  /* WARNING: THIS CAN INTERFERE WITH NRF52 */
  // uart_init();
  // P3OUT &= ~BIT6;
  // P3DIR |= BIT6;

  spi_init();
  dma_init();

  while (1) {
    /* Configure falling edge */
    P1IES |= BIT4;
    /* Clear pending interrupt */
    P1IFG &= ~BIT4;

    /* Go into LPM4 and wakeup on GPIO edge */
    __bis_SR_register(LPM4_bits);

    setup_transfer((uint8_t *)0x10000UL);

    /* Configure rising edge interrupt */
    P1IES &= ~BIT4;
    /* Clear pending interrupt */
    P1IFG &= ~BIT4;

    /* Go into LPM0 to still allow DMA to move data */
    __bis_SR_register(LPM0_bits);

    /* Reset DMA channels */
    DMA0CTL &= ~DMAEN;
    DMA1CTL &= ~DMAEN;
    DMA2CTL &= ~DMAEN;

    DMA0SZ = 3;
    DMA2SZ = 0xFFFF;
    DMA1SZ = 0xFFFF;

    /* Reset SPI */
    UCA0CTLW0 |= UCSWRST;
    UCA0TXBUF = 0x0;
    UCA0CTLW0 &= ~UCSWRST;
  };
}
