/* LeetCode 22 · Generate Parentheses（1 <= n <= 8）
 * 思路：回溯 (backtracking)。一格一格填字元：
 *   - 還能放 '('：open < n
 *   - 還能放 ')'：close < open（右括號不能比左括號多）
 *   填滿 2n 格就是一個答案。
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ===== 提交範圍 開始 ===== */
typedef struct {
    char **out;
    int size, cap;
    char *buf;
    int n;
} Ctx;

static void dfs(Ctx *c, int pos, int open, int close)
{
    if (pos == 2 * c->n) {
        if (c->size == c->cap) {
            c->cap = c->cap ? c->cap * 2 : 16;
            c->out = realloc(c->out, (size_t)c->cap * sizeof *c->out);
        }
        c->buf[pos] = '\0';
        c->out[c->size] = malloc((size_t)pos + 1);
        memcpy(c->out[c->size], c->buf, (size_t)pos + 1);
        c->size++;
        return;
    }
    if (open < c->n) {
        c->buf[pos] = '(';
        dfs(c, pos + 1, open + 1, close);
    }
    if (close < open) {
        c->buf[pos] = ')';
        dfs(c, pos + 1, open, close + 1);
    }
    /* 不用「復原」buf[pos]：下一次會直接覆蓋 */
}

char **generateParenthesis(int n, int *returnSize)
{
    char buf[17]; /* 2 * 8 + 1 */
    Ctx c = {NULL, 0, 0, buf, n};
    dfs(&c, 0, 0, 0);
    *returnSize = c.size;
    return c.out;
}
/* ===== 提交範圍 結束 ===== */

static int valid(const char *s)
{
    int bal = 0;
    for (; *s; s++) {
        bal += *s == '(' ? 1 : -1;
        if (bal < 0)
            return 0;
    }
    return bal == 0;
}

static int cmp_str(const void *a, const void *b)
{
    return strcmp(*(char *const *)a, *(char *const *)b);
}

int main(void)
{
    int catalan[] = {0, 1, 2, 5, 14, 42, 132, 429, 1430}; /* 答案數 = 第 n 個 Catalan 數 */
    for (int n = 1; n <= 8; n++) {
        int m;
        char **res = generateParenthesis(n, &m);
        assert(m == catalan[n]);
        qsort(res, (size_t)m, sizeof *res, cmp_str);
        for (int i = 0; i < m; i++) {
            assert((int)strlen(res[i]) == 2 * n);
            assert(valid(res[i]));
            assert(i == 0 || strcmp(res[i - 1], res[i]) != 0); /* 不重複 */
        }
        for (int i = 0; i < m; i++)
            free(res[i]);
        free(res);
    }

    int m;
    char **res = generateParenthesis(3, &m);
    const char *want[] = {"((()))", "(()())", "(())()", "()(())", "()()()"};
    for (int i = 0; i < 5; i++)
        assert(strcmp(res[i], want[i]) == 0); /* 先放 '(' → 產生順序就是字典序 */
    for (int i = 0; i < m; i++)
        free(res[i]);
    free(res);

    puts("0022: passed");
    return 0;
}
