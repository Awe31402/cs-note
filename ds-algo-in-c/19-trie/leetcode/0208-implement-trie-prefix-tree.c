/* LeetCode 208 · Implement Trie (Prefix Tree)
 * 思路：每個節點 26 個小孩指標 + 一個「是不是單字結尾」的旗標。
 *   insert：沒有路就建路；search：走得到而且是結尾；startsWith：走得到就好。
 */
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

/* ===== 提交範圍 開始 ===== */
typedef struct Trie {
    struct Trie *child[26];
    bool end;
} Trie;

Trie *trieCreate(void)
{
    return calloc(1, sizeof(Trie));
}

void trieInsert(Trie *t, char *word)
{
    for (; *word; word++) {
        int c = *word - 'a';
        if (!t->child[c])
            t->child[c] = trieCreate();
        t = t->child[c];
    }
    t->end = true;
}

static Trie *walk(Trie *t, const char *s)
{
    for (; *s && t; s++)
        t = t->child[*s - 'a'];
    return t;
}

bool trieSearch(Trie *t, char *word)
{
    Trie *n = walk(t, word);
    return n && n->end;
}

bool trieStartsWith(Trie *t, char *prefix)
{
    return walk(t, prefix) != NULL;
}

void trieFree(Trie *t)
{
    if (!t)
        return;
    for (int c = 0; c < 26; c++)
        trieFree(t->child[c]);
    free(t);
}
/* ===== 提交範圍 結束 ===== */

int main(void)
{
    Trie *t = trieCreate();
    trieInsert(t, "apple");
    assert(trieSearch(t, "apple"));
    assert(!trieSearch(t, "app")); /* 是前綴，但不是單字 */
    assert(trieStartsWith(t, "app"));
    trieInsert(t, "app");
    assert(trieSearch(t, "app"));
    assert(!trieStartsWith(t, "b"));
    assert(!trieSearch(t, "applex"));
    trieFree(t);
    puts("0208: passed");
    return 0;
}
