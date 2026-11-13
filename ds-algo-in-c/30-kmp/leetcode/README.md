# KMP · LeetCode 解法

每個 `.c` 檔都可以單獨使用。`提交範圍 開始` 到 `結束` 之間的程式碼可以直接貼到 LeetCode 上提交，下面的 `main` 是本機測試。四題都用同一個 10 行的 `prefix()` 函式。

執行：`make test T=30-kmp`

---

## 28 · Find the Index of the First Occurrence in a String（Easy）
[題目](https://leetcode.com/problems/find-the-index-of-the-first-occurrence-in-a-string/) · [程式](0028-find-the-index-of-the-first-occurrence-in-a-string.c)

**題意**：needle 第一次出現在 haystack 的位置，沒有就回傳 −1。

**思路**：標準 KMP。
1. 算 needle 的前綴函數 π。
2. 掃 haystack：對得上 q++；對不上就 `q = π[q−1]`，直到對得上或 q = 0。
3. q == m 就找到了。

**複雜度**：O(n + m)。

**陷阱**：`"mississippi"` 找 `"issip"`：第一次在 index 1 對到 `"issis"` 才失敗，要靠 π 退回去，繼續從同一個位置比，才會在 index 4 找到。

---

## 459 · Repeated Substring Pattern（Easy）
[題目](https://leetcode.com/problems/repeated-substring-pattern/) · [程式](0459-repeated-substring-pattern.c)

**題意**：s 能不能由某個子字串重複兩次以上組成？

**思路**：`L = π[n−1]`（最長的前綴 = 後綴）。
- 如果 s 是由長度 p 的單位重複組成，那 s 去掉開頭 p 個字元，就等於 s 去掉最後 p 個字元 → 最長的前綴 = 後綴是 n − p。
- 所以最小週期是 **n − L**，而且 n 要能被它整除。
```
"abcabcabc"：π[8] = 6 → 週期 9 − 6 = 3，9 % 3 == 0 ✅
"abaababaab" + "a"（長 11）：π 最後是 6 → 週期 5，11 % 5 ≠ 0 ❌
```

**另一個做法**：`(s + s)` 去掉頭尾各一個字元之後，裡面找得到 s，就是重複字串。

**複雜度**：O(n)。

**陷阱**：`L > 0` 也要檢查。像 `"abc"` 的 L = 0，週期 = n，n % n == 0，但它不是「重複兩次以上」。

---

## 1392 · Longest Happy Prefix（Hard）
[題目](https://leetcode.com/problems/longest-happy-prefix/) · [程式](1392-longest-happy-prefix.c)

**題意**：最長的「既是前綴、又是後綴」的字串（不能是整個 s）。

**思路**：這就是前綴函數 π 的**定義**，答案長度是 `π[n−1]`。

```
"ababab"：π = [0 0 1 2 3 4] → 答案 "abab"（前綴和後綴可以重疊）
```

**複雜度**：O(n)。

**其他做法**：Rabin-Karp，從短到長比較前綴和後綴的雜湊值。也是 O(n)，但要處理碰撞的問題。

---

## 214 · Shortest Palindrome（Hard）
[題目](https://leetcode.com/problems/shortest-palindrome/) · [程式](0214-shortest-palindrome.c)

**題意**：只能在 s 的**前面**加字元，最少加多少才會變成回文？回傳結果。

**思路**：
1. 找 s 的**最長回文前綴** s[0..L)。
2. 剩下的尾巴 s[L..n) 反轉之後放到前面。
```
s = "aacecaaa"
最長回文前綴 = "aacecaa"（L = 7），剩下 "a"
答案 = "a" + "aacecaaa" = "aaacecaaa"
```

**找最長回文前綴的技巧**：組 `t = s + "#" + reverse(s)`，算 π，**π[最後] 就是 L**。
- 一段字串 P 是 s 的前綴，又是 reverse(s) 的後綴 ⇔ P 反過來也是 s 的前綴 ⇔ P 是回文。
- 中間的 `"#"`（不會出現在 s 裡的字元）保證前綴和後綴不會跨過中間。

**複雜度**：O(n)。暴力是每個長度都檢查一次是不是回文，O(n²)。

**陷阱**：一定要有分隔字元 `#`。沒有的話，`s = "aa"` 時 `t = "aaaa"`，π[最後] = 3，會超過 n。
