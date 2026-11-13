/* LeetCode 720 · Longest Word in Dictionary
 * 找最長的單字，它的每一個前綴（"w", "wo", "wor"...）也都要在字典裡。一樣長就取字典順序最小的。
 * 思路：全部插進 trie，從根做 DFS，只能走「是單字結尾」的節點。
 *       由 a 到 z 走，而且只在「更長」時才更新答案 → 同樣長度時自然留下字典順序最小的。
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

static void dfs(T *n, char *buf, int depth, char *best, int *best_len)
{
    if (depth > *best_len) {
        *best_len = depth;
        memcpy(best, buf, (size_t)depth);
        best[depth] = '\0';
    }
    for (int c = 0; c < 26; c++)
        if (n->child[c] && n->child[c]->end) { /* 下一個前綴也必須是單字 */
            buf[depth] = (char)('a' + c);
            dfs(n->child[c], buf, depth + 1, best, best_len);
        }
}

static void free_t(T *n)
{
    if (!n)
        return;
    for (int c = 0; c < 26; c++)
        free_t(n->child[c]);
    free(n);
}

char *longestWord(char **words, int wordsSize)
{
    T *root = calloc(1, sizeof *root);
    for (int i = 0; i < wordsSize; i++) {
        T *n = root;
        for (const char *p = words[i]; *p; p++) {
            int c = *p - 'a';
            if (!n->child[c])
                n->child[c] = calloc(1, sizeof *n);
            n = n->child[c];
        }
        n->end = true;
    }
    char buf[32], *best = malloc(32); /* 題目：單字長度 <= 30 */
    int best_len = 0;
    best[0] = '\0';
    dfs(root, buf, 0, best, &best_len);
    free_t(root);
    return best;
}
/* ===== 提交範圍 結束 ===== */

static void check(char **w, int n, const char *want)
{
    char *r = longestWord(w, n);
    assert(strcmp(r, want) == 0);
    free(r);
}

int main(void)
{
    char *a[] = {"w", "wo", "wor", "worl", "world"};
    check(a, 5, "world");
    char *b[] = {"a", "banana", "app", "appl", "ap", "apply", "apple"};
    check(b, 7, "apple"); /* apple 和 apply 一樣長，取字典順序小的 */
    char *c[] = {"yo", "ew", "fc", "zrc", "yodn", "fcm", "qm", "qmo", "fcmz", "z", "ewq", "yod", "ewqz", "y"};
    check(c, 14, "yodn");
    char *d[] = {"abc"}; /* "a" 不在字典裡 */
    check(d, 1, "");
    puts("0720: passed");
    return 0;
}
