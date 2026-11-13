/* LeetCode 729 · My Calendar I
 * 思路：用有序集合（紅黑樹）以「開始時間」存已預訂的區間 [s, e)。新的 [s, e) 只要檢查兩個鄰居：
 *   - 下一個：第一個 start >= s 的區間，它的 start 必須 >= e
 *   - 上一個：最後一個 start < s 的區間，它的 end 必須 <= s
 *   每次 O(log n)。用陣列逐一檢查是 O(n)。
 */
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

/* ===== 提交範圍 開始 ===== */
/* ---- 精簡版紅黑樹（CLRS 第 13 章，哨兵 nil）---- */
typedef struct Node {
    long long key;
    int val;
    int red;
    struct Node *l, *r, *p;
} Node;

typedef struct {
    Node *root;
    Node nil;
} Tree;

static void t_init(Tree *t)
{
    t->nil.red = 0;
    t->nil.l = t->nil.r = t->nil.p = &t->nil;
    t->root = &t->nil;
}

static void t_free_rec(Tree *t, Node *x)
{
    if (x == &t->nil)
        return;
    t_free_rec(t, x->l);
    t_free_rec(t, x->r);
    free(x);
}

static void rot_l(Tree *t, Node *x)
{
    Node *y = x->r;
    x->r = y->l;
    if (y->l != &t->nil)
        y->l->p = x;
    y->p = x->p;
    if (x->p == &t->nil)
        t->root = y;
    else if (x == x->p->l)
        x->p->l = y;
    else
        x->p->r = y;
    y->l = x;
    x->p = y;
}

static void rot_r(Tree *t, Node *x)
{
    Node *y = x->l;
    x->l = y->r;
    if (y->r != &t->nil)
        y->r->p = x;
    y->p = x->p;
    if (x->p == &t->nil)
        t->root = y;
    else if (x == x->p->r)
        x->p->r = y;
    else
        x->p->l = y;
    y->r = x;
    x->p = y;
}

static void t_insert(Tree *t, long long key, int val) /* 呼叫前要確定 key 不存在 */
{
    Node *y = &t->nil, *x = t->root, *z = malloc(sizeof *z);
    while (x != &t->nil) {
        y = x;
        x = key < x->key ? x->l : x->r;
    }
    z->key = key;
    z->val = val;
    z->p = y;
    z->l = z->r = &t->nil;
    z->red = 1;
    if (y == &t->nil)
        t->root = z;
    else if (key < y->key)
        y->l = z;
    else
        y->r = z;
    while (z->p->red) {
        Node *g = z->p->p;
        if (z->p == g->l) {
            Node *u = g->r;
            if (u->red) {
                z->p->red = u->red = 0;
                g->red = 1;
                z = g;
            } else {
                if (z == z->p->r) {
                    z = z->p;
                    rot_l(t, z);
                }
                z->p->red = 0;
                z->p->p->red = 1;
                rot_r(t, z->p->p);
            }
        } else {
            Node *u = g->l;
            if (u->red) {
                z->p->red = u->red = 0;
                g->red = 1;
                z = g;
            } else {
                if (z == z->p->l) {
                    z = z->p;
                    rot_r(t, z);
                }
                z->p->red = 0;
                z->p->p->red = 1;
                rot_l(t, z->p->p);
            }
        }
    }
    t->root->red = 0;
}

static Node *t_lower(Tree *t, long long key) /* 第一個 >= key，沒有回傳 NULL */
{
    Node *x = t->root, *best = NULL;
    while (x != &t->nil) {
        if (x->key >= key) {
            best = x;
            x = x->l;
        } else {
            x = x->r;
        }
    }
    return best;
}

static Node *t_before(Tree *t, long long key) /* 最後一個 < key，沒有回傳 NULL */
{
    Node *x = t->root, *best = NULL;
    while (x != &t->nil) {
        if (x->key < key) {
            best = x;
            x = x->r;
        } else {
            x = x->l;
        }
    }
    return best;
}

typedef struct {
    Tree t; /* key = start, val = end */
} MyCalendar;

MyCalendar *myCalendarCreate(void)
{
    MyCalendar *c = malloc(sizeof *c);
    t_init(&c->t);
    return c;
}

bool myCalendarBook(MyCalendar *c, int startTime, int endTime)
{
    Node *next = t_lower(&c->t, startTime), *prev = t_before(&c->t, startTime);
    if (next && next->key < endTime)
        return false; /* 下一個在我結束前就開始了 */
    if (prev && prev->val > startTime)
        return false; /* 上一個在我開始後才結束 */
    t_insert(&c->t, startTime, endTime);
    return true;
}

void myCalendarFree(MyCalendar *c)
{
    t_free_rec(&c->t, c->t.root);
    free(c);
}
/* ===== 提交範圍 結束 ===== */

int main(void)
{
    MyCalendar *c = myCalendarCreate();
    assert(myCalendarBook(c, 10, 20));
    assert(!myCalendarBook(c, 15, 25));
    assert(myCalendarBook(c, 20, 30)); /* [10,20) 和 [20,30) 不重疊 */
    assert(!myCalendarBook(c, 5, 11));
    assert(myCalendarBook(c, 5, 10));
    assert(!myCalendarBook(c, 25, 26));
    myCalendarFree(c);

    /* 隨機對照：用陣列存所有區間，暴力檢查 */
    srand(729);
    c = myCalendarCreate();
    int s[2000], e[2000], n = 0;
    for (int k = 0; k < 2000; k++) {
        int a = rand() % 10000, b = a + 1 + rand() % 30;
        int ok = 1;
        for (int i = 0; i < n; i++)
            if (a < e[i] && s[i] < b)
                ok = 0;
        assert(myCalendarBook(c, a, b) == ok);
        if (ok) {
            s[n] = a;
            e[n++] = b;
        }
    }
    myCalendarFree(c);
    puts("0729: passed");
    return 0;
}
