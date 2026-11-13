# 07 · Queue / Deque 佇列與雙端佇列

## 一句話
**Queue（佇列）** 是**先進先出 (FIFO, First-In First-Out)** 的容器：從尾端 (rear) 進來，從前端 (front) 出去，就像排隊。
**Deque（雙端佇列，唸作 "deck"）** 兩端都可以進出。

## 圖示
```
enqueue 1, 2, 3 → dequeue 得到 1

front                rear
  ↓                   ↓
┌───┬───┬───┐       ┌───┬───┐
│ 1 │ 2 │ 3 │  →    │ 2 │ 3 │
└───┴───┴───┘       └───┴───┘
```

### 為什麼要做成「環狀」
一般陣列版的 queue，front 會一直往右跑，左邊空出來的格子就浪費了。環狀佇列讓 index 走到尾端後繞回 0：
```
cap = 5，head = 3，count = 3

index:  0     1     2     3     4
      ┌─────┬─────┬─────┬─────┬─────┐
      │  C  │     │     │  A  │  B  │
      └─────┴─────┴─────┴─────┴─────┘
                          ↑head
尾巴的下一格 = (head + count) % cap = (3 + 3) % 5 = 1
```

### 空和滿怎麼分？
如果只用 `front` 和 `rear` 兩個指標，空和滿的時候都是 `front == rear`，分不出來。常見兩種解法：
1. **多存一個 `count`**（本專案用這個）。
2. **故意空一格**：`(rear + 1) % cap == front` 就算滿。Thareja 8.4.1 用的是類似的判斷方式。

### Deque 往前放
```
head = (head - 1 + cap) % cap     ← 加 cap 是因為 C 的 -1 % 5 = -1，不是 4
```

### Deque 擴容不能直接 realloc
資料可能繞過尾端（像上圖的 A B | C），要依照邏輯順序搬到新陣列的開頭，再把 head 設成 0。

## 複雜度
| 操作 | 環狀佇列 `CQueue` | 串列佇列 `LQueue` | 雙端佇列 `Deque` |
|---|---|---|---|
| 尾端加入 | O(1)，滿了就失敗 | O(1) | 攤銷 O(1) |
| 前端移除 | O(1) | O(1) | O(1) |
| 前端加入／尾端移除 | — | — | 攤銷 O(1) / O(1) |
| 用 index 存取 | — | — | O(1) |

## 面試陷阱／常考點
1. **空和滿的判斷**（見上方）。面試問環狀佇列，這幾乎一定會考。
2. **負數取餘數**：C 的 `%` 結果可以是負的，往前一格要寫 `(i - 1 + cap) % cap`。
3. **串列佇列刪到空時，`tail` 也要設成 NULL**，否則 `tail` 會指向已經 free 掉的節點。
4. **BFS 一定用 queue**（第 20 主題）。
5. **單調佇列 (monotonic deque)**：滑動視窗最大／最小值的標準解法，O(n)（LeetCode 239）。
6. **Priority Queue 不是 FIFO**：它依優先權出隊，通常用 heap 做（第 13 主題）。
7. **用 queue 做 stack，或用 stack 做 queue**，是經典面試題（LeetCode 225、232）。

## 練習題（LeetCode）
| # | 題目 | 難度 | 重點 |
|---|---|---|---|
| 933 | [Number of Recent Calls](https://leetcode.com/problems/number-of-recent-calls/) | Easy | 滑動時間視窗 |
| 225 | [Implement Stack using Queues](https://leetcode.com/problems/implement-stack-using-queues/) | Easy | 單一 queue 旋轉 |
| 622 | [Design Circular Queue](https://leetcode.com/problems/design-circular-queue/) | Medium | 環狀陣列、空與滿 |
| 641 | [Design Circular Deque](https://leetcode.com/problems/design-circular-deque/) | Medium | 往前放的取餘數 |
| 239 | [Sliding Window Maximum](https://leetcode.com/problems/sliding-window-maximum/) | Hard | 單調 deque |

解法與說明：[leetcode/README.md](leetcode/README.md)

## 程式
| 檔案 | 內容 |
|---|---|
| `queue.h` / `.c` | 環狀佇列 `CQueue`（固定容量）、串列佇列 `LQueue`、可擴容的環狀雙端佇列 `Deque` |
| `test_queue.c` | 繞回開頭、滿了拒絕、兩萬次隨機操作對照陣列、deque 兩端混合操作與擴容 |

執行：`make test T=07-queue`

## 出處
- **CLRS** 10.1 Simple array-based data structures（Queues 小節，環狀 queue）· PDF p.344
- **Thareja** 第 8 章 Queues · p.253–278（8.2 陣列版 p.254、8.3 串列版 p.256、8.4.1 環狀佇列 p.260、8.4.2 Deque p.264、8.4.3 Priority Queue p.268、8.4.4 多重佇列 p.272、8.5 應用 p.275）
