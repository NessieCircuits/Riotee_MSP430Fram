#include "uart.h"
#include "printf.h"

#include <msp430.h>
#include <msp430fr5994.h>

int uart_init() {
  // Configure UART TX pin
  P6SEL0 |= BIT0;
  P6SEL1 &= ~BIT0;
  /* Hold in reset state */
  UCA3CTLW0 = UCSWRST;

  /* Configure for standard UART operation */
  UCA3CTLW0 = 0;

  /* 16MHz clock */
  UCA3CTLW0 |= UCSSEL__SMCLK;

  /* This should generate 115200 baudrate */
  UCA3BRW = 8;
  UCA3MCTLW = UCOS16_1 | (10 << 4) | (0xF7 << 8);

  /* Release */
  UCA3CTLW0 &= ~UCSWRST;
  return 0;
}

void _putchar(char c) {
  while (!(UCA3IFG & UCTXIFG)) {
  };
  UCA3TXBUF = c;
}

int uart_puts(char *buf) {
  char *ptr = buf;
  while (*ptr != '\0') {
    _putchar(*ptr);
    ptr++;
  }
  return 0;
}
