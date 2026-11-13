/* LeetCode 307 · Range Sum Query - Mutable
 * 跟 303 一樣問區間和，但中間會「改某一格的值」。
 * 前綴和每次修改要 O(n) 重算；樹狀陣列 (Fenwick) 修改和查詢都是 O(log n)。
 * 只有「單點修改 + 區間和」→ Fenwick 最短最好寫；需要區間修改時才用有懶標記的線段樹。
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/* ===== 提交範圍 開始 ===== */
typedef struct {
    int n;
    int *bit;  /* 1-based 樹狀陣列 */
    int *vals; /* 記住目前的值：update 給的是新值，要換算成差值 */
} NumArray;

static void add(NumArray *a, int i, int d)
{
    for (i++; i <= a->n; i += i & -i)
        a->bit[i] += d;
}

static int prefix(const NumArray *a, int i) /* nums[0..i] 的和 */
{
    int s = 0;
    for (i++; i > 0; i -= i & -i)
        s += a->bit[i];
    return s;
}

NumArray *numArrayCreate(int *nums, int numsSize)
{
    NumArray *a = malloc(sizeof *a);
    a->n = numsSize;
    a->bit = calloc((size_t)numsSize + 1, sizeof *a->bit);
    a->vals = malloc((size_t)numsSize * sizeof *a->vals);
    for (int i = 0; i < numsSize; i++) {
        a->vals[i] = nums[i];
        add(a, i, nums[i]); /* O(n log n) 建樹；也有 O(n) 的建法 */
    }
    return a;
}

void numArrayUpdate(NumArray *a, int index, int val)
{
    add(a, index, val - a->vals[index]);
    a->vals[index] = val;
}

int numArraySumRange(NumArray *a, int left, int right)
{
    return prefix(a, right) - prefix(a, left - 1);
}

void numArrayFree(NumArray *a)
{
    free(a->bit);
    free(a->vals);
    free(a);
}
/* ===== 提交範圍 結束 ===== */

int main(void)
{
    int nums[] = {1, 3, 5};
    NumArray *a = numArrayCreate(nums, 3);
    assert(numArraySumRange(a, 0, 2) == 9);
    numArrayUpdate(a, 1, 2);
    assert(numArraySumRange(a, 0, 2) == 8);
    numArrayFree(a);

    srand(307);
    int x[200];
    for (int i = 0; i < 200; i++)
        x[i] = rand() % 201 - 100;
    a = numArrayCreate(x, 200);
    for (int t = 0; t < 5000; t++) {
        if (rand() % 2) {
            int i = rand() % 200;
            x[i] = rand() % 201 - 100;
            numArrayUpdate(a, i, x[i]);
        } else {
            int l = rand() % 200, r = rand() % 200;
            if (l > r) {
                int tmp = l;
                l = r;
                r = tmp;
            }
            int want = 0;
            for (int i = l; i <= r; i++)
                want += x[i];
            assert(numArraySumRange(a, l, r) == want);
        }
    }
    numArrayFree(a);
    puts("0307: passed");
    return 0;
}
