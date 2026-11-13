/* LeetCode 560 · Subarray Sum Equals K
 * 思路：前綴和 + hash map。
 *   sum(i..j) = P[j+1] - P[i]。要 = k，就是 P[i] = P[j+1] - k。
 *   由左往右走，用 map 記「每個前綴和出現過幾次」，查 cur - k 出現幾次就是以 j 結尾的答案數。
 *   有負數，所以不能用滑動視窗。O(n)。
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/* ===== 提交範圍 開始 ===== */
#define BITS 16 /* 最多 2×10^4 + 1 個不同前綴和 */
#define CAP (1 << BITS)

typedef struct {
    int key[CAP], cnt[CAP];
    char used[CAP];
} Map;

static unsigned find(const Map *m, int key)
{
    unsigned i = ((unsigned)key * 2654435761u) >> (32 - BITS);
    while (m->used[i] && m->key[i] != key)
        i = (i + 1) & (CAP - 1);
    return i;
}

int subarraySum(int *nums, int numsSize, int k)
{
    Map *m = calloc(1, sizeof *m);
    unsigned z = find(m, 0);
    m->used[z] = 1; /* 空前綴：和為 0 出現 1 次（讓「從頭開始」的子陣列也算到） */
    m->key[z] = 0;
    m->cnt[z] = 1;
    int cur = 0, ans = 0; /* |和| ≤ 2×10^4 × 1000 = 2×10^7，int 夠 */
    for (int i = 0; i < numsSize; i++) {
        cur += nums[i];
        unsigned s = find(m, cur - k);
        if (m->used[s])
            ans += m->cnt[s];
        s = find(m, cur);
        if (!m->used[s]) {
            m->used[s] = 1;
            m->key[s] = cur;
        }
        m->cnt[s]++;
    }
    free(m);
    return ans;
}
/* ===== 提交範圍 結束 ===== */

static int brute(const int *a, int n, int k)
{
    int c = 0;
    for (int i = 0; i < n; i++) {
        int s = 0;
        for (int j = i; j < n; j++) {
            s += a[j];
            c += s == k;
        }
    }
    return c;
}

int main(void)
{
    int a[] = {1, 1, 1};
    assert(subarraySum(a, 3, 2) == 2);
    int b[] = {1, 2, 3};
    assert(subarraySum(b, 3, 3) == 2);
    int c[] = {1, -1, 0};
    assert(subarraySum(c, 3, 0) == 3);
    srand(560);
    for (int t = 0; t < 300; t++) {
        int n = 1 + rand() % 60, x[60];
        for (int i = 0; i < n; i++)
            x[i] = rand() % 11 - 5;
        int k = rand() % 11 - 5;
        assert(subarraySum(x, n, k) == brute(x, n, k));
    }
    puts("0560: passed");
    return 0;
}
