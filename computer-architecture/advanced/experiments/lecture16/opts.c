// opts.c：第 16 支的 compiler 最佳化例子，一個例子一個函數。用 gcc -O0／-O1／-O2／-O3 -S 看組合語言
int fold(void) { int a = 4 + 6; int b = a * 2; int c = b * b; return c; }      /* constant folding：應該變成 return 400 */
int strength(int x) { return x * 8; }                                            /* × 8 → 左移 3 */
int strength12(int x) { return x * 12; }                                         /* × 12 → 兩個 shift ＋ add（或 lea） */
int cse(int a, int b) { int c = (a + b) * 10; int d = (a + b) * (a + b); return c + d; }  /* a+b 只算一次 */
int dead(int x) { int y = x * 7; int z = y + 3; (void)z; return x; }             /* y、z 沒用到 */
void silent(int *arr, int n) { arr[0] = 3; for (int i = 1; i < n; i++) arr[i] = i; arr[0] = 3; }  /* 第二個 arr[0] = 3 是 silent store */
void hoist(int *a, int n, int *val) { for (int i = 0; i < n; i++) { *val = 5; a[i] = i; } }       /* *val = 5 移到迴圈外 */
void induct(int *a, int n) { for (int i = 0; i < n; i++) { int j = 6 * i; a[i] = j + i; } }       /* 6*i → j += 6 */
void fuse(int *a, int *b, int n) { for (int i = 0; i < n; i++) a[i] = 0; for (int i = 0; i < n; i++) b[i] = 0; }  /* 兩個迴圈 */
int unroll(void) { int sum = 0; for (int i = 0; i < 10; i++) sum += i; return sum; }              /* 10 圈：直接算出 45？ */
