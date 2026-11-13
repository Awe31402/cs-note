#ifndef AVL_H
#define AVL_H

/* AVL 樹：每個節點左右子樹的高度差（平衡因子 balance factor）只能是 -1、0、+1。
 * 插入、刪除後沿著路徑往上，用旋轉 (rotation) 修正。高度保證 < 1.44 log2(n + 2)。 */
typedef struct AVLNode {
    int key;
    int height; /* 葉子是 1 */
    struct AVLNode *left, *right;
} AVLNode;

typedef struct {
    AVLNode *root;
    int size;
    long rotations; /* 統計用：做了幾次單旋轉 */
} AVL;

void avl_init(AVL *t);
void avl_free(AVL *t);
int  avl_insert(AVL *t, int key);   /* 新增回傳 1，已存在 0 */
int  avl_delete(AVL *t, int key);   /* 有刪回傳 1 */
int  avl_contains(const AVL *t, int key);
int  avl_inorder(const AVL *t, int *out);
int  avl_height(const AVL *t);
int  avl_check(const AVL *t);       /* BST 性質、height 欄位正確、每個節點都平衡、size 正確 */

#endif
