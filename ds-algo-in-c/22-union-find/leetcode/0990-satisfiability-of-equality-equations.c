/* LeetCode 990 · Satisfiability of Equality Equations
 * 方程式像 "a==b"、"b!=c"，變數是 26 個小寫字母。能不能同時成立？
 * 思路：兩趟。第一趟把所有 "==" 的兩邊 union；第二趟檢查每個 "!=" 的兩邊，在同一組就矛盾。
 *       一定要先處理完所有 "=="：相等關係有遞移性，順序不能混。
 */
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

/* ===== 提交範圍 開始 ===== */
static int find(int *p, int x)
{
    while (p[x] != x) {
        p[x] = p[p[x]]; /* 路徑減半 (path halving)：一邊走一邊讓節點指向祖父，也很有效 */
        x = p[x];
    }
    return x;
}

bool equationsPossible(char **equations, int equationsSize)
{
    int p[26];
    for (int i = 0; i < 26; i++)
        p[i] = i;
    for (int i = 0; i < equationsSize; i++)
        if (equations[i][1] == '=')
            p[find(p, equations[i][0] - 'a')] = find(p, equations[i][3] - 'a');
    for (int i = 0; i < equationsSize; i++)
        if (equations[i][1] == '!' && find(p, equations[i][0] - 'a') == find(p, equations[i][3] - 'a'))
            return false;
    return true;
}
/* ===== 提交範圍 結束 ===== */

int main(void)
{
    char *a[] = {"a==b", "b!=a"};
    assert(!equationsPossible(a, 2));
    char *b[] = {"b==a", "a==b"};
    assert(equationsPossible(b, 2));
    char *c[] = {"a==b", "b!=c", "c==a"}; /* a=b、c=a → b=c，跟 b!=c 矛盾 */
    assert(!equationsPossible(c, 3));
    char *d[] = {"a!=a"};
    assert(!equationsPossible(d, 1));
    char *e[] = {"c==c", "b==d", "x!=z"};
    assert(equationsPossible(e, 3));
    puts("0990: passed");
    return 0;
}
