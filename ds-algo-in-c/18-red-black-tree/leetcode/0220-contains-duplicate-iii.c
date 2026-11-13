/* LeetCode 220 · Contains Duplicate III
 * 找 i != j，|i - j| <= indexDiff 且 |nums[i] - nums[j]| <= valueDiff。
 * 思路：滑動視窗 + 有序集合。集合裡放最近 indexDiff 個數。
 *       新的 x 進來時，找集合裡第一個 >= x - valueDiff 的數，如果它 <= x + valueDiff 就找到了。
 *       每步 O(log k)，總共 O(n log k)。
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

static Node *t_find(Tree *t, long long key)
{
    Node *x = t->root;
    while (x != &t->nil && x->key != key)
        x = key < x->key ? x->l : x->r;
    return x;
}

bool containsNearbyAlmostDuplicate(int *nums, int numsSize, int indexDiff, int valueDiff)
{
    Tree t;
    t_init(&t);
    bool found = false;
    for (int i = 0; i < numsSize && !found; i++) {
        long long x = nums[i]; /* long long：x ± valueDiff 可能超出 int */
        Node *c = t_lower(&t, x - valueDiff);
        if (c && c->key <= x + valueDiff) {
            found = true;
            break;
        }
        t_insert(&t, x, 0); /* 走到這裡代表 x 不在集合裡（否則上面就找到了） */
        if (i >= indexDiff) /* 視窗滿了：移除最舊的 */
            t_erase(&t, t_find(&t, nums[i - indexDiff]));
    }
    t_free_rec(&t, t.root);
    return found;
}
/* ===== 提交範圍 結束 ===== */

static bool brute(int *a, int n, int k, int v)
{
    for (int i = 0; i < n; i++)
        for (int j = i + 1; j < n && j - i <= k; j++)
            if (llabs((long long)a[i] - a[j]) <= v)
                return true;
    return false;
}

int main(void)
{
    int a[] = {1, 2, 3, 1};
    assert(containsNearbyAlmostDuplicate(a, 4, 3, 0));
    int b[] = {1, 5, 9, 1, 5, 9};
    assert(!containsNearbyAlmostDuplicate(b, 6, 2, 3));
    int c[] = {-2147483647 - 1, 2147483647};
    assert(!containsNearbyAlmostDuplicate(c, 2, 1, 1000000000));
    srand(220);
    for (int t = 0; t < 2000; t++) {
        int n = 2 + rand() % 40, x[42];
        for (int i = 0; i < n; i++)
            x[i] = rand() % 200 - 100;
        int k = 1 + rand() % n, v = rand() % 5;
        assert(containsNearbyAlmostDuplicate(x, n, k, v) == brute(x, n, k, v));
    }
    puts("0220: passed");
    return 0;
}
