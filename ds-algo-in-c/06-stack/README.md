# 06 · Stack 堆疊

## 一句話
**Stack（堆疊）** 是**後進先出 (LIFO, Last-In First-Out)** 的容器：只能從頂端 (top) 放進去 (push) 和拿出來 (pop)，像一疊盤子。

## 圖示
```
push 1, push 2, push 3, pop → 3

        │   │          │   │
        │ 3 │ ← top    │   │
        │ 2 │          │ 2 │ ← top
        │ 1 │          │ 1 │
        └───┘          └───┘
```

### 兩種實作
```
陣列版：data[0..top-1]，top 是下一個空位
  [1][2][3][ ][ ]
            ↑ top = 3

串列版：頭就是 top
  top → 3 → 2 → 1 → NULL
```

### 應用：中序轉後序 (infix → postfix)
`A + B * C` → `A B C * +`
```
讀到   輸出      運算子 stack
A      A
+      A         +
B      AB        +
*      AB        + *      ← * 比 + 優先，直接疊上去
C      ABC       + *
結束   ABC*+               ← 把 stack 剩下的全部倒出來
```
規則：
- 運算元直接輸出。
- `(` 直接 push；遇到 `)` 就一直 pop 到 `(` 為止。
- 運算子：先把 stack 頂端**優先權較高、或相同且左結合**的運算子 pop 出來，再 push 自己。
- `^`（次方）是**右結合**：`A^B^C` = `A^(B^C)` → `ABC^^`。

### 應用：後序式求值
`2 3 * 4 +`：數字 push；遇到運算子就 pop 兩個，算完 push 回去。
```
2 → [2]    3 → [2 3]    * → [6]    4 → [6 4]    + → [10]
```

## 複雜度
| 操作 | 陣列版 | 串列版 |
|---|---|---|
| push | 攤銷 O(1)，擴容時 O(n) | O(1)，每次都要 malloc |
| pop / peek | O(1) | O(1) |
| 額外空間 | 可能有空著的容量 | 每個元素多一個指標 |
| `paren_balanced` | O(n) | |
| `infix_to_postfix` | O(n) | |
| `eval_postfix` | O(n) | |

## 面試陷阱／常考點
1. **pop 前檢查是否為空 (underflow)。** 固定大小的陣列版，push 前也要檢查是否已滿 (overflow)。
2. **後序求值時，先 pop 出來的是右運算元。** `5 2 -` 是 5 − 2，不是 2 − 5。
3. **括號配對最後要檢查 stack 是空的**，否則 `"(("` 會被當成合法。
4. **單調堆疊 (monotonic stack)**：「下一個更大／更小的元素」這類問題的標準解法，O(n)（LeetCode 739）。
5. **遞迴就是用系統的 stack**（Thareja 7.7.4）。任何遞迴都能改用自己的 stack 寫成迴圈。
6. **兩個 stack 可以做出 queue**（LeetCode 232），攤銷 O(1)。
7. **Min Stack**：每格多存「到目前為止的最小值」，getMin 就是 O(1)（LeetCode 155）。
8. **Stack 的常見用途**：瀏覽器上一頁、undo、DFS（第 20 主題）、運算式解析、括號配對。

## 練習題（LeetCode）
| # | 題目 | 難度 | 重點 |
|---|---|---|---|
| 20 | [Valid Parentheses](https://leetcode.com/problems/valid-parentheses/) | Easy | 括號配對 |
| 232 | [Implement Queue using Stacks](https://leetcode.com/problems/implement-queue-using-stacks/) | Easy | 兩個 stack、攤銷 O(1) |
| 155 | [Min Stack](https://leetcode.com/problems/min-stack/) | Medium | 每格存目前最小值 |
| 150 | [Evaluate Reverse Polish Notation](https://leetcode.com/problems/evaluate-reverse-polish-notation/) | Medium | 後序求值 |
| 739 | [Daily Temperatures](https://leetcode.com/problems/daily-temperatures/) | Medium | 單調堆疊 |

解法與說明：[leetcode/README.md](leetcode/README.md)

## 程式
| 檔案 | 內容 |
|---|---|
| `stack.h` / `.c` | 陣列版 `Stack`、串列版 `LStack`、括號配對、中序轉後序（含右結合 `^`）、後序求值 |
| `test_stack.c` | 兩種 stack 做 2 萬次隨機操作對照、各種括號、結合性、錯誤輸入、中序 → 後序 → 求值 |

執行：`make test T=06-stack`

## 出處
- **CLRS** 10.1 Simple array-based data structures（Stacks 小節）· PDF p.344
- **Thareja** 第 7 章 Stacks · p.219–252（7.2 陣列版 p.220、7.4 串列版 p.224、7.6 多重 stack p.227、7.7.2 括號檢查 p.231、7.7.3 運算式求值與轉換 p.232、7.7.4 遞迴 p.243）
