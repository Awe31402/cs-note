#ifndef KMP_H
#define KMP_H

/* 字串比對 (string matching)：在文字 T（長 n）中找樣式 P（長 m）所有出現的位置。
 * 每個函式都把所有起點 (shift) 依序寫進 pos，回傳個數；會重疊的也要算（"aa" 在 "aaa" 出現 2 次）。 */

int naive_match(const char *t, const char *p, int *pos); /* CLRS 32.1：每個起點都比一次，O((n-m+1)·m) */
int rabin_karp(const char *t, const char *p, int *pos);  /* CLRS 32.2：滾動雜湊，平均 O(n + m) */
int kmp_match(const char *t, const char *p, int *pos);   /* CLRS 32.4：O(n + m) */

/* KMP 的前綴函數 (prefix function / failure function)：
 * pi[q] = P[0..q] 的「最長、而且不是整個字串本身」的前綴，同時也是它的後綴，長度是多少。 */
void kmp_prefix(const char *p, int m, int *pi);

/* Z 函數：z[i] = s 和 s[i..] 的最長共同前綴長度（z[0] 定義成 n）。O(n)。 */
void z_function(const char *s, int n, int *z);

#endif
