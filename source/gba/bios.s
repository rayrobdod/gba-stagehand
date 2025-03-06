	.section .text
	.thumb

	.align 2
	.global IntrWait
	.thumb_func
IntrWait:
	swi	0x04
	bx	lr

	.align 2
	.global VBlankIntrWait
	.thumb_func
VBlankIntrWait:
	swi	0x05
	bx	lr

	.align 2
	.global Div
	.thumb_func
Div:
	push	{r0}
	mov	r0, r1
	mov	r1, r2
	swi	0x06
	pop	{r2}
	stmia	r2!, {r0, r1, r3}
	bx	lr

	.align 2
	.global Sqrt
	.thumb_func
Sqrt:
	swi	0x08
	bx	lr

	.align 2
	.global CpuSet
	.thumb_func
CpuSet:
	swi	0x0B
	bx	lr

	.align 2
	.global CpuFastSet
	.thumb_func
CpuFastSet:
	swi	0x0C
	bx	lr

	.align 2
	.global BitUnPack
	.thumb_func
BitUnPack:
	swi	0x10
	bx	lr

	.section .text.lz
	.align 2
	.global LZ77UnCompWram
	.thumb_func
LZ77UnCompWram:
	swi	0x11
	bx	lr

	.align 2
	.global LZ77UnCompVram
	.thumb_func
LZ77UnCompVram:
	swi	0x12
	bx	lr

	.align 2
	.global HuffUnComp
	.thumb_func
HuffUnComp:
	swi	0x13
	bx	lr

	.align 2
	.global RLUnCompWram
	.thumb_func
RLUnCompWram:
	swi	0x14
	bx	lr

	.align 2
	.global RLUnCompVram
	.thumb_func
RLUnCompVram:
	swi	0x15
	bx	lr

	.align 2
	.global Diff8UnFilterWram
	.thumb_func
Diff8UnFilterWram:
	swi	0x16
	bx	lr

	.align 2
	.global Diff8UnFilterVram
	.thumb_func
Diff8UnFilterVram:
	swi	0x17
	bx	lr

	.align 2
	.global Diff16UnFilter
	.thumb_func
Diff16UnFilter:
	swi	0x18
	bx	lr
