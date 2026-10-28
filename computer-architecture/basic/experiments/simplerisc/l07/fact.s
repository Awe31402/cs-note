	mov r1, 1        /* prod = 1 */
	mov r2, r0       /* idx = num */
.loop:
	mul r1, r1, r2   /* prod = prod * idx */
	sub r2, r2, 1    /* idx = idx - 1 */
	cmp r2, 1        /* compare (idx, 1) */
	bgt .loop        /* if (idx > 1) goto .loop */
