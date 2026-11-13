/* LeetCode 295 · Find Median from Data Stream
 * 思路：兩個 heap。
 *   low  = max-heap，放「較小的一半」
 *   high = min-heap，放「較大的一半」
 *   保持 low.size == high.size 或 low.size == high.size + 1。
 *   中位數 = low 的堆頂，或兩個堆頂的平均。
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/* ===== 提交範圍 開始 ===== */
typedef struct {
    int *a;
    int size, cap;
    int is_max; /* 1 = max-heap, 0 = min-heap */
} PQ;

/* x 是否該排在 y 前面（更靠近堆頂） */
static int before(const PQ *q, int x, int y)
{
    return q->is_max ? x > y : x < y;
}

static void pq_push(PQ *q, int x)
{
    if (q->size == q->cap) {
        q->cap = q->cap ? q->cap * 2 : 16;
        q->a = realloc(q->a, (size_t)q->cap * sizeof *q->a);
    }
    int i = q->size++;
    q->a[i] = x;
    while (i > 0 && before(q, q->a[i], q->a[(i - 1) / 2])) {
        int p = (i - 1) / 2;
        int t = q->a[i];
        q->a[i] = q->a[p];
        q->a[p] = t;
        i = p;
    }
}

static int pq_pop(PQ *q)
{
    int top = q->a[0];
    q->a[0] = q->a[--q->size];
    int i = 0;
    for (;;) {
        int l = 2 * i + 1, r = l + 1, m = i;
        if (l < q->size && before(q, q->a[l], q->a[m]))
            m = l;
        if (r < q->size && before(q, q->a[r], q->a[m]))
            m = r;
        if (m == i)
            break;
        int t = q->a[i];
        q->a[i] = q->a[m];
        q->a[m] = t;
        i = m;
    }
    return top;
}

typedef struct {
    PQ low;  /* max-heap：較小的一半 */
    PQ high; /* min-heap：較大的一半 */
} MedianFinder;

MedianFinder *medianFinderCreate(void) /* LeetCode 上寫成 medianFinderCreate() */
{
    MedianFinder *obj = calloc(1, sizeof *obj);
    obj->low.is_max = 1;
    obj->high.is_max = 0;
    return obj;
}

void medianFinderAddNum(MedianFinder *obj, int num)
{
    /* 先進 low，再把 low 最大的送去 high：保證 low 全部 <= high 全部 */
    pq_push(&obj->low, num);
    pq_push(&obj->high, pq_pop(&obj->low));
    /* high 比較多就還一個回來：保證 low 不會比 high 少 */
    if (obj->high.size > obj->low.size)
        pq_push(&obj->low, pq_pop(&obj->high));
}

double medianFinderFindMedian(MedianFinder *obj)
{
    if (obj->low.size > obj->high.size)
        return obj->low.a[0];
    return ((double)obj->low.a[0] + obj->high.a[0]) / 2.0; /* 先轉 double，避免 int 相加溢位 */
}

void medianFinderFree(MedianFinder *obj)
{
    free(obj->low.a);
    free(obj->high.a);
    free(obj);
}
/* ===== 提交範圍 結束 ===== */

int main(void)
{
    MedianFinder *mf = medianFinderCreate();
    medianFinderAddNum(mf, 1);
    medianFinderAddNum(mf, 2);
    assert(medianFinderFindMedian(mf) == 1.5);
    medianFinderAddNum(mf, 3);
    assert(medianFinderFindMedian(mf) == 2.0);
    medianFinderFree(mf);

    /* 隨機對照：用插入排序維持一個排好的陣列 */
    srand(295);
    mf = medianFinderCreate();
    int sorted[500];
    for (int n = 0; n < 500; n++) {
        int x = rand() % 201 - 100;
        medianFinderAddNum(mf, x);
        int i = n;
        while (i > 0 && sorted[i - 1] > x) {
            sorted[i] = sorted[i - 1];
            i--;
        }
        sorted[i] = x;
        int len = n + 1;
        double want = len % 2 ? sorted[len / 2]
                              : (sorted[len / 2 - 1] + sorted[len / 2]) / 2.0;
        assert(medianFinderFindMedian(mf) == want);
    }
    medianFinderFree(mf);

    puts("0295: passed");
    return 0;
}
