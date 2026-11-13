/* LeetCode 699 · Falling Squares
 * 正方形一個一個從上面掉下來（左邊界 left、邊長 side），落在 [left, left+side) 範圍內最高的地方。
 * 每掉一個之後，回傳目前整體的最高高度。
 * 思路：線段樹，支援「區間最大值」和「區間設值」（懶標記）。
 *   新方塊的底 = [left, left+side) 目前的最大高度；頂 = 底 + side；把這段設成頂。
 *   座標到 10^8，但最多只有 2000 個方塊 → 座標離散化 (coordinate compression) 成最多 4000 個點。
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/* ===== 提交範圍 開始 ===== */
static int *mx, *tag; /* tag = 懶標記：這整段要被設成多少（0 = 沒有） */

static void push(int k)
{
    if (tag[k]) {
        mx[2 * k] = mx[2 * k + 1] = tag[k];
        tag[2 * k] = tag[2 * k + 1] = tag[k];
        tag[k] = 0;
    }
}

static int query(int k, int lo, int hi, int l, int r)
{
    if (r < lo || hi < l)
        return 0;
    if (l <= lo && hi <= r)
        return mx[k];
    push(k);
    int mid = (lo + hi) / 2, a = query(2 * k, lo, mid, l, r), b = query(2 * k + 1, mid + 1, hi, l, r);
    return a > b ? a : b;
}

static void assign(int k, int lo, int hi, int l, int r, int v)
{
    if (r < lo || hi < l)
        return;
    if (l <= lo && hi <= r) {
        mx[k] = tag[k] = v;
        return;
    }
    push(k);
    int mid = (lo + hi) / 2;
    assign(2 * k, lo, mid, l, r, v);
    assign(2 * k + 1, mid + 1, hi, l, r, v);
    mx[k] = mx[2 * k] > mx[2 * k + 1] ? mx[2 * k] : mx[2 * k + 1];
}

static int cmp_int(const void *a, const void *b)
{
    int x = *(const int *)a, y = *(const int *)b;
    return (x > y) - (x < y);
}

static int index_of(const int *xs, int m, int v) /* 二分找 v 在排序後座標裡的位置 */
{
    int lo = 0, hi = m;
    while (lo < hi) {
        int mid = (lo + hi) / 2;
        if (xs[mid] < v)
            lo = mid + 1;
        else
            hi = mid;
    }
    return lo;
}

int *fallingSquares(int **positions, int positionsSize, int *positionsColSize, int *returnSize)
{
    (void)positionsColSize;
    int n = positionsSize, m = 0;
    int *xs = malloc((size_t)2 * n * sizeof *xs);
    for (int i = 0; i < n; i++) { /* 方塊蓋住的格子是 [left, left+side-1]（閉區間） */
        xs[m++] = positions[i][0];
        xs[m++] = positions[i][0] + positions[i][1] - 1;
    }
    qsort(xs, (size_t)m, sizeof *xs, cmp_int);
    int u = 0;
    for (int i = 0; i < m; i++) /* 去重 */
        if (u == 0 || xs[i] != xs[u - 1])
            xs[u++] = xs[i];
    mx = calloc((size_t)4 * u, sizeof *mx);
    tag = calloc((size_t)4 * u, sizeof *tag);
    int *ans = malloc((size_t)n * sizeof *ans), best = 0;
    for (int i = 0; i < n; i++) {
        int l = index_of(xs, u, positions[i][0]);
        int r = index_of(xs, u, positions[i][0] + positions[i][1] - 1);
        int top = query(1, 0, u - 1, l, r) + positions[i][1];
        assign(1, 0, u - 1, l, r, top);
        if (top > best)
            best = top;
        ans[i] = best;
    }
    free(xs);
    free(mx);
    free(tag);
    *returnSize = n;
    return ans;
}
/* ===== 提交範圍 結束 ===== */

static void check(int p[][2], int n, const int *want)
{
    int *rows[64], cols[64], k;
    for (int i = 0; i < n; i++) {
        rows[i] = p[i];
        cols[i] = 2;
    }
    int *r = fallingSquares(rows, n, cols, &k);
    assert(k == n);
    for (int i = 0; i < n; i++)
        assert(r[i] == want[i]);
    free(r);
}

int main(void)
{
    int a[][2] = {{1, 2}, {2, 3}, {6, 1}};
    int wa[] = {2, 5, 5};
    check(a, 3, wa);
    int b[][2] = {{100, 100}, {200, 100}}; /* 只碰到邊不算疊上去 */
    int wb[] = {100, 100};
    check(b, 2, wb);

    srand(699);
    for (int t = 0; t < 300; t++) { /* 對照暴力：直接用陣列記每一格的高度 */
        int n = 1 + rand() % 30, p[30][2], h[60] = {0}, want[30], best = 0;
        for (int i = 0; i < n; i++) {
            p[i][0] = rand() % 40;
            p[i][1] = 1 + rand() % 10;
            int base = 0;
            for (int x = p[i][0]; x < p[i][0] + p[i][1]; x++)
                if (h[x] > base)
                    base = h[x];
            for (int x = p[i][0]; x < p[i][0] + p[i][1]; x++)
                h[x] = base + p[i][1];
            if (base + p[i][1] > best)
                best = base + p[i][1];
            want[i] = best;
        }
        check(p, n, want);
    }
    puts("0699: passed");
    return 0;
}
