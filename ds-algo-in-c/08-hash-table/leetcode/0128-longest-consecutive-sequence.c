/* LeetCode 128 · Longest Consecutive Sequence（要求 O(n)）
 * 思路：全部放進 hash set。只從「序列的起點」開始往上數（x - 1 不在 set 裡的 x）。
 *       每個數最多被「往上數」經過一次 → O(n)。
 */
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

/* ===== 提交範圍 開始 ===== */
typedef struct {
    int *keys;
    char *used;
    int bits;
} Set;

static unsigned slot(const Set *s, int key)
{
    return ((unsigned)key * 2654435761u) >> (32 - s->bits);
}

static bool set_has(const Set *s, int key)
{
    unsigned mask = (1u << s->bits) - 1;
    for (unsigned i = slot(s, key); s->used[i]; i = (i + 1) & mask)
        if (s->keys[i] == key)
            return true;
    return false;
}

static void set_add(Set *s, int key)
{
    unsigned mask = (1u << s->bits) - 1, i = slot(s, key);
    while (s->used[i] && s->keys[i] != key)
        i = (i + 1) & mask;
    s->used[i] = 1;
    s->keys[i] = key;
}

int longestConsecutive(int *nums, int numsSize)
{
    Set s;
    s.bits = 4;
    while ((1 << s.bits) < 2 * numsSize) /* 負載因子 ≤ 0.5 */
        s.bits++;
    s.keys = malloc(((size_t)1 << s.bits) * sizeof *s.keys);
    s.used = calloc((size_t)1 << s.bits, 1);
    for (int i = 0; i < numsSize; i++)
        set_add(&s, nums[i]);

    int best = 0;
    unsigned cap = 1u << s.bits;
    for (unsigned i = 0; i < cap; i++) { /* 走 set 而不是 nums：重複的值只處理一次 */
        if (!s.used[i])
            continue;
        int x = s.keys[i];
        if (x != -2147483647 - 1 && set_has(&s, x - 1))
            continue; /* 不是起點 */
        int len = 1;
        while (x != 2147483647 && set_has(&s, x + 1)) {
            x++;
            len++;
        }
        if (len > best)
            best = len;
    }
    free(s.keys);
    free(s.used);
    return best;
}
/* ===== 提交範圍 結束 ===== */

static int cmp_int(const void *a, const void *b)
{
    int x = *(const int *)a, y = *(const int *)b;
    return (x > y) - (x < y);
}

static int brute(int *a, int n) /* 排序後數連續段，O(n log n) */
{
    if (n == 0)
        return 0;
    int *b = malloc((size_t)n * sizeof *b);
    for (int i = 0; i < n; i++)
        b[i] = a[i];
    qsort(b, (size_t)n, sizeof *b, cmp_int);
    int best = 1, cur = 1;
    for (int i = 1; i < n; i++) {
        if (b[i] == b[i - 1])
            continue;
        cur = (long long)b[i] == (long long)b[i - 1] + 1 ? cur + 1 : 1;
        if (cur > best)
            best = cur;
    }
    free(b);
    return best;
}

int main(void)
{
    int a[] = {100, 4, 200, 1, 3, 2};
    assert(longestConsecutive(a, 6) == 4);
    int b[] = {0, 3, 7, 2, 5, 8, 4, 6, 0, 1};
    assert(longestConsecutive(b, 10) == 9);
    int c[] = {1, 0, 1, 2};
    assert(longestConsecutive(c, 4) == 3);
    assert(longestConsecutive(NULL, 0) == 0);
    int d[] = {2147483647, 2147483646, -2147483647 - 1}; /* 邊界不能溢位 */
    assert(longestConsecutive(d, 3) == 2);
    srand(128);
    for (int t = 0; t < 300; t++) {
        int n = rand() % 60, x[60];
        for (int i = 0; i < n; i++)
            x[i] = rand() % 80 - 40;
        assert(longestConsecutive(x, n) == brute(x, n));
    }
    puts("0128: passed");
    return 0;
}
