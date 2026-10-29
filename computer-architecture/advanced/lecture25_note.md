# Advanced Computer Architecture 第二十五支影片完整筆記：進階的 cache 設計：pipelined、non-blocking（MSHR）、skewed associative、way prediction、loop tiling、VIPT（Chapter 7, Part V）

> **課程**：Advanced Computer Architecture — Chapter 7: Caches (Part V) -- Advanced Cache Optimizations（播放清單第 25 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：**whisper 轉錄**（本機 `large-v3-turbo`；YouTube 字幕下載不到）（56:00）＋ 課本 *Next-Gen Computer Architecture* v3.2 **Ch7** §7.4（p.302–313）
> **章號**：影片 Chapter 7 = 課本 Ch7。
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄（本講沒有）
> - `【實測】`：在本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4、Python 3.10）實際執行的結果
> - `⚠️`：逐字稿缺字、或影片／課本／實測說法不一致之處
> - `[mm:ss]`：影片時間點

⚠️ whisper 把「way prediction」聽成「wave prediction」、「tiling」有時聽成「dialing」、「msq」應該是 LSQ，筆記直接改正。

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. Pipelined cache](#1-pipelined-cache)
- [2. 再切細 SRAM 的存取？](#2-再切細-sram-的存取)
- [3. Non-blocking cache 與 MSHR](#3-non-blocking-cache-與-mshr)
- [4. Primary miss 與 secondary miss](#4-primary-miss-與-secondary-miss)
- [5. Cache 就是資料結構](#5-cache-就是資料結構)
- [6. Skewed associative cache](#6-skewed-associative-cache)
- [7. Cuckoo search](#7-cuckoo-search)
- [8. Way prediction](#8-way-prediction)
- [9. 位址還沒算出來：用 XOR 估](#9-位址還沒算出來用-xor-估)
- [10. Row major 與 column major](#10-row-major-與-column-major)
- [11. Loop tiling](#11-loop-tiling)
- [12. VIPT cache](#12-vipt-cache)
- [13. 【實測】](#13-實測)
- [14. 一頁速查表](#14-一頁速查表)
- [15. 與後續講次的連結](#15-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

```
存取要 5 cycle？→ pipeline：tag/data 平行讀 → 比 tag → 選 block（再切 SRAM 不划算）
一個 miss 擋住後面？→ non-blocking：MSHR 記下每個 miss 的 block 和它的 miss queue（可以像 LSQ 一樣轉送）
conflict miss？→ skewed associative：4 個 sub-cache 各用不同的 hash；滿了就 cuckoo search
讀全部 way 太耗電？→ way prediction：先讀猜的那個 way；位址還沒好就用 base ⊕ offset 估
大矩陣的迴圈？→ loop tiling：切成 b × b 的小塊，放得進 cache
翻譯位址要多一個 cycle？→ VIPT：set index 全在 page offset 裡，就能邊翻譯邊讀 set
```

---

## 1. Pipelined cache

[00:00] 一次 cache 存取可能要 **3–5 cycle**（L1 很大、clock 很快時），L2 更久。要等 5 個 cycle 才做下一次存取嗎？**不用**：in-order pipeline 時就回答過了，把 cache **pipeline**。

[01:42] 延遲還是很高，但**吞吐量**高：每 1、2 個 cycle 就能接一個新請求，吞吐量導向的程式都會受益。

[03:01] 讀的三個階段，中間插 pipeline latch 就好，不用額外的電路：

| Stage | 讀 | 寫 |
|---|---|---|
| 1 | **平行**讀 tag array 和 data array（多花一點電，但同一個 cycle 拿到 k 個 block） | 讀 tag array |
| 2 | 比較 tag（相減，或逐 bit XOR：任何一對不同就不相等）：要一整個 sub-stage | 比較 tag |
| 3 | 選出 match 的 block | 寫 data array |

[04:33] 讀和寫同一個位址時，要加 **forwarding** 邏輯。

---

## 2. 再切細 SRAM 的存取？

[04:33] 想法：把 SRAM 的存取也 pipeline：在 row decoder 和陣列之間插一個大 latch，存解碼後的 word line。

[06:08] 不建議：

- latch 要很大，大約跟 decoder 一樣大：要快的話，每個 bit 一個 D flip-flop → 面積很大。
- 還會增加 word line 的電容延遲。

---

## 3. Non-blocking cache 與 MSHR

[06:08] block B miss 了，後面的存取怎麼辦？

- 存取**別的 block**：照樣通過，**不要擋住**。
- 存取**同一個 block 的其他 word**：記下來，等 block 從下一層回來時一次處理。

[07:34] 加一個硬體結構（【口頭】「我很想叫它 hardware data structure，因為概念一樣，但為了一致，叫它 hardware structure」）：**MSHR**（miss status holding register）。

[08:40] 每個 cache 一個 MSHR，追蹤這個 cache 的 miss：

- 很小（**4、8 或 16 格**），可以做成 fully associative。
- 每次讀寫都先查 MSHR。

每一格有兩個欄位：

| 欄位 | |
|---|---|
| **block address** | |
| **miss queue** | 一串 miss queue entry，**FIFO** 順序 |

[10:06] 每個 **miss queue entry**：

| 欄位 | 說明 |
|---|---|
| read／write | 1 bit |
| word address | block 裡哪一個 word 造成 miss |
| **tag** | 給 load 用（L1）：造成 miss 的 load 的**目的實體暫存器編號**。值回來時連同 tag 送給處理器，處理器就知道是哪一條 load |
| store value | 給 store 用：要存的值 |

[11:27] block 回來之後，miss queue 的 entry 照 FIFO 順序一個一個套用。

【課本補充】課本 p.304：MSHR 是 **Kroft 1981** 提出的。

---

## 4. Primary miss 與 secondary miss

[11:27] cache miss 時：

1. 在 MSHR 建一格。**沒有空格** → 請求被擋住，non-blocking 的 cache 變回 blocking。所以 MSHR 要夠大，讓這種情況很少發生。
2. 在 miss queue 建第一個 entry：**primary miss**（只有它會真的送請求到下一層）。

[12:54] block 回來之前，同一個 block 又有存取 → **secondary miss**：

| secondary miss 是 | 做什麼 |
|---|---|
| **寫** | 加到 miss queue 的尾巴 |
| **讀** | 像 **LSQ** 一樣（[第 12 支](lecture12_note.md)）：往前找有沒有寫同樣 byte 的 entry；有就**直接轉送**，處理器繼續；沒有就加進 queue，等 block 回來 |

[14:08] 新的地方：MSHR 也有**轉送**的能力。

（本機量了 non-blocking cache 能同時處理幾個 miss，見 [§13.1](#131-同時能等幾個-miss)。）

---

## 5. Cache 就是資料結構

[14:08] 【口頭】「架構是什麼？基本上就是把資料結構的概念包一層硬體。」

| Cache | 對應的資料結構 |
|---|---|
| Direct mapped | **陣列**：知道 index，去看在不在 |
| Fully associative | **hash table**：用 CAM，用內容找（【口頭】老師承認這個比較不是 100% 精確：hash table 只找幾個位置，fully associative 找全部，但都是「用內容找」） |
| Set associative | 做在陣列上的 hash table：被指到 k 個位置，再在裡面找（像 **linear probing**） |

[17:03] 問題：4-way 的 set 有 5 個 block 對到它 → 一直互相趕走（conflict miss）。

---

## 6. Skewed associative cache

[18:17] 另一種 hashing：**cuckoo hashing**，用**好幾個** hash 函數。

- cache 分成 **4 個 sub-cache**。
- 一個位址算 4 個不同的函數 f1、f2、f3、f4 → 4 個不同的位置（每個 sub-cache 一個）。

[19:34] 美妙的性質：兩個位址 A1、A2 在 sub-cache 1 撞了（f1(A1) = f1(A2)），在 sub-cache 2 **很可能不會撞**（f2(A1) ≠ f2(A2)）。所以一個 block 已經佔了 sub-cache 1 的位置，另一個可以放到別的 sub-cache。

[20:56] 要設計 f1–f4 讓這個性質成立。代價：存取時要算 4 個函數、查 4 個 sub-cache（block 可能在任何一個），比較耗電。

【課本補充】課本 p.305–306：skewed associative cache 是 **Seznec** 提出的；課本介紹了 Bodin and Seznec 1997 的 skewing 函數（用 XOR 組合位址的不同部分）。

---

## 7. Cuckoo search

[20:56] 插入 block B（位址 A）：

1. 看 f1(A)、f2(A)、f3(A)、f4(A) 四個位置，有空的就放。
2. 都滿了（最壞情況）：把 f1(A) 那裡的 B′（位址 A′）**踢出去**，B 放進去。
3. [22:19] B′ 去它自己的其他位置 f2(A′)、f3(A′)、f4(A′)。因為 hash 不同，這些位置**很可能**不是 f2(A)、f3(A)、f4(A)（那些已知是滿的），很可能有空位。
4. [23:38] 還是滿的？再把 B′ 放進其中一個，踢出 B″，重複。幾輪之後通常會找到空位。
5. [26:08] 整個 cache 都滿了就永遠找不到，所以只做幾輪，還找不到就把最後那個趕到下一層。

[24:55] 【口頭】建議去讀一般的 hashing：**chaining** 和 **linear probing**。set associative 比較像 linear probing；cuckoo search 是另一種範式，看不懂就先去讀 cuckoo hashing。

[26:08] 【口頭】為什麼叫 cuckoo（杜鵑）？杜鵑很聰明，把蛋下在烏鴉的巢裡，蛋長得差不多，烏鴉不知不覺幫牠孵蛋；小杜鵑出生後，媽媽來把牠們帶走。我們也在玩類似的把戲：把別人的 block 踢來踢去。

（本機模擬了 skewed associative cache，見 [§13.3](#133-skewed-associative-與-way-prediction)。）

---

## 8. Way prediction

[27:30] 預設：set associative cache **同時**讀一個 set 的所有 tag 和 block，為了效能，但做了比需要更多的工作 → 多耗電。

[28:53] 想法：**猜** tag 在哪一個 way：

- 先只讀那個 way。
- tag 不對 → 再讀其他的 way，照常判斷 hit 或 miss。
- 好處：不用花讀全部 way 的能量。前提是**預測器夠準**。

[30:10] 怎麼猜：

| 情況 | 做法 |
|---|---|
| 位址已經有了 | 用位址（最好的預測依據） |
| 位址還沒算出來 | 用 **PC**，像 branch predictor 一樣查表 → 猜一個 way。缺點：同一個 PC 的位址一直在變，way 也跟著變，不太準 |
| | 用**估計的位址**（[§9](#9-位址還沒算出來用-xor-估)） |

【課本補充】課本 p.307 引用 **Powell et al. 2001** 的兩種 way predictor。

---

## 9. 位址還沒算出來：用 XOR 估

[31:20] 例：`ld r2, 12[r1]` 的 pipeline：

```
讀暫存器（r1）→ 算位址（r1 + 12）→ LSQ forwarding（也可以多一級做 virtual → physical）→ 存取 D-cache（可能好幾個 cycle）
```

[32:33] 如果 way prediction 要 **2 個 cycle**，位址算好之後才開始猜就來不及了。

[33:54] 做法：讀到 r1 之後，馬上算 **r1 ⊕ 12**（XOR）：

- XOR 比加法快很多，而且每個 bit 獨立。
- 剩下的時間（約 2 cycle）拿來用類似 branch predictor 的表做 way prediction。
- 像 **gshare** predictor 用 XOR 把兩個資訊混在一起（[第 8 支](lecture08_note.md)）。

[35:09] 到了存取 D-cache 時，先讀猜的那個 way（快）；沒有就讀其他 way。

[32:33] 順便一提：LSQ 裡存 virtual 還是 physical 位址？同一個 thread 裡，如果不同的 virtual address 可能對到同一個 physical address，LSQ 就要存 physical；如果保證不會，就可以用 virtual address 查 LSQ、同時做翻譯。

---

## 10. Row major 與 column major

[35:09] 程式大部分時間在迴圈裡；大迴圈存取存在記憶體裡的大陣列時，cache 是個問題。

[36:22] 2D 陣列怎麼存（記憶體是一維的 byte 陣列）：

| 方式 | 怎麼存 |
|---|---|
| **Row major** | 第一列、第二列、第三列…… |
| **Column major** | 第一行、第二行…… |

⚠️ 老師說「很多語言用 row major，但 column major 稍微更常見」。C 語言的陣列是 **row major**（本機的 `tile.c` 就是照 C 的方式，`a[i][j]` 在 `a[i][j+1]` 旁邊）；哪一種「更常見」影片沒有說根據，這裡不判斷。

[36:22] 存的順序和讀的順序不一樣（例：row major 存、一行一行讀），就沒有 spatial locality，cache miss 很多。

[37:37] 矩陣乘法：

```c
for (i = 0; i < N; i++)
    for (j = 0; j < N; j++)
        for (k = 0; k < N; k++)
            C[i][j] += A[i][k] * B[k][j];
```

[38:50] A 照 row major 走（第 i 列），但 B 是**往下**走第 j 行：沒有 spatial locality。就算 compiler 聰明到把 B 存成 column major，一列也可能比整個 cache 還大。

---

## 11. Loop tiling

[40:02] 也叫 **blocking**。不要一次拿整列、整行，而是拿 **b × b 的小塊**：

- A 從 (ii, kk) 開始的一塊、B 從 (kk, jj) 開始的一塊，相乘得到一組**部分和**，加進 C[i][j]。
- 很多很多個這種小塊做完，C 才完整。

[42:30] 好處：一塊夠小就**整塊放得進 cache**，裡面的存取全部 hit。

```c
for (ii = 0; ii < N; ii += b)
  for (jj = 0; jj < N; jj += b)
    for (kk = 0; kk < N; kk += b)
      for (i = ii; i < ii + b; i++)
        for (j = jj; j < jj + b; j++)
          for (k = kk; k < kk + b; k++)
            C[i][j] += A[i][k] * B[k][j];
```

- 六層迴圈：外面三層以 tile 為單位走，裡面三層在 tile 裡面走。
- 假設 N 被 b 整除（不然補一些假的元素，或做一點代數處理）。

[46:17] 為什麼結果一樣：外面三層 (N/b)³ 次，裡面三層 b³ 次，相乘還是 N³ 次乘法，而且沒有一個乘法做兩次。「漢堡」：正式證明留給觀眾。

（本機實際比了，見 [§13.2](#132-loop-tiling)。）

---

## 12. VIPT cache

[47:41] 回到 way prediction 留下的問題：程式用的都是 **virtual address**，要先翻譯成 physical address 才能存取 D-cache → 算好位址之後至少多一個 cycle 查 TLB（[第 22 支](lecture22_note.md)）。

[48:54] 能不能**不翻譯**就找到 L1 的 set？直接用 virtual address 存取 cache 會出錯：另一個 process 跑在同一個 core 上，它的 virtual address 空間不同，同一個 virtual address 會拿到錯的資料（overlap 問題）。效能和正確性之間有取捨。

[50:05] 有些情況可以兩全：

```
virtual address：| page id（20 bit）     | page offset（12 bit） |
                        ↓ 翻譯                ↓ 不變
physical address：| frame id（20 bit）    | page offset（12 bit） |
cache 的位址：    | tag                   | set index | byte offset |
```

[51:16] **page offset 不會因為翻譯而改變**。如果 cache 的 **set index ＋ byte offset 全部落在 page offset 裡**，set index 就跟翻譯無關：

- 用 virtual address 的 page offset 直接找 set，**同時**讀出這個 set 所有的 tag 和 block。
- **同時**做位址翻譯（查 TLB）。
- 翻譯好了拿到 physical 的 tag，比較 tag、選 block。

[53:59] 這叫 **VIPT**（virtually indexed, physically tagged）：把翻譯和讀 set 重疊，省一個 cycle。

[54:30] 這不是通用的做法，要看 cache 大小和 set index 的大小：L2 不太適合；但「讀課本的例子就會發現，對大部分 L1 的大小這很合理，所以很多商用處理器用這個方法」。

【課本補充】課本 p.312：如果 block offset ＋ set index 有 **14 bit**（比 page offset 多 2 bit），VIPT 就不行了。兩種補救：

- **軟體**：OS 保證 virtual 和 physical 的最低 14 bit 一樣 → 等於用 16 KB 的 super-page 配置 frame，可能浪費記憶體（只用 8 KB 也佔 16 KB）。
- **硬體**：大部分連續的存取都在同一個 page → 把那個 frame 編號放在一個小暫存器，**推測**地用它組出 physical address 先開始存取。

（本機的 L1 剛好符合 VIPT 的條件，見 [§13.4](#134-本機的-l1-為什麼是-12-way)。）

[55:08] 下一講：**trace cache**：能不能完全跳過 branch predictor 和 decode，直接存一串展開好的指令，送進 rename？

---

## 13. 【實測】

程式在 `experiments/lecture25/`。

### 13.1 同時能等幾個 miss

`mlp.c`：K 條**互不相干**的 pointer chase，每一步都是 DRAM miss（1 GiB、隨機順序）。如果 cache 是 blocking 的，K 條只能一條一條等；non-blocking 的話，K 個 miss 可以同時在路上，直到 MSHR（或類似的結構）用完：

| K | P-core 每個 miss | 換算同時在路上 | E-core 每個 miss | 換算同時在路上 |
|---|---|---|---|---|
| 1 | 104.9 ns | 1 | 116.1 ns | 1 |
| 2 | 51.8 | 2.0 | 59.2 | 2.0 |
| 4 | 26.2 | 4.0 | 30.6 | 3.8 |
| 8 | 13.3 | 7.9 | 19.9 | **5.8** |
| 12 | 9.3 | 11.3 | 19.7 | 5.9 |
| 16 | 8.1 | 12.9 | 19.8 | 5.9 |
| 24 | 7.8 | **13.5** | 19.8 | 5.9 |
| 32 | 7.9 | 13.3 | 19.8 | 5.9 |

（「換算同時在路上」= K = 1 時每個 miss 的時間 ÷ 現在每個 miss 的時間。）

- K 加倍，每個 miss 的平均時間就減半：cache **真的是 non-blocking**，好幾個 miss 同時在等記憶體。
- **P-core 最多約 13–14 個** miss 同時在路上，**E-core 約 6 個**，再多也不會更快：這就是 MSHR（或 Intel 叫的 fill buffer 之類的結構）的數量上限，跟影片 [08:40] 說的「4、8 或 16 格」同一個數量級。
- 確切的格數 Intel 的公開資料這裡沒有，**待補**；而且每一步也有 TLB miss（1 GiB 隨機存取），page walk 本身也要排隊，所以這只是估計。

### 13.2 Loop tiling

`tile.c`：1024 × 1024 的 `double` 矩陣乘法（每個矩陣 8 MiB，比 P-core 的 L2 大），P-core：

```
N = 1024（每個矩陣 8 MiB）
naive      ：2.62 s
tiled b= 16：0.49 s（5.4 倍；一個 tile 2 KB；結果一樣）
tiled b= 32：0.64 s（4.1 倍；一個 tile 8 KB；結果一樣）
tiled b= 64：0.78 s（3.4 倍；一個 tile 32 KB；結果一樣）
tiled b=128：0.82 s（3.2 倍；一個 tile 128 KB；結果一樣）
```

- 同樣的 N³ 次乘法，tiling 之後**快 3–5 倍**，結果完全一樣（[46:17] 說的「做的是一樣的乘法」）。
- b = 16 最快：三個 16 × 16 的 tile 只有 6 KB，輕鬆放進 48 KB 的 L1。b 越大，tile 越不容易全部待在 L1（b = 64 時三個 tile 共 96 KB，已經超過 L1）。
- naive 慢的原因就是 [§10](#10-row-major-與-column-major)：`b(k, j)` 隨著 k 往下走，每一步跳 8 KB（一列），沒有 spatial locality。

### 13.3 Skewed associative 與 way prediction

`skew.py`：32 KB、64 B line、4-way（128 個 set）。比一般的 4-way（LRU）和 skewed associative（4 個 sub-cache、4 個不同的 hash、cuckoo search 最多 4 輪）：

```
5 個 block 擠同一個 set：存取 10000 次；set-assoc miss 100.00%，skewed miss 0.05%；way prediction（猜上次用的 way）在 hit 裡猜對 0.0%
work.c 的 trace：存取 4524404 次；set-assoc miss 0.38%，skewed miss 0.44%；way prediction（猜上次用的 way）在 hit 裡猜對 98.6%
```

- **故意的 conflict**（[§5](#5-cache-就是資料結構) 的「5 個 block 擠 4-way」）：一般的 4-way 用 LRU，**每次都 miss**；skewed 只有開始時的 5 次 compulsory miss（0.05%）：5 個 block 在 4 個 sub-cache 裡被 hash 到不同的位置。
- **真的程式**（第 14 支用過的 `work.c` 的 valgrind trace）：兩者差不多，skewed 甚至略差（0.44% 對 0.38%）。這個程式沒有嚴重的 conflict，而模擬裡的 skewed 沒有 LRU（cuckoo 是隨便踢一個）。所以 skewed 主要是「避免最壞情況」，不是一般情況更好。
- **Way prediction**（猜「這個 set 上次用到的那個 way」，最簡單的 MRU 預測）：在真的程式裡，hit 中有 **98.6%** 猜對，只要讀一個 way 就好，省下讀其他 3 個 way 的能量。但在故意的 conflict 裡永遠猜錯（每次都是不同的 block）。

### 13.4 本機的 L1 為什麼是 12-way

用 [第 21 支 §14.1](lecture21_note.md#141-p-core-和-e-core-的各層延遲) 的 cache 參數，算 set index ＋ byte offset 有幾 bit（page 是 4 KB → page offset 12 bit）：

| Cache | 大小、associativity | set 數 | set index ＋ byte offset | ≤ 12 bit？ |
|---|---|---|---|---|
| P-core L1d | 48 KB、12-way、64 B | 48 KB ÷ (12 × 64) = **64** | 6 ＋ 6 = **12 bit** | ✓ 剛好 |
| E-core L1d | 32 KB、8-way、64 B | 32 KB ÷ (8 × 64) = **64** | 6 ＋ 6 = **12 bit** | ✓ 剛好 |
| P-core L2 | 1.25 MB、10-way、64 B | 2048 | 11 ＋ 6 = 17 bit | ✗ |

- 兩種核心的 L1 都**剛好** 64 個 set、12 bit：set index 全部在 page offset 裡，可以做 VIPT。
- 這也解釋了一個看起來奇怪的數字：P-core 的 L1 是 48 KB、**12-way**。要讓 VIPT 成立，**每一個 way 最多只能 4 KB**（一個 page），所以 L1 越大，way 就要越多：32 KB → 8-way、48 KB → 12-way。
- L2 的 set index 有 17 bit，超過 page offset，不能 VIPT，跟影片 [54:30] 說的「L2 不太適合」一致。
- Intel 有沒有真的用 VIPT，這裡只能從參數推論，公開資料沒有寫，**待補**。

---

## 14. 一頁速查表

| 主題 | 重點 |
|---|---|
| Pipelined cache | tag/data 平行讀 → 比 tag → 選 block；延遲高、吞吐量高；讀寫同位址要 forwarding |
| 切 SRAM | 在 decoder 後加 latch：面積大、word line 變慢 → 不划算 |
| Non-blocking | miss 不擋住別的 block 的存取 |
| MSHR | 4–16 格；block address ＋ miss queue（R/W、word address、目的暫存器 tag、store value）；FIFO |
| Primary／secondary | 第一個 miss 才送下層；之後的寫加到 queue、讀可以像 LSQ 一樣轉送；MSHR 滿了就變 blocking |
| 資料結構 | direct mapped = 陣列；set assoc = 陣列上的 hash（linear probing）；fully = 用內容找 |
| Skewed associative | 4 個 sub-cache、4 個 hash；一邊撞了另一邊很可能不撞；cuckoo search 踢來踢去 |
| Way prediction | 先讀猜的 way；省電；用位址、PC，或 base ⊕ offset（像 gshare） |
| Row／column major | 存的順序 ≠ 讀的順序 → 沒有 spatial locality |
| Loop tiling | b × b 的小塊放得進 cache；六層迴圈；乘法數一樣 |
| VIPT | set index ＋ offset ⊂ page offset → 邊翻譯邊讀 set；適合 L1 |
| 實測 | P-core 同時約 13–14 個 miss、E-core 約 6 個；tiling 快 3–5 倍；skewed 讓 5 個 block 擠一個 set 從 100% miss 變 0.05%；MRU way prediction 98.6%；L1 64 set × 64 B = 4 KB → 48 KB 要 12-way |

---

## 15. 與後續講次的連結

| 本講觀念 | 在哪裡 |
|---|---|
| LSQ 的轉送 | [第 12 支](lecture12_note.md) |
| gshare、PC 查表的預測器 | [第 8 支](lecture08_note.md) |
| TLB、virtual address、page offset | [第 22 支](lecture22_note.md) |
| 本機 cache 的參數 | [第 21 支](lecture21_note.md) |
| GPU 用 MSHR 類的結構處理 L1 miss | [第 20 支](lecture20_note.md) |
| Software pipelining 也是迴圈轉換 | [第 16 支](lecture16_note.md) |
| Trace cache、prefetching | [第 26 支](lecture26_note.md) |
| 多核心的 cache coherence 怎麼處理 MSHR 裡的 block | [第 36 支](lecture36_note.md)（Ch9） |
| ML 加速器裡的 tiling | [第 57 支](lecture57_note.md)、[第 58 支](lecture58_note.md)（Ch14） |

---

## 附：本講一句話總結

> **進階的 cache 設計：存取要好幾個 cycle 就把它 pipeline（平行讀 tag 和 data、比 tag、選 block），提高吞吐量；一個 miss 不該擋住後面的存取，所以用 MSHR 記下每個 missing block 和它的 miss queue（primary miss 才送到下一層，secondary miss 的讀可以像 LSQ 一樣轉送）；conflict miss 可以用 skewed associative cache 處理，4 個 sub-cache 各用不同的 hash，滿了就用 cuckoo search 把 block 踢到它的其他位置；讀一個 set 的所有 way 太耗電，就用 way prediction 先讀猜中的那個（位址還沒好時用 base ⊕ offset 估）；大矩陣的迴圈用 loop tiling 切成放得進 cache 的小塊；而只要 set index 加 byte offset 都在 page offset 裡，L1 就能做成 VIPT，一邊翻譯位址一邊讀 set。本機實測：P-core 最多約 13–14 個 DRAM miss 同時在路上（E-core 約 6 個），1024 × 1024 的矩陣乘法 tiling 後快 3–5 倍，skewed cache 讓 5 個 block 擠同一個 4-way set 的情況從 100% miss 降到 0.05%，真實程式裡最簡單的 way prediction 猜對 98.6%，而 L1 的 64 個 set × 64 byte 剛好是一個 4 KB page，這就是 48 KB 的 L1 要做成 12-way 的原因。**

---

*筆記依據 Smruti R. Sarangi「Advanced Computer Architecture」Chapter 7: Caches (Part V) -- Advanced Cache Optimizations 的 whisper 轉錄稿（本機 `large-v3-turbo`，56:00）、Sarangi *Next-Gen Computer Architecture* v3.2 Ch7 §7.4（p.302–313）整理。實測為本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4、Python 3.10）結果。*
