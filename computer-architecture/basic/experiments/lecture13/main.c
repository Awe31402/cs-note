#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
int ex81(void); long copy_rep(int *, const int *, long); void copy_back(int *, const int *, long);
int fist_of(const double *); int fisttp_of(const double *);
void amean(int, int, double *); void gmean(int, int, double *);
int sin_check(float, float, int);

int main(void)
{
	printf("Example 81：eax = %d\n", ex81());
	int src[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}, dst[10] = {0};
	long ecx = copy_rep(dst, src, 10);
	printf("rep movsd：dst = ");
	for (int i = 0; i < 10; i++) printf("%d ", dst[i]);
	printf("，之後 ecx = %ld\n", ecx);
	int a[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
	memcpy(a + 2, a, 0);           /* 不做事，只是讓下面的對照清楚 */
	copy_back(a + 2, a, 8);        /* 重疊的往後搬：要從尾巴開始 */
	printf("std ＋ rep movsd（重疊、往後搬 2 格）：");
	for (int i = 0; i < 10; i++) printf("%d ", a[i]);
	printf("\n");
	double v[] = {2.7, 2.5, 3.5, -2.5, -2.7};
	for (int i = 0; i < 5; i++) printf("fist(%4.1f) = %2d   fisttp(%4.1f) = %2d\n", v[i], fist_of(&v[i]), v[i], fisttp_of(&v[i]));
	double m;
	amean(8, 9, &m); printf("amean(8, 9) = %g\n", m);
	gmean(8, 9, &m); printf("gmean(8, 9) = %.6f（sqrt(72) = %.6f）\n", m, sqrt(72));
	printf("sin 2θ vs 2 sinθ cosθ，θ=0.7，門檻 1e-5：ja → %d，jg → %d\n", sin_check(0.7f, 1e-5f, 0), sin_check(0.7f, 1e-5f, 1));
	printf("門檻 −1（門檻比差小，應該是 0）：         ja → %d，jg → %d\n", sin_check(0.7f, -1.0f, 0), sin_check(0.7f, -1.0f, 1));
	return 0;
}
