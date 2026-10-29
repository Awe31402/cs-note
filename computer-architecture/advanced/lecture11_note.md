# Advanced Computer Architecture 第十一支影片完整筆記：Dispatch、wakeup、select、broadcast 與 early broadcast（Chapter 4, Part 2/4）

> **課程**：Advanced Computer Architecture — Chapter 4: The Issue, Execute and Commit Stages - Part 2/4 (Wakeup, Select, and Broadcast)（播放清單第 11 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：**whisper 轉錄**（本機 `large-v3-turbo`；YouTube 字幕一直下載不到）（1:01:40）＋ 課本 *Next-Gen Computer Architecture* v3.2 **Ch4** §4.2.1–4.2.4（p.108–121）
> **章號**：影片 Chapter 4 = 課本 Ch4。
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄（本講沒有）
> - `【實測】`：在本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4、Python 3.10）實際執行的結果
> - `⚠️`：逐字稿缺字、或影片／課本／實測說法不一致之處
> - `[mm:ss]`：影片時間點

⚠️ whisper 轉錄的開頭 [00:00] 少了幾個字（從「…collect stages」開始），最後 [61:26] 之後是沒有內容的「…」，影片在那裡結束、下一講接著講 early broadcast 的麻煩。

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. Dispatch](#1-dispatch)
- [2. Instruction window 的 entry](#2-instruction-window-的-entry)
- [3. Forwarding 在 OOO 裡叫 bypassing](#3-forwarding-在-ooo-裡叫-bypassing)
- [4. 綠色和紅色的指令](#4-綠色和紅色的指令)
- [5. Broadcast 與 wakeup](#5-broadcast-與-wakeup)
- [6. 為什麼只 broadcast tag](#6-為什麼只-broadcast-tag)
- [7. 多條 tag bus](#7-多條-tag-bus)
- [8. Issue 的完整流程](#8-issue-的完整流程)
- [9. Select unit：request 與 grant](#9-select-unitrequest-與-grant)
- [10. 樹狀的 select unit](#10-樹狀的-select-unit)
- [11. 很多個功能單元](#11-很多個功能單元)
- [12. 選誰：select policy](#12-選誰select-policy)
- [13. 什麼時候 broadcast：要能背靠背執行](#13-什麼時候-broadcast要能背靠背執行)
- [14. Early broadcast](#14-early-broadcast)
- [15. 執行要好幾個 cycle 的指令](#15-執行要好幾個-cycle-的指令)
- [16. 【實測】](#16-實測)
- [17. 一頁速查表](#17-一頁速查表)
- [18. 與後續講次的連結](#18-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

```
rename（照順序）→ dispatch：放進 instruction window（照順序）
  → 等來源運算元：producer 做完時 broadcast tag → 比對 → wakeup
  → select：在 ready 的指令裡挑一部分（功能單元有限）
  → 讀 register file 或 bypass → execute
  wakeup ＋ select ＋ 讀運算元 ＋ 送去執行 = issue
關鍵：為了讓相依的指令背靠背執行，要 early broadcast（只送 tag，不送值）
```

---

## 1. Dispatch

[01:47] 到 rename 為止，指令都**照程式順序**走。rename 的輸出是照程式順序的，指令也照程式順序放進 **instruction window**。這個動作叫 **dispatch**。

[02:13] ⚠️ 名詞：不同的書會混用 **dispatch** 和 **issue**。本課的慣例：

| 名詞 | 意思 |
|---|---|
| **Dispatch** | rename stage → instruction window |
| **Issue** | instruction window → 執行單元 |

[02:47] Instruction window 通常也是一個 **circular queue**：硬體裡做不出 linked list 這麼複雜的東西，所以用一個陣列加 head、tail 兩個指標、會繞回來（[第 10 支 §8](lecture10_note.md#8-circular-queue)）。

【課本補充】課本 p.108：issue width 通常在 **1 到 6** 之間。課本 Figure 4.12 和定義（p.109）：window 是 rename 之後暫存指令的地方；把指令放進去叫 dispatch、從裡面送出去執行叫 issue。用 queue 也有「速率控制」的作用。

---

## 2. Instruction window 的 entry

[03:25] 每一格存：

| 欄位 | 內容 |
|---|---|
| **valid** | 這一格有沒有東西（1 bit；幾乎所有 queue 結構都有） |
| **opcode** | add、sub、mul、div… |
| 來源 1：**immediate** 或 **source tag** | 是常數就存常數；是暫存器就存**實體暫存器編號**（叫 tag） |
| 來源 1：**ready bit** | immediate → 1；暫存器 → 從 RAT 的 available bit 來（[第 10 支 §12](lecture10_note.md#12-available-bit)） |
| 來源 2：同上 | |
| **destination tag** | 目的實體暫存器的編號（如果有；例如 store 就沒有） |

[06:27] 還有一個 bit 表示是 immediate 還是暫存器（圖上沒畫）。

[07:22] 最重要的是：**全部都用 tag 來處理**。

【課本補充】課本 p.110 的例子：64 位元處理器、16 個架構暫存器、128 個實體暫存器；imm 欄位放 32 位元的 immediate（decode 時就算好了，所以 immediate 一進來就是 ready）。

---

## 3. Forwarding 在 OOO 裡叫 bypassing

[05:57] OOO pipeline 一樣需要 forwarding：有很多需要好幾個 cycle 的功能單元，「躲不掉」。名字不同、意思完全一樣：

| | 名稱 |
|---|---|
| In-order pipeline | **forwarding** |
| OOO pipeline | **bypassing** |

[09:52] 執行單元前後有一整套 **bypass network**（很多多工器），圖上為了簡單沒畫。指令從 register file 或 bypass network 拿值都可以。

---

## 4. 綠色和紅色的指令

[08:01] 任何時刻，window 裡有：

| | 意思 |
|---|---|
| **綠色** | 所有運算元都 ready |
| **紅色** | 至少一個運算元還沒 ready，在等（像紅燈） |

[09:26] 綠色的都可以執行，但如果只有 2 個加法器而 4 個綠色都是 add，**只能做 2 個**：要**挑（select）**兩個，它們讀值、執行。

[10:39] OOO pipeline 裡**會有 structural hazard**（[第 4 支 §4](lecture04_note.md#4-三種-hazard) 說 in-order 的我們那條 pipeline 不會）。

---

## 5. Broadcast 與 wakeup

[11:23] 問題：一條在等的指令，怎麼知道它的運算元好了？「not ready → ready」要有人告訴它。

[11:59] 答案：**producer** 告訴它。例如 p5 的值被一條指令產生、被後面的指令用。producer 做完時，把它的 **tag**（目的實體暫存器編號）**broadcast** 給 window 裡的每一格。

[12:58] 每一格都在「聽」：檢查 p5 是不是自己的來源之一，是的話就把那個來源的 ready bit 設成 1。

[13:25] 這叫 **wakeup**：喚醒一個運算元。如果一條指令的兩個運算元都 ready 了，就說這條**指令醒了**，可以去讀運算元、執行。

[15:09] 硬體：一條 **tag bus** 送到每一格。每一格有**兩個比較器**（來源 1、來源 2），比對到就設對應的 ready bit。

---

## 6. 為什麼只 broadcast tag

[13:47] 重要的一點：broadcast 的是 **tag**，**不是值**。

- 128 個實體暫存器，tag 只有 log₂128 = **7 bit**。
- 值可能是很胖的 64 位元。
- 7 bit 送起來又快又方便。

[14:35] 為什麼不送值？老師先賣關子（「漢堡」：自己想想），答案在 [§14](#14-early-broadcast)。

---

## 7. 多條 tag bus

[18:46] Multi-issue 處理器每個 cycle 送好幾條指令去執行，每個 cycle 就有好幾個 producer 完成、要 broadcast → **每個 producer 一條 tag bus**。

[19:53] 兩條 tag bus 時，每個來源要**兩個比較器**（跟兩條 bus 各比一次），結果 **OR** 起來設 ready bit。

[20:18] 同一個 tag 不會在兩條 bus 上同時出現，所以最多一個比對成功。

---

## 8. Issue 的完整流程

[16:55]

```
dispatch → 在 window 裡等 → producer broadcast tag → wakeup
  → 完全醒了的指令裡，依 structural hazard 選一部分（select）
  → 從 register file 或 bypass network 讀運算元 → execute
```

[17:41] **Wakeup 並送到執行單元**的整個過程叫 **issue**（從 instruction window 送到執行單元）。

---

## 9. Select unit：request 與 grant

[21:21] 每一格有一條 **request line**：ready（綠色）就把它設成 1。

[22:23] **Select unit**：從所有 request 裡挑一個，把它的 **grant line** 設成 1，其他的 grant 都是 0。拿到 grant 的就去讀 register file、執行。

[22:48] 先看最簡單的：只有一種指令、只有**一個**功能單元。

---

## 10. 樹狀的 select unit

[24:13] 用一棵**樹**：葉子是 request，內部的節點叫 **choice box**（不一定是二元樹，老師的圖是最下層一個 box 收 4 個 request、上面每個收 2 個）。

每個 choice box：

| 輸入 | 做什麼 |
|---|---|
| 全是 0 | 輸出 0 |
| 一個是 1 | 記住是哪一個，輸出 1 |
| 好幾個是 1 | 挑一個（怎麼挑之後講），**記住**它，輸出 1 |

[25:51] 要記住的狀態很少：4 個輸入只要一個 2 bit 的暫存器。

[26:30] 一路往上到**根**。根挑一個之後，**grant 沿著原路往下走**：每個 choice box 照它記住的那一個往下傳。

[27:05] 【口頭】「誰說樹、linked list、queue 只是軟體的資料結構？這裡就是一棵**做在硬體裡的樹**。」

---

## 11. 很多個功能單元

[29:01] 例如 2 個加法器、2 個乘法器、1 個除法器：

- 每一**類**指令一個 select unit：加法器的 select unit 只看 add 類的指令，其他的當作沒有 request。
- 【口頭】加法器是「被超用的單元」：加法、減法，還要算 load、store 的位址，所以通常加法器比需要的多。

[29:54] 同一類有好幾個（例如 2 個加法器），一個 select unit 要選 **2 條**。三種做法：

| 做法 | 怎麼做 | 缺點 |
|---|---|---|
| **串起來** | 第一個 select unit 選一個；被選中的 request 跟 grant XOR 掉，剩下的送給第二個 select unit 再選一個 | [32:13] 要**兩倍時間** |
| **複雜的 choice box** | 每個 box 最多往上送 n 個 request，根最多 grant n 個 | 比較快，但 choice box 很複雜 |
| **對稱的兩棵樹** | 兩棵一樣的樹平行跑：第一棵兩邊都要時選**左邊**，第二棵選**右邊** | （最好的做法） |

[35:52] 對稱的兩棵樹：可以證明只要有 **≥ 2 個** request，兩棵樹**一定不會選到同一個**。兩個 request 最後一定會在某個 choice box「相遇」，那裡一棵選左、一棵選右，結果就不同。延遲跟一棵樹一樣，吞吐量兩倍。

[37:20] 【口頭】「用一點數學和聰明的設計，大幅加快硬體。」老師也邀請大家自己證明。（本機把所有組合都試過了，見 [§16.1](#161-兩棵樹會不會選到同一個)。）

---

## 12. 選誰：select policy

[38:27] Select policy 對效能影響很大。指令不是都一樣：

- 有的指令沒人依賴它 → 優先權應該**低**。
- 有的指令很多指令依賴它 → 先做它，很多指令會一起醒來，ILP 大增。

[39:01] 好的 select unit 要找出「很多指令依賴的」那些。在硬體裡又快又準很難，但很多現代的 select unit 都有某種估計。

| Policy | 評價 |
|---|---|
| **Random** | 最簡單，但最不實用 |
| **Oldest first** | 公平：在 window 裡最久的優先；window 是 queue，從位置就大概知道多老 |
| **年紀 ＋ 影響** | 理想的做法 |

[40:31] 【口頭】就像選村長：大家自然尊重年紀，但也看誰最有能力（這裡的「能力」是有幾條指令依賴它）。如果永遠只選最有影響的，有些指令會**餓死（starvation）**。

[40:58] 其他考量：**load 的優先權通常比 store 高**。

- 誰依賴 store？只有之後讀同一個位址的 load，而那種情況有 **load-store queue** 處理（[第 12 支](lecture12_note.md)）；一般程式很少寫了又馬上讀回來。
- Load 在**關鍵路徑**上：它的值越早回來，越多指令能醒來。

[42:11] 現代的 select unit 會算一個分數：年紀、影響（有幾條依賴它）、opcode（load 比 store 高）……這個函數還會隨時間**學習、改變**。分數送給 choice box 去比。這種複雜度是「好的複雜度」，因為它提升效能。

---

## 13. 什麼時候 broadcast：要能背靠背執行

[43:38] 例子：

```
add r1, r2, r3
add r4, r1, r5      ← RAW（rename 之後還是 RAW）
```

[44:06] In-order pipeline 有 forwarding（MA → EX），這兩條可以**背靠背（back-to-back）**執行：連續兩個 cycle。

[44:33] OOO pipeline 如果做不到，碰到一長串相依的程式碼，IPC 會**比 in-order 還低**，這不能接受。

[45:31] 要求：producer 在 cycle i wakeup 並往執行走，consumer 要在 **cycle i + 1** wakeup，中間不能有 stall。

[46:27] 設計點：假設 **wakeup 和 select 在同一個 cycle** 做完（兩個效能關鍵的步驟擠在一起，「假設我們有好的設計師」；分開的話更複雜，課本最後有習題）：

```
rename → dispatch → wakeup＋select → register read → execute → write back
```

[47:02] 三種 broadcast 的時間點（pipeline 有完整的 bypass）：

| Producer 什麼時候 broadcast | 中間空幾個 cycle |
|---|---|
| 跟 **write back** 一起（最直覺） | **2**（[47:34]） |
| 跟 **execute** 一起 | **1**（[49:41]） |
| 跟 **register read** 一起 | **0：背靠背** |

[48:17] 空出來的 cycle 不一定是 bubble：這是 OOO，可以塞別的指令進去。但如果程式 ILP 很低、是一長串相依，就真的是 bubble，而 in-order 不用這樣。「這讓我們不太開心。」

（本機模擬見 [§16.2](#162-broadcast-的時間點)。）

---

## 14. Early broadcast

[50:03] 選項 3：producer 在 **register read** 的時候就 broadcast tag，叫 **early broadcast**。

- 同一個 cycle，consumer 收到 tag、wakeup、select。
- 下一個 cycle：producer 在 execute，consumer 在 register read。consumer 從 register file **讀不到**對的值，沒關係。
- 再下一個 cycle：consumer 在 execute，producer 剛算好，用 **bypass** 送過去。背靠背。

[52:12] 這是一種**推測（speculative）**：假設執行只要一個 cycle。「我們之後會談到這個假設的後果。」（[第 12 支](lecture12_note.md)：early broadcast 的麻煩。）

[54:40] 另一個大膽的假設：broadcast 在 cycle 一開始，consumer 在同一個 cycle 就醒來，而且（最好的情況下）剛好被選中；實際上也可能晚幾個 cycle 才被選。

[60:10] **[§6](#6-為什麼只-broadcast-tag) 的答案**：為什麼 broadcast tag 而不是值？因為 **early broadcast 的時候，值還沒算出來**，根本沒有值可以送。

---

## 15. 執行要好幾個 cycle 的指令

[55:41] 例如 **load**：執行要 2 個 cycle（第一個算位址、第二個存取記憶體）。

[56:23] 這時背靠背本來就不可能，in-order pipeline 也要停 1 個 cycle（load-use，[第 4 支 §9](lecture04_note.md#9-load-use-hazard)）。OOO 的好處是：那 1 個 cycle 不用 stall，可以做別的指令。

[57:18] 但是 load **不能**一被選中就 broadcast，要**晚一個 cycle**（在 execute 的第一個 cycle）。

[58:02] 【口頭】「給你 5 秒冥想這張圖：3、4、5。」重點：**不同的指令在不同的時間 broadcast**。

[59:17] 一般化：

> 執行需要 **k** 個 cycle 的指令，在**被選中之後 k 個 cycle** broadcast tag。

| 指令 | k | 什麼時候 broadcast |
|---|---|---|
| add | 1 | 被選中後 1 cycle（register read） |
| load（L1 命中） | 2 | 被選中後 2 cycle（execute 的第一個 cycle） |

---

## 16. 【實測】

程式在 `experiments/lecture11/`。

### 16.1 兩棵樹會不會選到同一個

`select.py` 做了一個二元樹的 select unit（choice box 兩邊都有 request 時，第一棵選左、第二棵選右），把**所有** request 組合都試一遍：

```
 4 個 entry：16 種 request 組合，≥ 2 個 request 時兩棵樹選到同一個：0 次；只有 1 個 request 時兩棵都選它：True
 8 個 entry：256 種 request 組合，≥ 2 個 request 時兩棵樹選到同一個：0 次；只有 1 個 request 時兩棵都選它：True
16 個 entry：65536 種 request 組合，≥ 2 個 request 時兩棵樹選到同一個：0 次；只有 1 個 request 時兩棵都選它：True
```

- 影片 [35:52] 的定理成立：只要有 2 個以上的 request，兩棵樹一定選不同的。
- 只有 1 個 request 時兩棵都會選它，所以實際用時要再加一個簡單的判斷（例如第二個 adder 在只有一個 request 時不動作）。

### 16.2 Broadcast 的時間點

`select.py` 後半照影片的 pipeline（rename、dispatch、wakeup＋select、register read、execute、write back）算 consumer 最早什麼時候能執行：

```
跟 write back 一起 broadcast         ：producer 做完到 consumer 開始執行，中間空 2 個 cycle
跟 execute 一起                      ：producer 做完到 consumer 開始執行，中間空 1 個 cycle
跟 register read 一起（early broadcast）：producer 做完到 consumer 開始執行，中間空 0 個 cycle
load（執行 2 cycle），select 後 2 cycle 才 broadcast：中間空 0 個 cycle
load 卻跟 add 一樣，select 後 1 cycle 就 broadcast：中間空 -1 個 cycle（負的 = consumer 太早，拿不到值）
```

- 前三行就是影片 [47:34]、[49:41]、[50:32] 的 2、1、0。
- Load 照「k 個 cycle 後 broadcast」：0，剛好接上。如果 load 也跟 add 一樣早 broadcast，consumer 會比值早一個 cycle 到 EX（−1），拿到錯的值。這就是為什麼不同指令要在不同時間 broadcast。
- 第 4 支 [§12](lecture04_note.md#12-實測) 量到真的 CPU 上相依的 add 一個 cycle 一條（背靠背），所以真的處理器有做到 early broadcast（或同等效果的機制）。

### 16.3 真的 CPU 有幾個功能單元

`ports.c`：**8 條互不相干**的相依鏈，每條一直做同一種運算。夠多條獨立的鏈，就能把某一種功能單元塞滿，看每個 cycle 最多能做幾條：

| 運算 | P-core（4.7 GHz） | P-core 每 cycle 幾條 | E-core | E-core 每 cycle 幾條（以 3.6 GHz 估） |
|---|---|---|---|---|
| `add` | 0.061 ns/條 | ≈ **3.5** | 0.078 ns/條 | ≈ 3.6 |
| `imul` | 0.214 ns/條 | ≈ **1** | 0.194 ns/條 | ≈ 1.4 |
| `load`（L1） | 0.074 ns/條 | ≈ **2.9** | 0.139 ns/條 | ≈ 2 |

- **P-core 只能一個 cycle 做 1 條乘法**：不管 window 裡有幾條 `imul` 是綠色的，select 每個 cycle 只能挑 1 條給乘法器。加法有好幾個 ALU 可以用（量到約 3.5 條，還要扣掉迴圈本身的指令佔用的 ALU），load 大約 3 條。這就是 [§11](#11-很多個功能單元) 說的「每類指令各有一個 select unit，依功能單元的數量選」。
- E-core 的加法差不多，乘法似乎稍多一點、load 少一些。兩種核心各有幾個、哪一類功能單元，Intel 的公開文件這裡沒有，**待補**。

---

## 17. 一頁速查表

| 主題 | 重點 |
|---|---|
| Dispatch | rename → window（照順序） |
| Issue | window → 執行單元（不照順序）；wakeup ＋ select ＋ 讀運算元 |
| Window | circular queue；entry：valid、opcode、兩個（imm 或 tag ＋ ready）、dest tag |
| Bypassing | OOO 裡的 forwarding |
| 綠／紅 | ready／在等 |
| Wakeup | producer broadcast tag；每格兩個比較器；比對到設 ready |
| 只送 tag | 7 bit；而且 early broadcast 時值還沒好 |
| 多條 tag bus | 每個來源每條 bus 一個比較器，再 OR |
| Select | request → grant；樹狀 choice box；grant 沿原路回去 |
| 多個 FU | 每類一個 select unit；同類多個：串起來（慢）、複雜 box、**對稱兩棵樹**（一左一右，永不撞） |
| Policy | random、oldest first、年紀＋影響；load 優先於 store；避免 starvation |
| 背靠背 | broadcast 跟 WB：空 2；跟 EX：空 1；跟 RR（early）：空 0 |
| k cycle 的指令 | 被選中後 k 個 cycle broadcast |
| 實測 | 兩棵樹 65536 種組合 0 次撞；broadcast 時間 2／1／0；P-core 每 cycle 約 3.5 add、1 imul、3 load |

---

## 18. 與後續講次的連結

| 本講觀念 | 在哪裡 |
|---|---|
| Available bit、RAT、circular queue | [第 10 支](lecture10_note.md) |
| In-order 的 forwarding、load-use、structural hazard | [第 4 支](lecture04_note.md) |
| Early broadcast 的麻煩、load-store queue | [第 12 支](lecture12_note.md) |
| Commit、釋放實體暫存器 | [第 13 支](lecture13_note.md) |
| Load 猜錯時的 replay | [第 15 支](lecture15_note.md)（Ch5） |
| Instruction window 的大小與 branch prediction | [第 5 支](lecture05_note.md) |

---

## 附：本講一句話總結

> **Rename 之後的指令照順序 dispatch 進 instruction window（一個 circular queue，每格存 opcode、兩個來源的 immediate 或實體暫存器 tag 加 ready bit、目的 tag），運算元沒好的就等；producer 完成時在 tag bus 上 broadcast 它的 7-bit tag，每格用比較器比對、設 ready bit，兩個都 ready 就算醒了（wakeup），再由樹狀的 select unit 依功能單元的數量和 policy（年紀、影響、load 優先於 store）選出要 issue 的指令（兩個同類單元時用一棵偏左、一棵偏右的對稱樹，永遠不會選到同一條）；為了讓相依的指令能像 in-order pipeline 一樣背靠背執行，producer 要在 register read 時就 early broadcast（所以只能送 tag，因為值還沒算出來），執行要 k 個 cycle 的指令則在被選中後 k 個 cycle 才 broadcast。本機實測：對稱兩棵樹在 65536 種組合裡一次都沒撞，三種 broadcast 時間點分別空 2、1、0 個 cycle，而真的 P-core 每個 cycle 只有一個乘法器能用、加法約 3.5 條、load 約 3 條。**

---

*筆記依據 Smruti R. Sarangi「Advanced Computer Architecture」Chapter 4: The Issue, Execute and Commit Stages - Part 2/4 (Wakeup, Select, and Broadcast) 的 whisper 轉錄稿（本機 `large-v3-turbo`，1:01:40）、Sarangi *Next-Gen Computer Architecture* v3.2 Ch4 §4.2.1–4.2.4（p.108–121）整理。實測為本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4、Python 3.10）結果。*
