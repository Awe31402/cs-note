/* LeetCode 1947 · Maximum Compatibility Score Sum
 * m 個學生、m 個導師，每人回答 n 題 0/1 問卷。一對的分數 = 答案相同的題數。一對一配對，總分最大是多少？
 * 思路：這是「指派問題 (assignment problem)」= 二分圖的最大權重完美匹配。
 *   用匈牙利演算法 (Hungarian algorithm, CLRS 25.3)，O(m³)。m <= 8 時暴力 8! 也會過，
 *   但匈牙利演算法才是這類題目的標準解（m 到幾百也沒問題）。
 *   實作的是「最小成本」版本（頂點位能 u、v），所以把分數取負號變成成本。
 */
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

/* ===== 提交範圍 開始 ===== */
/* cost 是 (m+1)×(m+1)，1-based。回傳最小總成本。經典 O(m³) 寫法（位能 + 逐列加入） */
static int hungarian(int m, int cost[9][9])
{
    int u[9] = {0}, v[9] = {0}, p[9] = {0}, way[9] = {0};
    for (int i = 1; i <= m; i++) { /* 一次加入一個學生 i，找一條增廣路徑 */
        p[0] = i;
        int j0 = 0, minv[9], used[9] = {0};
        for (int j = 0; j <= m; j++)
            minv[j] = INT_MAX;
        do {
            used[j0] = 1;
            int i0 = p[j0], delta = INT_MAX, j1 = 0;
            for (int j = 1; j <= m; j++)
                if (!used[j]) {
                    int cur = cost[i0][j] - u[i0] - v[j]; /* 縮減成本 (reduced cost) */
                    if (cur < minv[j]) {
                        minv[j] = cur;
                        way[j] = j0;
                    }
                    if (minv[j] < delta) {
                        delta = minv[j];
                        j1 = j;
                    }
                }
            for (int j = 0; j <= m; j++) { /* 調整位能：讓至少一條新的邊變成「緊的」(reduced cost = 0) */
                if (used[j]) {
                    u[p[j]] += delta;
                    v[j] -= delta;
                } else {
                    minv[j] -= delta;
                }
            }
            j0 = j1;
        } while (p[j0] != 0); /* 找到一個還沒配對的導師 → 增廣路徑完成 */
        do { /* 沿著 way 把配對翻過來 */
            int j1 = way[j0];
            p[j0] = p[j1];
            j0 = j1;
        } while (j0);
    }
    int total = 0;
    for (int j = 1; j <= m; j++)
        total += cost[p[j]][j];
    return total;
}

int maxCompatibilitySum(int **students, int studentsSize, int *studentsColSize, int **mentors, int mentorsSize,
                        int *mentorsColSize)
{
    (void)mentorsSize;
    (void)mentorsColSize;
    int m = studentsSize, n = studentsColSize[0], cost[9][9] = {{0}};
    for (int i = 0; i < m; i++)
        for (int j = 0; j < m; j++) {
            int score = 0;
            for (int k = 0; k < n; k++)
                score += students[i][k] == mentors[j][k];
            cost[i + 1][j + 1] = -score; /* 最大化分數 = 最小化負分數 */
        }
    return -hungarian(m, cost);
}
/* ===== 提交範圍 結束 ===== */

static int best_perm(int m, int score[8][8], int i, int used)
{
    if (i == m)
        return 0;
    int best = -1;
    for (int j = 0; j < m; j++)
        if (!(used >> j & 1)) {
            int v = score[i][j] + best_perm(m, score, i + 1, used | 1 << j);
            if (v > best)
                best = v;
        }
    return best;
}

int main(void)
{
    int s0[] = {1, 1, 0}, s1[] = {1, 0, 1}, s2[] = {0, 0, 1};
    int t0[] = {1, 0, 0}, t1[] = {0, 0, 1}, t2[] = {1, 1, 0};
    int *st[] = {s0, s1, s2}, *mt[] = {t0, t1, t2}, cols[] = {3, 3, 3};
    assert(maxCompatibilitySum(st, 3, cols, mt, 3, cols) == 8);
    int z0[] = {0, 0}, z1[] = {0, 0}, z2[] = {0, 0}, o0[] = {1, 1}, o1[] = {1, 1}, o2[] = {1, 1};
    int *st2[] = {z0, z1, z2}, *mt2[] = {o0, o1, o2}, cols2[] = {2, 2, 2};
    assert(maxCompatibilitySum(st2, 3, cols2, mt2, 3, cols2) == 0);

    srand(1947);
    for (int t = 0; t < 500; t++) { /* 對照暴力枚舉所有排列 */
        int m = 1 + rand() % 8, n = 1 + rand() % 8, a[8][8], b[8][8], score[8][8];
        int *pa[8], *pb[8], cn[8];
        for (int i = 0; i < m; i++) {
            for (int k = 0; k < n; k++) {
                a[i][k] = rand() % 2;
                b[i][k] = rand() % 2;
            }
            pa[i] = a[i];
            pb[i] = b[i];
            cn[i] = n;
        }
        for (int i = 0; i < m; i++)
            for (int j = 0; j < m; j++) {
                score[i][j] = 0;
                for (int k = 0; k < n; k++)
                    score[i][j] += a[i][k] == b[j][k];
            }
        assert(maxCompatibilitySum(pa, m, cn, pb, m, cn) == best_perm(m, score, 0, 0));
    }
    puts("1947: passed");
    return 0;
}
