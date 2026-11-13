/* LeetCode 151 · Reverse Words in a String
 * 思路（原地）：
 *   1. 清掉多餘空白：頭尾不留，字之間只留一個（快慢指標）
 *   2. 整個字串反轉
 *   3. 每個字再各自反轉回來
 *   "  the sky  " → "the sky" → "yks eht" → "sky the"
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

/* ===== 提交範圍 開始 ===== */
static void rev(char *s, int l, int r)
{
    while (l < r) {
        char t = s[l];
        s[l++] = s[r];
        s[r--] = t;
    }
}

char *reverseWords(char *s)
{
    int w = 0; /* 寫入位置 */
    for (int r = 0; s[r];) {
        if (s[r] == ' ') {
            r++;
            continue;
        }
        if (w > 0)
            s[w++] = ' '; /* 字與字之間補一個空白 */
        while (s[r] && s[r] != ' ')
            s[w++] = s[r++];
    }
    s[w] = '\0';

    rev(s, 0, w - 1);
    for (int i = 0; i < w;) {
        int j = i;
        while (j < w && s[j] != ' ')
            j++;
        rev(s, i, j - 1);
        i = j + 1;
    }
    return s;
}
/* ===== 提交範圍 結束 ===== */

int main(void)
{
    char a[] = "the sky is blue";
    assert(strcmp(reverseWords(a), "blue is sky the") == 0);
    char b[] = "  hello world  ";
    assert(strcmp(reverseWords(b), "world hello") == 0);
    char c[] = "a good   example";
    assert(strcmp(reverseWords(c), "example good a") == 0);
    char d[] = "single";
    assert(strcmp(reverseWords(d), "single") == 0);
    puts("0151: passed");
    return 0;
}
