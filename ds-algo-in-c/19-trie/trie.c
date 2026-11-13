#include "trie.h"

#include <stdlib.h>
#include <string.h>

static TrieNode *new_node(void)
{
    TrieNode *n = calloc(1, sizeof *n);
    if (!n)
        abort();
    return n;
}

void trie_init(Trie *t)
{
    t->root = new_node();
    t->words = 0;
}

static void free_rec(TrieNode *n)
{
    if (!n)
        return;
    for (int c = 0; c < 26; c++)
        free_rec(n->child[c]);
    free(n);
}

void trie_free(Trie *t)
{
    free_rec(t->root);
    t->root = NULL;
    t->words = 0;
}

/* 沿著 s 往下走，走不下去回傳 NULL */
static TrieNode *walk(const Trie *t, const char *s)
{
    TrieNode *n = t->root;
    for (; *s && n; s++)
        n = n->child[*s - 'a'];
    return n;
}

int trie_insert(Trie *t, const char *w)
{
    if (trie_contains(t, w))
        return 0; /* 先確認不存在，pass 計數才不會多加 */
    TrieNode *n = t->root;
    n->pass++;
    for (; *w; w++) {
        int c = *w - 'a';
        if (!n->child[c])
            n->child[c] = new_node();
        n = n->child[c];
        n->pass++;
    }
    n->end = 1;
    t->words++;
    return 1;
}

int trie_contains(const Trie *t, const char *w)
{
    TrieNode *n = walk(t, w);
    return n && n->end; /* 走得到還不夠，要剛好是單字的結尾 */
}

int trie_count_prefix(const Trie *t, const char *p)
{
    TrieNode *n = walk(t, p);
    return n ? n->pass : 0;
}

int trie_delete(Trie *t, const char *w)
{
    if (!trie_contains(t, w))
        return 0;
    TrieNode *n = t->root;
    n->pass--;
    for (; *w; w++) {
        TrieNode **slot = &n->child[*w - 'a'];
        if (--(*slot)->pass == 0) { /* 沒有其他單字經過了：整條剪掉 */
            free_rec(*slot);
            *slot = NULL;
            t->words--;
            return 1;
        }
        n = *slot;
    }
    n->end = 0;
    t->words--;
    return 1;
}

static void collect(const TrieNode *n, char *buf, int len, char (*out)[32], int cap, int *k)
{
    if (*k >= cap)
        return;
    if (n->end) {
        buf[len] = '\0';
        memcpy(out[(*k)++], buf, (size_t)len + 1);
    }
    if (len >= 30)
        return; /* 放不下更長的了 */
    for (int c = 0; c < 26; c++) /* 由 a 到 z：前序走訪就是字典順序 */
        if (n->child[c]) {
            buf[len] = (char)('a' + c);
            collect(n->child[c], buf, len + 1, out, cap, k);
        }
}

int trie_complete(const Trie *t, const char *p, char (*out)[32], int cap)
{
    TrieNode *n = walk(t, p);
    if (!n)
        return 0;
    char buf[32];
    int len = (int)strlen(p), k = 0;
    if (len > 30)
        return 0;
    memcpy(buf, p, (size_t)len);
    collect(n, buf, len, out, cap, &k);
    return k;
}
