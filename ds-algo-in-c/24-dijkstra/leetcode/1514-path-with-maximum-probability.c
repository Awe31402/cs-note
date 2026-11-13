/* LeetCode 1514 · Path with Maximum Probability
 * 無向圖，每條邊有成功機率。從 start 到 end，成功機率最大的路徑？
 * 思路：Dijkstra 的變形。「距離」換成「機率」，「最小」換成「最大」，「相加」換成「相乘」。
 *   為什麼可以：機率都在 [0, 1]，路越長只會越小（不會變大），就像非負權重的最短路徑。
 *   用 max-heap。O((V + E) log E)。
 */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

/* ===== 提交範圍 開始 ===== */
typedef struct {
    double p;
    int v;
} Item;

static void push(Item *h, int *n, Item x)
{
    int i = (*n)++;
    h[i] = x;
    while (i > 0 && h[(i - 1) / 2].p < h[i].p) { /* max-heap */
        Item t = h[i];
        h[i] = h[(i - 1) / 2];
        h[(i - 1) / 2] = t;
        i = (i - 1) / 2;
    }
}

static Item pop(Item *h, int *n)
{
    Item top = h[0];
    h[0] = h[--*n];
    for (int i = 0;;) {
        int l = 2 * i + 1, r = l + 1, s = i;
        if (l < *n && h[l].p > h[s].p)
            s = l;
        if (r < *n && h[r].p > h[s].p)
            s = r;
        if (s == i)
            break;
        Item t = h[i];
        h[i] = h[s];
        h[s] = t;
        i = s;
    }
    return top;
}

double maxProbability(int n, int **edges, int edgesSize, int *edgesColSize, double *succProb, int succProbSize,
                      int start_node, int end_node)
{
    (void)edgesColSize;
    (void)succProbSize;
    int m = edgesSize;
    int *start = calloc((size_t)n + 1, sizeof *start), *fill = calloc((size_t)n, sizeof *fill);
    int *to = malloc((size_t)(2 * m + 1) * sizeof *to);
    double *pr = malloc((size_t)(2 * m + 1) * sizeof *pr);
    for (int i = 0; i < m; i++) {
        start[edges[i][0] + 1]++;
        start[edges[i][1] + 1]++;
    }
    for (int v = 0; v < n; v++)
        start[v + 1] += start[v];
    for (int i = 0; i < m; i++) {
        int a = edges[i][0], b = edges[i][1];
        to[start[a] + fill[a]] = b;
        pr[start[a] + fill[a]++] = succProb[i];
        to[start[b] + fill[b]] = a;
        pr[start[b] + fill[b]++] = succProb[i];
    }
    double *best = calloc((size_t)n, sizeof *best); /* 0 = 還沒到過 */
    Item *h = malloc((size_t)(2 * m + 2) * sizeof *h);
    int hs = 0;
    best[start_node] = 1.0;
    push(h, &hs, (Item){1.0, start_node});
    double ans = 0.0;
    while (hs > 0) {
        Item it = pop(h, &hs);
        if (it.p < best[it.v])
            continue;
        if (it.v == end_node) {
            ans = it.p; /* 第一次拿出終點，機率就確定是最大的 */
            break;
        }
        for (int i = start[it.v]; i < start[it.v + 1]; i++) {
            double np = it.p * pr[i];
            if (np > best[to[i]]) {
                best[to[i]] = np;
                push(h, &hs, (Item){np, to[i]});
            }
        }
    }
    free(start);
    free(fill);
    free(to);
    free(pr);
    free(best);
    free(h);
    return ans;
}
/* ===== 提交範圍 結束 ===== */

static double run(int n, int e[][2], double *p, int m, int s, int t)
{
    int *rows[8], cols[8];
    for (int i = 0; i < m; i++) {
        rows[i] = e[i];
        cols[i] = 2;
    }
    return maxProbability(n, rows, m, cols, p, m, s, t);
}

int main(void)
{
    int e[][2] = {{0, 1}, {1, 2}, {0, 2}};
    double p1[] = {0.5, 0.5, 0.2}, p2[] = {0.5, 0.5, 0.3};
    assert(fabs(run(3, e, p1, 3, 0, 2) - 0.25) < 1e-9); /* 0→1→2：0.25 > 0.2 */
    assert(fabs(run(3, e, p2, 3, 0, 2) - 0.3) < 1e-9);
    int f[][2] = {{0, 1}};
    double p3[] = {0.5};
    assert(run(3, f, p3, 1, 0, 2) == 0.0); /* 走不到 */
    puts("1514: passed");
    return 0;
}
