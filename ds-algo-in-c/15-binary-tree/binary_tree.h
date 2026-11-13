#ifndef BINARY_TREE_H
#define BINARY_TREE_H

typedef struct TNode {
    int val;
    struct TNode *left, *right;
} TNode;

TNode *tn_new(int val);
void   tree_free(TNode *root);

/* 用 LeetCode 的層序陣列建樹，例如 [3,9,20,null,null,15,7]；vals[i] == null_mark 代表空 */
TNode *tree_from_level(const int *vals, int n, int null_mark);

/* 走訪：結果寫進 out，回傳節點數。out 至少要有 tree_size 格。 */
int preorder(const TNode *root, int *out);  /* 根 左 右（遞迴） */
int inorder(const TNode *root, int *out);   /* 左 根 右（遞迴） */
int postorder(const TNode *root, int *out); /* 左 右 根（遞迴） */
int preorder_iter(const TNode *root, int *out);  /* 用自己的 stack */
int inorder_iter(const TNode *root, int *out);
int postorder_iter(const TNode *root, int *out);
int level_order(const TNode *root, int *out);    /* 用 queue，一層一層 */

int tree_size(const TNode *root);
int tree_height(const TNode *root); /* 空樹 0、只有根 1（以節點數計） */

/* 由前序 + 中序重建（值不可重複）。Thareja 9.4.5 */
TNode *build_pre_in(const int *pre, const int *in, int n);

#endif
