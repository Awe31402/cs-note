// precise.c：課本 p.56 的例子 mov r4,1 / div / sub r4,r4,1，在 x86 上真的除以 0。
// 在 SIGFPE handler 裡看：r12（課本的 r4）是不是還是 1？div 後面的 100 條 add 有沒有做？
#define _GNU_SOURCE
#include <signal.h>
#include <stdio.h>
#include <ucontext.h>

extern char div_insn[];
static volatile int hits;

static void handler(int sig, siginfo_t *si, void *ctx)
{
	greg_t *r = ((ucontext_t *)ctx)->uc_mcontext.gregs;
	printf("SIGFPE：RIP %s div 指令，r12 = %lld，r13 = %lld\n",
	       (char *)r[REG_RIP] == div_insn ? "正好指著" : "沒有指著", r[REG_R12], r[REG_R13]);
	r[REG_RCX] = 1;			/* 把除數改成 1，回去重做 div（stop → 處理 → resume） */
	hits++;
}

int main(void)
{
	struct sigaction sa = { .sa_sigaction = handler, .sa_flags = SA_SIGINFO };
	sigaction(SIGFPE, &sa, NULL);
	register long r12 asm("r12"), r13 asm("r13");
	asm volatile("mov $1, %%r12\n\t"		/* 1: mov r4, 1 */
		     "xor %%r13, %%r13\n\t"
		     "mov $100, %%rax\n\txor %%edx, %%edx\n\txor %%ecx, %%ecx\n\t"
		     ".globl div_insn\n"
		     "div_insn: div %%rcx\n\t"		/* 2: div（除以 0） */
		     "sub $1, %%r12\n\t"		/* 3: sub r4, r4, 1 */
		     ".rept 100\n\tadd $1, %%r13\n\t.endr"	/* 跟 div 無關的 100 條 add */
		     : "=r"(r12), "=r"(r13) :: "rax", "rcx", "rdx");
	printf("回到程式之後：r12 = %ld（sub 只做了一次），r13 = %ld，handler 進了 %d 次\n", r12, r13, hits);
	return 0;
}
