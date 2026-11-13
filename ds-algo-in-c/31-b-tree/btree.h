#ifndef BTREE_H
#define BTREE_H

/* B-Tree（CLRS 第 18 章），最小度數 (minimum degree) t >= 2：
 *   - 每個節點最多 2t-1 個 key、2t 個小孩
 *   - 除了根，每個節點至少 t-1 個 key
 *   - 所有葉子在同一層
 * t = 2 就是 2-3-4 樹。資料庫、檔案系統用很大的 t，讓樹很矮、讀磁碟的次數很少。 */
typedef struct BTNode {
    int n;    /* 目前有幾個 key */
    int leaf; /* 是不是葉子 */
    int *keys;             /* 長度 2t-1，keys[0..n-1] 由小到大 */
    struct BTNode **child; /* 長度 2t，child[0..n] */
} BTNode;

typedef struct {
    BTNode *root;
    int t;
    int size;
} BTree;

void bt_init(BTree *b, int t);
void bt_free(BTree *b);
int  bt_search(const BTree *b, int key);     /* 找到回傳 1 */
int  bt_insert(BTree *b, int key);           /* 新增回傳 1，已存在 0 */
int  bt_delete(BTree *b, int key);           /* 有刪回傳 1 */
int  bt_lower_bound(const BTree *b, int key, int *out); /* 第一個 >= key 的值寫進 *out；沒有回傳 0 */
int  bt_inorder(const BTree *b, int *out);   /* 回傳 key 的個數 */
int  bt_height(const BTree *b);              /* 只有根 = 1 */
int  bt_check(const BTree *b);               /* 檢查所有 B-Tree 性質；正確回傳 1 */

#endif
