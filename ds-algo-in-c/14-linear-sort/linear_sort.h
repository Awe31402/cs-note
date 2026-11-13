#ifndef LINEAR_SORT_H
#define LINEAR_SORT_H

/* 非比較排序 (non-comparison sort)：不靠兩兩比較，所以能突破 Ω(n log n) 的下界。 */

/* 計數排序：值的範圍 k = max - min + 1。O(n + k) 時間、O(n + k) 空間。結果寫到 out（不可與 in 重疊）。 */
void counting_sort(const int *in, int n, int *out);

/* 同上，但排的是 (key, id) 紀錄，用來示範「穩定」：key 相同時 id 保持原順序。key 範圍 0..max_key。 */
typedef struct {
    int key;
    int id;
} Rec;
void counting_sort_rec(const Rec *in, int n, int max_key, Rec *out);

/* 基數排序 (LSD radix sort)：一次處理 8 個位元，做 4 趟穩定的計數排序。支援負數。O(4 × (n + 256))。 */
void radix_sort(int *a, int n);

/* 桶排序：值要在 [0, 1) 而且大致均勻分布。n 個桶，每桶用插入排序。平均 O(n)。 */
void bucket_sort(double *a, int n);

#endif
