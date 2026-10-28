#include <stdio.h>
#include <stdint.h>
/* rax / eax / ax / ah / al 是同一個暫存器的不同部分 */
int main(void)
{
	uint64_t r;
	asm volatile("movabs $0x1122334455667788, %%rax\n\t"
		     "mov $0xAB, %%al\n\t"            /* 寫 al：只改最低 8 位 */
		     "mov %%rax, %0" : "=r"(r) :: "rax");
	printf("after mov al, 0xAB   : rax = 0x%016lx\n", r);
	asm volatile("movabs $0x1122334455667788, %%rax\n\t"
		     "mov $0xCD, %%ah\n\t"            /* 寫 ah：第 8–15 位 */
		     "mov %%rax, %0" : "=r"(r) :: "rax");
	printf("after mov ah, 0xCD   : rax = 0x%016lx\n", r);
	asm volatile("movabs $0x1122334455667788, %%rax\n\t"
		     "mov $0xBEEF, %%ax\n\t"          /* 寫 ax：只改最低 16 位 */
		     "mov %%rax, %0" : "=r"(r) :: "rax");
	printf("after mov ax, 0xBEEF : rax = 0x%016lx\n", r);
	asm volatile("movabs $0x1122334455667788, %%rax\n\t"
		     "mov $0xDEADBEEF, %%eax\n\t"     /* 寫 eax：高 32 位被清成 0！ */
		     "mov %%rax, %0" : "=r"(r) :: "rax");
	printf("after mov eax, ...   : rax = 0x%016lx\n", r);
	return 0;
}
