// work.c：拿來產生指令 trace 的小程式：qsort 一堆亂數、再做一點字串處理
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int cmp(const void *a, const void *b) { return *(const int *)a - *(const int *)b; }
int main(void)
{
	static int a[20000]; char buf[64]; long s = 0;
	srand(1);
	for (int i = 0; i < 20000; i++) a[i] = rand() % 100000;
	qsort(a, 20000, sizeof(int), cmp);
	for (int i = 0; i < 20000; i += 7) { snprintf(buf, sizeof buf, "%d", a[i]); s += strlen(buf); }
	printf("%ld\n", s);
	return 0;
}
