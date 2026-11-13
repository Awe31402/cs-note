/* LeetCode 1143 · Longest Common Subsequence（CLRS 14.4）
 * 思路：c[i][j] = text1 前 i 個字元和 text2 前 j 個字元的 LCS 長度
 *   最後一個字元相同 → c[i-1][j-1] + 1
 *   不同           → max(c[i-1][j], c[i][j-1])
 *   只需要上一列，空間 O(min(n, m))。時間 O(nm)。
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ===== 提交範圍 開始 ===== */
int longestCommonSubsequence(char *text1, char *text2)
{
    int n = (int)strlen(text1), m = (int)strlen(text2);
    int *prev = calloc((size_t)m + 1, sizeof *prev), *cur = calloc((size_t)m + 1, sizeof *cur);
    for (int i = 1; i <= n; i++) {
        for (int j = 1; j <= m; j++)
            cur[j] = text1[i - 1] == text2[j - 1] ? prev[j - 1] + 1 : (prev[j] > cur[j - 1] ? prev[j] : cur[j - 1]);
        int *t = prev;
        prev = cur;
        cur = t;
    }
    int ans = prev[m];
    free(prev);
    free(cur);
    return ans;
}
/* ===== 提交範圍 結束 ===== */

int main(void)
{
    assert(longestCommonSubsequence("abcde", "ace") == 3);
    assert(longestCommonSubsequence("abc", "abc") == 3);
    assert(longestCommonSubsequence("abc", "def") == 0);
    assert(longestCommonSubsequence("ABCBDAB", "BDCABA") == 4); /* CLRS 的例子 */
    assert(longestCommonSubsequence("bl", "yby") == 1);
    puts("1143: passed");
    return 0;
}
