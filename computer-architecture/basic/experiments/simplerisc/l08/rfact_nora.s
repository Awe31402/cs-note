	b .main
.factorial:
	cmp r0, 1          /* compare (1,num) */
	beq .return
	bgt .continue
	b .return
.continue:
	sub sp, sp, 8      /* create space on the stack */
	st r0, [sp]        /* push r0 on the stack */

	sub r0, r0, 1      /* num = num - 1 */
	call .factorial    /* result will be in r1 */
	ld r0, [sp]        /* pop r0 from the stack */

	mul r1, r0, r1     /* factorial(n) = n * factorial(n-1) */
	add sp, sp, 8      /* delete the activation block */
	ret
.return:
	mov r1, 1
	ret
.main:
	mov r0, 10
	call .factorial
	b .end
.end:
