# Stack · LeetCode 解法

每個 `.c` 檔都可以單獨使用。`提交範圍 開始` 到 `結束` 之間的程式碼可以直接貼到 LeetCode 上提交，下面的 `main` 是本機測試。

執行：`make test T=06-stack`

---

## 20 · Valid Parentheses（Easy）
[題目](https://leetcode.com/problems/valid-parentheses/) · [程式](0020-valid-parentheses.c)

**題意**：只含 `()[]{}` 的字串，括號是否正確配對、正確巢狀。

**思路**：左括號 push；右括號就 pop 一個，檢查是不是對應的左括號。
```
"([)]"
( → [(]
[ → [( []
) → pop 出 [，不是 ( → false
```

**複雜度**：O(n) 時間、O(n) 空間。

**陷阱**（三種失敗情況都要處理）：
1. 右括號出現時 stack 是空的：`"]"`。
2. 類型不對：`"(]"`。
3. 結束時 stack 不是空的：`"(("`。

---

## 232 · Implement Queue using Stacks（Easy）
[題目](https://leetcode.com/problems/implement-queue-using-stacks/) · [程式](0232-implement-queue-using-stacks.c)

**題意**：只用 stack 的操作，做出一個 queue（先進先出）。

**思路**：兩個 stack。
```
push 1,2,3 → in = [1 2 3]   out = []
pop        → out 是空的，把 in 整個倒過去：in = []  out = [3 2 1] → pop 出 1 ✅
push 4     → in = [4]   out = [3 2]
pop        → out 不是空的，直接 pop 出 2 ✅（4 先留在 in）
```

**複雜度**：每個元素最多「進 in、搬到 out、從 out 出來」各一次，所以**攤銷 O(1)**。單次 pop 最壞 O(n)。

**陷阱**：**只有 out 空了才能倒。** 如果 out 還有東西就把 in 倒過去，新元素會蓋在舊元素上面，順序就亂了。

---

## 155 · Min Stack（Medium）
[題目](https://leetcode.com/problems/min-stack/) · [程式](0155-min-stack.c)

**題意**：stack 加上 `getMin()`，所有操作都要 O(1)。

**思路**：每一格同時記「值」和「從底部到這一格的最小值」。
```
push -2 → val [-2]        min [-2]
push  0 → val [-2  0]     min [-2 -2]
push -3 → val [-2  0 -3]  min [-2 -2 -3]   getMin = -3
pop     → val [-2  0]     min [-2 -2]      getMin = -2 ✅
```
pop 以後，新的頂端記錄的最小值，本來就不包含被 pop 掉的元素，所以不用重算。

**複雜度**：全部 O(1)，空間 O(n)。

**陷阱**：
- 只用一個變數記最小值是錯的：最小值被 pop 掉之後，就不知道第二小的是誰了。
- 省空間的做法：第二個 stack 只在 `x <= 目前最小值` 時才 push。注意要用 `<=`，否則重複的最小值 pop 一次就會出錯。

---

## 150 · Evaluate Reverse Polish Notation（Medium）
[題目](https://leetcode.com/problems/evaluate-reverse-polish-notation/) · [程式](0150-evaluate-reverse-polish-notation.c)

**題意**：計算後序式 (Reverse Polish Notation)。token 是字串，數字可能是負數。

**思路**：數字 push；運算子就 pop 兩個（**先 pop 的是 b，後 pop 的是 a**），把 `a op b` 的結果 push 回去。

**複雜度**：O(n)。

**陷阱**：
- **`"-11"` 是數字，不是減號。** 判斷運算子要看「長度是 1」而且是 `+-*/` 其中之一。
- **除法向 0 截斷**：-7 / 2 = -3。C 的 `/` 剛好就是這樣（C99 之後有明確規定）。Python 的 `//` 是向下取整，得到 -4，要特別注意。
- 中間結果用 `long long`，避免乘法溢位。

---

## 739 · Daily Temperatures（Medium）
[題目](https://leetcode.com/problems/daily-temperatures/) · [程式](0739-daily-temperatures.c)

**題意**：對每一天，要等幾天才會遇到更暖的一天？等不到就填 0。

**思路：單調遞減堆疊**。stack 裡放「還在等更暖日子」的 index。
```
溫度: 73 74 75 71 69 72 76 73
i=0  73 → st [0]
i=1  74 > 73 → ans[0]=1, pop；push → st [1]
i=2  75 > 74 → ans[1]=1；st [2]
i=3  71      → st [2 3]
i=4  69      → st [2 3 4]
i=5  72 > 69 → ans[4]=1；72 > 71 → ans[3]=2；72 < 75 停 → st [2 5]
i=6  76 → ans[5]=1, ans[2]=4 → st [6]
...
```

**複雜度**：每個 index 最多 push、pop 各一次，所以 **O(n)**。暴力解是每天往後找，O(n²)。

**陷阱**：
- stack 存的是 **index**，不是溫度。用 index 才能算出相差幾天，也才知道要填答案的哪一格。
- 溫度「相等」不算更暖，所以比較要用 `>`，不能用 `>=`。
- 同一招可以解「下一個更大元素」系列：496、503、84（Largest Rectangle in Histogram）。
