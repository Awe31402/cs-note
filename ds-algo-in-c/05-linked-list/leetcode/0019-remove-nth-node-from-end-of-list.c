/* LeetCode 19 · Remove Nth Node From End of List
 * 思路：一趟完成。fast 先走 n 步，然後 fast、slow 一起走；
 *       fast 到最後一個節點時，slow 剛好在「要刪的前一個」。
 *       用 dummy 假頭，刪第一個節點時就不用特別處理。
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

struct ListNode {
    int val;
    struct ListNode *next;
};

/* ===== 提交範圍 開始 ===== */
struct ListNode *removeNthFromEnd(struct ListNode *head, int n)
{
    struct ListNode dummy = {0, head};
    struct ListNode *fast = &dummy, *slow = &dummy;
    for (int i = 0; i < n; i++)
        fast = fast->next;
    while (fast->next) {
        fast = fast->next;
        slow = slow->next;
    }
    struct ListNode *dead = slow->next;
    slow->next = dead->next;
    free(dead); /* LeetCode 上可省略，本機要 free 才不會被 ASan 抓 */
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

static void check(const int *a, int len, int n, const int *want)
{
    struct ListNode *h = removeNthFromEnd(make(a, len), n), *p = h;
    for (int i = 0; i < len - 1; i++, p = p->next)
        assert(p && p->val == want[i]);
    assert(p == NULL);
    free_list(h);
}

int main(void)
{
    int a[] = {1, 2, 3, 4, 5}, wa[] = {1, 2, 3, 5};
    check(a, 5, 2, wa);
    int wa1[] = {2, 3, 4, 5}; /* 刪第一個 */
    check(a, 5, 5, wa1);
    int wa2[] = {1, 2, 3, 4}; /* 刪最後一個 */
    check(a, 5, 1, wa2);
    int b[] = {1};
    check(b, 1, 1, NULL); /* 刪完變空 */
    int c[] = {1, 2}, wc[] = {1};
    check(c, 2, 1, wc);
    puts("0019: passed");
    return 0;
}
