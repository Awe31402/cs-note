/* LeetCode 641 · Design Circular Deque
 * 思路：跟 622 一樣的環狀陣列，多了「從前面放」和「從後面拿」。
 *       往前一格：head = (head - 1 + k) % k，+k 是為了避免負數取餘數。
 */
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

/* ===== 提交範圍 開始 ===== */
typedef struct {
    int *a;
    int k, head, count;
} MyCircularDeque;

MyCircularDeque *myCircularDequeCreate(int k)
{
    MyCircularDeque *d = malloc(sizeof *d);
    d->a = malloc((size_t)k * sizeof *d->a);
    d->k = k;
    d->head = 0;
    d->count = 0;
    return d;
}

bool myCircularDequeInsertFront(MyCircularDeque *d, int value)
{
    if (d->count == d->k)
        return false;
    d->head = (d->head - 1 + d->k) % d->k;
    d->a[d->head] = value;
    d->count++;
    return true;
}

bool myCircularDequeInsertLast(MyCircularDeque *d, int value)
{
    if (d->count == d->k)
        return false;
    d->a[(d->head + d->count) % d->k] = value;
    d->count++;
    return true;
}

bool myCircularDequeDeleteFront(MyCircularDeque *d)
{
    if (d->count == 0)
        return false;
    d->head = (d->head + 1) % d->k;
    d->count--;
    return true;
}

bool myCircularDequeDeleteLast(MyCircularDeque *d)
{
    if (d->count == 0)
        return false;
    d->count--; /* 尾巴是算出來的，count 減一就等於刪掉了 */
    return true;
}

int myCircularDequeGetFront(MyCircularDeque *d)
{
    return d->count ? d->a[d->head] : -1;
}

int myCircularDequeGetRear(MyCircularDeque *d)
{
    return d->count ? d->a[(d->head + d->count - 1) % d->k] : -1;
}

bool myCircularDequeIsEmpty(MyCircularDeque *d)
{
    return d->count == 0;
}

bool myCircularDequeIsFull(MyCircularDeque *d)
{
    return d->count == d->k;
}

void myCircularDequeFree(MyCircularDeque *d)
{
    free(d->a);
    free(d);
}
/* ===== 提交範圍 結束 ===== */

int main(void)
{
    MyCircularDeque *d = myCircularDequeCreate(3);
    assert(myCircularDequeInsertLast(d, 1));
    assert(myCircularDequeInsertLast(d, 2));
    assert(myCircularDequeInsertFront(d, 3));
    assert(!myCircularDequeInsertFront(d, 4));
    assert(myCircularDequeGetRear(d) == 2);
    assert(myCircularDequeIsFull(d));
    assert(myCircularDequeDeleteLast(d));
    assert(myCircularDequeInsertFront(d, 4));
    assert(myCircularDequeGetFront(d) == 4);
    myCircularDequeFree(d);

    /* 隨機對照 */
    srand(641);
    d = myCircularDequeCreate(5);
    int model[5], n = 0;
    for (int t = 0; t < 5000; t++) {
        int op = rand() % 4, x = rand() % 100;
        if (op == 0) {
            bool ok = myCircularDequeInsertFront(d, x);
            assert(ok == (n < 5));
            if (ok) {
                for (int i = n; i > 0; i--)
                    model[i] = model[i - 1];
                model[0] = x;
                n++;
            }
        } else if (op == 1) {
            bool ok = myCircularDequeInsertLast(d, x);
            assert(ok == (n < 5));
            if (ok)
                model[n++] = x;
        } else if (op == 2) {
            assert(myCircularDequeDeleteFront(d) == (n > 0));
            if (n > 0) {
                for (int i = 0; i < n - 1; i++)
                    model[i] = model[i + 1];
                n--;
            }
        } else {
            assert(myCircularDequeDeleteLast(d) == (n > 0));
            if (n > 0)
                n--;
        }
        assert(myCircularDequeGetFront(d) == (n ? model[0] : -1));
        assert(myCircularDequeGetRear(d) == (n ? model[n - 1] : -1));
        assert(myCircularDequeIsEmpty(d) == (n == 0) && myCircularDequeIsFull(d) == (n == 5));
    }
    myCircularDequeFree(d);
    puts("0641: passed");
    return 0;
}
