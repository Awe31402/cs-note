#include <stdio.h>
/* 指令就是記憶體裡的位元組：把函式當成資料讀出來 */
__attribute__((noinline)) int add(int x, int y) { return x + y; }

int main(void)
{
	const unsigned char *p = (const unsigned char *)add;
	printf("add() at %p:", (void *)p);
	for (int i = 0; i < 12; i++)
		printf(" %02x", p[i]);
	printf("\nadd(2, 3) = %d\n", add(2, 3));
	return 0;
}
