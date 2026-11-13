#ifndef TRIE_H
#define TRIE_H

/* 字典樹 (Trie / prefix tree)，只收小寫字母 a-z。 */
typedef struct TrieNode {
    struct TrieNode *child[26];
    int pass; /* 有幾個單字經過這個節點（= 以「到這裡為止的字串」為前綴的單字數） */
    int end;  /* 有幾個單字剛好在這裡結束（本專案不存重複單字，所以是 0 或 1） */
} TrieNode;

typedef struct {
    TrieNode *root;
    int words;
} Trie;

void trie_init(Trie *t);
void trie_free(Trie *t);
int  trie_insert(Trie *t, const char *w);           /* 新單字回傳 1，已存在 0 */
int  trie_contains(const Trie *t, const char *w);
int  trie_count_prefix(const Trie *t, const char *p); /* 以 p 開頭的單字數，O(|p|) */
int  trie_delete(Trie *t, const char *w);           /* 有刪回傳 1；沒人經過的節點會被釋放 */

/* 自動完成 (autocomplete)：以 p 開頭的單字，依字典順序寫進 out（每個最多 maxlen-1 字元），
 * 最多 cap 個，回傳個數。 */
int trie_complete(const Trie *t, const char *p, char (*out)[32], int cap);

#endif
