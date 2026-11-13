/* LeetCode 933 · Number of Recent Calls
 * 思路：queue 存每次 ping 的時間。新的 ping 進來後，把 < t - 3000 的舊時間從前面丟掉，
 *       剩下的個數就是答案。t 保證遞增，所以舊的一定在前面。
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/* ===== 提交範圍 開始 ===== */
#define MAXCALLS 10000 /* 題目：最多 10^4 次 ping */

typedef struct {
    int t[MAXCALLS];
    int head, tail;
} RecentCounter;

RecentCounter *recentCounterCreate(void)
{
    return calloc(1, sizeof(RecentCounter));
}

int recentCounterPing(RecentCounter *obj, int t)
{
    obj->t[obj->tail++] = t;
    while (obj->t[obj->head] < t - 3000)
        obj->head++;
    return obj->tail - obj->head;
}

void recentCounterFree(RecentCounter *obj)
{
    free(obj);
}
/* ===== 提交範圍 結束 ===== */

int main(void)
{
    RecentCounter *rc = recentCounterCreate();
    assert(recentCounterPing(rc, 1) == 1);
    assert(recentCounterPing(rc, 100) == 2);
    assert(recentCounterPing(rc, 3001) == 3);
    assert(recentCounterPing(rc, 3002) == 3); /* 1 被丟掉 */
    assert(recentCounterPing(rc, 10000) == 1);
    recentCounterFree(rc);
    puts("0933: passed");
    return 0;
}
