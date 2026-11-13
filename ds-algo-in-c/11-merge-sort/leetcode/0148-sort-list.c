/* LeetCode 148 · Sort List（O(n log n)）
 * 思路：串列上的 merge sort。
 *   1. 快慢指標找中間，切成兩條
 *   2. 兩條各自遞迴排好
 *   3. 合併兩條已排序串列（LeetCode 21）
 *   串列的合併不需要額外陣列，只要改指標。
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

struct ListNode {
    int val;
    struct ListNode *next;
};

/* ===== 提交範圍 開始 ===== */
static struct ListNode *merge(struct ListNode *a, struct ListNode *b)
{
    struct ListNode dummy = {0, NULL}, *t = &dummy;
    while (a && b) {
        if (a->val <= b->val) {
            t->next = a;
            a = a->next;
        } else {
            t->next = b;
            b = b->next;
        }
        t = t->next;
    }
    t->next = a ? a : b;
    return dummy.next;
}

struct ListNode *sortList(struct ListNode *head)
{
    if (!head || !head->next)
        return head;
    /* fast 從 head->next 出發：兩個節點時 slow 停在第一個，才切得開 */
    struct ListNode *slow = head, *fast = head->next;
    while (fast && fast->next) {
        slow = slow->next;
        fast = fast->next->next;
    }
    struct ListNode *right = slow->next;
    slow->next = NULL; /* 切斷 */
    return merge(sortList(head), sortList(right));
}
/* ===== 提交範圍 結束 ===== */

static int cmp_int(const void *x, const void *y)
{
    int a = *(const int *)x, b = *(const int *)y;
    return (a > b) - (a < b);
}

int main(void)
{
    srand(148);
    for (int t = 0; t < 300; t++) {
        int n = rand() % 50, a[50];
        struct ListNode *h = NULL;
        for (int i = n - 1; i >= 0; i--) {
            a[i] = rand() % 201 - 100;
            struct ListNode *p = malloc(sizeof *p);
            p->val = a[i];
            p->next = h;
            h = p;
        }
        qsort(a, (size_t)n, sizeof *a, cmp_int);
        h = sortList(h);
        for (int i = 0; i < n; i++) {
            assert(h && h->val == a[i]);
            struct ListNode *next = h->next;
            free(h);
            h = next;
        }
        assert(h == NULL);
    }
    puts("0148: passed");
    return 0;
}
