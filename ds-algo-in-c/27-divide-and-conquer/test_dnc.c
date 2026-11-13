#include "dnc.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void test_matmul(void)
{
    srand(27);
    for (int n = 1; n <= 128; n *= 2) {
        size_t sz = (size_t)n * n;
        long long *A = malloc(sz * sizeof *A), *B = malloc(sz * sizeof *B);
        long long *C1 = malloc(sz * sizeof *C1), *C2 = malloc(sz * sizeof *C2), *C3 = malloc(sz * sizeof *C3);
        for (size_t i = 0; i < sz; i++) {
            A[i] = rand() % 21 - 10;
            B[i] = rand() % 21 - 10;
        }
        matmul_naive(A, B, C1, n);
        matmul_recursive(A, B, C2, n);
        strassen(A, B, C3, n);
        assert(memcmp(C1, C2, sz * sizeof *C1) == 0);
        assert(memcmp(C1, C3, sz * sizeof *C1) == 0);
        free(A);
        free(B);
        free(C1);
        free(C2);
        free(C3);
    }
}

static void test_max_subarray(void)
{
    /* CLRS 第 3 版 Figure 4.3 的股價差：答案是 A[7..10] = 18 + 20 - 7 + 12 = 43 */
    int a[] = {13, -3, -25, 20, -3, -16, -23, 18, 20, -7, 12, -5, -22, 15, -4, 7};
    int lo, hi;
    assert(max_subarray_dc(a, 16, &lo, &hi) == 43 && lo == 7 && hi == 10);

    srand(270);
    for (int t = 0; t < 2000; t++) {
        int n = 1 + rand() % 40, x[40];
        for (int i = 0; i < n; i++)
            x[i] = rand() % 21 - 10;
        long long best = x[0];
        for (int i = 0; i < n; i++) {
            long long s = 0;
            for (int j = i; j < n; j++) {
                s += x[j];
                if (s > best)
                    best = s;
            }
        }
        long long got = max_subarray_dc(x, n, &lo, &hi);
        assert(got == best && 0 <= lo && lo <= hi && hi < n);
        long long s = 0;
        for (int i = lo; i <= hi; i++)
            s += x[i];
        assert(s == got); /* 回報的範圍加起來真的等於答案 */
    }
}

static void test_majority(void)
{
    srand(2727);
    for (int t = 0; t < 2000; t++) {
        int n = 1 + rand() % 50, x[50], m = rand() % 5;
        int k = n / 2 + 1 + rand() % (n - n / 2); /* 放 k > n/2 個 m */
        for (int i = 0; i < n; i++)
            x[i] = i < k ? m : 5 + rand() % 5;
        for (int i = n - 1; i > 0; i--) { /* 打亂 */
            int j = rand() % (i + 1), tmp = x[i];
            x[i] = x[j];
            x[j] = tmp;
        }
        assert(majority_dc(x, n) == m);
    }
}

int main(void)
{
    test_matmul();
    test_max_subarray();
    test_majority();
    puts("dnc: all tests passed");
    return 0;
}
