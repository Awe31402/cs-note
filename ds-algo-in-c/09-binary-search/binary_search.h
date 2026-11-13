#ifndef BINARY_SEARCH_H
#define BINARY_SEARCH_H

/* 以下陣列都必須由小到大排好。區間一律用左閉右開 [lo, hi)。 */

int linear_search(const int *a, int n, int x);          /* O(n)，不需排序；找不到 -1 */
int bs_find(const int *a, int n, int x);                /* O(log n)；回傳任一個等於 x 的 index，找不到 -1 */
int bs_find_rec(const int *a, int lo, int hi, int x);   /* 遞迴版，在 [lo, hi) 找 */
int lower_bound(const int *a, int n, int x);            /* 第一個 >= x 的位置；全部都 < x 就回傳 n */
int upper_bound(const int *a, int n, int x);            /* 第一個 >  x 的位置 */

/* 在「答案」上二分：最大的 r 使 r*r <= x（x >= 0） */
int isqrt_bs(int x);

/* Thareja 14.4–14.5 的另外兩種搜尋，找不到回傳 -1 */
int interpolation_search(const int *a, int n, int x); /* 值均勻分布時平均 O(log log n)，最壞 O(n) */
int jump_search(const int *a, int n, int x);          /* 每次跳 √n 格，O(√n) */

#endif
