# 設計自動化（一）第四支影片完整筆記：Python 示範——transportation 問題的 backtracking、DP 與 UCS（ch2 python example）

> **課程**：Electronic Design Automation (1)（清大 CSR 5302）— Lecture 4 | ch2.python_example（播放清單第 4 支）
> **講者**：李炯霆（JT Li），清大半導體學院，2026 春，NTHU OCW 錄影
> **來源**：YouTube 錄影（11:33，無字幕，whisper 轉錄）＋ 螢幕錄影截圖（終端機執行 `tram.py`）＋ 課本 Wang, Chang, Cheng（eds.）*Electronic Design Automation* Ch4 §4.3–4.4
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【實測】`：在本機（Intel i7-13650HX、Ubuntu、Python 3.10）實際執行的結果
> - `⚠️`：逐字稿辨識不清、或影片／投影片說法不一致之處
> - `[mm:ss]`：影片時間點

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. 問題的定義：Transportation class](#1-問題的定義transportation-class)
- [2. Backtracking 的寫法](#2-backtracking-的寫法)
- [3. Recursion limit：不調高就會炸掉](#3-recursion-limit不調高就會炸掉)
- [4. Dynamic programming 的寫法](#4-dynamic-programming-的寫法)
- [5. Uniform cost search 的寫法](#5-uniform-cost-search-的寫法)
- [6. 執行結果](#6-執行結果)
- [7. 【實測】重寫並計時](#7-實測重寫並計時)
- [8. 一頁速查表](#8-一頁速查表)
- [9. 與前後講次的連結](#9-與前後講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

這支是[第 3 支](lecture03_note.md) §16 的補充：老師直接在終端機裡打開一個 Python 檔 `tram.py`（改寫自 Stanford 線上 AI 課 CS221 的程式），用三種方法解同一個問題並比較速度。

```
Transportation 問題（N = 500）
  ├─ backtracking：遞迴展開所有路徑 → 慢；要調高 recursion limit
  ├─ dynamic programming：futureCost + cache → 很快
  └─ uniform cost search：priority queue → 很快，也能處理有 loop 的圖
三種方法都得到 totalCost = 20
```

---

## 1. 問題的定義：Transportation class

[00:00]–[03:16]

- 【口頭】這個 Python 檔是老師從 Stanford online 的 AI 課學下來的。
- `Transportation` 類別：
  - 初始化時給城市數 N；起點是 city 1。
  - `isEnd(state)`：state 等於 N 就結束。
  - `succAndCost(state)`：列出每個 state 的 successor 和 cost，結果用一個 list 存起來。每個 state 有兩個可能的 action：
    - **walk**：state + 1，花 1 分鐘
    - **tram**（魔術電車）：state × 3，花 3 分鐘（新 state ≤ N 才加入）
- 示範 N = 500，先印出 city 3 和 city 6 的 successor：

```
[('walk', 4, 1), ('tram', 9, 3)]
[('walk', 7, 1), ('tram', 18, 3)]
```

- ⚠️ 這支影片裡的 tram 是 **×3、cost 3**；[第 3 支](lecture03_note.md)投影片的版本是 **×2、cost 2**（CS221 原題）。老師在程式裡改了倍數。逐字稿把「state 9」聽成「state 19」，以螢幕上的輸出為準。

---

## 2. Backtracking 的寫法

[03:16]–[06:02]

- 一開始最佳 cost 設成一個很大的數，history 是空的。
- 遞迴函式帶著目前的 **state**、走到這裡的 **history**、**total cost**：
  - 如果 state 是終點：新的 cost 比存的最佳 cost 小，就更新最佳 cost，並把 history 帶過去。
  - 否則：對每一個可能的 action，走到新的 state，history 加上這一步、total cost 加上這一步的 cost，**遞迴**呼叫。
- 從起點、空 history 開始呼叫，最後印出最佳 cost 和 history。
- 【口頭】這個 recursive 的寫法非常 powerful。但跑到 city 500 時，backtracking 花了一段時間才走完。

---

## 3. Recursion limit：不調高就會炸掉

[06:02]–[06:47]

- 程式前面有一行把 Python 的 **recursion limit** 調得很大。
- 【口頭】把這行 comment 掉再跑，程式一直在做 recursive，整個就炸掉了。所以這行是必須的。
- 這就是[第 3 支](lecture03_note.md)投影片寫的「can comment out recursion limit → then, N=400 will fail」。

---

## 4. Dynamic programming 的寫法

[06:47]–[09:21]

- 【口頭】DP 的寫法很短。
- `futureCost(state)`：
  - 終點回傳 0；
  - 如果 state 已在 **cache** 就直接回傳；
  - 否則對所有可能的 action，算「這一步的 cost + 新 state 的 future cost」，取最小，存進 cache 再回傳。
- 實際呼叫：從起點 city 1 算 future cost。
- 【口頭】backtracking 遞迴會花時間，DP 很快就算完，因為被拜訪過的 state 用 cache 裡的資訊加速——這就是 DP 的 powerful 之處。這個版本沒有把 history 印出來。

---

## 5. Uniform cost search 的寫法

[09:21]–[11:02]

- 用 Python 的 **priority queue**（`heapq`）。【口頭】老師先自己玩了一下：import 進來，一直放東西進去，不是空的就把第一個 pop 出來印。也可以用另一種 priority queue 的寫法。
- UCS 的寫法：
  1. 把起點和它的 cost 0 放進 queue。
  2. `while True` 迴圈：pop 出 queue 裡 cost 最小的 state。
  3. 如果是終點就結束。
  4. 否則把目前 state 所有 action 走到的新 state 和累計 cost 全部放進去。
- 【口頭】因為是 priority queue，放進去時就排好次序，最小的在最上面。這個簡單的寫法也能把圖裡有 **loop** 的情況處理掉。

---

## 6. 執行結果

[11:02]–[11:30]　螢幕截圖

```
--- backtracking
totalCost: 20
('walk', 2, 1)
('tram', 6, 3)
('tram', 18, 3)
('tram', 54, 3)
('walk', 55, 1)
('tram', 165, 3)
('walk', 166, 1)
('tram', 498, 3)
('walk', 499, 1)
('walk', 500, 1)
--- dynamicProgramming
totalCost: 20
--- uniformCostSearch
totalCost: 20
```

- 三種方法都得到 20。
- 【口頭】backtracking 比較花時間；dynamic programming 和 uniform cost search 都很快。
- ⚠️ 逐字稿最後一行「字幕志願者 楊茜茜」是 whisper 的幻覺，不是影片內容。

---

## 7. 【實測】重寫並計時

影片沒有給出程式碼檔，這裡依影片描述重寫：`experiments/lecture04/tram.py`，輸出 `experiments/lecture04/output.txt`。

```python
def dynamic_programming(p):
    cache = {}

    def future(s):
        if p.is_end(s):
            return 0
        if s not in cache:
            cache[s] = min(c + future(t) for _, t, c in p.succ_and_cost(s))
        return cache[s]

    return future(p.start())
