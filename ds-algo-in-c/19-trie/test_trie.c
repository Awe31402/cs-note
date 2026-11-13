#include "trie.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void test_example(void)
{
    Trie t;
    trie_init(&t);
    const char *w[] = {"apple", "app", "apply", "ape", "bat", "bath"};
    for (int i = 0; i < 6; i++)
        assert(trie_insert(&t, w[i]) == 1);
    assert(trie_insert(&t, "app") == 0);
    assert(trie_contains(&t, "app") && !trie_contains(&t, "ap") && !trie_contains(&t, "apples"));
    assert(trie_count_prefix(&t, "ap") == 4 && trie_count_prefix(&t, "app") == 3);
    assert(trie_count_prefix(&t, "") == 6 && trie_count_prefix(&t, "c") == 0);

    char out[10][32];
    int n = trie_complete(&t, "ap", out, 10);
    const char *want[] = {"ape", "app", "apple", "apply"}; /* 字典順序 */
    assert(n == 4);
    for (int i = 0; i < 4; i++)
        assert(strcmp(out[i], want[i]) == 0);

    assert(trie_delete(&t, "app") == 1);  /* 中間的單字：節點要留著給 apple、apply */
    assert(!trie_contains(&t, "app") && trie_contains(&t, "apple"));
    assert(trie_delete(&t, "bath") == 1); /* 只剪掉 'h' */
    assert(trie_contains(&t, "bat") && trie_count_prefix(&t, "bat") == 1);
    assert(trie_delete(&t, "bat") == 1);  /* 整條 b-a-t 都剪掉 */
    assert(t.root->child['b' - 'a'] == NULL);
    assert(trie_delete(&t, "zzz") == 0);
    assert(t.words == 3);
    trie_free(&t);
}

static void random_word(char *w)
{
    int len = 1 + rand() % 5;
    for (int i = 0; i < len; i++)
        w[i] = (char)('a' + rand() % 3); /* 只用 a-c，前綴會大量重疊 */
    w[len] = '\0';
}

static void test_random(void)
{
    enum { M = 400 };
    static char words[M][8];
    int n = 0;
    Trie t;
    trie_init(&t);
    srand(19);
    for (int step = 0; step < 20000; step++) {
        char w[8];
        random_word(w);
        int idx = -1;
        for (int i = 0; i < n; i++)
            if (strcmp(words[i], w) == 0)
                idx = i;
        if (rand() % 2) {
            assert(trie_insert(&t, w) == (idx < 0));
            if (idx < 0 && n < M)
                strcpy(words[n++], w);
            else if (idx < 0)
                trie_delete(&t, w); /* 模型滿了就不收 */
        } else {
            assert(trie_delete(&t, w) == (idx >= 0));
            if (idx >= 0)
                memmove(words[idx], words[--n], sizeof words[0]); /* idx 可能就是最後一個：來源和目的地重疊 */
        }
        assert(t.words == n);
        char p[8];
        random_word(p);
        p[rand() % strlen(p) + 1] = '\0';
        int cnt = 0;
        for (int i = 0; i < n; i++)
            cnt += strncmp(words[i], p, strlen(p)) == 0;
        assert(trie_count_prefix(&t, p) == cnt);
    }
    trie_free(&t);
}

int main(void)
{
    test_example();
    test_random();
    puts("trie: all tests passed");
    return 0;
}
