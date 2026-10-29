# Basic Computer Architecture 第四十支影片完整筆記：用 fence 寫平行程式、shared 與 private cache、snoopy 協定（write-update、MSI write-invalidate）（Chapter 11 Cache Coherence Protocols, Part 3/5 → 課本 Ch12）

> **課程**：Basic Computer Architecture — Chapt 11: Cache Coherence Protocols, Part 3/5（播放清單第 40 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：YouTube 錄影逐字稿（1:19:05，自動字幕）＋ 課本 *Basic Computer Architecture* v3.09 **Ch12** §12.4.3 結尾、§12.4.4–12.4.6（p.590–603）
> **章號**：影片的 Chapter 11 是課本 v3.09 的 **Chapter 12**；影片說的「chapter 9」（cache）是課本 Ch11。
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄（本講沒有，fence 見[第 39 支](lecture39_note.md#16-原始碼linux-的-memory-barrier)）
> - `【實測】`：在本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4、Python 3.10）實際執行的結果
> - `⚠️`：逐字稿缺字、或影片／課本／實測說法不一致之處
> - `[mm:ss]`：影片時間點

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. Weak consistency 機器上的平行加總](#1-weak-consistency-機器上的平行加總)
- [2. 兩個 fence 放在哪裡、為什麼](#2-兩個-fence-放在哪裡為什麼)
- [3. Shared cache 與 private cache](#3-shared-cache-與-private-cache)
- [4. 兩種 cache 比一比](#4-兩種-cache-比一比)
- [5. 大的 shared cache：切成 bank](#5-大的-shared-cache切成-bank)
- [6. Private cache 要看起來像一個 cache](#6-private-cache-要看起來像一個-cache)
- [7. 關鍵：寫入要有全域的順序 → bus](#7-關鍵寫入要有全域的順序--bus)
- [8. Snoopy 協定與 shared bus](#8-snoopy-協定與-shared-bus)
- [9. Write-update 協定](#9-write-update-協定)
- [10. Write-invalidate 協定（MSI）](#10-write-invalidate-協定msi)
- [11. 為什麼 MSI 遵守 coherence](#11-為什麼-msi-遵守-coherence)
- [12. Directory 協定（一點點）](#12-directory-協定一點點)
- [13. 【實測】](#13-實測)
- [14. 一頁速查表](#14-一頁速查表)
- [15. 與後續講次的連結](#15-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

```
weak consistency 上的平行加總：初始化後一個 fence、寫完 partialSums 後一個 fence
physical view：
  shared cache（每個位置只有一份，簡單但大、慢、port 多）→ 切成 bank → 適合 L2 以下
  private cache（小、快、近，但有複本）→ 要 cache coherence → 適合 L1
snoopy 協定（共用的 bus 給寫入一個全域順序）：
  write-update：每次寫都廣播資料
  write-invalidate（MSI）：要寫先讓別人變 I；在 M 狀態寫不用送訊息
directory 協定：記錄誰有這個 block，只送給它們
```

---

## 1. Weak consistency 機器上的平行加總

[00:00]–[12:46]　課本 Example 154（p.590–592）

上一講在 SC 機器上寫了平行加總（[第 39 支 §15](lecture39_note.md#15-自己寫平行加總sc-機器)）。現在換成 **weak consistency** 的機器。

複習兩者的差別：
- SC：每個 thread 的記憶體指令都照 program order 完成。
- WC：**不同位址**的存取不保證順序（x = 1; y = 1 別人可能看到相反的順序）。但 **coherence 和單一 thread 的語意永遠成立**：同一位址照順序，自己寫過的自己一定讀得到（或更新的值）。

【口頭】[03:55] 為什麼不直接用 OpenMP？① 要先知道 OpenMP 是怎麼寫出來的；② 哪天換一台機器、OpenMP 效能不好，可能要直接在機器的層級寫程式。

【口頭】[04:38] 假設機器是**最弱的**：完全不保證順序，但有 fence。照最弱的寫，在比較強的機器（多保證一點順序）上一定也對。SC 是最強的、理想的，「沒有人有它」；高效能伺服器都是 weak。

```c
int partialSums[N];
int finished[N];
int numbers[SIZE];
int result = 0;
/* 把 partialSums、finished 設成 0 */
...
fence();                                   /* ① 初始化全部完成 */
parallel {
    int myId = getThreadId();
    int startIdx = myId * SIZE/N;
    int endIdx = startIdx + SIZE/N;
    for (int jdx = startIdx; jdx < endIdx; jdx++)
        partialSums[myId] += numbers[jdx];
    fence();                               /* ② partialSums 寫完才設 finished */
    finished[myId] = 1;
}
do {
    flag = 1;
    for (int i = 0; i < N; i++)
        if (finished[i] == 0) { flag = 0; break; }
} while (flag == 0);
for (int idx = 0; idx < N; idx++)
    result += partialSums[idx];
```

`fence()`：【口頭】[06:28] C 語言沒有叫 fence 的函式，但可以自己寫一個，裡面放一條 fence 指令。幾乎所有指令集都有，不一定叫 fence；例如 x86 帶 **lock 前綴**的指令（lock cmpxchg、lock xchg）就有 fence 的效果。

---

## 2. 兩個 fence 放在哪裡、為什麼

[12:46]–[17:59]　課本 p.592

**fence ①（初始化之後）**：
- SC 版用 doneInit 旗標：在 SC 上看到 doneInit = 1 就表示初始化都做完了。
- WC 上**不夠**：doneInit 可能已經是 1，但初始化的某些寫還沒到記憶體。
- 所以改成：初始化完 → fence（保證都寫進記憶體了）→ 才開新的 thread，新 thread 讀到的一定是對的值。

**fence ②（partialSums 和 finished 之間）**：
- 單一 thread 自己永遠沒問題：`+=` 一定拿到自己上一次寫的值（coherence 保證）。
- 但**另一個 thread** 可能還沒看到 partialSums 的最新值：寫訊息可能卡在記憶體系統的某處。
- 加了 fence：finished[myId] = 1 一定在 partialSums 寫完之後才執行。
- coherence 的 **completion 公理**保證 finished[myId] = 1 最終所有 thread 都看得到。看到它 → 這條指令執行了 → 它前面的 fence 執行完了 → partialSums 一定已經寫好。

【口頭】[15:47]「如果把 finished 放在 fence 前面，就**完全錯了**」：最後一個 thread 看到 finished = 1 就往下走，但 partialSums 的最後一次更新可能還沒到記憶體。

【口頭】[17:26]「主要的差別就是**兩條 fence 指令**。在 weak consistency 系統裡，最燒腦的就是 fence 要放在哪裡，一定要放在剛好對的地方，不然結果可能是錯的。」

【課本補充】p.592：
- weak memory model 不會犧牲正確性，前提是程式設計師知道它、把 fence 放對地方。很多平行程式的微妙 bug 都來自不管底層的 memory model。
- 除了 **MIPS R10000** 之外，沒有大廠提供 SC 的機器；**現在的 x86 和 ARM 都是不同版本的 weak memory model**。

---

## 3. Shared cache 與 private cache

[17:59]–[19:40]　課本 §12.4.4、Fig 12.8（p.593）

到這裡講的是**邏輯**觀點；接著看記憶體**實際**怎麼做。

| | 意思 |
|---|---|
| **Shared cache** | 一個 cache 給**所有**處理器共用，是一整塊 |
| **Private cache** | 每個處理器（或一組處理器）有**自己的** cache |

三種組合（Fig 12.8）：
- (a) L1 shared、L2 shared
- (b) L1 private、L2 shared
- (c) L1 private、L2 private

---

## 4. 兩種 cache 比一比

[19:40]–[28:00]　課本 Table 12.3（p.594）

| | Private cache | Shared cache |
|---|---|---|
| 面積 | 小 | 大 |
| 速度 | 快 | 慢 |
| 離處理器 | 近 | 遠（要對所有處理器一樣近 → 離每一個都有點遠） |
| 能做多大 | 不行（coherence 太複雜） | 可以 |
| 資料複本 | 有 | 沒有 |
| 複雜度 | 高（要 cache coherence） | 低 |

**shared cache 的難處**（【口頭】[21:12]）：最簡單，處理器 1 寫 x、處理器 2 讀 x 都去同一個地方。但如果 32 個處理器每個週期都可能送讀寫，就要 **32 個存取 port**，面積會大到不可行（【口頭】「500 倍是誇張了，但大到不能做」）。

**private cache 的難處**（【口頭】[21:46]）：
- x 在某個 cache 裡，處理器 1 怎麼知道它在那裡？
- 處理器 1 要寫 x，可能多做出一份 x → 現在有**兩份**。
- 處理器 N 要讀 x，要給它哪一份？怎麼知道哪一份比較新？
- 所以要讓這一群 private L1 **看起來就像一個 shared L1**，遵守 coherence 和機器的 consistency model；外部觀察者完全看不出差別。

**結論**（【口頭】[26:22]）：
- L1 的流量非常大 → 一定要 **private**（一個大 L1 太慢、port 太多、會橫跨整顆晶片，讀一個值要幾十個週期）。
- L1 濾掉了大部分存取，L2 的流量小很多 → L2 以下可以 **shared**。
- 商用處理器通常：**L1 private，L2 以下 shared**。

【課本補充】p.594：Definition 128：在一組 private cache 之間的 coherence 叫 **cache coherence**。複本也會浪費空間。

---

## 5. 大的 shared cache：切成 bank

[28:00]–[32:42]　課本 §12.4.5（p.595–596）

4 MB 的 shared cache：tag array、data array 都很大 → lookup 很慢；又要很多 port → 面積、功耗都大。

**解法：切成很多 bank**（每個 bank 是一個小的子 cache）：

```
4 MB = 2²² bytes，切成 16 個 bank（各 256 KB）
位址的第 19–22 位（4 bit）→ 選 bank
剩下的位元 → 在那個 bank 裡做一般的 cache lookup（bank 可以是 direct mapped 或 set associative）
```

- 等於把實體位址空間切成 16 份，每個 bank 負責一份（第 19–22 位相同的位址）。
- 不同的請求很可能落在**不同的 bank** → 可以平行處理。最壞情況全落在同一個 bank 就會塞車。

⚠️ [29:06] 老師說到「4 MB 是 2²² bytes」時一度改口「不是 bytes，是 bits」，其實 2²² 指的是 byte 數，第 19–22 是**位址的 bit 位置**。

**還是慢**（【口頭】[31:14]）：bank 散布在整顆晶片上，處理器在一角、要的 bank 在另一角，訊號來回可能要**超過 50 個週期**。L2 可以接受，L1 不行。

【課本補充】p.595：
- **LLC**（last level cache）：晶片上最低一層的 cache，例如有 L3 就是 L3。
- 2012 年晶片上的 cache 可達 4–8 MB；4 MB 的 LLC 通常切成 8–16 個 bank，每個 256–512 KB。
- 切 bank 的好處：4 個 thread、16 個 bank，兩個 thread 撞同一個 bank 的機率很低；每個 bank 小，也比較快。bank 選擇和 associativity 無關。

---

## 6. Private cache 要看起來像一個 cache

[32:42]–[37:20]　課本 §12.4.6（p.596）

最常見的做法：每個處理器一個 private L1，別人不能直接存取；這一群 L1 合起來，對處理器和外部觀察者來說就是**一個大的邏輯 cache**。

什麼時候會出問題（【口頭】[33:34]）：

```
T1: x = 1; x = 2; x = 3      T2: x = 4; x = 5; x = 6
T3: t1 = x; t2 = x           T4: t3 = x; t4 = x
```

- (t1, t2) = (2, 4)：可以。
- (t3, t4) = (4, 2)：單獨看也可以。
- **但兩個同時出現不行**：違反 order 公理（一個 thread 看到 2 在 4 前，另一個看到 4 在 2 前）。

觀察（【口頭】[35:47]）：
- 我們在乎的是 completion 和 order，也就是**寫入**：寫入一定要完成、對同一位置的寫入大家看到的順序一樣。
- 兩次寫之間，**讀的順序不重要**：永遠讀到最後那次寫的值。

---

## 7. 關鍵：寫入要有全域的順序 → bus

[37:20]–[40:12]　課本 p.596–597

寫入（對同一位址）需要一個**全域的順序**，所有 thread 看到同樣的順序。做法：每次寫之前先取得一個**獨占的資源**（exclusive resource：任何時刻只有一個請求能拿到）。

選一組銅線當這個資源：**bus**（匯流排）。

```
cache 1   cache 2   cache 3 … cache N
   └────────┴─────────┴────────┘  ← bus（一組銅線，同一時間只有一個能寫）
                bus controller（arbiter，決定給誰）
```

- 每個 cache 向 bus controller 要求獨占使用權，controller 用它自己的演算法挑一個。
- **誰在什麼時候拿到 bus**，就定出了所有請求的全域順序。

【課本補充】p.599 Fig 12.12：**bus arbiter**（匯流排仲裁器）很簡單，裡面是一個請求的**佇列**，依序把 bus 給下一個。

---

## 8. Snoopy 協定與 shared bus

[40:12]–[42:41]　課本 Fig 12.10（p.597）

cache coherence 有兩大類方法：**snoopy**（窺探）和 **directory**（目錄）。directory 比較難，這裡只講一點點。

**Snoopy 協定**：所有 private L1 接到一條 **multi-reader single-writer bus**（多人讀、一人寫）：
- 一個 cache 寫到 bus 上，**其他所有 cache 都讀得到**（廣播）。
- 同一時間只有拿到使用權的那個能寫 → 給寫入一個順序。
- 所有 cache 看到**一樣順序**的訊息，而且**每一則**都看得到 → 訊息不會遺失。

---

## 9. Write-update 協定

[42:41]–[60:16]　課本 Fig 12.11（p.597–599）

每個 private L1 的 tag array，每個 tag 旁邊多存一個**狀態**（2 bit，三種）：

| 狀態 | 意思 |
|---|---|
| **M**（modified） | 拿到這個 line 之後，這個處理器寫過它 |
| **S**（shared） | 有這個 line，沒改過 |
| **I**（invalid） | 這個 entry 沒有有效資料 |

所以叫 **MSI write-update** 協定。

**規則**（【口頭】[49:58] 兩句話版：「寫的時候把寫入送給所有處理器；讀的時候，自己的 cache 有就拿，沒有就問所有姊妹 cache，誰有就給」）：

| 事件 | 動作 |
|---|---|
| 寫 | 把寫入**廣播**到 bus，所有有這個 line 的 cache 都更新 → 大家看到同樣的寫入順序（因為 bus 的順序） |
| 讀 hit | 直接讀，一定是最新的 |
| 讀 miss | 在 bus 上廣播 read miss；有人有就經 bus 送過來，否則去下層拿 |
| evict，沒改過 | 直接丟（下層一定有最新的） |
| evict，改過（M） | 寫回下層（L2）。不知道別的 cache 有沒有一份，乾脆直接寫回 |
| 從 bus 收到別人的寫（在 S 或 M） | 更新自己的值，這就是「update」 |

**sister cache**（姊妹 cache）：同一層的其他 cache。【口頭】[49:31] 讀 miss 時去問下層的 L2 是**錯的**，因為最新的值可能在另一個 L1 裡。

**狀態圖**（處理器的動作）：

```
I ──read miss（廣播）──→ S ──read hit──→ S
S ──evict──→ I
S ──write hit（廣播寫入）──→ M ──read hit／write hit（廣播寫入）──→ M
M ──evict（寫回 L2）──→ I
I ──write miss（廣播，先拿到整個 block 再寫）──→ M
```

**write miss 為什麼要先拿整個 block**（【口頭】[56:04]，「學生最常搞錯的地方」）：一次 store 只寫 4 bytes，但 block 是 64 bytes。不能只寫 4 bytes，否則要記錄哪幾個 byte 寫過，非常複雜。所以先從姊妹 cache 或下層拿到**舊的整個 block**，再改那 4 bytes（[第 35 支 §11](lecture35_note.md#11-data-write一定要先有整個-block)）。

**問題**（【口頭】[59:14]）：每次寫都要廣播資料，程式寫很多次就會有**大量 bus 流量**；bus 是唯一的共用資源，可能塞住、變慢。

⚠️ [62:21] 老師說投影片上把「processor」和「cache」混著用（因為一個處理器對一個 L1），之後的版本會統一改成 cache。coherence 其實可以做在任何一層（例如 private L2 之間），只是通常不這樣做。

---

## 10. Write-invalidate 協定（MSI）

[60:16]–[73:29]　課本 Fig 12.13–12.14（p.600–603）

**關鍵想法**：如果某個 cache 的 block 在 **M** 狀態，**其他 cache 都沒有**這個 block 的有效複本。所以在 M 狀態寫入**不用廣播**。

**處理器的動作**（Fig 12.13）：

| 從 | 事件 | 到 | bus 訊息 |
|---|---|---|---|
| I | read miss | S | read miss；姊妹 cache 有就給 |
| S | read hit | S | — |
| S | evict | I | —（沒改過，直接丟） |
| S | write hit | **M** | **write hit**：向別人「申請寫入權」，別人全部變 I |
| I | write miss | **M** | **write miss**：別人變 I，有的給我 block，我再寫 |
| M | read hit／write hit | M | **不送任何訊息** |
| M | evict | I | 寫回下層 |

**從 bus 收到的事件**（Fig 12.14）：

| 狀態 | 收到 | 動作 |
|---|---|---|
| S | read miss | 送 block 給它（誰先拿到 bus 誰送） |
| S | write hit | 變 I（對方有 block，只是在 S，沒有寫入權） |
| S | write miss | 變 I，送 block 給它 |
| M | read miss | 送 block，**寫回 L2**，變 **S** |
| M | write miss | 送 block，變 I |
| M | write hit | **不可能**：M 的時候別人不可能有複本 |

**S 的意思**（【口頭】[67:47]）：同一個變數可以在很多個 cache 裡，**唯讀**模式：可以讀、不能寫。要寫一定要先變 M。

**為什麼 M 收到 read miss 要變 S、還要寫回**（【口頭】[70:21]）：
- 同一個 block 不能在一個 cache 是 M、另一個是 S，所以自己也要降成 S。
- 從 S 要 evict 時是直接丟、不寫回；所以變成 S 之前，先把在 M 時寫的新資料寫回 L2，保證下層是最新的。

**小結**（【口頭】[72:28]）：write-update 每次寫都廣播，流量大、耗電；MSI 好很多，在 M 的時候可以一直寫，不受干擾。

---

## 11. 為什麼 MSI 遵守 coherence

[73:29]–[76:23]

**Completion**：要寫，就要拿到 bus、在 bus 上送 write hit 或 write miss，送出去就差不多完成了。只要 bus 是**公平**的（不會讓某個請求永遠等），寫入一定會完成。

**Order**：考慮 x 所在的 line。任何要寫它的請求，都要先讓它在自己的 cache 變成 **M**；同一時間只有一個 cache 能是 M。所以只要追蹤「各個 cache 依序在什麼時候拿到 M」，就是對 x 的**寫入順序**。進入 M 要靠 bus 上的訊息，bus 是中央的資源 → 這個順序**所有處理器都看得到、都一樣**。

【口頭】[76:04] 兩個都是很基本的 MSI 協定：write-update 簡單，但訊息多、耗電；write-invalidate 稍微複雜一點（也沒多複雜），訊息少很多，所以**比較常用**。

---

## 12. Directory 協定（一點點）

[76:23]–[77:37]　課本 Definition 129（p.599–600）

snoopy 的問題：
- bus 上所有訊息（write hit、write miss、read miss）都是**廣播**，很貴。
- 32 或 64 個處理器共用一條 bus，bus 會變成瓶頸。

觀察：大部分程式裡，一個 block 只被**少數幾個** cache 存著。

**Directory 協定**：一個專門的結構叫 **directory**，記錄每個 block 的**所有 sharer**（有這個 block 的 cache）。狀態改變時，directory 只送訊息給那些 sharer，sharer 名單動態更新 → **完全不用廣播**，效率高很多。「這比較適合在進階課程教。」

【課本補充】p.599–600：directory 版的 write-update：cache 把訊息都送給 directory；read／write miss 時 directory 從有複本的姊妹 cache 拿 block 轉給請求者；寫入只送給 sharer；cache insert 或 evict block 時要更新 sharer 名單。directory 也要保證所有 cache 收到訊息的順序一樣、沒有訊息遺失。

【口頭】[78:06] 總結：MIMD 多處理器最難的部分都做完了：coherence、consistency、SC、WC、write-update、write-invalidate。「這些其實都是進階的概念」，但從 2010 年左右多核心時代就開始了，「現在沒有一個大學課程能不教平行處理器怎麼運作」。接下來比較簡單：multithreading、vector processor、interconnect。

---

## 13. 【實測】

程式在 `experiments/lecture40/`。

### 13.1 Example 154：用 fence 自己寫平行加總

`sum_fence.c`：照課本 Example 154，用 pthreads 代替 `parallel`，`fence()` 用 C11 的 `atomic_thread_fence(memory_order_seq_cst)`；不用 `pthread_join`，主 thread 自己轉圈等 finished[]（就是課本的 do-while）。`experiments/lecture40/sum_fence.c:14-21`：

```c
static void *worker(void *arg)
{
	int myId = (int)(long)arg;                       /* getThreadId() */
	int startIdx = myId * (SIZE / N), endIdx = startIdx + SIZE / N;
	for (int jdx = startIdx; jdx < endIdx; jdx++) partialSums[myId] += numbers[jdx];
	fence();                                          /* 確保 partialSums[myId] 寫完才設 finished */
	finished[myId] = 1;
	return NULL;
}
```

```
$ gcc -O2 -pthread -o sum_fence sum_fence.c && ./sum_fence
result = 234881024 (expected 234881024)
$ objdump -d sum_fence | grep lock
    10dc:	lock orq $0x0,(%rsp)
    1132:	lock orq $0x0,(%rsp)
    12ca:	lock orq $0x0,(%rsp)
```

- 答案正確。
- gcc 把 fence 編成 **`lock orq $0, (%rsp)`**：對 stack 頂端做一個「加 lock 前綴、什麼都不改」的運算。這就是影片說的「x86 上帶 lock 前綴的指令就是 fence」，和 Linux 的 `smp_mb()`（`lock addl`，[第 39 支 §16](lecture39_note.md#16-原始碼linux-的-memory-barrier)）同一招。
- 我多加了第三個 fence（主 thread 看到 finished 之後、讀 partialSums 之前）。課本沒有，但在真正很弱的機器上，讀和讀也可能被調換，加上比較保險。⚠️ 在 x86 上拿掉 fence ② 也不會出錯（x86 不調換寫寫，[第 39 支 §17.2](lecture39_note.md#172-寫寫讀讀x86-沒有調換)），所以這裡沒辦法示範「放錯位置就錯」。

### 13.2 一條 cache line 在核心之間搬一次要多久

`pingpong.c`：兩個 thread 輪流寫**同一條** cache line（A 看到 0 就寫 1，B 看到 1 就寫 0），200 萬次。每次換手，那條 line 都要從一個核心的 cache 變成 M、搬到另一個核心（write-invalidate 的 write miss）。

```
cpu2  <-> cpu3  :   32.3 ns per round trip    ← 同一個 P-core 的兩個 hardware thread（共用 L1）
cpu2  <-> cpu4  :   77.7 ns per round trip    ← 兩個不同的 P-core
cpu2  <-> cpu12 :   71.8 ns per round trip    ← P-core 和 E-core
cpu12 <-> cpu13 :  127.3 ns per round trip    ← 同一組的兩個 E-core
cpu12 <-> cpu16 :   89.9 ns per round trip    ← 不同組的兩個 E-core
```

- 同一個核心的兩個 thread 共用 L1，不用 coherence 搬運，最快（32 ns 一個來回）。
- **不同核心之間一個來回約 70–90 ns**（一次換手約 35–45 ns），比 L1 hit（約 1 ns）慢幾十倍。這就是 private cache 要付的 coherence 代價，也是[第 38 支 false sharing](lecture38_note.md#144-false-sharing-的影子) 變慢的原因。
- ⚠️ 同一組 E-core（cpu12、cpu13，共用 L2）反而最慢（127 ns），為什麼「待補」。

### 13.3 MSI 模擬器：誰的 bus 訊息比較少

`msi.py`：4 個 cache、1 個 block，照 §9、§10 的規則各模擬一次，數 bus 上的訊息數：

```
$ python3 -I msi.py
一個處理器一直寫（1000 次）               write-update  1000 則訊息   write-invalidate     1 則
一個寫、一個讀，輪流（各 500 次）            write-update   501 則訊息   write-invalidate  1000 則
4 個處理器隨機讀寫（各 1000 次）           write-update  1336 則訊息   write-invalidate  2466 則
update    ：2667 次讀，每次都讀到最後一次寫的值 → True
invalidate：2667 次讀，每次都讀到最後一次寫的值 → True
```

- **一個處理器一直寫**：write-invalidate 只要 1 則（第一次寫變成 M，之後都不用送）；write-update 每次都廣播，1000 則。這就是影片說 MSI 比較好的情況，也是最常見的情況（大部分資料只有一個 thread 在寫）。
- ⚠️ **一個寫、一個讀，輪流**：反過來了。write-invalidate 每次寫都要把讀者變 I、讀者每次讀都 miss，1000 則；write-update 只要第一次 read miss，之後每次寫順便更新讀者，501 則。所以「write-update 流量比較大」不是絕對，看共享的模式；而且 write-update 的每則訊息都帶資料，比 invalidate 的訊息大。
- 兩個協定在 2667 次讀裡**每次都讀到最後一次寫的值**，就像只有一個大 cache 一樣（§6）。

---

## 14. 一頁速查表

| 主題 | 重點 |
|---|---|
| WC 平行加總 | fence ① 初始化後、fence ② partialSums 和 finished 之間 |
| fence 放錯 | finished 在 fence 前 → 別人可能讀到舊的 partialSums |
| x86 的 fence | lock 前綴的指令（gcc：`lock orq $0,(%rsp)`） |
| 誰有 SC | 只有 MIPS R10000；x86、ARM 都是某種 weak model |
| Shared cache | 每個位置一份；大、慢、遠、port 多；適合 L2 以下 |
| Private cache | 小、快、近；有複本，要 cache coherence；適合 L1 |
| Bank | 4 MB 切 16 個 bank，第 19–22 位選 bank；請求分散到不同 bank 平行處理 |
| LLC | 晶片上最低一層 cache |
| 關鍵 | 寫入要全域順序 → 用獨占的 bus |
| Snoopy | 所有 L1 接一條 bus，廣播、同一順序、不遺失 |
| 狀態 | M（改過）、S（有、沒改）、I（無效） |
| Write-update | 每次寫都廣播資料，大家更新 |
| Write-invalidate（MSI） | 寫之前別人全變 I；在 M 讀寫都不送訊息；M 收到 read miss → 寫回、變 S |
| write miss | 先拿整個 block 再寫 |
| MSI 正確性 | completion：bus 公平；order：進入 M 的順序 = 寫入順序 |
| Directory | 記錄每個 block 的 sharer，只送給它們，不用廣播 |
| 實測 | fence 版加總正確；跨核心 line 來回 70–90 ns；MSI 不一定訊息少，看共享模式 |

---

## 15. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| Multithreaded processor（一個核心兩個 hardware thread，共用 L1） | [第 41 支](lecture41_note.md)：Multithreading and Vector Processing |
| Interconnection network（bus 不夠用時，用什麼把 bank 和核心接起來） | [第 42 支](lecture42_note.md)：GPUs and Interconnects |
| SC、WC、fence 的定義 | [第 39 支](lecture39_note.md) |
| cache 的 modified bit、write-back、整個 block 才能寫 | [第 35 支](lecture35_note.md) |
| MESI、MOESI、directory 的細節、記憶體模型的正式定義 | [Advanced Computer Architecture](../advanced/README.md) |

---

## 附：本講一句話總結

> **在 weak consistency 機器上寫平行加總，只要比 SC 版多兩條 fence：一條在初始化之後、開 thread 之前，一條在寫完 partialSums 和設 finished 之間（放錯就會讀到舊值），x86 上任何帶 lock 前綴的指令都能當 fence；實體上 shared cache 每個位置只有一份、簡單但大而慢、要很多 port（切成 bank 可以緩解），適合 L2 以下，L1 則要用小而快的 private cache，代價是要用 cache coherence 讓它們看起來像一個 cache；snoopy 協定讓所有 L1 共用一條同一時間只有一人能寫的 bus，用取得 bus 的順序定出寫入的全域順序，write-update 每次寫都廣播資料，write-invalidate（MSI）則在寫之前讓別人都變 I、在 M 狀態寫不用送訊息、M 收到 read miss 就寫回並降成 S，bus 公平保證 completion、「誰依序拿到 M」保證 order；處理器多了 bus 會塞，directory 協定記錄每個 block 的 sharer 只送給它們。實測 fence 版加總正確且 gcc 把 fence 編成 lock orq；同一條 cache line 在不同核心之間來回一次要 70–90 ns，同一核心的兩個 thread 只要 32 ns；模擬顯示一個處理器一直寫時 MSI 只要 1 則訊息（update 要 1000 則），但一寫一讀輪流時反而是 update 比較省。**

---

*筆記依據 Smruti R. Sarangi「Basic Computer Architecture」Chapt 11: Cache Coherence Protocols, Part 3/5 錄影逐字稿（YouTube 自動字幕，1:19:05）、Sarangi *Basic Computer Architecture* v3.09 Ch12 §12.4.3 結尾與 §12.4.4–12.4.6（p.590–603）整理。實測為本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4、Python 3.10）結果。*
