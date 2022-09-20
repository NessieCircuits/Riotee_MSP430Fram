#include "uart.h"
#include "printf.h"

#include <msp430.h>
#include <msp430f5529.h>

int uart_init() {
  // Configure UART pins
  P4SEL |= BIT4 | BIT5; // set 2-UART pin as second function

  /* Hold in reset state */
  UCA1CTL1 = UCSWRST;

  /* Configure for standard UART operation */
  UCA1CTL0 = 0;

  UCA1CTL1 |= UCSSEL__ACLK;

  /* This should generate 32768 baudrate */
  UCA1BR0 = 0x01;
  UCA1BR1 = 0x00;
  UCA1MCTL = 0;

  /* Release */
  UCA1CTL1 &= ~UCSWRST;
  return 0;
}

void _putchar(char c) {
  while (!(UCA1IFG & UCTXIFG)) {
  };
  UCA1TXBUF = c;
}

int uart_puts(char *buf) {
  char *ptr = buf;
  while (*ptr != '\0') {
    _putchar(*ptr);
    ptr++;
  }
  return 0;
}
