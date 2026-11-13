/* LeetCode 703 · Kth Largest Element in a Stream
 * 思路：跟 215 一樣，維持大小為 k 的 min-heap，只是資料一個一個來。
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/* ===== 提交範圍 開始 ===== */
typedef struct {
    int *h;
    int size;
    int k;
} KthLargest;

static void swap(int *a, int *b)
{
    int t = *a;
    *a = *b;
    *b = t;
}

static void sift_up_min(int *a, int i)
{
    while (i > 0 && a[(i - 1) / 2] > a[i]) {
        swap(&a[(i - 1) / 2], &a[i]);
        i = (i - 1) / 2;
    }
}

static void sift_down_min(int *a, int n, int i)
{
    for (;;) {
        int l = 2 * i + 1, r = l + 1, m = i;
        if (l < n && a[l] < a[m])
            m = l;
        if (r < n && a[r] < a[m])
            m = r;
        if (m == i)
            return;
        swap(&a[i], &a[m]);
        i = m;
    }
}

int kthLargestAdd(KthLargest *obj, int val)
{
    if (obj->size < obj->k) { /* 還沒滿 k 個：直接放進去 */
        obj->h[obj->size] = val;
        sift_up_min(obj->h, obj->size);
        obj->size++;
    } else if (val > obj->h[0]) { /* 滿了：比堆頂大才取代堆頂 */
        obj->h[0] = val;
        sift_down_min(obj->h, obj->k, 0);
    }
    return obj->h[0];
}

KthLargest *kthLargestCreate(int k, int *nums, int numsSize)
{
    KthLargest *obj = malloc(sizeof *obj);
    obj->h = malloc((size_t)k * sizeof *obj->h);
    obj->size = 0;
    obj->k = k;
    for (int i = 0; i < numsSize; i++)
        kthLargestAdd(obj, nums[i]);
    return obj;
}

void kthLargestFree(KthLargest *obj)
{
    free(obj->h);
    free(obj);
}
/* ===== 提交範圍 結束 ===== */

int main(void)
{
    int nums[] = {4, 5, 8, 2};
    KthLargest *obj = kthLargestCreate(3, nums, 4);
    assert(kthLargestAdd(obj, 3) == 4);
    assert(kthLargestAdd(obj, 5) == 5);
    assert(kthLargestAdd(obj, 10) == 5);
    assert(kthLargestAdd(obj, 9) == 8);
    assert(kthLargestAdd(obj, 4) == 8);
    kthLargestFree(obj);

    /* 一開始 nums 少於 k 個 */
    KthLargest *o2 = kthLargestCreate(2, NULL, 0);
    kthLargestAdd(o2, 1);
    assert(kthLargestAdd(o2, 7) == 1);
    assert(kthLargestAdd(o2, 3) == 3);
    kthLargestFree(o2);

    puts("0703: passed");
    return 0;
}
