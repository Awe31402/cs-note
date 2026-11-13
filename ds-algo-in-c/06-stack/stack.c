#include "stack.h"

#include <assert.h>
#include <stdlib.h>

void stack_init(Stack *s)
{
    s->data = NULL;
    s->top = 0;
    s->cap = 0;
}

void stack_free(Stack *s)
{
    free(s->data);
    stack_init(s);
}

void stack_push(Stack *s, int x)
{
    if (s->top == s->cap) {
        int cap = s->cap ? s->cap * 2 : 8;
        int *p = realloc(s->data, (size_t)cap * sizeof *p);
        if (!p)
            abort();
        s->data = p;
        s->cap = cap;
    }
    s->data[s->top++] = x;
}

int stack_pop(Stack *s)
{
    assert(s->top > 0); /* underflow */
    return s->data[--s->top];
}

int stack_peek(const Stack *s)
{
    assert(s->top > 0);
    return s->data[s->top - 1];
}

int stack_empty(const Stack *s)
{
    return s->top == 0;
}

void lstack_init(LStack *s)
{
    s->top = NULL;
    s->size = 0;
}

void lstack_push(LStack *s, int x)
{
    LNode *n = malloc(sizeof *n);
    if (!n)
        abort();
    n->val = x;
    n->next = s->top;
    s->top = n;
    s->size++;
}

int lstack_pop(LStack *s)
{
    assert(s->top);
    LNode *n = s->top;
    int x = n->val;
    s->top = n->next;
    free(n);
    s->size--;
    return x;
}

int lstack_peek(const LStack *s)
{
    assert(s->top);
    return s->top->val;
}

void lstack_free(LStack *s)
{
    while (s->top)
        lstack_pop(s);
}

int paren_balanced(const char *s)
{
    Stack st;
    stack_init(&st);
    int ok = 1;
    for (; *s && ok; s++) {
        char c = *s;
        if (c == '(' || c == '[' || c == '{') {
            stack_push(&st, c);
        } else if (c == ')' || c == ']' || c == '}') {
            char open = c == ')' ? '(' : c == ']' ? '[' : '{';
            ok = !stack_empty(&st) && stack_pop(&st) == open;
        }
    }
    ok = ok && stack_empty(&st); /* 還有沒關的左括號也不行 */
    stack_free(&st);
    return ok;
}

static int prec(char op)
{
    switch (op) {
    case '^': return 3;
    case '*': case '/': return 2;
    case '+': case '-': return 1;
    default: return 0; /* '(' 優先權最低，不會被彈出 */
    }
}

static int is_operand(char c)
{
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

int infix_to_postfix(const char *in, char *out, size_t cap)
{
    Stack ops;
    stack_init(&ops);
    size_t k = 0;
    int ok = 1;
#define EMIT(ch) do { if (k + 1 >= cap) { ok = 0; break; } out[k++] = (ch); } while (0)
    for (; *in && ok; in++) {
        char c = *in;
        if (c == ' ') {
            continue;
        } else if (is_operand(c)) {
            EMIT(c); /* 運算元直接輸出 */
        } else if (c == '(') {
            stack_push(&ops, c);
        } else if (c == ')') {
            while (ok && !stack_empty(&ops) && stack_peek(&ops) != '(')
                EMIT((char)stack_pop(&ops));
            if (stack_empty(&ops))
                ok = 0; /* 多了右括號 */
            else
                stack_pop(&ops); /* 丟掉 '(' */
        } else if (prec(c) > 0) {
            /* 堆疊頂端優先權較高（或相同且左結合）的運算子要先輸出 */
            while (ok && !stack_empty(&ops)) {
                char t = (char)stack_peek(&ops);
                if (prec(t) > prec(c) || (prec(t) == prec(c) && c != '^'))
                    EMIT((char)stack_pop(&ops));
                else
                    break;
            }
            stack_push(&ops, c);
        } else {
            ok = 0; /* 不認得的字元 */
        }
    }
    while (ok && !stack_empty(&ops)) {
        char t = (char)stack_pop(&ops);
        if (t == '(')
            ok = 0; /* 多了左括號 */
        else
            EMIT(t);
    }
#undef EMIT
    stack_free(&ops);
    if (!ok)
        return -1;
    out[k] = '\0';
    return 0;
}

int eval_postfix(const char *s, int *result)
{
    Stack st;
    stack_init(&st);
    int ok = 1;
    for (; *s && ok; s++) {
        char c = *s;
        if (c == ' ')
            continue;
        if (c >= '0' && c <= '9') {
            stack_push(&st, c - '0');
            continue;
        }
        if (st.top < 2) {
            ok = 0;
            break;
        }
        int b = stack_pop(&st), a = stack_pop(&st); /* 先彈出的是右運算元 */
        switch (c) {
        case '+': stack_push(&st, a + b); break;
        case '-': stack_push(&st, a - b); break;
        case '*': stack_push(&st, a * b); break;
        case '/':
            if (b == 0)
                ok = 0;
            else
                stack_push(&st, a / b);
            break;
        default: ok = 0;
        }
    }
    ok = ok && st.top == 1; /* 最後剛好剩一個才是正確的式子 */
    if (ok)
        *result = st.data[0];
    stack_free(&st);
    return ok ? 0 : -1;
}
