#include <stdio.h>
int ex60(void), ex67_q(void), ex67_r(void), prime(int), fact74(int), fact78(int), cf_after_sub(void);
unsigned shifts_sal(void), shifts_sar(void), shifts_shr(void);
int main(void)
{
	int r = ex60();
	printf("Ex60: ebx = %d, esp moved by %d bytes\n", r & 0xffff, r >> 16);
	printf("Ex67: -50 / 3 -> quotient %d, remainder %d\n", ex67_q(), ex67_r());
	printf("Ex68-70: sal 0x%08x  sar 0x%08x  shr 0x%08x\n", shifts_sal(), shifts_sar(), shifts_shr());
	printf("Ex72: prime(97)=%d prime(91)=%d prime(11)=%d\n", prime(97), prime(91), prime(11));
	printf("Ex74: 10! = %d   Ex78: 10! = %d\n", fact74(10), fact78(10));
	printf("x86: CF after (0 - 5) = %d\n", cf_after_sub());
	return 0;
}
