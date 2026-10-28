#include <stdio.h>
#include <float.h>
int main(void)
{
	printf("sizeof(long double) = %zu bytes, mantissa = %d bits\n", sizeof(long double), LDBL_MANT_DIG);
	printf("double mantissa = %d bits\n", DBL_MANT_DIG);
	long double x = 1.0L / 3;
	double d = 1.0 / 3;
	printf("1/3 long double = %.25Lf\n1/3 double      = %.25f\n", x, d);
	return 0;
}
