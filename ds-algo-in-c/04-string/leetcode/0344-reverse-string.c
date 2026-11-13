/* LeetCode 344 · Reverse String
 * 思路：左右兩個指標往中間走，一路交換。O(n)、O(1) 空間。
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

/* ===== 提交範圍 開始 ===== */
void reverseString(char *s, int sSize)
{
    for (int l = 0, r = sSize - 1; l < r; l++, r--) {
        char t = s[l];
        s[l] = s[r];
        s[r] = t;
    }
}
/* ===== 提交範圍 結束 ===== */

int main(void)
{
    char a[] = {'h', 'e', 'l', 'l', 'o'};
    reverseString(a, 5);
    assert(memcmp(a, "olleh", 5) == 0);
    char b[] = {'H', 'a', 'n', 'n', 'a', 'h'};
    reverseString(b, 6);
    assert(memcmp(b, "hannaH", 6) == 0);
    char c[] = {'x'};
    reverseString(c, 1);
    assert(c[0] == 'x');
    puts("0344: passed");
    return 0;
}
