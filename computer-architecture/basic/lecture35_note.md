# Basic Computer Architecture 第三十五支影片完整筆記：Cache 的設計：fully associative、direct mapped、set associative、寫入與替換策略（Chapter 10 The Memory System, Part 2/4 → 課本 Ch11）

> **課程**：Basic Computer Architecture — Chapt 10: The Memory System, Part 2/4（播放清單第 35 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：YouTube 錄影逐字稿（1:08:13，自動字幕）＋ 課本 *Basic Computer Architecture* v3.09 **Ch11** §11.2（p.517–531）
> **章號**：影片的 Chapter 10 是課本 v3.09 的 **Chapter 11**；影片說的「chapter 6」（CAM）是課本 Ch7。
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄（本講沒有）
> - `【實測】`：在本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4、Python 3.10、valgrind）實際執行的結果
> - `⚠️`：逐字稿缺字、或影片／課本／實測說法不一致之處
> - `[mm:ss]`：影片時間點
>
> ⚠️ **這支影片的字幕品質很差**，看起來像是被機器翻譯過再翻回英文：load 變成「download」、store 變成「storage／repository／record」、way 變成「path」、write-back 變成「reverse write」、write-through 有時變成「direct access／random access cache」，「fully associative」常常整個被吃掉。下面的內容是對照課本 §11.2 還原的；聽不出來的地方會標 ⚠️。

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. Cache 是一個黑盒子](#1-cache-是一個黑盒子)
- [2. 六個基本操作](#2-六個基本操作)
- [3. 範例：8 KB、64-byte block、32 位元](#3-範例8-kb64-byte-block32-位元)
- [4. Tag array 與 data array](#4-tag-array-與-data-array)
- [5. Fully associative cache](#5-fully-associative-cache)
- [6. Direct mapped cache](#6-direct-mapped-cache)
- [7. Set associative cache](#7-set-associative-cache)
- [8. Tag 有幾個 bit](#8-tag-有幾個-bit)
- [9. 三種 cache 比一比](#9-三種-cache-比一比)
- [10. Data read：和 lookup 重疊](#10-data-read和-lookup-重疊)
- [11. Data write：一定要先有整個 block](#11-data-write一定要先有整個-block)
- [12. Write-through 與 write-back](#12-write-through-與-write-back)
- [13. Insert 與 valid bit](#13-insert-與-valid-bit)
- [14. Replace：random、FIFO、LRU、pseudo-LRU](#14-replacerandomfifolrupseudo-lru)
- [15. Evict](#15-evict)
- [16. 全部串起來：讀與寫的流程](#16-全部串起來讀與寫的流程)
- [17. 【實測】](#17-實測)
- [18. 一頁速查表](#18-一頁速查表)
- [19. 與後續講次的連結](#19-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

【口頭】[00:35]「cache 非常有趣，研究非常多」，這講只講現代 cache 背後的**基本機制**。

```
cache = tag array（SRAM 或 CAM）＋ data array（SRAM）＋ cache controller
位址 = tag | index | offset
  fully associative：tag 26 | offset 6           （block 可以放任何地方，用 CAM 找）
  direct mapped：    tag 19 | index 7 | offset 6  （block 只能放一個地方）
  4-way set assoc.： tag 21 | index 5 | offset 6  （block 可以放在一個 set 的 4 個 way 之一）
操作：lookup、data read、data write、insert、replace、evict
寫入：write-through（每次也寫下層）vs write-back（設 modified bit，被趕出去時才寫）
替換：random、FIFO、LRU、pseudo-LRU（計數器）
```

---

## 1. Cache 是一個黑盒子

[00:49]–[03:35]　課本 §11.2.1（p.517，Fig 11.4）

複習上一講：L1 ⊂ L2 ⊂ 主記憶體（inclusive）；記憶體被切成 **block**，block 是不可分割的單位，「不會有半個或三分之一個 block」。

把 cache 當成黑盒子：

| 操作 | 輸入 | 輸出 |
|---|---|---|
| load | 記憶體位址 | load 的值、**hit／miss** |
| store | 記憶體位址、要存的值 | hit／miss |

miss 時 load 的值沒有意義。

---

## 2. 六個基本操作

[03:35]–[05:26]　課本 §11.2.1（p.517–518）

| 操作 | 什麼時候 | 做什麼 |
|---|---|---|
| **lookup** | 每次 | 看這個位址在不在 cache |
| **data read** | hit 且是 load | 從 cache 讀 |
| **data write** | hit 且是 store | 寫進 cache |
| **insert** | miss 後，block 從下層來了 | 放進 cache |
| **replace** | insert 時 cache（這個 set）滿了 | 選一個要被丟掉的 block |
| **evict** | replace 選好之後 | 把它趕出去 |

前三個是 hit 的路徑，後三個是 miss 才要做的。

【課本補充】p.518：從下層拿 block 再放進 cache 的整個過程叫 **fill**，它是不同層級上 lookup、insert、replace 的組合。

---

## 3. 範例：8 KB、64-byte block、32 位元

[05:26]–[07:58]　課本 §11.2.1（p.517）

這整節都用同一個例子，【口頭】「用具體的例子比用符號好想」。

```
block = 64 B = 2⁶ B           → 位址最低 6 bit 是 block 裡的 offset
32 位元記憶體：2³² / 2⁶ = 2²⁶ 個 block → block address 是 26 bit
cache = 8 KB = 2³ × 2¹⁰ = 2¹³ B
cache 裡的 block 數 = 2¹³ / 2⁶ = 2⁷ = 128 個 entry
```

cache 隨時只放 2²⁶ 個 block 中的最多 128 個。

---

## 4. Tag array 與 data array

[05:50]–[09:14]　課本 §11.2.2（p.518，Fig 11.5）

cache 裡有兩個陣列：

| 陣列 | 存什麼 | 每個 entry |
|---|---|---|
| **tag array** | block address 的一部分（**tag**），用來唯一認出這個 block | 幾十個 bit |
| **data array** | block 的內容 | 64 bytes |

- 兩個陣列 entry 數**一樣多**，而且**一對一對應**。
- 兩者都和 **cache controller**（管理 cache 的邏輯）溝通。controller 收到位址（store 時還有值），輸出 hit／miss 和 load 的值。

【課本補充】p.518：與其用一個 SRAM 陣列同時解決「找」和「讀寫」，分成兩個陣列比較好。tag 通常是 block address 的一部分，取哪一部分看 cache 的種類。

---

## 5. Fully associative cache

[09:14]–[23:53]　課本 §11.2.2（p.518–520，Fig 11.6）

**一個 block 可以放在 128 個 entry 的任何一個。**

- tag = 整個 **26 bit** block address（地址的高 26 bit）。
- 位址格式：`tag (26) | offset (6)`；offset 先忽略。
- lookup：拿 26 bit tag 和 tag array 的**每一個** entry 比。

最適合的結構是 **CAM**（content-addressable memory，用內容找的記憶體，[第 20 支](lecture20_note.md#9-cam用內容找資料)）：

```
tag ─→ CAM（128 列，每列有比較器）─→ 128 條 match 線 ─┬→ OR 閘 → hit／miss
                                                     └→ encoder → 7 bit 的列號 → data array
```

- cache 裡不會有重複的 block，所以 hit 時**只有一列**是 1。
- **OR 閘**：任何一列 match → hit；全是 0 → miss。
- **encoder**：把那一條是 1 的線變成 **7 bit** 的列號（128 = 2⁷）。例如 `0000010` → 第 2 個 entry。
- 用列號去讀 data array（普通的 SRAM 陣列）：load 讀出 64 bytes 再挑需要的部分，store 寫進去。

名字的由來（【口頭】[22:36]）：一個 block 可以和**任何** entry「聯結」（associate）→ fully（完全）associative。這個例子也叫 **128-way associative**。

**缺點**（【口頭】[24:01]）：
- CAM 的 cell 是 **10 顆電晶體**，SRAM 只要 6 顆 → 比較大。
- 每次都要和**每一個** entry 比 → 很耗電。

【課本補充】p.520：
- tag array 也可以用普通 SRAM，讓 controller 一個一個比，但太慢；所以用 CAM 比較常見。
- fully associative 只適合**小**結構（大約 **2–32 個 entry**）；大的話 CAM 的面積和功耗太高。

---

## 6. Direct mapped cache

[23:53]–[28:52]　課本 §11.2.2（p.520–521，Fig 11.7）

另一個極端：**一個 block 只能放在一個固定的 entry。**

```
位址：tag (19) | index (7) | offset (6)
```

- 26 bit block address 拆成 **19 bit tag** 和 **7 bit index**（128 = 2⁷，剛好指到 128 個 entry 之一）。
- lookup：用 index 直接讀 tag array 的那一個 entry，和位址的 tag 比。相等 → hit，不等 → miss。
- hit 時用同一個 index 讀 data array（一對一）。

【課本補充】p.520–521：
- 位置的算法就是 **block address % 128**；128 是 2 的次方，所以只要取 block address 最低 7 bit。
- 能放在同一個 entry 的 block，最低 7 bit 一定相同，所以這 7 bit **不用存**，tag 只要存其餘 **19 bit**。
- hit／miss 的結果用來打開或關掉 data array。

**優點**：很簡單，**不需要 CAM**，tag array 和 data array 都是 SRAM → 比 fully associative **快**。

**缺點**：**hit rate 比較低**。【口頭】[28:08] 兩個 cache line 剛好對到同一個 entry，就會**互相擠掉**，miss 變多。在 fully associative cache 裡它們可以各放一個地方。

【口頭】[27:51] cache **block** 也叫 cache **line**，兩個詞混著用。

---

## 7. Set associative cache

[28:52]–[38:58]　課本 §11.2.2（p.521–524，Fig 11.8–11.9，Definition 103）

中間的做法：把 entry 分成每 **4 個一組**，叫一個 **set**。128 ÷ 4 = **32 個 set**。

```
位址：tag (21) | set index (5) | offset (6)
```

1. 去掉最低 6 bit（offset）。
2. 剩下的取最低 **5 bit** 當 **set index**（2⁵ = 32），找到那個 set。
3. 讀出這個 set 的 4 個 tag，**同時**和位址的 tag 比。
4. 任一個相等 → hit；都不相等 → miss。

- 同一個 set 裡的 block，那 5 bit 都一樣，不用存 → tag = 26 − 5 = **21 bit**。
- set 裡的每個 entry 叫一個 **way**（路）。⚠️ 字幕把 way 翻成「path」。
- 每個 set 有 k 個 way 就叫 **k-way set associative cache**；這個例子是 **4-way**。
- 只比 4 個，**不需要 CAM**，用 SRAM 就好，還是很快。

**電路**（【口頭】[34:38]、課本 Fig 11.9）：

```
set index → 產生 4 個 tag array index → 讀 4 個 tag → 4 個比較器 ─┬→ OR 閘 → hit／miss
                                                              └→ encoder（00–11）→ MUX 選出那個 entry 的 index → data array
```

**數學**：set index 是 i、每個 set 有 K 個 entry，要讀的 tag array 位置是 i·K, i·K + 1, …, i·K + K − 1。

**data array 完全不變**（【口頭】[32:51]）：三種 cache 的 data array 都一樣，只有 tag array 的組織和 tag 長度不同。

**定義（Definition 103）**：**associativity**（關聯度）= 一個 set 裡有幾個 block；**way** = set 裡的一個 entry。

【課本補充】Important Point 17（p.524）：為什麼 index 取 block address 的**最低**幾 bit，而不是把被忽略的 bit 放在 index 右邊？
- 課本的做法（叫 NON-CONT）：**連續的 block 對到不同的 set**，資料平均散在整個 cache。
- 反過來（CONT）：連續的 block 對到同一個 set。兩個小陣列 A、B 就算 cache 放得下，也可能搶同一塊區域。
- 所以 NON-CONT 比較能減少衝突。

---

## 8. Tag 有幾個 bit

[38:58]–[41:24]　課本 Example 148（p.525）

| cache | set 數 | index | tag |
|---|---|---|---|
| direct mapped（1-way） | 128 | 7 | **19** |
| 4-way | 32 | 5 | **21** |
| fully associative（128-way） | 1 | 0 | **26** |

【口頭】set 越多，tag 越短，最短到每個 set 只有 1 個 entry（direct mapped）；set 越少，tag 越長，最長到整個 cache 只有 1 個 set（fully associative，沒有 index）。

【課本補充】Example 148 的一般公式（32 位元，大小 N、associativity K、block B）：

```
offset bits = log B
set 數 = N / (B·K)  →  index bits = log N − log B − log K
tag bits = 32 − log N + log K
```

---

## 9. 三種 cache 比一比

[41:24]–[42:36]　課本 Way Point 10（p.521）

| | 速度 | hit rate（彈性） | 功耗 |
|---|---|---|---|
| direct mapped | **最快** | 最低 | 最低 |
| set associative | 中 | 中 | 中 |
| fully associative | 最慢 | **最高** | 最高 |

⚠️ [41:39] 字幕把 cache 的名字吃掉了，只聽得到「obviously the ... is the fastest, followed by the set associative cache, and then the ...」；照課本 Way Point 10（fully associative「latency 較高」、direct mapped「較快」）還原成上表。

【口頭】[42:20]「沒有一定要用哪一種的規則」，但一般大家選中間：**2-way、4-way，有時 8-way**。

【課本補充】p.524：set associative 是**到目前為止最常見**的設計，就算很大的 cache，功耗和延遲也能接受。

---

## 10. Data read：和 lookup 重疊

[42:36]–[44:33]　課本 §11.2.3（p.524、p.526）

data read 就是普通的 SRAM 讀取。對 **load** 可以加速：

- 一般做法：讀 4 個 tag → 比較 → 找到是哪一個 → **再**去讀那一個 data entry。
- 重疊的做法：讀 4 個 tag 的**同時**，也把 4 個 way 的 data 全部讀出來；比完之後用 MUX 挑對的那一個。

多做一點事（讀了 3 個用不到的 block），但把讀 data 的時間藏在 lookup 裡。

【課本補充】p.526：讀出 block 之後，還要用一組 MUX 從 64 bytes 裡挑出要的那幾個 byte。

---

## 11. Data write：一定要先有整個 block

[44:33]–[46:19]　課本 §11.2.3（p.526）

**寫之前，整個 block 一定要已經在 cache 裡。**

【口頭】[44:40]「大部分學生都不懂為什麼要這樣」：學生會說「反正我要寫新資料，舊的不重要」。問題是：
- 一次寫通常只有 **4 bytes**（int）或 **8 bytes**（long、double）。
- 一個 block 是 **32 或 64 bytes**，而且是不可分割的單位。
- 如果只寫那 4 bytes，就要另外記住「這個 block 哪些部分被寫過、哪些沒有」，很麻煩，還破壞「block 不可分割」的抽象。

所以 miss 時，先從下層拿回**整個舊 block**，再把新資料寫進去。

【課本補充】p.526：不能把 block 的 4 bytes 放 L1、其他放 L2。就算只寫 1 byte，也要先把整個 block 搬進 cache。

---

## 12. Write-through 與 write-back

[46:19]–[50:02]　課本 §11.2.3（p.526–527，Fig 11.10）

因為是 inclusive：L1 有的 block，L2 一定也有，但 **L1 的可能比較新**。兩種處理方法：

| | **Write-through** | **Write-back** |
|---|---|---|
| 寫 L1 時 | **同時也寫**下層的那份 | 只寫 L1，把 **modified bit** 設成 1 |
| 要不要 modified bit | 一般不用 | 要 |
| 下層的流量 | **很多**（每次寫都下去） | 少 |
| evict | **便宜**：直接丟 | **貴**：modified 的要先寫回下層 |

【口頭】[48:34] 例子：同一個 block 在 L1 被寫幾百、幾千次。write-through 每一次都送去 L2，很耗電、L2 流量很大；write-back 一直寫 L1、只設 modified bit 就好。

⚠️ 字幕把 write-back 翻成「reverse write」「right reverse cache」。

**modified bit**：block 剛從下層搬上來時是 0；之後只要被寫過就變 1。存在 tag array 裡。

【課本補充】p.526：課本第 12 章（影片 Ch11）會講到，如果 L1 用 write-through，多處理器的 cache 比較好做。

---

## 13. Insert 與 valid bit

[50:02]–[51:22]　課本 §11.2.4（p.527–528，Fig 11.11）

miss 後從下層拿到 block，要 **insert** 進 cache。

- 在 tag array 每個 entry 多加一個 **valid bit**：data array 那格有有效資料 → 1；空的 → 0。
- 現在 tag array 的一個 entry = **valid bit ＋ modified bit ＋ tag**。
- insert：看這個 set 的每個 way，有沒有 valid = 0 的（空的）。有就放進去，valid 設成 1。
- 沒有空位 → 要做 replace。

【口頭】例子：4-way cache 對到某個 set，發現最後一個 entry 是空的 → 新的 block 就放那裡。

【課本補充】p.527：cache 一開始所有 entry 都是 invalid。額外的狀態放在 tag array，因為它比 data array 小、通常也比較快。

---

## 14. Replace：random、FIFO、LRU、pseudo-LRU

[51:22]–[61:12]　課本 §11.2.5（p.528–530，Definition 104）

**Replacement policy**（替換策略）：set 滿了時，選哪一個 block 丟掉。理想上丟**將來最不可能用到**的，但未來無法預知，只能**用過去猜未來**。

【口頭】[52:00] fully associative cache 就是「整個 cache 只有一個 set」的 set associative cache，所以這些策略都適用。

### Random

隨便選一個 way 丟掉。很簡單，但不管時間局部性：可能把很常用的丟掉，miss 變多。

### FIFO（first in first out，先進先出）

- 每個 way 在 tag array 裡多一個**計數器**。
- 新 block 進來：它的計數器 = 0，set 裡其他 block 的計數器 +1。
- 要丟的時候：選**計數器最大**的（最早進來的）。

【口頭】[53:53] 例子（4-way，一開始全空）：

```
放第 1 個：0
放第 2 個：1 0
放第 3 個：2 1 0
放第 4 個：3 2 1 0
```

問題：第 1 個（計數器 3）可能**非常熱門**，一直被存取，但下一個 block 進來時被丟掉的就是它 → **違反時間局部性**。

【口頭】3 bit 計數器最多到 7，「但沒關係，至少找得到最老的那個」。課本 p.528：計數器最大要等於 associativity，例如 8-way 要 3 bit。

### LRU（least recently used，最近最少使用）

- 丟掉**最久沒被用過**的 block。
- 直接來自 **stack distance** 的定義（[第 34 支](lecture34_note.md#7-量時間局部性stack-distance)）：選在 stack 最底下的那個。
- 但真的維護一個 stack 太花工夫，硬體裡位元數有限，做不到。【口頭】「雖然在某些情境下它被證明是最佳的」。

【課本補充】p.529：假想的 LRU 是每個 block 存一個時間戳記，每次存取就更新，選時間戳記最小的。問題：每次讀寫都要多做事（大約每 3 條指令就有 1 條記憶體指令），而且時間戳記要夠多 bit，不然要常常重設。

### Pseudo-LRU（虛擬 LRU，用計數器近似）

【口頭】[56:44]「給我們 LRU 承諾的幾乎全部好處」。想法：不去找「最少用的」，而是**標記最近用過的**，沒被標記的自然就是最少用的。

1. 每個 block 在 tag array 多一個 **3 bit 計數器**。
2. 每次存取這個 block：計數器 +1，到 **7** 就停（7 + 1 = 7）。
3. **定期**（每幾百或幾千個週期）把 set 裡所有計數器 −1。不然最後全部都停在 7。
4. 新的 block 進來：丟掉**計數器最小**的（有兩個一樣小就挑一個）。
5. 新 block 的計數器設成 **7**（最大值）：它最新，照時間局部性要讓它留久一點。

【口頭】[57:29] 例子：計數器 3、6、4、3 → 一陣子沒人用，減 1 → 2、5、3、2。

【口頭】[60:25]「pseudo-LRU 非常常用，被認為是**最好的替換策略之一**」。

⚠️ 計數器大小：影片用 **3 bit**（最多 7）；課本 p.529 的例子用 **2 bit**（最多 3），並說也可以定期**歸零**而不只是減 1。課本說「可以有很多種這樣的做法」，歡迎讀者用 Dinero 或 sim-cache 模擬器試。

【課本補充】p.530：計數器 +1 的開銷很小，也不在時序的關鍵路徑上，可以平行做或晚一點做。

---

## 15. Evict

[61:12]–[61:40]　課本 §11.2.6（p.530）

| cache | 要做什麼 |
|---|---|
| write-through | 什麼都不用做，直接丟 |
| write-back，modified = 0 | 直接丟 |
| write-back，modified = 1 | 先把 block **寫回下層**，再丟 |

⚠️ [61:19] 字幕說「If the cache is completely full, nothing needs to be done」，對照課本應該是「如果是 write-through cache」。

---

## 16. 全部串起來：讀與寫的流程

[61:40]–[68:13]　課本 §11.2.7（p.530–531，Fig 11.12–11.14）

**讀（load）**：

```
hit ： lookup ─(重疊)─ data read → 回傳
miss： lookup → 下層 cache 讀 block → insert →(replace? → evict? → 寫回?)→ insert
```

- 問號表示**不一定會發生**：只有 set 全滿時才要 replace、evict。
- 【口頭】[64:25] 老師先說 insert 完還要再 data read，馬上改口：「收回我剛剛 15 秒說的話」。資料已經從下層讀到了，直接拿來當輸出就好，不用再讀一次。

**寫（store），write-back**：
- hit：先 lookup，**不能**和 data write 重疊（不然 4 個 way 都寫，至少弄壞 3 個），找到對的 entry 再寫，設 modified bit。
- miss：從下層讀 block → 把新資料寫進這個 block（write block）→ insert；沒空位就 replace → evict（可能要寫回）→ insert。

**寫（store），write-through**：
- hit：lookup、data write，**同時把資料送到下層**。
- miss：讀 block、寫新資料，**新的 block 內容也寫回下層**，同時 insert 到上層。
- replace → evict → insert 時**不用碰下層**：write-through cache 裡沒有任何 modified 的 line，modified bit 全是 0。

【課本補充】p.531：write miss 時，block 到了以後大部分 cache controller 會先放在一個小的**暫存 buffer**，把那 4 bytes 寫進 buffer 就返回；有些處理器會等所有步驟都做完。

⚠️ 影片在 [68:05] 句子講到一半就結束了。

---

## 17. 【實測】

程式在 `experiments/lecture35/`。

### 17.1 量出本機 L1 的 associativity

`ways.c`：n 個位址彼此相差 **4096 bytes**。本機 L1d 是 48 KB、64-byte line、12-way → 48 K ÷ (64 × 12) = **64 個 set**；64 個 set × 64 B = 4096 B，所以相差 4096 的位址 index 都一樣，**全部擠在同一個 set**。用 pointer chasing 輪流讀這 n 個位址：

```
$ gcc -O2 -o ways ways.c && taskset -c 2 ./ways
 1 lines in one set: 1.10 ns per load
 ...
11 lines in one set: 1.07 ns per load
12 lines in one set: 1.07 ns per load
13 lines in one set: 3.20 ns per load
14 lines in one set: 3.21 ns per load
 ...
24 lines in one set: 3.22 ns per load
```

- **12 條以內**：全部放得下，都是 L1 hit（約 1.07 ns）。
- **第 13 條**：馬上跳到 3.2 ns（L2 的延遲，[第 20 支](lecture20_note.md#13-實測)量過約 2.8–4.5 ns）。
- 剛好 **12**，和 `getconf LEVEL1_DCACHE_ASSOC` 的 12 一致。我們用時間「看見」了 set associative 的結構。

對照組：把間隔改成 4160（= 4096 + 64），每個位址落在**不同**的 set：

```
$ gcc -O2 -DSTEP=4160 -o ways2 ways.c && taskset -c 2 ./ways2
12 lines in one set: 1.07 ns per load
13 lines in one set: 1.07 ns per load
24 lines in one set: 1.07 ns per load
```

24 條都在 L1。所以跳到 3.2 ns 不是因為資料太多，而是因為**擠在同一個 set**（衝突）。

### 17.2 Cache 模擬器

`cachesim.py` 照 §11.2 寫：tag array 每個 entry 是 [valid, modified, tag, 計數器]，lookup、insert、replace、evict 都照課本（`experiments/lecture35/cachesim.py:25-46`）：

```python
    def access(self, addr, is_write):
        self.t += 1
        if self.policy == 'plru' and self.t % self.decay == 0:      # 定期把所有計數器減 1
            for s in self.tagarr:
                for e in s: e[3] = max(0, e[3] - 1)
        i, tag = self.split(addr); s = self.tagarr[i]
        for e in s:                                                  # lookup
            if e[0] and e[2] == tag:
                self.hits += 1; self.touch(e, s)
                if is_write:
                    if self.write == 'wb': e[1] = 1                  # write-back：只設 modified bit
                    else: self.lower_writes += 1                     # write-through：每次都寫下層
                return True
        self.misses += 1; self.lower_reads += 1                      # miss：從下層讀整個 block
        if is_write and self.write == 'wt': self.lower_writes += 1
        e = next((e for e in s if not e[0]), None)                   # insert：先找 invalid entry
        if e is None:
            e = self.replace(s)                                      # replace：選犧牲者
            if e[1]: self.lower_writes += 1                          # evict：modified 就寫回
        e[:] = [1, int(is_write and self.write == 'wb'), tag, 0]
        self.insert(e, s)
        return False
```

輸入是[第 34 支](lecture34_note.md#142-真實程式的-stack-distance-與-address-distance)用 valgrind 錄下來的 `sort`、`gzip` 記憶體 trace（各前 200 萬行）。

**Associativity**（8 KB、64 B block、LRU）：

| ways | set | tag／index／offset | sort miss rate | gzip miss rate |
|---|---|---|---|---|
| 1（direct mapped） | 128 | **19**／7／6 | 1.87% | 28.18% |
| 2 | 64 | 20／6／6 | 1.23% | 26.94% |
| 4 | 32 | **21**／5／6 | 0.73% | 26.60% |
| 8 | 16 | 22／4／6 | 0.66% | 26.39% |
| 128（fully associative） | 1 | **26**／0／6 | 0.63% | 26.24% |

- tag 位元數（19、21、26）和課本完全一樣。
- associativity 越高 miss 越少；從 direct mapped 到 4-way 進步最多，4-way 以上就差不多了 → 這就是為什麼大家選 2/4/8-way。
- gzip 的 miss rate 高很多：gzip 用 32 KB 的滑動視窗加上雜湊表，8 KB 的 cache 根本裝不下它的工作集（working set）。

**替換策略**（8 KB、4-way）：

| 策略 | sort | gzip |
|---|---|---|
| random | 0.92% | 27.31% |
| FIFO | 0.85% | 27.31% |
| LRU | **0.73%** | **26.60%** |
| pseudo-LRU（3 bit 計數器，每 1000 次存取減 1） | 0.74% | 26.78% |

LRU 最好，pseudo-LRU 幾乎一樣好，FIFO 和 random 比較差。和影片「pseudo-LRU 給我們 LRU 幾乎全部的好處」一致。

**寫入策略**（sort，8 KB、4-way、LRU）：

| | 從下層讀 | 寫到下層 |
|---|---|---|
| write-back | 15,058 | **6,392** |
| write-through | 15,058 | **598,421** |

miss 一樣多，但 write-through 寫到下層的次數多了約 **94 倍**。這就是影片說的「write-through 讓下層流量很大」。

**Block 大小**（sort，8 KB、4-way、LRU）：

| block | 16 B | 32 B | 64 B | 128 B | 256 B |
|---|---|---|---|---|---|
| miss rate | 1.41% | 1.06% | **0.73%** | 0.92% | 1.47% |

block 變大先有好處（空間局部性），太大反而變差：8 KB 裡的 block 數變少，放不下那麼多不同的地方。64 B 最好，剛好是本機用的大小。

### 17.3 LRU 的弱點：比 way 多一條的循環

用模擬器模擬 17.1 的情況：48 KB、12-way，13 個位址（相差 4096）不斷循環：

| 策略 | miss rate |
|---|---|
| FIFO | **100%** |
| LRU | **100%** |
| random | 15.4% |
| pseudo-LRU | 16.4% |

- 循環 13 個、只有 12 個位置時，LRU 每次丟掉的**剛好就是下一個要用的**，結果全部 miss。FIFO 也一樣。
- random 和計數器 pseudo-LRU 反而只有約 15% miss。
- 17.1 實測 13 條時**每一次**都是 3.2 ns（沒有一部分是 1 ns），看起來比較像「每次都 miss」。⚠️ 這只能說明本機 L1 在這個模式下的表現像 LRU／FIFO，不能確定 Intel 真正用的是哪一種策略，「待補」。

---

## 18. 一頁速查表

| 主題 | 重點 |
|---|---|
| 6 個操作 | lookup、data read、data write、insert、replace、evict |
| 例子 | 8 KB、64 B block、32 位元 → 128 entry、26 bit block address |
| tag／data array | 一對一；tag array 存 tag＋valid＋modified（＋計數器） |
| Fully associative | 放任何地方；tag 26；CAM＋OR＋encoder；只適合 2–32 entry |
| Direct mapped | 只能放一個地方；tag 19、index 7；最快、hit rate 最低、會互相擠掉 |
| Set associative | 放在一個 set 的 k 個 way 之一；4-way：tag 21、index 5 |
| tag bits | 32 − log N + log K |
| index 取最低位 | 連續 block 對到不同 set（NON-CONT），減少衝突 |
| 速度 | DM > SA > FA；hit rate 相反；實務用 2/4/8-way |
| data read | 可以和 lookup 重疊（同時讀出所有 way 再用 MUX 挑） |
| data write | 一定先有整個 block；不能和 lookup 重疊 |
| Write-through | 每次也寫下層；evict 便宜；流量大 |
| Write-back | modified bit；evict 時寫回；流量小 |
| Valid bit | entry 是否有效；insert 先找 invalid 的 |
| Random／FIFO | 簡單；不管時間局部性 |
| LRU | 丟最久沒用的；來自 stack distance；完整實作太貴 |
| Pseudo-LRU | 存取 +1（飽和）、定期 −1、新進的給最大、丟最小的 |
| Evict | WT 或沒改過：直接丟；WB 改過：寫回再丟 |
| 實測 | 本機 L1d 第 13 條同 set 的 line 就 miss → 12-way |

---

## 19. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| 平均存取時間、miss 的種類（compulsory、capacity、conflict）、怎麼降低 miss | [第 36 支](lecture36_note.md)：Memory System Part 3（課本 §11.3） |
| Virtual memory、page table、TLB（TLB 就是一種 cache） | **第 37 支**（課本 §11.4） |
| write-through 讓多處理器的 cache 好做；coherence | **第 39–40 支**（課本 Ch12） |
| 時間、空間局部性；stack distance 和 LRU 的關係 | [第 34 支](lecture34_note.md) |
| CAM、SRAM 的電路 | [第 20 支](lecture20_note.md) |

---

## 附：本講一句話總結

> **cache 由 tag array、data array（一對一）和 cache controller 組成，靠 lookup、data read、data write、insert、replace、evict 六個操作運作；以 8 KB、64-byte block、32 位元為例，fully associative 讓 block 放在 128 個 entry 的任何地方（tag 26 bit，用 CAM＋OR 閘＋encoder 找，只適合很小的結構），direct mapped 只能放在 block address % 128 那一格（tag 19、index 7，最快但會互相擠掉），4-way set associative 放在 32 個 set 之一的 4 個 way 裡（tag 21、index 5），是最常用的折衷；寫之前一定要先有整個 block，write-through 每次也寫下層（evict 便宜、流量大），write-back 只設 modified bit、被趕出去時才寫回；insert 先找 valid = 0 的空位，滿了就用替換策略選犧牲者：random、FIFO 不管時間局部性，LRU 來自 stack distance 但太貴，pseudo-LRU 用飽和計數器（存取 +1、定期 −1、新進的給最大、丟最小的）近似它。實測本機 L1d 在同一個 set 放到第 13 條 line 時延遲從 1.07 ns 跳到 3.2 ns，證明它是 12-way；模擬器在 sort 的 trace 上重現了 tag 位元數、associativity 越高 miss 越少、pseudo-LRU 和 LRU 幾乎一樣好、write-through 寫下層的次數是 write-back 的 94 倍，也看到 LRU 遇到「比 way 多一條的循環」會 100% miss。**

---

*筆記依據 Smruti R. Sarangi「Basic Computer Architecture」Chapt 10: The Memory System, Part 2/4 錄影逐字稿（YouTube 自動字幕，1:08:13，字幕品質很差）、Sarangi *Basic Computer Architecture* v3.09 Ch11 §11.2（p.517–531）整理。實測為本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4、Python 3.10、valgrind）結果。*
