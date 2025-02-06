	.section	.text, "ax"
	.global	 vblank_ack
	.cpu		arm7tdmi

	.macro mov_full reg, value
	mov \reg, #(\value & 0xFF000000)
	add \reg, \reg, #(\value & 0xFF0000)
	add \reg, \reg, #(\value & 0xFF00)
	add \reg, \reg, #(\value & 0xFF)
	.endm

	.arm
vblank_ack:
	.if 0
	mov r0, #0x05000000
	sub r0, r0, #0xA00 // REG_DEBUG_STRING
	mov r1, #0x69
	add r1, #0x7300
	add r1, r1, #0x720000 // 'isr\0'
	str r1, [r0]
	add r0, r0, #0x100
	mov r1, #0x100
	add r1, r1, #3
	strh r1, [r0]
	.endif


	mov r1, #0x1  // INTR_FLAG_VBLANK

	mov r0, #0x04000000
	add r0, r0, #0x200
	strh r1, [r0, #0x2]  // IF

	mov r0, #0x03000000
	add r0, r0, #0x8000
	//sub r0, r0, #0x8
	strh r1, [r0, #-8]  // IFBIOS

	bx lr
