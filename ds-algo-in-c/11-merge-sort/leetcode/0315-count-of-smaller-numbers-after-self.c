/* LeetCode 315 · Count of Smaller Numbers After Self
 * 思路：merge sort 排「index」。合併時，左半的元素 x 被放下去之前，
 *       右半已經有 j - mid 個元素被放下去了 —— 它們都比 x 小，而且原本都在 x 的右邊。
 *       所以 count[x 的原 index] += j - mid。O(n log n)。
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ===== 提交範圍 開始 ===== */
static void sort_rec(const int *v, int *idx, int *tmp, int *cnt, int lo, int hi)
{
    if (hi - lo < 2)
        return;
    int mid = lo + (hi - lo) / 2;
    sort_rec(v, idx, tmp, cnt, lo, mid);
    sort_rec(v, idx, tmp, cnt, mid, hi);
    int i = lo, j = mid, k = lo;
    while (i < mid && j < hi) {
        if (v[idx[j]] < v[idx[i]]) { /* 嚴格小於才算「更小」；相等時左邊先放 */
            tmp[k++] = idx[j++];
        } else {
            cnt[idx[i]] += j - mid;
            tmp[k++] = idx[i++];
        }
    }
    while (i < mid) {
        cnt[idx[i]] += j - mid; /* 右半全部都放完了，都比它小 */
        tmp[k++] = idx[i++];
    }
    while (j < hi)
        tmp[k++] = idx[j++];
    memcpy(idx + lo, tmp + lo, (size_t)(hi - lo) * sizeof *idx);
}

int *countSmaller(int *nums, int numsSize, int *returnSize)
{
    int n = numsSize;
    int *idx = malloc((size_t)n * sizeof *idx), *tmp = malloc((size_t)n * sizeof *tmp);
    int *cnt = calloc((size_t)n, sizeof *cnt);
    for (int i = 0; i < n; i++)
        idx[i] = i; /* 排 index 而不是值：才知道要加到誰的計數 */
    sort_rec(nums, idx, tmp, cnt, 0, n);
    free(idx);
    free(tmp);
    *returnSize = n;
    return cnt;
}
/* ===== 提交範圍 結束 ===== */

int main(void)
{
    int a[] = {5, 2, 6, 1}, wa[] = {2, 1, 1, 0}, m;
    int *r = countSmaller(a, 4, &m);
    assert(m == 4 && memcmp(r, wa, sizeof wa) == 0);
    free(r);
    int b[] = {-1, -1}, wb[] = {0, 0};
    r = countSmaller(b, 2, &m);
    assert(memcmp(r, wb, sizeof wb) == 0);
    free(r);
    srand(315);
    for (int t = 0; t < 300; t++) {
        int n = 1 + rand() % 60, x[60];
        for (int i = 0; i < n; i++)
            x[i] = rand() % 15 - 7;
        r = countSmaller(x, n, &m);
        for (int i = 0; i < n; i++) {
            int c = 0;
            for (int j = i + 1; j < n; j++)
                c += x[j] < x[i];
            assert(r[i] == c);
        }
        free(r);
    }
    puts("0315: passed");
    return 0;
}
