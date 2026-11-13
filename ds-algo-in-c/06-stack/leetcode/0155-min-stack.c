/* LeetCode 155 · Min Stack（每個操作都要 O(1)，包括 getMin）
 * 思路：每一格除了存值，也存「到這一格為止的最小值」。
 *       pop 掉之後，新的頂端記的最小值自然就是對的。
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/* ===== 提交範圍 開始 ===== */
typedef struct {
    int *val, *min;
    int top, cap;
} MinStack;

MinStack *minStackCreate(void)
{
    return calloc(1, sizeof(MinStack));
}

void minStackPush(MinStack *s, int x)
{
    if (s->top == s->cap) {
        s->cap = s->cap ? s->cap * 2 : 16;
        s->val = realloc(s->val, (size_t)s->cap * sizeof *s->val);
        s->min = realloc(s->min, (size_t)s->cap * sizeof *s->min);
    }
    s->val[s->top] = x;
    s->min[s->top] = (s->top == 0 || x < s->min[s->top - 1]) ? x : s->min[s->top - 1];
    s->top++;
}

void minStackPop(MinStack *s)
{
    s->top--;
}

int minStackTop(MinStack *s)
{
    return s->val[s->top - 1];
}

int minStackGetMin(MinStack *s)
{
    return s->min[s->top - 1];
}

void minStackFree(MinStack *s)
{
    free(s->val);
    free(s->min);
    free(s);
}
/* ===== 提交範圍 結束 ===== */

int main(void)
{
    MinStack *s = minStackCreate();
    minStackPush(s, -2);
    minStackPush(s, 0);
    minStackPush(s, -3);
    assert(minStackGetMin(s) == -3);
    minStackPop(s);
    assert(minStackTop(s) == 0);
    assert(minStackGetMin(s) == -2);
    minStackFree(s);

    /* 隨機：getMin 跟掃一遍的結果比 */
    srand(155);
    s = minStackCreate();
    int model[3000], n = 0;
    for (int t = 0; t < 3000; t++) {
        if (n == 0 || rand() % 3) {
            int x = rand() % 100 - 50;
            minStackPush(s, x);
            model[n++] = x;
        } else {
            minStackPop(s);
            n--;
        }
        if (n > 0) {
            int m = model[0];
            for (int i = 1; i < n; i++)
                if (model[i] < m)
                    m = model[i];
            assert(minStackGetMin(s) == m && minStackTop(s) == model[n - 1]);
        }
    }
    minStackFree(s);
    puts("0155: passed");
    return 0;
}
