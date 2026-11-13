#ifndef STACK_H
#define STACK_H

#include <stddef.h>

/* ---- 陣列版 stack ----  top 同時是「元素個數」和「下一個要放的位置」 */
typedef struct {
    int *data;
    int top;
    int cap;
} Stack;

void stack_init(Stack *s);
void stack_free(Stack *s);
void stack_push(Stack *s, int x); /* 攤銷 O(1) */
int  stack_pop(Stack *s);         /* O(1)；不可為空 */
int  stack_peek(const Stack *s);  /* O(1)；不可為空 */
int  stack_empty(const Stack *s);

/* ---- 串列版 stack ----  頭就是 top，push/pop 都在頭 */
typedef struct LNode {
    int val;
    struct LNode *next;
} LNode;

typedef struct {
    LNode *top;
    int size;
} LStack;

void lstack_init(LStack *s);
void lstack_free(LStack *s);
void lstack_push(LStack *s, int x); /* O(1) */
int  lstack_pop(LStack *s);         /* O(1)；不可為空 */
int  lstack_peek(const LStack *s);

/* ---- 應用（Thareja 7.7） ---- */
int paren_balanced(const char *s); /* ()[]{} 是否配對；其他字元忽略 */

/* 中序轉後序 (infix → postfix)。運算元是單一字母或數字，運算子 + - * / ^，可有括號與空白。
 * ^ 是右結合，其他左結合。成功 0；括號不配對或放不下 -1。 */
int infix_to_postfix(const char *in, char *out, size_t cap);

/* 計算後序式，運算元是單一數字 0-9。成功 0；格式錯誤或除以 0 回傳 -1。 */
int eval_postfix(const char *s, int *result);

#endif
