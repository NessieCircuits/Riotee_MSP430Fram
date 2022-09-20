#include <msp430.h>
#include <msp430f5529.h>
#include <stdbool.h>
#include <stdint.h>

#include "printf.h"
#include "uart.h"

void spi_init() {
  /* Configure MOSI, MISO and CLK pins (primary function) */
  P3SEL |= BIT3 | BIT4;
  P2SEL |= BIT7;

  /* Input direction for CS */
  P1DIR &= ~BIT2;
  /* Enable pullup */
  P1REN |= BIT2;
  P1OUT |= BIT2;
  /* Configure falling edge */
  P1IES |= BIT2;
  /* Clear interrupt */
  P1IFG = 0;
  /* Enable interrupt */
  P1IE |= BIT2;

  /* Put to reset */
  UCA0CTL1 |= UCSWRST;
  /* Configure */
  UCA0CTL0 = UCMSB | UCSYNC | UCMODE_0;
  /* Release reset */
  UCA0CTL1 &= ~UCSWRST;

  /* Enable USCI1 RX interrupt */
  // UCB0IE |= UCRXIE;
}

void dma_init(void) {

  /* DMA2 reads the first three command bytes into memory */
  DMA2CTL = DMASRCBYTE | DMADSTBYTE | DMADSTINCR_3 | DMADT_0;
  /* trigger 16 is USCIA0RXIFG*/
  DMACTL1 |= DMA2TSEL_16;
  DMA2SA = (uint16_t)&UCA0RXBUF;
  DMA2SZ = 3;

  /* DMA0 handles 'write' requests by shoveling data into memory */
  DMA0SZ = 0xFFFF;
  DMA0CTL &= ~DMAEN;
  DMA0CTL = (DMASRCBYTE | DMADSTBYTE | DMADSTINCR_3 | DMADT_0);
  DMACTL0 |= DMA0TSEL_16;
  DMA0SA = (uint32_t)&UCA0RXBUF;

  /* DMA1 handles 'reads' requests by shoveling data out of memory */
  DMA1SZ = 0xFFFF;
  DMA1CTL &= ~DMAEN;
  DMA1CTL = (DMASRCBYTE | DMADSTBYTE | DMASRCINCR_3 | DMADT_0);
  DMACTL0 |= DMA1TSEL_17;
  DMA1DA = (uint32_t)&UCA0TXBUF;
}

static volatile bool tfrequest = false;

__attribute__((interrupt(PORT1_VECTOR))) void PORT1_ISR(void) {

  if (P1IFG & BIT2) {
    P1IFG &= ~BIT2;
    tfrequest = true;
  }
}

void delay_cycles(long unsigned int cycles) {
  for (long unsigned int i = 0; i < cycles; i++)
    __no_operation();
}

extern void cs_handler(uint8_t *data_buf);

static uint8_t data_buf[1024] = {0};

int main(void) {

  /* Stop watchdog timer */
  WDTCTL = WDTPW | WDTHOLD;

  /* Disable FLL */
  __bis_SR_register(SCG0);

  /* Set DCO val and frequency range to f_DCO(7,0) [datasheet tab. 8.19] */
  UCSCTL1 = DCORSEL_4;
  UCSCTL0 = (28 << 8);

  /* Derive SMCLK from DCOCLK directly */
  UCSCTL4 = SELM__DCOCLK | SELS__DCOCLK | SELA__XT1CLK;

  /* This pin shall output ACLK */
  P1DIR |= BIT0;
  P1SEL |= BIT0;

  /* This pin shall output SMCLK */
  P2DIR |= BIT2;
  P2SEL |= BIT2;

  /* Set to output direction */
  P2DIR |= BIT0;
  P4DIR |= BIT1;

  /* Apply the GPIO configuration */
  // PM5CTL0 &= ~LOCKLPM5;

  uart_init();
  _putchar('!');

  spi_init();
  dma_init();
  __bis_SR_register(GIE);

  while (1) {
    P2OUT ^= BIT0;
    if (tfrequest) {

      cs_handler(data_buf);

      /* Wait for CS high */
      while ((P1IN & BIT2) == 0) {
      };

      for (unsigned int i = 255; i < 255 + 255; i++) {
        _putchar(data_buf[i]);
      }

      /* Reset DMA channels */
      DMA0CTL &= ~DMAEN;
      DMA1CTL &= ~DMAEN;
      DMA2CTL &= ~DMAEN;
      DMA0SZ = 0xFFFF;
      DMA1SZ = 0xFFFF;

      /* Reset SPI */
      UCA0CTL1 |= UCSWRST;
      UCA0CTL1 &= ~UCSWRST;

      tfrequest = false;
    }
  };
}
