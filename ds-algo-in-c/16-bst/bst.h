#ifndef BST_H
#define BST_H

/* 二元搜尋樹 (Binary Search Tree)：左子樹全部 < 節點 < 右子樹全部。key 不重複。
 * 節點有 parent 指標（CLRS 的寫法），才能 O(h) 找後繼。 */
typedef struct BNode {
    int key;
    struct BNode *left, *right, *parent;
} BNode;

typedef struct {
    BNode *root;
    int size;
} BST;

void   bst_init(BST *t);
void   bst_free(BST *t);
int    bst_insert(BST *t, int key);        /* 新增回傳 1，已存在回傳 0 */
BNode *bst_search(const BST *t, int key);  /* 找不到 NULL */
int    bst_delete(BST *t, int key);        /* 有刪回傳 1 */

BNode *bst_min(BNode *x); /* 子樹最小：一直往左 */
BNode *bst_max(BNode *x);
BNode *bst_successor(BNode *x);   /* 中序的下一個；沒有回傳 NULL */
BNode *bst_predecessor(BNode *x);
BNode *bst_floor(const BST *t, int key); /* <= key 的最大者 */
BNode *bst_ceil(const BST *t, int key);  /* >= key 的最小者 */

int bst_inorder(const BST *t, int *out); /* 回傳個數，結果一定是遞增的 */
int bst_height(const BST *t);            /* 空樹 0 */
int bst_check(const BST *t);             /* 檢查 BST 性質、parent 指標、size；正確回傳 1 */

#endif
