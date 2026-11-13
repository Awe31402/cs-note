#include "queue.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

static void test_circular(void)
{
    CQueue q;
    cq_init(&q, 3);
    assert(cq_empty(&q) && !cq_full(&q));
    assert(cq_push(&q, 1) == 0 && cq_push(&q, 2) == 0 && cq_push(&q, 3) == 0);
    assert(cq_full(&q) && cq_push(&q, 4) == -1);
    assert(cq_pop(&q) == 1);
    assert(cq_push(&q, 4) == 0); /* 繞回陣列開頭 */
    assert(cq_pop(&q) == 2 && cq_pop(&q) == 3 && cq_front(&q) == 4 && cq_pop(&q) == 4);
    assert(cq_empty(&q));
    cq_free(&q);
}

/* CQueue、LQueue 隨機操作，拿一個很大的陣列當標準答案 */
static void test_random_queues(void)
{
    CQueue c;
    LQueue l;
    cq_init(&c, 7);
    lq_init(&l);
    static int model[20000];
    int h = 0, t = 0;
    srand(7);
    for (int k = 0; k < 20000; k++) {
        if (h == t || (t - h < 7 && rand() % 2)) {
            int x = rand();
            assert(cq_push(&c, x) == 0);
            lq_push(&l, x);
            model[t++] = x;
        } else {
            assert(cq_front(&c) == model[h]);
            assert(cq_pop(&c) == model[h]);
            assert(lq_pop(&l) == model[h]);
            h++;
        }
        assert(c.count == t - h && l.size == t - h);
        assert(cq_full(&c) == (t - h == 7));
        assert((l.head == NULL) == (l.tail == NULL));
    }
    cq_free(&c);
    lq_free(&l);
}

static void test_deque(void)
{
    Deque d;
    dq_init(&d);
    static int model[40000];
    int h = 20000, t = 20000; /* model[h..t-1]，兩邊都能長 */
    srand(77);
    for (int k = 0; k < 20000; k++) {
        int op = rand() % 4, x = rand();
        if (op == 0) {
            dq_push_front(&d, x);
            model[--h] = x;
        } else if (op == 1) {
            dq_push_back(&d, x);
            model[t++] = x;
        } else if (op == 2 && t > h) {
            assert(dq_pop_front(&d) == model[h++]);
        } else if (op == 3 && t > h) {
            assert(dq_pop_back(&d) == model[--t]);
        }
        assert(d.size == t - h);
        if (t > h) {
            assert(dq_front(&d) == model[h] && dq_back(&d) == model[t - 1]);
            int i = rand() % (t - h);
            assert(dq_get(&d, i) == model[h + i]);
        }
    }
    dq_free(&d);
}

int main(void)
{
    test_circular();
    test_random_queues();
    test_deque();
    puts("queue: all tests passed");
    return 0;
}
