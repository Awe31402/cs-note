# Operating Systems: A Linux Kernel-Oriented Approach 第二十四講完整筆記：排程（Chapter 5, Part VII）

> **課程**：Operating Systems: A Linux Kernel-Oriented Approach — Lecture 24（Chapter 5, Part VII：Scheduling）
> **講者**：Smruti R. Sarangi
> **來源**：YouTube 錄影逐字稿（41:21，自動字幕）＋ 課本 *Operating Systems: A Linux Kernel-Oriented Approach*（Sarangi, v0.96）§5.4–5.4.2（p.228–236）＋ Linux kernel **v6.2.12** 原始碼（`os/linux-src/`）
> **標記說明**：
> - `【口頭】`：教授在課堂上口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與教授回答
> - `【課本補充】`：課本有、但這堂課沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄（`檔名:行號`），用來對照觀念
> - `【實測】`：在本機（Ubuntu、kernel 6.5.0、x86-64）實際執行的結果（本講多為 Python 暴力驗證）
> - `⚠️ 逐字稿缺字`：自動字幕在這裡掉字或聽不清，內容依上下文或課本補上；也用來標示課堂說法與原始碼不一致之處
> - `[mm:ss]`：影片時間點

---

## 目錄

- [0. 這堂課要做什麼](#0-這堂課要做什麼)
- [1. Task 與 job](#1-task-與-job)
- [2. 排程問題有很多種](#2-排程問題有很多種)
  - [2.1 一顆 CPU、一堆 job](#21-一顆-cpu一堆-job)
  - [2.2 目標函數](#22-目標函數)
  - [2.3 KSW 模型：α | β | γ](#23-ksw-模型α--β--γ)
- [3. 證明最佳的標準方法：交換論證](#3-證明最佳的標準方法交換論證)
- [4. SJF：最短的先做](#4-sjf最短的先做)
  - [4.1 演算法](#41-演算法)
  - [4.2 證明](#42-證明)
  - [4.3 加權版本](#43-加權版本)
- [5. EDF：截止時間最早的先做](#5-edf截止時間最早的先做)
- [6. SRTF：剩餘時間最短的先做](#6-srtf剩餘時間最短的先做)
- [7. 不可能的結果與 NP-complete](#7-不可能的結果與-np-complete)
- [8. 【實測】暴力驗證](#8-實測暴力驗證)
- [9. 實務：不知道 job 要跑多久](#9-實務不知道-job-要跑多久)
- [10. 傳統演算法](#10-傳統演算法)
  - [10.1 FIFO、random、round robin](#101-fiforandomround-robin)
  - [10.2 優先權排程](#102-優先權排程)
  - [10.3 Multi-level feedback queue](#103-multi-level-feedback-queue)
  - [10.4 【原始碼】Linux 的即時排程：每個優先權一條 queue](#104-原始碼linux-的即時排程每個優先權一條-queue)
  - [10.5 【原始碼】`SCHED_FIFO` 與 `SCHED_RR`](#105-原始碼sched_fifo-與-sched_rr)
  - [10.6 【原始碼】排程類別：一個多層 queue](#106-原始碼排程類別一個多層-queue)
- [11. 預告：多處理器排程](#11-預告多處理器排程)
- [12. 一頁速查表](#12-一頁速查表)
- [13. 與後續講次的連結](#13-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 這堂課要做什麼

[00:33] 開始講**排程（scheduling）**。教授說他會用「shed-uling」這個念法。

本講只看**單核心**：

1. 排程問題的分類（KSW 模型）。
2. 幾個可以證明最佳的演算法：SJF、加權 SJF、EDF、SRTF。
3. 稍微改一下就變成 NP-complete 的問題。
4. 實務上的演算法：FIFO、round robin、優先權、多層回饋 queue。

---

## 1. Task 與 job

[00:33] 兩個名詞：

| 名詞 | 意思 |
|---|---|
| **Task** | 交給系統執行的高層工作，例如一個 process |
| **Job** | task 裡**一塊工作**。排程器看到的單位 |

[01:02] 在即時系統裡特別重要。task 可以是：

- **週期性（periodic）**：做一小塊工作，過一段時間再做一小塊，每個週期一塊。還是同一個 task。
- **非週期 / 偶發（aperiodic / sporadic）**：任何時間點都可能冒出一小塊工作。

[01:31] task 也可能就是**一大塊**工作，例如「找出第 117 個質數」。

[02:00] 從排程的角度，**每一塊工作都叫 job**。

- task 只有一塊工作時，task = job。
- 週期性 task：每個週期產生一個新的 job。

排程器認得的是 job，所以本講用「job」這個正式術語。

【課本補充】課本：task 是擁有資源、可以被換出、可以在核心間遷移的 process。它一生可以產生很多 job，每個 job 是一段 CPU 活動，可以獨立排程。

---

## 2. 排程問題有很多種

### 2.1 一顆 CPU、一堆 job

[02:33] 一顆 CPU，job 不停進來，它就不停跑 job。可以做很多假設：

- 所有 job 一開始就到了。
- 每個 job 要跑多久**事先知道**。[03:00] 這本身可能就不實際，但在即時系統、嵌入式系統裡有時知道。
- 順序可以是 J1 J2 J3 J4，也可以是 J4 J1 J2 J3……
- **可搶占（preemptable）**：跑到一半停下來，跑別的，剩下的晚點再跑。[03:30] 或**不可搶占**。

課本 Figure 5.22 的例子：

| Job | J1 | J2 | J3 | J4 |
|---|---|---|---|---|
| 執行時間 | 3 | 2 | 4 | 1 |

### 2.2 目標函數

[03:30] 最常見的是**平均完成時間（mean completion time）**或**總完成時間**：

```
平均完成時間 = (t1 + t2 + t3 + t4) / 4
```

[04:01] job 數固定時，平均和總和是**同一個指標**。

[06:30] 其他目標：

| 目標 | 意思 |
|---|---|
| 平均（總）完成時間 ΣC_i | 系統的反應速度 |
| **最大完成時間 C_max**（課本叫 **makespan**） | [07:02] 單核心不太重要，**多核心很重要**：最後一個 job 什麼時候做完 |
| **加權完成時間** Σw_i C_i | 不同 job 權重不同，高優先權的完成時間比較重要 |
| **最大延遲 L_max** | 有截止時間時用（§5） |

【課本補充】課本：完成時間 = 完成的時刻 − 到達的時刻（中間可能被搶占好幾次）。如果排程器延遲很多 job，平均完成時間就很差。

### 2.3 KSW 模型：α | β | γ

[05:30] 需要一個**分類法**，把所有排程問題放進同一個框架，每個問題有自己的解。

⚠️ 教授說「Carer, Stein and Wine」，正確是 **Karger、Stein、Wein**（KSW 模型，Karger et al., 1999）。

三個參數：

| 參數 | 意思 | 例子 |
|---|---|---|
| **α** | [05:30] **幾顆核心**（機器環境） | 1 = 單核心；10 = 10 顆；n = n 顆 |
| **β** | [06:02] **限制條件** | 可不可搶占（pmtn）？到達時間都一樣（t = 0）還是不同（r_i）？job 之間有沒有依賴（J1 要在 J4 前、J4 要在 J2 前）？有沒有截止時間（d_i）？ |
| **γ** | [06:30] **最佳化目標** | ΣC_i、C_max、Σw_i C_i、L_max |

寫法：`α | β | γ`，例如 `1 || ΣC_j`（單核心、沒有限制、最小化總完成時間）。

⚠️ 課本寫「α 決定機器環境，它指定 job 的數量和每個 job 的處理時間」。α 是**機器**的數量（和種類），不是 job 的數量。教授的說法是對的。

[07:31] 用這個分類會發現：

- 有些問題**容易解**：有多項式時間的演算法給出最佳排程，也能證明。[08:01] 演算法可能有點複雜，但還算簡單。
- 但常常**稍微改一點**，就突然變成 **NP-complete**：不知道多項式時間的演算法。
- [08:31] 那時只能給**啟發式（heuristic）**：多項式時間內給出還不錯的解。

---

## 3. 證明最佳的標準方法：交換論證

[08:31] 這類演算法有一套**標準的證明方法**（不是全部都能證，但很多可以）。

[09:01] **最佳（optimal）**的意思：理論上最好。不管目標函數是什麼，沒有別的演算法能做得**更好**（可以一樣好，但不會更好）。

【口頭】[09:01] 教授說：「Optimal 不是 good，也不是 best。Optimal 是**可能的最好**，比 best 還好，意思是沒有人能做得比它好。」

[09:32] **反證法**：

```
1. 想證明演算法 A 是最佳的。
2. 假設 A 不是最佳 → 存在另一個最佳演算法 A′。
3. A 和 A′ 做相同的決定，直到某個「分歧點」（可能一開始就分歧）。
4. 在分歧點，把 A′ 的排程做一個小交換，得到 A″。
5. 證明 A″ 的目標函數比 A′ 更好。
6. 但 A′ 是最佳的，不可能有更好的 → 矛盾。
7. 所以 A′ 不存在，A 是最佳的。
```

[11:02] 第 4–5 步通常用**簡單的交換（exchange）**就能做到。

---

## 4. SJF：最短的先做

### 4.1 演算法

[04:30] 假設所有 job 一開始就到、執行時間已知：J1 = 3、J2 = 2、J3 = 4、J4 = 1。

[04:30] **Shortest Job First（SJF，最短工作優先）**：

```
時間  0 1   3     6        10
      |J4|J2 |J1  |  J3    |
完成   1   3    6       10      → 總和 20，平均 5
```

（課本 Figure 5.24）

### 4.2 證明

[11:31] 證明 SJF 對 `1 || ΣC_j` 是最佳的：單核心、沒有限制（所以**不可搶占**）、最小化總（平均）完成時間。

[12:30] 假設 A = SJF 不是最佳，A′ 是最佳。

[12:30]–[13:30] 在 A′ 的排程裡，看分歧點：一定有一對**相鄰**的 job：j **緊接在** k 前面，但 **p_j > p_k**（長的在短的前面）。SJF 絕不會這樣排，這就是分歧的原因。

[14:00] 把 j 和 k **交換**，得到 A″。其他 job 都不動：

- 前面的 job 不受影響。
- [14:00] 後面的 job 也不受影響，因為 p_j + p_k 不變。

[14:30] 設 j 在時間 t 開始：

```
A′ ：   t ──[  j (p_j)  ][ k (p_k) ]──
                         ↑          ↑
                     t+p_j      t+p_j+p_k

A″ ：   t ──[ k (p_k) ][  j (p_j)  ]──
                       ↑            ↑
                   t+p_k        t+p_k+p_j
```

| | j、k 對總完成時間的貢獻 |
|---|---|
| A′ | (t + p_j) + (t + p_j + p_k) = **2t + 2p_j + p_k** |
| A″ | (t + p_k) + (t + p_k + p_j) = **2t + 2p_k + p_j** |

[16:31] 相減：A′ − A″ = **p_j − p_k > 0**。

[17:01] 所以 A″ **比 A′ 更好**。但 A′ 應該是最佳的 → 矛盾。所以 A′ 不存在，SJF 是最佳的。

[17:30] 教授：這是經典的**交換論證**。

### 4.3 加權版本

[18:00] 每個 job 有權重 w_j，問題是 `1 || Σw_j C_j`。

[18:32] 演算法：照 **w_j / p_j** 的**非遞增**順序排（大的先）。

[19:02] 檢查：如果所有 w_j = 1，就變成照 1/p_j 由大到小，也就是**最短的先**，就是 SJF。

[19:30]–[20:00] 這是一種加權平均：優先權高的 job（w 大）想早點排。證明一模一樣，用交換論證。

---

## 5. EDF：截止時間最早的先做

[20:30] **EDF（Earliest Deadline First，最早截止時間優先）**，也叫 EDD，在即時系統裡非常流行。

[20:30] 設定：

- 每個 job 有**截止時間（deadline）**：必須在某個時間前做完。
- [21:01] 每個 job 有**不同的到達時間**。

[21:01] **延遲（lateness）** = 完成時間 − 截止時間。希望所有 job 都在截止時間前完成，也就是 lateness 是**負的**。

[21:30] 問題：

```
1 | r_i, d_i, pmtn | L_max
```

單核心、到達時間不同、有截止時間、**可搶占**；最小化**最大延遲** L_max。

[22:01]–[23:04] L_max 的意思：

- 最小化 L_max 不只是「大家都準時」。
- 最小化一個可能是負的數：−10 比 −2 小。所以已經是負的，還會想讓它**更負**（更早完成）。
- **L_max < 0 ⟺ 每個 job 都趕上截止時間**。

[23:04] 這在即時系統很重要：job 有到達時間、截止時間、優先權。高優先權的 job 來了，要搶占低優先權的。

[23:30] 演算法：**永遠跑截止時間最近的那個 job**。

```
J1 跑到一半 ─▶ J2 到了，J2 的截止時間比 J1 早
              ─▶ 馬上 context switch 到 J2
```

- [24:01] 每一刻都在跑截止時間最近的 job。
- 假設 context switch 時間是 0（也有考慮非零 context switch 時間的變體）。
- [24:30] 這樣得到**最佳**的排程。會不會還是錯過截止時間，那是另一回事。
- [24:30] 截止時間一樣就隨便選一個。

【口頭】[24:30] 教授說他們有一整門**即時系統**的課專門研究「會不會錯過截止時間、限制條件是什麼」。（就是本筆記另一門課 Real Time Systems。）

[25:03] 證明：一樣的交換論證。假設 EDF 不是最佳，X 是最佳。X 和 EDF 在某點分歧，因為 X 選了一個截止時間**不是最近**的 job。交換之後得到 X′，L_max 比 X 低。矛盾。

【課本補充】課本：這裡的 EDF 是**非週期**的版本（可以零成本搶占）。即時系統通常考慮週期性 task（截止時間 = 週期），那時可以做更強的可排程性保證（§5.5.2，第 27 講）。證明可參考 Rajib Mall 的書（也就是 Real Time Systems 課的課本）。

---

## 6. SRTF：剩餘時間最短的先做

[26:00] 非即時系統、沒有截止時間：

```
1 | r_i, pmtn | ΣC_i
```

單核心、到達時間不同、可搶占；最小化平均完成時間。

[26:00] 教授提醒：job **不可搶占**時，這類問題常常會變成 NP-complete（§7）。

[26:32] 最佳演算法是 **SRTF（Shortest Remaining Time First，剩餘時間最短優先）**：

```
J1 在跑，還剩這麼多 ─────────────┐
                      J2 來了，它的剩餘時間 = 全部執行時間，但比 J1 剩的少
                      ─▶ 換 J2 跑
```

[27:00]–[27:31] 什麼時候要重新決定？只有兩種「有興趣的事件」，也就是**有新資訊進來**的時候：

1. 新 job 到了。
2. 有 job 做完了。

[27:31] 證明一樣用交換論證。

[28:00]–[28:32] 回顧本講看過的：

| 問題 | 最佳演算法 |
|---|---|
| `1 ‖ ΣC_j`（不可搶占） | SJF |
| `1 ‖ Σw_j C_j` | 照 w_j/p_j 由大到小 |
| `1 \| r_i, d_i, pmtn \| L_max` | EDF |
| `1 \| r_i, pmtn \| ΣC_i` | SRTF |

---

## 7. 不可能的結果與 NP-complete

[29:00] **不可能的結果**：告訴你某個排程問題沒必要找最佳解，因為它是 NP-complete。那就該找**近似解**：不是最好，但還不錯。

**結果 1：所有 job 都在 t = 0 到時，搶占沒有幫助**

[29:33] KSW 論文裡的理論結果：所有 job 都在 t = 0 釋出、執行時間都知道時，**能不能搶占不影響**最佳排程的品質。

[30:01] 因為一開始就有**完整的資訊**。對 **ΣC_i 和 L_max** 都成立。

【課本補充】課本的例子：

- `1 ‖ ΣC_j`：SJF 和 SRTF 排出來一樣，所以搶占沒差。
- `1 | pmtn, d_i | L_max`：不管能不能搶占，EDF 排出來都一樣。

**結果 2：稍微改一下就 NP-complete**

[30:30]–[31:31]

| 問題 | 跟哪個比 | 結果 |
|---|---|---|
| `1 \| r_i \| ΣC_i` | 拿掉 SRTF 問題的 pmtn（不可搶占） | **NP-complete** |
| `1 \| r_i \| L_max` | 同上，目標換成 L_max | **NP-complete** |
| `1 \| r_i, pmtn \| Σw_i C_i` | SRTF 問題加上一般權重 | **NP-complete** |

[31:02] 教授：「如果能搶占，就簡單了，用 SRTF。但只要拿掉搶占……就變 NP-complete。」

[32:00] 小小的改變，就把問題移出「可解」的邊界。

【課本補充】課本解釋為什麼搶占讓問題變簡單：可搶占時，job 可以任意切開、剩下的稍後跑或換核心跑，彈性大很多。這很像**線性規劃**（連續變數，多項式時間可解）和**整數線性規劃**（變數必須是整數，NP-complete）的差別：整數限制帶來組合爆炸。

---

## 8. 【實測】暴力驗證

用 Python 列舉所有排列，驗證上面的說法（`sched1.py`）。

**課本例子（J1 = 3、J2 = 2、J3 = 4、J4 = 1）**：

```
FIFO J1..J4 sumC = 27
SJF ['J4', 'J2', 'J1', 'J3'] sumC = 20
best over all 24 orders = 20
```

SJF 的 20 就是全部 24 種排法裡最好的；照編號順序做是 27。

**隨機 500 組（每組 6 個 job，執行時間、權重 1–9）**：

```
1||sumC   : SJF not optimal in 0 of 500
1||sumwC  : WSPT (w/p desc) not optimal in 0 of 500
```

SJF 和「照 w/p 由大到小」**每一組**都是最佳。

**`1 | r_j | ΣC_j`（不可搶占、到達時間不同）**：試一個直覺的貪心法「目前到了的 job 裡，挑最短的做」：

```
1|r_j|sumC: greedy SPT worse than optimum in 100 of 500; e.g. p,r,greedy,opt = ([9, 1, 1, 2, 9, 6], [7, 0, 10, 8, 10, 10], 112, 100)
```

500 組裡有 **100 組**貪心法不是最佳。例子：job 0 長度 9、在 t = 7 到；但 t = 8、10 會有短 job 到。貪心法在 t = 7 看到只有 job 0，就開始做這個長 job，結果後面的短 job 全被擋住。最佳解反而會**先空等**一下。這種「要不要等」的抉擇，正是問題變難的原因。

**搶占的威力**：長 job（8）在 t = 0 先到，短 job（1、2）在 t = 1、2 到：

```
long job first then short arrivals: non-preemptive FIFO sumC = 28  SRTF sumC = 17
```

- 不可搶占：完成時間 8、9、11，總和 28。這就是 §10.1 的 **convoy effect**。
- SRTF：短 job 一到就搶占，完成時間 2、4、11，總和 17。

---

## 9. 實務：不知道 job 要跑多久

[32:30] 這些理論最大的不實際之處：**假設知道 job 要跑多久**。大多數情況下不知道。

[32:30] 幸好有規律：程式通常在兩種狀態間輪流：

```
[ CPU burst ][ I/O burst ][ CPU burst ][ I/O burst ] ...
```

（課本 Figure 5.25）

- **CPU burst**：一陣 CPU 活動。把它當成一個 job。
- [33:01] **I/O burst**：硬碟、快閃記憶體、網路卡在工作，CPU 沒什麼事。

[33:01] 用**自迴歸移動平均**之類的模型，拿前 k 次 CPU burst 的長度跟一組係數做內積，預測下一次：

```
t_n = α·t_(n−1) + β·t_(n−2) + γ·t_(n−3)          （課本式 5.1）
```

[33:30] 最近一次的權重最大，再前面的小一點。

【口頭】[33:30] 教授說這就是經典的機器學習問題：給一個序列，預測下一個；取最近 k 個觀察值的視窗做點計算。

[34:01] 預測出來的長度，就當成「已知」的執行時間，再用 SJF、SRTF、EDF 等。

【課本補充】課本：這種時間序列的方法常常有效，因為 CPU burst 的長度有**時間相關性**：最近的過去是不久未來的好預測。

【實測】α = 0.5、β = 0.3、γ = 0.2，一串 burst 長度：

```
burst  predicted
    4     5.4
   13     4.6      ← 突然變長，預測跟不上
   13     8.9
   13    11.2      ← 幾次之後追上
   12    13.0
   11    12.5
   12    11.7
```

行為改變時會落後幾步，穩定後就很準。

---

## 10. 傳統演算法

[34:01] 接下來幾個演算法**不能證明最佳**，但用了很久。

### 10.1 FIFO、random、round robin

| 演算法 | 做法 | 問題 |
|---|---|---|
| **FIFO** | [34:01] 一條 queue，每次挑**最前面**的 | [34:31] **convoy effect（車隊效應）**：一個大 job 擋住一堆小 job，小 job 可能是互動式的短工作 |
| **Random** | [34:31] 隨便挑 | — |
| **Round robin** | [34:31] 每個 job 跑一個**時間片（time quantum）**，設個鬧鐘，時間到換下一個 | [35:02] 大家都能公平地用到 CPU，但**沒有優先權**的概念 |

【課本補充】課本：隨機演算法有很多分析，常常表現得還不錯，但現代系統最好用可證明最佳或近似最佳的演算法。round robin 至少對小 job 比較公平，它們比較早做完。

### 10.2 優先權排程

[35:02] 每個 job 有優先權。

- 更高優先權的來了：可搶占就換它；不可搶占就等目前的跑完。
- 每次有機會就挑優先權最高的。

[35:30] 很多演算法都可以看成優先權排程：

| 演算法 | 優先權 |
|---|---|
| SRTF | 剩餘時間越短，優先權越高 |
| FIFO | [36:00] 越早到，優先權越高（優先權 = 1 / 到達時間） |

[36:00] **公平與優先權**之間有拉扯（課本 Figure 5.26）。提高一個 task 的優先權，可能對其他 task 不公平。

[36:00] Linux 的優先權系統很複雜：**140 個優先權等級**，100 個即時、40 個非即時，而且可以動態改變（第 7–8 講）。

[36:32] 一般作業系統（從手機到大型系統）會分：

| 類別 | 例子 |
|---|---|
| 即時（real-time） | 飛彈、火箭上的處理器，要非常即時 |
| 互動（interactive） | [37:00] 打電動，反應很重要 |
| 一般（regular） | 一般運算，例如你的作業 |
| 批次 / 背景（batch） | `kthreadd`、`ksoftirqd`，有空才做一點 |

[37:00] 也可以依**行為**調整。【口頭】教授說他讀過：早期 Windows 對**前景**程式給 **3 倍**優先權加成。把視窗縮小，優先權就降；正在互動，優先權就升。「現在的系統我不確定。」

### 10.3 Multi-level feedback queue

[38:04] Linux 對**即時優先權**的做法：**每個優先權一條 queue**。只要更高優先權的 queue 裡有 job，就**不碰**低優先權的 queue。

[38:04] 教授：如果高優先權的 job 太多，系統會**很不穩定**，連一般工作都跑不了。

[38:33] **多層回饋 queue（multi-level feedback queue）**（課本 Figure 5.27）。教授說「技術上 Linux 沒用這個」：

```
Queue level 1（最高優先權） ──▶ ○ ○ ○
Queue level 2              ──▶ ○ ○
Queue level 3（最低）       ──▶ ○ ○ ○ ○
```

- job 一開始在 level 1。
- 在高優先權 queue 跑了一陣子，就**降級**到 level 2。[39:01] 再待夠久，再降到 level 3。
- [38:33] 教授說這跟第 16 講的 **softirq** 同一個邏輯：處理夠多個之後，剩下的交給低優先權的 `ksoftirqd`。
- 【口頭】[39:01] 教授的比喻：「坐商務艙坐太久了，很快就會被送去經濟艙。」
- [39:31] 每條 queue 可以用**不同的排程演算法**：SRTF、FIFO、round robin……
- 也可以**升級**：例如使用者開始跟一個 task 互動（把視窗放大），它的優先權就變高。

【課本補充】課本：先從最高優先權的 queue 開始排；還有空的核心，再往下排。在高優先權 queue 待太久的降級；在低優先權 queue 等太久的升級，以保證公平。

### 10.4 【原始碼】Linux 的即時排程：每個優先權一條 queue

[38:04] 教授說「Linux 對即時優先權，每個優先權等級維護一條 queue」。原始碼：

`kernel/sched/sched.h:273-276`

```c
struct rt_prio_array {
	DECLARE_BITMAP(bitmap, MAX_RT_PRIO+1); /* include 1 bit for delimiter */
	struct list_head queue[MAX_RT_PRIO];
};
```

| 欄位 | 說明 |
|---|---|
| `queue[MAX_RT_PRIO]` | `MAX_RT_PRIO` = 100：**100 條 queue**，每個即時優先權一條（第 14 講的 `list_head`） |
| `bitmap` | 101 個位元：第 i 個位元 = 1 表示第 i 條 queue **不是空的**。多一個位元當結尾標記 |

挑下一個要跑的即時 task：

`kernel/sched/rt.c:1769-1785`

```c
static struct sched_rt_entity *pick_next_rt_entity(struct rt_rq *rt_rq)
{
	struct rt_prio_array *array = &rt_rq->active;
	struct sched_rt_entity *next = NULL;
	struct list_head *queue;
	int idx;

	idx = sched_find_first_bit(array->bitmap);
	BUG_ON(idx >= MAX_RT_PRIO);

	queue = array->queue + idx;
	if (SCHED_WARN_ON(list_empty(queue)))
		return NULL;
	next = list_entry(queue->next, struct sched_rt_entity, run_list);

	return next;
}
```

| 行 | 說明 |
|---|---|
| 1776 | 在 bitmap 裡找**第一個是 1 的位元**：最高優先權的非空 queue。（kernel 內部數字越小優先權越高。）x86 上這是一道 `bsf` / `tzcnt` 指令，**O(1)**，跟 task 數量無關 |
| 1779 | 取那條 queue |
| 1782 | 拿 queue 的**第一個**（`list_entry` 就是 `container_of`） |

這就是教授說的：只要高優先權的 queue 有東西，就永遠輪不到低優先權的。（為了避免即時 task 把 CPU 吃光，Linux 有 RT throttling：預設每 1 秒最多給即時 task 0.95 秒，`/proc/sys/kernel/sched_rt_runtime_us`。）

### 10.5 【原始碼】`SCHED_FIFO` 與 `SCHED_RR`

同一個即時優先權的 queue 裡面怎麼排？Linux 有兩種政策，正好就是本講的 **FIFO** 和 **round robin**：

`include/uapi/linux/sched.h:114-120`

```c
#define SCHED_NORMAL		0
#define SCHED_FIFO		1
#define SCHED_RR		2
#define SCHED_BATCH		3
...
#define SCHED_IDLE		5
#define SCHED_DEADLINE		6
```

每個 tick 對即時 task 做的事：

`kernel/sched/rt.c:2635-2667`（節錄）

```c
static void task_tick_rt(struct rq *rq, struct task_struct *p, int queued)
{
	...
	/*
	 * RR tasks need a special form of timeslice management.
	 * FIFO tasks have no timeslices.
	 */
	if (p->policy != SCHED_RR)
		return;

	if (--p->rt.time_slice)
		return;

	p->rt.time_slice = sched_rr_timeslice;

	/*
	 * Requeue to the end of queue if we (and all of our ancestors) are not
	 * the only element on the queue
	 */
	for_each_sched_rt_entity(rt_se) {
		if (rt_se->run_list.prev != rt_se->run_list.next) {
			requeue_task_rt(rq, p, 0);
			resched_curr(rq);
			return;
		}
	}
}
```

| 部分 | 說明 |
|---|---|
| `p->policy != SCHED_RR` → return | **`SCHED_FIFO` 沒有時間片**：一直跑，直到它自己阻塞、讓出，或更高優先權的來了 |
| `--p->rt.time_slice` | `SCHED_RR`：每個 tick 時間片減 1 |
| 減到 0 | 補滿時間片；如果同優先權的 queue 裡還有別人，`requeue_task_rt` 把自己**移到 queue 尾端**，`resched_curr` 要求重新排程 |

這就是 round robin：時間到了排到隊尾。時間片長度：

`include/linux/sched/rt.h:57`

```c
#define RR_TIMESLICE		(100 * HZ / 1000)
```

100 毫秒（換算成 jiffies）。

【實測】本機：

```
$ cat /proc/sys/kernel/sched_rr_timeslice_ms
100
$ chrt -m
SCHED_OTHER min/max priority	: 0/0
SCHED_FIFO min/max priority	: 1/99
SCHED_RR min/max priority	: 1/99
SCHED_BATCH min/max priority	: 0/0
SCHED_IDLE min/max priority	: 0/0
SCHED_DEADLINE min/max priority	: 0/0
```

- RR 時間片 100 ms。
- `SCHED_FIFO`、`SCHED_RR` 的使用者優先權是 1–99（99 最高；kernel 內部會反過來換算）。一般 task（`SCHED_OTHER` = `SCHED_NORMAL`）沒有這個即時優先權，改用 nice 值（第 7 講）。
- `SCHED_DEADLINE` 就是 Linux 版的 **EDF**（§5），第 26–27 講會看。

### 10.6 【原始碼】排程類別：一個多層 queue

教授說 Linux「技術上不用多層回饋 queue」。不過 Linux 在**最上層**確實是一個「多層 queue」（只是沒有回饋，task 不會自己升降級）：

`include/asm-generic/vmlinux.lds.h:127-135`

```c
#define SCHED_DATA				\
	STRUCT_ALIGN();				\
	__sched_class_highest = .;		\
	*(__stop_sched_class)			\
	*(__dl_sched_class)			\
	*(__rt_sched_class)			\
	*(__fair_sched_class)			\
	*(__idle_sched_class)			\
	__sched_class_lowest = .;
```

這是 **linker script**：把五個**排程類別（scheduling class）**照順序擺在記憶體裡，位址越低優先權越高。

`kernel/sched/core.c:5938-5944`

```c
	for_each_class(class) {
		p = class->pick_next_task(rq);
		if (p)
			return p;
	}

	BUG(); /* The idle class should always have a runnable task. */
```

從最高的類別開始問「你有要跑的 task 嗎？」，有就跑它：

| 順序 | 類別 | 政策 | 對應本講 |
|---|---|---|---|
| 1 | stop | （kernel 內部用，例如遷移 task） | — |
| 2 | dl | `SCHED_DEADLINE` | EDF |
| 3 | rt | `SCHED_FIFO`、`SCHED_RR` | 每個優先權一條 queue（§10.4） |
| 4 | fair | `SCHED_NORMAL`、`SCHED_BATCH`、`SCHED_IDLE` | CFS（第 26 講） |
| 5 | idle | idle task | 沒事做時跑它 |

- 只要上層類別有 task，下層的就輪不到。
- 最後的 idle 類別**一定**有 task（每顆 CPU 都有 idle task），所以迴圈一定會找到；找不到就是 bug。

---

## 11. 預告：多處理器排程

[40:00] 單處理器排程講完了。下一講：**多處理器排程**和**多資源排程**。

[40:30] 到目前為止看世界的方式很簡單：一顆核心；一種資源就拿一把鎖。queue 有多個格子時用了 semaphore（full、empty），稍微打破這個規則，但還不是真正的多資源排程。

[41:00] 下一講進入真正的多核心、多資源排程。（第 21 講教授提醒過：這部分需要 NP-completeness 的知識。）

---

## 12. 一頁速查表

### 12.1 KSW 記號

| 符號 | 意思 |
|---|---|
| α | 機器（核心）數：1、m |
| β | pmtn（可搶占）、r_i（到達時間）、d_i（截止時間）、依賴 |
| γ | ΣC_i、Σw_i C_i、C_max（makespan）、L_max |

### 12.2 單核心最佳演算法

| 問題 | 演算法 | 證明 |
|---|---|---|
| `1 ‖ ΣC_j` | SJF | 交換相鄰的 j、k：差 p_j − p_k > 0 |
| `1 ‖ Σw_j C_j` | w_j/p_j 由大到小 | 交換論證 |
| `1 \| r_i, d_i, pmtn \| L_max` | EDF | 交換論證 |
| `1 \| r_i, pmtn \| ΣC_i` | SRTF | 交換論證 |

### 12.3 NP-complete

`1 | r_i | ΣC_i`、`1 | r_i | L_max`、`1 | r_i, pmtn | Σw_i C_i`。

全部在 t = 0 到時，搶占對 ΣC_i、L_max 沒有幫助。

### 12.4 實務

| 方法 | 重點 |
|---|---|
| 預測 burst | t_n = α t_(n−1) + β t_(n−2) + γ t_(n−3) |
| FIFO | convoy effect |
| Round robin | 公平、沒優先權；Linux `SCHED_RR` 時間片 100 ms |
| 優先權 | 公平 vs 優先權的拉扯 |
| 多層回饋 queue | 待久降級、互動升級 |

### 12.5 本講看過的原始碼位置

| 位置 | 內容 |
|---|---|
| `kernel/sched/sched.h:273-276` | `rt_prio_array`：100 條 queue + bitmap |
| `kernel/sched/rt.c:1769-1785` | `pick_next_rt_entity`：找第一個非空 queue |
| `include/uapi/linux/sched.h:114-120` | `SCHED_*` 政策 |
| `kernel/sched/rt.c:2635-2667` | `task_tick_rt`：FIFO 沒時間片、RR 到期排隊尾 |
| `include/linux/sched/rt.h:57` | `RR_TIMESLICE` = 100 ms |
| `include/asm-generic/vmlinux.lds.h:127-135` | 排程類別的優先順序 |
| `kernel/sched/core.c:5938-5944` | `for_each_class` 挑下一個 task |

---

## 13. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| **多核心、makespan（C_max）、NP-complete** | **第 25 講**：Multiprocessor scheduling |
| **公平性、nice、排程類別 fair** | **第 26 講**：Linux scheduling（CFS） |
| **EDF、週期性 task、`SCHED_DEADLINE`** | **第 26–27 講**；另一門課 Real Time Systems |
| **即時優先權、priority inversion** | **第 28 講**：Real-time resource allocation |
| **140 個優先權等級** | 回顧第 7–8 講 |

（講次對應依播放清單標題推估。）

---

## 附：本講一句話總結

> **排程器看的是 job（task 裡的一塊工作），排程問題用 KSW 的 α|β|γ 分類（核心數 | 搶占、到達時間、截止時間等限制 | 目標函數）。單核心有幾個可以用交換論證證明最佳的演算法：不可搶占、最小化總完成時間用 SJF（交換相鄰的長短 job，差 p_j − p_k > 0 就矛盾），加權版用 w/p 由大到小，有截止時間要最小化最大延遲用 EDF，可搶占且到達時間不同用 SRTF；但只要拿掉搶占或加上權重，就變成 NP-complete（實測「挑最短的先做」在 500 組裡有 100 組不是最佳，因為有時該先空等）。實務上不知道 job 多長，就用前幾次 CPU burst 加權預測；傳統做法有 FIFO（convoy effect）、round robin、優先權排程和多層回饋 queue。Linux 的即時排程正是每個優先權一條 queue 加一個 bitmap，O(1) 挑最高優先權，同一優先權內 `SCHED_FIFO` 不切、`SCHED_RR` 每 100 ms 輪一次；最上層的 stop > deadline > rt > fair > idle 排程類別則是一個多層 queue。**

---

*筆記依據 Smruti R. Sarangi「Operating Systems: A Linux Kernel-Oriented Approach」Lecture 24（Chapter 5, Part VII）錄影逐字稿（YouTube 自動字幕，41:21）、課本 *Operating Systems: A Linux Kernel-Oriented Approach*（Sarangi, v0.96）§5.4–5.4.2，以及 Linux kernel v6.2.12 原始碼整理。實測為本機（Ubuntu、kernel 6.5.0、x86-64）結果。*
