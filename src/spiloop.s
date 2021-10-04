.file "spiloop.s"

#include <msp430fr2433.h>

#define PC r0
#define SP r1
#define SR r2


.section .data

.global spiloop
.global spiend


spiloop:
    bic.b   #3, &0x053c; clear all pending interrupts
    bit.b	#1,	&0x053c; SPI RX?
    jz $-4
    mov &0x052c, r12; store first byte of description

    bis.b #4,&0x0202; set pin high

    bit.b	#1,	&0x053c; SPI RX?
    jz $-4
    mov &0x052c, r13; store second byte of description
    swpb.w r13;
    add r13, r12; r12 should now hold offset+r/w bit
    mov r12, r13; store it for later

    bic #0x4000, r12; clear the r/w bit
    add #__nvm_start, r12; add offset of nvm

    bit.w #0x4000, r13; check for r/w bit
    jz txloop; jump to tx routine

    bit.b	#1,	&0x053c; drop next byte
    jz $-4
    bic.b   #1, &0x053c
    bit.b	#1,	&0x053c; drop next byte
    jz $-4
    bic.b   #1, &0x053c

rxloop:
    ;xor.b #4,&0x0202
    bit.b	#1,	&0x053c
    jz rxloop;
    mov.b &0x052c, @r12
    inc r12;
    jmp rxloop

txloop:
    ;xor.b #4,&0x0202
    mov.b @r12+, &0x052e
    bit.b	#2,	&0x053c
    jz $-4;
    jmp txloop


spiend:
    bic #0x4000, r13; clear the r/w bit
    sub r13, r12; subtract offset
    sub #__nvm_start, r12; subtract nvm start address
    bic.b #4,&0x0202
    mov.b #0, &0x052e; clean TX pipe
    ret

.end
