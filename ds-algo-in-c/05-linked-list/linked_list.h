#ifndef LINKED_LIST_H
#define LINKED_LIST_H

/* ---- 單向串列 (singly linked list) ----
 * 用 Node *head 表示，空串列是 NULL。會改到 head 的函式收 Node **。 */
typedef struct Node {
    int val;
    struct Node *next;
} Node;

void  sl_push_front(Node **head, int x);          /* O(1) */
void  sl_push_back(Node **head, int x);           /* O(n)：要走到尾 */
int   sl_insert_at(Node **head, int idx, int x);  /* 插成第 idx 個（0 = 最前面）；成功 0，越界 -1 */
int   sl_remove(Node **head, int x);              /* 刪第一個值為 x 的節點；有刪回傳 1 */
Node *sl_find(Node *head, int x);                 /* 找不到回傳 NULL */
void  sl_reverse(Node **head);                    /* 迴圈版反轉，O(n)、O(1) 空間 */
int   sl_len(const Node *head);
int   sl_to_array(const Node *head, int *out, int cap); /* 回傳寫入個數 */
void  sl_free(Node **head);

/* ---- 雙向環狀串列，含哨兵節點 (doubly circular list with sentinel) ----
 * head 是不存資料的哨兵：空串列時 head.next == head.prev == &head。
 * 有了哨兵，插入刪除都不用特別處理「頭」「尾」「空」的情況。Thareja 6.6 叫它 header node。 */
typedef struct DNode {
    int val;
    struct DNode *prev, *next;
} DNode;

typedef struct {
    DNode head;
    int size;
} DList;

void   dl_init(DList *l);
void   dl_free(DList *l);
DNode *dl_push_front(DList *l, int x); /* O(1)，回傳新節點 */
DNode *dl_push_back(DList *l, int x);  /* O(1)：從哨兵往前一格就是尾巴 */
int    dl_pop_front(DList *l);         /* O(1)；不可為空 */
int    dl_pop_back(DList *l);          /* O(1)；不可為空 */
DNode *dl_find(DList *l, int x);       /* O(n)；找不到回傳 NULL */
void   dl_remove(DList *l, DNode *n);  /* O(1)：拿到節點就能直接刪 */
int    dl_to_array(const DList *l, int *out, int cap);

#endif
