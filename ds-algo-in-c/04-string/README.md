# 04 · 字串 String

## 一句話
C 的**字串 (string)** 其實就是一個 `char` 陣列，最後放一個 `'\0'`（NUL，值是 0）當作結尾記號。沒有長度欄位，所以要知道長度只能一路數到 `'\0'`。

## 圖示
```
char s[8] = "HELLO";

index:  0    1    2    3    4    5    6    7
      ┌────┬────┬────┬────┬────┬────┬────┬────┐
      │ H  │ E  │ L  │ L  │ O  │ \0 │ ?  │ ?  │
      └────┴────┴────┴────┴────┴────┴────┴────┘
strlen(s) = 5，sizeof(s) = 8
```

### 陣列 vs 指標（Thareja 4.4）
```c
char a[] = "hi";     // 陣列：內容複製到 a，可以改 a[0]
char *p  = "hi";     // 指向字串常數：p[0] = 'X' 是未定義行為（通常會當掉）
```

### 插入要連 `'\0'` 一起搬
```
"HelloWorld\0"  在 5 插入 ", "
 → 先把 "World\0" 往後搬 2 格 → "Hello__World\0"
 → 再把 ", " 放進去          → "Hello, World\0"
```

## 複雜度
n = 字串長度，m = 另一個字串（或 pattern）長度。

| 操作 | 時間 | 說明 |
|---|---|---|
| `s_len` / `strlen` | O(n) | 要一路數到 `'\0'` |
| `s_cmp` / `strcmp` | O(min(n, m)) | 遇到第一個不同就停 |
| `s_reverse` | O(n) | 左右交換 |
| `s_copy` | O(n) | |
| `s_index`（暴力搜尋） | O(nm) | KMP 可以做到 O(n + m)，第 30 主題 |
| `s_insert` / `s_delete` | O(n) | 後面的字元都要搬 |
| `s_atoi` | O(n) | |

## 面試陷阱／常考點
1. **忘了留 `'\0'` 的空間。** 長度 n 的字串需要 n + 1 個 byte。這是最常見的緩衝區溢位原因。
2. **`strlen` 是 O(n)。** 寫 `for (i = 0; i < strlen(s); i++)` 會讓整個迴圈變成 O(n²)。要先存成變數。
3. **`strcpy`、`strcat`、`gets` 不檢查長度**，很危險。本專案的 `s_copy` 像 BSD 的 `strlcpy`：最多寫 `cap-1` 個字元，一定補 `'\0'`，回傳原字串長度，讓呼叫者判斷有沒有被截斷。
4. **`strncpy` 不一定會補 `'\0'`**（來源太長時就不會補），很多人以為它是安全版。
5. **比較字元要轉 `unsigned char`。** `char` 在很多平台是 signed，中文 UTF-8 的位元組會變成負數。`isalpha` 等 ctype 函式收到負數是未定義行為。
6. **不要用 `==` 比較字串**，那是在比較指標位址。要用 `strcmp`。
7. **只有小寫字母時，用 `int cnt[26]` 代替 hash table**（LeetCode 242）。
8. **atoi 要處理溢位。** 標準 `atoi` 溢位是未定義行為，實務上要用 `strtol` 並檢查 `errno`。`s_atoi` 用 `long long` 累加，超過就回傳錯誤。
9. **字串常用技巧**：雙指標（反轉、回文）、計數陣列（anagram）、滑動視窗（第 08 主題）、中心擴展（最長回文）。

## 練習題（LeetCode）
| # | 題目 | 難度 | 重點 |
|---|---|---|---|
| 344 | [Reverse String](https://leetcode.com/problems/reverse-string/) | Easy | 雙指標 |
| 125 | [Valid Palindrome](https://leetcode.com/problems/valid-palindrome/) | Easy | 雙指標 + 跳過字元 |
| 242 | [Valid Anagram](https://leetcode.com/problems/valid-anagram/) | Easy | 計數陣列 |
| 151 | [Reverse Words in a String](https://leetcode.com/problems/reverse-words-in-a-string/) | Medium | 清空白 + 兩次反轉 |
| 5 | [Longest Palindromic Substring](https://leetcode.com/problems/longest-palindromic-substring/) | Medium | 中心擴展 |

解法與說明：[leetcode/README.md](leetcode/README.md)

## 程式
| 檔案 | 內容 |
|---|---|
| `mystring.h` / `.c` | 長度、比較、反轉、轉大寫、安全複製、暴力子字串搜尋、取子字串、插入、刪除、atoi |
| `test_mystring.c` | 跟標準函式對照（含 UTF-8 位元組）、截斷、邊界、`INT_MIN`/`INT_MAX` 與溢位 |

執行：`make test T=04-string`

## 出處
- **CLRS**：沒有專門講 C 字串的章節。字串搜尋演算法在第 32 章（第 30 主題 KMP）。
- **Thareja** 第 4 章 Strings · p.115–137（4.1 讀寫字串 p.115–118、4.2 字串操作 p.118–129、4.3 字串陣列 p.129、4.4 指標與字串 p.132）
