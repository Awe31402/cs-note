/* LeetCode 214 · Shortest Palindrome
 * 只能在 s 的「前面」加字元，把它變成回文。最短的結果是什麼？
 * 思路：關鍵是找 s 的「最長回文前綴」P，把剩下的部分反轉加在前面就好。
 *   找最長回文前綴：組 t = s + "#" + reverse(s)，算前綴函數，pi 最後一個值就是答案長度。
 *   （P 是回文 ⇔ P 是 s 的前綴、也是 reverse(s) 的後綴。"#" 確保不會跨過中間。）O(n)。
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

char *shortestPalindrome(char *s)
{
    int n = (int)strlen(s), m = 2 * n + 1;
    char *t = malloc((size_t)m + 1);
    memcpy(t, s, (size_t)n);
    t[n] = '#';
    for (int i = 0; i < n; i++)
        t[n + 1 + i] = s[n - 1 - i];
    t[m] = '\0';
    int *pi = malloc((size_t)m * sizeof *pi);
    prefix(t, m, pi);
    int L = pi[m - 1]; /* s[0..L) 是最長回文前綴 */
    free(pi);
    free(t);
    int add = n - L;
    char *ans = malloc((size_t)(add + n) + 1);
    for (int i = 0; i < add; i++)
        ans[i] = s[n - 1 - i]; /* 剩下的尾巴反過來放到前面 */
    memcpy(ans + add, s, (size_t)n + 1);
    return ans;
}
/* ===== 提交範圍 結束 ===== */

static void check(char *s, const char *want)
{
    char *r = shortestPalindrome(s);
    assert(strcmp(r, want) == 0);
    free(r);
}

int main(void)
{
    check("aacecaaa", "aaacecaaa");
    check("abcd", "dcbabcd");
    check("", "");
    check("a", "a");
    check("aba", "aba");
    check("aabba", "abbaabba");
    puts("0214: passed");
    return 0;
}
