/* LeetCode 242 · Valid Anagram
 * 思路：只有小寫字母，開 26 格的計數陣列。s 的字母 +1，t 的字母 -1，最後全部是 0 就是 anagram。O(n)。
 */
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

/* ===== 提交範圍 開始 ===== */
bool isAnagram(char *s, char *t)
{
    int cnt[26] = {0};
    int i = 0;
    for (; s[i] && t[i]; i++) {
        cnt[s[i] - 'a']++;
        cnt[t[i] - 'a']--;
    }
    if (s[i] || t[i]) /* 長度不同 */
        return false;
    for (int k = 0; k < 26; k++)
        if (cnt[k] != 0)
            return false;
    return true;
}
/* ===== 提交範圍 結束 ===== */

int main(void)
{
    assert(isAnagram("anagram", "nagaram"));
    assert(!isAnagram("rat", "car"));
    assert(!isAnagram("a", "ab"));
    assert(!isAnagram("aacc", "ccac"));
    assert(isAnagram("", ""));
    puts("0242: passed");
    return 0;
}
