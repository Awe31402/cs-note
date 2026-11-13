#include "binary_search.h"

#include <math.h>

int linear_search(const int *a, int n, int x)
{
    for (int i = 0; i < n; i++)
        if (a[i] == x)
            return i;
    return -1;
}

int bs_find(const int *a, int n, int x)
{
    int lo = 0, hi = n; /* 答案在 [lo, hi) 裡 */
    while (lo < hi) {
        int mid = lo + (hi - lo) / 2; /* 不寫 (lo + hi) / 2：lo + hi 可能溢位 */
        if (a[mid] == x)
            return mid;
        if (a[mid] < x)
            lo = mid + 1; /* mid 已經看過，排除 */
        else
            hi = mid; /* 右開，mid 本身已排除 */
    }
    return -1;
}

int bs_find_rec(const int *a, int lo, int hi, int x)
{
    if (lo >= hi)
        return -1;
    int mid = lo + (hi - lo) / 2;
    if (a[mid] == x)
        return mid;
    if (a[mid] < x)
        return bs_find_rec(a, mid + 1, hi, x);
    return bs_find_rec(a, lo, mid, x);
}

/* 找「第一個讓條件成立的位置」的模板。條件 a[i] >= x 對 i 來說是 F F F T T T。 */
int lower_bound(const int *a, int n, int x)
{
    int lo = 0, hi = n;
    while (lo < hi) {
        int mid = lo + (hi - lo) / 2;
        if (a[mid] >= x)
            hi = mid; /* mid 可能就是答案，保留 */
        else
            lo = mid + 1;
    }
    return lo; /* lo == hi */
}

int upper_bound(const int *a, int n, int x)
{
    int lo = 0, hi = n;
    while (lo < hi) {
        int mid = lo + (hi - lo) / 2;
        if (a[mid] > x)
            hi = mid;
        else
            lo = mid + 1;
    }
    return lo;
}

int isqrt_bs(int x)
{
    /* 找第一個 r 使 r*r > x，答案是 r - 1。r 在 [0, 46341]（46341² > INT_MAX） */
    long long lo = 0, hi = 46341;
    if (hi > (long long)x + 1)
        hi = (long long)x + 1;
    while (lo < hi) {
        long long mid = lo + (hi - lo) / 2;
        if (mid * mid > x)
            hi = mid;
        else
            lo = mid + 1;
    }
    return (int)(lo - 1);
}

int interpolation_search(const int *a, int n, int x)
{
    int lo = 0, hi = n - 1;
    while (lo <= hi && x >= a[lo] && x <= a[hi]) {
        if (a[hi] == a[lo]) /* 全部一樣：避免除以 0 */
            return a[lo] == x ? lo : -1;
        /* 依照 x 在 [a[lo], a[hi]] 的比例，猜它的位置 */
        int pos = lo + (int)(((long long)x - a[lo]) * (hi - lo) / ((long long)a[hi] - a[lo])); /* 先轉 long long 再減，INT_MAX - INT_MIN 會溢位 */
        if (a[pos] == x)
            return pos;
        if (a[pos] < x)
            lo = pos + 1;
        else
            hi = pos - 1;
    }
    return -1;
}

int jump_search(const int *a, int n, int x)
{
    if (n == 0)
        return -1;
    int step = (int)sqrt((double)n);
    if (step < 1)
        step = 1;
    int prev = 0, cur = step;
    while (cur < n && a[cur - 1] < x) { /* 一次跳一整塊，直到這塊的最後一個 >= x */
        prev = cur;
        cur += step;
    }
    if (cur > n)
        cur = n;
    for (int i = prev; i < cur; i++) /* 在這一塊裡線性找 */
        if (a[i] == x)
            return i;
    return -1;
}
