# Greedy · LeetCode 解法

每個 `.c` 檔都可以單獨使用。`提交範圍 開始` 到 `結束` 之間的程式碼可以直接貼到 LeetCode 上提交，下面的 `main` 是本機測試。

執行：`make test T=29-greedy`

---

## 455 · Assign Cookies（Easy）
[題目](https://leetcode.com/problems/assign-cookies/) · [程式](0455-assign-cookies.c)

**題意**：小孩 i 要拿到大小 ≥ g[i] 的餅乾才會滿足，每人最多一塊。最多能滿足幾個小孩？

**貪心**：兩邊都由小到大排序。每塊餅乾依序看：夠給「目前胃口最小的小孩」就給他；不夠的話，這塊誰都不夠，丟掉。

**為什麼對**：用「剛好夠」的小餅乾滿足胃口小的小孩，把大餅乾留給胃口大的。把任何最佳解裡的分配換成這種方式，滿足的人數不會變少。

**複雜度**：O(n log n)。

**陷阱**：陣列可能是空的。`qsort` 收到 NULL 是未定義行為，本機的 UBSan 有抓到，所以先判斷長度。

---

## 55 · Jump Game（Medium）
[題目](https://leetcode.com/problems/jump-game/) · [程式](0055-jump-game.c)

**題意**：站在 i 時，最多可以往前跳 `nums[i]` 格。從第 0 格出發，能不能到最後一格？

**貪心**：只要記「目前最遠能到哪裡」`reach`：
- 掃到 i 時，如果 `i > reach`，代表這格到不了 → false。
- 否則用 `i + nums[i]` 更新 reach。

**為什麼不需要 DP**：能到的格子一定是連續的一段 [0, reach]（能到 reach，中間的格子也都跳得到）。

**複雜度**：O(n)、O(1)。DP 版記錄每一格能不能到，是 O(n²)。

---

## 435 · Non-overlapping Intervals（Medium）
[題目](https://leetcode.com/problems/non-overlapping-intervals/) · [程式](0435-non-overlapping-intervals.c)

**題意**：最少刪掉幾個區間，剩下的就不會重疊？（`[1,2]` 和 `[2,3]` 不算重疊）

**貪心**：「最少刪幾個」＝ 總數 −「最多留幾個」，而「最多留幾個」就是 **CLRS 的活動選擇**：
依**結束時間**排序，選每個跟上一個不重疊的區間。

**為什麼要依結束時間**：選最早結束的，留給後面的空間最大。
依**開始時間**排序是錯的：`[1,100] [1,11] [2,12] [11,22]` 會先選到 `[1,100]`，把其他都擋掉。

**複雜度**：O(n log n)。

---

## 452 · Minimum Number of Arrows to Burst Balloons（Medium）
[題目](https://leetcode.com/problems/minimum-number-of-arrows-to-burst-balloons/) · [程式](0452-minimum-number-of-arrows-to-burst-balloons.c)

**題意**：氣球是水平線段 `[start, end]`，在 x 射一支垂直的箭，所有 `start ≤ x ≤ end` 的氣球都會破。最少要幾支箭？

**貪心**：依**右端點**排序。
- 第一支箭射在第一顆氣球的**右端點**：盡量往右射，才能順便射破更多後面的氣球。
- 接下來的氣球，左端點 ≤ 箭的位置就已經破了；否則要一支新的箭，射在它的右端點。

**複雜度**：O(n log n)。

**陷阱**：
- 座標範圍是整個 int（−2³¹ ~ 2³¹−1）。比較函式寫 `return a[1] - b[1];` 會**溢位**，排序結果就錯了。
- 端點相等算射得到：`[1,2]` 和 `[2,3]` 一支箭射在 2 就好。所以條件是 `start > pos` 才需要新箭。

---

## 134 · Gas Station（Medium）
[題目](https://leetcode.com/problems/gas-station/) · [程式](0134-gas-station.c)

**題意**：環狀路線上有 n 個加油站，站 i 可以加 `gas[i]` 公升，開到下一站要花 `cost[i]` 公升。從哪一站出發能繞一圈？做不到回傳 −1。

**兩個關鍵事實**：
1. **總油量 ≥ 總花費，就一定有解**；否則一定無解。
2. 從 start 出發，開到 i 時油箱變負（到不了 i+1）→ **start 到 i 之間的任何一站當起點也都到不了 i+1**。
   因為從 start 開到中間任一站 j 時，油箱都 ≥ 0；改從 j 出發，油箱是從 0 開始，只會更少。

所以一旦油箱變負，直接把起點跳到 i+1、油箱歸零。一趟就找得到。

**複雜度**：O(n)。暴力是每個起點都試一圈，O(n²)。

**陷阱**：本機的隨機測試可能有好幾個起點都可行（LeetCode 保證答案唯一），所以測試是檢查「選出來的起點真的能繞一圈」，而不是跟暴力法的答案比。
