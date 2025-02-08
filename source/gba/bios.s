    .section .text
    .thumb

    .align 2
    .global VBlankIntrWait
    .thumb_func
VBlankIntrWait:
    swi     0x05
    bx      lr

    .align 2
    .global CpuSet
    .thumb_func
CpuSet:
    swi     0x0B
    bx      lr

    .align 2
    .global CpuFastSet
    .thumb_func
CpuFastSet:
    swi     0x0C
    bx      lr

    .align 2
    .global BitUnPack
    .thumb_func
BitUnPack:
    swi     0x10
    bx      lr
