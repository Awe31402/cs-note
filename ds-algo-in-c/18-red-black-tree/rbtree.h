#ifndef RBTREE_H
#define RBTREE_H

/* 紅黑樹（CLRS 第 13 章）。五條性質：
 *   1. 每個節點不是紅就是黑
 *   2. 根是黑的
 *   3. 每個葉子（NIL）是黑的
 *   4. 紅節點的兩個小孩都是黑的（不能連續兩個紅）
 *   5. 從任一節點往下到每個 NIL 的路徑，黑節點數都一樣（black-height）
 * 保證高度 <= 2 log2(n + 1)。
 * 用一個哨兵 (sentinel) 節點 nil 代表所有的 NULL，程式裡就不用一直判斷 NULL。 */
typedef enum { RED, BLACK } Color;

typedef struct RBNode {
    int key;
    Color color;
    struct RBNode *left, *right, *parent;
} RBNode;

typedef struct {
    RBNode *root;
    RBNode nil; /* 哨兵：rb_init 之後 RBTree 就不能再搬動（很多指標指向它） */
    int size;
} RBTree;

void    rb_init(RBTree *t);
void    rb_free(RBTree *t);
int     rb_insert(RBTree *t, int key);             /* 新增回傳 1，已存在 0 */
int     rb_delete(RBTree *t, int key);             /* 有刪回傳 1 */
RBNode *rb_search(const RBTree *t, int key);       /* 找不到回傳 NULL（不是 nil） */
RBNode *rb_lower_bound(const RBTree *t, int key);  /* 第一個 >= key，沒有回傳 NULL */
int     rb_inorder(const RBTree *t, int *out);
int     rb_height(const RBTree *t);
int     rb_check(const RBTree *t);                 /* 五條性質 + BST 順序 + parent 指標 + size；回傳 black-height，錯誤回傳 -1 */

#endif
