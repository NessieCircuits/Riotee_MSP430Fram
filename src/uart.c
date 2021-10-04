#include "uart.h"
#include "printf.h"

#include <msp430.h>
#include <msp430fr2433.h>

int uart_init() {
  // Configure UART pins
  P1SEL0 |= BIT4 | BIT5; // set 2-UART pin as second function

  // Configure UART
  UCA0CTLW0 |= UCSWRST;
  UCA0CTLW0 |= UCSSEL__SMCLK;

  UCA0BR0 = 0x8B;
  UCA0BR1 = 0x0;
  UCA0MCTLW = 0x08;

  /* Initialize eUSCI */
  UCA0CTLW0 &= ~UCSWRST;
  return 0;
}

void _putchar(char c) {
  while (!(UCA0IFG & UCTXIFG)) {
  };
  UCA0TXBUF = c;
}

int uart_puts(char *buf) {
  char *ptr = buf;
  while (*ptr != '\0') {
    _putchar(*ptr);
    ptr++;
  }
  return 0;
}
