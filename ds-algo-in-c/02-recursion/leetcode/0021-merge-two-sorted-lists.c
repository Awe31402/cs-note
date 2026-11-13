/* LeetCode 21 · Merge Two Sorted Lists
 * 思路（遞迴）：兩條的頭比大小，小的那個當答案的頭，
 *   它的 next = 合併(它剩下的, 另一條)。
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

struct ListNode {
    int val;
    struct ListNode *next;
};

/* ===== 提交範圍 開始 ===== */
struct ListNode *mergeTwoLists(struct ListNode *list1, struct ListNode *list2)
{
    if (list1 == NULL)
        return list2;
    if (list2 == NULL)
        return list1;
    if (list1->val <= list2->val) { /* <= 讓相等時 list1 先，保持穩定 */
        list1->next = mergeTwoLists(list1->next, list2);
        return list1;
    }
    list2->next = mergeTwoLists(list1, list2->next);
    return list2;
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
    int a[] = {1, 2, 4}, b[] = {1, 3, 4};
    int want[] = {1, 1, 2, 3, 4, 4};
    check_and_free(mergeTwoLists(make(a, 3), make(b, 3)), want, 6);

    assert(mergeTwoLists(NULL, NULL) == NULL);
    int c[] = {0};
    check_and_free(mergeTwoLists(NULL, make(c, 1)), c, 1);

    int d[] = {-5, 10}, e[] = {-7, -6, 20};
    int want2[] = {-7, -6, -5, 10, 20};
    check_and_free(mergeTwoLists(make(d, 2), make(e, 3)), want2, 5);
    puts("0021: passed");
    return 0;
}
