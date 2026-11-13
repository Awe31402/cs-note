#ifndef GREEDY_H
#define GREEDY_H

/* ---- CLRS 15.1 活動選擇 (activity selection) ----
 * 活動 i 佔用 [s[i], f[i])，同一時間只能做一個。最多能做幾個？
 * f 必須已經由小到大排好。chosen 寫入選中的 index，回傳個數。O(n)。 */
int activity_select(const int *s, const int *f, int n, int *chosen);

/* ---- CLRS 15.3 Huffman 編碼 ----
 * freq[i] = 字元 i 出現的次數（2 <= n <= 64：最長的編碼是 n-1 位，要放得進 codes[i][64]）。
 * 算出每個字元的編碼 codes[i]（'0'/'1' 字串）。
 * 回傳總位元數 = Σ freq[i] × 編碼長度。O(n log n)。 */
long long huffman(const long long *freq, int n, char (*codes)[64]);

/* ---- 分數背包 (fractional knapsack)：東西可以切開拿 → 貪心照「單位價值」拿就是最佳 ---- */
double fractional_knapsack(const int *w, const int *v, int n, int cap);

/* ---- CLRS 15.4 離線快取 (offline caching)：事先知道所有請求時，
 * 快取滿了就踢掉「下次使用時間最遠」的那個（Belady 的 furthest-in-future）。回傳 cache miss 次數。 */
int offline_cache_misses(const int *req, int m, int k);

#endif
