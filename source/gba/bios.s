    .section .text
    .thumb

    .align 2
    .global VBlankIntrWait
    .thumb_func
VBlankIntrWait:
    swi     0x05
    bx      lr