```
（`experiments/lecture04/tram.py:47-56`）

結果：

```
succ(3) = [('walk', 4, 1), ('tram', 9, 3)]  succ(6) = [('walk', 7, 1), ('tram', 18, 3)]
backtracking   N=500: totalCost 20  (6.505 s)
dynamicProg    N=500: totalCost 20  (0.520 ms)
uniformCost    N=500: totalCost 20  (0.261 ms)
```

- successor 和最佳路徑（walk 2、tram 6、tram 18、tram 54、walk 55、tram 165、walk 166、tram 498、walk 499、walk 500）和影片螢幕上**完全一樣**。
- 速度：backtracking 6.5 秒；DP 0.5 毫秒；UCS 0.26 毫秒——差了**一萬倍以上**。

backtracking 隨 N 的成長：

| N | 100 | 200 | 300 | 400 | 500 | 700 |
|---|---|---|---|---|---|---|
| 時間 (s) | 0.004 | 0.062 | 0.392 | 1.773 | 6.601 | 51.869 |

- N 每多 100，時間大約乘 4–8 倍，是 exponential 成長（路徑數隨 N 爆炸）。DP 和 UCS 每個 state 只處理一次，是 N 的線性左右。
- 不加 `sys.setrecursionlimit` 時，backtracking 和 DP 的遞迴深度都會到幾百層（每一步 walk 就深一層），超過 Python 預設的 1000 個 frame 就丟 RecursionError——和影片「把那行 comment 掉就炸掉」一致（[第 3 支](lecture03_note.md) §19 也實測了 N = 400 的 DP 會爆）。

---

## 8. 一頁速查表

| 主題 | 重點 |
|---|---|
| 問題 | walk s→s+1 cost 1；tram s→3s cost 3；從 1 到 N = 500 |
| state／action | state = 目前 city；action = walk 或 tram |
| Backtracking | 帶 history、total cost 遞迴；到終點時更新最佳解；exponential 時間 |
| Recursion limit | 遞迴寫法要調高，否則炸掉 |
| DP | futureCost + cache；每個 state 只算一次 |
| UCS | priority queue；pop 最小 cost；能處理 loop |
| 結果 | 三種都 20；backtracking 秒級，DP/UCS 毫秒級 |

---

## 9. 與前後講次的連結

| 本講觀念 | 在哪裡 |
|---|---|
| backtracking、DP、UCS 的理論與投影片 | [第 3 支](lecture03_note.md) §13–16 |
| DP 的另一個應用：technology mapping | 第 6 支：ch10 synthesis |
| shortest path／priority queue 在 routing 的應用（maze routing、A\*） | 第 11–12 支：routing |

---

## 附：本講一句話總結

> **老師在終端機示範改寫自 Stanford CS221 的 `tram.py`：城市 1 到 500，走路 +1 花 1 分鐘、電車 ×3 花 3 分鐘，用 backtracking（帶 history 遞迴、要調高 recursion limit 否則炸掉）、dynamic programming（futureCost + cache）、uniform cost search（priority queue，也能處理 loop）三種方法都得到 totalCost 20；本機重寫後路徑和影片完全一樣，backtracking 要 6.5 秒、而且每多 100 個城市就慢 4–8 倍，DP 和 UCS 只要零點幾毫秒。**

---

*筆記依據李炯霆「Electronic Design Automation (1)」Lecture 4 | ch2.python_example 錄影（NTHU OCW，11:33；影片無字幕，以 whisper large-v3-turbo 轉錄）與螢幕截圖整理。實測為本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、Python 3.10）結果。*
