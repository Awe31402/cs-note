/* LeetCode 2 · Add Two Numbers
 * 思路：數字是反著存的（個位數在最前面），剛好就是直式加法的順序。
 *       兩條一起往後走，每位相加再加進位 carry。任一條沒了就當 0。
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

struct ListNode {
    int val;
    struct ListNode *next;
};

/* ===== 提交範圍 開始 ===== */
struct ListNode *addTwoNumbers(struct ListNode *l1, struct ListNode *l2)
{
    struct ListNode dummy = {0, NULL}, *tail = &dummy;
    int carry = 0;
    while (l1 || l2 || carry) { /* 最後還有進位也要多一位，例如 5 + 5 = 10 */
        int s = carry;
        if (l1) {
            s += l1->val;
            l1 = l1->next;
        }
        if (l2) {
            s += l2->val;
            l2 = l2->next;
        }
        carry = s / 10;
        tail->next = malloc(sizeof *tail->next);
        tail = tail->next;
        tail->val = s % 10;
        tail->next = NULL;
    }
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

static void free_list(struct ListNode *p)
{
    while (p) {
        struct ListNode *next = p->next;
        free(p);
        p = next;
    }
}

static void check(const int *a, int n, const int *b, int m, const int *want, int k)
{
    struct ListNode *x = make(a, n), *y = make(b, m);
    struct ListNode *r = addTwoNumbers(x, y), *p = r;
    for (int i = 0; i < k; i++, p = p->next)
        assert(p && p->val == want[i]);
    assert(p == NULL);
    free_list(x);
    free_list(y);
    free_list(r);
}

int main(void)
{
    int a[] = {2, 4, 3}, b[] = {5, 6, 4}, w[] = {7, 0, 8}; /* 342 + 465 = 807 */
    check(a, 3, b, 3, w, 3);
    int z[] = {0};
    check(z, 1, z, 1, z, 1);
    int c[] = {9, 9, 9, 9, 9, 9, 9}, d[] = {9, 9, 9, 9}, wcd[] = {8, 9, 9, 9, 0, 0, 0, 1};
    check(c, 7, d, 4, wcd, 8); /* 長度不同 + 最後進位 */
    puts("0002: passed");
    return 0;
}
