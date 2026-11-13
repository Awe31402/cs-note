#ifndef RECURSION_H
#define RECURSION_H

long long factorial(int n);      /* 一般遞迴：n * f(n-1)，有「待辦的乘法」 */
long long factorial_tail(int n); /* 尾遞迴：結果放在累積參數裡 */

int gcd(int a, int b); /* 歐幾里得：gcd(a, b) = gcd(b, a % b) */

long long power_naive(long long x, int n); /* O(n)：x * x^(n-1) */
long long power_fast(long long x, int n);  /* O(log n)：x^n = (x^(n/2))² */

/* Fibonacci 三種寫法，n <= 92（fib(92) 是 long long 能放的最大值） */
long long fib_naive(int n, long *calls); /* O(φ^n)，calls 可為 NULL，用來數呼叫次數 */
long long fib_memo(int n);               /* O(n)：記住算過的 */
long long fib_iter(int n);               /* O(n) 時間、O(1) 空間 */

/* 河內塔：把 n 個盤子從 from 移到 to。
 * moves 不是 NULL 時，第 k 步寫進 moves[k][0]（從）、moves[k][1]（到）。
 * 回傳總步數 = 2^n - 1。 */
long hanoi(int n, char from, char to, char via, char (*moves)[2]);

#endif
