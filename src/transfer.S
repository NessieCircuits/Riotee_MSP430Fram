.file "spiloop.s"

#include <msp430f5529.h>

#define PC r0
#define SP r1
#define SR r2
#define DMA_READ_MASK (DMASRCBYTE | DMADSTBYTE | DMASRCINCR_3 | DMADT_0)
#define DMA_WRITE_MASK (DMASRCBYTE | DMADSTBYTE | DMADSTINCR_3 | DMADT_0)

.section .data


.global cs_handler

cs_handler:
    push r4-r5
    sub #4, SP; make room for 3B command buffer

    movx SP,&DMA2DA; point DMA2 to command buffer on stack
    bic.b #UCRXIFG,&UCA0IFG; Clear RX interrupt
    bis.w #DMAEN,&DMA2CTL; Enable DMA2

    bis.b #2,&0x0223; set pin high
    bit.w #8,&DMA2CTL; DMA interrupt?
    jz $-4
    bic.w #8,&DMA2CTL; clear DMA interrupt
    ;bic.b #2,&PBOUT_H; set pin low
    movx @SP, r4; move received data into r4
    addx r12, r4
    bit.b #0x80,2(SP); test for R/W bit (first byte on stack)
    mov.b 2(SP), r12
    jz read
write:
    movx r4, &DMA0DA; destination is requested address
    bic.b #UCRXIFG,&UCA0IFG; Clear RX interrupt
    bis.w #DMAEN,&DMA0CTL; Enable DMA0
    jmp handler_end
read:
    mov.b @r4,&UCA0TXBUF; put first byte into buffer
    addx #1, r4; point to second byte
    movx r4, &DMA1SA; source is requested address
    bis.w #DMAEN,&DMA1CTL; Enable DMA1
handler_end:
    bic.b #2,&PBOUT_H; set pin low
    add #4, SP
    pop r4-r5
    ret



.end
