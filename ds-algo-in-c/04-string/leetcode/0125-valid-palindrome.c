/* LeetCode 125 · Valid Palindrome
 * 思路：左右指標，各自跳過非英數字元，比較時忽略大小寫。O(n)、O(1) 空間。
 */
#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>

/* ===== 提交範圍 開始 ===== */
bool isPalindrome(char *s)
{
    int l = 0, r = 0;
    while (s[r])
        r++;
    r--;
    while (l < r) {
        /* ctype 函式的參數要先轉 unsigned char，負的 char 是未定義行為 */
        if (!isalnum((unsigned char)s[l])) {
            l++;
        } else if (!isalnum((unsigned char)s[r])) {
            r--;
        } else {
            if (tolower((unsigned char)s[l]) != tolower((unsigned char)s[r]))
                return false;
            l++;
            r--;
        }
    }
    return true;
}
/* ===== 提交範圍 結束 ===== */

int main(void)
{
    char a[] = "A man, a plan, a canal: Panama";
    assert(isPalindrome(a));
    char b[] = "race a car";
    assert(!isPalindrome(b));
    char c[] = " ";
    assert(isPalindrome(c));
    char d[] = "0P"; /* 數字和字母不能混為一談 */
    assert(!isPalindrome(d));
    char e[] = "";
    assert(isPalindrome(e));
    puts("0125: passed");
    return 0;
}
