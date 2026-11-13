#ifndef DP_H
#define DP_H

/* ---- CLRS 14.1 切鋼條 (rod cutting) ----
 * price[i] = 長度 i 的鋼條賣多少（price[0] 不用）。長度 n 的鋼條怎麼切賣最多錢？ */
int rod_cut(const int *price, int n, int *first_cut); /* 由下而上；first_cut[j] = 長度 j 時第一刀切下的長度，可為 NULL */
int rod_cut_memo(const int *price, int n);            /* 由上而下 + 備忘錄 (memoization) */

/* ---- CLRS 14.2 矩陣鏈乘 (matrix-chain multiplication) ----
 * n 個矩陣，A_i 是 p[i-1] × p[i]。怎麼加括號讓純量乘法次數最少？
 * s 是 n×n（s[(i-1)*n + (j-1)] = A_i..A_j 最好的切點 k），可為 NULL。 */
long long matrix_chain(const int *p, int n, int *s);
int matrix_chain_paren(const int *s, int n, char *out); /* 例如 "((A1(A2A3))((A4A5)A6))"，回傳長度 */

/* ---- CLRS 14.4 最長共同子序列 (LCS) ----  回傳長度，out 寫入其中一個 LCS（可為 NULL） */
int lcs(const char *x, const char *y, char *out);

/* ---- 其他經典 ---- */
int edit_distance(const char *a, const char *b);                  /* 插入、刪除、替換各算 1 步 */
int knapsack01(const int *w, const int *v, int n, int cap);       /* 0/1 背包：每樣最多拿一次 */
int lis_length(const int *a, int n);                              /* 最長嚴格遞增子序列，O(n log n) */

#endif
