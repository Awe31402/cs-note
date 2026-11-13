#include "stack.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void test_both_stacks(void)
{
    Stack a;
    LStack b;
    stack_init(&a);
    lstack_init(&b);
    int model[5000], n = 0;
    srand(6);
    for (int t = 0; t < 20000; t++) {
        if (n == 0 || (rand() % 3 != 0 && n < 5000)) {
            int x = rand();
            stack_push(&a, x);
            lstack_push(&b, x);
            model[n++] = x;
        } else {
            assert(stack_peek(&a) == model[n - 1]);
            assert(lstack_peek(&b) == model[n - 1]);
            assert(stack_pop(&a) == model[n - 1]);
            assert(lstack_pop(&b) == model[n - 1]);
            n--;
        }
        assert(a.top == n && b.size == n);
        assert(stack_empty(&a) == (n == 0));
    }
    stack_free(&a);
    lstack_free(&b);
}

static void test_paren(void)
{
    assert(paren_balanced(""));
    assert(paren_balanced("()[]{}"));
    assert(paren_balanced("{[()()]}"));
    assert(paren_balanced("a*(b+c)-[d/e]"));
    assert(!paren_balanced("(]"));
    assert(!paren_balanced("([)]"));
    assert(!paren_balanced("(("));
    assert(!paren_balanced("))"));
    assert(!paren_balanced("}"));
}

static void check_postfix(const char *in, const char *want)
{
    char out[64];
    assert(infix_to_postfix(in, out, sizeof out) == 0);
    assert(strcmp(out, want) == 0);
}

static void test_infix_to_postfix(void)
{
    check_postfix("A+B*C", "ABC*+");
    check_postfix("(A+B)*C", "AB+C*");
    check_postfix("A-B-C", "AB-C-");     /* 左結合 */
    check_postfix("A^B^C", "ABC^^");     /* 右結合 */
    check_postfix("A + (B * C - (D / E ^ F) * G) * H", "ABC*DEF^/G*-H*+");
    char out[64];
    assert(infix_to_postfix("(A+B", out, sizeof out) == -1);
    assert(infix_to_postfix("A+B)", out, sizeof out) == -1);
    assert(infix_to_postfix("A+B*C", out, 4) == -1); /* 放不下 */
}

static void test_eval(void)
{
    int r;
    assert(eval_postfix("23*4+", &r) == 0 && r == 10);
    assert(eval_postfix("934*8+4/-", &r) == 0 && r == 4); /* 9 - (3*4+8)/4 */
    assert(eval_postfix("52-", &r) == 0 && r == 3);         /* 順序：5 - 2 */
    assert(eval_postfix("50/", &r) == -1);
    assert(eval_postfix("5+", &r) == -1);
    assert(eval_postfix("56", &r) == -1);

    /* 中序 → 後序 → 計算，一條龍 */
    const char *infix[] = {"1+2*3", "(1+2)*3", "9-4-3", "8/2/2", "(7-(2+3))*(4/2)"};
    int want[] = {7, 9, 2, 2, 4};
    for (int i = 0; i < 5; i++) {
        char post[64];
        assert(infix_to_postfix(infix[i], post, sizeof post) == 0);
        assert(eval_postfix(post, &r) == 0 && r == want[i]);
    }
}

int main(void)
{
    test_both_stacks();
    test_paren();
    test_infix_to_postfix();
    test_eval();
    puts("stack: all tests passed");
    return 0;
}
