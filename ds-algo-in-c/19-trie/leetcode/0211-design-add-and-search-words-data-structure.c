/* LeetCode 211 · Design Add and Search Words Data Structure
 * search 的字串可能有 '.'，代表「任何一個字母」。
 * 思路：trie。遇到一般字母就走那一條；遇到 '.' 就把 26 個小孩都試一遍（DFS）。
 */
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

/* ===== 提交範圍 開始 ===== */
typedef struct WordDictionary {
    struct WordDictionary *child[26];
    bool end;
} WordDictionary;

WordDictionary *wordDictionaryCreate(void)
{
    return calloc(1, sizeof(WordDictionary));
}

void wordDictionaryAddWord(WordDictionary *d, char *word)
{
    for (; *word; word++) {
        int c = *word - 'a';
        if (!d->child[c])
            d->child[c] = wordDictionaryCreate();
        d = d->child[c];
    }
    d->end = true;
}

static bool match(const WordDictionary *d, const char *w)
{
    for (; *w; w++) {
        if (*w == '.') {
            for (int c = 0; c < 26; c++)
                if (d->child[c] && match(d->child[c], w + 1))
                    return true;
            return false;
        }
        d = d->child[*w - 'a'];
        if (!d)
            return false;
    }
    return d->end;
}

bool wordDictionarySearch(WordDictionary *d, char *word)
{
    return match(d, word);
}

void wordDictionaryFree(WordDictionary *d)
{
    if (!d)
        return;
    for (int c = 0; c < 26; c++)
        wordDictionaryFree(d->child[c]);
    free(d);
}
/* ===== 提交範圍 結束 ===== */

int main(void)
{
    WordDictionary *d = wordDictionaryCreate();
    wordDictionaryAddWord(d, "bad");
    wordDictionaryAddWord(d, "dad");
    wordDictionaryAddWord(d, "mad");
    assert(!wordDictionarySearch(d, "pad"));
    assert(wordDictionarySearch(d, "bad"));
    assert(wordDictionarySearch(d, ".ad"));
    assert(wordDictionarySearch(d, "b.."));
    assert(!wordDictionarySearch(d, "b..."));
    assert(!wordDictionarySearch(d, ".."));
    assert(wordDictionarySearch(d, "..."));
    wordDictionaryAddWord(d, "a");
    assert(wordDictionarySearch(d, "."));
    wordDictionaryFree(d);
    puts("0211: passed");
    return 0;
}
