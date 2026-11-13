#include "simple_sort.h"

long insertion_sort(int *a, int n)
{
    long moves = 0;
    for (int i = 1; i < n; i++) {
        int key = a[i]; /* 左邊 a[0..i-1] 已排好；把 key 插進去 */
        int j = i - 1;
        while (j >= 0 && a[j] > key) { /* 用 > 不用 >=：相等不動 → 穩定 */
            a[j + 1] = a[j];
            j--;
            moves++;
        }
        a[j + 1] = key;
    }
    return moves;
}

void binary_insertion_sort(int *a, int n)
{
    for (int i = 1; i < n; i++) {
        int key = a[i];
        int lo = 0, hi = i; /* 找第一個 > key 的位置（upper_bound），維持穩定 */
        while (lo < hi) {
            int mid = lo + (hi - lo) / 2;
            if (a[mid] > key)
                hi = mid;
            else
                lo = mid + 1;
        }
        for (int j = i; j > lo; j--)
            a[j] = a[j - 1];
        a[lo] = key;
    }
}

void bubble_sort(int *a, int n)
{
    for (int end = n - 1; end > 0; end--) {
        int swapped = 0;
        for (int j = 0; j < end; j++) /* 每趟把最大的「浮」到 end */
            if (a[j] > a[j + 1]) {
                int t = a[j];
                a[j] = a[j + 1];
                a[j + 1] = t;
                swapped = 1;
            }
        if (!swapped)
            return;
    }
}

void selection_sort(int *a, int n)
{
    for (int i = 0; i < n - 1; i++) {
        int m = i;
        for (int j = i + 1; j < n; j++) /* 找 a[i..n-1] 最小的 */
            if (a[j] < a[m])
                m = j;
        int t = a[i];
        a[i] = a[m];
        a[m] = t;
    }
}

void shell_sort(int *a, int n)
{
    int gap = 1;
    while (gap < n / 3)
        gap = 3 * gap + 1; /* Knuth 的間隔序列：1, 4, 13, 40, ... */
    for (; gap > 0; gap /= 3)
        for (int i = gap; i < n; i++) {
            int key = a[i], j = i;
            while (j >= gap && a[j - gap] > key) {
                a[j] = a[j - gap];
                j -= gap;
            }
            a[j] = key;
        }
}

int is_sorted(const int *a, int n)
{
    for (int i = 1; i < n; i++)
        if (a[i - 1] > a[i])
            return 0;
    return 1;
}
