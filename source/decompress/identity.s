	.section .text.IdentityUnComp
	.thumb
	.balign 2
	.syntax unified
	.global IdentityUnComp
	.thumb_func
	.type	IdentityUnComp, %function
IdentityUnComp:
	// r0 - src
	// r1 - dest
	// r2 - length remaining
	ldmia	r0!, {r2}
	lsrs	r2, r2, #8
	// fallthrough

	.type	IdentityUnComp_FastCopy, %function
	.global IdentityUnComp_FastCopy
IdentityUnComp_FastCopy:
	cmp	r2, #31
	bls	.L_32LoopEnd
	push	{r4, r5, r6}
.L_32LoopBegin:
	ldmia	r0!, {r3, r4, r5, r6}
	stmia	r1!, {r3, r4, r5, r6}
	ldmia	r0!, {r3, r4, r5, r6}
	stmia	r1!, {r3, r4, r5, r6}
	subs	r2, r2, #32
	cmp	r2, #31
	bgt	.L_32LoopBegin
	pop	{r4, r5, r6}
.L_32LoopEnd:

	// most common use case should be 4bpp tiles, which are guarenteed to be a multiple of 32 bytes,
	// and maps which are likely to have a width of 32 and thus be a multiple of 32 bytes.
	// Exit early in this common case
	cmp	r2, #0
	bne	1f
	bx	lr
1:

	cmp	r2, #3
	ble	.L_4LoopEnd
.L_4LoopBegin:
	ldmia	r0!, {r3}
	stmia	r1!, {r3}
	subs	r2, r2, #4
	cmp	r2, #3
	bgt	.L_4LoopBegin
.L_4LoopEnd:
	cmp	r2, #1
	ble	1f
	ldrh	r3, [r0]
	strh	r3, [r1]
	adds	r0, r0, #2
	adds	r1, r1, #2
	subs	r2, r2, #2
1:
	cmp	r2, #0
	bne	1f
	bx	lr
1:
	ldrb	r3, [r0]
	strb	r3, [r1]
	bx	lr
	.size	IdentityUnComp, .-IdentityUnComp
	.size	IdentityUnComp_FastCopy, .-IdentityUnComp_FastCopy
