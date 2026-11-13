#ifndef MERGE_SORT_H
#define MERGE_SORT_H

/* 由小到大、穩定。額外空間 O(n)。 */
void merge_sort(int *a, int n);           /* 由上而下 (top-down)，遞迴 */
void merge_sort_bottom_up(int *a, int n); /* 由下而上，迴圈：寬度 1, 2, 4, ... */

/* 用 merge sort 順便數逆序對 (i < j 且 a[i] > a[j])，O(n log n)。會把 a 排好。 */
long long count_inversions(int *a, int n);

#endif
