/* LeetCode 876 · Middle of the Linked List
 * 思路：快慢指標。fast 一次走 2 步、slow 一次走 1 步；fast 走到底時 slow 剛好在中間。
 *       偶數個節點時回傳「第二個」中間節點。
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

struct ListNode {
    int val;
    struct ListNode *next;
};

/* ===== 提交範圍 開始 ===== */
struct ListNode *middleNode(struct ListNode *head)
{
    struct ListNode *slow = head, *fast = head;
    while (fast && fast->next) {
        slow = slow->next;
        fast = fast->next->next;
    }
    return slow;
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

int main(void)
{
    int a[] = {1, 2, 3, 4, 5};
    struct ListNode *h = make(a, 5);
    assert(middleNode(h)->val == 3);
    free_list(h);

    int b[] = {1, 2, 3, 4, 5, 6};
    h = make(b, 6);
    assert(middleNode(h)->val == 4); /* 第二個中間 */
    free_list(h);

    int c[] = {9};
    h = make(c, 1);
    assert(middleNode(h)->val == 9);
    free_list(h);
    puts("0876: passed");
    return 0;
}
