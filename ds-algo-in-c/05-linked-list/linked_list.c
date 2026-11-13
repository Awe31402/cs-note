#include "linked_list.h"

#include <assert.h>
#include <stdlib.h>

static Node *new_node(int x, Node *next)
{
    Node *n = malloc(sizeof *n);
    if (!n)
        abort();
    n->val = x;
    n->next = next;
    return n;
}

void sl_push_front(Node **head, int x)
{
    *head = new_node(x, *head);
}

/* 指標的指標 (pointer to pointer)：p 指向「要改的那個 next 欄位」，
 * 一開始是 head 本身。這樣「插在最前面」和「插在中間」是同一段程式。 */
void sl_push_back(Node **head, int x)
{
    Node **p = head;
    while (*p)
        p = &(*p)->next;
    *p = new_node(x, NULL);
}

int sl_insert_at(Node **head, int idx, int x)
{
    if (idx < 0)
        return -1;
    Node **p = head;
    for (int i = 0; i < idx; i++) {
        if (!*p)
            return -1;
        p = &(*p)->next;
    }
    *p = new_node(x, *p);
    return 0;
}

int sl_remove(Node **head, int x)
{
    for (Node **p = head; *p; p = &(*p)->next) {
        if ((*p)->val == x) {
            Node *dead = *p;
            *p = dead->next; /* 前一個的 next（或 head）直接跳過它 */
            free(dead);
            return 1;
        }
    }
    return 0;
}

Node *sl_find(Node *head, int x)
{
    for (; head; head = head->next)
        if (head->val == x)
            return head;
    return NULL;
}

void sl_reverse(Node **head)
{
    Node *prev = NULL, *cur = *head;
    while (cur) {
        Node *next = cur->next; /* 1. 先記住下一個 */
        cur->next = prev;       /* 2. 反過來指 */
        prev = cur;             /* 3. 兩個指標都往前一步 */
        cur = next;
    }
    *head = prev;
}

int sl_len(const Node *head)
{
    int n = 0;
    for (; head; head = head->next)
        n++;
    return n;
}

int sl_to_array(const Node *head, int *out, int cap)
{
    int k = 0;
    for (; head && k < cap; head = head->next)
        out[k++] = head->val;
    return k;
}

void sl_free(Node **head)
{
    while (*head) {
        Node *next = (*head)->next;
        free(*head);
        *head = next;
    }
}

/* ---- 雙向環狀 ---- */

void dl_init(DList *l)
{
    l->head.prev = l->head.next = &l->head;
    l->size = 0;
}

/* 把新節點 n 插在 pos 的後面 */
static DNode *insert_after(DList *l, DNode *pos, int x)
{
    DNode *n = malloc(sizeof *n);
    if (!n)
        abort();
    n->val = x;
    n->prev = pos;
    n->next = pos->next;
    pos->next->prev = n; /* 順序重要：先改後面那個的 prev，再改 pos->next */
    pos->next = n;
    l->size++;
    return n;
}

DNode *dl_push_front(DList *l, int x) { return insert_after(l, &l->head, x); }
DNode *dl_push_back(DList *l, int x) { return insert_after(l, l->head.prev, x); }

void dl_remove(DList *l, DNode *n)
{
    assert(n != &l->head);
    n->prev->next = n->next;
    n->next->prev = n->prev;
    free(n);
    l->size--;
}

int dl_pop_front(DList *l)
{
    assert(l->size > 0);
    int x = l->head.next->val;
    dl_remove(l, l->head.next);
    return x;
}

int dl_pop_back(DList *l)
{
    assert(l->size > 0);
    int x = l->head.prev->val;
    dl_remove(l, l->head.prev);
    return x;
}

DNode *dl_find(DList *l, int x)
{
    for (DNode *n = l->head.next; n != &l->head; n = n->next) /* 繞回哨兵就停 */
        if (n->val == x)
            return n;
    return NULL;
}

int dl_to_array(const DList *l, int *out, int cap)
{
    int k = 0;
    for (const DNode *n = l->head.next; n != &l->head && k < cap; n = n->next)
        out[k++] = n->val;
    return k;
}

void dl_free(DList *l)
{
    while (l->size > 0)
        dl_remove(l, l->head.next);
}
