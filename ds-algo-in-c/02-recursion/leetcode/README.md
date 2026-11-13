# 遞迴 · LeetCode 解法

每個 `.c` 檔都可以單獨使用。`提交範圍 開始` 到 `結束` 之間的程式碼可以直接貼到 LeetCode 上提交，下面的 `main` 是本機測試。

執行：`make test T=02-recursion`

---

## 509 · Fibonacci Number（Easy）
[題目](https://leetcode.com/problems/fibonacci-number/) · [程式](0509-fibonacci-number.c)

**題意**：F(0) = 0，F(1) = 1，F(n) = F(n-1) + F(n-2)。求 F(n)，n ≤ 30。

**思路**：直接照公式遞迴會重複計算，時間 O(1.618ⁿ)。用 `memo[]` 記住算過的值，每個 F(k) 只算一次，時間 **O(n)**。

| 做法 | 時間 | 空間 |
|---|---|---|
| 直接遞迴 | O(φⁿ) | O(n) 堆疊 |
| 遞迴 + memo（本檔） | O(n) | O(n) |
| 迴圈，只留前兩項 | O(n) | **O(1)** |
| 矩陣快速冪 | O(log n) | O(1) |

**陷阱**：
- LeetCode 會在同一個程式裡多次呼叫 `fib()`。本檔的 `memo` 是全域變數，跨呼叫共用也沒關係，因為答案不會變。
- 面試官問「還能更好嗎」，答案是 O(1) 空間的迴圈版，或 O(log n) 的矩陣快速冪。

---

## 206 · Reverse Linked List（Easy）
[題目](https://leetcode.com/problems/reverse-linked-list/) · [程式](0206-reverse-linked-list.c)

**題意**：反轉一條單向串列。

**思路（遞迴）**：
```
head → 2 → 3 → 4 → NULL
       └── 相信 reverseList(2) 會把後段變成 4 → 3 → 2 → NULL，並回傳 4

此時 head(1)->next 還指著 2，而 2 是後段的尾巴：
  head->next->next = head   → 2 指回 1
  head->next = NULL         → 1 變成新的尾巴
```

**複雜度**：時間 O(n)，空間 O(n)（堆疊）。迴圈版只要 O(1) 空間，第 05 主題會寫。

**陷阱**：
- `head->next = NULL` 一定要寫，不然 1 和 2 會互相指，形成環。
- 串列長度到 5000 時遞迴還沒問題；如果到 10⁶，就要用迴圈版。

---

## 21 · Merge Two Sorted Lists（Easy）
[題目](https://leetcode.com/problems/merge-two-sorted-lists/) · [程式](0021-merge-two-sorted-lists.c)

**題意**：合併兩條已經排好序的串列。

**思路（遞迴）**：
- 其中一條是空的 → 回傳另一條（base case）。
- 比較兩個頭，小的那個當答案的頭，它的 `next` = 合併（它剩下的部分, 另一整條）。

**複雜度**：時間 O(n + m)，空間 O(n + m)（堆疊）。

**陷阱**：
- 用 `<=` 而不是 `<`，值相等時 list1 先放，這樣是**穩定 (stable)** 的。merge sort 的穩定性就靠這個。
- 迴圈版要用 dummy 假頭，可以參考 `13-heap/leetcode/0023` 的寫法。

---

## 50 · Pow(x, n)（Medium）
[題目](https://leetcode.com/problems/powx-n/) · [程式](0050-powx-n.c)

**題意**：算 xⁿ，n 可以是負數，範圍是整個 int。

**思路：快速冪**
```
x¹⁰ = (x⁵)²
x⁵  = (x²)² · x
x²  = (x¹)²
x¹  = (x⁰)² · x
```
每次 n 減半，遞迴式 T(n) = T(n/2) + O(1)，答案是 **O(log n)**。

**陷阱**：
- **n = INT_MIN（-2147483648）時，`-n` 在 int 裡會溢位。** 要先轉成 `long long` 再取負號。這是這題最常錯的地方。
- **`pow_rec(x, n/2)` 只能呼叫一次**，把結果存進 `half`。呼叫兩次會變成 O(n)。
- n < 0 時先把 x 換成 1/x。
- 本機測試不能直接用 `==` 比較浮點數，要允許一點誤差。

---

## 22 · Generate Parentheses（Medium）
[題目](https://leetcode.com/problems/generate-parentheses/) · [程式](0022-generate-parentheses.c)

**題意**：列出所有 n 對括號的合法組合。

**思路：回溯 (backtracking)**，一格一格決定要放什麼：
```
                     ""
                 /
               "("
            /       \
         "(("        "()"
        /    \          \
    "((("   "(()"      "()(" ...
```
有兩個規則，它們負責把不合法的分支**剪掉 (pruning)**：
- 放 `(`：只要 `open < n`。
- 放 `)`：只要 `close < open`，也就是右括號不能比左括號多。

填滿 2n 格就收集成一個答案。

**複雜度**：答案有 Catalan(n) 個，約 4ⁿ / (n^1.5)。每個答案要花 O(n) 複製，所以總時間 O(n · Catalan(n))。

**陷阱**：
- 暴力法是產生全部 2²ⁿ 種字串再檢查，很慢。剪枝後，每條分支最後都會是合法答案。
- C 要自己管理回傳的 `char**`：外層陣列用 realloc 長大，每個字串另外 malloc。
- 回溯的標準模板是「做選擇 → 遞迴 → 撤銷選擇」。這題用固定的 `buf[pos]`，下一次直接覆蓋，所以不用撤銷。
