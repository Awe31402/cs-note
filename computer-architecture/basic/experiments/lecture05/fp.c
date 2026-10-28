#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <float.h>
/* 印出 float 的 S | E | M 三個欄位 */
static void show(const char *name, float f)
{
	uint32_t u;
	memcpy(&u, &f, 4);
	printf("%-12s %08X  S=%u E=%3u M=0x%06X  (%g)\n", name, u, u >> 31, (u >> 23) & 0xff,
	       u & 0x7fffff, f);
}

int main(void)
{
	volatile float zero = 0.0f;
	show("-3.0", -3.0f);
	show("1.0", 1.0f);
	show("17.625", 17.625f);
	show("+0", 0.0f);
	show("-0", -0.0f);
	show("+inf", 1.0f / zero);
	show("nan", zero / zero);
	show("FLT_MIN", FLT_MIN);               /* 最小正 normal = 2^-126 */
	show("FLT_MAX", FLT_MAX);               /* 最大 normal = 2^128 - 2^104 */
	show("min denorm", ldexpf(1, -149));
	show("max denorm", ldexpf(1, -126) - ldexpf(1, -149));

	volatile float f = FLT_MIN, g = f / 2;
	printf("g = f/2 = %g, g == 0 ? %s\n", g, g == 0 ? "error" : "no");

	volatile float A = ldexpf(1, 50), B = ldexpf(1, 10);
	printf("float  (B+A)-A = %g,  B+(A-A) = %g\n", (B + A) - A, B + (A - A));
	volatile double Ad = A, Bd = B;
	printf("double (B+A)-A = %g\n", (Bd + Ad) - Ad);
	volatile float n = zero / zero;
	printf("nan == nan ? %d,  nan - nan = %g\n", n == n, n - n);
	printf("-0 == +0 ? %d\n", -0.0f == 0.0f);
	printf("1.11f = %.10f\n", 1.11f);
	return 0;
}
