#include "complexity.h"

#include <assert.h>
#include <stdio.h>

/* ⌊log2 n⌋ + 1，也就是 n 的二進位位數 */
static long bit_length(long n)
{
    long b = 0;
    while (n > 0) {
        b++;
        n >>= 1;
    }
    return b;
}

static long isqrt(long n)
{
    long r = 0;
    while ((r + 1) * (r + 1) <= n)
        r++;
    return r;
}

static void test_loop_counts(void)
{
    int ns[] = {0, 1, 2, 3, 7, 8, 9, 100, 1000, 1024, 1025};
    for (int k = 0; k < (int)(sizeof ns / sizeof ns[0]); k++) {
        long n = ns[k];
        assert(count_constant((int)n) == 100);
        assert(count_log((int)n) == bit_length(n));
        assert(count_sqrt((int)n) == isqrt(n));
        assert(count_linear((int)n) == n);
        assert(count_n_log_n((int)n) == n * bit_length(n));
        assert(count_triangular((int)n) == n * (n + 1) / 2);
        assert(count_quadratic((int)n) == n * n);
    }
}

/* n 變 2 倍時，各種成長速度的比值 */
static void test_growth_ratio(void)
{
    assert(count_linear(2000) == 2 * count_linear(1000));
    assert(count_quadratic(2000) == 4 * count_quadratic(1000));
    assert(count_log(2048) == count_log(1024) + 1); /* 翻倍只多 1 次 */
}

static void test_amortized(void)
{
    for (int n = 1; n <= 5000; n++) {
        assert(dynarray_copies_double(n) < 2L * n);
        assert(dynarray_copies_plus1(n) == (long)n * (n - 1) / 2);
    }
    /* 剛好超過 2 的次方時最多：n = 1025 → 1+2+...+1024 = 2047 */
    assert(dynarray_copies_double(1025) == 2047);
}

int main(void)
{
    test_loop_counts();
    test_growth_ratio();
    test_amortized();
    puts("complexity: all tests passed");
    return 0;
}
