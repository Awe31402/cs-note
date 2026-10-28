# Basic Computer Architecture 第三十六支影片完整筆記：記憶體系統的數學模型、三種 miss 與降低 miss 的方法（Chapter 10 Memory Systems, Part 3/4 → 課本 Ch11）

> **課程**：Basic Computer Architecture — Chapt 10: Memory Systems, Part 3/4（播放清單第 36 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：YouTube 錄影逐字稿（55:33，自動字幕）＋ 課本 *Basic Computer Architecture* v3.09 **Ch11** §11.3（p.532–539）
> **章號**：影片的 Chapter 10 是課本 v3.09 的 **Chapter 11**。
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄（本講沒有）
> - `【實測】`：在本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4、Python 3.10、valgrind）實際執行的結果
> - `⚠️`：逐字稿缺字、或影片／課本／實測說法不一致之處
> - `[mm:ss]`：影片時間點

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. 有 cache miss 的 CPI](#1-有-cache-miss-的-cpi)
- [2. 平均記憶體存取時間 AMAT](#2-平均記憶體存取時間-amat)
- [3. Local miss rate 與 global miss rate](#3-local-miss-rate-與-global-miss-rate)
- [4. Working set](#4-working-set)
- [5. 三種 miss：三個 C](#5-三種-miss三個-c)
- [6. 降低 compulsory miss：大 block、prefetch](#6-降低-compulsory-miss大-blockprefetch)
- [7. 降低 conflict miss：提高 associativity、victim cache](#7-降低-conflict-miss提高-associativityvictim-cache)
- [8. 降低 capacity miss：大 cache、prefetch](#8-降低-capacity-miss大-cacheprefetch)
- [9. 兩個經驗法則](#9-兩個經驗法則)
- [10. Software prefetching](#10-software-prefetching)
- [11. Hardware prefetching](#11-hardware-prefetching)
- [12. 降低 hit time](#12-降低-hit-time)
- [13. 降低 miss penalty：write buffer](#13-降低-miss-penaltywrite-buffer)
- [14. 降低 miss penalty：critical word first 與 early restart](#14-降低-miss-penaltycritical-word-first-與-early-restart)
- [15. 所有技巧總整理](#15-所有技巧總整理)
- [16. 【實測】](#16-實測)
- [17. 一頁速查表](#17-一頁速查表)
- [18. 與後續講次的連結](#18-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

```
CPI = CPI_ideal + f_mem × (AMAT − 1)
AMAT = L1 hit time + L1 miss rate × (L2 hit time + L2 miss rate × L2 miss penalty)
三種 miss：
  compulsory（cold）→ 大 block、prefetch
  conflict           → 提高 associativity、victim cache、cache-friendly 的程式
  capacity           → 大 cache、prefetch
經驗法則：miss rate ∝ 1/√size；associativity ×2 ≈ size ×2
hit time → 小而簡單的 cache
miss penalty → write buffer（寫）、critical word first + early restart（讀）
```

---

## 1. 有 cache miss 的 CPI

[00:00]–[06:45]　課本 §11.3.1（p.533，Eq 11.1）

上一章的公式（[第 32 支](lecture32_note.md#15-真實-pipelinecpi--cpi_ideal--stall-rate--stall-penalty)）：

```
CPI = CPI_ideal + stall rate × stall cycles
```

- **CPI_ideal**：記憶體**理想**時的 CPI（每次存取 1 週期、每次都 hit）。可以包含其他 pipeline stall，唯一假設是記憶體 1 週期。
- 【口頭】例子：CPI_ideal = 1，每 10 條指令有 1 條 stall，每次平均浪費 3 週期 → CPI = 1 + 3 × 1/10 = **1.3**。

假設**每條記憶體指令都可能 stall**，就可以換成：

```
CPI = CPI_ideal + f_mem × (AMAT − 1)
```

- **f_mem**：記憶體指令（load／store）占的比例。
- **AMAT**（average memory access time）：平均記憶體存取時間（週期）。
- **為什麼減 1**：理想情況下每次存取本來就要 1 週期，這 1 週期已經算在 CPI_ideal 裡，不要重複算。

【口頭】[03:02] 例子：AMAT = 1.1 週期 → 每條記憶體指令平均多 0.1 週期；30% 是記憶體指令 → CPI = CPI_ideal + 0.3 × 0.1。

結論：記憶體指令越多、AMAT 越大，CPI 越大、效能越差。AMAT 會變大，是因為 miss 多了，要往記憶體階層下面去拿。

【課本補充】p.533：
- pipeline 在 **MA** 級接到記憶體系統；回應沒在 1 週期內回來，5 級 in-order pipeline 就要插 bubble。存取要 n 週期就有 n − 1 個 stall 週期。
- 實際上大部分指令在 i-cache、d-cache 都 hit（1 週期），不會 stall；長 stall 來自 L1、L2 miss。公式還是成立，因為我們要的是很多指令的**平均** CPI。
- p.532：最上層有兩個 cache：**i-cache**（指令）和 **d-cache**（資料），存取方式一樣，下面統稱 L1；L2 通常指令和資料**共用**。
- Definition 105：**physical address space**（實體位址空間）= cache 和主記憶體裡所有記憶體位置的集合。inclusive 時，記憶體系統的總容量 = 主記憶體的大小（cache 只是子集，不增加容量）。

---

## 2. 平均記憶體存取時間 AMAT

[06:45]–[12:02]　課本 Eq 11.2–11.3（p.534）

```
AMAT = L1 hit time + L1 miss rate × L1 miss penalty
L1 miss penalty = L2 hit time + L2 miss rate × L2 miss penalty
L2 miss penalty = L3 hit time + L3 miss rate × L3 miss penalty
...
L(n−1) miss penalty = Ln hit time
```

- **每一次**存取都要經過 L1，所以不管 hit 或 miss 都要付 **L1 hit time**（要花時間才知道是 hit 還是 miss）。
- 一部分（L1 miss rate）miss，要付 **L1 miss penalty**：往下走、拿回來、寫回 L1 所需的週期。
- L2 也一樣：不管 hit 或 miss 都付 L2 hit time，再加上 L2 miss 的部分。
- 最後一層是主記憶體，包含所有 line，**miss rate = 0**，所以倒數第二層的 miss penalty 就是最後一層的 hit time。

知道每一層的 miss rate 和 hit time，就能算 AMAT。

【口頭】[09:02]–[10:26] 算 stall penalty 時要用 **AMAT − L1 hit time**（也就是 L1 miss rate × L1 miss penalty），因為 L1 hit time 已經算在 CPI_ideal 裡了，「它不是 penalty」。

---

## 3. Local miss rate 與 global miss rate

[12:02]–[13:50]　課本 Definition 106（p.534）

| | 定義 |
|---|---|
| **local miss rate** | 第 i 層的 miss 數 ÷ **到達第 i 層**的存取數 |
| **global miss rate** | 第 i 層的 miss 數 ÷ **全部**記憶體存取數 |

【口頭】[12:40] 例子：100 次存取到 L1，90 次 hit、10 次到 L2；L2 有 5 次 miss 到主記憶體。

```
L2 local miss rate  = 5 / 10  = 50%
L2 global miss rate = 5 / 100 = 5%
```

AMAT 公式用的是 **local** miss rate，它比較重要；但 global miss rate 這個詞也要知道。

---

## 4. Working set

[13:50]–[16:05]　課本 Definition 107（p.535）

【口頭】程式通常是：一段 for／while 迴圈跑很多圈 → 換到另一段 → 又跑很多圈……每一段存取一個或一組陣列。

**Working set**（工作集）：在一小段時間裡，程式存取的記憶體量。

【口頭】[15:01] 例子：一個程式一段時間存取 2 MB，接著一段時間存取 4 MB（迴圈掃過整個 4 MB），再回到 2 MB → working set 依序是 2 MB、4 MB、2 MB。「這是個大概的定義，沒有正式的」。

我們希望 **working set 放得進 cache**，這樣程式要的資料大多在 cache 裡。

【課本補充】p.535：時間區間的長度有點主觀，但意思是：比整個執行時間短很多，又長到系統行為已經穩定。

---

## 5. 三種 miss：三個 C

[16:05]–[19:34]　課本 §11.3.2（p.535）

三種的英文都是 C 開頭，叫 **three C's**：

| 種類 | 何時發生 | 例子 |
|---|---|---|
| **Compulsory**（強制，也叫 **cold**） | 第一次讀某個資料 | 程式剛啟動，cache 裡什麼都沒有；「程式剛開始時是冷的」 |
| **Conflict**（衝突） | associativity 有限 | 4-way cache，程式常存取 5 個對到**同一個 set** 的 line → 一直互相擠掉 |
| **Capacity**（容量） | working set 比 cache 大 | working set 10 KB、cache 8 KB：對一個 10 KB 陣列做很多運算，整個放不下 |

【課本補充】p.535：
- capacity 的例子：一直存取 1 MB 的陣列，L2 只有 512 KB。
- conflict miss 只在 direct mapped 和 set associative cache 裡發生（fully associative 沒有）。

---

## 6. 降低 compulsory miss：大 block、prefetch

[19:34]–[23:10]　課本 p.535–536，Definition 108

**① 加大 block**：64 B 改成 128 B，一次從下層拿更多資料。因為空間局部性，接下來的存取更可能落在同一個 block。【口頭】例子：block 64 B 時，用完這 64 B 接著要下一個 64 B → miss；block 128 B 時兩塊一起來了 → 不會 miss。

**② Prefetching**（預取）：一個小電路**猜**不久後會存取哪些位置，**事先**從下層抓上來。
- 【口頭】[21:25] 例子：bubble sort 依序存取陣列元素，電路看出「位址一直往上加」，就先把後面幾個抓上來。
- prefetcher 看著進到 L1 的存取，自己對 L2 發請求，把資料送進 L1。

【課本補充】p.535–536：
- block 太大也有壞處：cache 裡能放的 block 變少、額外好處變小、從下層讀和傳更大的 block 要更久。**32–128 bytes** 都算合理。
- 存取的 index 差一個固定值（例如每次跳 4 個元素）也猜得出來。
- **Definition 108**：**hardware prefetcher** 是專門預測不久後的存取、從下層抓資料的硬體單元。大部分現代處理器都有。
- prefetcher **不能太積極**，不然趕出去的有用資料比抓進來的還多。

---

## 7. 降低 conflict miss：提高 associativity、victim cache

[23:10]–[31:46]　課本 p.536

**① 提高 associativity**：4-way 改 8-way。【口頭】「說比做容易」：way 越多，比較越多 → 越耗電、越慢。

**② Victim cache**（受害者 cache）：一個**小的 fully associative cache**，放被主 cache 趕出來的 line。

```
            ┌── L1 ──┐
處理器 ───→ │        │ ──→ L2
            └ victim ┘   （L1 趕出來的 line 先放這裡；victim 滿了才送去 L2）
```

讀的時候：
1. L1 hit → 直接給處理器。
2. L1 miss、victim hit → 從 victim 拿給處理器。（L1 和 victim 可以平行查，也可以先 L1 再 victim。）
3. 兩個都 miss → 去 L2。

**為什麼這麼小卻有用**（【口頭】[27:11]–[31:27]）：
- 例子：L1 8 KB、L2 128 KB，victim cache **8 個 entry** × 64 B = **512 B**，只有半 KB。
- 2-way、64 個 set 的 cache，理想上每個 set 被存取的次數差不多；但實際上有些「倒楣的」set 存取特別多 → 衝突特別多。
- victim cache 等於**替這些熱門的 set 加大了容量**：常用但因為 associativity 不夠而被趕出去的 line，有了另一個家。
- 存取時間：L1 1 週期，victim 大約多 2 週期；L2 要 10–15 週期 → 還是快很多。
- 【口頭】「這就是為什麼 victim cache 很受歡迎，是現代處理器很常見的東西。」文獻裡有很多演算法決定哪些 line 該放進 victim cache。

**③ Cache-friendly 的程式**（【口頭】[25:09]）：主要是**編譯器**的事：配置資料結構時讓存取平均分到所有 set，沒有哪個 set 壓力特別大。

【課本補充】p.536：課本的流程是先查主 cache，miss 了再查 victim cache，再往下。「第 i 層的 victim cache 可以濾掉一部分要去第 i+1 層的請求。」

---

## 8. 降低 capacity miss：大 cache、prefetch

[31:46]–[33:37]　課本 p.536

- **加大 cache**：最簡單，但「說比做容易」，大 cache 比較慢、比較耗電。
- **更好的 prefetching**：預測並抓好將來要的資料，減輕 cache 小的影響。【口頭】prefetching 本身不在這門課範圍，屬於進階課程。

【課本補充】p.536：照課本的設計，cache 大小要是 **2 的次方**，所以加大 cache 至少是**加倍**：面積兩倍、更慢、更耗電。（有進階技術可以打破這個規則。）

---

## 9. 兩個經驗法則

[33:37]–[36:05]　課本 Eq 11.4（p.537）

【口頭】經驗法則（rule of thumb）：大家觀察到、在用，但**沒有理論證明**。

**Square Root Rule**（平方根法則）：

```
miss rate ∝ 1 / √(cache size)
```

【口頭】cache 變 **4 倍**，miss rate 變 **一半**。例如 miss rate 10% 想降到 5%，cache 要大約 4 倍，「可能 3.5 倍、可能 5.5 倍」。

**Associativity Rule**（關聯度法則）：associativity 加倍 ≈ 保持原 associativity、cache 大小加倍。例：**64 KB 4-way** 的 miss rate 和 **128 KB 2-way** 差不多。

【課本補充】p.537：
- Square Root Rule 出自 Hartstein 等人（2006），他們用機率論試著解釋；實驗得到的指數在 **−0.3 到 −0.7** 之間。
- 兩條都只是概念上的輔助，**不精確成立**，一定找得到違反的例子。

---

## 10. Software prefetching

[36:05]–[44:21]

【口頭】prefetching 有兩種：硬體電路猜（hardware），或在 C 程式裡**自己**提早抓（software）。

影片的例子（我依口述重寫）：

```c
long addAll(int data[], int vals[]) {
    long sum = 0;
    for (int i = 0; i < N; i++)
        sum += data[vals[i]];
    return sum;
}
```

- vals 陣列是**依序**存取，有空間局部性。
- 但 data 的 index 來自 vals[i]，例如 vals[0] = 10、vals[1] = 32、vals[2] = 120……位址「完全隨機」→ data **既沒有時間也沒有空間局部性**，cache miss 很多。
- 這種「index 本身是陣列元素」的**雙重間接**叫 **irregular array access**（不規則陣列存取），在科學計算程式裡非常常見，cache 表現很差。

加一行就好（gcc 在 Linux 上；Mac 上的 gcc 老師沒試過）：

```c
    for (int i = 0; i < N; i++) {
        __builtin_prefetch(&data[vals[i + 100]]);   // 提早 100 圈抓
        sum += data[vals[i]];
    }
```

- `__builtin_prefetch(位址)`：把那個位址的資料抓到最上層（L1）。
- 位址**就算無效也沒關係**：不會 segmentation fault，不會讓程式當掉。所以 i 接近 N 時 vals[i + 100] 超出範圍也安全。
- 多了一條指令有點成本，但之後真的存取時，資料已經在上層了。
- 【口頭】[43:10]「你會發現效能提升 **200% 到 300%**」；「有些實驗顯示效能甚至可以變成 3 倍」。
- 這個模式**硬體很難猜**：硬體要猜出 data 的 index 來自另一個陣列 vals。有研究在做，但軟體做比較簡單直覺。

⚠️ 老師說「書裡一定也有這些程式」；但課本 v3.09 的 §11.3 只說編譯器可以插入 prefetch 程式碼，「超出本書範圍」，全書都找不到 `__builtin_prefetch`。上面的程式是依影片口述重寫的。實測結果見 [§16.1](#161-software-prefetch-真的有用嗎)。

---

## 11. Hardware prefetching

[44:21]–[45:25]

hardware prefetcher 可以看兩種資訊：
- 到 L1 的**所有存取**，或
- L1 的 **miss**（比較常見）。

根據這些決定處理器接下來最可能存取哪些位址，向 L2 發請求，把 line 先搬進 L1，等處理器要時已經準備好。

**存取模式可預測時**硬體 prefetcher 有用；不可預測時就要靠 software prefetching。

---

## 12. 降低 hit time

[45:25]–[45:59]　課本 §11.3.3（p.537）

用**小而簡單**的 cache。缺點：hit rate 也會下降。要權衡。

---

## 13. 降低 miss penalty：write buffer

[45:59]–[49:13]　課本 §11.3.3（p.537–538，Fig 11.16）

問題：L1 **寫 miss** 時，整個 block 要從 L2 搬上來（> 10 週期），寫完之前 pipeline 不能繼續。

解法：**write buffer**（寫入緩衝區），很小（**4 或 8 個 entry**），放在 L1 後面（也可以在處理器和 L1 之間，概念一樣）。

【口頭】[47:20] 放大一個 64-byte block 來看：
1. 處理器要寫中間某 4 bytes → L1 miss → 在 write buffer 配一個 entry，寫進這 4 bytes，**處理器繼續往下跑**。
2. 又來一個寫同一個 block 的另外 4 bytes → 寫進同一個 entry。
3. L2 的 block 終於到了 → 已經寫過的那兩塊保留，**其他部分**用 L2 來的資料補上。
4. 之後的讀要**同時查** write buffer 和 L1。

天真的做法是等 L2 的資料搬到 L1 再寫，「這要花時間」。

⚠️ write buffer 的結構，兩邊說法不同：影片 [46:43] 說「**fully associative** write buffer」；課本 p.537 說是「small **set associative** cache」。

【課本補充】p.537–538：
- 處理器可以每次寫都寫進 write buffer，也可以只在 L1 miss 時才寫進去（課本假設後者）。
- 資料到了 L1，對應的 entry 就可以移除。
- write buffer **沒有空位**時，pipeline 還是要 stall。

---

## 14. 降低 miss penalty：critical word first 與 early restart

[49:13]–[52:01]　課本 p.538

這兩個通常**一起用**，處理**讀 miss**。

觀察：block 是 64 bytes，但處理器讀一個 int 只要 **4 bytes**。

【口頭】[50:01] L2 和 L1 之間是銅線，一次大約只能傳 **16 bytes**，64 bytes 要 **4 個週期**才傳完。

- **Critical word first**：把處理器要的那個字（所在的 16-byte 小塊）**最先**傳。
- **Early restart**：那一塊一到 L1，**馬上**把 4 bytes 給處理器，處理器重新開始執行，不用等整個 block 到齊。

做法是**改變**從下層傳上來的 byte 順序：不是照順序傳，而是先傳要的那部分。

【課本補充】p.538：兩者都會增加記憶體系統的複雜度，但「相當有效」。

---

## 15. 所有技巧總整理

[52:01]–[55:20]　課本 Table 11.2（p.539）

| 技巧 | 針對 | 缺點 |
|---|---|---|
| 大 block | compulsory | cache 裡的 block 變少 |
| prefetching（軟體＋硬體） | compulsory、capacity（其實各種都有用） | 多了複雜度；可能趕走有用的資料 |
| 大 cache | capacity | 延遲高、功耗高、面積大 |
| 提高 associativity | conflict | 延遲高、功耗高（要比更多 tag） |
| victim cache | conflict | 多一點複雜度 |
| 編譯器技巧（重排程式碼） | 各種 miss | 不太通用 |
| 小而簡單的 cache | hit time | miss rate 高 |
| write buffer | miss penalty（寫） | 多一點複雜度 |
| critical word first | miss penalty（讀） | 多了複雜度和狀態 |
| early restart | miss penalty（讀） | 多了複雜度 |

【口頭】[53:16]「victim cache **幾乎總是**好主意」：很小、額外成本很低；效果看程式而定，主要在某些 set 流量特別大時減少 conflict miss。編譯器重排程式碼「超出這本書範圍，不通用，但極度有效」。

【課本補充】p.538：每個技巧都有副作用，改善一面就會傷到另一面。設計記憶體系統要在目標工作負載的需求、設計限制和製造技術之間仔細權衡，同時注意功耗、面積和複雜度。

下一講：virtual memory。

---

## 16. 【實測】

程式在 `experiments/lecture36/`。trace 是[第 34 支](lecture34_note.md#142-真實程式的-stack-distance-與-address-distance)用 valgrind 錄的 `sort`、`gzip`（各前 200 萬行），模擬一律用 64-byte block、LRU。

### 16.1 Software prefetch 真的有用嗎

`prefetch.c` 照影片的 addAll 寫：data、vals 各 3200 萬個 int（各 128 MB，遠大於 24 MB 的 L3），vals 是隨機 index。比較兩個版本：

- `addall_indep`：就是影片的程式，每一圈的 load 彼此無關。
- `addall_chain`：index 多 XOR 一個 `(sum & m)`，m 在執行時才知道是 0。結果一樣，但**每個 load 都要等上一個 load 的值回來**，就像這門課的 in-order pipeline：一個 miss 沒回來，後面就不能動。

`experiments/lecture36/prefetch.c:23-31`：

```c
static long addall_chain(int dist)
{
	long sum = 0, m = zero;
	for (int i = 0; i < N; i++) {
		if (dist) __builtin_prefetch(&data[vals[i + dist < N ? i + dist : i]]);
		sum += data[vals[i] ^ (sum & m)];
	}
	return sum;
}
```

```
$ gcc -O2 -o prefetch prefetch.c && taskset -c 2 ./prefetch
-- independent loads --
prefetch distance    0 :   228.0 ms    6.8 ns/elem  (1.00x)
prefetch distance    1 :   257.0 ms    7.7 ns/elem  (0.89x)
prefetch distance   16 :   223.2 ms    6.7 ns/elem  (1.02x)
prefetch distance  100 :   221.9 ms    6.6 ns/elem  (1.03x)
prefetch distance 1024 :   257.6 ms    7.7 ns/elem  (0.89x)
-- each load waits for the previous one --
prefetch distance    0 :  3125.0 ms   93.1 ns/elem  (1.00x)
prefetch distance    1 :   272.7 ms    8.1 ns/elem  (11.46x)
prefetch distance   16 :   223.1 ms    6.6 ns/elem  (14.01x)
prefetch distance  100 :   226.2 ms    6.7 ns/elem  (13.82x)
prefetch distance 1024 :   358.6 ms   10.7 ns/elem  (8.71x)
```

- **每個 load 要等前一個**時：沒 prefetch 每個元素 **93 ns**，正好是一次 DRAM 延遲（[第 20 支](lecture20_note.md#13-實測)量到約 80–95 ns）。加上 prefetch（提早 100 圈）變成 6.7 ns，快了 **13.8 倍**，比影片說的 2–3 倍還多。
- **影片原本的程式**（load 互不相干）：沒 prefetch 就只要 6.8 ns，prefetch **幾乎沒用**，甚至有時慢一點（多了指令）。
- 原因：本機是 **out-of-order**（亂序執行）的 CPU，它自己就會先往後跑、同時發出很多個 miss，等於硬體幫你做了 prefetch。影片的 2015 年說法比較適合簡單的 in-order 處理器。
- prefetch 距離太遠（1024）效果變差：抓進來的資料還沒用到就可能被擠出去了。

⚠️ 即使「每個 load 等前一個」，distance = 1 也快了 11 倍：prefetch 指令本身**不等**結果，CPU 可以先跑到很多圈後面，把一大堆 prefetch 同時發出去。

### 16.2 把 miss 分成三個 C

`threec.py`：compulsory = 第一次碰到的 block；capacity = 同大小 **fully associative** LRU 還會 miss 的（扣掉 compulsory）；conflict = 真正的 cache 比 fully associative 多出來的 miss。8 KB cache：

| | sort 總 miss | compulsory | capacity | conflict | gzip 總 miss | compulsory | capacity | conflict |
|---|---|---|---|---|---|---|---|---|
| direct mapped | 1.87% | 0.34% | 0.29% | **1.24%** | 28.18% | 0.28% | **25.95%** | 1.94% |
| 2-way | 1.23% | 0.34% | 0.29% | 0.60% | 26.94% | 0.28% | 25.95% | 0.70% |
| 4-way | 0.73% | 0.34% | 0.29% | 0.10% | 26.60% | 0.28% | 25.95% | 0.36% |
| fully assoc. | 0.63% | 0.34% | 0.29% | 0 | 26.24% | 0.28% | 25.95% | 0 |

- **sort** 用 direct mapped 時，三分之二的 miss 是 **conflict**；提高 associativity 就幾乎消失。
- **gzip** 幾乎全是 **capacity**：它的 working set 比 8 KB 大很多，提高 associativity 沒用，要加大 cache。
- 兩個程式的 compulsory 都很少（只有幾千個不同的 block）。

### 16.3 Victim cache

8 KB direct mapped，加上一個小的 fully associative victim cache：

| | 沒有 | 4 entry（256 B） | 8 entry（512 B） | 16 entry（1 KB） | 對照：2-way |
|---|---|---|---|---|---|
| sort | 1.87% | 1.05% | **0.87%** | 0.71% | 1.23% |
| gzip | 28.18% | 27.33% | 26.90% | 26.17% | 26.94% |

- sort 的 miss 主要是 conflict，**8 個 entry（512 B，影片的例子）**就把 miss rate 從 1.87% 砍到 0.87%，比換成 2-way（1.23%）還好。這就是影片說的「很小卻很有效」。
- gzip 的 miss 主要是 capacity，victim cache 幫助有限。和影片說的一致：victim cache 主要減少 **conflict** miss。

### 16.4 兩個經驗法則

4-way cache，大小每次加倍時 miss rate 變幾倍（Square Root Rule 預測每次 × 0.71）：

| 大小 | 1 KB | 2 | 4 | 8 | 16 | 32 | 64 | 128 | 256 KB |
|---|---|---|---|---|---|---|---|---|---|
| sort | 6.84% | ×0.60 | ×0.37 | ×0.48 | ×0.71 | ×0.81 | ×0.90 | ×0.94 | ×0.97 |
| gzip | 39.4% | ×0.89 | ×0.89 | ×0.85 | ×0.77 | ×0.65 | ×0.41 | **×0.10** | ×0.50 |

Associativity Rule：

| 比較 | sort | gzip |
|---|---|---|
| 16 KB 4-way vs 32 KB 2-way | 0.519% vs 0.452% | 20.5% vs 14.0% |
| 64 KB 4-way vs 128 KB 2-way（影片的例子） | 0.378% vs 0.357% | 5.47% vs 1.53% |

- 兩條規則都只**大致**成立。sort 在 16 KB 附近剛好是 × 0.71，但其他地方偏離很多；兩邊的平均趨勢差不多是「大 4 倍、miss 少一半」的量級。
- gzip 在 64 → 128 KB 突然 × 0.10：**working set 剛好放進去了**，capacity miss 一下子消失。單一程式的曲線有這種「膝蓋」，平方根規則是很多程式平均起來的趨勢。
- 正好印證課本說的：「只是經驗法則，不精確成立，一定找得到違反的例子。」

---

## 17. 一頁速查表

| 主題 | 重點 |
|---|---|
| CPI | CPI_ideal + f_mem × (AMAT − 1)；減 1 是因為 1 週期已算在 CPI_ideal |
| AMAT | L1 hit + L1 miss rate × (L2 hit + L2 miss rate × L2 miss penalty) … 最後一層 miss rate = 0 |
| stall penalty | AMAT − L1 hit time |
| local miss rate | 這層 miss ÷ 到這層的存取（公式用這個） |
| global miss rate | 這層 miss ÷ 全部存取 |
| working set | 一小段時間內存取的記憶體量；要放得進 cache |
| compulsory／cold | 第一次存取 → 大 block、prefetch |
| conflict | associativity 不夠 → 高 associativity、victim cache、cache-friendly 程式 |
| capacity | working set > cache → 大 cache、prefetch |
| block 大小 | 32–128 B 合理；太大反而變差 |
| victim cache | 小 fully associative，放被趕出的 line；例：8 entry = 512 B |
| Square Root Rule | miss rate ∝ 1/√size（指數 −0.3 到 −0.7） |
| Associativity Rule | assoc ×2 ≈ size ×2 |
| software prefetch | `__builtin_prefetch(&data[vals[i+100]])`；無效位址也安全 |
| irregular access | data[vals[i]]：硬體很難猜 |
| hardware prefetcher | 看 L1 存取或 miss，猜下一個位址；不能太積極 |
| hit time | 小而簡單的 cache（但 miss rate 變高） |
| write buffer | 4–8 entry；寫 miss 先寫進去就繼續；讀要一起查 |
| critical word first + early restart | 先傳要的字，一到就讓處理器繼續 |
| 實測 | 本機 out-of-order：影片的 addAll 加 prefetch 沒效；load 有相依時快 13.8 倍 |

---

## 18. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| Virtual memory、page table、TLB（TLB miss 也算進 AMAT） | **第 37 支**：The Memory System Part 4（課本 §11.4） |
| 多核心共用 L2／L3、coherence miss（第四個 C） | **第 39–40 支**（課本 Ch12） |
| CPI = CPI_ideal + stall rate × stall penalty | [第 32 支](lecture32_note.md#15-真實-pipelinecpi--cpi_ideal--stall-rate--stall-penalty) |
| cache 的結構、associativity、LRU | [第 35 支](lecture35_note.md) |
| 局部性、cache block | [第 34 支](lecture34_note.md) |
| Out-of-order、memory-level parallelism、進階 prefetcher | Advanced Computer Architecture 課程 |

---

## 附：本講一句話總結

> **有了 cache，CPI = CPI_ideal + f_mem × (AMAT − 1)，而 AMAT = L1 hit time + L1 miss rate × (L2 hit time + L2 miss rate × L2 miss penalty)…，一路遞迴到 miss rate 為 0 的主記憶體，公式裡用的是 local miss rate；miss 分成三個 C：compulsory（第一次存取，用大 block、prefetch 降低）、conflict（associativity 不夠，用高 associativity 或很小的 fully associative victim cache 降低）、capacity（working set 比 cache 大，用大 cache 或 prefetch 降低），還有 miss rate ∝ 1/√size 和「associativity 加倍 ≈ 大小加倍」兩條不精確的經驗法則；data[vals[i]] 這種不規則存取硬體猜不到，可以用 __builtin_prefetch 提早抓；hit time 靠小而簡單的 cache，miss penalty 靠 write buffer（寫）和 critical word first ＋ early restart（讀）。實測 sort 在 direct mapped 時三分之二是 conflict miss，8 entry 的 victim cache 就把 miss rate 從 1.87% 砍到 0.87%（比 2-way 還好），gzip 幾乎全是 capacity miss，到 128 KB working set 放進去才突然下降；而在本機亂序執行的 CPU 上，影片原本的 addAll 加 prefetch 幾乎沒效，只有在每個 load 都要等前一個時才快 13.8 倍。**

---

*筆記依據 Smruti R. Sarangi「Basic Computer Architecture」Chapt 10: Memory Systems, Part 3/4 錄影逐字稿（YouTube 自動字幕，55:33）、Sarangi *Basic Computer Architecture* v3.09 Ch11 §11.3（p.532–539）整理。實測為本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4、Python 3.10、valgrind）結果。*
