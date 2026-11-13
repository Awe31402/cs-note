/* LeetCode 1392 · Longest Happy Prefix
 * 「快樂前綴」= 既是前綴、又是後綴、而且不是整個字串。找最長的。
 * 思路：這就是 KMP 前綴函數的定義本身：答案長度 = pi[n-1]。O(n)。
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ===== 提交範圍 開始 ===== */
static void prefix(const char *p, int m, int *pi)
{
    pi[0] = 0;
    for (int q = 1, k = 0; q < m; q++) {
        while (k > 0 && p[k] != p[q])
            k = pi[k - 1];
        if (p[k] == p[q])
            k++;
        pi[q] = k;
    }
}

char *longestPrefix(char *s)
{
    int n = (int)strlen(s);
    int *pi = malloc((size_t)n * sizeof *pi);
    prefix(s, n, pi);
    int L = pi[n - 1];
    free(pi);
    char *ans = malloc((size_t)L + 1);
    memcpy(ans, s, (size_t)L);
    ans[L] = '\0';
    return ans;
}
/* ===== 提交範圍 結束 ===== */

static void check(char *s, const char *want)
{
    char *r = longestPrefix(s);
    assert(strcmp(r, want) == 0);
    free(r);
}

int main(void)
{
    check("level", "l");
    check("ababab", "abab"); /* 前綴和後綴可以重疊 */
    check("a", "");
    check("abcd", "");
    check("aaaa", "aaa");
    puts("1392: passed");
    return 0;
}
