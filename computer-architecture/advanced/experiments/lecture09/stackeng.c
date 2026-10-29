// stackeng.c：同樣是一串互相依賴的 sub／add，改 rsp 跟改一般暫存器（rbx）哪個快？
#include <stdio.h>
#include <time.h>
#define N 100000000L
static double now(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec + t.tv_nsec * 1e-9; }

int main(void)
{
	long x = 0, eight = 8;
	double t0 = now();
	for (long i = 0; i < N; i++)            /* rsp：先減再加，回到原值；4 條互相依賴 */
		asm volatile("sub $8, %%rsp\n\tsub $8, %%rsp\n\tadd $8, %%rsp\n\tadd $8, %%rsp" ::: "memory");
	double t1 = now();
	for (long i = 0; i < N; i++)            /* 一樣的事做在 rbx（加暫存器，避開「加常數」的捷徑） */
		asm volatile("sub %1, %0\n\tsub %1, %0\n\tadd %1, %0\n\tadd %1, %0" : "+b"(x) : "r"(eight));
	double t2 = now();
	for (long i = 0; i < N; i++)            /* push／pop：也是在改 rsp，再加上存取記憶體 */
		asm volatile("push %%rax\n\tpush %%rax\n\tpop %%rax\n\tpop %%rax" ::: "rax", "memory");
	double t3 = now();
	for (long i = 0; i < N; i++)            /* 角落情況：rsp 被當一般暫存器讀寫（mov 到別的暫存器再寫回） */
		asm volatile("sub $8, %%rsp\n\tmov %%rsp, %%rcx\n\tmov %%rcx, %%rsp\n\tadd $8, %%rsp" ::: "rcx", "memory");
	double t4 = now();
	for (long i = 0; i < N; i++)            /* 對照：rbx 加減常數 */
		asm volatile("sub $8, %0\n\tsub $8, %0\n\tadd $8, %0\n\tadd $8, %0" : "+b"(x));
	double t5 = now();
	printf("sub/add rsp（常數）    ：%.3f ns/條\n", (t1 - t0) / N / 4 * 1e9);
	printf("sub/add rbx（暫存器）  ：%.3f ns/條（%ld）\n", (t2 - t1) / N / 4 * 1e9, x);
	printf("sub/add rbx（常數）    ：%.3f ns/條（%ld）\n", (t5 - t4) / N / 4 * 1e9, x);
	printf("push/pop               ：%.3f ns/條\n", (t3 - t2) / N / 4 * 1e9);
	printf("sub、mov rsp↔rcx、add  ：%.3f ns/條\n", (t4 - t3) / N / 4 * 1e9);
	return 0;
}
