# 08 · Hash Table 雜湊表

## 一句話
**Hash table（雜湊表）** 用一個**雜湊函數 (hash function)** 把 key 算成陣列的 index，直接跳到那一格存取。平均 O(1) 就能新增、查詢、刪除。

## 圖示
```
key ──► hash(key) ──► index ──► table[index]

hash(42) = 3
        ┌───┬───┬───┬────────┬───┐
table:  │   │   │   │ 42→"A" │   │
        └───┴───┴───┴────────┴───┘
          0   1   2     3      4
```

### 碰撞 (collision)：兩個 key 算出同一格
key 很多、格子有限，一定會撞。兩大解法：

**1. 分離鏈結 (separate chaining)**：每格是一條串列，撞到就接在同一條。
```
[0] → NULL
[1] → (17,a) → (9,b) → NULL      ← 17 和 9 撞在一起
[2] → (42,c) → NULL
```

**2. 開放定址 (open addressing)**：全部放在陣列裡，撞到就去找下一個空位。
線性探測 (linear probing)：`h, h+1, h+2, ...`，走到尾端就繞回開頭。
```
put 17 → hash=1 → [1] 空，放進去
put 9  → hash=1 → [1] 有人 → [2] 空，放進去
get 9  → [1] 不是 → [2] 是 ✅
```

### 開放定址的刪除：要留墓碑 (tombstone)
```
[1]=17  [2]=9  [3]=空
刪 17，如果直接設成「空」：
[1]=空  [2]=9
get 9 → hash=1 → [1] 空 → 以為 9 不存在 ❌
所以要設成「墓碑」：查詢時跳過它、繼續往下找；插入時可以重複使用它。
```

### 負載因子 (load factor) α = 元素數 / 格子數
- 鏈結法：α 是平均串列長度。本專案讓 α ≤ 1，超過就把格子數加倍，所有元素重新放 (rehash)。
- 開放定址：α 一定 < 1。α 接近 1 時探測會變很長，本專案保持「有資料 + 墓碑」≤ 一半。

### 雜湊函數
| 方法 | 公式 | 說明 |
|---|---|---|
| 除法法 (division) | h(k) = k mod m | m 最好選**質數**，而且不要太靠近 2 的次方（CLRS 11.3、Thareja 15.4.1） |
| 乘法法 (multiplication) | h(k) = ⌊m · (kA mod 1)⌋ | m 可以是 2 的次方。本專案用 32 位元整數版：`(k × 2654435761) >> (32 − bits)` |
| 平方取中 (mid-square) | 取 k² 中間幾位 | Thareja 15.4.3 |
| 折疊法 (folding) | 把 k 切段相加 | Thareja 15.4.4 |

字串常用 FNV-1a 或 djb2（LeetCode 49 用 FNV-1a）。

## 複雜度
| 操作 | 平均 | 最壞 |
|---|---|---|
| 新增 / 查詢 / 刪除 | **O(1)** | O(n)，全部撞在一起時 |
| 擴容 (rehash) | 攤銷 O(1) | 單次 O(n) |
| 依照順序走訪 | 做不到 | 要排序，得另外 O(n log n) |

| | 分離鏈結 `ChainMap` | 開放定址 `ProbeMap` |
|---|---|---|
| 每個元素的額外空間 | 一個指標 + 一次 malloc | 1 byte 狀態 |
| Cache 表現 | 差，節點分散 | 好，資料連續 |
| 刪除 | 直接移除節點 | 要留墓碑 |
| α 可以 > 1 嗎 | 可以 | 不行 |

## 面試陷阱／常考點
1. **O(1) 是平均情況。** 最壞是 O(n)，而且可以被刻意攻擊（hash flooding）。實務上會用隨機種子的雜湊函數。
2. **開放定址不能直接清空被刪的格子**，要留墓碑（本專案有專門的測試抓這個 bug）。
3. **除法法的 m 選 2 的次方很糟**：只會用到 key 的最低幾位，偶數 key 就全部擠在偶數格。
4. **key 是負數**：C 的 `%` 結果可能是負的，會變成負的 index。本專案先轉成 `unsigned`。
5. **Hash table 沒有順序。** 需要「找最小值」「依序走訪」「找範圍內的 key」時，改用平衡 BST（第 17、18 主題）。
6. **值的範圍很小時，直接開陣列**就好，例如 26 個字母、0..10⁴ 的值（直接定址表，CLRS 11.1）。
7. **常見套路**：
   - 查「補數」有沒有出現過（Two Sum）。
   - 前綴和 + 計數（560）。
   - 把「key 的標準形式」當分組依據（49）。
   - 用 set 判斷有沒有（128）。
8. **C 沒有內建雜湊表**，面試時要能在 10 分鐘內寫出一個簡單的版本（LeetCode 706）。

## 練習題（LeetCode）
| # | 題目 | 難度 | 重點 |
|---|---|---|---|
| 706 | [Design HashMap](https://leetcode.com/problems/design-hashmap/) | Easy | 自己實作鏈結法 |
| 1 | [Two Sum](https://leetcode.com/problems/two-sum/) | Easy | 查補數，O(n) |
| 49 | [Group Anagrams](https://leetcode.com/problems/group-anagrams/) | Medium | 標準形式當 key、字串雜湊 |
| 128 | [Longest Consecutive Sequence](https://leetcode.com/problems/longest-consecutive-sequence/) | Medium | Hash set，只從起點數 |
| 560 | [Subarray Sum Equals K](https://leetcode.com/problems/subarray-sum-equals-k/) | Medium | 前綴和 + 計數 map |

Two Sum 在 01 主題寫過「排序 + 雙指標」的 O(n log n) 版本，這裡是 hash 的 O(n) 版本。

解法與說明：[leetcode/README.md](leetcode/README.md)

## 程式
| 檔案 | 內容 |
|---|---|
| `hash_table.h` / `.c` | 乘法雜湊函數、分離鏈結 `ChainMap`（α > 1 時擴容）、線性探測 `ProbeMap`（含墓碑與重建） |
| `test_hash_table.c` | 雜湊分布均勻、更新與刪除、墓碑 bug 專用測試、20 萬次隨機操作對照陣列 |

執行：`make test T=08-hash-table`

## 出處
- **CLRS** 第 11 章 Hash Tables · PDF p.368（11.1 直接定址表、11.2 鏈結法 p.371、11.3 雜湊函數 p.380、11.4 開放定址 p.394）
- **Thareja** 第 15 章 Hashing and Collision · p.464–488（15.3–15.4 雜湊函數 p.466–468、15.5.1 開放定址 p.469、15.5.2 鏈結法 p.481、15.6 優缺點 p.485）
