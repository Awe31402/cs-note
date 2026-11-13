#ifndef HASH_TABLE_H
#define HASH_TABLE_H

/* int → int 的雜湊表，兩種解決碰撞的方法。 */

/* 雜湊函數：乘法法 (multiplication method, CLRS 11.3.2)。
 * 乘上一個跟 2^32 互質的大常數，取高位元。回傳 0 .. 2^bits - 1。 */
unsigned hash_int(int key, int bits);

/* ---- 分離鏈結 (separate chaining) ----
 * 每個 bucket 是一條串列。平均每條長度 α = size / nbuckets（負載因子 load factor）。 */
typedef struct HNode {
    int key, val;
    struct HNode *next;
} HNode;

typedef struct {
    HNode **buckets;
    int bits; /* bucket 數 = 2^bits */
    int size;
} ChainMap;

void cm_init(ChainMap *m);
void cm_free(ChainMap *m);
void cm_put(ChainMap *m, int key, int val);         /* 有就更新，沒有就新增；α > 1 時 bucket 加倍 */
int  cm_get(const ChainMap *m, int key, int *val); /* 找到回傳 1 */
int  cm_remove(ChainMap *m, int key);              /* 有刪回傳 1 */

/* ---- 開放定址 + 線性探測 (open addressing, linear probing) ----
 * 所有資料都放在陣列裡；位置被佔了就看下一格。
 * 刪除要留「墓碑 (tombstone)」，否則會切斷後面元素的探測路徑。 */
typedef struct {
    int *keys, *vals;
    unsigned char *state; /* 0 = 空, 1 = 有資料, 2 = 墓碑 */
    int bits;
    int size; /* 有資料的格數 */
    int used; /* 有資料 + 墓碑 */
} ProbeMap;

void pm_init(ProbeMap *m);
void pm_free(ProbeMap *m);
void pm_put(ProbeMap *m, int key, int val); /* used 超過一半就重建 */
int  pm_get(const ProbeMap *m, int key, int *val);
int  pm_remove(ProbeMap *m, int key);

#endif
