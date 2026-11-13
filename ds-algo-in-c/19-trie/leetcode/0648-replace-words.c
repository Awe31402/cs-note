/* LeetCode 648 · Replace Words
 * 把句子裡每個字換成字典裡「最短的字根 (root)」—— 如果它有字根是它的前綴的話。
 * 思路：字根全部插進 trie。每個字從 trie 根往下走，第一次碰到 end 就是最短字根；
 *       走不下去就代表沒有字根，保留原字。O(句子長度 + 字典總長)。
 */
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ===== 提交範圍 開始 ===== */
typedef struct T {
    struct T *child[26];
    bool end;
} T;

static void free_t(T *n)
{
    if (!n)
        return;
    for (int c = 0; c < 26; c++)
        free_t(n->child[c]);
    free(n);
}

char *replaceWords(char **dictionary, int dictionarySize, char *sentence)
{
    T *root = calloc(1, sizeof *root);
    for (int i = 0; i < dictionarySize; i++) {
        T *n = root;
        for (const char *p = dictionary[i]; *p; p++) {
            int c = *p - 'a';
            if (!n->child[c])
                n->child[c] = calloc(1, sizeof *n);
            n = n->child[c];
        }
        n->end = true;
    }
    size_t len = strlen(sentence);
    char *out = malloc(len + 1); /* 替換只會變短 */
    size_t k = 0;
    for (size_t i = 0; i < len;) {
        size_t start = i;
        while (i < len && sentence[i] != ' ')
            i++; /* [start, i) 是一個字 */
        T *n = root;
        size_t cut = i; /* 預設：沒有字根，整個字保留 */
        for (size_t j = start; j < i && n; j++) {
            n = n->child[sentence[j] - 'a'];
            if (n && n->end) {
                cut = j + 1; /* 第一個碰到的就是最短字根 */
                break;
            }
        }
        memcpy(out + k, sentence + start, cut - start);
        k += cut - start;
        if (i < len)
            out[k++] = sentence[i++]; /* 空白 */
    }
    out[k] = '\0';
    free_t(root);
    return out;
}
/* ===== 提交範圍 結束 ===== */

static void check(char **dict, int n, char *s, const char *want)
{
    char *r = replaceWords(dict, n, s);
    assert(strcmp(r, want) == 0);
    free(r);
}

int main(void)
{
    char *d1[] = {"cat", "bat", "rat"};
    check(d1, 3, "the cattle was rattled by the battery", "the cat was rat by the bat");
    char *d2[] = {"a", "b", "c"};
    check(d2, 3, "aadsfasf absbs bbab cadsfafs", "a a b c");
    char *d3[] = {"catt", "cat", "bat", "rat"}; /* 有兩個字根：取短的 cat */
    check(d3, 4, "the cattle", "the cat");
    char *d4[] = {"xyz"};
    check(d4, 1, "hello world", "hello world");
    puts("0648: passed");
    return 0;
}
