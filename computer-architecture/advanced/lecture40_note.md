# Advanced Computer Architecture 第四十支影片完整筆記：Multicore（九）：transactional memory：ACID、版本管理、Bartok、TL2 與硬體 TM（Chapter 9, Part IX）

> **課程**：Advanced Computer Architecture — Chapter 9: Multicore Systems (Part IX) -- Transactional Memory（播放清單第 40 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：YouTube 自動字幕（1:21:54）＋ 課本 *Next-Gen Computer Architecture* v3.2 **Ch9** §9.7（p.509–527）
> **章號**：影片 Chapter 9 = 課本 Ch9。
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄（本講沒有）
> - `【實測】`：在本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4、Python 3.10）實際執行的結果
> - `⚠️`：逐字稿缺字、或影片／課本／實測說法不一致之處
> - `[mm:ss]`：影片時間點

⚠️ 自動字幕把「lock」寫成「log／knob」、「abort」寫成「about／our boats」、「opacity」寫成「acclimated」、「Bartok」寫成「park rock」，筆記直接改正。

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. 鎖的問題](#1-鎖的問題)
- [2. Transaction 與 atomic block](#2-transaction-與-atomic-block)
- [3. ACID](#3-acid)
- [4. Read set、write set 與衝突](#4-read-setwrite-set-與衝突)
- [5. Commit 與 abort](#5-commit-與-abort)
- [6. Concurrency control](#6-concurrency-control)
- [7. Version management](#7-version-management)
- [8. Conflict detection](#8-conflict-detection)
- [9. 正確性：serializability、strict serializability、opacity](#9-正確性serializabilitystrict-serializabilityopacity)
- [10. Mixed mode：transaction 與一般存取混在一起](#10-mixed-modetransaction-與一般存取混在一起)
- [11. Software TM：Bartok](#11-software-tmbartok)
- [12. Software TM：TL2](#12-software-tmtl2)
- [13. Hardware TM](#13-hardware-tm)
- [14. 第 9 章總結](#14-第-9-章總結)
- [15. 【實測】](#15-實測)
- [16. 一頁速查表](#16-一頁速查表)
- [17. 與後續講次的連結](#17-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

```
一把鎖：沒有 disjoint access parallelism；每個帳戶一把鎖：可能 deadlock → 照順序拿鎖，但要事先知道要哪些鎖
transaction：atomic { ... }，系統自動處理；ACID
衝突：R ∩ W、W ∩ R、W ∩ W 不是空的（R ∩ R 沒關係）→ abort（之後重試，random backoff）或 stall
concurrency control：pessimistic（馬上偵測、解決）vs optimistic（最後才檢查）
version management：eager（直接寫記憶體 ＋ undo log；commit 快）vs lazy（redo log；abort 快）
conflict detection：eager（每次存取）vs lazy（commit 時）
正確性：serializable ⊂ strictly serializable（加即時順序）⊂ opacity（abort 的也要看到一致的狀態）
STM：Bartok（undo log、寫時上鎖、commit 時比 version；沒有 opacity）、TL2（redo log、全域時鐘、commit 時才上鎖；有 opacity）
HTM（LogTM）：每個 L1 line 加 R、W bit；改 coherence 協定，衝突就回 NACK；被趕出的 block 用 M@C 的黏性狀態 ＋ overflow bit
```

---

## 1. 鎖的問題

[00:00]「這一章的第九講。」先看 lock、unlock 的問題：

```c
void updateBalance(int amount, Account account) {
    lock();
    int temp = account.balance;      // load
    temp = temp + amount;            // add
    account.balance = temp;          // store
    unlock();
}
```

[01:53] 只有**一把鎖**：一家有 10000 個客戶的大銀行，客戶的帳戶彼此無關，卻不能平行操作。

- [02:24] 唯一的限制應該是：**同一個帳戶**不能有兩個平行的存取（或者兩個帳戶有關聯時才要一起鎖）。
- [02:50] 不相干的帳戶應該能平行更新：**disjoint access parallelism**。

[03:26] 改成**每個帳戶一把鎖**：

- 記憶體用量變大：10000 個帳戶就有 10000 把鎖。「但這是一個很能擴充的解法，實際上也有人這樣用。」
- [04:06] 新問題：**有關聯的帳戶**。例：從 A 轉 100 盧比到 B，要檢查 A 的餘額夠不夠、B 有沒有被**凍結**（「因為各種法律的原因」）→ 要同時鎖 A 和 B。
- [05:03] 多把鎖 → 可能 **deadlock**。

[05:35] Deadlock：thread A 拿了鎖 A、等鎖 B；thread B 拿了鎖 B、等鎖 A → **循環等待**，誰都動不了。

[06:11] 解法：**規定拿鎖的順序**（例如先拿鎖 1 再拿鎖 2）。

- [06:42] 拿著鎖 1 的人等鎖 2、拿著鎖 2 的人等鎖 3……照順序拿的話，拿了鎖 4 的人永遠不會去等鎖 1 → 不可能循環等待 → 不可能 deadlock。
- [07:13] 假設：程式開始前就知道它會拿**哪些**鎖。很多時候不知道。

⚠️ [06:11] 老師把「照順序拿鎖」叫做 **two-phase locking**；課本 p.510 沒有用這個名字，只說「照順序拿鎖，就像 NoC 裡把 virtual channel 排順序」（[第 29 支](lecture29_note.md)）。

【課本補充】課本 p.510 Definition 87：disjoint access parallelism。課本 p.510 也說：帳戶的位址是**執行時**才算出來的話，要到執行到那裡才知道鎖在哪；可以保守一點禁止這種程式碼，或一開始就拿一個超集合的鎖，但這些都限制了寫程式的自由、降低效能。

（本機實測一把鎖、每個帳戶一把鎖、不照順序拿鎖的 deadlock，見 [§15.3](#153-本機的銀行轉帳)。）

---

## 2. Transaction 與 atomic block

[07:13]「這就是 **transactional memory** 上場的地方。」

⚠️ [07:47] 老師說 transactional memory「大約 2010 年左右提出」；課本沒有寫它的歷史，本機的資料查不到最早是什麼時候，**待補**。

[07:47] 兩大優點：

1. 提供 **disjoint access parallelism**。
2. 自動處理鎖、deadlock：「整個鎖和同步的事情都交給 transactional memory。」

[08:14] 同一個函式，只是把 critical section 包進 **atomic block**：

```c
void updateBalance(int amount, Account account) {
    atomic {
        int temp = account.balance;
        temp = temp + amount;
        account.balance = temp;
    }
}
```

[08:46] TM 子系統「**自動又神奇地**」（auto-magically）做剩下的事：保證這段程式 atomic 地執行，該拿的鎖它拿，程式設計師不用管，也不會 deadlock。atomic block 就是一個 **transaction**。可以用軟體或硬體實作。

【課本補充】課本 p.512 Definition 88：transaction = atomic 執行的一段程式；像 critical section，但允許 disjoint access parallelism。軟體的叫 **STM**，硬體的叫 **HTM**。成功結束叫 **commit**，失敗叫 **abort**。

---

## 3. ACID

[09:15] 跟資料庫一樣的 **ACID** 性質，稍微改過：

| 性質 | 意思 |
|---|---|
| **Atomicity** | [09:50] 整個 transaction 要嘛全部完成，要嘛失敗（**abort**）；外面看起來要嘛全做完、要嘛根本沒開始。[10:17] 跟讀寫的 atomicity 一樣：看起來在某一刻瞬間完成，或完全沒開始 |
| **Consistency** | [10:50] consistent 的狀態 = 符合某組規格的合法狀態。[11:23] transaction 開始前系統是 consistent，結束後也還是。[11:52] 跟 memory consistency 不同：那是「**每一刻**都符合規格」，這裡是「開始和結束時符合」 |
| **Isolation** | [12:20] 執行中看起來沒有別的指令或 transaction 在跑：它不影響別人，別人也不影響它。[12:49] 直接的結果：**沒有人看得到它寫到一半的值**（還沒 commit 的） |
| **Durability** | [13:14] 做完後結果寫進**穩定的儲存**，不會被抹掉 |

【課本補充】課本 p.511–512：isolation 的說法是「平行的 transaction 歷史等價於某個循序的 transaction 歷史」；consistency 舉例：失敗的 transaction 留下一部分更新，狀態就不合法了。

---

## 4. Read set、write set 與衝突

[14:11] **read set** R：transaction 讀的變數；**write set** W：寫的變數。

[14:45] 兩個 transaction i、j **衝突**：

```
Wi ∩ Wj ≠ ∅          （都寫同一個變數）
Wi ∩ Rj ≠ ∅  或  Ri ∩ Wj ≠ ∅     （一個寫、一個讀）
```

- [15:52] 跟 data race 的定義很像（[第 38 支 §10](lecture38_note.md#10-data-race)）：衝突 = 至少一個是寫；只是這裡看的是**集合**，不是單一存取。
- [16:56] **Ri ∩ Rj** 不算衝突：讀的順序不重要。

【課本補充】課本 p.512 式 9.16–9.18；「transaction 之間的衝突，就像指令之間的相依：有相依就不能平行執行。」

---

## 5. Commit 與 abort

[17:23] **Commit**：transaction 完成、沒有任何衝突。

[17:58] **Abort**：因為衝突不能完成。例：兩個 transaction 改同一個帳戶 → write set 重疊 → 兩個不能都 commit。

- [19:07] 一個要被 abort：它做的改變**完全看不到**，好像它從沒開始過；另一個看起來像是單獨在跑。
- [20:00] abort 之後要**重新執行**，可能又被 abort。好辦法：先等一段**隨機**的時間再重試（**random backoff**）。由 TM 系統自動處理：

```c
do {
    // 執行 transaction 的內容
} while (!tx.commit());        // tx：文獻裡通常用 tx 代表一個 transaction
```

---

## 6. Concurrency control

[21:08] **concurrency control**：管理 transaction、偵測和處理衝突的子系統。

[22:12] 衝突的兩種解法：

| 解法 | |
|---|---|
| **abort** 一個 transaction | 上一節 |
| **stall**（讓它等） | 例：tx1、tx2 都要寫 x → 讓 tx2 停在那個寫前面，等 tx1 完成再繼續 → 不會有衝突 |

[23:13] 為什麼 stall 是對的：想像 tx2 剛好在那個寫之前被作業系統暫停（處理器睡著了），tx1 就會開開心心做完，tx2 醒來再寫，根本不會偵測到衝突。stall 只是**刻意**讓這種事發生。

[23:43] 三個要注意的面向：衝突的**發生**（程式本身的性質）、**偵測**、**解決**。

[24:18] 兩種方式：

| 方式 | 做法 | 代價 |
|---|---|---|
| **Pessimistic** | [24:57] 衝突一發生就偵測、解決（stall 或 abort） | [25:34] 每一刻都要偵測，開銷大 |
| **Optimistic** | [24:57] 衝突發生時什麼都不做，晚一點（通常是 transaction 結束、決定 commit 還是 abort 時）才偵測、解決 | 中間的工作可能白做 |

【課本補充】課本 p.513：pessimistic 在純軟體裡很難做，**硬體**就沒什麼開銷；optimistic 很適合 STM（每次存取要做的事最少）。課本也提醒：**不能**在 transaction 結束**之後**才偵測（太晚了）；TA、TB 衝突，但 TB 最後被 abort 了，那 abort TA 就是錯的。

---

## 7. Version management

[26:04] transaction 寫的值放在哪：

| 方式 | 寫的時候 | commit | abort |
|---|---|---|---|
| **Eager** | [26:31] **直接寫進記憶體**；第一次覆寫某個值時，把舊值存進 **undo log** | [27:06] **快**：什麼都不用做，丟掉 undo log | [27:38] 慢：讀 undo log 把狀態修回去 |
| **Lazy** | [28:09] 寫進另一塊區域 **redo log**，不寫記憶體 | 慢：把 redo log 寫回記憶體 | [28:52] **快**：丟掉 redo log 就好 |

[29:20] 一個「樂觀、積極」（直接寫、出事再修），一個「悲觀」（先寫在旁邊，commit 才寫回）。

【課本補充】課本 p.514–516：

- eager 最大的問題是 **isolation**：值直接寫進記憶體，別的 thread 在 commit 前就看得到。軟體的解法：每個變數加 version、寫之前上鎖；硬體的解法：每個 cache line 加 bit 標記被 transaction 讀寫過，不把這些值給別的 thread。
- lazy 的讀也要**先查 redo log**（它像一個 cache），找不到才讀記憶體；它比較彈性，支援很大的 transaction。
- Important Point 16：**flash clearing**：一次把整個 subarray 的某個 bit 清成 0。做法：所有 word line 都打開、一條 bit line 設 0、另一條設 1；或做一個雙 port 的記憶體，一個 port 專門拿來清。同時寫一大片 cell 要很大的電流，所以實際的設計（Miyaji 1991、Rastegar 1994）會**分組、分階段**清。

---

## 8. Conflict detection

[29:51]

| 方式 | 什麼時候檢查 | |
|---|---|---|
| **Eager** | 一存取記憶體位置就檢查 | [29:51] 每次存取都背負檢查，開銷大 |
| **Lazy** | [30:22] **commit** 的時候 | |

【課本補充】課本 p.513：eager 也可以只在第一次存取某個 cache block、或它的 coherence 狀態改變時檢查；lazy 也可以在 compiler 或硬體插入的 **validation point** 檢查。

---

## 9. 正確性：serializability、strict serializability、opacity

[30:22] 跟一般存取有各種 consistency model 一樣，transaction 也有：

| 性質 | 意思 |
|---|---|
| **Serializable** | [30:57] 資料庫也用。可以把所有 transaction 排成一列，而且每個 thread 的 transaction 照程式順序：**transaction 層級的 SC** |
| **Strictly serializable** | [31:32] 再加上**即時順序**：A 在 B 結束**之後**才開始（不同 thread），排出來的順序裡 A 就要在 B 後面；時間上重疊的就隨便 |
| **Opacity** | [34:07] 連 **abort 掉的** transaction 也只能看到一致的狀態（只由 commit 過的 transaction 產生的狀態） |

[31:32] 為什麼需要「即時」：SC 不在乎實際時間。thread 1 設 x = 1，兩天後 thread 2 設 x = 2，SC 也允許排成「兩天後那個先」。

⚠️ [33:08] 老師說 strict serializability「也叫 **linearizability**，但我不想講，那個定義不一樣，嚴格說不是給 transaction 用的」。課本沒有提 linearizability。

### 9.1 為什麼需要 opacity

[35:10]（課本圖 9.60）x、y 初值 0：

```c
// Thread 1                       // Thread 2
atomic {                          atomic {
    t1 = x;                           x = 5;
    t2 = y;                           y = 5;
    while (t1 != t2) {}           }
}
```

- [35:43] commit 過的 transaction 看到的一定是 x = y（都是 0 或都是 5）。
- 但用 **optimistic** concurrency control（最後才偵測）：thread 1 讀到 x = 0 → thread 2 整個做完、commit → thread 1 讀到 y = 5。
- [36:14] 有衝突，thread 1 **注定**要被 abort，但 abort 的決定要到 transaction **結束**才做。
- [36:42] 它卡在 `while (t1 != t2)` → **永遠到不了**結束 → 永遠不會被 abort → 一直佔著 CPU：系統處於錯誤的狀態。

[38:28] 原因：明明知道它會被 abort，卻讓它讀到了**不一致**的狀態（x = 0、y = 5：任何 commit 過的 transaction 組合都產生不了）。**Opacity** 不允許讀到 y = 5。

[39:01]【口頭】「這是有點刻意的例子，但實際上很多這種情況會**不小心**發生。」尤其是 optimistic 的 **STM**，opacity 非常需要。

⚠️ 影片和課本的情境不一樣：

| | 影片 [35:43] | 課本 p.518 |
|---|---|---|
| 假設 | optimistic concurrency control | lazy conflict detection ＋ **eager** version management |
| 交錯 | T1 讀 x = 0 → T2 整個 commit → T1 讀 y = 5 | T2 寫了 x = 5（還沒寫 y，值已經在記憶體裡）→ T1 讀 x = 5、y = 0 |

兩個都會讓 T1 卡住。本機的模擬兩種都跑了，見 [§15.1](#151-bartok-與-tl2-的模擬)。

【課本補充】課本 p.518：opacity 是 strict serializability 的延伸：所有 transaction（commit 的、還在跑的、abort 的）都能排成一列，每個 transaction 看到的都是排在它前面的 commit 過的 transaction 產生的狀態；而且**沒有人**看得到 abort 掉的 transaction 的寫。

---

## 10. Mixed mode：transaction 與一般存取混在一起

[39:29] **mixed mode access**：transaction 的存取和一般的存取混在一起。

| 模型 | 意思 | 問題 |
|---|---|---|
| **Single lock atomicity**（SLA） | [40:00] 假設所有 transaction 都由**一把**假想的全域鎖保護：開始時拿、結束時放；data race 也照同樣的方式定義 | 真的這樣實作的話，沒有 disjoint access parallelism |
| **Disjoint lock atomicity**（DLA） | [40:29] 用很多把鎖（例如每個變數一把），照順序拿 | 要**事先**知道會用哪些鎖 |
| **Transactional sequential consistency**（TSC） | [41:02] 把每個一般指令當成只有一個指令的 transaction；所有 transaction（commit 或 abort）和一般指令能排成一列，**照程式順序**（包括 transaction 裡面的指令）而且 legal | [42:05]「transactional memory 的黃金標準」，實際上很難做到；通常做到的是 **opacity** |

【課本補充】課本 p.518：SLA 下，等價的鎖程式有 data race，TM 程式就有 data race。DLA 後來的模型放寬成「第一次存取某個變數前才拿它的鎖」，就不用事先知道。課本也定義 **TDRF**（transactional data race freedom：禁止一般存取之間、以及一般存取和 transaction 存取之間的 race），並說可以證明 TDRF 程式符合 TSC（跟 [第 38 支 §11.2](lecture38_note.md#112-drf--sc) 的 DRF ⇒ SC 一樣）。

---

## 11. Software TM：Bartok

[42:42] 設計 STM 要選：concurrency control（optimistic／pessimistic）、version management（lazy／eager）、conflict detection（lazy／eager）。

[42:42] 基本的支援：

- 每個 transaction 用到的變數加上 **metadata**：追蹤狀態、偵測衝突；也可以放一把鎖，用來讓別的 transaction 等。
- [43:52] 每個 transaction 維護 read set、write set。
- [44:22] 每個讀、寫換成函式呼叫 `readTX`、`writeTX`：送出讀寫、把位址加進 read／write set、更新 undo／redo log。[44:56] **compiler** 看到 atomic block 就自動轉換。

[45:27] **Bartok STM**（「簡單好懂」）：**eager** version management（undo log）＋ **lazy** conflict detection。[46:05] 每個變數：

```
| value | version | lock |
```

| 操作 | 做法 |
|---|---|
| **讀** | [46:41] 記下變數的 **version**、加進 read set、讀值：「超快」 |
| **寫** | [47:15] **上鎖**（寫的時候上，commit 才放；被別人鎖著就 abort）→ [47:47] 第一次寫這個變數的話，把舊值放進 **undo log**（同一個變數第二次寫就不用）→ 寫新值（**直接寫進記憶體**） |
| **Commit** | [48:24] read set 的每個變數：version 還跟讀的時候一樣嗎？不一樣 = 中間有人寫過 → abort（用 undo log 修回去）。都一樣 → write set 的每個變數：**version 加一**、**放鎖** |

- [49:56] version 加一 = 告訴別人「值改了，你讀的是舊的就要 abort」。
- [49:56] 鎖保證兩個 thread 不會同時寫同一個變數 → 沒有寫寫衝突；讀寫衝突靠 version 抓。

[50:23] 優缺點：

| 優 | 缺 |
|---|---|
| 讀簡單、快 | **沒有 opacity**：[51:33] undo log 的做法，寫的值直接進記憶體，就算最後 abort，別人中間也看得到 |
| 對 transaction 提供很強的語意（serializable） | 鎖從**寫的時候**一直拿到 **commit**，拿很久；寫因此變慢 |
| | [52:02] abort 比 commit 貴（要修狀態） |

【課本補充】課本 p.518–521 圖 9.61–9.64：lock 是一個 1 bit 的欄位；Bartok「對讀用 optimistic concurrency control」。課本 p.521：read set 的變數從第一次讀到 commit 之間沒有人 commit 寫它 → 寫它的 transaction 排在後面 → 讀寫衝突處理正確。

⚠️ 本機照課本**字面**寫的 Bartok 模擬時發現：讀不檢查鎖、version 只在 commit 時加一，所以一個 transaction 可能讀到別人**還沒 commit**、之後又被 **abort（修回去）**的值，而它自己 commit 時 version 沒變、驗證會通過 → commit 了一個從來沒存在過的值。課本 p.521 只說明了「中間有人 **commit** 寫它」的情況。真正的 Bartok（Harris et al. 2006）怎麼處理，本機沒有那篇論文，**待補**。見 [§15.1](#151-bartok-與-tl2-的模擬)。

---

## 12. Software TM：TL2

[52:40] **TL2 STM**：比較精巧，**有 opacity**。改用 **lazy** version management（**redo log**）：[53:10] 寫只有 commit 才看得到，所以不管 commit 或 abort 的 transaction，看到的永遠是 commit 過的 transaction 產生的狀態。

[53:10] 代價：一個**全域時鐘**，所有 transaction 都要存取。好處：變數**只在 commit 時**才上鎖。

[53:46] transaction 開始：**atomically** 把全域時鐘加一、讀出來存在 **tx.rv**（read version）。「像 fetch-and-increment，硬體很容易支援」（[第 37 支](lecture37_note.md)）。每個變數的 metadata：timestamp 和 lock。

| 操作 | 做法 |
|---|---|
| **讀** | [54:49] 在 redo log 裡就回傳 redo log 的值。不在的話：`v1 = obj.timestamp; result = obj.value; v2 = obj.timestamp;` → `v1 ≠ v2`、`v1 > tx.rv`、或被鎖著 → **abort**；否則加進 read set、回傳 |
| **寫** | [58:30]「謝天謝地很簡單」：第一次寫的話加進 redo log，寫進 redo log |
| **Commit** | [58:58] ① write set 的每個變數**上鎖**（失敗就 abort）② `tx.wv = ++globalClock`（atomic）③ read set 的每個變數：timestamp > tx.rv → abort ④ 把 redo log 寫回記憶體 ⑤ write set 的每個變數：timestamp = tx.wv、**放鎖** |

讀的三個檢查：

- [55:20] **timestamp 讀兩次**：讀 timestamp 和讀值中間可能有任意的延遲，兩個可能對不上。[56:32] 前後兩次一樣，才能確定這個值對應這個 timestamp：「並行系統裡非常標準的寫法」（課本叫 **atomic snapshot**）。
- [57:06] `v1 > tx.rv`：這個 transaction 開始**之後**，別的 transaction 寫了這個變數而且 commit 了 → 值過時 → abort。
- 被鎖著：別人正在 commit 它。

[59:29] commit 時的鎖拿得很短（只在 commit 期間）；拿鎖的方式會避開 deadlock，偵測到了就自動處理，「程式設計師不用管」。

[62:09] 為什麼寫寫衝突不是問題：有 redo log，寫只在 commit 成功後才看得見；兩個 write set 重疊的 transaction 先後 commit 沒關係（commit 一個一個排）；同時 commit 的話其中一個拿鎖會失敗 → abort。讀寫衝突才是大事，用 timestamp 解決。

[63:48] 優缺點：

| 優 | 缺 |
|---|---|
| 簡單；**有 opacity** | redo log 讓 commit 比較慢（「沒關係，換到了 opacity」） |
| 鎖拿的時間短 | 讀比較貴（要檢查）；還是要用鎖：「到硬體時我們試著拿掉鎖」 |

[64:17] 細節：

- `tx.wv ≥ tx.rv + 1`：中間沒有別的 transaction commit 就是等於（兩次加一），有的話就更大。
- [64:50] **先**把值寫進永久狀態、**再**更新 timestamp：別人看到新的 timestamp，就確定值已經在那裡了。最後放鎖，讓之後的讀可以通過。

⚠️ [60:00]、[60:36] 老師說投影片上有幾處 `tx` 應該是大寫的 `Tx`，「會改正」。

（本機模擬 TL2 在兩種 opacity 情境下都不會讀到不一致的狀態，見 [§15.1](#151-bartok-與-tl2-的模擬)。）

---

## 13. Hardware TM

### 13.1 為什麼要硬體

[65:20] HTM 的主要優點：**不用鎖**。STM 的問題：

- [65:58] 一般（非 transaction）的存取沒有被換成函式呼叫 → STM **管不到**它們，它們造成的 data race 也抓不到。
- 拿鎖、放鎖、管鎖很貴；在軟體裡維護 undo、redo log 也很麻煩。
- 硬體快很多，但要做硬體：開頭的成本很高。

【課本補充】課本 p.524：HTM 的缺點：硬體更複雜、更耗電；資源有限：undo、redo log 放在硬體裡就限制了 transaction 的大小，太大的話要 abort 或做複雜的機制去要記憶體。

### 13.2 LogTM 的設計

[66:24] 主要根據很流行的 **LogTM** 提案：

- **ISA**：三個新指令：**begin**、**abort**、**commit**。
- **Version management**：硬體通常用 **eager**（undo log）：redo log 要另一塊區域，很麻煩；[66:58] 而 L1、L2 cache 本身就可以當作 undo log 用。
- undo log 放在**虛擬記憶體**裡一塊專用的位址（process 和硬體都存取得到），要 undo 時硬體自動讀出來修回去。
- [66:58] 每個 cache line 加兩個 bit：**R**（這個 line 裡有某個 word 被 transaction 讀過 → 在 read set）、**W**（被寫過 → 在 write set）。

【課本補充】課本 p.524–525：transaction 裡面通常只能有改記憶體和暫存器的指令，大部分實作不允許 system call 或寫 I/O。巢狀的 transaction：begin 把巢狀層數加一，commit、abort 減一。課本的設計「受 LogTM（Moore et al. 2006）啟發，有一些簡化和修改」。

### 13.3 用 coherence 協定偵測衝突

[68:06]「所有 TM 方案的招牌」：**不設計新的協定**，只是稍微改一般的 directory 協定（[第 36 支](lecture36_note.md)）來偵測衝突。

[68:35] 例：core D read miss → directory 發現 core C 有 → 把 read miss 轉給 C → C 讀自己的 cache 發現這個 line 的 W bit 是 1（在 write set 裡）→ **衝突** → 回 **NACK** 給 directory → directory 轉給 D → D 知道有衝突 → D 的 transaction **abort**（硬體或軟體幫忙清理狀態、重試）。

[69:35] commit：整個 read、write set 都在 L1 裡的話很簡單：把所有 R、W bit 清成 0（**flash clear**，[70:09] R、W bit 做成獨立的 subarray 一次清；「書裡有描述，也有很多專利」）。abort：也要 flash clear，還要把 undo log 讀回來寫回去。

### 13.4 被趕出的 block

[71:46] transaction 進行中，在 **M** 狀態的 block 被 core C 趕出（到 L2）：

- directory 的狀態設成 **M@C**（例：core 3 趕出的就是 M@3），core C 的 **overflow bit** 設成 1。
- [72:16] S 狀態的 block：假設是 silent eviction（不通知 directory）。

[72:52] directory 收到某個 M@C 的 block 的請求（例如 core 4 來要）→ 連同 M@C 的狀態一起轉給 core C。

- [73:27] core C 的 L1 裡已經沒有這個 block 了，但看到 directory 說 M@C、自己的 overflow bit 又是 1 → 「directory 不會說謊：我之前用 M 拿著它，後來趕出了，它一定還在我的 write set 裡」→ 推論**衝突** → 回 NACK → 請求的 transaction abort。
- [74:35] 一般的請求（沒有 M@C）轉來：推論它原本是 S 被趕出的；對方只是要讀就沒問題，要寫就是讀寫衝突。

[77:52] 一些細節：

- 被趕出的 block 碰到**非 transaction** 的存取 → 目前的 transaction 必須 abort（非 transaction 的存取沒辦法 rollback）。
- [78:24] abort 時：被趕到下層的 block 可能還帶著被 abort 的 transaction 寫的錯誤值。沒關係：undo log 會直接寫回 **L1**，「處理器只相信 L1 裡的資料」，下層的被當成舊的。
- [79:22] transaction 結束後：每個 line 的 R、W bit、core 的 overflow bit、directory 裡的 M@C 都要清掉。

【課本補充】課本 p.526–527：M@C 叫 **sticky state**（黏性狀態）。commit 時清 M@C 的簡單做法：送一個帶 core id 的訊息給 directory，讓它把所有 M@C 的 entry 清掉；write set 很小的話也可以每個 block 送一個訊息。abort 要花的時間跟 undo log 的大小成正比。

（本機 CPU 沒有 Intel 的 HTM 指令，見 [§15.2](#152-本機有沒有硬體-tm)。）

---

## 14. 第 9 章總結

[79:53]「這個超長的章節講完了。」

- 平行程式兩大派：shared memory 和 message passing。message passing 適合鬆散耦合的多處理器系統；多核心都用 shared memory（[第 32 支](lecture32_note.md)）。
- [80:25] 需要規格：**PLSC** 是所有 memory model 都遵守的，它是 cache coherence 的基礎。
- memory consistency model 有很多種，關鍵是 **write atomicity** 和 **program order**，用 po、rf、fr、ws 描述（[第 35 支](lecture35_note.md)、[第 38 支](lecture38_note.md)）。
- coherence 的公理：**write serialization**（來自 PLSC）、**write propagation**（常識：寫不能遺失）。很多協定實作它們：snoopy、directory（[第 36 支](lecture36_note.md)、[第 37 支](lecture37_note.md)）。
- **data race freedom**、properly synchronized 程式；DRF 的程式執行起來是 SC。
- [81:26] transactional memory：正確性、各種軟硬體協定；用 atomic block 包起來，寫出沒有 data race 的程式碼很容易，「底層系統做剩下的事」。

---

## 15. 【實測】

程式在 `experiments/lecture40/`：`tm.py`（Bartok、TL2 模擬）、`bank.c`（銀行轉帳）。

### 15.1 Bartok 與 TL2 的模擬

`tm.py` 照課本圖 9.61–9.67 寫 Bartok 和 TL2。每個 transaction 是一個 generator，每一步（一個讀或寫）之後讓排程器決定下一步給誰；abort 就從頭重試。

**opacity 的兩種情境**（[§9.1](#91-為什麼需要-opacity)）：

```
影片 [35:43]：T1 讀 x，T2 整個做完並 commit，T1 再讀 y
  Bartok：T1 第一次執行看到 x = 0、y = 5  ← 不一致：會卡在 while；abort 1 次；最後 commit (5, 5)
  TL2   ：T1 第一次執行只看到 x = 0（讀 y 時就 abort 了）；abort 1 次；最後 commit (5, 5)
課本 p.518：T2 寫了 x = 5、還沒寫 y，T1 讀 x 和 y
  Bartok：T1 第一次執行讀到 (5, 0)  ← 不一致：會卡在 while；最後 commit (5, 5)
  TL2   ：T1 第一次執行讀到 (0, 0)；最後 commit (5, 5)
```

- 兩種情境 Bartok 都讓 T1 看到不一致的狀態（模擬裡 T1 不真的進無窮迴圈，看到 t1 ≠ t2 就結束、交給 commit 去 abort），**沒有 opacity**（[50:23]）。
- TL2 不會：影片的情境在讀 y 時就因為 `v1 > tx.rv` 立刻 abort；課本的情境 T2 的寫還在 redo log 裡，T1 讀到的都是舊值。**有 opacity**（[63:48]）。

**轉帳的壓力測試**：4 個帳戶各 100；三筆轉帳（A→B 10、B→C 20、C→A 5）加一個查總額的 transaction，隨機交錯 2000 次：

| | 最後餘額錯 | 查到的總額不是 400 | 平均 abort 次數 |
|---|---|---|---|
| **Bartok**（照課本字面） | **348 次** | **560 次** | 5.59 |
| Bartok＋（abort 時也把 version 加一） | 0 | 0 | 7.96 |
| **TL2** | 0 | 0 | 4.26 |

**最小反例**：T2 第一次寫 A = 999（之後重試改寫 2），T1 讀 A、把讀到的值寫進 B，T2 撞到 T3 的鎖而 abort：

```
Bartok    ：T2 abort 1 次；A = 2、B = 999；B 讀到的值有沒有被 commit 過：沒有！
Bartok＋  ：T2 abort 1 次；A = 2、B = 2；  有
TL2       ：T2 abort 0 次；A = 999、B = 1；有（TL2 commit 時才上鎖，T2 第一次就成功了）
```

- 照課本字面寫的 Bartok：T1 讀到 T2 **還沒 commit** 的 999；T2 abort、用 undo log 把 A 修回去；但 version 只在 commit 時加一，A 的 version 沒變 → T1 commit 時驗證通過 → B = 999，一個**從來沒被 commit 過**的值。這就是 [§11](#11-software-tmbartok) 的 ⚠️。
- 加一條規則「abort 時也把寫過的變數的 version 加一」就沒事了（讀過髒值的人驗證會失敗）；代價是 abort 次數變多（7.96 vs 5.59）。這條規則是我加的，不是課本的。
- TL2 的讀只拿 commit 過的值（redo log 在別人那裡），沒有這個問題。

### 15.2 本機有沒有硬體 TM

```
$ grep -o -w -E "rtm|hle" /proc/cpuinfo | sort | uniq -c
（沒有任何輸出）
```

本機的 i7-13650HX 沒有 Intel TSX 的 `rtm`、`hle` 旗標：**沒有**可用的硬體 TM。GCC 的 `libitm` 提供的方法也只有軟體的（`strings` 看到 `serial`、`serialirr`、`serialirr_onwrite`、`gl_wt`、`ml_wt`）。

### 15.3 本機的銀行轉帳

`bank.c`：N 個帳戶各 1000 元（每個帳戶一個 64 B 的 line，避免 false sharing），T 個 thread（綁在不同的 P-core），每個做 100 萬筆隨機轉帳（1 元）。四種寫法：

| 寫法 | 程式 |
|---|---|
| global | 一把全域的 pthread mutex |
| ordered | 每個帳戶一把鎖，**編號小的先拿**（[06:11]） |
| unordered | 每個帳戶一把鎖，先拿轉出的、再拿轉入的 |
| tm | `__transaction_atomic { acc[a].bal--; acc[b].bal++; }`（`gcc -fgnu-tm`，libitm 預設的方法） |

結果（全部做完的時間 ÷ 總筆數，ns，越小越好；兩次）：

| T | 4 個帳戶：global | ordered | unordered | tm | 4096 個帳戶：global | ordered | unordered | tm |
|---|---|---|---|---|---|---|---|---|
| 1 | 13.9–14.4 | 25.4–25.9 | 24.0–25.1 | 33.5–36.7 | 13.7–14.0 | 25.2–26.4 | 24.1–24.8 | 33.7–35.8 |
| 2 | 78–86 | 78–80 | **DEADLOCK** | 231 | 75–90 | **30–34** | **DEADLOCK** | 135–139 |
| 4 | 88–122 | 97–100 | **DEADLOCK** | 431–435 | 107–111 | **35–39** | 30–39 | 130–137 |
| 6 | 145–152 | 127–130 | **DEADLOCK** | 517–589 | 132–133 | **32–36** | 35 或 **DEADLOCK** | 125–132 |

（看門狗：連續 1 秒都沒有任何一筆完成就印 DEADLOCK。每一次沒有 deadlock 的執行，總額都對。）

- **不照順序拿鎖就會 deadlock**（[05:35]）：只有 4 個帳戶時每次都卡死；4096 個帳戶時也會（兩個 thread 剛好同時做 A→B 和 B→A 的機率低，但跑 100 萬筆總會碰到）。**照順序拿鎖**從來沒卡過（[06:42]）。
- **disjoint access parallelism**：4096 個帳戶時，ordered（每個帳戶一把鎖）6 個 thread 是 32–36 ns，global（一把鎖）是 132 ns，差 4 倍。只有 4 個帳戶時大家都搶同幾把鎖，兩種差不多。
- 但 ordered 多 thread 也沒有比 1 個 thread（25 ns）快：每筆轉帳的兩個帳戶上次多半是被別的 core 碰過的，要把 line 搬過來（[第 36 支](lecture36_note.md#122-本機的-false-sharing)、[第 27 支](lecture27_note.md)）。
- **libitm 的 TM 在這裡比鎖慢**：1 個 thread 就慢 2.4 倍（每個讀寫都變成函式呼叫，[44:22]）；4 個帳戶、6 個 thread 時最慢（517–589 ns，衝突多、abort 多）。4096 個帳戶時它不會隨 thread 變多而變慢（125–137），但也比不上 ordered。

換 libitm 的方法（`ITM_DEFAULT_METHOD`）：

| 方法 | 4 個帳戶 T=1 | T=6 | 4096 個帳戶 T=1 | T=6 |
|---|---|---|---|---|
| serial | 41.4 | 186.5 | 39.5 | 209.6 |
| serialirr | 35.6 | 179.3 | 35.7 | 189.1 |
| gl_wt | 51.0 | 297.6 | 47.2 | 267.2 |
| ml_wt | 70.2 | 531.2 | 67.3 | **126.6** |

- 只有 `ml_wt`（預設的那個，跟上表的 tm 一樣）在帳戶多時比較快（126.6，其他都 189 以上）；但帳戶少、衝突多時它最慢（531.2）。
- 這符合 [§4](#4-read-setwrite-set-與衝突) 的道理：資料不重疊時 TM 能平行，衝突多時 abort 的代價就出來了。
- 這些方法的內部怎麼做，本機只看名字，**待補**。

---

## 16. 一頁速查表

| 主題 | 重點 |
|---|---|
| 一把鎖 | 沒有 disjoint access parallelism |
| 每個帳戶一把鎖 | 可能 deadlock（循環等待）→ 照順序拿鎖；但要事先知道要哪些鎖 |
| Transaction | atomic { }；自動處理鎖和 deadlock；STM、HTM |
| ACID | atomicity、consistency（開始和結束時合法）、isolation（看不到寫到一半的值）、durability |
| 衝突 | R∩W、W∩R、W∩W ≠ ∅；R∩R 沒關係 |
| 解決 | abort（重試、random backoff）或 stall |
| Concurrency control | pessimistic（馬上）vs optimistic（最後） |
| Version management | eager：直接寫 ＋ undo log（commit 快）；lazy：redo log（abort 快） |
| Conflict detection | eager（每次存取）vs lazy（commit 時） |
| 正確性 | serializable（transaction 的 SC）⊂ strictly serializable（加即時順序）⊂ opacity（abort 的也看到一致的狀態） |
| Mixed mode | SLA（一把全域鎖）、DLA（多把鎖照順序）、TSC（黃金標準） |
| Bartok | value／version／lock；讀記 version；寫上鎖、undo log；commit 比 version、version 加一、放鎖；沒有 opacity |
| TL2 | 全域時鐘、rv、wv；讀 timestamp 讀兩次；redo log；commit 才上鎖；有 opacity |
| HTM（LogTM） | begin／abort／commit；undo log；L1 line 加 R、W bit；衝突回 NACK；M@C ＋ overflow bit；flash clear |
| 實測 | Bartok 沒有 opacity、TL2 有；課本字面的 Bartok 會 commit 被 rollback 的值；本機沒有 TSX；不照順序拿鎖會 deadlock；每帳戶一把鎖比一把鎖快 4 倍；libitm 比鎖慢 |

---

## 17. 與後續講次的連結

| 本講觀念 | 在哪裡 |
|---|---|
| 鎖、atomic 指令、lock-free、wait-free | [第 37 支](lecture37_note.md) |
| Data race、DRF ⇒ SC | [第 38 支](lecture38_note.md) |
| Directory 協定（HTM 改的就是它） | [第 36 支](lecture36_note.md)、[第 37 支](lecture37_note.md) |
| Virtual channel 排順序避免 deadlock | [第 29 支](lecture29_note.md) |
| Cache 的 subarray（flash clear） | [第 23 支](lecture23_note.md) |
| 下一章：主記憶體（DRAM） | [第 41 支](lecture41_note.md) |

---

## 附：本講一句話總結

> **一把鎖讓不相干的帳戶也不能平行，每個帳戶一把鎖又可能 deadlock，照順序拿鎖則要事先知道要哪些鎖，所以有 transactional memory：把程式包進 atomic block，系統保證 ACID，read set 和 write set 有交集（不只是都讀）就是衝突，用 abort 重試或 stall 解決；設計上要選 pessimistic 或 optimistic 的 concurrency control、eager（直接寫加 undo log）或 lazy（redo log）的 version management、eager 或 lazy 的 conflict detection，正確性則從 serializable、strictly serializable 到要求連 abort 的 transaction 都只看到一致狀態的 opacity；軟體的 Bartok 用 undo log 和寫時上鎖、commit 時比 version，沒有 opacity，TL2 用 redo log 和全域時鐘、commit 時才上鎖，有 opacity；硬體的 LogTM 在 L1 每個 line 加 R、W bit、改 coherence 協定回 NACK 偵測衝突，被趕出的 block 用 M@C 和 overflow bit 記住。本機實測：模擬確認 Bartok 沒有 opacity、TL2 有，還發現照課本字面寫的 Bartok 會 commit 一個被 rollback 的值；本機 CPU 沒有硬體 TM，不照順序拿鎖真的會 deadlock，每個帳戶一把鎖在帳戶多時比一把鎖快 4 倍，GCC 的軟體 TM 則比鎖慢。**

---

*筆記依據 Smruti R. Sarangi「Advanced Computer Architecture」Chapter 9: Multicore Systems (Part IX) -- Transactional Memory 的 YouTube 字幕（1:21:54）、Sarangi *Next-Gen Computer Architecture* v3.2 Ch9 §9.7（p.509–527）整理。實測為本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4、Python 3.10）結果。*
