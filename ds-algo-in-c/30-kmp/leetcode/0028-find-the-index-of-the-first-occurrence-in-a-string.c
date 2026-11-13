/* LeetCode 28 · Find the Index of the First Occurrence in a String
 * 思路：KMP。先算 needle 的前綴函數 pi，再掃 haystack 一次。O(n + m)。
 *       暴力法 O(nm) 在這題的限制（10^4）下也會過，但面試會問 KMP。
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

int strStr(char *haystack, char *needle)
{
    int n = (int)strlen(haystack), m = (int)strlen(needle);
    int *pi = malloc((size_t)m * sizeof *pi), ans = -1;
    prefix(needle, m, pi);
    for (int i = 0, q = 0; i < n; i++) {
        while (q > 0 && needle[q] != haystack[i])
            q = pi[q - 1];
        if (needle[q] == haystack[i])
            q++;
        if (q == m) {
            ans = i - m + 1;
            break;
        }
    }
    free(pi);
    return ans;
}
/* ===== 提交範圍 結束 ===== */

int main(void)
{
    assert(strStr("sadbutsad", "sad") == 0);
    assert(strStr("leetcode", "leeto") == -1);
    assert(strStr("mississippi", "issip") == 4); /* 第一次在 i=1 對到 issis 就失敗，要靠 pi 退回 */
    assert(strStr("aaaaab", "aab") == 3);
    assert(strStr("a", "a") == 0);
    puts("0028: passed");
    return 0;
}
