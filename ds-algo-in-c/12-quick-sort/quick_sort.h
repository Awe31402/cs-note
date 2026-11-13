#ifndef QUICK_SORT_H
#define QUICK_SORT_H

/* 分割 (partition)：以 pivot 為界，小的放左邊、大的放右邊。 */
int lomuto_partition(int *a, int lo, int hi); /* CLRS 版，pivot = a[hi]；回傳 pivot 最後的位置。閉區間 [lo, hi] */
int hoare_partition(int *a, int lo, int hi);  /* Hoare 原版，pivot = a[lo]；回傳 j，使 [lo, j] <= pivot <= [j+1, hi] */

void quick_sort_lomuto(int *a, int n);   /* 固定用最後一個當 pivot：已排序輸入會退化成 O(n²) */
void quick_sort_random(int *a, int n);   /* 隨機 pivot + Hoare 分割 */
void quick_sort_3way(int *a, int n);     /* 三路切分：< pivot | == pivot | > pivot，大量重複值時很快 */

/* 快速選擇 (quickselect)：回傳第 k 小的值（k 從 0 開始），平均 O(n)。會打亂 a。 */
int quickselect(int *a, int n, int k);

#endif
