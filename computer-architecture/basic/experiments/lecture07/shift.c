#include <stdio.h>
int div2(int x) { return x / 2; }
int sar1(int x) { return x >> 1; }
int mul6(int x) { return x * 6; }
unsigned udiv2(unsigned x) { return x / 2; }
int main(void)
{
	for (int x = -7; x <= 7; x += 2)
		printf("x=%2d  x/2=%2d  x>>1=%2d\n", x, div2(x), sar1(x));
	return 0;
}
