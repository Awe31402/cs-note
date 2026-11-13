/* LeetCode 706 · Design HashMap（不能用內建雜湊表）
 * 思路：分離鏈結。固定 10007 個 bucket（質數），key % 10007 決定放哪條串列。
 *       key 範圍 0..10^6、最多 10^4 次操作，平均每條很短。
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/* ===== 提交範圍 開始 ===== */
#define NB 10007

typedef struct Entry {
    int key, val;
    struct Entry *next;
} Entry;

typedef struct {
    Entry *b[NB];
} MyHashMap;

MyHashMap *myHashMapCreate(void)
{
    return calloc(1, sizeof(MyHashMap));
}

void myHashMapPut(MyHashMap *m, int key, int value)
{
    Entry **head = &m->b[key % NB];
    for (Entry *e = *head; e; e = e->next)
        if (e->key == key) {
            e->val = value;
            return;
        }
    Entry *e = malloc(sizeof *e);
    e->key = key;
    e->val = value;
    e->next = *head;
    *head = e;
}

int myHashMapGet(MyHashMap *m, int key)
{
    for (Entry *e = m->b[key % NB]; e; e = e->next)
        if (e->key == key)
            return e->val;
    return -1;
}

void myHashMapRemove(MyHashMap *m, int key)
{
    for (Entry **p = &m->b[key % NB]; *p; p = &(*p)->next)
        if ((*p)->key == key) {
            Entry *dead = *p;
            *p = dead->next;
            free(dead);
            return;
        }
}

void myHashMapFree(MyHashMap *m)
{
    for (int i = 0; i < NB; i++)
        while (m->b[i]) {
            Entry *next = m->b[i]->next;
            free(m->b[i]);
            m->b[i] = next;
        }
    free(m);
}
/* ===== 提交範圍 結束 ===== */

int main(void)
{
    MyHashMap *m = myHashMapCreate();
    myHashMapPut(m, 1, 1);
    myHashMapPut(m, 2, 2);
    assert(myHashMapGet(m, 1) == 1);
    assert(myHashMapGet(m, 3) == -1);
    myHashMapPut(m, 2, 1);
    assert(myHashMapGet(m, 2) == 1);
    myHashMapRemove(m, 2);
    assert(myHashMapGet(m, 2) == -1);
    myHashMapPut(m, 1 + NB, 9); /* 跟 1 撞同一個 bucket */
    assert(myHashMapGet(m, 1) == 1 && myHashMapGet(m, 1 + NB) == 9);
    myHashMapFree(m);

    static int model[20000];
    srand(706);
    m = myHashMapCreate();
    for (int i = 0; i < 20000; i++)
        model[i] = -1;
    for (int t = 0; t < 10000; t++) {
        int k = rand() % 20000 * 50; /* 0..10^6，很多碰撞 */
        if (rand() % 3) {
            int v = rand() % 1000000;
            myHashMapPut(m, k, v);
            model[k / 50] = v;
        } else {
            myHashMapRemove(m, k);
            model[k / 50] = -1;
        }
        int q = rand() % 20000;
        assert(myHashMapGet(m, q * 50) == model[q]);
    }
    myHashMapFree(m);
    puts("0706: passed");
    return 0;
}
