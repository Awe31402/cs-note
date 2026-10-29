# 課本 Example 107：a1 是不是完全平方數（課本把 a1 寫死成 101，這裡改從外面給）
	addi a0, zero, 0
	addi t1, zero, 1
.loop:
	mul t2, t1, t1
	beq t2, a1, .square
	addi t1, t1, 1
	blt a1, t2, .end
	jal zero, .loop
.square:
	addi a0, a0, 1
.end:
	ecall
