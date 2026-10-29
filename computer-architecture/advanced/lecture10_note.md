# Advanced Computer Architecture 第十支影片完整筆記：Register renaming 的硬體：physical register file、RAT、free list、dependence check logic（Chapter 4, Part 1/4）

> **課程**：Advanced Computer Architecture — Chapter 4: The Issue, Execute, and Commit Stages - Part 1/4 (Instruction renaming)（播放清單第 10 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：**whisper 轉錄**（本機 `large-v3-turbo`；YouTube 字幕一直下載不到）（54:23）＋ 課本 *Next-Gen Computer Architecture* v3.2 **Ch4** §4.1（p.95–111）
> **章號**：影片 Chapter 4 = 課本 Ch4。
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄（本講沒有）
> - `【實測】`：在本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4、Python 3.10）實際執行的結果
> - `⚠️`：逐字稿缺字、或影片／課本／實測說法不一致之處
> - `[mm:ss]`：影片時間點

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. 先修知識](#1-先修知識)
- [2. 每個 cycle 要改名幾個暫存器](#2-每個-cycle-要改名幾個暫存器)
- [3. 架構暫存器](#3-架構暫存器)
- [4. Physical register file](#4-physical-register-file)
- [5. 例子：avatar](#5-例子avatar)
- [6. 實體暫存器的數量限制了 window](#6-實體暫存器的數量限制了-window)
- [7. 三個硬體結構](#7-三個硬體結構)
- [8. Circular queue](#8-circular-queue)
- [9. RAT（rename table）](#9-ratrename-table)
- [10. Free list](#10-free-list)
- [11. Dependence check logic](#11-dependence-check-logic)
- [12. Available bit](#12-available-bit)
- [13. 【實測】](#13-實測)
- [14. 一頁速查表](#14-一頁速查表)
- [15. 與後續講次的連結](#15-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

第 4 章是 **OOO pipeline 的核心**：最重要的結構和它們怎麼運作。分四講：

| 講 | 內容 |
|---|---|
| **本講（第 10 支）** | renaming：RAT、free list、dependence check logic |
| [第 11 支](lecture11_note.md) | dispatch、wakeup、select、broadcast |
| [第 12 支](lecture12_note.md) | early broadcast、**load-store queue**（記憶體相依） |
| [第 13 支](lecture13_note.md) | instruction commit（ROB） |

[02:39] 原則還是「**out of order inside, in order outside**」，靠 **commit**（也叫 retirement）達成（[第 6 支](lecture06_note.md)）。

---

## 1. 先修知識

[00:44] in-order pipeline（[第 4 支](lecture04_note.md)）、precise exception（[第 6 支](lecture06_note.md)）、OOO 的基本想法：要有一大池指令，依**資料相依順序**而非程式順序發出去。

[01:48] Renaming 消掉 **WAR** 和 **WAW**；RAW 是真相依，「什麼都做不了」，之後的章再看怎麼對付它。

---

## 2. 每個 cycle 要改名幾個暫存器

[03:20] Renaming：把用**架構暫存器**寫的程式，換成用**實體暫存器**寫的程式。

4-issue 處理器，每個 cycle 要改名 **4 條**指令：

- 最多 **8 個讀**的運算元（每條 2 個來源），全部可能是暫存器，每個都要從某個實體暫存器讀。
- 最多 **4 個寫**的運算元，每個都要分到一個實體暫存器。

[04:20] 兩種主要做法：

| 做法 | 現況 |
|---|---|
| 用 **physical register file** 改名 | 現代高效能處理器的做法，**本講** |
| 用 **reservation station** 改名 | 下一章（[第 14 支](lecture14_note.md)） |

---

## 3. 架構暫存器

[04:51] **架構暫存器（architectural register）**：ISA 裡、軟體看得到的暫存器。compiler 把高階語言的變數對應到它們；呼叫函式時要處理它們。

[05:23] 各種 ISA 有幾個：

| ISA | 架構暫存器 |
|---|---|
| x86 | 8 |
| x86-64 | 16（⚠️ 老師先說「x86」，馬上更正為 x86-64） |
| ARM（基本版） | 16 |
| MIPS | 32 |

[06:08] 為了 precise exception，外面要看起來架構暫存器是**照程式順序**更新的，即使裡面是亂序執行。

---

## 4. Physical register file

[07:29] 硬體自動把每個架構暫存器對應到一個**實體暫存器（physical register）**。**軟體完全看不到**，處理器裡有一個硬體的 renaming 引擎，軟體完全不參與。

[08:19] 例子：**16 個架構暫存器、128 個實體暫存器**。

- [08:59] 架構暫存器**只存在概念上**：硬體沒有替它們另外準備儲存空間，只有一個**對應關係**。只有一個 register file，就是 physical register file。
- [09:28] 任何時刻，每個架構暫存器**剛好對應一個**實體暫存器，而且沒有兩個架構暫存器對應到同一個實體暫存器。
- 所以任何時刻有 128 − 16 = **112 個實體暫存器沒被對應**：要嘛是空的，要嘛裝著**以前的對應**的值。
- [10:14] 例：r4 → p5；有一條新指令寫 r4，r4 改成 → p6。但在 p5 被釋放之前，**還在飛的（in flight）**指令可能還要用 p5 的值，所以 p5 還是有效資料；最新的對應是 r4 → p6。

【課本補充】課本 p.97–98：128 個實體暫存器，每個的編號要 7 bit（2⁷ = 128）。

---

## 5. 例子：avatar

[11:16] 慣例（「只為了說明，不是硬體真的這樣做」）：一開始 rᵢ → pᵢ；之後 rᵢ 第 j 次被寫，叫 **pᵢⱼ**（例如 r1 第一次被寫是 p11，再來 p12、p13）。

[13:13] Renaming 的想法：把時間線切成一個暫存器的好幾個**版本（avatar）**：

```
寫 r1(p11) ─ 讀 ─ 讀 ─ 讀 │ 寫 r1(p12) ─ 讀 ─ 讀 │ 寫 r1(p13) …
└──── p11 的一生 ─────┘└── p12 的一生 ──┘
```

[15:10] 不同 avatar 的指令可以**交錯**，因為它們用不同的實體暫存器；唯一要遵守的是「讀的要在寫的之後」。

[15:35] p12 一直是 r1 的最新值，直到 p13 出現；但 p13 出現時，可能還有指令要用 p12，所以要等它們都做完，才能**釋放** p12。

[17:04] 影片的例子逐條改名：

| 原本 | 改名後 | 說明 |
|---|---|---|
| `mov r1, 1` | 寫 **p11** | r1 的第一個 avatar；RAT 的 r1 那格寫成 p11 |
| `add r1, r2, r3` | 寫 **p12**，讀 p2、p3 | WAW 沒關係，給新的就好；r2、r3 還沒被改過，用初始的 p2、p3 |
| 讀 r1、寫 r4 | 讀 **p12**、寫 p41 | 第一個 RAW：用最新的 avatar |
| 寫 r2 | 寫 p21 | |
| 讀 r2、r8，寫 r6 | 讀 p21、p8，寫 p61 | |
| 寫 r1 | 寫 **p13** | 第三個 avatar |
| 讀 r1、r2，寫 r9 | 讀 p13、p21，寫 p91 | p21：往回找最後一次寫 r2 的對應 |

[22:10] 結果：**RAW 還在**（renaming 不會拿掉它）；**WAW、WAR 都不見了**。r1 的三個 avatar 實際上是三個不同的暫存器，指令可以混在一起排，ILP 變多。

[24:34] 【口頭】例子裡有一條是 dead code（寫了沒人讀），很多 compiler 會拿掉；就算沒拿掉，它也可以被搬到任何地方，不影響正確性。

（第 6 支 [§6](lecture06_note.md#6-register-renaming) 講過同樣的概念，這裡是同一套想法放進硬體。）

---

## 6. 實體暫存器的數量限制了 window

[25:02] 重點：

- 任何時刻要有 **16 個**實體暫存器裝著架構暫存器的**最新值**（像一張即時快照）。
- 其他 **112 個**是空的，或裝著**還在飛的**指令需要的中間值。
- [25:47] **In-flight 指令**：還在 pipeline 裡的指令（之後會更精確地定義）。

[26:18] **實體暫存器越多，同時在飛的指令就越多**：實體暫存器用完了，就不能再改名新的指令，rename 停住、decode 停住、fetch 停住，整個**前端（front end）**都停。

[26:59] 三件事互相牽動：

| 因素 | 限制 |
|---|---|
| 實體暫存器的數量 | 能同時在飛幾條會寫暫存器的指令 |
| instruction window 的大小 | 能同時看幾條指令 |
| branch predictor 的準確率 | window 裡有多少條是對的（[第 5 支](lecture05_note.md)） |

設計者決定處理器的大小時，三個都要考慮。它們一起決定可以看到、可以利用的 **ILP**。

（本機實測：實體暫存器真的比 window 先用完，見 [§13.2](#132-實體暫存器先用完)。）

---

## 7. 三個硬體結構

[27:34] Renaming 需要（**全部由硬體做**，軟體不知道）：

| 結構 | 做什麼 |
|---|---|
| **RAT**（register alias table，也叫 rename table） | 架構暫存器編號 → 實體暫存器編號 |
| **Dependence check logic** | 同一個 cycle 一起改名的指令之間有相依時，處理它 |
| **Free list**（free queue） | 還沒被對應的實體暫存器編號；寫暫存器的指令從這裡拿一個 |

[29:45] ⚠️ 老師口誤說「整個過程在**軟體**裡發生」，馬上更正：「抱歉，整個過程在**硬體**裡發生，軟體完全不知道。」

---

## 8. Circular queue

[30:42] 很多硬體結構都是 **circular queue（環狀佇列）**，free list 就是一個。

- 概念上：一個圓圈，有 **head** 和 **tail**；永遠從 tail **enqueue**、從 head **dequeue**。
- [32:37] 實作上：一個陣列（硬體上是一排 flip-flop／暫存器）加兩個指標。
- [33:20] 走到陣列尾巴就**繞回**開頭，所以叫 circular。

```
enqueue(x):                    dequeue():
    tail ← (tail + 1) mod n        x ← array[head]
    array[tail] ← x                head ← (head + 1) mod n
                                   return x
```

- [35:41] 陣列大小通常是 **2 的次方**，mod 和加 1 都很簡單。
- [36:21] 判斷空的：可以比較 head 和 tail，但更簡單的是用一個**計數器**記有幾個元素。

---

## 9. RAT（rename table）

[36:59] 輸入架構暫存器編號、輸出實體暫存器編號，出來的就是 WAW、WAR 都隱含消掉的一串實體暫存器編號。

[37:45] 結構很簡單：16 個架構暫存器就是 **16 列**，第 i 列存 rᵢ 目前對應的實體暫存器編號。

[37:45] 例子（課本 p.99 也有類似的）：

```
add r1, r2, r3
  RAT：r2 → p29、r3 → p34
  free list 頭是 32 → r1 → p32，RAT 的 r1 改成 32
  ⇒ add p32, p29, p34

mov r3, r1
  free list 下一個是 28 → r3 → p28
  r1 從 RAT 讀到 32
  ⇒ mov p28, p32
```

（本機重跑一模一樣，見 [§13.1](#131-改名的模擬)。）

---

## 10. Free list

[40:15] **Free list**：沒被對應的實體暫存器編號組成的 circular queue。

- 要分配：**dequeue** 一個。
- 分配之後，更新 RAT。
- [40:58] **什麼時候把暫存器還回 free list？**「很重要、也很棘手的問題。」之後再講（[第 13 支](lecture13_note.md) commit 時釋放）。

---

## 11. Dependence check logic

[41:36] 問題：同一個 cycle 一起改名的兩條：

```
add r1, r2, r3      ← 寫 r1
add r4, r1, 1       ← 讀 r1
```

[42:25] 兩條**同時**去讀 RAT。第二條讀 r1 的時候，第一條剛分到的 p11 **還沒寫進 RAT**，會讀到 r1 **以前的**對應（舊的、錯的）。

[43:08] 三條的例子：

```
1: add r1, r2, r3
2: add r4, r1, …     ← RAW：讀第 1 條寫的 r1
3: sub r3, r4, r6    ← RAW：讀第 2 條寫的 r4；WAR：寫第 1 條讀的 r3
```

[44:03] 做法：

1. 三個目的暫存器**平行**從 free list 拿：r1 → px、r4 → py、r3 → pz。
2. **第 1 條**最簡單：來源直接用 RAT 讀到的。
3. **第 2 條**的每個來源：一個**多工器**，在「RAT 讀到的」和「第 1 條的目的 px」之間選。控制訊號：**第 1 條的目的 == 這個來源嗎？**
4. [47:46] **第 3 條**的每個來源：三選一（RAT、py、px），兩個比較：
   - 第 2 條的目的 == 這個來源？→ 用 py（**越近的優先**）。
   - 否則第 1 條的目的 == 這個來源？→ 用 px。
   - 否則用 RAT 的。

[46:23] 執行時不知道會遇到什麼指令，所以這些多工器**事先做好**擺著；實際選哪個看比較的結果。

[50:52] 同時改名 4、5、6 條，多工器就更多；訊號要穿過這麼多多工器很慢，這**限制了每個 cycle 能改名幾條**。

【課本補充】課本 p.102 起說這就像 pipeline 的 forwarding：第 2 條不用等 RAT 更新，直接拿第 1 條的結果。

（本機模擬：沒有 dependence check logic 時，第 2 條會讀到舊的對應，見 [§13.1](#131-改名的模擬)。）

---

## 12. Available bit

[51:33] RAT 的每一格再加一個 **available bit**：這個實體暫存器的值**算好了沒**（register file 裡是不是已經有結果）。

[52:18] 指令往後走時看它的兩個來源：

- 都 available：可以直接去 register file 讀值、執行。
- 有一個還沒：要在 **instruction window** 裡**等**，等產生那個值的指令做完。

[53:42] 「現在它像眼中釘」，但下一講會大量用到：等待（wait）、醒來（**wakeup**）、很多指令同時醒來但資源有限時要**挑（select）**一部分去執行。

---

## 13. 【實測】

程式在 `experiments/lecture10/`。

### 13.1 改名的模擬

`renamer.py`：RAT（16 格）、free list（circular queue，一開始是 p16–p127）、dependence check logic（同一組裡越近的優先）。RAT 在一個 cycle **結束**時才寫回，所以同一組裡的指令讀 RAT 會讀到舊值。

```
沒有 dependence check logic（只讀 RAT）：
   r1 ← r2, r3   →   p16 ← p2, p3
   r4 ← r1   →   p17 ← p1         ← 錯：讀到 r1 舊的對應
   r3 ← r4, r6   →   p18 ← p4, p6  ← 錯
有 dependence check logic：
   r1 ← r2, r3   →   p16 ← p2, p3
   r4 ← r1   →   p17 ← p16         ← 對
   r3 ← r4, r6   →   p18 ← p17, p6 ← 對
一個 cycle 一條：
   r1 ← r2, r3   →   p32 ← p29, p34
   r3 ← r1   →   p28 ← p32
```

- 沒有 dependence check logic：同一組裡的第 2、3 條讀到 RAT 裡**上一個 cycle** 的對應（p1、p4），RAW 被弄斷，程式就錯了。這就是 [§11](#11-dependence-check-logic) 為什麼一定要那些多工器。
- 第 3 條寫 r3，第 1 條讀 r3（WAR）：第 3 條拿到新的 p18，第 1 條讀的 p3 不受影響，WAR 消失。
- 一個 cycle 一條的例子和影片 [39:22] 一模一樣：`add p32, p29, p34`、`mov p28, p32`。

### 13.2 實體暫存器先用完

拿第 3 支的視窗測試（兩條互不相干的 pointer chase，每一步都 cache miss，中間夾 K 條指令）改成 `window2.c`，夾的指令有兩種：

- `nop`：**不寫**任何暫存器，不需要新的實體暫存器。
- `add %r9, %r8`：每一條都**寫** r8，每一條都要一個**新的**實體暫存器。

兩個 miss 能同時等的時候約 100–130 ns；第二個 load 進不了 window，要等兩次 miss，約 200 ns 以上：

| K | P-core nop | P-core add | E-core nop | E-core add |
|---|---|---|---|---|
| 128 | 101 | 107 | 115 | 119 |
| 192 | 103 | 124 | 124 | 163 |
| 224 | 103 | 136 | 125 | **223** |
| 240 | | **204** | | |
| 256 | 106 | 207 | **219** | 228 |
| 384 | 116 | 271 | | |
| 512 | **199** | 286 | | |

（單位：ns/圈）

- **P-core**：夾 `nop` 可以到約 **500 條**（第 3 支量到的）；夾 `add` 在 **K ≈ 230–240** 就撐不住了，少了一半。
- **E-core**：`nop` 約 250 條，`add` 約 **200 條**就撐不住。
- 差別就在 `add` 每一條都要一個新的實體暫存器，而 `nop` 不用。實體暫存器先用完，rename 就停住，後面的 load 進不了 window。這正是 [§6](#6-實體暫存器的數量限制了-window) 說的「實體暫存器越多，同時在飛的指令就越多」：**window 的大小不只看 ROB，也看實體暫存器**。
- P-core 大約 230–240 條會寫暫存器的指令就用完。加上 16 個架構暫存器的最新值、還有其他在飛的，代表整數的實體暫存器大約有兩百多個；確切數字 Intel 的公開資料這裡沒有，**待補**。

---

## 14. 一頁速查表

| 主題 | 重點 |
|---|---|
| Renaming | 架構暫存器 → 實體暫存器；消掉 WAW、WAR；RAW 留著 |
| 4-issue | 每 cycle 最多 8 個讀、4 個寫要改名 |
| 兩種做法 | physical register file（現代）、reservation station（下一章） |
| 架構暫存器數 | x86 8、x86-64 16、ARM 16、MIPS 32 |
| PRF | 只有一個 register file；架構暫存器只存在概念上 |
| 16 / 128 | 16 個裝最新值，112 個空的或裝中間值 |
| Avatar | 一次寫 ＋ 後面的讀；不同 avatar 可以交錯 |
| 釋放 | 要等用到它的指令都做完（第 13 支） |
| Window 的限制 | 實體暫存器數、window 大小、predictor 準確率 |
| RAT | 16 列，存目前的對應 |
| Free list | circular queue；dequeue 分配；大小是 2 的次方 |
| Dependence check | 同一組裡：比較前面的目的和這個來源，多工器選；越近的優先；組越大越慢 |
| Available bit | 值算好了沒；沒好就在 window 等 |
| 實測 | 照課本例子改名正確；沒有 DCL 會讀到舊對應；P-core 夾 add 只撐 ~235 條（nop 可到 ~500）、E-core ~200 → 實體暫存器先用完 |

---

## 15. 與後續講次的連結

| 本講觀念 | 在哪裡 |
|---|---|
| WAW、WAR、renaming 的概念、precise exception | [第 6 支](lecture06_note.md) |
| OOO 視窗的大小（夾 nop 的版本） | [第 3 支 §10.2](lecture03_note.md#102-本機的-out-of-order-視窗有多大) |
| Window 大小與 branch predictor 準確率 | [第 5 支 §12](lecture05_note.md#12-要填滿-windowbranch-一定要猜得很準) |
| Wakeup、select（用到 available bit） | [第 11 支](lecture11_note.md) |
| 什麼時候釋放實體暫存器、ROB | [第 13 支](lecture13_note.md) |
| 用 reservation station 改名 | [第 14 支](lecture14_note.md)（Ch5） |
| In-order pipeline 的 forwarding（DCL 的類比） | [第 4 支 §7](lecture04_note.md#7-forwarding) |

---

## 附：本講一句話總結

> **現代 OOO 處理器用一個比架構暫存器大很多的 physical register file 做 renaming（例如 16 個架構暫存器、128 個實體暫存器）：架構暫存器只存在概念上，任何時刻各對應一個實體暫存器，其餘的空著或裝著還在飛的指令需要的舊 avatar；改名靠三個硬體結構：RAT（rename table）記住目前的對應、free list（一個 circular queue）提供空的實體暫存器、dependence check logic 用比較器和多工器處理同一個 cycle 一起改名的指令之間的相依（越近的優先，組越大越慢），再在每一格加一個 available bit 告訴指令值算好了沒；實體暫存器的數量、instruction window 的大小和 branch predictor 的準確率一起決定能利用多少 ILP。本機實測：照課本的例子改名結果完全一樣，拿掉 dependence check logic 就會讀到舊的對應；而在兩個 cache miss 之間夾會寫暫存器的 add，P-core 只撐得住約 235 條（夾不寫暫存器的 nop 可以到約 500 條），表示實體暫存器比 window 先用完。**

---

*筆記依據 Smruti R. Sarangi「Advanced Computer Architecture」Chapter 4: The Issue, Execute, and Commit Stages - Part 1/4 (Instruction renaming) 的 whisper 轉錄稿（本機 `large-v3-turbo`，54:23）、Sarangi *Next-Gen Computer Architecture* v3.2 Ch4 §4.1（p.95–111）整理。實測為本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4、Python 3.10）結果。*
