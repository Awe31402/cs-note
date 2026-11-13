/* LeetCode 459 · Repeated Substring Pattern
 * s 能不能由某個子字串重複好幾次組成？（例如 "abcabc" = "abc" × 2）
 * 思路：前綴函數。L = pi[n-1] 是「最長的前綴 = 後綴」。如果 s 是週期字串，最小週期就是 n - L。
 *       成立條件：L > 0 而且 n 可以被 (n - L) 整除。O(n)。
 */
#include <assert.h>
#include <stdbool.h>
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

bool repeatedSubstringPattern(char *s)
{
    int n = (int)strlen(s);
    int *pi = malloc((size_t)n * sizeof *pi);
    prefix(s, n, pi);
    int L = pi[n - 1];
    free(pi);
    return L > 0 && n % (n - L) == 0;
}
/* ===== 提交範圍 結束 ===== */

static bool brute(const char *s)
{
    int n = (int)strlen(s);
    for (int len = 1; len <= n / 2; len++) {
        if (n % len)
            continue;
        int ok = 1;
        for (int i = len; i < n && ok; i++)
            ok = s[i] == s[i - len];
        if (ok)
            return true;
    }
    return false;
}

int main(void)
{
    assert(repeatedSubstringPattern("abab"));
    assert(!repeatedSubstringPattern("aba"));
    assert(repeatedSubstringPattern("abcabcabcabc"));
    assert(!repeatedSubstringPattern("a"));
    assert(!repeatedSubstringPattern("abaababaab" "a")); /* pi 最後是 >0，但長度不整除 */
    srand(459);
    for (int t = 0; t < 3000; t++) {
        char s[40];
        int n = 1 + rand() % 12, rep = 1 + rand() % 3, k = 0;
        char unit[13];
        for (int i = 0; i < n; i++)
            unit[i] = (char)('a' + rand() % 2);
        for (int r = 0; r < rep; r++)
            for (int i = 0; i < n; i++)
                s[k++] = unit[i];
        if (rand() % 2 && k > 1)
            s[rand() % k] = (char)('a' + rand() % 2); /* 有時候故意破壞 */
        s[k] = '\0';
        assert(repeatedSubstringPattern(s) == brute(s));
    }
    puts("0459: passed");
    return 0;
}
