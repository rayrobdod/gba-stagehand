	.section .text
	.thumb
	.cpu arm7tdmi

	.align 2
	.global keyinput_read
	.thumb_func
keyinput_read:
	// r0-r2: previous, down, new
	ldr r3, .Lhwreg
	ldrh r1, [r3]
	ldr r3, .Lstruct
	ldrh r0, [r3, #0]

	// performing bit-ops on the entire keypad struct at once
	// is the part I could not convince the c compiler to do
	mvn r2, r0
	orr r2, r1

	strh r1, [r3, #0]
	strh r2, [r3, #2]

	bx lr
	.align 4
.Lstruct:
	.word keyinputs
.Lhwreg:
	.word reg_keypad

	.size keyinput_read, .-keyinput_read
