# Advanced Computer Architecture 第四十三支影片完整筆記：主記憶體（三）：DDR4 的狀態圖、時序參數與 memory controller（Chapter 10, Part III）

> **課程**：Advanced Computer Architecture — Chapter 10: Main Memory (DDR4 timing and the memory controller) -- Part III（播放清單第 43 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：**whisper 轉錄**（本機 `large-v3-turbo`；YouTube 的字幕是亂翻的機器翻譯，例如把 command 翻成「team」、refresh 翻成「update」）（1:07:58）＋ 課本 *Next-Gen Computer Architecture* v3.2 **Ch10** §10.3–10.4（p.562–577）
> **章號**：影片 Chapter 10 = 課本 Ch10。
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄
> - `【實測】`：在本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、Python 3.10）實際執行的結果
> - `⚠️`：逐字稿缺字、或影片／課本／實測說法不一致之處
> - `[mm:ss]`：影片時間點

⚠️ whisper 把「write」寫成「right」，筆記直接改正。

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. 一個 DRAM 操作的生命週期](#1-一個-dram-操作的生命週期)
- [2. DDR4 的狀態圖](#2-ddr4-的狀態圖)
- [3. Memory controller 和 DRAM 的分工](#3-memory-controller-和-dram-的分工)
- [4. ACT 與 PRE 的時序](#4-act-與-pre-的時序)
- [5. 限制耗電的時序：tRRD、tFAW](#5-限制耗電的時序trrdtfaw)
- [6. 讀：preamble、RL = AL + CL + PL](#6-讀preamblerl--al--cl--pl)
- [7. 寫：WL = AL + PL + CWL](#7-寫wl--al--pl--cwl)
- [8. 讀寫之間的時序](#8-讀寫之間的時序)
- [9. Refresh](#9-refresh)
- [10. 一個例子：DDR-1600 的參數](#10-一個例子ddr-1600-的參數)
- [11. Memory controller 的四個部分](#11-memory-controller-的四個部分)
- [12. Transaction scheduling 與 open／close page](#12-transaction-scheduling-與-openclose-page)
- [13. Address mapping](#13-address-mapping)
- [14. Command scheduling](#14-command-scheduling)
- [15. 【原始碼】Linux 怎麼從實體位址算出 channel](#15-原始碼linux-怎麼從實體位址算出-channel)
- [16. 【實測】](#16-實測)
- [17. 一頁速查表](#17-一頁速查表)
- [18. 與後續講次的連結](#18-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

```
DDR4 狀態圖：Idle ─ACT→ Bank active ─READ/WRITE→ Reading/Writing ─READA/WRITEA/PRE→ Precharging → Idle
  另外有 REF（refresh）、self refresh（SRE/SRX）、power down（PDE/PDX）
時序參數：命令 A 到命令 B 至少要隔多久
  ACT→PRE: tRAS；ACT→內部讀寫: tRCD；ACT→ACT: tRC = tRAS + tRP；PRE→ACT: tRP
  耗電：tRRD_S（不同 bank group）≤ tRRD_L（同一個）；tFAW：任何一段 tFAW 裡最多 4 次 ACT
  讀：RL = AL + CL + PL；寫：WL = AL + PL + CWL；讀→寫：tRTW = RL + BL/2 + 1 + tWPRE − WL
memory controller：transaction scheduler → address mapper → command generator → command scheduler（bank queue）
  先讀後寫；write cache；open page vs close page
  address mapping：open page r:k:b:g:l:c、close page r:l:k:b:g:c（右邊是最低位）
  command scheduling：bank round robin、rank round robin、greedy
```

---

## 1. 一個 DRAM 操作的生命週期

[00:21]「記憶體系統的第三講：DRAM 的命令和時序」，專門講 **DDR4**。

[00:44] 一個 DRAM 操作：

1. 送出命令到 DRAM，在那裡解碼。
2. 照命令讀或寫 DRAM 陣列。
3. [01:04] 在 DRAM 裡面搬資料。
4. 把結果送回 bus、送回 pipeline。

[01:25] 起因通常是一個 read miss：一路找過 cache 階層都沒有，資料回來後還要填回各層 cache。

[01:44] 例子的設定：一個 DRAM 元件有 **16 個 bank**；DDR4 把 bank 分成 **bank group**，一組 4 或 8 個。[02:07] 所有延遲都以**bus cycle**（memory controller 到 DRAM 那條 bus）為單位。

【課本補充】課本 p.562–563：只有三種基本操作（讀、寫、refresh），但為了從複雜的 DRAM 系統榨出最大的吞吐量，需要一大堆命令。完整的 DDR4 規格請看 **JEDEC** 標準，課本只講一小部分，而且做了很多簡化；狀態圖和參數「著作權屬於 JEDEC，經 JEDEC 同意轉載」。

---

## 2. DDR4 的狀態圖

[02:33]（課本圖 10.23）

```
Power on → Reset → Initialise → Idle
Idle ⇄ Power down（PDE／PDX）
Idle ⇄ Self refreshing（SRE／SRX）
Idle → Refreshing（REF）→ 自動回 Idle
Idle ─ACT→ Activating → 自動 → Bank active
Bank active ⇄ Active power down（PDE／PDX）
Bank active ─READ→ Reading（一直 READ 就一直讀）→ 回 Bank active
Bank active ─WRITE→ Writing → 回 Bank active；Reading ⇄ Writing
Bank active ─READA→ Reading-Pre ─自動→ Precharging
Bank active ─WRITEA→ Writing-Pre ─自動→ Precharging
任何狀態 ─PRE／PREA→ Precharging ─自動→ Idle
```

- [03:00] 不用的時候可以送命令進 **power down**，之後再送命令出來。
- [03:18] **self refresh**：沒有命令進來時 DRAM 自己 refresh 每個 row（讀出來、送進 sense amp、恢復電壓）。
- [04:14] 實線箭頭 = 收到命令才轉換；虛線 = 自動轉換。
- [04:42]「最重要的命令」：**ACT**（activate）：打開 row、充 bit line、讓值進 sense amp。讀寫都要在這之後。
- [05:17] bank 會依 bank 位址被啟動；DDR 協定裡 bank **可以分別**接受命令，也可以單獨 power down（「不常用，但有」）。
- [05:36] 一直送 **READ** 就一直讀：row 的內容都在 sense amp 裡，只要把 column mux 指到別的地方。
- [07:04] 想**關掉** row（把 bit line 重新 precharge、關掉 word line transistor）→ **READA**（讀完就 precharge）：sense amp 回到 Vdd/2，內容就沒了 → Precharging → Idle。
- [08:36] **寫**：寫進 sense amp，因為整個 row 是打開的，值也傳到對應的 cell。
- [10:09] 在 Bank active 也可以直接送 **PRE**（或 PREA：全部 bank）。

[10:32]「解釋過之後這張狀態圖很簡單，一開始看起來很嚇人。」重點：**打開 row → 等讀寫、一直服務 → 收到 PRE 或帶 A 的讀寫 → precharge → Idle**。

[11:55] 差別：

| 命令 | 意思 |
|---|---|
| READ／WRITE | 讀（寫）完 row **保持開著** |
| READA／WRITEA | 讀（寫）**一次**就關掉 row |

[10:52] refresh 是**強制的**：

⚠️ [10:54] 老師說「通常每 50 到 100 **微秒** refresh 一個 row」，又說「最好看你的 DRAM 規格」。課本 p.573：一個 row 能保存資料 **32–64 ms**；8192 個 row、64 ms 的話，每 **7.8 µs** 送一次 refresh 命令。老師的數字跟課本的兩個數字都對不上。

【課本補充】課本 p.564：外部 refresh 是 memory controller 送 **REF**；self refresh 是 CPU 關機時 DRAM 用內建的計時器產生 SRE、SRX。memory controller 也保存每個 DRAM 元件狀態的**副本**、追蹤它的轉換，才能在正確的時刻送正確的命令。

---

## 3. Memory controller 和 DRAM 的分工

[12:22] DRAM 有它自己的狀態機（「絕對不笨」），但計算能力很小：[12:40] 像一個有限狀態機，**除了定期 refresh 之外不會自己產生命令**。

[13:01] memory controller 的工作：維護一份跟 DRAM 同步的狀態機；它清楚知道 DRAM 的所有細節。

- DRAM 可以是一般的 DIMM 或 **RDIMM**（[第 42 支 §10](lecture42_note.md#10-registered-dimm)）；FB-DIMM 的協定不一樣，已經不流行。
- [13:24] RDIMM 跟 DIMM 的接腳、插槽相容，memory controller 不用知道它是 RDIMM。register 稍微增加延遲、大幅減少電容負載；時序可能會變。
- [13:47] **初始化 DIMM 時，memory controller 直接從模組上讀出時序表**：每個 DIMM 的表現可能稍微不同，controller 要精確知道。之後 controller 自己維護整個狀態機，**所有**跟 DRAM 的溝通都由它發起。

[14:38] 讀寫看起來是 atomic 的，但在 DRAM 裡**不是**：會產生**內部**的讀寫命令。[14:57] 每一對相繼的命令之間有**最小延遲**：controller 送出第一個命令後，要等至少那麼久才能送第二個。

---

## 4. ACT 與 PRE 的時序

（課本表 10.2、10.3；第一欄是一對相繼的事件，第二欄是最小間隔）

| 一對命令 | 最小間隔 | 說明 |
|---|---|---|
| ACT → PRE | **tRAS** | [15:38] activate 要時間，activate 之後要關掉也至少要隔 tRAS（名字來自 row address strobe） |
| ACT → 內部讀／寫 | **tRCD** | [15:56] row to column delay |
| ACT → ACT（另一個 row）| **tRC** | [16:14] row cycle time |
| ACT → REF | **tRC** | [16:40] refresh 就像 activate 一個 row 再去 activate 另一個 |
| PRE → ACT | **tRP** | [20:44] precharge 要等電壓穩定才能 activate |

```
tRC > tRAS > tRCD
tRC = tRAS + tRP          （act → act = act → pre ＋ pre → act）
```

- [17:19] tRC 最大：要再加上 precharge 的時間。
- [17:40] tRAS > tRCD：資料已經在 sense amp 裡，內部讀寫只是存取它們，比較快。
- [18:24] 讀的時候：activate → 讀資料（tRCD）→ 恢復資料：讀是破壞性的，要 **restore** 的時間（[第 41 支 §8](lecture41_note.md#8-讀寫refresh-的步驟)）。[19:07] 這些加起來就是 tRAS。
- [19:45] ACT 的流程：解碼命令 → 位址送進 row decoder → 確定已經 precharge 到位 → activate row → 讓 sense amp 跟 cell 同步、穩定 → column 解碼。

【課本補充】課本 p.564：一個 row（page）可以有 1024 或 2048 bit，activate 會把全部搬進 sense amp → **很貴**的操作。

---

## 5. 限制耗電的時序：tRRD、tFAW

[22:22]「DRAM 本來就是很脆弱的元件。」activate 非常**耗電**（要讀一整個 row 的 512–2048 個 cell、存進 sense amp、還要 restore）→ 不能太快連續 activate。（課本表 10.4）

| 一對命令 | 最小間隔 |
|---|---|
| ACT → ACT，**不同** bank group | **tRRD_S**（short） |
| ACT → ACT，**同一個** bank group 的不同 bank | **tRRD_L**（long） |
| 任何一段長 **tFAW** 的時間裡 | 最多 **4 次** ACT（four-bank activation window） |

```
tRRD_L ≥ tRRD_S        tFAW ≥ 4 × tRRD_S
```

- [23:53] 為什麼同一個 bank group 要等比較久：同一組太頻繁 activate → 單位時間的活動多 → **溫度**高 → 漏電功率高。換到別組，每組的 activate 就少一點。
- [25:02] tFAW：超過 4 次會造成功率的問題。
- [26:23] 更正：tFAW 是在**同一個 rank** 裡，不管哪個 bank group，都只能 4 次。
- [26:47] 實際上 tFAW 比 4 × tRRD_S 大很多，看功率和溫度的限制。

【課本補充】課本 p.566：例：tFAW 是 50 cycle 的話，任何一段 50 cycle 的時間裡都不會超過 4 次 activate → 限制整個元件的功耗。「same／different」都是相對於**上一次**存取說的。

---

## 6. 讀：preamble、RL = AL + CL + PL

[27:28]（課本圖 10.24）命令各佔一個 beat：ACT → NOP → READ；address bus 同時送 row、再送 column。

[27:56] **Preamble**：資料開始前的幾個 cycle，讓接收端把時鐘跟送端同步；[28:51] 這段時間 DQS 被啟動（變成 0、1 的樣式）。所有資料都跟 DQS 同步，DQS 又跟時鐘同步。

[29:15] 資料以兩倍速送（DDR 的 D），prefetch 通常是 8（800 MHz bus ÷ 200 MHz 內部 × 2，[第 42 支 §8](lecture42_note.md#8-為什麼要-prefetch內部時鐘比-bus-慢)）→ 送 8 次。[29:46] 最後一筆送完後還要等一下（**postamble**）讓資料到達、讓接收端鎖好。

（課本表 10.5、10.6）

| 一對事件 | 間隔 |
|---|---|
| READ → 第一個 bit 上 bus | **RL**（read latency） |
| READ → 內部讀（IntREAD） | **AL** |
| IntREAD → 第一個 bit 上 bus（不含 parity 檢查） | **CL**（CAS latency） |
| 檢查 parity 的時間 | **PL** |
| IntREAD → PRE | **tRTP** |
| read preamble／postamble | **tRPRE**／**tRPST** |

```
RL = AL + CL + PL
```

- [31:45] 資料有 parity 保護：64 bit 可能多 4 或 8 bit 的 parity → 要時間檢查。
- [32:55] IntREAD → PRE：整個傳輸要完成、資料 bit 要穩定被讀走，才能 precharge、關 row。

[36:05]「**AL** 是什麼？這是我們一直守著的祕密。」**additive latency**：命令從 controller 到達 DRAM、再產生內部的讀寫事件的那段，「名字沒什麼特別的意思」。

[33:46] 連續讀同一個 row 的不同 column（課本表 10.7）：

| | 間隔 |
|---|---|
| 同一個 bank group | **tCCD_L** |
| 不同 bank group | **tCCD_S** |

tCCD_L ≥ tCCD_S，理由一樣：功率密度、溫度。[34:55] 所以 controller 想把讀請求排到**不同的 bank group**、不同 rank 或不同模組。

【課本補充】課本 p.566–567：read preamble 通常 1–2 個 cycle；調整讀的時鐘叫 **read leveling**。連續一串讀寫時有時不需要 preamble、postamble。DDR4 允許 ACT 之後**馬上**送 READ，但內部讀要等 tRCD：READ 在 DRAM 裡先**暫存**著，等的這段就是 AL。READ 到 PRE 的最小間隔是 **AL + tRTP**。tCCD_L 比較大的另一個理由：目前在用的 bank group 有很多資源被這次傳輸佔用，換一組資源比較多。

---

## 7. 寫：WL = AL + PL + CWL

[35:15]（課本表 10.8、10.9）跟讀很像：

| 一對事件 | 間隔 |
|---|---|
| WRITE → 第一個 bit 上 bus | **WL**（write latency） |
| WRITE → 內部寫（IntWRITE） | **AL** |
| 算 parity 的時間 | **PL** |
| IntWRITE → 第一個 bit 上 bus | **CWL**（CAS write latency） |
| write preamble／postamble | **tWPRE**／**tWPST** |

```
WL = AL + PL + CWL
```

⚠️ [35:38] 老師口頭先說成「WL = AL + PL + CL」，[36:26]「抱歉，應該是 CWL」。

【課本補充】課本 p.568：調整寫的時鐘（偵測資料、strobe、DRAM 內部時鐘之間的 skew）叫 **write leveling**。連續寫也用 tCCD_S、tCCD_L。

---

## 8. 讀寫之間的時序

### 8.1 讀 → 寫：tRTW

[36:57]「讀完能不能馬上寫？不行，這有點難解釋，我盡量慢慢講。」（課本圖 10.25；假設 PL = 0）

```
Command: READ  NOP  WRITE
         |←RL=4→|←── BL=8（4 個 cycle）──→| 1 | tWPRE |
                      |←────── WL = 8 ──────→| 第一個寫的 bit
```

- [37:56] READ 之後 RL = 4 個 cycle，第一個 bit 上 bus；burst length 8（8 個 beat = 4 個 cycle）。
- [38:58] 讀完要等一下：讀是破壞性的（bit line 的電被放掉了），要讓系統穩定；再加一個 cycle、再加寫的 preamble。
- [39:39] 寫的時序不一樣：讀的時候 bit 在時鐘邊緣開始傳；寫的時候資料要先穩定，時鐘再轉換：DRAM 是比較弱的電路，要求 bus 上的資料先穩定（[第 42 支 §6](lecture42_note.md#6-ddr)）。

```
tRTW = RL + BL/2 + 1 + tWPRE − WL
```

- [41:06]「BL/2」：BL 是 8 個 beat，換成 bus cycle 要除以 2（DDR）；「+1」是那一個多出來的 cycle。最後減掉 WL：寫命令要提早 WL 送出。

### 8.2 其他

（課本表 10.10）

| 一對事件 | 間隔 |
|---|---|
| READ → WRITE | **tRTW** |
| WRITE → READ（同一個／不同 bank group） | **tWTR_L**／**tWTR_S** |
| burst length | **tBL** |
| WRITE → PRE | **tWR**（write recovery time） |

- [42:38] 寫要額外的工作（設定 sense amp 的狀態等）→ 寫 → 讀要好幾個 cycle。
- [43:02] **tWR**：[43:27] 寫用**很強**的 write driver 把值強制改掉（0 變 1 或 1 變 0），sense amp 和 cell 之間的回授要一段時間才穩定，穩定了才能 precharge。

【課本補充】課本 p.569–570：tRTW 裡的「+1」：大部分 DDR4 元件要多一個 cycle **把 bus 的方向反過來**。read postamble 被併進讀的 burst 裡了，不然還要加上它。tWTR 和 tWR 都是從寫（含 postamble）**結束**算起。burst chop 模式下 tBL 是 4。

---

## 9. Refresh

[43:58]（課本表 10.11）

| | |
|---|---|
| **tREFI** | 最大 refresh 間隔：每 tREFI 個 cycle 要做一次 refresh |
| **tRFC** | refresh cycle time：整個 refresh 要花的時間 |

[44:16] 兩種模式：controller 手動送 **REF**；或 DRAM 自己產生（self refresh）。

【課本補充】課本 p.570、p.573–574：送 REF 之前所有 bank 都要 precharge、閒置至少 tRP。每個 DRAM 元件有內部的 refresh controller，產生要 refresh 的位址清單；refresh 完所有 bank 回到 idle。流量很大時可以**延後** refresh（有上限）：DDR4 最多可以延後 **8 個** refresh 間隔，先送重要的讀寫，之後再趕快補完。tRFC 期間那個 bank 不能用。

（第 41 支量到一個像 refresh 的週期性延遲，見 [第 41 支 §11.2](lecture41_note.md#112-每隔約-18-µs-就有一次特別慢)。）

---

## 10. 一個例子：DDR-1600 的參數

[45:04]（課本表 10.12）DDR-1600：bus 800 MHz、每秒 1600 百萬次傳輸，一個 cycle = **1.25 ns**。單位：cycle。

| 參數 | 意思 | 值 |
|---|---|---|
| tRC | row cycle time | **38** |
| tRAS | ACT → PRE | **28** |
| tRP | PRE → ACT | **10** |
| tRCD | ACT → 內部讀寫 | **10** |
| CL | CAS latency | 10 |
| CWL | CAS write latency | 9 |
| tCCD_L／tCCD_S | column to column（同／不同 bank group） | 5／4 |
| tRRD_L／tRRD_S | row to row（同／不同 bank group，1 Kb page） | 5／4 |
| tFAW | four-bank activation window | **20** |
| tWTR_L／tWTR_S | write to read（同／不同 bank group） | 6／2 |
| tRTP | IntREAD → PRE | 6 |
| tWR | write recovery | **12** |

- [45:49] tRC = 38 = tRAS 28 ＋ tRP 10：「從 activate 到 precharge 是主要的苦工」。
- [46:27] column 之間的延遲（4、5）比 row cycle（38）小很多：**打開的 row 很便宜，換 row 很貴**。
- [46:51]「讓你們不要像我一樣搞混」：tRC 和 tRRD_L 都是「兩次 ACT 之間」，為什麼一個 38、一個 5？tRC 是**同一個 bank** 換 row，要經過 precharge；tRRD 是 activate **別的 bank**，precharge 不在 critical path 上。
- [47:58] tFAW = 20 = 4 × tRRD_L；至少要 16（= 4 × tRRD_S），實際上可以大很多，看功率和溫度。
- [48:31] tWR = 12：「不成比例地大」→ 寫在記憶體模組裡不算快，主要就是因為它。

（本機的 row conflict 跟這張表對得上嗎？見 [§16.2](#162-本機量到的-row-conflict-跟表-1012-對得上嗎)。）

---

## 11. Memory controller 的四個部分

[48:53]（課本圖 10.26）

```
記憶體請求（從 LLC）→ ① transaction scheduler（旁邊有 write cache）
                      → ② address mapper → ③ command generator
                      → ④ command scheduler（每個 bank 一個 queue）→ DIMM
DIMM → read unit → 回應給請求者（通常是 LLC）
```

| 部分 | 工作 |
|---|---|
| **Transaction scheduler** | [49:35] 每個請求變成一個 transaction；決定優先順序 |
| **Write cache** | 一個小的 cache 吸收寫：寫可以送去記憶體，也存在這裡，讀可以從這裡拿 |
| **Address mapper** | [49:53] 把 transaction 對應到 channel、rank、bank group、bank…… |
| **Command generator** | [50:17] 遵守所有時序規則，產生命令 |
| **Command scheduler** | 每個 bank 一個 **bank queue**，把命令放進去再送出 |

[50:33] 這假設 bank 之間不需要同步；實際上同一個 rank 的 bank 常常是同步的（這就是 rank 的主要好處），所以 bank queue 比圖上複雜。

【課本補充】課本 p.571–572：通常**每個 channel 一個** memory controller。transaction scheduler 也可以決定「很需要的 refresh」優先。DDR4 每個命令帶三個資訊：**chip select**（選 rank）、**bank group id**、**bank id**。例：一個 rank 8 個 DRAM 元件，每個元件 4 個 bank、每個 bank 8 個 array → 每個 bank 一次 8 bit；送 bank id 2 → 8 個元件的 2 號 bank 都被選到 → 一次 64 bit。等讀的時候可以去 activate 別的 bank。DDR4 也允許對 rank 裡的個別元件送命令，但主要是設定電氣參數。Important Point 19：組成 rank 是為了增加平行度；每個元件一次只有一個 bank 能處理 bus 上的命令。

---

## 12. Transaction scheduling 與 open／close page

[50:56] 不同操作優先順序不同：

- [51:22] **讀**通常在 critical path 上 → 優先，連續地做；讀也沒有長的恢復時間（不像寫有 tWR）。
- [51:41] **寫**不急 → 等 DRAM 比較閒的時候再做。
- 在 memory consistency 的規則下可以重排讀（[第 38 支](lecture38_note.md)）；讀可以從 write cache 拿資料（有沒有用看局部性）。

[52:00] 兩種 **access policy**：

| | **Open page** | **Close page** |
|---|---|---|
| 做法 | [52:21] row 打開後**保持**開著，sense amp 跟它同步；一直存取不同的 column。這排 sense amp 叫 **row buffer**（page buffer） | [53:29] 存取一次就關（用 READA、WRITEA）：[54:01] 不用另外送關 row 的命令 |
| 適合 | [53:01] 有時間、空間局部性 → row buffer hit 率高 | 預期存取是**隨機**的，沒有局部性 |
| 換 row | [53:18] 請求到別的 row 才 precharge、關掉 | 每次都重新 activate |

[54:01] 也可以 **hybrid**：根據別的資訊決定開或關，「但那需要程式層級的提示等等」。

【課本補充】課本 p.572–573：在 DDR4 裡連續讀比「先寫再讀」快很多（寫完要恢復）。**write cache** 也解決重排讀寫的正確性問題：讀先查 write cache，有就直接回傳。open page 時大部分 controller 會**重排**請求，讓連續的請求打到同一個打開的 page。課本說「現在大部分 memory controller 用 hybrid」：觀察存取序列，預測某個 row 不太會再用就馬上關掉。

---

## 13. Address mapping

[54:19]「最有趣的部分。」一個位址要對應到：

| 符號 | 意思 |
|---|---|
| c | channel id |
| k | rank id |
| g | bank group id |
| b | bank id |
| r | row id |
| l | row 裡第幾個 64 B 的 block |

[55:17] 一個 bank 有很多 array（例如 64 個），每個出 1 bit → 一次 64 bit；prefetch 8 → 64 B。[56:10] 一次讀的單位通常是 64 B。

[56:53] DRAM 位址的 bit 數 N = c + k + g + b + r + l。[57:12]「最重要的問題：它們的**順序**。」寫法 `r:k:b:g:l:c`：**最右邊用最低的 bit**。

### 13.1 Open page

[57:52] 原則：**低位的 bit 變化最頻繁** → 把「切換成本最低」的東西放在最低位。最常見的樣式是**存取下一個 block**（指令、陣列、stack）。

| 順序（低 → 高） | 理由 |
|---|---|
| **c**（channel） | [58:47] 切換 channel 最快：controller 在處理器的時鐘頻率下就能決定，不牽涉 DRAM；不同 channel 可以平行讀 |
| **l**（row 裡的 block） | [59:07] row 是打開的，換一組 column 很容易 |
| **g**（bank group） | [59:24] 換 bank group 比在同一組裡換 bank 好（tCCD_S、tRRD_S 比較小） |
| **b**（bank） | [60:10] 剩下 b、k、r 都難：換 row 要重新 activate，換 rank 要重新同步時鐘；換 bank「稍微久一點，但不多」 |
| **k**（rank） | 要重新同步，但可以接受 |
| **r**（row） | [60:44] 最後才換：activate 的延遲很高 |

```
open page：r:k:b:g:l:c
```

### 13.2 Close page

[61:11]

| 順序（低 → 高） | 理由 |
|---|---|
| **c** | 永遠最快 |
| **g**、**b** | [61:34] row 不會保持開著，換 column 沒好處 → 至少試著利用 **bank 層級的平行度** |
| **k** | [61:51] 換 rank 稍微久一點，但不太多 |
| **l** | [62:11] 剛打開過的 row，資料可能還留在 controller 的 cache 裡 → 「保險一點」用同一個 row 的別的 column |
| **r** | 最後：activate 新 row 很久 |

```
close page：r:l:k:b:g:c
```

[62:46]「兩種都把 row 放在最後，因為 activate 和 precharge 的時間很長。」

[63:09] 如果 block 比 N bit 能表示的還多：要某種虛擬化或重新導向，「但很少見」。

【課本補充】課本 p.574–575：controller 只取位址的 N 個 bit（通常是 block 位址的最低位），其餘丟掉 → 虛擬記憶體系統要保證這 N 個 bit 相同的兩個 block 不會同時在 DRAM 裡 → 就**不需要 tag array**。「跟 cache 不同，主記憶體**沒有 miss**」（page 在就一定找得到）。換 rank 時要重新同步 processor-memory bus 的時序，還要 read leveling、write leveling。

（本機用模擬器把這兩種 mapping 都跑了一遍，見 [§16.1](#161-模擬-address-mapping-與-openclose-page)；Linux 怎麼算 channel，見 [§15](#15-原始碼linux-怎麼從實體位址算出-channel)。）

---

## 14. Command scheduling

[63:49] 照時序參數產生命令，放進 bank queue。假設一個 bank 由很多 array 組成，夠強，一個 64 B 的請求可以**全部由一個 bank** 提供。

| 演算法 | 做法 | 假設 |
|---|---|---|
| **Bank round-robin**（BRR） | [64:04] 一個 rank 一個 rank 處理；在 rank 裡輪流看每個 bank 的 queue。[64:29] 同一個 bank 裡「activate、讀、activate、讀」可以合併：一次 activate 盡量多讀幾個（**batch**，open page 時） | 一個 rank 裡的請求都一樣重要，碰到這個 rank 就把它的請求都做完 |
| **Rank round-robin**（RRR） | [65:05] 先看每個 rank 的**同一個** bank id，再換下一個 bank id | 資料和它的重要性平均分布在各 rank |
| **Greedy** | [66:14] 看所有命令：哪一個**現在就能送**（不管哪個 bank、rank）就送；用 priority queue | [66:53] 系統負載低時很好；但 priority queue 在硬體裡不好做 |

[67:11]「傳統 DRAM、時序、記憶體存取怎麼排程，討論完了。」下一講：新的（大多是非揮發的）記憶體技術，和 roofline model。

【課本補充】課本 p.576：每個 bank 可以有三個子 queue：讀、寫、refresh。BRR 也可以讀優先於寫；DDR 允許 ACT 之後馬上送 READ，可以在這時一起送，不然要等很久才會再輪到這個 bank queue。BRR、RRR 做決定時**不看**時序限制（目的是公平），決定之後才去遵守時序；greedy 在中高負載時會有**餓死**和不公平的問題。「實際的演算法同時想縮短等待時間、又不太犧牲公平。」排程演算法和 address mapping 是相關的，設計者會做大量模擬再決定。

---

## 15. 【原始碼】Linux 怎麼從實體位址算出 channel

Linux 的 EDAC（error detection and correction）驅動要把「哪個實體位址出錯」換算成「哪個 channel、哪條 DIMM」，所以裡面有 Intel client 處理器（Alder Lake 等）的 address mapping 解碼。`drivers/edac/igen6_edac.c:468-510`（節錄）：

```c
static int decode_chan_idx(u64 addr, u64 mask, int intlv_bit)
{
	u64 hash_addr = addr & mask, hash = 0;
	u64 intlv = (addr >> intlv_bit) & 1;
	int i;

	for (i = 6; i < 20; i++)
		hash ^= (hash_addr >> i) & 1;

	return (int)hash ^ intlv;
}
...
static void decode_addr(u64 addr, u32 hash, u64 s_size, int l_map,
			int *idx, u64 *sub_addr)
{
	int intlv_bit = CHANNEL_HASH_LSB_MASK_BIT(hash) + 6;

	if (addr > 2 * s_size) {
		*sub_addr = addr - s_size;
		*idx = l_map;
		return;
	}

	if (CHANNEL_HASH_MODE(hash)) {
		*sub_addr = decode_channel_addr(addr, intlv_bit);
		*idx = decode_chan_idx(addr, CHANNEL_HASH_MASK(hash), intlv_bit);
	} else {
		*sub_addr = decode_channel_addr(addr, 6);
		*idx = GET_BITFIELD(addr, 6, 6);
	}
}
```

- **不用 hash** 時：channel = 位址的 **bit 6**：每 64 B（一個 block）換一個 channel → 就是 [§13](#13-address-mapping) 把 **c 放在最低位**（bit 0–5 是 block 裡的 byte）。
- **用 hash** 時：把位址 bit 6–19（被 mask 選中的那些）全部 **XOR** 起來，再 XOR 上「交錯 bit」（`intlv_bit` = 暫存器設定 + 6，不低於 bit 6）→ channel。比單純取一個 bit 更能把各種存取樣式打散到兩個 channel（[§16.1](#161-模擬-address-mapping-與-openclose-page) 會看到單純的 mapping 碰到某些樣式會全部擠在一個 bank）。
- `decode_channel_addr`（`:480-489`）把交錯用的那個 bit 拿掉，上面的 bit 往下補 → 得到 channel 內的位址。
- `addr > 2 * s_size`：兩條 channel 容量不一樣時，超過「小的那條 × 2」的部分不交錯，全部在大的那條（`l_map`）。
- 這個驅動只解到 channel 為止；rank、bank group、bank、row 怎麼排，Linux 裡沒有對應的程式碼，本機的實際 mapping **待補**。

---

## 16. 【實測】

### 16.1 模擬 address mapping 與 open／close page

`experiments/lecture43/memsim.py`：照課本表 10.12（DDR-1600）的時序，模擬一個**只處理讀**的 memory controller：

- 2 個 channel、每個 2 個 rank、4 個 bank group × 4 個 bank、每個 row 128 個 64 B 的 block。
- 每個命令最早能送的時間 = 所有限制（tRCD、tRP、tRAS、CL、tCCD_S/L、tRRD_S/L、tFAW、tRTP、data bus 一次一個 burst、換 rank 多 1 cycle）的最大值。
- 簡化：每個 channel 的命令**照請求順序**送（不重排），所以隨機存取的數字偏低。
- 4000 個請求；兩個 channel 全速的上限 = 2 × 64 B ÷（4 cycle × 1.25 ns）= **25.6 GB/s**。

結果（GB/s ／ row buffer hit 率）：

| 存取樣式 | r:k:b:g:l:c open（課本的 open page） | r:k:b:g:l:c close | r:k:b:l:g:c open（g 往下移） | r:l:k:b:g:c open | r:l:k:b:g:c close（課本的 close page） | c:k:g:b:r:l open（channel 放最高位） |
|---|---|---|---|---|---|---|
| **循序** | 20.26／99% | 2.71／0% | **25.21／99%** | 24.50／98% | 9.30／0% | 9.89／99% |
| **隨機** | 4.56／0% | 8.08／0% | 4.57／0% | 4.56／0% | **8.16／0%** | 4.55／0% |
| **兩條循序 stream 交錯** | 2.63／0% | 2.70／0% | 3.36／0% | 3.38／0% | 4.18／0% | **9.97／99%** |
| **每次跳 8 KB** | 3.82／50% | 2.09／0% | 7.20／88% | **9.58／98%** | 1.35／0% | 1.31／0% |

- **循序存取用 open page 才對**（[52:21]）：課本的 open page mapping 20.26 GB/s、99% hit；同一個 mapping 改用 close page 只剩 2.71（每次都重新 activate）。
- **隨機存取用 close page 比較好**（[53:29]）：8.08–8.16 vs 4.56：反正不會命中，用 READA 順便 precharge 省掉之後的 PRE。
- **channel 放在最低位很重要**（[58:47]）：channel 放最高位（最後一欄）時，循序存取全擠在一個 channel，只有 9.89 GB/s。
- ⚠️ 課本的 open page mapping `r:k:b:g:l:c` 在**循序**存取時只有 20.26 GB/s，不是上限 25.6：l 直接在 c 上面，連續的 block 都在**同一個 bank group** → 每個 READ 要隔 **tCCD_L = 5** cycle，而 data burst 只要 4 cycle（2 × 64 B ÷ 5 × 1.25 ns = 20.48 GB/s，剛好就是這個上限）。把 g 移到 l 下面（`r:k:b:l:g:c`），連續的 block 輪流到不同的 bank group，只要 tCCD_S = 4 → **25.21 GB/s**、hit 率還是 99%。這是 DDR4 bank group 的時序造成的，課本在挑 mapping 時沒有考慮 tCCD_L 和 tCCD_S 的差別。
- **任何固定的 mapping 都有剋星**：兩條 stream 相隔 2²² 個 block（256 MB）時，課本兩種 mapping 的差落在 row 的 bit 上 → 同一個 bank 的兩個 row 互相把對方關掉（0% hit）；只有恰好把那個 bit 放在 bank 的 mapping 才好。這就是 [§15](#15-原始碼linux-怎麼從實體位址算出-channel) 的 Intel 硬體用 **XOR hash** 打散的理由。

### 16.2 本機量到的 row conflict 跟表 10.12 對得上嗎

[第 41 支 §11.1](lecture41_note.md#111-row-buffer同一個-bank-的不同-row-互相干擾) 量到：同一個 bank 的不同 row 輪流讀，比一般慢 **15–50 ns**。

照表 10.12，row conflict 比 row hit 多出來的是先 PRE、再 ACT：**tRP + tRCD = 10 + 10 = 20 cycle × 1.25 ns = 25 ns**（DDR-1600）。量到的範圍包含這個數字。本機的記憶體不是 DDR-1600（[第 42 支 §11.2](lecture42_note.md#112-本機的記憶體頻寬) 的頻寬比 DDR4 一個 channel 的上限還高），但 tRP、tRCD 這類以**奈秒**計的延遲在各世代之間變化不大（cycle 數會跟著時鐘變多）。本機的實際參數要從 DIMM 讀（要 root），**待補**。

---

## 17. 一頁速查表

| 主題 | 重點 |
|---|---|
| 狀態圖 | Idle → ACT → Bank active → READ／WRITE（保持開）、READA／WRITEA（做完就關）→ PRE → Idle；REF、SRE／SRX、PDE／PDX |
| 分工 | DRAM 是有限狀態機，只會自己 refresh；controller 讀 DIMM 上的時序表、維護狀態機、發所有命令 |
| ACT／PRE | tRAS（ACT→PRE）、tRCD（ACT→內部讀寫）、tRP（PRE→ACT）、tRC = tRAS + tRP；tRC > tRAS > tRCD |
| 耗電 | tRRD_S ≤ tRRD_L；tFAW：同一個 rank 任一段時間最多 4 次 ACT；tFAW ≥ 4 tRRD_S |
| 讀 | RL = AL + CL + PL；preamble、postamble（read leveling）；tCCD_S ≤ tCCD_L |
| 寫 | WL = AL + PL + CWL；write leveling |
| 讀寫交錯 | tRTW = RL + BL/2 + 1 + tWPRE − WL；tWTR_S／L；tWR（寫完要穩定才能 precharge） |
| Refresh | tREFI、tRFC；課本：每 7.8 µs 一次，最多延後 8 次 |
| DDR-1600 | 1.25 ns/cycle；tRC 38、tRAS 28、tRP 10、tRCD 10、CL 10、tFAW 20、tWR 12 |
| Controller | transaction scheduler（讀優先、write cache）→ address mapper → command generator → command scheduler（bank queue） |
| Page policy | open（局部性好，row buffer）、close（隨機，READA）、hybrid |
| Mapping | open：r:k:b:g:l:c；close：r:l:k:b:g:c；channel 永遠最低位、row 永遠最高位 |
| 排程 | bank round-robin（rank 內輪流 bank、batch）、rank round-robin、greedy（低負載好、可能餓死） |
| 原始碼 | Intel client：channel = bit 6，或 bit 6–19 的 XOR hash |
| 實測 | 模擬：循序要 open、隨機要 close；課本的 open mapping 被 tCCD_L 卡在 20.5 GB/s，g 往下移到 25.2；固定 mapping 會被某些樣式打爆 |

---

## 18. 與後續講次的連結

| 本講觀念 | 在哪裡 |
|---|---|
| DRAM cell、row、sense amp、rank、bank | [第 41 支](lecture41_note.md) |
| DDR、prefetch、RDIMM | [第 42 支](lecture42_note.md) |
| 讀寫重排與 memory consistency | [第 38 支](lecture38_note.md) |
| Flash、FeRAM | [第 44 支](lecture44_note.md) |
| Roofline model | [第 45 支](lecture45_note.md) |

---

## 附：本講一句話總結

> **DDR4 的每個 DRAM 元件是一個有限狀態機：ACT 打開 row 進入 bank active，READ／WRITE 讓 row 保持打開，READA／WRITEA 或 PRE 讓它 precharge 回到 idle，再加上 refresh、self refresh 和 power down；每一對相繼的命令之間都有最小間隔，例如 tRAS、tRCD、tRP（tRC = tRAS + tRP），為了限制耗電還有 tRRD 和 tFAW，讀的延遲是 AL + CL + PL、寫是 AL + PL + CWL，讀轉寫要 RL + BL/2 + 1 + tWPRE − WL；memory controller 從 DIMM 讀出這些參數、維護狀態機，經過 transaction scheduler（讀優先、write cache）、address mapper（open page 用 r:k:b:g:l:c、close page 用 r:l:k:b:g:c，channel 在最低位、row 在最高位）、command generator 和 command scheduler（bank／rank round-robin 或 greedy）發出所有命令。本機實測：照 DDR-1600 時序模擬，循序存取要 open page、隨機存取要 close page，但課本的 open page mapping 讓連續的 block 落在同一個 bank group，被 tCCD_L 卡在 20.5 GB/s，把 bank group 往下移就到 25.2 GB/s；Linux 的 Intel 驅動顯示 channel 由位址 bit 6 或 bit 6–19 的 XOR hash 決定，而第 41 支量到的 row conflict 多出 15–50 ns，跟 tRP + tRCD = 25 ns 對得上。**

---

*筆記依據 Smruti R. Sarangi「Advanced Computer Architecture」Chapter 10: Main Memory (DDR4 timing and the memory controller) -- Part III 的 whisper 轉錄稿（本機 `large-v3-turbo`，1:07:58）、Sarangi *Next-Gen Computer Architecture* v3.2 Ch10 §10.3–10.4（p.562–577）、Linux v6.2.12 原始碼整理。實測為本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、Python 3.10）結果。*
