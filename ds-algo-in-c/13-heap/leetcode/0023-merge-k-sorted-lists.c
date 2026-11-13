/* LeetCode 23 · Merge k Sorted Lists
 * 思路：min-heap 裡放每條串列「目前的頭」。每次拿最小的接到答案後面，
 *       再把它的下一個節點放回 heap。
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

struct ListNode {
    int val;
    struct ListNode *next;
};

/* ===== 提交範圍 開始（LeetCode 已經定義好 struct ListNode） ===== */
static void swap(struct ListNode **a, struct ListNode **b)
{
    struct ListNode *t = *a;
    *a = *b;
    *b = t;
}

static void sift_up(struct ListNode **h, int i)
{
    while (i > 0 && h[(i - 1) / 2]->val > h[i]->val) {
        swap(&h[(i - 1) / 2], &h[i]);
        i = (i - 1) / 2;
    }
}

static void sift_down(struct ListNode **h, int n, int i)
{
    for (;;) {
        int l = 2 * i + 1, r = l + 1, m = i;
        if (l < n && h[l]->val < h[m]->val)
            m = l;
        if (r < n && h[r]->val < h[m]->val)
            m = r;
        if (m == i)
            return;
        swap(&h[i], &h[m]);
        i = m;
    }
}

struct ListNode *mergeKLists(struct ListNode **lists, int listsSize)
{
    struct ListNode **h = malloc((size_t)(listsSize ? listsSize : 1) * sizeof *h);
    int size = 0;
    for (int i = 0; i < listsSize; i++) {
        if (lists[i]) { /* 空串列不要放進 heap */
            h[size] = lists[i];
            sift_up(h, size);
            size++;
        }
    }

    struct ListNode dummy = {0, NULL}; /* 假頭，省掉「第一個節點」的特例 */
    struct ListNode *tail = &dummy;
    while (size > 0) {
        struct ListNode *min = h[0];
        tail->next = min;
        tail = min;
        if (min->next) {
            h[0] = min->next; /* 同一條的下一個直接取代堆頂 */
        } else {
            h[0] = h[--size];
        }
        sift_down(h, size, 0);
    }
    free(h);
    return dummy.next;
}
/* ===== 提交範圍 結束 ===== */

static struct ListNode *make(const int *a, int n)
{
    struct ListNode *head = NULL;
    for (int i = n - 1; i >= 0; i--) {
        struct ListNode *p = malloc(sizeof *p);
        p->val = a[i];
        p->next = head;
        head = p;
    }
    return head;
}

static void check_and_free(struct ListNode *p, const int *want, int n)
{
    for (int i = 0; i < n; i++) {
        assert(p && p->val == want[i]);
        struct ListNode *next = p->next;
        free(p);
        p = next;
    }
    assert(p == NULL);
}

int main(void)
{
    int a[] = {1, 4, 5}, b[] = {1, 3, 4}, c[] = {2, 6};
    struct ListNode *lists[] = {make(a, 3), make(b, 3), make(c, 2)};
    int want[] = {1, 1, 2, 3, 4, 4, 5, 6};
    check_and_free(mergeKLists(lists, 3), want, 8);

    assert(mergeKLists(NULL, 0) == NULL);        /* lists = [] */
    struct ListNode *empty[] = {NULL};
    assert(mergeKLists(empty, 1) == NULL);       /* lists = [[]] */

    int d[] = {-2, 0, 7};
    struct ListNode *mixed[] = {NULL, make(d, 3), NULL};
    check_and_free(mergeKLists(mixed, 3), d, 3);

    puts("0023: passed");
    return 0;
}
