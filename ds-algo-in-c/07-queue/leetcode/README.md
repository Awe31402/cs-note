# Queue / Deque · LeetCode 解法

每個 `.c` 檔都可以單獨使用。`提交範圍 開始` 到 `結束` 之間的程式碼可以直接貼到 LeetCode 上提交，下面的 `main` 是本機測試。

執行：`make test T=07-queue`

---

## 933 · Number of Recent Calls（Easy）
[題目](https://leetcode.com/problems/number-of-recent-calls/) · [程式](0933-number-of-recent-calls.c)

**題意**：每次 `ping(t)`，回傳 `[t-3000, t]` 這段時間內總共有幾次 ping。t 保證遞增。

**思路**：queue 存時間。每次 ping：
1. 把 t 放進尾端。
2. 從前端丟掉所有 `< t - 3000` 的舊時間。
3. 剩下的個數就是答案。

因為 t 遞增，過期的一定集中在前端，所以只要從前面丟就好。

**複雜度**：每個時間只進出一次，所以攤銷 O(1)。

**陷阱**：題目最多 10⁴ 次呼叫，所以直接用一個大陣列、head 和 tail 都只往右走就好，不用做成環狀。

---

## 225 · Implement Stack using Queues（Easy）
[題目](https://leetcode.com/problems/implement-stack-using-queues/) · [程式](0225-implement-stack-using-queues.c)

**題意**：只用 queue 的操作，做出 stack。

**思路：只用一個 queue**。push x 之後，把 x 前面的 size - 1 個元素依序出隊再入隊，x 就被轉到最前面了。
```
queue: [1 2]    push 3 → [1 2 3]
轉 2 次：         [2 3 1] → [3 1 2]
front = 3 = stack 的 top ✅
```

**複雜度**：push O(n)，pop 和 top O(1)。

**陷阱**：也有兩個 queue 的做法，但複雜度一樣，一個 queue 比較好寫。跟 232（兩個 stack 做 queue）不同，這題**沒辦法做到攤銷 O(1)**。

---

## 622 · Design Circular Queue（Medium）
[題目](https://leetcode.com/problems/design-circular-queue/) · [程式](0622-design-circular-queue.c)

**題意**：設計固定容量 k 的環狀佇列。空的時候 Front 和 Rear 要回傳 -1。

**思路**：陣列 + `head` + `count`。
- 入隊：`a[(head + count) % k] = x; count++`
- 出隊：`head = (head + 1) % k; count--`
- Rear：`a[(head + count - 1) % k]`

**複雜度**：全部 O(1)。

**陷阱**：
- 只用 head、tail 兩個 index 的話，空和滿都是 head == tail。解法是多存 count，或故意空一格（開 k + 1 格）。
- Rear 的位置是 `head + count - 1`，別忘了 -1。

---

## 641 · Design Circular Deque（Medium）
[題目](https://leetcode.com/problems/design-circular-deque/) · [程式](0641-design-circular-deque.c)

**題意**：622 的雙端版本，前後都能放、都能拿。

**思路**：跟 622 一樣的結構，多兩個操作：
- 從前面放：`head = (head - 1 + k) % k`，再寫進 `a[head]`。
- 從後面拿：`count--` 就好，因為尾巴的位置是用 `head + count` 算出來的。

**複雜度**：全部 O(1)。

**陷阱**：`(head - 1) % k` 在 head = 0 時會得到 **-1**（C 的取餘數會保留負號），一定要先加 k。

---

## 239 · Sliding Window Maximum（Hard）
[題目](https://leetcode.com/problems/sliding-window-maximum/) · [程式](0239-sliding-window-maximum.c)

**題意**：大小 k 的視窗從左滑到右，回傳每個位置的視窗最大值。

| 做法 | 時間 |
|---|---|
| 每個視窗掃一遍 | O(nk) |
| Max-heap（存 值 + index，過期的延後刪除） | O(n log n) |
| **單調 deque**（本檔） | **O(n)** |

**單調遞減 deque**（存 index，對應的值由頭到尾遞減）：
```
nums = [1 3 -1 -3 5 3 6 7], k = 3
i=0 1    dq [1]
i=1 3    3 ≥ 1，pop 1          dq [3]
i=2 -1                         dq [3 -1]     → max 3
i=3 -3                         dq [3 -1 -3]  → max 3
i=4 5    pop -3、-1、3          dq [5]        → max 5
...
（這裡寫的是值，程式裡存的是 index）
```
為什麼可以 pop 掉比較小的？新元素 x 比它們晚離開視窗，又比它們大，所以只要 x 還在，它們就不可能是最大值。

**陷阱**：
- deque 存 **index**，才能判斷頭端有沒有滑出視窗（`dq[head] <= i - k`）。
- 從尾端 pop 的條件用 `<=`（相等也 pop），deque 會比較短。用 `<` 答案也對。
- 從 `i >= k - 1` 才開始輸出答案，因為在那之前視窗還沒滿。
