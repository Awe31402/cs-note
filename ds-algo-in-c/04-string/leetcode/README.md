# 字串 · LeetCode 解法

每個 `.c` 檔都可以單獨使用。`提交範圍 開始` 到 `結束` 之間的程式碼可以直接貼到 LeetCode 上提交，下面的 `main` 是本機測試。

執行：`make test T=04-string`

---

## 344 · Reverse String（Easy）
[題目](https://leetcode.com/problems/reverse-string/) · [程式](0344-reverse-string.c)

**題意**：原地反轉一個 `char` 陣列。注意這題給的是陣列加長度，**沒有 `'\0'`**。

**思路**：`l` 從頭、`r` 從尾，交換後往中間走，直到相遇。

**複雜度**：O(n)，額外空間 O(1)。

**陷阱**：這題不是 C 字串，不能用 `strlen`，要用 `sSize`。

---

## 125 · Valid Palindrome（Easy）
[題目](https://leetcode.com/problems/valid-palindrome/) · [程式](0125-valid-palindrome.c)

**題意**：只看英文字母和數字、忽略大小寫之後，是不是回文 (palindrome)。

**思路**：左右指標。
- 左邊不是英數字 → `l++` 跳過。
- 右邊不是英數字 → `r--` 跳過。
- 兩邊都是 → 轉小寫比較，不同就 false。

**複雜度**：O(n)，額外空間 O(1)。也可以先過濾成新字串再比，但那要 O(n) 空間。

**陷阱**：
- `"0P"` 不是回文：`'0'`（48）和 `'p'`（112）不一樣。有人會誤以為數字也要忽略。
- 呼叫 `isalnum`、`tolower` 前要轉成 `unsigned char`。

---

## 242 · Valid Anagram（Easy）
[題目](https://leetcode.com/problems/valid-anagram/) · [程式](0242-valid-anagram.c)

**題意**：t 是不是 s 重新排列字母得到的（anagram，字母異位詞）。

| 做法 | 時間 | 空間 |
|---|---|---|
| 兩個都排序後比較 | O(n log n) | 看排序法 |
| **26 格計數陣列**（本檔） | O(n) | O(1)，固定 26 格 |

**思路**：s 的每個字母 +1、t 的每個字母 -1，最後 26 格全部是 0 就是 anagram。

**陷阱**：
- 長度不同直接 false。本檔在同一個迴圈裡邊走邊檢查，不用另外呼叫 `strlen`。
- 延伸題：如果輸入是 Unicode，26 格不夠用，要改用 hash table。

---

## 151 · Reverse Words in a String（Medium）
[題目](https://leetcode.com/problems/reverse-words-in-a-string/) · [程式](0151-reverse-words-in-a-string.c)

**題意**：把字的順序反過來。頭尾的空白要去掉，字跟字之間只留一個空白。

**思路（原地，O(1) 額外空間）**：
```
"  the sky  is "
1. 清空白（快慢指標）   → "the sky is"
2. 整個反轉             → "si yks eht"
3. 每個字各自反轉       → "is sky the" ✅
```
跟 189 Rotate Array 的「三次反轉」是同一個想法：整個反轉會把順序顛倒，再把每一段內部轉回來。

**複雜度**：O(n)，額外空間 O(1)。

**陷阱**：
- 清空白時，只有在「已經寫過字」（`w > 0`）的情況下才補空白，這樣開頭就不會多一個空白。
- 用 `split` 再倒過來 `join` 比較好寫，但要 O(n) 的額外空間。

---

## 5 · Longest Palindromic Substring（Medium）
[題目](https://leetcode.com/problems/longest-palindromic-substring/) · [程式](0005-longest-palindromic-substring.c)

**題意**：找最長的回文**子字串**（要連續）。

| 做法 | 時間 | 空間 |
|---|---|---|
| 列舉所有子字串再檢查 | O(n³) | O(1) |
| DP：`dp[i][j]` = s[i..j] 是否回文 | O(n²) | O(n²) |
| **中心擴展**（本檔） | O(n²) | **O(1)** |
| Manacher | O(n) | O(n)，面試很少要求 |

**中心擴展**：回文從中心往兩邊看是對稱的。每個可能的中心都往外擴，擴到兩邊不一樣為止。
- 奇數長度：中心是一個字元，例如 "aba" 的 b。
- 偶數長度：中心在兩個字元之間，例如 "abba" 的 bb 中間。
- 總共 2n - 1 個中心，每個最多擴 O(n)。

**陷阱**：
- **偶數長度的中心很容易忘記**，只寫 `expand(i, i)` 的話 "cbbd" 會答錯。
- `expand` 結束時 l、r 都多走了一步，所以長度是 `r - l - 1`。
- 起點 `i - (len - 1) / 2` 同時適用奇數和偶數長度。
