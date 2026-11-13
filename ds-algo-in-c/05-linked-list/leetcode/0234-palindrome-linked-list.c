/* LeetCode 234 · Palindrome Linked List（O(n) 時間、O(1) 空間）
 * 思路：
 *   1. 快慢指標找中間
 *   2. 把後半段反轉
 *   3. 前半段和反轉後的後半段逐一比較
 *   4.（好習慣）把後半段轉回來，不破壞輸入
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
static struct ListNode *reverse(struct ListNode *cur)
{
    struct ListNode *prev = NULL;
    while (cur) {
        struct ListNode *next = cur->next;
        cur->next = prev;
        prev = cur;
        cur = next;
    }
    return prev;
}

bool isPalindrome(struct ListNode *head)
{
    struct ListNode *slow = head, *fast = head;
    while (fast && fast->next) {
        slow = slow->next;
        fast = fast->next->next;
    }
    /* 奇數個時 slow 在正中間，中間那個不用比；後半段從 slow 開始反轉也沒關係 */
    struct ListNode *second = reverse(slow);
    bool ok = true;
    for (struct ListNode *p = head, *q = second; q; p = p->next, q = q->next)
        if (p->val != q->val) {
            ok = false;
            break;
        }
    reverse(second); /* 復原 */
    return ok;
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

static void check(const int *a, int n, bool want)
{
    struct ListNode *h = make(a, n);
    assert(isPalindrome(h) == want);
    /* 確認有復原 */
    struct ListNode *p = h;
    for (int i = 0; i < n; i++, p = p->next)
        assert(p && p->val == a[i]);
    assert(p == NULL);
    free_list(h);
}

int main(void)
{
    int a[] = {1, 2, 2, 1}, b[] = {1, 2}, c[] = {1}, d[] = {1, 2, 3, 2, 1}, e[] = {1, 2, 3, 1};
    check(a, 4, true);
    check(b, 2, false);
    check(c, 1, true);
    check(d, 5, true);
    check(e, 4, false);
    puts("0234: passed");
    return 0;
}
