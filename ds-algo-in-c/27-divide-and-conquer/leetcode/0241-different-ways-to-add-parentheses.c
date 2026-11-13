/* LeetCode 241 · Different Ways to Add Parentheses
 * 算式像 "2*3-4*5"，用所有可能的加括號方式計算，回傳全部的結果（可以重複）。
 * 思路（分治）：最後一個被計算的運算子把算式切成左右兩段。
 *   對每個運算子 op：左邊所有可能的值 × 右邊所有可能的值，用 op 兩兩組合。
 *   沒有運算子 → 就是一個數字（base case）。
 *   結果的個數是 Catalan 數，本身就是指數級的。
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/* ===== 提交範圍 開始 ===== */
typedef struct {
    int *v;
    int n, cap;
} List;

static void push(List *l, int x)
{
    if (l->n == l->cap) {
        l->cap = l->cap ? l->cap * 2 : 4;
        l->v = realloc(l->v, (size_t)l->cap * sizeof *l->v);
    }
    l->v[l->n++] = x;
}

static List solve(const char *s, int lo, int hi) /* [lo, hi) */
{
    List res = {NULL, 0, 0};
    for (int i = lo; i < hi; i++) {
        char op = s[i];
        if (op != '+' && op != '-' && op != '*')
            continue;
        List L = solve(s, lo, i), R = solve(s, i + 1, hi); /* 以 op 為最後一步，切成兩半 */
        for (int a = 0; a < L.n; a++)
            for (int b = 0; b < R.n; b++)
                push(&res, op == '+' ? L.v[a] + R.v[b] : op == '-' ? L.v[a] - R.v[b] : L.v[a] * R.v[b]);
        free(L.v);
        free(R.v);
    }
    if (res.n == 0) { /* 沒有運算子：整段是一個數字 */
        int x = 0;
        for (int i = lo; i < hi; i++)
            x = x * 10 + (s[i] - '0');
        push(&res, x);
    }
    return res;
}

int *diffWaysToCompute(char *expression, int *returnSize)
{
    int n = 0;
    while (expression[n])
        n++;
    List r = solve(expression, 0, n);
    *returnSize = r.n;
    return r.v;
}
/* ===== 提交範圍 結束 ===== */

static int cmp_int(const void *x, const void *y)
{
    int a = *(const int *)x, b = *(const int *)y;
    return (a > b) - (a < b);
}

static void check(char *e, const int *want, int k)
{
    int m;
    int *r = diffWaysToCompute(e, &m);
    assert(m == k);
    qsort(r, (size_t)m, sizeof *r, cmp_int);
    for (int i = 0; i < k; i++)
        assert(r[i] == want[i]);
    free(r);
}

int main(void)
{
    int a[] = {0, 2};
    check("2-1-1", a, 2); /* (2-1)-1 = 0, 2-(1-1) = 2 */
    int b[] = {-34, -14, -10, -10, 10};
    check("2*3-4*5", b, 5);
    int c[] = {42};
    check("42", c, 1);
    puts("0241: passed");
    return 0;
}
