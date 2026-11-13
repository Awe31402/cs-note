#include "hash_table.h"

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

static void test_hash_range(void)
{
    for (int bits = 1; bits <= 16; bits++)
        for (int k = -1000; k <= 1000; k++)
            assert(hash_int(k, bits) < (1u << bits));
    assert(hash_int(INT_MIN, 10) < 1024 && hash_int(INT_MAX, 10) < 1024);

    /* 連續的 key 應該分散開，不該擠在少數 bucket */
    int cnt[64] = {0};
    for (int k = 0; k < 6400; k++)
        cnt[hash_int(k, 6)]++;
    for (int b = 0; b < 64; b++)
        assert(cnt[b] > 50 && cnt[b] < 150);
}

static void test_basic(void)
{
    ChainMap c;
    ProbeMap p;
    cm_init(&c);
    pm_init(&p);
    int v;
    assert(!cm_get(&c, 1, &v) && !pm_get(&p, 1, &v));
    cm_put(&c, 1, 100);
    pm_put(&p, 1, 100);
    cm_put(&c, 1, 200); /* 更新 */
    pm_put(&p, 1, 200);
    assert(cm_get(&c, 1, &v) && v == 200 && c.size == 1);
    assert(pm_get(&p, 1, &v) && v == 200 && p.size == 1);
    cm_put(&c, -5, 7);
    pm_put(&p, -5, 7);
    assert(cm_remove(&c, 1) && !cm_remove(&c, 1) && !cm_get(&c, 1, NULL));
    assert(pm_remove(&p, 1) && !pm_remove(&p, 1) && !pm_get(&p, 1, NULL));
    assert(cm_get(&c, -5, &v) && v == 7);
    assert(pm_get(&p, -5, &v) && v == 7);
    cm_free(&c);
    pm_free(&p);
}

/* 刪除只設成空格的話，後面同一串探測的 key 會找不到。這個測試專門抓這個 bug。 */
static void test_tombstone(void)
{
    ProbeMap p;
    pm_init(&p);
    /* 找三個會撞到同一格的 key */
    int keys[3], n = 0;
    unsigned target = hash_int(0, p.bits);
    for (int k = 0; n < 3; k++)
        if (hash_int(k, p.bits) == target)
            keys[n++] = k;
    for (int i = 0; i < 3; i++)
        pm_put(&p, keys[i], i);
    assert(p.bits == 3); /* 還沒重建，三個真的擠在一起 */
    pm_remove(&p, keys[0]);
    int v;
    assert(pm_get(&p, keys[1], &v) && v == 1);
    assert(pm_get(&p, keys[2], &v) && v == 2);
    pm_free(&p);
}

/* 隨機操作：key 範圍小，用陣列當標準答案；兩種表要跟答案完全一致 */
static void test_random(void)
{
    enum { R = 2000 };
    static int present[R], value[R];
    ChainMap c;
    ProbeMap p;
    cm_init(&c);
    pm_init(&p);
    srand(8);
    int size = 0;
    for (int t = 0; t < 200000; t++) {
        int k = rand() % R, op = rand() % 3, v;
        if (op < 2) {
            int x = rand();
            cm_put(&c, k - R / 2, x); /* 讓 key 有負數 */
            pm_put(&p, k - R / 2, x);
            size += !present[k];
            present[k] = 1;
            value[k] = x;
        } else {
            int r1 = cm_remove(&c, k - R / 2), r2 = pm_remove(&p, k - R / 2);
            assert(r1 == present[k] && r2 == present[k]);
            size -= present[k];
            present[k] = 0;
        }
        assert(c.size == size && p.size == size);
        int q = rand() % R;
        int g1 = cm_get(&c, q - R / 2, &v);
        assert(g1 == present[q] && (!g1 || v == value[q]));
        int g2 = pm_get(&p, q - R / 2, &v);
        assert(g2 == present[q] && (!g2 || v == value[q]));
    }
    assert(p.used * 2 <= 1 << p.bits); /* 探測表至少一半是空格 */
    cm_free(&c);
    pm_free(&p);
}

int main(void)
{
    test_hash_range();
    test_basic();
    test_tombstone();
    test_random();
    puts("hash_table: all tests passed");
    return 0;
}
