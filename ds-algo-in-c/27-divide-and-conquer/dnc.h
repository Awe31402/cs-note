#ifndef DNC_H
#define DNC_H

/* 分治法 (divide and conquer)：切成小問題 → 遞迴解 → 合併答案。 */

/* n×n 矩陣乘法 C = A · B，矩陣用一維陣列 row-major 存。 */
void matmul_naive(const long long *A, const long long *B, long long *C, int n);     /* 三層迴圈 Θ(n³) */
void matmul_recursive(const long long *A, const long long *B, long long *C, int n); /* CLRS 4.1：切成四塊、8 次遞迴，還是 Θ(n³)；n 必須是 2 的次方 */
void strassen(const long long *A, const long long *B, long long *C, int n);         /* CLRS 4.2：7 次遞迴，Θ(n^lg7) ≈ Θ(n^2.81)；n 必須是 2 的次方 */

/* 最大子陣列（CLRS 第 3 版 4.1）：回傳最大和，*lo、*hi 是那一段的範圍 [lo, hi]。Θ(n log n)。 */
long long max_subarray_dc(const int *a, int n, int *lo, int *hi);

/* 多數元素：出現超過 n/2 次的元素（呼叫者保證存在）。分治版 Θ(n log n)。 */
int majority_dc(const int *a, int n);

#endif
