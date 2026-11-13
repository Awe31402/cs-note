#include "linked_list.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void expect_sl(const Node *h, const int *want, int n)
{
    int got[64];
    assert(sl_len(h) == n);
    assert(sl_to_array(h, got, 64) == n);
    assert(n == 0 || memcmp(got, want, (size_t)n * sizeof *got) == 0);
}

static void test_singly(void)
{
    Node *h = NULL;
    expect_sl(h, NULL, 0);
    sl_push_back(&h, 2);
    sl_push_front(&h, 1);
    sl_push_back(&h, 4);
    assert(sl_insert_at(&h, 2, 3) == 0);
    assert(sl_insert_at(&h, 0, 0) == 0);
    assert(sl_insert_at(&h, 5, 5) == 0); /* 插在最尾巴 */
    assert(sl_insert_at(&h, 9, 9) == -1);
    int w1[] = {0, 1, 2, 3, 4, 5};
    expect_sl(h, w1, 6);

    assert(sl_find(h, 3) && sl_find(h, 3)->val == 3);
    assert(sl_find(h, 42) == NULL);

    assert(sl_remove(&h, 0) == 1); /* 頭 */
    assert(sl_remove(&h, 3) == 1); /* 中 */
    assert(sl_remove(&h, 5) == 1); /* 尾 */
    assert(sl_remove(&h, 7) == 0);
    int w2[] = {1, 2, 4};
    expect_sl(h, w2, 3);

    sl_reverse(&h);
    int w3[] = {4, 2, 1};
    expect_sl(h, w3, 3);
    sl_free(&h);
    assert(h == NULL);
    sl_reverse(&h); /* 空串列 */
    assert(h == NULL);
}

static void expect_dl(const DList *l, const int *want, int n)
{
    int got[64];
    assert(l->size == n);
    assert(dl_to_array(l, got, 64) == n);
    assert(n == 0 || memcmp(got, want, (size_t)n * sizeof *got) == 0);
    /* 反向走一次，確認 prev 都接對 */
    const DNode *p = l->head.prev;
    for (int i = n - 1; i >= 0; i--, p = p->prev)
        assert(p->val == want[i]);
    assert(p == &l->head);
}

static void test_doubly(void)
{
    DList l;
    dl_init(&l);
    expect_dl(&l, NULL, 0);
    dl_push_back(&l, 2);
    dl_push_front(&l, 1);
    DNode *three = dl_push_back(&l, 3);
    dl_push_back(&l, 4);
    int w1[] = {1, 2, 3, 4};
    expect_dl(&l, w1, 4);

    dl_remove(&l, three); /* O(1) 刪中間 */
    int w2[] = {1, 2, 4};
    expect_dl(&l, w2, 3);

    assert(dl_pop_front(&l) == 1);
    assert(dl_pop_back(&l) == 4);
    assert(dl_find(&l, 2) != NULL && dl_find(&l, 9) == NULL);
    assert(dl_pop_back(&l) == 2);
    expect_dl(&l, NULL, 0);
    dl_free(&l);
}

/* 隨機操作，拿普通陣列當標準答案 */
static void test_random(void)
{
    srand(5);
    DList l;
    dl_init(&l);
    Node *h = NULL;
    int model[64], n = 0;
    for (int t = 0; t < 3000; t++) {
        int op = rand() % 4, x = rand() % 10;
        if (op == 0 && n < 60) {
            dl_push_front(&l, x);
            sl_push_front(&h, x);
            memmove(model + 1, model, (size_t)n * sizeof *model);
            model[0] = x;
            n++;
        } else if (op == 1 && n < 60) {
            dl_push_back(&l, x);
            sl_push_back(&h, x);
            model[n++] = x;
        } else if (op == 2 && n > 0) {
            assert(dl_pop_front(&l) == model[0]);
            assert(sl_remove(&h, model[0]) == 1); /* 頭一定是第一個出現的 model[0] */
            memmove(model, model + 1, (size_t)(n - 1) * sizeof *model);
            n--;
        } else if (op == 3 && n > 0) {
            assert(dl_pop_back(&l) == model[n - 1]);
            n--;
            sl_free(&h); /* 單向串列刪尾巴很麻煩，直接重建 */
            for (int i = n - 1; i >= 0; i--)
                sl_push_front(&h, model[i]);
        }
        expect_dl(&l, model, n);
        expect_sl(h, model, n);
    }
    dl_free(&l);
    sl_free(&h);
}

int main(void)
{
    test_singly();
    test_doubly();
    test_random();
    puts("linked_list: all tests passed");
    return 0;
}
