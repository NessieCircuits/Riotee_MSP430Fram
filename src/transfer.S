.file "spiloop.s"

#include <msp430fr5994.h>

#define PC r0
#define SP r1
#define SR r2
#define DMA_READ_MASK (DMASRCBYTE | DMADSTBYTE | DMASRCINCR_3 | DMADT_0)
#define DMA_WRITE_MASK (DMASRCBYTE | DMADSTBYTE | DMADSTINCR_3 | DMADT_0)

.section .text

.global cs_handler

cs_handler:
    push r4
    sub #4, SP; make room for 3B command buffer
    mova SP,&DMA3DA; point DMA2 to command buffer on stack
    bic.b #UCRXIFG,&UCA3IFG; Clear RX interrupt
    bis.w #DMAEN,&DMA3CTL; Enable DMA3

    bis.b #BIT1,&P4OUT; set pin high
    bit.w #DMAIFG,&DMA3CTL; DMA interrupt?
    jz $-4
    bic.w #DMAIFG,&DMA3CTL; clear DMA interrupt

    mova @SP, r4; move received data into r4
    adda r12, r4; add base buffer address
    bit.b #0x80,2(SP); test for R/W bit (first byte on stack)
    jz read
write:
    mova r4, &DMA4DA; destination is requested address
    bic.b #UCRXIFG,&UCA3IFG; Clear RX interrupt
    bis.w #DMAEN,&DMA4CTL; Enable DMA4
    jmp handler_end
read:
    mov.b @r4,&UCA3TXBUF; put first byte into buffer
    adda #1, r4; point to second byte
    mova r4, &DMA5SA; source is requested address
    bis.w #DMAEN,&DMA5CTL; Enable DMA5
handler_end:
    bic.b #BIT1,&P4OUT; set pin low
    mova r4, r12
    add #4, SP
    pop r4
    reta

.end
