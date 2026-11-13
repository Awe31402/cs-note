/* LeetCode 20 · Valid Parentheses
 * 思路：遇到左括號就 push；遇到右括號就 pop，檢查是不是對應的左括號。最後 stack 要是空的。
 */
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ===== 提交範圍 開始 ===== */
bool isValid(char *s)
{
    int n = (int)strlen(s), top = 0;
    char *st = malloc((size_t)n + 1); /* 最多 n 個左括號 */
    bool ok = true;
    for (int i = 0; i < n && ok; i++) {
        char c = s[i];
        if (c == '(' || c == '[' || c == '{') {
            st[top++] = c;
        } else {
            char want = c == ')' ? '(' : c == ']' ? '[' : '{';
            ok = top > 0 && st[--top] == want;
        }
    }
    free(st);
    return ok && top == 0;
}
/* ===== 提交範圍 結束 ===== */

int main(void)
{
    assert(isValid("()"));
    assert(isValid("()[]{}"));
    assert(!isValid("(]"));
    assert(isValid("([])"));
    assert(!isValid("([)]"));
    assert(!isValid("("));
    assert(!isValid("]"));
    assert(isValid("{[]}(({}))"));
    puts("0020: passed");
    return 0;
}
