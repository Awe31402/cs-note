#include <stdio.h>
/* 單一指令 ISA：sbn a, b, c  →  mem[a] -= mem[b]; if (mem[a] < 0) goto c; */
enum { TEMP, INDEX, SUM, ONE, COUNTER, NVARS };
#define EXIT 0
struct ins { int a, b, c; };

static int run(const struct ins *prog, int n, int *mem)
{
	int pc = 1, steps = 0;
	while (pc != EXIT) {
		const struct ins *i = &prog[pc - 1];
		steps++;
		mem[i->a] -= mem[i->b];
		pc = (mem[i->a] < 0) ? i->c : (pc % n) + 1;
	}
	return steps;
}

int main(void)
{
	/* 課本 §1.6.5：7 行，counter = 9 控制迴圈 */
	struct ins book[] = {
		{TEMP, TEMP, 2}, {TEMP, INDEX, 3}, {SUM, TEMP, 4}, {INDEX, ONE, 5},
		{COUNTER, ONE, EXIT}, {TEMP, TEMP, 7}, {TEMP, ONE, 1},
	};
	/* 影片 [50:23]：6 行，index 減到負數就離開 */
	struct ins video[] = {
		{TEMP, TEMP, 2}, {TEMP, INDEX, 3}, {SUM, TEMP, 4}, {INDEX, ONE, EXIT},
		{TEMP, TEMP, 6}, {TEMP, ONE, 1},
	};
	int m1[NVARS] = { [INDEX] = 10, [ONE] = 1, [COUNTER] = 9 };
	int m2[NVARS] = { [INDEX] = 10, [ONE] = 1 };
	int s1 = run(book, 7, m1), s2 = run(video, 6, m2);
	printf("book : sum=%d  steps=%d\n", m1[SUM], s1);
	printf("video: sum=%d  steps=%d\n", m2[SUM], s2);
	return 0;
}
