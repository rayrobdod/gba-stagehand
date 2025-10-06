	.section	.text.RlZeroUnComp
	.thumb
	.balign	2
	.syntax	unified
	.global	RlZeroUnComp
	.thumb_func
	.type	RlZeroUnComp, %function
RlZeroUnComp:
	// r0 - src
	// r1 - dest

	// r2 - dest end
	// r3 - remaining zeros
	// r4 - remaining copies
	// r5 - const 0

	push	{r4-r5}
	ldmia	r0!, {r2}
	lsrs	r2, r2, #8
	adds	r2, r1
	movs	r5, #0

	cmp	r1, r2
	bhs	.Lend
.Louter:
	ldrh	r3, [r0]
	adds	r0, r0, #2
	lsrs	r4, r3, #8
	lsls	r3, r3, #24
	lsrs	r3, r3, #24

	subs	r3, #1
	bmi	.Lzero_end
.Lzero:
	strh	r5, [r1]
	adds	r1, r1, #2
	subs	r3, #1
	bpl	.Lzero
.Lzero_end:

	subs	r4, #1
	bmi	.Lcopy_end
.Lcopy:
	ldrh	r3, [r0]
	adds	r0, r0, #2
	strh	r3, [r1]
	adds	r1, r1, #2
	subs	r4, #1
	bpl	.Lcopy
.Lcopy_end:

	cmp	r1, r2
	blo	.Louter
.Lend:
	pop	{r4-r5}
	bx	lr
	.size	RlZeroUnComp, .-RlZeroUnComp
