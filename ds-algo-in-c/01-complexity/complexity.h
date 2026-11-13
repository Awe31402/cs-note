#ifndef COMPLEXITY_H
#define COMPLEXITY_H

/* 數「迴圈本體執行幾次」，用來驗證常見迴圈型態的成長速度。 */
long count_constant(int n);   /* O(1)：跟 n 無關 */
long count_log(int n);        /* O(log n)：i 每次乘 2 */
long count_sqrt(int n);       /* O(√n)：i*i <= n */
long count_linear(int n);     /* O(n) */
long count_n_log_n(int n);    /* O(n log n)：外層 n 次，內層乘 2 */
long count_triangular(int n); /* O(n²)：內層跑到 i，共 n(n+1)/2 次 */
long count_quadratic(int n);  /* O(n²)：n × n */

/* 攤銷分析 (amortized analysis)：一個一個 push 進動態陣列，
 * 回傳「搬移舊元素」的總次數。 */
long dynarray_copies_double(int n); /* 滿了容量 ×2：總搬移 < 2n，平均每次 O(1) */
long dynarray_copies_plus1(int n);  /* 滿了容量 +1：總搬移 n(n-1)/2，平均每次 O(n) */

#endif
