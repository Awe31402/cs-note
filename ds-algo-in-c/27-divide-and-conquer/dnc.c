#include "dnc.h"

#include <stdlib.h>
#include <string.h>

void matmul_naive(const long long *A, const long long *B, long long *C, int n)
{
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++) {
            long long s = 0;
            for (int k = 0; k < n; k++)
                s += A[i * n + k] * B[k * n + j];
            C[i * n + j] = s;
        }
}

/* ---- 遞迴版：直接在大矩陣上用「子矩陣視窗」，每塊用 (起點指標, 列寬 stride) 表示 ---- */

static void rec(const long long *A, int sa, const long long *B, int sb, long long *C, int sc, int n)
{
    if (n == 1) {
        C[0] += A[0] * B[0];
        return;
    }
    int h = n / 2;
    /* A11 = A、A12 = A + h、A21 = A + h*sa、A22 = A + h*sa + h，B、C 同理 */
    const long long *A11 = A, *A12 = A + h, *A21 = A + h * sa, *A22 = A + h * sa + h;
    const long long *B11 = B, *B12 = B + h, *B21 = B + h * sb, *B22 = B + h * sb + h;
    long long *C11 = C, *C12 = C + h, *C21 = C + h * sc, *C22 = C + h * sc + h;
    /* C11 = A11·B11 + A12·B21，其他三塊類似：一共 8 次遞迴 */
    rec(A11, sa, B11, sb, C11, sc, h);
    rec(A12, sa, B21, sb, C11, sc, h);
    rec(A11, sa, B12, sb, C12, sc, h);
    rec(A12, sa, B22, sb, C12, sc, h);
    rec(A21, sa, B11, sb, C21, sc, h);
    rec(A22, sa, B21, sb, C21, sc, h);
    rec(A21, sa, B12, sb, C22, sc, h);
    rec(A22, sa, B22, sb, C22, sc, h);
}

void matmul_recursive(const long long *A, const long long *B, long long *C, int n)
{
    memset(C, 0, (size_t)n * n * sizeof *C);
    rec(A, n, B, n, C, n, n);
}

/* ---- Strassen ---- */

static void add(const long long *X, const long long *Y, long long *Z, int n, int sign)
{
    for (int i = 0; i < n * n; i++)
        Z[i] = X[i] + sign * Y[i];
}

/* 取出子矩陣 (r, c) 那一塊，複製成連續的 h×h 矩陣 */
static void take(const long long *M, int n, int r, int c, long long *out)
{
    int h = n / 2;
    for (int i = 0; i < h; i++)
        memcpy(out + i * h, M + (r * h + i) * n + c * h, (size_t)h * sizeof *out);
}

static void put(long long *M, int n, int r, int c, const long long *in)
{
    int h = n / 2;
    for (int i = 0; i < h; i++)
        memcpy(M + (r * h + i) * n + c * h, in + i * h, (size_t)h * sizeof *in);
}

