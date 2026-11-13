# Hash Table · LeetCode 解法

每個 `.c` 檔都可以單獨使用。`提交範圍 開始` 到 `結束` 之間的程式碼可以直接貼到 LeetCode 上提交，下面的 `main` 是本機測試。

C 沒有內建雜湊表，所以每題都自己寫一個小的。大部分用「線性探測 + 乘法雜湊」，大約 15 行。

執行：`make test T=08-hash-table`

---

## 706 · Design HashMap（Easy）
[題目](https://leetcode.com/problems/design-hashmap/) · [程式](0706-design-hashmap.c)

**題意**：不用內建函式庫，自己做出 `put`、`get`、`remove`。

**思路：分離鏈結**
- 10007 個 bucket（質數），`key % 10007` 決定放哪條串列。
- `put`：串列裡找到就更新，找不到就插在串列頭。
- `remove`：用「指標的指標」刪除，不用特別處理串列頭（05 主題的技巧）。

**複雜度**：key 分布均勻時平均 O(1)。最多 10⁴ 個 key，每條平均長度 < 1。

**陷阱**：
- 最偷懶的做法是直接開一個 10⁶ + 1 格的陣列，也會過。但面試官想看的是你會處理碰撞。
- `put` 已經存在的 key 要**更新**，不能再新增一個節點。

---

## 1 · Two Sum（Easy，Hash 版）
[題目](https://leetcode.com/problems/two-sum/) · [程式](0001-two-sum.c)

**題意**：找兩個數加起來等於 target，回傳它們的 index。

**思路**：一趟就好。走到 `nums[i]` 時：
1. 先查 `target - nums[i]` 有沒有出現過。有 → 找到答案。
2. 沒有 → 把 `nums[i] → i` 存進 map。

```
nums = [2, 7, 11, 15], target = 9
i=0: 找 7，沒有 → 存 {2:0}
i=1: 找 2，有！→ [0, 1]
```

**複雜度**：O(n) 時間、O(n) 空間。01 主題的排序版是 O(n log n)。

**陷阱**：
- **要先查、再存。** 先存的話，`target = 6, nums[i] = 3` 會找到自己。
- `[3, 3]` 這種重複值的情況：先查就不會出錯。
- `target - nums[i]` 可能超出 int 範圍，本檔先用 `long long` 計算。

---

## 49 · Group Anagrams（Medium）
[題目](https://leetcode.com/problems/group-anagrams/) · [程式](0049-group-anagrams.c)

**題意**：把互為 anagram 的字串分在同一組。

**思路**：互為 anagram 的字串，排序後一定長得一樣。所以用「排序後的字串」當 key：
```
"eat" → "aet"  ┐
"tea" → "aet"  ├─ 同一組
"ate" → "aet"  ┘
"tan" → "ant"  ┐
"nat" → "ant"  ┘
"bat" → "abt"  ── 自己一組
```
- 排序用**計數排序**（只有 26 個字母），每個字串 O(L)。
- 雜湊表：key 是字串，用 FNV-1a 雜湊 + 線性探測。格子裡存「組別編號」。

**複雜度**：O(n · L)，L 是字串長度。用 `qsort` 排序的話是 O(n · L log L)。

**其他 key 的做法**：26 個字母的次數，例如 `"1#0#0#...#"`，一樣是 O(L)。

**陷阱**：
- C 的回傳格式是 `char***` 加上每組的大小 `returnColumnSizes`，要仔細看題目的函式簽名。
- **每組不要一開始就開 n 格**：n 組 × n 格 = O(n²) 記憶體，n = 10⁴ 時會用掉數百 MB。本檔讓每組自己用 realloc 長大。
- 空字串 `""` 也是合法的一組。

---

## 128 · Longest Consecutive Sequence（Medium）
[題目](https://leetcode.com/problems/longest-consecutive-sequence/) · [程式](0128-longest-consecutive-sequence.c)

**題意**：未排序的陣列裡，最長的連續整數序列有多長？要求 O(n)。

**思路**：
1. 全部放進 hash set。
2. 只從**序列的起點**開始往上數。起點的定義是：`x - 1` 不在 set 裡。
```
{100, 4, 200, 1, 3, 2}
1：0 不在 → 起點，往上數 1,2,3,4 → 長度 4
2：1 在 → 不是起點，跳過
100：99 不在 → 起點，101 不在 → 長度 1
```

**為什麼是 O(n)**：每個數字只會在「從它的起點往上數」時被經過一次。不是起點的數字，只花 O(1) 檢查一下就跳過了。

**陷阱**：
- 不檢查「是不是起點」的話，每個數字都往上數，最壞是 O(n²)。
- 走訪 **set** 而不是原陣列：原陣列可能有大量重複值，同一個起點會被數很多次。
- `x - 1`、`x + 1` 在 `INT_MIN`、`INT_MAX` 會溢位，本檔有擋。
- 排序後數一數是 O(n log n)，也會過，但不符合題目要求。

---

## 560 · Subarray Sum Equals K（Medium）
[題目](https://leetcode.com/problems/subarray-sum-equals-k/) · [程式](0560-subarray-sum-equals-k.c)

**題意**：有幾個**連續**子陣列的和剛好等於 k？陣列裡可能有負數。

**思路：前綴和 + 計數 map**
- 前綴和 P[j] = nums[0] + ... + nums[j-1]。
- 子陣列 nums[i..j] 的和 = P[j+1] − P[i]。
- 要等於 k → 往前找有幾個 P[i] 等於 `P[j+1] − k`。

由左往右走，map 記「每個前綴和出現過幾次」：
```
nums = [1, 1, 1], k = 2
map 一開始 {0: 1}              ← 空前綴，代表「從頭開始的子陣列」
cur=1: 找 -1 → 0 次；map {0:1, 1:1}
cur=2: 找  0 → 1 次；map {0:1, 1:1, 2:1}
cur=3: 找  1 → 1 次；總共 2 ✅
```

**複雜度**：O(n)。暴力解是 O(n²)。

**陷阱**：
- **有負數，所以不能用滑動視窗**：視窗變大，和不一定變大。
- **一定要先放 `{0: 1}`**，否則從 index 0 開始的子陣列都算不到。
- **先查、再存**目前的前綴和，否則 k = 0 時會把空子陣列也算進去。
