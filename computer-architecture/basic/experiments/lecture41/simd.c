#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <immintrin.h>
/* 課本 Example 155（SSE 加兩個 float 陣列）和 §12.5.4（向量的 if：比較 → 遮罩），對照一般的迴圈。
 * 編譯時用 -fno-tree-vectorize，免得 gcc 自己把一般迴圈也變成向量指令 */
#define N 4096                                  /* 3 個陣列共 48 KB，放得進 L1／L2，量的是運算不是記憶體 */
#define REP 50000
static float a[N] __attribute__((aligned(32))), b[N] __attribute__((aligned(32))), c[N] __attribute__((aligned(32)));

static void scalarAdd(const float *a, const float *b, float *c, int n) { for (int i = 0; i < n; i++) c[i] = a[i] + b[i]; }
static void sseAdd(const float *a, const float *b, float *c, int n)
{
	int numIters = n / 4;                        /* strip mining */
	for (int i = 0; i < numIters; i++) {
		__m128 val1 = _mm_load_ps(a), val2 = _mm_load_ps(b);
		__m128 res = _mm_add_ps(val1, val2);
		_mm_store_ps(c, res);
		a += 4; b += 4; c += 4;
	}
}
static void avxAdd(const float *a, const float *b, float *c, int n)
{
	for (int i = 0; i < n; i += 8) _mm256_store_ps(c + i, _mm256_add_ps(_mm256_load_ps(a + i), _mm256_load_ps(b + i)));
}
/* if (x < 10) x = x + 10 */
static void scalarIf(float *x, int n) { for (int i = 0; i < n; i++) if (x[i] < 10) x[i] += 10; }
static void ssePred(float *x, int n)
{
	__m128 ten = _mm_set1_ps(10);
	for (int i = 0; i < n; i += 4) {
		__m128 v = _mm_load_ps(x + i);
		__m128 mask = _mm_cmplt_ps(v, ten);                     /* vector compare：每格 x < 10 就全 1 */
		v = _mm_add_ps(v, _mm_and_ps(mask, ten));               /* 「predicated add」：只有 mask 為真的格子加 10 */
		_mm_store_ps(x + i, v);
	}
}
static double ms(void (*f)(const float *, const float *, float *, int))
{
	struct timespec t0, t1; clock_gettime(CLOCK_MONOTONIC, &t0);
	for (int r = 0; r < REP; r++) { f(a, b, c, N); asm volatile("" ::: "memory"); }
	clock_gettime(CLOCK_MONOTONIC, &t1);
	return (t1.tv_sec - t0.tv_sec) * 1e3 + (t1.tv_nsec - t0.tv_nsec) / 1e6;
}
static double msIf(void (*f)(float *, int), int randomData, float *check)
{
	static float x[N] __attribute__((aligned(32)));
	struct timespec t0, t1; double total = 0;
	srand(1);
	for (int r = 0; r < 2000; r++) {
		for (int i = 0; i < N; i++) x[i] = randomData ? rand() % 20 : i % 20;   /* 0–19：一半 < 10 */
		clock_gettime(CLOCK_MONOTONIC, &t0);
		f(x, N); asm volatile("" ::: "memory");
		clock_gettime(CLOCK_MONOTONIC, &t1);
		total += (t1.tv_sec - t0.tv_sec) * 1e3 + (t1.tv_nsec - t0.tv_nsec) / 1e6;
	}
	*check = 0; for (int i = 0; i < N; i++) *check += x[i];
	return total;
}
int main(void)
{
	for (int i = 0; i < N; i++) { a[i] = i * 0.5f; b[i] = i * 0.25f; }
	double ts = ms(scalarAdd), t4 = ms(sseAdd), t8 = ms(avxAdd);
	printf("add  scalar %6.1f ms | SSE (4 floats) %6.1f ms  x%.2f | AVX (8 floats) %6.1f ms  x%.2f   c[100]=%g\n",
	       ts, t4, ts / t4, t8, ts / t8, c[100]);
	for (int rnd = 0; rnd < 2; rnd++) {
		float s1, s2; double a1 = msIf(scalarIf, rnd, &s1), a2 = msIf(ssePred, rnd, &s2);
		printf("if (x<10) x+=10, %s data: scalar %6.1f ms | SSE compare+mask %5.1f ms  x%.1f   (checksums %g %g)\n",
		       rnd ? "random " : "pattern", a1, a2, a1 / a2, s1, s2);
	}
	return 0;
}
