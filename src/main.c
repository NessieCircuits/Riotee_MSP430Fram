#include <msp430.h>
#include <msp430fr5994.h>
#include <stdbool.h>
#include <stdint.h>

#include "printf.h"
#include "uart.h"

void spi_init() {

  /* UCA0CLK on P1.5 */
  P1SEL0 &= ~BIT5;
  P1SEL1 |= BIT5;

  /* UCA0MOSI on P2.0 and UCA0MISO on P2.1 */
  P2SEL0 &= ~(BIT0 | BIT1);
  P2SEL1 |= (BIT0 | BIT1);

  /* Input direction for CS */
  P5DIR &= ~BIT3;
  /* Enable pullup */
  P5REN |= BIT3;
  P5OUT |= BIT3;
  /* Enable interrupt */
  P5IE |= BIT3;
  /* Configure falling edge */
  P5IES |= BIT3;

  /* Put to reset */
  UCA0CTLW0 |= UCSWRST;
  /* Configure */
  UCA0CTLW0 = UCMSB | UCSYNC | UCMODE_0;
  /* Release reset */
  UCA0CTLW0 &= ~UCSWRST;
}

void dma_init(void) {

  DMA2CTL &= ~DMAEN;

  /* DMA2 reads the first three command bytes into memory */
  DMA2CTL = DMASRCBYTE | DMADSTBYTE | DMADSTINCR_3 | DMADT_0;
  /* trigger 16 is USCIA0RXIFG*/
  DMACTL1 |= DMA2TSEL_14;
  DMA2SA = (uint16_t)&UCA0RXBUF;
  DMA2SZ = 3;

  /* DMA0 handles 'write' requests by shoveling data into memory */
  DMA0SZ = 0xFFFF;
  DMA0CTL &= ~DMAEN;
  DMA0CTL = (DMASRCBYTE | DMADSTBYTE | DMADSTINCR_3 | DMADT_0);
  DMACTL0 |= DMA0TSEL_14;
  DMA0SA = (uint32_t)&UCA0RXBUF;

  /* DMA1 handles 'reads' requests by shoveling data out of memory */
  DMA1SZ = 0xFFFF;
  DMA1CTL &= ~DMAEN;
  DMA1CTL = (DMASRCBYTE | DMADSTBYTE | DMASRCINCR_3 | DMADT_0);
  DMACTL0 |= DMA1TSEL_15;
  DMA1DA = (uint32_t)&UCA0TXBUF;
}

static volatile bool tfrequest = false;

__attribute__((interrupt(PORT5_VECTOR))) void PORT5_ISR(void) {
  P5IFG &= ~BIT3;

  tfrequest = true;
}

void delay_cycles(long unsigned int cycles) {
  for (long unsigned int i = 0; i < cycles; i++)
    __no_operation();
}

extern __int20__ cs_handler(uint8_t *data_buf);
volatile uint32_t result;

static uint8_t data_buf[1024] = {0};

int main(void) {

  /* Stop watchdog timer */
  WDTCTL = WDTPW | WDTHOLD;

  /* Apply the GPIO configuration */
  PM5CTL0 &= ~LOCKLPM5;

  /* Unlock clock system registers */
  CSCTL0 = 0xA5 << 8;

  /* Set to 16MHz */
  CSCTL1 = (DCORSEL | DCOFSEL_4);

  /* No divider */
  CSCTL3 = DIVS_0 | DIVM_0 | DIVA_0;

  CSCTL2 = SELM__DCOCLK | SELS__DCOCLK | SELA__LFXTCLK;

  /* This pin shall output SMCLK */
  P3DIR |= BIT4;
  P3SEL1 |= BIT4;

  P4DIR |= BIT1 | BIT2;
  P4OUT &= ~(BIT1 | BIT2);

  uart_init();
  spi_init();

  _putchar('$');

  /* Clear interrupt flags */
  P5IFG &= ~BIT3;
  /* Enable interrupts */
  __bis_SR_register(GIE);

  dma_init();

  while (1) {
    P4OUT ^= BIT1;
    if (tfrequest) {

      result = cs_handler(data_buf);

      /* Wait for CS high */
      while ((P5IN & BIT3) == 0) {
      };

      for (unsigned int i = 0xDE; i < 0xDE + 255; i++) {
        _putchar(data_buf[i]);
      }

      /* Reset DMA channels */
      DMA0CTL &= ~DMAEN;
      DMA1CTL &= ~DMAEN;
      DMA2CTL &= ~DMAEN;
      DMA0SZ = 0xFFFF;
      DMA1SZ = 0xFFFF;
      DMA2SZ = 3;

      /* Reset SPI */
      UCA0CTLW0 |= UCSWRST;
      UCA0CTLW0 &= ~UCSWRST;

      tfrequest = false;
    }
  };
}
