# 05 · Linked List 鏈結串列

## 一句話
**Linked list（鏈結串列）** 是一串用指標接起來的節點。每個節點存一個值，加上指向下一個節點的指標。節點不需要在連續的記憶體裡，所以插入、刪除只要改指標，不用搬資料。

## 圖示

### 單向串列 (singly)
```
head
 │
 ▼
┌───┬───┐   ┌───┬───┐   ┌───┬──────┐
│ 1 │ ●─┼──►│ 2 │ ●─┼──►│ 3 │ NULL │
└───┴───┘   └───┴───┘   └───┴──────┘
```

### 在中間插入：只改兩個指標
```
插入 9 到 1 和 2 之間：
  new->next = p->next;   ← 先接後面（順序不能反！）
  p->next   = new;       ← 再接前面
┌───┐   ┌───┐   ┌───┐
│ 1 │──►│ 9 │──►│ 2 │
└───┘   └───┘   └───┘
```
如果先寫 `p->next = new`，原本指向 2 的指標就被蓋掉了，後面整段都會找不到。

### 迴圈版反轉：三個指標
```
prev  cur   next
NULL   1 ──► 2 ──► 3
       │
每一步：next = cur->next;  cur->next = prev;  prev = cur;  cur = next;
結果：  NULL ◄── 1 ◄── 2 ◄── 3   head = prev
```

### 雙向環狀串列 + 哨兵節點 (sentinel / header node)
```
        ┌──────────────────────────────────┐
        ▼                                  │
    ┌──────┐    ┌───┐    ┌───┐    ┌───┐    │
    │ head │◄──►│ 1 │◄──►│ 2 │◄──►│ 3 │◄───┘
    │(哨兵)│
    └──────┘
空串列：head.next == head.prev == &head
```
哨兵節點不存資料，但它讓「頭」「尾」「空串列」都變成一般情況，程式裡不用寫任何 `if (head == NULL)`。

### 指標的指標 (pointer to pointer)
```c
for (Node **p = head; *p; p = &(*p)->next)
    if ((*p)->val == x) { Node *dead = *p; *p = dead->next; free(dead); }
```
`p` 指向「要改的那個欄位」，一開始是 `head` 本身，之後是前一個節點的 `next`。所以刪頭和刪中間是同一段程式。

## 複雜度
| 操作 | 陣列 | 單向串列 | 雙向串列（含哨兵） |
|---|---|---|---|
| 用 index 存取 | **O(1)** | O(n) | O(n) |
| 頭部插入／刪除 | O(n) | **O(1)** | **O(1)** |
| 尾部插入 | 攤銷 O(1) | O(n)，有 tail 指標就 O(1) | **O(1)** |
| 尾部刪除 | O(1) | O(n)，要找前一個 | **O(1)** |
| 已知節點，刪掉它 | — | O(n)，要找前一個 | **O(1)** |
| 搜尋 | O(n) | O(n) | O(n) |
| 額外空間 | 0 | 每節點 1 個指標 | 每節點 2 個指標 |

## 面試陷阱／常考點
1. **一定要處理空串列和只有一個節點的情況。** 用 `dummy` 假頭（LeetCode 19、2）或指標的指標，可以少掉很多特例。
2. **改指標的順序很重要。** 先接「新節點 → 後面」，再接「前面 → 新節點」。
3. **快慢指標三大用途**：找中間（876）、判斷有沒有環（141）、找倒數第 n 個（19）。
4. **有環的串列不能直接 free**，會無限迴圈。
5. **比較節點要比位址，不是比值**（141 的 `slow == fast`）。
6. **反轉串列是必考題**，迴圈版和遞迴版都要會寫（遞迴版在 02 主題）。
7. **free 之前先存 `next`**，否則 free 掉的節點不能再讀 `->next`。
8. **串列的 cache 表現很差**：節點散在記憶體各處。所以實務上就算要頻繁插入，陣列也常常比較快。
9. **回傳前要不要復原輸入？** 像 234 那樣反轉了一半，面試時主動提「最後會轉回來」會加分。

## 練習題（LeetCode）
| # | 題目 | 難度 | 重點 |
|---|---|---|---|
| 876 | [Middle of the Linked List](https://leetcode.com/problems/middle-of-the-linked-list/) | Easy | 快慢指標 |
| 141 | [Linked List Cycle](https://leetcode.com/problems/linked-list-cycle/) | Easy | Floyd 龜兔賽跑 |
| 234 | [Palindrome Linked List](https://leetcode.com/problems/palindrome-linked-list/) | Easy | 找中間 + 反轉後半 |
| 19 | [Remove Nth Node From End of List](https://leetcode.com/problems/remove-nth-node-from-end-of-list/) | Medium | 前後指標 + dummy |
| 2 | [Add Two Numbers](https://leetcode.com/problems/add-two-numbers/) | Medium | 進位、長度不同 |

另外 206 Reverse Linked List 和 21 Merge Two Sorted Lists 已經在 02 主題用遞迴寫過，本主題的 `sl_reverse` 是迴圈版。

解法與說明：[leetcode/README.md](leetcode/README.md)

## 程式
| 檔案 | 內容 |
|---|---|
| `linked_list.h` / `.c` | 單向串列（push front/back、插入、刪除、搜尋、迴圈反轉）；雙向環狀串列 + 哨兵（push/pop 前後、O(1) 刪節點） |
| `test_linked_list.c` | 頭中尾插入刪除、反向走訪檢查 `prev`、3000 次隨機操作對照陣列 |

執行：`make test T=05-linked-list`

## 出處
- **CLRS** 10.2 Linked lists（含 sentinel 哨兵節點）· PDF p.352
- **Thareja** 第 6 章 Linked Lists · p.162–218（6.2 單向 p.167、6.3 環狀 p.180、6.4 雙向 p.188、6.5 雙向環狀 p.199、6.6 header linked list p.207、6.8 多項式應用 p.211）
