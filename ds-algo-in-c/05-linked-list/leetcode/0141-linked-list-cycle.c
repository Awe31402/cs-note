/* LeetCode 141 · Linked List Cycle
 * 思路：Floyd 龜兔賽跑。fast 走 2 步、slow 走 1 步。
 *       有環的話 fast 一定會在環裡追上 slow；沒環的話 fast 會先走到 NULL。O(n)、O(1) 空間。
 */
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

struct ListNode {
    int val;
    struct ListNode *next;
};

/* ===== 提交範圍 開始 ===== */
bool hasCycle(struct ListNode *head)
{
    struct ListNode *slow = head, *fast = head;
    while (fast && fast->next) {
        slow = slow->next;
        fast = fast->next->next;
        if (slow == fast) /* 比的是位址，不是值 */
            return true;
    }
    return false;
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

static void free_list(struct ListNode *p)
{
    while (p) {
        struct ListNode *next = p->next;
        free(p);
        p = next;
    }
}

/* 讓最後一個節點接回第 pos 個（pos = -1 表示沒有環），回傳尾巴方便之後拆掉 */
static struct ListNode *link_tail(struct ListNode *h, int pos)
{
    struct ListNode *tail = h, *target = NULL;
    for (int i = 0; tail->next; i++, tail = tail->next)
        if (i == pos)
            target = tail;
    if (pos >= 0 && !target)
        target = tail; /* pos 就是最後一個 */
    tail->next = target;
    return tail;
}

int main(void)
{
    int a[] = {3, 2, 0, -4};
    for (int pos = -1; pos < 4; pos++) {
        struct ListNode *h = make(a, 4);
        struct ListNode *tail = link_tail(h, pos);
        assert(hasCycle(h) == (pos >= 0));
        tail->next = NULL; /* 拆環才能 free */
        free_list(h);
    }
    int b[] = {1};
    struct ListNode *h = make(b, 1);
    assert(!hasCycle(h));
    h->next = h; /* 自己指自己 */
    assert(hasCycle(h));
    h->next = NULL;
    free_list(h);
    assert(!hasCycle(NULL));
    puts("0141: passed");
    return 0;
}
