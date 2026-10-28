	b .main
.foo:
	add r2, r0, r1
	ret
.main:
	mov r0, 3
	mov r1, 5
	call .foo
	add r3, r2, 10
