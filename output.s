.macro rem dst, divd, divr
sdiv \dst, \divd, \divr
msub \dst, \dst, \divr, \divd
.endm
.text
.align 2
.globl MAX
.type MAX, @function
MAX:
	str x29,[sp,#-16]!
	add w14,w0,w1

	mov w0,w14
	ldr x29,[sp],#16
	ret
.text
.align 2
.globl max_sum_nonadjacent
.type max_sum_nonadjacent, @function
max_sum_nonadjacent:
	stp x29,x30,[sp,#-16]!
	stp x28,x27,[sp,#-16]!
	stp x26,x25,[sp,#-16]!
	str x24,[sp,#-16]!
	sub sp,sp,#16
	add x29,sp,#0
	mov x27,x0
	mov w28,w1
	ldr w26,[x27]
	str w26,[x29]
	ldr w26,[x27]
	ldr w25,[x27,#4]
	mov w0,w26
	mov w1,w25
	bl MAX
	mov w25,w0
	str w25,[x29,#4]
	mov w25,#2
	b .L2
.L1:
	sub w26,w25,#2
	add x17,x29,x26,lsl 2
	ldr w26,[x17]
	add x17,x27,x25,lsl 2
	ldr w24,[x17]
	add w24,w26,w24
	sub w26,w25,#1
	add x17,x29,x26,lsl 2
	ldr w26,[x17]
	mov w0,w24
	mov w1,w26
	bl MAX
	mov w26,w0
	add x17,x29,x25,lsl 2
	str w26,[x17]
	add w26,w25,#1
	mov w25,w26
.L2:
	subs wzr,w25,w28
	blt .L1

	sub w26,w28,#1
	add x17,x29,x26,lsl 2
	ldr w26,[x17]

	mov w0,w26
	add sp,sp,#16
	ldr x24,[sp],#16
	ldp x26,x25,[sp],#16
	ldp x28,x27,[sp],#16
	ldp x29,x30,[sp],#16
	ret
.text
.align 2
.globl func4
.type func4, @function
func4:
	str x29,[sp,#-16]!
	subs wzr,w0,wzr
	bne .L6

	mov w15,w2
	b .L8
	b .L7
.L6:
	mov w15,w1
	b .L8
.L7:
.L8:
	mov w0,w15
	ldr x29,[sp],#16
	ret
