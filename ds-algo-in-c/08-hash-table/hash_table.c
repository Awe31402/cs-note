#include "hash_table.h"

#include <stdlib.h>

unsigned hash_int(int key, int bits)
{
    /* 2654435761 ≈ 2^32 × (√5 − 1)/2，Knuth 建議的常數。乘完取最高 bits 位。 */
    unsigned h = (unsigned)key * 2654435761u;
    return bits == 0 ? 0 : h >> (32 - bits);
}

static void *xcalloc(size_t n, size_t sz)
{
    void *p = calloc(n, sz);
    if (!p)
        abort();
    return p;
}

/* ---- 分離鏈結 ---- */

void cm_init(ChainMap *m)
{
    m->bits = 3;
    m->buckets = xcalloc((size_t)1 << m->bits, sizeof *m->buckets);
    m->size = 0;
}

void cm_free(ChainMap *m)
{
    for (int i = 0; i < 1 << m->bits; i++) {
        HNode *n = m->buckets[i];
        while (n) {
            HNode *next = n->next;
            free(n);
            n = next;
        }
    }
    free(m->buckets);
    m->buckets = NULL;
    m->size = 0;
}

static void cm_grow(ChainMap *m)
{
    int old_n = 1 << m->bits;
    HNode **old = m->buckets;
    m->bits++;
    m->buckets = xcalloc((size_t)1 << m->bits, sizeof *m->buckets);
    for (int i = 0; i < old_n; i++) { /* 節點直接搬過去，不用重新 malloc */
        HNode *n = old[i];
        while (n) {
            HNode *next = n->next;
            unsigned b = hash_int(n->key, m->bits);
            n->next = m->buckets[b];
            m->buckets[b] = n;
            n = next;
        }
    }
    free(old);
}

void cm_put(ChainMap *m, int key, int val)
{
    unsigned b = hash_int(key, m->bits);
    for (HNode *n = m->buckets[b]; n; n = n->next)
        if (n->key == key) {
            n->val = val;
            return;
        }
    HNode *n = malloc(sizeof *n);
    if (!n)
        abort();
    n->key = key;
    n->val = val;
    n->next = m->buckets[b]; /* 插在串列頭，O(1) */
    m->buckets[b] = n;
    if (++m->size > 1 << m->bits)
        cm_grow(m);
}

int cm_get(const ChainMap *m, int key, int *val)
{
    for (HNode *n = m->buckets[hash_int(key, m->bits)]; n; n = n->next)
        if (n->key == key) {
            if (val)
                *val = n->val;
            return 1;
        }
    return 0;
}

int cm_remove(ChainMap *m, int key)
{
    for (HNode **p = &m->buckets[hash_int(key, m->bits)]; *p; p = &(*p)->next)
        if ((*p)->key == key) {
            HNode *dead = *p;
            *p = dead->next;
            free(dead);
            m->size--;
            return 1;
        }
    return 0;
}

/* ---- 開放定址 ---- */

static void pm_alloc(ProbeMap *m, int bits)
{
    size_t cap = (size_t)1 << bits;
    m->keys = xcalloc(cap, sizeof *m->keys);
    m->vals = xcalloc(cap, sizeof *m->vals);
    m->state = xcalloc(cap, 1);
    m->bits = bits;
    m->size = 0;
    m->used = 0;
}

void pm_init(ProbeMap *m)
{
    pm_alloc(m, 3);
}

void pm_free(ProbeMap *m)
{
    free(m->keys);
    free(m->vals);
    free(m->state);
    m->keys = m->vals = NULL;
    m->state = NULL;
}

/* 找 key 所在的格子；沒有的話回傳可以放它的格子（遇到的第一個墓碑，或空格）。 */
static int pm_slot(const ProbeMap *m, int key, int *found)
{
    int mask = (1 << m->bits) - 1, tomb = -1;
    for (int i = (int)hash_int(key, m->bits);; i = (i + 1) & mask) { /* 線性探測：下一格，繞回開頭 */
        if (m->state[i] == 0) {
            *found = 0;
            return tomb >= 0 ? tomb : i;
        }
        if (m->state[i] == 2) {
            if (tomb < 0)
                tomb = i; /* 記住，但要繼續找：key 可能在墓碑後面 */
        } else if (m->keys[i] == key) {
            *found = 1;
            return i;
        }
    }
}

static void pm_rebuild(ProbeMap *m, int bits)
{
    ProbeMap old = *m;
    pm_alloc(m, bits);
    for (int i = 0; i < 1 << old.bits; i++)
        if (old.state[i] == 1)
            pm_put(m, old.keys[i], old.vals[i]); /* 墓碑在這裡被清掉 */
    pm_free(&old);
}

void pm_put(ProbeMap *m, int key, int val)
{
    int found, i = pm_slot(m, key, &found);
    if (found) {
        m->vals[i] = val;
        return;
    }
    if (m->state[i] == 0)
        m->used++; /* 用掉一個空格；重用墓碑則 used 不變 */
    m->state[i] = 1;
    m->keys[i] = key;
    m->vals[i] = val;
    m->size++;
    if (m->used * 2 > 1 << m->bits) /* 空格太少，探測會變很長 */
        pm_rebuild(m, m->size * 4 > 1 << m->bits ? m->bits + 1 : m->bits);
}

int pm_get(const ProbeMap *m, int key, int *val)
{
    int found, i = pm_slot(m, key, &found);
    if (found && val)
        *val = m->vals[i];
    return found;
}

int pm_remove(ProbeMap *m, int key)
{
    int found, i = pm_slot(m, key, &found);
    if (!found)
        return 0;
    m->state[i] = 2; /* 墓碑，不能直接設成空的 */
    m->size--;
    return 1;
}
