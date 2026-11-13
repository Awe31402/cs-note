/* LeetCode 206 · Reverse Linked List
 * 思路（遞迴）：相信 reverseList(head->next) 會把後面整段反轉好，
 *   回傳新的頭；這時 head->next 變成後段的「尾巴」，把 head 接在它後面就好。
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

struct ListNode {
    int val;
    struct ListNode *next;
};

/* ===== 提交範圍 開始 ===== */
struct ListNode *reverseList(struct ListNode *head)
{
    if (head == NULL || head->next == NULL) /* 0 或 1 個節點：本來就是反的 */
        return head;
    struct ListNode *new_head = reverseList(head->next);
    head->next->next = head; /* 原本的下一個，現在指回我 */
    head->next = NULL;       /* 我變成尾巴 */
    return new_head;
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
    int a[] = {1, 2, 3, 4, 5}, ra[] = {5, 4, 3, 2, 1};
    check_and_free(reverseList(make(a, 5)), ra, 5);
    int b[] = {1, 2}, rb[] = {2, 1};
    check_and_free(reverseList(make(b, 2)), rb, 2);
    assert(reverseList(NULL) == NULL);
    int c[] = {7};
    check_and_free(reverseList(make(c, 1)), c, 1);
    puts("0206: passed");
    return 0;
}
