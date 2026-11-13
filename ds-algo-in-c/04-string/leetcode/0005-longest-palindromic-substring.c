/* LeetCode 5 · Longest Palindromic Substring
 * 思路：中心擴展 (expand around center)。每個位置當中心往兩邊擴，
 *       中心有 2n-1 個（奇數長度以字元為中心、偶數長度以兩字元之間為中心）。O(n²)、O(1) 空間。
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ===== 提交範圍 開始 ===== */
static int expand(const char *s, int n, int l, int r)
{
    while (l >= 0 && r < n && s[l] == s[r]) {
        l--;
        r++;
    }
    return r - l - 1; /* 停下來時 l、r 已經多走一步 */
}

char *longestPalindrome(char *s)
{
    int n = (int)strlen(s), best_start = 0, best_len = 0;
    for (int i = 0; i < n; i++) {
        int odd = expand(s, n, i, i);      /* "aba" */
        int even = expand(s, n, i, i + 1); /* "abba" */
        int len = odd > even ? odd : even;
        if (len > best_len) {
            best_len = len;
            best_start = i - (len - 1) / 2;
        }
    }
    char *out = malloc((size_t)best_len + 1);
    memcpy(out, s + best_start, (size_t)best_len);
    out[best_len] = '\0';
    return out;
}
/* ===== 提交範圍 結束 ===== */

static int is_pal(const char *s, int l, int r)
{
    while (l < r)
        if (s[l++] != s[r--])
            return 0;
    return 1;
}

int main(void)
{
    char *r = longestPalindrome("babad");
    assert(strcmp(r, "bab") == 0 || strcmp(r, "aba") == 0);
    free(r);
    r = longestPalindrome("cbbd");
    assert(strcmp(r, "bb") == 0);
    free(r);
    r = longestPalindrome("a");
    assert(strcmp(r, "a") == 0);
    free(r);

    /* 隨機：跟 O(n³) 暴力解比「長度」 */
    srand(5);
    for (int t = 0; t < 500; t++) {
        char s[31];
        int n = 1 + rand() % 30;
        for (int i = 0; i < n; i++)
            s[i] = (char)('a' + rand() % 3);
        s[n] = '\0';
        int best = 0;
        for (int i = 0; i < n; i++)
            for (int j = i; j < n; j++)
                if (is_pal(s, i, j) && j - i + 1 > best)
                    best = j - i + 1;
        r = longestPalindrome(s);
        assert((int)strlen(r) == best && is_pal(r, 0, best - 1) && strstr(s, r));
        free(r);
    }
    puts("0005: passed");
    return 0;
}
