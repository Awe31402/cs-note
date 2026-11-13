#include "dsu.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static void test_basic(void)
{
    DSU d;
    dsu_init(&d, 10);
    assert(d.sets == 10 && !dsu_same(&d, 1, 2));
    assert(dsu_union(&d, 1, 2) && dsu_union(&d, 3, 4) && dsu_union(&d, 2, 4));
    assert(!dsu_union(&d, 1, 3)); /* 已經同一組 */
    assert(dsu_same(&d, 1, 4) && dsu_size(&d, 3) == 4 && d.sets == 7);
    assert(!dsu_same(&d, 0, 1) && dsu_size(&d, 0) == 1);
    dsu_free(&d);
}

/* 暴力：label[i] = 集合編號，合併時把一邊全部改成另一邊，O(n) */
static void test_random(void)
{
    enum { N = 300 };
    srand(22);
    for (int t = 0; t < 30; t++) {
        DSU d;
        dsu_init(&d, N);
        int label[N], sets = N;
        for (int i = 0; i < N; i++)
            label[i] = i;
        for (int step = 0; step < 2000; step++) {
            int a = rand() % N, b = rand() % N;
            if (rand() % 2) {
                int merged = label[a] != label[b];
                assert(dsu_union(&d, a, b) == merged);
                if (merged) {
                    int old = label[b];
                    for (int i = 0; i < N; i++)
                        if (label[i] == old)
                            label[i] = label[a];
                    sets--;
                }
            }
            assert(dsu_same(&d, a, b) == (label[a] == label[b]));
            int cnt = 0;
            for (int i = 0; i < N; i++)
                cnt += label[i] == label[a];
            assert(dsu_size(&d, a) == cnt && d.sets == sets);
        }
        /* 依秩合併的性質：rank 為 r 的根，集合至少有 2^r 個元素 → rank <= log2(n) */
        for (int i = 0; i < N; i++)
            if (d.parent[i] == i)
                assert((1 << d.rank[i]) <= d.size[i]);
        dsu_free(&d);
    }
}

static void test_long_chain(void)
{
    enum { N = 1000000 };
    DSU d;
    dsu_init(&d, N);
    for (int i = 1; i < N; i++)
        dsu_union(&d, i - 1, i);
    assert(d.sets == 1 && dsu_size(&d, 12345) == N);
    for (int i = 0; i < N; i += 1000)
        assert(dsu_same(&d, 0, i));
    int max_rank = 0;
    for (int i = 0; i < N; i++)
        if (d.rank[i] > max_rank)
            max_rank = d.rank[i];
    assert(max_rank <= (int)log2((double)N));
    dsu_free(&d);
}

int main(void)
{
    test_basic();
    test_random();
    test_long_chain();
    puts("dsu: all tests passed");
    return 0;
}
