#include <stdio.h>
#include <stdint.h>
/* 4-bit 2's complement：只留低 4 位 */
static int f4(int u) { return u & 0xf; }               /* F(u) = u mod 16 */
static int val4(int x) { return (x & 8) ? x - 16 : x; } /* 反過來解讀 */

static void add4(int u, int v)
{
	int r = f4(f4(u) + f4(v));
	int ovf = ((u < 0) == (v < 0)) && ((val4(r) < 0) != (u < 0) || val4(r) == 0) && u && v;
	printf("%3d + %3d : %X + %X = %X → %3d  %s\n", u, v, f4(u), f4(v), r, val4(r),
	       ovf ? "overflow" : "");
}

int main(void)
{
	add4(2, -4); add4(4, -3); add4(5, 5); add4(-5, -5); add4(-8, -8); add4(-3, -2);
	printf("2 * -3 : %X * %X = %X → %d\n", f4(2), f4(-3), f4(f4(2) * f4(-3)),
	       val4(f4(f4(2) * f4(-3))));
	int8_t m4 = -4;
	printf("-4 as int8 = 0x%02X, as int32 = 0x%08X\n", (uint8_t)m4, (uint32_t)(int32_t)m4);
	printf("~4 + 1 = %d\n", ~4 + 1);
	int s;
	printf("INT32_MAX + 1 overflow? %d\n", __builtin_add_overflow(INT32_MAX, 1, &s));
	return 0;
}
