	.cpu arm7tdmi
	.arch armv4t
	.section .text
	.thumb
	.align 2
	.syntax unified
	.global IdentityUnComp
	.thumb_func
	.type	IdentityUnComp, %function
IdentityUnComp:
	// r0 - src
	// r1 - dest
	// r2 - length remaining
	ldmia r0!, {r2}
	lsrs	r2, r2, #8
	cmp r2, #31
	bls  .LTEQ31
	push {r4, r5, r6}
.GT31:
	ldmia r0!, {r3, r4, r5, r6}
	stmia r1!, {r3, r4, r5, r6}
	ldmia r0!, {r3, r4, r5, r6}
	stmia r1!, {r3, r4, r5, r6}
	subs r2, r2, #32
	cmp r2, #31
	bgt  .GT31
	pop {r4, r5, r6}
.LTEQ31:

	// most common use case should be 4bpp tiles, which are guarenteed to be a multiple of 32 bytes,
	// and maps which are likely to have a width of 32 and thus be a multiple of 32 bytes.
	// Exit early in this common case
	cmp r2, #0
	bne .NEQ0_1
	bx	lr
.NEQ0_1:

	cmp r2, #3
	ble  .LTEQ3
.GT3:
	ldmia r0!, {r3}
	stmia r1!, {r3}
	subs r2, r2, #4
	cmp r2, #3
	bgt  .GT3
.LTEQ3:
	cmp r2, #1
	ble  .LTEQ1
	ldrh	r3, [r0]
	strh	r3, [r1]
	adds r0, r0, #2
	adds r1, r1, #2
	subs r2, r2, #2
.LTEQ1:
	cmp r2, #0
	bne  .NEQ0
	bx	lr
.NEQ0:
	ldrb	r3, [r0]
	strb	r3, [r1]
	bx	lr
	.size	IdentityUnComp, .-IdentityUnComp
