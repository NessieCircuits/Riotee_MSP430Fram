#include <msp430.h>
#include <msp430fr2433.h>
#include <stdbool.h>
#include <stdint.h>

#include "printf.h"
#include "uart.h"

extern unsigned int spiloop();
extern uint16_t spiend;

int clock_init() {
  // Configure one FRAM waitstate as required by the device datasheet for MCLK
  // operation beyond 8MHz _before_ configuring the clock system.
  FRCTL0 = FRCTLPW | NWAITS_1;

  __bis_SR_register(SCG0);   // disable FLL
  CSCTL3 |= SELREF__REFOCLK; // Set REFO as FLL reference source
  CSCTL0 = 0;                // clear DCO and MOD registers
  CSCTL1 &= ~(DCORSEL_7);    // Clear DCO frequency select bits first
  CSCTL1 |= DCORSEL_5;       // Set DCO = 16MHz
  CSCTL2 = FLLD_0 + 487;     // set to fDCOCLKDIV = (FLLN + 1)*(fFLLREFCLK/n)
                             //                   = (487 + 1)*(32.768 kHz/1)
                             //                   = 16 MHz

  CSCTL4 = SELMS__DCOCLKDIV |
           SELA__REFOCLK; // set default REFO(~32768Hz) as ACLK source, ACLK =
                          // 32768Hz default DCODIV as MCLK and SMCLK source
  __delay_cycles(3);
  __bic_SR_register(SCG0); // enable FLL
  while (CSCTL7 & (FLLUNLOCK0 | FLLUNLOCK1))
    ; // FLL locked
#if 0
  __bis_SR_register(SCG0);   // disable FLL
  CSCTL3 |= SELREF__REFOCLK; // Set REFO as FLL reference source
  CSCTL1 = DCOFTRIMEN | DCOFTRIM0 | DCOFTRIM1 |
           DCORSEL_3;    // DCOFTRIM=3, DCO Range = 8MHz
  CSCTL2 = FLLD_0 + 243; // DCODIV = 8MHz
  __delay_cycles(3);
  __bic_SR_register(SCG0); // enable FLL
#endif

  return 0;
}

int gpio_init() {
  P1DIR = 0xFF;
  P2DIR = 0xFF;
  P3DIR = 0xFF;

  P1REN = 0xFF;
  P2REN = 0xFF;
  P3REN = 0xFF;

  P1OUT = 0x00;
  P2OUT = 0x00;
  P3OUT = 0x00;

  /* Set to output direction */
  P1DIR |= (BIT0 | BIT1 | BIT2);
  /* Clear output latch for a defined power-on state */
  P1OUT &= ~(BIT0 | BIT1 | BIT2);

  /* Configure P2.3 as input direction pin */
  P2DIR &= ~(BIT3);

  P2OUT |= BIT3; // Configure P2.3 as pulled-up
  P2REN |= BIT3; // P2.3 pull-up register enable

  P2IES |= BIT3; // P2.3 Falling edge
  P2IFG = 0;     // Clear all P2 interrupt flags

  /* P2.3 interrupt enabled */
  P2IE |= BIT3;

  return 0;
}

void spi_init() {
  /* Configure MOSI and MISO pins (primary function) */
  P2SEL0 |= BIT4 | BIT5 | BIT6;

  /* Input direction for CS */
  P2DIR &= ~BIT7;

#if 0
  P2OUT |= BIT7; // Configure P2.3 as pulled-up
  P2REN |= BIT7; // P2.3 pull-up register enable
#else
  /* Disable pull register */
  P2REN &= ~BIT7;
#endif
  /* Configure falling edge */
  P2IES |= BIT7;
  /* Clear interrupt */
  P2IFG = 0;
  /* Enable interrupt */
  P2IE |= BIT7;

  /* Put to reset */
  UCA1CTLW0 = UCSWRST;
  UCA1CTLW0 |= UCCKPL | UCMSB | UCSYNC | UCMODE_0;
  /* Release reset */
  UCA1CTLW0 &= ~UCSWRST;
  /* Enable USCI1 RX interrupt */
  // UCA1IE |= UCRXIE;
}

static volatile unsigned int active = false;

uint16_t c_handler(uint16_t address) {
  if (P2IFG & BIT3) {
    P2IFG &= ~BIT3;
    P1OUT ^= BIT0;
  }
  if (P2IFG & BIT7) {
    P2IFG &= ~BIT7;

    if (!active) {
      active = true;
      /* Configure rising edge */
      P2IES &= ~BIT7;

    } else {
      active = false;
      P2IES |= BIT7;
      return (uint16_t)&spiend;
    }
  }
  return address;
}

void __attribute__((interrupt(PORT2_VECTOR), naked)) PORT2_ISR(void) {
  uint16_t ret_addr;
  __asm__ __volatile__("pushm.a #2,r13\n"
                       "mov 10(r1), %0 \n"
                       "mov %0, r12\n"
                       "calla #c_handler\n"
                       "mov r12, %0\n"
                       : "=r"(ret_addr));

  __asm__ __volatile__("mov %0, 10(r1)\n"
                       "bic	#0xf0, 8(r1)\n"
                       "popm.a #2,r13\n"
                       "reti" ::"r"(ret_addr));
}

void tmploop(uint8_t *);
int main(void) {
  unsigned int n_tcvd = 0;
  /* Stop watchdog timer */
  WDTCTL = WDTPW | WDTHOLD;

  gpio_init();

  clock_init();
  uart_init();
  spi_init();

  /* Apply the GPIO configuration */
  PM5CTL0 &= ~LOCKLPM5;
  /* Enable FRAM R/W */
  SYSCFG0 = FRWPPW | DFWP;

  uart_puts("Henlo");

  while (1)
    while (1) {
      __bis_SR_register(LPM4_bits | GIE);
      if (!active)
        continue;

      /* Configure pointer */
      n_tcvd = spiloop();
      printf("Transferred: %u,0x%04X", n_tcvd, n_tcvd);
    }
}