void strassen(const long long *A, const long long *B, long long *C, int n)
{
    if (n <= 32) { /* 小矩陣用三層迴圈比較快：遞迴和配置記憶體的成本太高 */
        matmul_naive(A, B, C, n);
        return;
    }
    int h = n / 2;
    size_t sz = (size_t)h * h;
    long long *buf = malloc(21 * sz * sizeof *buf);
    if (!buf)
        abort();
    long long *a11 = buf, *a12 = a11 + sz, *a21 = a12 + sz, *a22 = a21 + sz;
    long long *b11 = a22 + sz, *b12 = b11 + sz, *b21 = b12 + sz, *b22 = b21 + sz;
    long long *S1 = b22 + sz, *S2 = S1 + sz; /* 暫存兩個加總 */
    long long *P[7];
    P[0] = S2 + sz;
    for (int i = 1; i < 7; i++)
        P[i] = P[i - 1] + sz;
    long long *T = P[6] + sz, *U = T + sz;
    take(A, n, 0, 0, a11);
    take(A, n, 0, 1, a12);
    take(A, n, 1, 0, a21);
    take(A, n, 1, 1, a22);
    take(B, n, 0, 0, b11);
    take(B, n, 0, 1, b12);
    take(B, n, 1, 0, b21);
    take(B, n, 1, 1, b22);

    /* 7 個乘積（CLRS 4.2 的 P1..P7） */
    add(b12, b22, S1, h, -1);                               /* S1 = B12 - B22 */
    strassen(a11, S1, P[0], h);                             /* P1 = A11·S1 */
    add(a11, a12, S1, h, +1);                               /* S2 = A11 + A12 */
    strassen(S1, b22, P[1], h);                             /* P2 = S2·B22 */
    add(a21, a22, S1, h, +1);                               /* S3 = A21 + A22 */
    strassen(S1, b11, P[2], h);                             /* P3 = S3·B11 */
    add(b21, b11, S1, h, -1);                               /* S4 = B21 - B11 */
    strassen(a22, S1, P[3], h);                             /* P4 = A22·S4 */
    add(a11, a22, S1, h, +1);                               /* S5 = A11 + A22 */
    add(b11, b22, S2, h, +1);                               /* S6 = B11 + B22 */
    strassen(S1, S2, P[4], h);                              /* P5 = S5·S6 */
    add(a12, a22, S1, h, -1);                               /* S7 = A12 - A22 */
    add(b21, b22, S2, h, +1);                               /* S8 = B21 + B22 */
    strassen(S1, S2, P[5], h);                              /* P6 = S7·S8 */
    add(a11, a21, S1, h, -1);                               /* S9 = A11 - A21 */
    add(b11, b12, S2, h, +1);                               /* S10 = B11 + B12 */
    strassen(S1, S2, P[6], h);                              /* P7 = S9·S10 */

    /* 組回 C（只用加減法） */
    add(P[4], P[3], T, h, +1);
    add(T, P[1], U, h, -1);
    add(U, P[5], T, h, +1); /* C11 = P5 + P4 - P2 + P6 */
    put(C, n, 0, 0, T);
    add(P[0], P[1], T, h, +1); /* C12 = P1 + P2 */
    put(C, n, 0, 1, T);
    add(P[2], P[3], T, h, +1); /* C21 = P3 + P4 */
    put(C, n, 1, 0, T);
    add(P[4], P[0], T, h, +1);
    add(T, P[2], U, h, -1);
    add(U, P[6], T, h, -1); /* C22 = P5 + P1 - P3 - P7 */
    put(C, n, 1, 1, T);
    free(buf);
}

/* ---- 最大子陣列 ---- */

typedef struct {
    long long sum;
    int lo, hi;
} Range;

/* 跨過中點的最大子陣列：從 mid 往左延伸的最大和 + 從 mid+1 往右延伸的最大和。Θ(n)。 */
static Range crossing(const int *a, int lo, int mid, int hi)
{
    long long s = 0, left = a[mid], right = a[mid + 1];
    int l = mid, r = mid + 1;
    for (int i = mid; i >= lo; i--) {
        s += a[i];
        if (s > left) {
            left = s;
            l = i;
        }
    }
    s = 0;
    for (int j = mid + 1; j <= hi; j++) {
        s += a[j];
        if (s > right) {
            right = s;
            r = j;
        }
    }
    return (Range){left + right, l, r};
}

static Range msub(const int *a, int lo, int hi)
{
    if (lo == hi)
        return (Range){a[lo], lo, hi};
    int mid = lo + (hi - lo) / 2;
    Range L = msub(a, lo, mid), R = msub(a, mid + 1, hi), C = crossing(a, lo, mid, hi);
    /* 答案只有三種可能：全在左半、全在右半、跨過中點 */
    if (L.sum >= R.sum && L.sum >= C.sum)
        return L;
    if (R.sum >= C.sum)
        return R;
    return C;
}

long long max_subarray_dc(const int *a, int n, int *lo, int *hi)
{
    Range r = msub(a, 0, n - 1);
    *lo = r.lo;
    *hi = r.hi;
    return r.sum;
}

/* ---- 多數元素 ---- */

static int count_in(const int *a, int lo, int hi, int x)
{
    int c = 0;
    for (int i = lo; i <= hi; i++)
        c += a[i] == x;
    return c;
}

static int maj(const int *a, int lo, int hi)
{
    if (lo == hi)
        return a[lo];
    int mid = lo + (hi - lo) / 2;
    int l = maj(a, lo, mid), r = maj(a, mid + 1, hi);
    if (l == r)
        return l;
    /* 整段的多數元素，一定是某一半的多數元素（否則兩半都不超過一半，加起來也不會超過） */
    return count_in(a, lo, hi, l) > count_in(a, lo, hi, r) ? l : r;
}

int majority_dc(const int *a, int n)
{
    return maj(a, 0, n - 1);
}
