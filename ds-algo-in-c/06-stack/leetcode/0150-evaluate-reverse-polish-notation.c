/* LeetCode 150 · Evaluate Reverse Polish Notation（後序式求值）
 * 思路：數字 push；遇到運算子就 pop 兩個（先彈出的是右運算元）、算完再 push。
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ===== 提交範圍 開始 ===== */
int evalRPN(char **tokens, int tokensSize)
{
    long long *st = malloc((size_t)tokensSize * sizeof *st);
    int top = 0;
    for (int i = 0; i < tokensSize; i++) {
        char *t = tokens[i];
        /* "-3" 是數字不是減號：長度 1 且是運算子字元才算運算子 */
        if (t[1] == '\0' && strchr("+-*/", t[0])) {
            long long b = st[--top], a = st[--top];
            switch (t[0]) {
            case '+': st[top++] = a + b; break;
            case '-': st[top++] = a - b; break;
            case '*': st[top++] = a * b; break;
            default:  st[top++] = a / b; break; /* C 的整數除法本來就向 0 截斷，符合題意 */
            }
        } else {
            st[top++] = atoll(t);
        }
    }
    int ans = (int)st[0];
    free(st);
    return ans;
}
/* ===== 提交範圍 結束 ===== */

int main(void)
{
    char *a[] = {"2", "1", "+", "3", "*"};
    assert(evalRPN(a, 5) == 9);
    char *b[] = {"4", "13", "5", "/", "+"};
    assert(evalRPN(b, 5) == 6);
    char *c[] = {"10", "6", "9", "3", "+", "-11", "*", "/", "*", "17", "+", "5", "+"};
    assert(evalRPN(c, 13) == 22);
    char *d[] = {"-7", "2", "/"}; /* 向 0 截斷：-3，不是 -4 */
    assert(evalRPN(d, 3) == -3);
    char *e[] = {"42"};
    assert(evalRPN(e, 1) == 42);
    puts("0150: passed");
    return 0;
}
