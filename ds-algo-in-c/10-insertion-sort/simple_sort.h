#ifndef SIMPLE_SORT_H
#define SIMPLE_SORT_H

/* 基本排序，全部由小到大、原地排序。 */

/* 插入排序 (insertion sort)：回傳「搬移」的次數，剛好等於逆序對 (inversion) 的個數。 */
long insertion_sort(int *a, int n);

/* 用二分搜尋找插入點：比較次數降到 O(n log n)，但搬移還是 O(n²)。 */
void binary_insertion_sort(int *a, int n);

void bubble_sort(int *a, int n);    /* 一整趟沒交換就提早結束：已排序時 O(n) */
void selection_sort(int *a, int n); /* 交換次數最多 n-1 次；不穩定 */
void shell_sort(int *a, int n);     /* 間隔 gap 的插入排序，gap 逐步縮小到 1 */

int is_sorted(const int *a, int n);

#endif
