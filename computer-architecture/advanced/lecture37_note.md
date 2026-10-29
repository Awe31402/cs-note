# Advanced Computer Architecture 第三十七支影片完整筆記：Multicore（六）：directory 的優化、false sharing、atomic 指令、spin lock 與 lock-free（Chapter 9, Part VI）

> **課程**：Advanced Computer Architecture — Chapter 9: Multicore Systems (Part VI) -- Optimized directories and atomic instructions（播放清單第 37 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：YouTube 自動字幕（59:55）＋ 課本 *Next-Gen Computer Architecture* v3.2 **Ch9** §9.4.6–9.4.8（p.475–489）
> **章號**：影片 Chapter 9 = 課本 Ch9。
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄
> - `【實測】`：在本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4、clang 14、Python 3.10）實際執行的結果
> - `⚠️`：逐字稿缺字、或影片／課本／實測說法不一致之處
> - `[mm:ss]`：影片時間點

⚠️ 自動字幕把「CAS」寫成「cast／Gast」、「lock」寫成「log」、「wait-free」寫成「weight free」、「Herlihy and Shavit」寫成「har and Sha」，筆記直接改正。

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. Directory 的三個問題](#1-directory-的三個問題)
- [2. 多個 directory](#2-多個-directory)
- [3. Sharer 清單怎麼省空間](#3-sharer-清單怎麼省空間)
- [4. Directory 本身的大小與趕出](#4-directory-本身的大小與趕出)
- [5. False sharing](#5-false-sharing)
- [6. Race condition：真實的協定有很多狀態](#6-race-condition真實的協定有很多狀態)
- [7. 銀行帳戶：為什麼需要 critical section](#7-銀行帳戶為什麼需要-critical-section)
- [8. Atomic exchange 與 lock／unlock](#8-atomic-exchange-與-lockunlock)
- [9. 用 coherence 協定實作 atomic 指令](#9-用-coherence-協定實作-atomic-指令)
- [10. Test-and-exchange（TAX）lock 與 backoff](#10-test-and-exchangetaxlock-與-backoff)
- [11. 其他 atomic 操作](#11-其他-atomic-操作)
- [12. Lock-free 與 wait-free](#12-lock-free-與-wait-free)
- [13. Consensus number：atomic 操作的「威力」](#13-consensus-numberatomic-操作的威力)
- [14. 【原始碼】Linux 的 spinlock 與 mutex](#14-原始碼linux-的-spinlock-與-mutex)
- [15. 【實測】](#15-實測)
- [16. 一頁速查表](#16-一頁速查表)
- [17. 與後續講次的連結](#17-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

```
directory 的優化：
  依 block 位址的 LSB 分成好幾個 directory → 小、快、不搶
  sharer 清單：fully mapped（N bit）→ 一個 bit 管一組 cache → 只記 K 個 id ＋ overflow bit（partially mapped）
  directory 是 set-associative cache；entry 被趕出 → 所有副本都要 invalidate
false sharing：不同的 byte、同一個 block → block 在 core 之間彈來彈去
真實協定：race condition → 100+ 個狀態 → 要自動驗證
atomic 指令：用 coherence 實作 → 拿到 M → 不回應別人 → 讀、改、寫 → 也是 fence
  lock：xchg 迴圈 → TAX（先用 load 在 S 狀態轉，看到 0 才 xchg）→ exponential backoff
  test-and-set、fetch-and-add、CAS、LL/SC
  lock-free（CAS 迴圈，可能餓死）→ wait-free（互相幫忙，不會餓死）
  consensus number：xchg、TAS、FAA = 2；CAS、LL/SC = ∞ → 每個 ISA 都要有 CAS 或 LL/SC
```

---

## 1. Directory 的三個問題

[00:00] 上一講的 directory 協定正確、完整，**但沒效率**。

[00:48] 問題：

1. 程式 **working set** 裡的每個 block 都要一個 entry。working set：程式在短時間內用到的記憶體；課本用程式的「**phase**」來定義短時間：程式在一個穩定的 phase 裡大約用固定大小的記憶體。所有 core 加起來 → directory **很大**。
2. [01:11] 每個 entry 裡，每個 sister cache 都要一份記錄 → 儲存開銷。
3. [01:33] 只有一個 directory → 它自己變成**搶來搶去的熱點**（point of contention）。

---

## 2. 多個 directory

[02:01] 把實體位址空間**分成互不相交的幾份**，每份配一個 directory：

- [02:28] 用 block 位址的 **LSB** 分，不要用 MSB：「分得**均勻**很多」。
- 一個 block 永遠對到**同一個** directory → **沒有正確性的問題**。
- [02:53] 熱點問題解決了：不同的 directory 管不同的位址；訊息依位址在 NoC 上送到對的 directory；每個 directory 也比較小。

【課本補充】課本 p.476：例如用 block 位址的最低 3 個 bit 分成 8 份、8 個 directory，「跟做 banked cache 的道理一樣」。一個大 directory 要很多讀寫 port，又慢又耗電。

---

## 3. Sharer 清單怎麼省空間

[03:14] 三種做法：

| 做法 | 怎麼記 | 例子 |
|---|---|---|
| **Fully mapped** | 每個 sister cache 一個 bit | [03:40] 64 個 sharer → 64 bit = 8 B／block |
| **一個 bit 管一組 cache** | [04:09] 64 個 cache 分成 16 組，每組 4 個；bit 是 1 = 這組**至少一個**有副本 | 組裡面再跑一個小的 **snoopy** 協定，或送 multicast 問組裡每個 cache |
| **Partially mapped** | [05:33] 只記 **K 個** sharer 的 id，再加一個 **overflow bit** | [05:57] 64 個 cache、K = 4：4 × 6 bit = 24 bit，加 overflow = 25 bit |

「一個 bit 管一組」的兩個好處：

1. [04:39] directory 省空間。
2. [05:06] 剛好符合現代多核心的 **tile** 結構：每個 tile 有 core 和 cache bank，tile 裡共用 bus → 「tile 裡跑 snoopy、整體跑 directory」：組裡有寫的順序，整體也有全域的寫順序。

Partially mapped：

- [05:57] 大部分 block 沒那麼多 sharer：「最多 2、4、6 個」。
- [06:25] overflow bit = 1：sharer 超過 K 個，「非常少見」。
- [06:50] 這時退回**廣播**給所有 sister cache：慢很多，但很少發生，可以接受。「相當流行。」

【課本補充】課本 p.476–478：

- 大的 server 可以有 256 個 core（還能在執行時插入更多處理器），每個 entry 256 bit 太多。課本 p.477 說 overwhelming majority 的情況只有 4 或 8 個 sister cache 共用。256 個 cache 的 id 是 8 bit，記 4 或 8 個 → 32 或 64 bit。
- Definition 77：partially mapped scheme。
- overflow 的三種處理：

| 策略 | 做法 |
|---|---|
| **Replace** | 挑一個現有的 sharer invalidate 掉，騰出位置 |
| **Invalidate All** | 設 overflow bit；讀沒問題，**寫**的時候要 invalidate **所有** sister cache |
| **Coarse grained** | 改變粒度：例如 256 個 cache 兩兩一組變 128 組（7 bit 的 id），64 bit 能記 9 組 = 最多 18 個 cache；最壞一組 256 個。缺點：組裡誰真的有副本不知道，invalidate 要送給整組 |

影片的「一個 bit 管一組」跟課本的 coarse grained 想法類似，但影片是一開始就分組，課本是 overflow 時才改粒度。

---

## 4. Directory 本身的大小與趕出

[07:17] 理想上 directory 要跟所有程式 working set 的**總和**一樣大（多個 directory 時是分攤）。

[07:38] 每個實體 block 都給一個 entry 不實際 → directory 做成 **set-associative cache**：

- [08:05] tag array：block 位址的一部分；data array：sharer 清單和 state。

[08:28] entry 被趕出時怎麼辦？

- [08:51] 位址空間有些部分是**唯讀**或**私有**的，根本不會進 directory。
- 共用的 block 一定要進 directory；它的 entry 被趕出 → **所有 cache 裡的副本都要 invalidate**。
- [09:15] 這就是為什麼 directory 要大、要做前面那些省空間的優化。

【課本補充】課本 p.475–476：32 GB 記憶體、64 B 的 block 要 **5 億**個 entry。entry 被趕出時兩個選擇：存到記憶體或硬碟（太多空間，不實際），或直接**忘掉**（下次不知道 state 和 sharer）。唯一的辦法：invalidate 所有 sharer，改過的要 write back，下次從乾淨的 **U** 狀態開始。所以「需要非常好的 replacement 演算法」。

---

## 5. False sharing

[09:15] 多處理器才有的新 miss。

[09:45] 一個 64 B 的 block：thread 1 存取其中一組 byte，thread 2 存取**另一組**。同時存取時：

- thread 1 invalidate thread 2 的副本，thread 2 再 invalidate thread 1 的……block 一直**彈來彈去**，效能下降。
- [10:09] 但**沒有真的共用**：存取的 byte 不重疊，只是剛好在同一個 64 B 的 block 裡 → **false sharing**。
- [10:32] 原因：block 是 coherence 的**不可分割單位**。
- 解法：**聰明的 compiler** 把資料放得更聰明。

【課本補充】課本 p.478–479 Definition 78：**true sharing miss**：兩個 thread 對**同一組** byte 有衝突的存取。**false sharing**：衝突的存取在同一個 block 裡**不重疊**的 byte。解法除了 compiler，還有 **word-level 的 coherence 追蹤**（記錄每個 word 被誰改過），但硬體很貴、很複雜，「compiler 的做法簡單很多」。課本 p.478 說一般的 block 是 64 或 128 B，一般的存取是 4 或 8 B。

（本機在 [第 36 支 §12.2](lecture36_note.md#122-本機的-false-sharing) 量過：`lock add` 慢 4.7 倍，界線剛好在 64 B；Linux 的對策見 [第 36 支 §11](lecture36_note.md#11-原始碼linux-怎麼避開-false-sharing)。）

---

## 6. Race condition：真實的協定有很多狀態

[10:57] 上一講的 MOESI 其實有 **7** 個狀態（加上 St、Se）。

[11:20] 真實硬體裡有很多互動：同一個 block 可能**同時**到幾種不同的訊息，還有 context switch、interrupt……

- [11:45] 例：正要從 M 轉到 S，同時收到 read miss 和 write miss：該轉到 S 還是 I？誰送資料給誰？
- [12:11] 這種對同一個 block 的並行事件叫 **race condition**。「不能讓一個特殊情況毀掉整個協定。」
- [12:35] 全部考慮進去，實用的協定「很可能有 **100 多個狀態**」，「**200 多個狀態**也不少見」。人沒辦法證明它對，要用**自動驗證器**（automated verifier）吃狀態圖檢查。

⚠️ 數字跟課本不一樣：

| | 影片 [12:35] | 課本 p.478 |
|---|---|---|
| 規模 | 100+ 個**狀態**，200+ 也不少見 | 20–30 個**狀態**、100+ 個**轉換**；Vantrease et al. 2011 報告過有 **400 個轉換**的協定 |

【課本補充】課本 p.477–478：

- 狀態轉換實際上不是瞬間完成的：S → M 時 cache 送訊息給 directory 再等；directory 排隊、送 invalidate、等每個 sharer 回 **acknowledgement**，收齊了才讓 cache 寫 → 要加很多「**等待**」狀態。
- **split transaction protocol**：把一連串動作拆開，跟其他請求交錯。
- 例：A 有 M 的 block，B 要讀，directory 去跟 A 要；訊息到 A 之前 A 決定趕出它 → read miss 和 evict 的 race。NoC 不保證訊息順序：write back 一定要比之後去下層找的請求先到。保守的做法（A 等 directory 確認才能趕出）會降低效能 → 真實的協定（例如 **Cray X1**）加更多狀態、轉換、訊息。

---

## 7. 銀行帳戶：為什麼需要 critical section

[12:59] 把 coherence 協定延伸，實作 **critical section** 和 **atomic 操作**（也是 synchronization 操作）。

[13:25] 經典例子：存 100 盧比到銀行帳戶（【口頭】「國際觀眾請換成美元或你們的貨幣」；盧比 = INR）。

```
t1 = account.balance       ← load
t2 = t1 + 100              ← add
account.balance = t2       ← store
```

[14:25] 希望三行 **atomic**：要嘛全做、要嘛全不做。

[15:21] 兩個 thread 同時跑：原本 100，兩個都讀到 100，都算出 200，都存 200 → **最後是 200，應該是 300**。[16:11] 原因：沒有同步，同一個位址的 **race condition**。

[16:40] 一個整段跑完再跑另一個就對了：200 → 300。

[17:06] 解法：**lock** 起來 → 一次只有一個 thread 能執行 → **critical section**（CS）。

- [17:34] atomic 的意思跟 write 的 atomicity 一樣：三行看起來**瞬間**全部執行，或看起來完全沒執行。
- [18:27] lock 函式回傳時，thread 對這段程式有**獨占權**；unlock 放掉。

【課本補充】課本 p.481 Definition 79：critical section：連續的一段敘述，全部 atomic 執行；thread t 開始執行第一個指令後，t 做完之前別的 thread 不能執行同一個 critical section 的指令。課本的例子起始 balance 是 0、兩個人各存 100，錯的結果是 100。

（本機實測不保護的話少掉多少，見 [§15.2](#152-七種做法搶同一個-balance)。）

---

## 8. Atomic exchange 與 lock／unlock

[18:53] 要實作 lock、unlock，需要 **atomic 指令**：

- 跟 atomic 的 critical section 一樣：完全執行或完全不執行，沒人看得到一半的狀態。
- [19:21] 大部分 atomic 操作**也是 fence**。

[19:43] **Atomic exchange**（`xchg`）：一個暫存器、一個記憶體位址，看起來瞬間交換兩者的內容。[21:23] 例：3 和 5，你只會看到 (3, 5) 或 (5, 3)。

- [20:09] 商用的指令集都有，「沒有它們就做不出 lock」。
- **x86**：在一般的 exchange 前面加 **`lock`** prefix 就變成 atomic。
- [20:34] 為什麼貴：要寫（不算太貴）；**更貴的是它是 fence**：禁止 fence 兩邊的指令亂序執行。
- [20:59] 用來實作 execution witness 裡的 **synchronization 邊**（[第 35 支 §8](lecture35_note.md#8-synchronization-邊so)）。

[21:49] lock 的組語（r0 = lock 的位址；0 = 沒人拿、1 = 有人拿了）：

```
.lock:                         .unlock:
    mov  r1, 1                     mov  r1, 0
    xchg r1, 0[r0]                 xchg r1, 0[r0]
    cmp  r1, 0                     ret
    bne  .lock
    ret
```

- [23:02] 重點是 r1 拿回來的**舊值**：0 → 是我把它從 0 改成 1 的 → 拿到 lock；1 → 別人早就拿了 → 再試。
- [23:54] 一直試到拿到為止。
- [24:24] unlock 也用 xchg 存 0：不用迴圈，因為「最後一定會成功」（coherence 的要求）。
- [25:17] lock 可能**無限地**轉下去：別的 thread 一直拿到。

[32:13] 為什麼要 fence？critical section 裡的指令**不能在拿到 lock 之前**執行，否則 lock 就沒意義 → lock 一個 fence、unlock 一個 fence。

[33:29] 一個重要的性質：把所有 thread 的 xchg 指令單獨拿出來看，它們是 **SC**：

- program order：每個 xchg 都是 fence，順序守住了。
- atomicity：它們是 atomic 指令。
- 「記住這一點，之後會用到。」

【課本補充】課本 p.482 Theorem 9.4.7.1：這個 lock 保證 **mutual exclusion**（互斥）。反證：T1、T2 同時在 critical section，假設 T1 先拿；T2 執行 xchg 時一定看到 T1 設的 1，不可能進去 → 矛盾。課本 p.482 也說 unlock 用 xchg 而不用一般 store 的原因：fence 保證別的 thread 看到 unlock 時，也看得到 critical section 裡所有的讀寫，「**不管 memory model 是什麼**」。

---

## 9. 用 coherence 協定實作 atomic 指令

[25:43] `xchg r1, 0[r2]` 拆開：

```
temp ← r1;  r1 ← mem[r2];  mem[r2] ← temp
```

[26:12] 這是 **read-modify-write**（RMW）：讀記憶體、（改）、寫記憶體。三步要看起來**瞬間**完成。（fetch-and-increment 就會真的「改」。）

[27:05] 做法：

1. [27:37] 拿到這個位址的**獨占寫權**：line 進 **M**（送一個 write miss）。
2. [28:02] 執行交換。**快速地**做完三步，就能保證 atomicity。

[28:30] fence 的部分：

- 等 xchg 到 **ROB 的最前面**（前面的指令都完成了）。
- [28:57] 有些寫可能已送進記憶體系統但還沒到最終位置 → **清空 write buffer、MSHR**，等記憶體系統回報「前面的都完全完成了」。
- [29:22] xchg 後面的指令都不准讀寫（停住）。

[29:49] 執行中：

- **interrupt 先擱著**，送給下一個指令，不送給它。
- 【口頭】有的變體會回報有沒有成功；那種的 interrupt 可以送，指令可能只做一半，使用者會知道沒成功。
- [30:35] 寫完要等它**完全完成**：清空所有中間的 buffer。架構要提供某種「寫完全完成」的機制，「大部分時候清空 buffer 就夠了」。
- [30:58] 執行中可能收到本地 cache、別的 cache、directory 的請求（例如讀了還沒寫，突然來了 write miss）→ **做完之前一律不回應**。

[31:21] 所以 atomic 指令很重：① 有 fence；② 執行時要很小心，不回應任何訊息 → 不會被打斷。

【課本補充】課本 p.483：在 directory 協定裡，這個過程從 directory 收到 write miss 開始：invalidate 所有副本、收齊 ack、通知要寫的 cache，它把 line 設成 M。中途有人送 miss 時兩種做法：① cache **晚點才回 ack** 給 directory；② **lease**（租約）：directory 保證拿到 M 的 cache 至少有 κ 個 cycle，期間的請求先排隊；κ 夠做完 atomic 操作就不需要 ack。

（x86、ARM、RISC-V 實際用哪些指令，見 [§15.1](#151-atomic-操作編譯成什麼指令)。）

---

## 10. Test-and-exchange（TAX）lock 與 backoff

[34:38] 基本 lock 的問題：每次嘗試都用 xchg 把 line 鎖在 **M**；很多 thread 一起試 → block 在它們之間**彈來彈去**，開銷很大。

[35:25] 改良版（**TAX**，test-and-exchange）：

```
.lock:
    mov  r1, 1
.test:
    ld   r2, 0[r0]          ← 先用一般的 load「測」
    cmp  r2, 0
    bne  .test              ← 還被拿著 → 繼續用 load 轉
    xchg r1, 0[r0]          ← 看到 0（有希望）才 xchg
    cmp  r1, 0
    bne  .test
    ret
```

- [35:54] 一個 thread 拿著 lock、其他 n 個在等：它們最後都把 line 放在 **S**，只在**自己的本地副本**上轉，非常快，不用把 line 鎖在 M。
- [36:44] 看到 0 才試 xchg；成功就拿到，失敗再回去用 load 轉。
- [38:28] unlock 一樣用 xchg 存 0。

[37:11] 更省電的版本：**random delay** 或 **exponential backoff**：失敗後等 0～T 的隨機時間；再失敗等 0～2T；再失敗 0～4T……「機率極高地」最後會拿到 lock。

【課本補充】課本 p.483–485：

- spin lock 的三個缺點：① 每次嘗試都做記憶體、算術、同步操作，耗電又慢；② 等的 thread 在做沒用的事，處理器卻以為它很忙，不會排別的 thread；③ 可能**餓死**。公平的 lock 演算法請看 **Herlihy and Shavit** 的書。
- TAX 的代價：**沒人搶**時多了一個 test 的步驟，拿 lock 稍微慢一點；有人搶時，把 write miss 換成 **read hit**（S 狀態），不送任何訊息給 directory。
- Definition 80：spin lock。

⚠️ 課本 p.484 說「Linux 的 lock 通常等固定一段時間，**通常 100 µs**，然後自動送 interrupt 給 OS kernel，讓 thread 去睡」。Linux v6.2.12 的原始碼不是這樣：spinlock 永遠不睡；mutex 只在 lock 的主人**還在某個 CPU 上跑**時才轉，主人不在 CPU 上或自己該讓出 CPU 時就停止轉、去睡，原始碼裡沒有固定的 100 µs。見 [§14](#14-原始碼linux-的-spinlock-與-mutex)。

（本機實測 xchg、TAX、backoff 的差別，見 [§15.2](#152-七種做法搶同一個-balance)。）

---

## 11. 其他 atomic 操作

[39:21]（課本圖 9.51）整段 pseudo code 都 atomic 執行：

| 操作 | 範例 | 做什麼 |
|---|---|---|
| **Test and set** | `tas r1, 8[r0]` | 8[r0] == 0 → 設成 1、r1 = 1（成功）；否則 r1 = 0 |
| **Fetch and increment** | `fai r1, 8[r0]` | r1 = 8[r0]；8[r0] = r1 + 1 |
| **Fetch and add** | `faa r1, r2, 8[r0]` | r1 = 8[r0]；8[r0] = r1 + r2（ISA 要支援） |
| **Compare and set**（CAS） | `cas r1, r2, r3, 8[r0]` | 8[r0] == r3 → 8[r0] = r2、r1 = 1；否則 r1 = 0 |
| **Load linked** ／ **Store conditional**（LL／SC） | `ll r1, 8[r0]` … `sc r3, r2, 8[r0]` | LL：像一般 load；SC：**從上次 LL 之後 8[r0] 沒被寫過** → 8[r0] = r2、r3 = 1；否則 r3 = 0 |

[40:41] CAS 和 LL／SC 被認為**威力最強**（[§13](#13-consensus-numberatomic-操作的威力)）。

[41:41] LL／SC 一定**成對**使用：

- [42:09] LL 是一個「同步的」load，【口頭】「也是一個 fence」。
- [43:07] SC 不在乎 lock 位址裡是什麼值，只在乎「從上次 LL 到現在，有沒有人寫過這個位址」。
- [44:55] 可以用來做跟 CAS 類似的事。
- [46:16] 實作：LL 時把位址以 M 狀態拿進來，等之後的 SC；中間如果有人 invalidate 它 → 有人寫過了 → SC 失敗。

[42:39] CAS 要 **4 個運算元**（3 個暫存器 ＋ 1 個記憶體），大部分 ISA 不做一點手腳就支援不了；**LL／SC 很適合 RISC 指令集**。

【課本補充】課本 p.486：LL 在含這個 block 的 cache line 上設一個 **flag**；別的 cache 的 write miss 進來就把它清成 0；SC 時除了拿寫權還要檢查 flag：還是 1 才成功；是 0 **或 block 被趕出了**就失敗。課本 Example 12 用 LL／SC 寫 lock（先 `ll` 測是不是 0，再 `sc`），unlock 是 `fence` 加一般的 `st`。課本 p.487：fetch-and-add 都可以用 CAS 做出來，但很麻煩；更深的理由是不同 atomic 操作的「**威力**」不一樣，強的可以做出弱的，反過來不行。

---

## 12. Lock-free 與 wait-free

[45:26] 有了這些 atomic 操作，可以寫**不用 lock** 的演算法。lock 的問題：拿著 lock 的 thread 去睡了，其他 thread 都只能等。

[46:49] 用 CAS 存錢（**lock-free**）：

```c
while (1) {
    t1 = account.balance;
    t2 = t1 + 100;
    if (CAS(&account.balance, t1, t2)) break;    // balance 還是 t1 才改成 t2
}
```

- [47:16] 兩個 thread 一起 CAS：一個成功（100 → 200），另一個的 balance 跟它讀到的不一樣 → **失敗**，再試。
- [47:42] 把 lock、unlock 那一大堆函式呼叫，摺成一小段程式。
- [48:53] 整個 CAS 是 atomic 的：「這就是 atomic 指令的魔法」。
- [49:20] 最好的情況：不用執行 lock、unlock 的額外指令。
- [49:44] 一個 thread **可能餓死**（它的 CAS 一直失敗），但「實際上很少發生」，「大部分人不用擔心」，所以 lock-free 演算法現在很流行。

[50:07] 要**完全消除**餓死 → **wait-free** 演算法：

- [50:31] 一個陣列存所有待處理的請求。
- [51:09] 新的請求 T 先找一個**等很久**的請求 R（快餓死了），**幫 R 做完它的工作**。
- [51:36] R 變成最老的請求後，**所有** thread 都會去幫 R，總有一個成功 → R 一定完成（自己做完或別人幫它做完）。
- [52:03]「沒有請求被丟下」：這種先幫別人的演算法叫 **altruistic**（利他的）演算法 → 整個系統 wait-free。
- [52:54] lock-free 有無限迴圈，**不保證完成**；wait-free **保證**每個請求在**有限**時間（不一定有上界）內完成。

【課本補充】課本 p.487–488：

- lock 的其他問題：不允許 **disjoint access parallelism**（兩個 thread 改不同的帳戶也不能同時進 critical section；用 lock 保護的 concurrent queue 等於變成循序的 queue）；**deadlock**（T1 拿 A 要 B、T2 拿 B 要 A）；OS 讓等的 thread 睡，「叫醒它要花不成比例的時間」。
- 有一個一般的結果：stack、queue 等資料結構的所有操作都能用 lock-free 演算法實作（Herlihy and Shavit）；但 lock-free「很難寫、很難 debug」。
- 指令數：沒人搶時 lock-free 存錢要 5 個指令；用 lock 要 5（lock）＋ 3（unlock）＋ 3（更新）個指令和 2 次函式呼叫。
- Definition 81：lock-free 可能餓死；wait-free 保證在有限或有上界的步數內完成。wait-free「比較複雜、平均比較慢」。

---

## 13. Consensus number：atomic 操作的「威力」

[53:17] 所有 atomic 操作一樣強嗎？「威力」是什麼？

[53:47] **Consensus 問題**：每個 thread 提一個值（thread 1 提 3、thread 2 提 5、thread 10 提 7……），跑一個平行演算法，從提出的值中**選一個**，**所有人都同意**。

[54:40] 為什麼重要：它是大部分並行演算法的理論基礎。例：5 個 thread 搶一個 lock，最後大家同意一個**贏家**進 critical section → 就是 consensus。[55:56] 平行的 lock、queue、stack 都建立在它上面。

[56:23] **Consensus number**（某種操作的）：只用一般的讀寫加上**這種操作**，用 **wait-free** 演算法能解 consensus 問題的**最多 thread 數**。

| 操作 | Consensus number |
|---|---|
| Atomic exchange | 2 |
| Test and set | 2 |
| Fetch and add、fetch and increment | 2 |
| **CAS** | **∞** |
| **LL／SC** | **∞** |

- [57:11] 證明不講，請看 **Herlihy and Shavit** 的書。
- [57:37] exchange 那一家族超過 2 個 thread 就解不了：限制了能寫的平行程式（例如「沒有餓死的 lock」對 3 個以上的 thread 就做不到）。
- [58:00] 所以**每個架構都要實作 CAS 或 LL／SC**（兩個都是 ∞）。

[58:51]「花了兩整講在 coherence 上。」下一講：memory model 的總結、**data race**：「瓶子的蓋子」，有了它就有在極度寬鬆的 memory model 機器上寫實際程式所需的所有結果。

【課本補充】課本 p.488–489 Definition 82；consensus number 是 k 表示**理論上**不可能寫出 k + 1 個 thread 的 wait-free consensus 演算法。課本 p.489：「現代加密貨幣（例如 **Bitcoin**）的核心是一個 consensus 問題」，線上付款等交易處理系統也是。consensus number 低的操作**不能**用 wait-free 演算法做出 consensus number 高的操作（例如不能用 test-and-set 做 CAS）。

（本機看到 x86 的 `xchg` 和 ARM 的 `swpal`、RISC-V 的 `amoswap` 都是 exchange 家族；CAS 在 x86 是 `lock cmpxchg`、在沒有 LSE 的 ARM 和 RISC-V 是 LL／SC 迴圈，見 [§15.1](#151-atomic-操作編譯成什麼指令)。）

---

## 14. 【原始碼】Linux 的 spinlock 與 mutex

**spinlock 的快速路徑**：一個 CAS。`include/asm-generic/qspinlock.h:107-115`：

```c
static __always_inline void queued_spin_lock(struct qspinlock *lock)
{
	int val = 0;

	if (likely(atomic_try_cmpxchg_acquire(&lock->val, &val, _Q_LOCKED_VAL)))
		return;

	queued_spin_lock_slowpath(lock, val);
}
```

- lock 是 0（沒人拿）就用 CAS 改成「已上鎖」，一個指令搞定；有人拿著才走 slowpath。
- 用的是 **acquire**（[第 34 支 §13](lecture34_note.md#13-acquire-與-release)），不是完整的 fence。

**unlock**：一般的 store 加 release。`include/asm-generic/qspinlock.h:123-129`：

```c
static __always_inline void queued_spin_unlock(struct qspinlock *lock)
{
	/*
	 * unlock() needs release semantics:
	 */
	smp_store_release(&lock->locked, 0);
}
```

- 影片和課本的 unlock 用 xchg（完整的 fence）；Linux 只用 **release**：前面的讀寫都完成才能寫 0，後面的可以先跑。
- x86 的 `__smp_store_release` 只是 compiler barrier 加一般的 store（`arch/x86/include/asm/barrier.h:63-68`）：x86 的 store 本來就不會跟前面的讀寫重排（TSO，[第 35 支 §15.1](lecture35_note.md#151-execution-witness-檢查程式)）。ARM 上會變成 `stlr`（見 [§15.1](#151-atomic-操作編譯成什麼指令)）。

**slowpath 的等待**：每個等的 CPU 在**自己的**節點上轉（MCS queue）。`kernel/locking/mcs_spinlock.h:24-36`：

```c
#ifndef arch_mcs_spin_lock_contended
/*
 * Using smp_cond_load_acquire() provides the acquire semantics
 * required so that subsequent operations happen after the
 * lock is acquired. Additionally, some architectures such as
 * ARM64 would like to do spin-waiting instead of purely
 * spinning, and smp_cond_load_acquire() provides that behavior.
 */
#define arch_mcs_spin_lock_contended(l)					\
do {									\
	smp_cond_load_acquire(l, VAL);					\
} while (0)
#endif
```

- 跟 TAX 同一個想法再往前一步：TAX 讓大家用 load 在**同一個** line 的 S 副本上轉，lock 一放開所有人一起衝；MCS 讓每個等的 CPU 在**自己的**變數上轉，前一個人放開時只叫醒下一個。

**mutex 什麼時候轉、什麼時候睡**。`kernel/locking/mutex.c:352-384`（節錄）：

```c
bool mutex_spin_on_owner(struct mutex *lock, struct task_struct *owner,
			 struct ww_acquire_ctx *ww_ctx, struct mutex_waiter *waiter)
{
	bool ret = true;

	lockdep_assert_preemption_disabled();

	while (__mutex_owner(lock) == owner) {
		...
		barrier();
		...
		if (!owner_on_cpu(owner) || need_resched()) {
			ret = false;
			break;
		}
		...
		cpu_relax();
	}
```

- 只要 lock 的主人**還在某個 CPU 上跑**（很快會放開），就一直轉；主人不在 CPU 上（被換下去了）或自己該讓出 CPU（`need_resched()`），就不轉了、去睡。
- 這就是 [§10](#10-test-and-exchangetaxlock-與-backoff) ⚠️ 說的：Linux 看的是「主人在不在跑」，不是固定的 100 µs。
- `cpu_relax()` 在 x86 是 `pause` 指令：本機的 TAX 測試也在轉的迴圈裡用了 `pause`。

---

## 15. 【實測】

程式在 `experiments/lecture37/`：`atomics.c`（看指令）、`locks.c`（搶 lock）。

### 15.1 atomic 操作編譯成什麼指令

`atomics.c` 用 GCC 的 `__atomic_*` 寫 5 個函式，分別編譯成 x86、ARM（`aarch64-linux-gnu-gcc -O2`）、RISC-V（`clang --target=riscv64-linux-gnu -march=rv64gc -O2`）：

| 操作 | x86-64 | ARMv8.0（`-mno-outline-atomics`） | ARMv8.1（有 LSE） | RISC-V rv64gc |
|---|---|---|---|---|
| exchange | `xchgq` | `ldaxr`／`stlxr` 迴圈 | `swpal` | `amoswap.d.aqrl` |
| test and set | `xchgb`（存 1） | `ldaxrb`／`stlxrb` 迴圈 | `swpalb` | `lr.w`／`sc.w` 迴圈（byte 要遮罩） |
| fetch and add | `lock xaddq` | `ldaxr`／`add`／`stlxr` 迴圈 | `ldaddal` | `amoadd.d.aqrl` |
| CAS | `lock cmpxchgq` | `ldaxr`／`cmp`／`stlxr` 迴圈 | `casal` | `lr.d`／`bne`／`sc.d` 迴圈 |
| store-release（unlock） | 一般的 `movl` | `stlr` | `stlr` | `fence rw,w` ＋ `sw` |

ARMv8.0 的 CAS（LL／SC 版）：

```
.L9:
	ldaxr	x3, [x0]        ← LL（acquire）
	cmp	x3, x1
	bne	.L10            ← 值不對：CAS 失敗
	stlxr	w4, x2, [x0]    ← SC（release）；w4 ≠ 0 = 中間有人寫過
	cbnz	w4, .L9         ← SC 失敗就重來
```

- x86 **沒有 LL／SC**：用 `lock` 開頭的指令（影片 [20:09]）；`xchg` 碰到記憶體時**本來就是** atomic 的，gcc 不加 `lock`。
- ARMv8.0、RISC-V 用 **LL／SC**（`ldaxr`／`stlxr`、`lr`／`sc`）做出 CAS、exchange、fetch-and-add：跟 [42:39]「LL／SC 很適合 RISC」一致。
- 較新的 ARM（v8.1 的 LSE）和 RISC-V 的 A 擴充也有直接的 exchange、fetch-and-add 指令（`swp`、`ldadd`、`amoswap`、`amoadd`），ARM v8.1 還有 `cas`。
- ARM 的 `a`（acquire）、`l`（release）字尾直接標在指令上，就是 [第 34 支 §13](lecture34_note.md#13-acquire-與-release) 的 acquire、release。不加 `-mno-outline-atomics` 時，gcc 在 ARMv8.0 上是呼叫 `__aarch64_swp8_acq_rel` 這類函式，執行時才挑 LSE 或 LL／SC。
- 只看了 compiler 的輸出，本機沒有 ARM、RISC-V 機器可以跑，速度**待補**。

### 15.2 七種做法搶同一個 balance

`locks.c`：T 個 thread（綁在 cpu0、2、4、6、8、10 六個不同的 P-core，第 7、8 個是 E-core cpu12、13），每個把同一個 `balance` 加 1 共 100 萬次。數字是「全部做完的時間 ÷ 總次數」（ns，越小越好），兩次的範圍：

| T | 不保護 | xchg lock | TAX | TAX＋backoff | pthread mutex | CAS 迴圈 | fetch-and-add |
|---|---|---|---|---|---|---|---|
| 1 | 1.6 | 5.7–6.4 | 5.7–6.4 | 5.7–6.2 | 11.7–13.4 | 7.1–7.3 | **4.1–4.3** |
| 2 | 0.9–1.3 | 13–14 | 9–23 | 9–20 | 32–39 | 16–19 | **7.4–8.5** |
| 4 | 0.5 | 26–38 | 31–40 | 25–27 | 58–62 | 35–38 | **7.5–9.3** |
| 6 | 0.4 | 51–63 | 52–59 | 48–53 | 74–82 | 46–52 | **8.2–9.3** |
| 8 | 0.4 | 87–103 | 80–83 | 77–80 | 88–90 | 51–58 | **10–11** |

不保護的少掉多少次、CAS 平均重試幾次：

| T | 不保護：少掉的比例 | CAS：平均每次重試 |
|---|---|---|
| 2 | 25–50 % | 0.14–0.25 |
| 4 | 74–75 % | 0.54–0.67 |
| 6 | 78–80 % | 0.80–0.96 |
| 8 | 80–85 % | 0.87–1.04 |

- **不保護就是錯的**：2 個 thread 就少掉 25–50 % 的存款，8 個少掉 80 % 以上（[15:21] 的 race）。其他六種做法每一次的結果都對（程式會檢查）。
- **fetch-and-add 幾乎不隨 thread 數變慢**（4 → 11 ns）：一個 `lock xadd` 拿到 M、改完就放，沒有「拿 lock → 改 → 放 lock」三次搬 line。
- **所有 lock 都隨 thread 數線性變慢**（8 個 thread 要 80–100 ns 才完成一次）：line 在 core 之間搬一次大約幾十 ns（[第 27 支](lecture27_note.md)），每次 critical section 至少搬兩次。
- **TAX、backoff 的效果有，但不大，而且很吵**：8 個 thread 時 TAX（80–83）、TAX＋backoff（77–80）比 xchg（87–103）快一點；2 個 thread 時 TAX 反而可能比較慢（多了 test 那一步，跟課本 p.485「沒人搶時比較慢」一致）。本機不能用 perf 數 line 搬了幾次，只能看時間。
- **CAS 迴圈（lock-free）比所有 lock 都快**：8 個 thread 時平均每次重試約 1 次，但從來沒有「一直失敗」的情況（[49:44]「實際上很少發生」）。
- **pthread mutex 最慢**：沒人搶時就要 12–13 ns（函式呼叫、多做檢查）；但 8 個 thread 時跟 spin lock 差不多。

---

## 16. 一頁速查表

| 主題 | 重點 |
|---|---|
| 多個 directory | 依 block 位址的 LSB 分；同一個 block 永遠同一個 directory |
| Sharer 清單 | fully mapped（N bit）；一個 bit 管一組（組裡跑 snoopy）；partially mapped（K 個 id ＋ overflow bit → 廣播） |
| Directory 大小 | 做成 set-associative cache；entry 被趕出 → 所有副本 invalidate；要好的 replacement |
| False sharing | 不同的 byte、同一個 block；block 是不可分割的單位；靠 compiler 擺放資料 |
| Race condition | 同一 block 的並行事件；真實協定 100+ 狀態（影片）／20–30 狀態、100+ 轉換（課本）；自動驗證 |
| Critical section | 三行 load／add／store 要 atomic；lock／unlock |
| Atomic exchange | `xchg`；x86 加 `lock`；是 fence；所有 xchg 合起來是 SC |
| 實作 RMW | 等到 ROB 最前面、清空 write buffer／MSHR → 拿 M → 不回應別人、擱著 interrupt → 讀改寫 → 等寫完全完成 |
| TAX lock | 先 load 在 S 上轉，看到 0 才 xchg；再加 exponential backoff |
| 其他操作 | test-and-set、fetch-and-increment／add、CAS（4 個運算元）、LL／SC（適合 RISC） |
| Lock-free | CAS 迴圈；可能餓死但很少 |
| Wait-free | 互相幫忙（altruistic）；保證有限時間完成 |
| Consensus number | exchange、TAS、FAA = 2；CAS、LL／SC = ∞；每個 ISA 都要有 CAS 或 LL／SC |
| 原始碼 | Linux spinlock：CAS（acquire）上鎖、store-release 開鎖、MCS 各轉各的；mutex：主人在跑才轉 |
| 實測 | x86：`xchg`、`lock xadd`、`lock cmpxchg`；ARM、RISC-V：LL／SC 迴圈或 LSE／AMO；不保護少掉 80 %；fetch-and-add 最好，lock 線性變慢 |

---

## 17. 與後續講次的連結

| 本講觀念 | 在哪裡 |
|---|---|
| Directory 協定的基本版 | [第 36 支](lecture36_note.md) |
| False sharing 的實測、Linux 的 padding | [第 36 支 §11、§12.2](lecture36_note.md#122-本機的-false-sharing) |
| Fence、acquire、release | [第 34 支](lecture34_note.md) |
| Synchronization 邊（so） | [第 35 支](lecture35_note.md) |
| ROB | [第 13 支](lecture13_note.md) |
| MSHR（non-blocking cache） | [第 25 支](lecture25_note.md) |
| Core 之間搬一個 line 要多久 | [第 27 支](lecture27_note.md) |
| Memory model 的總結、data race | [第 38 支](lecture38_note.md) |
| Data race 偵測 | [第 39 支](lecture39_note.md) |
| Transactional memory（不用 lock 的另一條路） | [第 40 支](lecture40_note.md) |

---

## 附：本講一句話總結

> **基本的 directory 太大又會變成熱點，所以依 block 位址的 LSB 分成好幾個 directory、把 sharer 清單從每個 cache 一個 bit 縮成一組一個 bit 或只記幾個 id 加 overflow bit、把 directory 做成 set-associative cache（entry 被趕出就 invalidate 所有副本），另外還要面對 block 粒度造成的 false sharing 和讓真實協定膨脹到上百個狀態的 race condition；在 coherence 之上可以做出 atomic 的 read-modify-write 指令（拿到 M、做完之前不回應任何人、本身也是 fence），用 atomic exchange 寫 spin lock，再改良成先用 load 在 S 狀態上等的 TAX 和 exponential backoff，也能用 CAS 寫不用 lock 的 lock-free 演算法和互相幫忙的 wait-free 演算法；exchange、test-and-set、fetch-and-add 的 consensus number 只有 2，CAS 和 LL／SC 是無限大，所以每個 ISA 都必須有其中之一。本機實測：x86 用 `xchg`、`lock xadd`、`lock cmpxchg`，ARM 和 RISC-V 用 LL／SC 迴圈或新的單一指令；不保護的 balance 少掉 80 % 以上，fetch-and-add 幾乎不隨 thread 數變慢，所有 lock 都線性變慢，而 Linux 的 mutex 看的是 lock 的主人在不在跑，不是課本說的固定 100 µs。**

---

*筆記依據 Smruti R. Sarangi「Advanced Computer Architecture」Chapter 9: Multicore Systems (Part VI) -- Optimized directories and atomic instructions 的 YouTube 字幕（59:55）、Sarangi *Next-Gen Computer Architecture* v3.2 Ch9 §9.4.6–9.4.8（p.475–489）、Linux v6.2.12 原始碼整理。實測為本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4、clang 14、Python 3.10）結果。*
