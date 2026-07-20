	.global	sram_version
	.section	.rodata.sram_version,"a"
	.balign	4
	.type	sram_version, %object
sram_version:
	.asciz	"SRAM_Vnnn"
	.size sram_version, .-sram_version



	.balign 4
	.section .text.memcpy_sram
_memcpy_sram_begin:
	.type	_memcpy_sram, %function
	.thumb_func
_memcpy_sram:
	// r0: dest
	// r1: src
	// r2: count

	cmp	r2, #0
	beq	.L_1LoopEnd
.L_1LoopBegin:
	ldrb	r3, [r1]
	strb	r3, [r0]
	add	r0, r0, #1
	add	r1, r1, #1
	sub	r2, r2, #1
	bne	.L_1LoopBegin
.L_1LoopEnd:
	bx	lr
	.balign 4
	.size _memcpy_sram, .-_memcpy_sram
	.set _memcpy_sram_len, .-_memcpy_sram



	.thumb
	.balign 2
	.section .text.memcpy_from_sram
	.global memcpy_from_sram
	.thumb_func
	.type	memcpy_from_sram, %function
memcpy_from_sram:
	push	{r0, r1, r2, r6, r7}
	push	{lr}
	mov	r7, sp
	ldr	r0, .Lsubfn
	mov	r2, #_memcpy_sram_len
	sub	r6, r7, r2
	mov	sp, r6
	mov	r1, sp
	bl	IdentityUnComp_FastCopy
	add	r3, r6, #1
	mov	sp, r7
	pop	{r0}
	mov	lr, r0
	pop	{r0, r1, r2, r6, r7}
	bx	r3
	.balign 4
.Lsubfn:
	.word _memcpy_sram_begin
	.size memcpy_from_sram, .-memcpy_from_sram



	.thumb
	.balign 2
	.section .text.memcpy_to_sram
	.global memcpy_to_sram
	.thumb_func
	.type	memcpy_to_sram, %function
memcpy_to_sram:
	b _memcpy_sram
	.size memcpy_to_sram, .-memcpy_to_sram
