/* LeetCode 134 · Gas Station
 * 環狀路線上 n 個加油站，站 i 可以加 gas[i]，開到下一站要花 cost[i]。從哪一站出發可以繞一圈？（答案唯一或 -1）
 * 思路：
 *   1. 總油量 < 總花費 → 一定不行
 *   2. 否則一定可以。從 0 開始累加油箱 tank，一旦 tank < 0（從 start 開不到 i+1），
 *      start 到 i 之間的任何一站都不可能是答案（它們出發時油箱只會比 start 出發時更少），
 *      所以直接把起點改成 i+1、油箱歸零。O(n)。
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/* ===== 提交範圍 開始 ===== */
int canCompleteCircuit(int *gas, int gasSize, int *cost, int costSize)
{
    (void)costSize;
    long long total = 0, tank = 0;
    int start = 0;
    for (int i = 0; i < gasSize; i++) {
        int diff = gas[i] - cost[i];
        total += diff;
        tank += diff;
        if (tank < 0) {
            start = i + 1;
            tank = 0;
        }
    }
    return total < 0 ? -1 : start;
}
/* ===== 提交範圍 結束 ===== */

static int brute(const int *g, const int *c, int n)
{
    for (int s = 0; s < n; s++) {
        long long tank = 0;
        int ok = 1;
        for (int k = 0; k < n && ok; k++) {
            int i = (s + k) % n;
            tank += g[i] - c[i];
            ok = tank >= 0;
        }
        if (ok)
            return s;
    }
    return -1;
}

int main(void)
{
    int g1[] = {1, 2, 3, 4, 5}, c1[] = {3, 4, 5, 1, 2};
    assert(canCompleteCircuit(g1, 5, c1, 5) == 3);
    int g2[] = {2, 3, 4}, c2[] = {3, 4, 3};
    assert(canCompleteCircuit(g2, 3, c2, 3) == -1);
    srand(134);
    for (int t = 0; t < 2000; t++) {
        int n = 1 + rand() % 12, g[12], c[12];
        for (int i = 0; i < n; i++) {
            g[i] = rand() % 6;
            c[i] = rand() % 6;
        }
        int want = brute(g, c, n);
        int got = canCompleteCircuit(g, n, c, n);
        /* 題目保證答案唯一；隨機資料可能有多個起點都行，只要檢查「有沒有」一致、而且選的起點真的可以 */
        assert((want < 0) == (got < 0));
        if (got >= 0) {
            long long tank = 0;
            for (int k = 0; k < n; k++) {
                int i = (got + k) % n;
                tank += g[i] - c[i];
                assert(tank >= 0);
            }
        }
    }
    puts("0134: passed");
    return 0;
}
