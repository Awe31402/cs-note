/* LeetCode 347 · Top K Frequent Elements
 * 思路：先數次數，再用大小為 k 的 min-heap（以「次數」比大小）留下前 k 名。
 * 題目範圍：-10^4 <= nums[i] <= 10^4，所以用陣列計數即可，不用 hash table。
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/* ===== 提交範圍 開始 ===== */
#define OFFSET 10000
#define RANGE (2 * OFFSET + 1)

/* heap 裡存的是「值 + OFFSET」，比大小時看 cnt[] */
static void sift_down(int *h, int n, int i, const int *cnt)
{
    for (;;) {
        int l = 2 * i + 1, r = l + 1, m = i;
        if (l < n && cnt[h[l]] < cnt[h[m]])
            m = l;
        if (r < n && cnt[h[r]] < cnt[h[m]])
            m = r;
        if (m == i)
            return;
        int t = h[i];
        h[i] = h[m];
        h[m] = t;
        i = m;
    }
}

static void sift_up(int *h, int i, const int *cnt)
{
    while (i > 0 && cnt[h[(i - 1) / 2]] > cnt[h[i]]) {
        int p = (i - 1) / 2;
        int t = h[i];
        h[i] = h[p];
        h[p] = t;
        i = p;
    }
}

int *topKFrequent(int *nums, int numsSize, int k, int *returnSize)
{
    int *cnt = calloc(RANGE, sizeof *cnt);
    for (int i = 0; i < numsSize; i++)
        cnt[nums[i] + OFFSET]++;

    int *h = malloc((size_t)k * sizeof *h);
    int size = 0;
    for (int v = 0; v < RANGE; v++) {
        if (cnt[v] == 0)
            continue;
        if (size < k) {
            h[size] = v;
            sift_up(h, size, cnt);
            size++;
        } else if (cnt[v] > cnt[h[0]]) {
            h[0] = v;
            sift_down(h, k, 0, cnt);
        }
    }
    for (int i = 0; i < size; i++)
        h[i] -= OFFSET;
    free(cnt);
    *returnSize = size;
    return h;
}
/* ===== 提交範圍 結束 ===== */

static int cmp_int(const void *x, const void *y)
{
    int a = *(const int *)x, b = *(const int *)y;
    return (a > b) - (a < b);
}

/* 答案順序不限，排序後再比 */
static void check(int *nums, int n, int k, int *want)
{
    int m;
    int *got = topKFrequent(nums, n, k, &m);
    assert(m == k);
    qsort(got, (size_t)m, sizeof *got, cmp_int);
    qsort(want, (size_t)k, sizeof *want, cmp_int);
    for (int i = 0; i < k; i++)
        assert(got[i] == want[i]);
    free(got);
}

int main(void)
{
    int a[] = {1, 1, 1, 2, 2, 3};
    int wa[] = {1, 2};
    check(a, 6, 2, wa);

    int b[] = {1};
    int wb[] = {1};
    check(b, 1, 1, wb);

    int c[] = {-10000, 10000, 10000, -10000, -10000, 5};
    int wc[] = {-10000, 10000};
    check(c, 6, 2, wc);

    puts("0347: passed");
    return 0;
}
