/* LeetCode 1845 · Seat Reservation Manager
 * 思路：座位 1..n。next = 還沒被訂過的最小座位號。被退回的座位放進有序集合（紅黑樹）。
 *   reserve：集合不是空的 → 拿最小的並刪掉；否則拿 next++。
 *   退回的座位一定 < next，所以集合裡的最小值一定比 next 小。每次 O(log n)。
 *   （用 min-heap 也可以；這裡示範有序集合的「取最小 + 刪除」。）
 */
#include <assert.h>
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

static void transplant(Tree *t, Node *u, Node *v)
{
    if (u->p == &t->nil)
        t->root = v;
    else if (u == u->p->l)
        u->p->l = v;
    else
        u->p->r = v;
    v->p = u->p;
}

static void t_erase(Tree *t, Node *z)
{
    Node *y = z, *x;
    int y_red = y->red;
    if (z->l == &t->nil) {
        x = z->r;
        transplant(t, z, z->r);
    } else if (z->r == &t->nil) {
        x = z->l;
        transplant(t, z, z->l);
    } else {
        y = z->r;
        while (y->l != &t->nil)
            y = y->l;
        y_red = y->red;
        x = y->r;
        if (y != z->r) {
            transplant(t, y, y->r);
            y->r = z->r;
            y->r->p = y;
        } else {
            x->p = y;
        }
        transplant(t, z, y);
        y->l = z->l;
        y->l->p = y;
        y->red = z->red;
    }
    free(z);
    if (y_red)
        return;
    while (x != t->root && !x->red) {
        if (x == x->p->l) {
            Node *w = x->p->r;
            if (w->red) {
                w->red = 0;
                x->p->red = 1;
                rot_l(t, x->p);
                w = x->p->r;
            }
            if (!w->l->red && !w->r->red) {
                w->red = 1;
                x = x->p;
            } else {
                if (!w->r->red) {
                    w->l->red = 0;
                    w->red = 1;
                    rot_r(t, w);
                    w = x->p->r;
                }
                w->red = x->p->red;
                x->p->red = 0;
                w->r->red = 0;
                rot_l(t, x->p);
                x = t->root;
            }
        } else {
            Node *w = x->p->l;
            if (w->red) {
                w->red = 0;
                x->p->red = 1;
                rot_r(t, x->p);
                w = x->p->l;
            }
            if (!w->r->red && !w->l->red) {
                w->red = 1;
                x = x->p;
            } else {
                if (!w->l->red) {
                    w->r->red = 0;
                    w->red = 1;
                    rot_l(t, w);
                    w = x->p->l;
                }
                w->red = x->p->red;
                x->p->red = 0;
                w->l->red = 0;
                rot_r(t, x->p);
                x = t->root;
            }
        }
    }
    x->red = 0;
}

typedef struct {
    Tree t;
    int next;
} SeatManager;

SeatManager *seatManagerCreate(int n)
{
    (void)n;
    SeatManager *m = malloc(sizeof *m);
    t_init(&m->t);
    m->next = 1;
    return m;
}

int seatManagerReserve(SeatManager *m)
{
    if (m->t.root == &m->t.nil)
        return m->next++;
    Node *x = m->t.root;
    while (x->l != &m->t.nil)
        x = x->l; /* 最小的 */
    int seat = (int)x->key;
    t_erase(&m->t, x);
    return seat;
}

void seatManagerUnreserve(SeatManager *m, int seatNumber)
{
    t_insert(&m->t, seatNumber, 0);
}

void seatManagerFree(SeatManager *m)
{
    t_free_rec(&m->t, m->t.root);
    free(m);
}
/* ===== 提交範圍 結束 ===== */

int main(void)
{
    SeatManager *m = seatManagerCreate(5);
    assert(seatManagerReserve(m) == 1);
    assert(seatManagerReserve(m) == 2);
    seatManagerUnreserve(m, 2);
    assert(seatManagerReserve(m) == 2);
    assert(seatManagerReserve(m) == 3);
    assert(seatManagerReserve(m) == 4);
    assert(seatManagerReserve(m) == 5);
    seatManagerUnreserve(m, 5);
    seatManagerFree(m);

    /* 隨機對照：用布林陣列記哪些座位被訂了 */
    enum { N = 500 };
    static int taken[N + 1];
    srand(1845);
    m = seatManagerCreate(N);
    int cnt = 0;
    for (int k = 0; k < 20000; k++) {
        if (cnt == 0 || (cnt < N && rand() % 2)) {
            int want = 1;
            while (taken[want])
                want++;
            assert(seatManagerReserve(m) == want);
            taken[want] = 1;
            cnt++;
        } else {
            int s;
            do
                s = 1 + rand() % N;
            while (!taken[s]);
            seatManagerUnreserve(m, s);
            taken[s] = 0;
            cnt--;
        }
    }
    seatManagerFree(m);
    puts("1845: passed");
    return 0;
}
