/* LeetCode 147 · Insertion Sort List
 * 思路：對串列做插入排序。建一條新的已排序串列（dummy 開頭），
 *       原串列的節點一個一個拿下來，從 dummy 往後找插入點。
 *       小技巧：如果新節點 >= 上次插入的位置，就從那裡開始找，已排序的輸入會變成 O(n)。
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

struct ListNode {
    int val;
    struct ListNode *next;
};

/* ===== 提交範圍 開始 ===== */
struct ListNode *insertionSortList(struct ListNode *head)
{
    struct ListNode dummy = {0, NULL}, *last = &dummy;
    while (head) {
        struct ListNode *node = head;
        head = head->next;
        if (last == &dummy || last->val > node->val)
            last = &dummy; /* 要插在 last 前面：從頭找 */
        while (last->next && last->next->val <= node->val) /* <= 讓相等的排在後面：穩定 */
            last = last->next;
        node->next = last->next;
        last->next = node;
    }
    return dummy.next;
}
/* ===== 提交範圍 結束 ===== */

static int cmp_int(const void *x, const void *y)
{
    int a = *(const int *)x, b = *(const int *)y;
    return (a > b) - (a < b);
}

int main(void)
{
    srand(147);
    for (int t = 0; t < 300; t++) {
        int n = rand() % 30, a[30];
        struct ListNode *h = NULL;
        for (int i = n - 1; i >= 0; i--) {
            a[i] = rand() % 21 - 10;
            struct ListNode *p = malloc(sizeof *p);
            p->val = a[i];
            p->next = h;
            h = p;
        }
        qsort(a, (size_t)n, sizeof *a, cmp_int);
        h = insertionSortList(h);
        for (int i = 0; i < n; i++) {
            assert(h && h->val == a[i]);
            struct ListNode *next = h->next;
            free(h);
            h = next;
        }
        assert(h == NULL);
    }
    puts("0147: passed");
    return 0;
}
