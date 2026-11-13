/* LeetCode 14 · Longest Common Prefix
 * 思路（垂直掃描）：一欄一欄比，第 i 個字元只要有一個字串不同（或已經結束）就停。
 *       O(S)，S = 所有字元總數，不用額外空間。
 *       Trie 的做法：全部插進去，從根往下走，直到某節點有兩個以上的小孩或是單字結尾 —— 見 README。
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ===== 提交範圍 開始 ===== */
char *longestCommonPrefix(char **strs, int strsSize)
{
    int len = 0;
    for (;; len++) {
        char c = strs[0][len];
        if (c == '\0')
            break;
        int ok = 1;
        for (int i = 1; i < strsSize && ok; i++)
            ok = strs[i][len] == c; /* strs[i] 比較短時，這裡讀到的是 '\0'，一定不相等 */
        if (!ok)
            break;
    }
    char *ans = malloc((size_t)len + 1);
    memcpy(ans, strs[0], (size_t)len);
    ans[len] = '\0';
    return ans;
}
/* ===== 提交範圍 結束 ===== */

static void check(char **s, int n, const char *want)
{
    char *r = longestCommonPrefix(s, n);
    assert(strcmp(r, want) == 0);
    free(r);
}

int main(void)
{
    char *a[] = {"flower", "flow", "flight"};
    check(a, 3, "fl");
    char *b[] = {"dog", "racecar", "car"};
    check(b, 3, "");
    char *c[] = {"alone"};
    check(c, 1, "alone");
    char *d[] = {"ab", "a"};
    check(d, 2, "a");
    char *e[] = {"", "abc"};
    check(e, 2, "");
    puts("0014: passed");
    return 0;
}
