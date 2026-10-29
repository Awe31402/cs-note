# Basic Computer Architecture 第四十四支影片完整筆記：Data link、network、protocol layer：frame、錯誤更正、仲裁、I/O port、memory-mapped I/O、polling、interrupt、DMA（Chapter 12, Part 2/4 → 課本 Ch13）

> **課程**：Basic Computer Architecture — Chapt 12: Data Link, Network, and Protocol Layers, Part 2/4（播放清單第 44 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：YouTube 錄影逐字稿（1:21:42，自動字幕）＋ 課本 *Basic Computer Architecture* v3.09 **Ch13** §13.4–13.6（p.667–688）
> **章號**：影片的 Chapter 12 是課本 v3.09 的 **Chapter 13**；影片說的「chapter 9」（interrupt）是課本 Ch10、「chapter 11」（snoopy）是課本 Ch12。
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄
> - `【實測】`：在本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、Python 3.10）實際執行的結果
> - `⚠️`：逐字稿缺字、或影片／課本／實測說法不一致之處
> - `[mm:ss]`：影片時間點

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. Frame：怎麼知道一組 bit 從哪開始、到哪結束](#1-frame怎麼知道一組-bit-從哪開始到哪結束)
- [2. 錯誤偵測：parity bit](#2-錯誤偵測parity-bit)
- [3. 更強的碼：SEC、SECDED、CRC](#3-更強的碼secsecdedcrc)
- [4. 仲裁：centralized 與 daisy chain](#4-仲裁centralized-與-daisy-chain)
- [5. Transaction、full duplex、half duplex](#5-transactionfull-duplexhalf-duplex)
- [6. 例子：DRAM 讀取的 transaction](#6-例子dram-讀取的-transaction)
- [7. Split transaction bus](#7-split-transaction-bus)
- [8. Network layer：I/O port](#8-network-layerio-port)
- [9. x86 的 in、out 與 I/O-mapped I/O](#9-x86-的-inout-與-io-mapped-io)
- [10. Memory-mapped I/O](#10-memory-mapped-io)
- [11. Protocol layer：polling](#11-protocol-layerpolling)
- [12. Interrupt](#12-interrupt)
- [13. DMA](#13-dma)
- [14. 【原始碼】Linux 的 inb／outb](#14-原始碼linux-的-inboutb)
- [15. 【實測】](#15-實測)
- [16. 一頁速查表](#16-一頁速查表)
- [17. 與後續講次的連結](#17-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

```
data link layer：
  frame：demarcation（停頓）、bit count、bit/byte stuffing（特殊序列，出現在內容裡就重複一次）
  錯誤：parity（偵測 1 個錯）、SEC、SECDED、CRC（burst error）
  arbitration：centralized（request／grant／release）、daisy chain（token）
  transaction：full／half duplex；DRAM 讀取；split transaction bus
network layer：I/O port（一組暫存器）；I/O-mapped（in、out）vs memory-mapped（TLB 導到 I/O）
protocol layer：polling、interrupt、DMA（burst mode、cycle stealing mode）
```

---

## 1. Frame：怎麼知道一組 bit 從哪開始、到哪結束

[00:09]–[07:54]　課本 §13.4.1（p.667–668）

physical layer 只管把 bit 送到；data link layer 要送**一大塊**資料：**frame**（框），一組 logical bit（4、8、32、128……），當成一個不可分割的封包。

三種切 frame 的方法：

| 方法 | 做法 | 問題 |
|---|---|---|
| **Demarcation**（分隔） | 送一串、停一下、再送一串 | 停頓時浪費頻寬；要能偵測「停頓」，ternary 訊號（有 idle 狀態）很好做，binary 就難 |
| **Bit count**（數 bit） | 例如每 32 個 bit 就是一個 frame | **漏一個 bit**，之後所有 frame 的邊界都錯位 → 大量錯誤 |
| **Bit／byte stuffing** | frame 的頭尾加上**特殊序列** | 特殊序列也可能剛好出現在內容裡 |

**Stuffing 的例子**（【口頭】[04:44]）：用 `0xDEADBEEF` 當 frame 的開頭（或頭尾）。如果內容裡剛好有 DEADBEEF：
- 一種做法是額外配合 bit count，判斷它在 frame 中間。
- 另一種像 C 的 `printf("\\")`：要印一個反斜線就打兩個。**內容裡的 DEADBEEF 就送兩次**（DEADBEEF DEADBEEF），接收端看到連續兩次就知道是資料，丟掉一份。
- 序列要設計成就算漏了一個 bit，也能以夠高的機率找到 frame 的開頭和結尾。「這是**編碼理論**和**通訊理論**的課在教的。」

【課本補充】p.667–668：
- 課本的例子：`0xDEADBEEF` 當開頭、`0x12345678` 當結尾；內容裡任一個 32-bit 剛好相同的機率是 2⁻³² ≈ 2.5 × 10⁻¹⁰，很小但不是 0，所以要重複的做法。這些特殊序列叫 **comma**。
- 大部分商用 I/O bus 都用 stuffing。
- physical layer 收到的 bit 先放進 serial-in parallel-out shift register，一塊一塊交給 data link layer。

---

## 2. 錯誤偵測：parity bit

[07:54]–[15:34]　課本 §13.4.2（p.668–669）

**錯誤從哪來**（【口頭】[08:27]）：漏 bit 不是最常見的；最常見的是**電磁干擾**（旁邊電話響、開微波爐）在銅線上感應出電壓，把一個 bit 從 0 變 1 或從 1 變 0（**bit flip**）。

**最簡單的錯誤模型**：**single error detection**，最多偵測一個 bit flip。

```
8 個 data bit：D1 … D8
parity bit P = D1 ⊕ D2 ⊕ … ⊕ D8      ← 一起送，frame 變成 9 bit
接收端重算 parity，和收到的 P 比：不一樣 → 有錯 → 要求重送
```

- 不知道是**哪個** bit 錯（可能是 data，也可能是 P 本身），但知道有錯。
- 重送通常就對了：造成 bit flip 的干擾大多**非常短暫**。

**為什麼一個 data bit 翻了 parity 也會翻**（【口頭】[11:31]，用 2 個 bit 推）：

```
P = x ⊕ y
x 翻成 x̄：  x̄ ⊕ y = (1 ⊕ x) ⊕ y = 1 ⊕ (x ⊕ y) = 1 ⊕ P = P̄
```

（x̄ = 1 ⊕ x：x = 0 時兩邊都是 1，x = 1 時兩邊都是 0。）重算出來的是 P̄，和收到的 P 不一樣 → 偵測到。8 個 bit 也一樣。

**兩個 bit 翻**：重算的 parity 是 P 的補數的補數 = P，**偵測不到**。所以叫 single error detection。

---

## 3. 更強的碼：SEC、SECDED、CRC

[15:34]–[21:09]　課本 §13.4.2（p.669–674，Fig 13.23，Table 13.2–13.3，Example 156）

在 frame 後面加 **ECC**（error correcting code，錯誤更正碼），比一個 parity bit 複雜：

| 碼 | 能力 | 適合 |
|---|---|---|
| **SEC** | 單一錯誤**更正** | 只會錯 1 個 bit |
| **SECDED** | 單一錯誤**更正**、雙重錯誤**偵測** | 可能錯 2 個 bit；「非常常見」；1 個錯不用重送 |
| **CRC**（cyclic redundancy check，循環冗餘檢查） | 偵測 **burst error** | 很強的干擾持續一段時間：連續 5、10、15、20 個 bit 全錯（被翻、或全變 0／1）。很多協定都用 |

【口頭】[16:08]「這是資料通訊和編碼理論的課才教，課本裡有講一個更正單一錯誤的方法。」

**取捨**（【口頭】[20:21]）：要更強的偵測和復原，每個 frame 要加更多資訊，接收端也要做更多事 → 效能和功耗的成本。要看**錯誤模型**（會有什麼外部干擾、會出什麼錯）決定需不需要那麼可靠。

【課本補充】課本的 (12,8) SEC 碼（p.669–671）：
- 8 個 data bit 需要 ⌈log₂(8 + 1)⌉ = **4** 個 parity bit（8 種錯的位置 ＋ 沒錯，共 9 種結果）。
- **P1、P2、P3、P4 放在第 1、2、4、8 位**，D1–D8 放在第 3、5、6、7、9、10、11、12 位。
- Pk = 位置的二進位**第 k 位是 1** 的那些 data bit 的 XOR：

```
P1 = D1 ⊕ D2 ⊕ D4 ⊕ D5 ⊕ D7     （位置 3、5、7、9、11）
P2 = D1 ⊕ D3 ⊕ D4 ⊕ D6 ⊕ D7     （位置 3、6、7、10、11）
P3 = D2 ⊕ D3 ⊕ D4 ⊕ D8          （位置 5、6、7、12）
P4 = D5 ⊕ D6 ⊕ D7 ⊕ D8          （位置 9、10、11、12）
```

- 接收端算 **syndrome** = 收到的 P4P3P2P1 ⊕ 重算的 P4′P3′P2′P1′。**syndrome 正好等於出錯的位置**（0 表示沒錯），把那個 bit 翻回來就更正了。例：D5 在第 9 位（1001），它錯了會讓 P1、P4 翻 → syndrome 1001。
- **SECDED**（p.672–673）：SEC 分不出「D2、D3 都錯」和「D4 錯」（syndrome 都是 0111）。加一個 **P5** = 全部 12 個 bit 的 parity，放第 13 位，不算進 syndrome：syndrome ≠ 0 且 P5 對 → 兩個錯（只能偵測、要求重送）；P5 不對 → 一個錯（可以更正）。
- **CRC**（p.673–674）：在訊息後面補 n 個 0，用 n + 1 bit 的**除數**（generator polynomial）做 **modulo-2 除法**（每一步是 XOR），餘數就是 n-bit CRC。Example 156：訊息 10110011、除數 11001 → CRC **0100**。32-bit CRC 能偵測長度 32 以內的 burst error。

⚠️ 課本 p.673 的 Hamming distance：課本說 parity 碼「任何 code word 距離 1 以內沒有別的 code word」，接著說「同樣地，SEC 碼的最小 Hamming distance 是 2，SECDED 是 3」。我用課本自己的碼窮舉算出 code word 之間的**最小距離**是 parity 2、SEC 3、SECDED 4（[§15.1](#151-課本的-secsecdedcrc-都對)）。課本的數字看起來是用前一句的說法算的（「距離 k 以內沒有別的 code word」，比真正的最小距離少 1）。這是我的解讀，已對照過 PDF 原頁面。

---

## 4. 仲裁：centralized 與 daisy chain

[21:09]–[30:38]　課本 §13.4.3、Fig 13.24–13.25（p.674–677）

**Multi-drop bus**（多點匯流排）：好幾個傳送端接在同一條 bus 上（像第 40 支 snoopy 協定的 bus）。同一時間只能有一個送 → 需要 **arbitration**（仲裁）：選一個傳送端給它 bus 的使用權，而且最好**公平**，大家都輪得到。

【口頭】[22:45] 像老師上課，很多學生想發問，老師先點一個、再點下一個。

**Centralized arbitration**（集中式）：
```
裝置 ─request→ arbiter
裝置 ←grant── arbiter      （資源空了，arbiter 用某種規則挑一個）
裝置 ─release→ arbiter     （用完了，arbiter 可以考慮下一個）
```
- 很簡單。
- 問題：**擴充性**。16、32 個裝置都要接到 arbiter，線很多；arbiter 要同時考慮那麼多請求，變得很複雜。適合 2、4、6 個裝置。

**Daisy chain arbitration**（菊鏈式）：
```
arbiter → 裝置1 → 裝置2 → … → 裝置n
```
- 裝置像 linked list 一樣串起來。想送的裝置拉高共用的 **bus grant** 線。
- arbiter 看到了，如果系統裡還沒有 **token**（令牌，其實可以只是一條拉高的線），就放一個 token 進去，token 從裝置 1 一路傳下去。
- 拿到 token 的裝置可以用資源；不需要就傳給下一個。最後一個裝置拉高 **release** 線，token 就消失。之後有人要再放新的。
- 隱含的**優先順序**：裝置 1 比裝置 2 先拿到 token。
- 為什麼最後一個不把 token 傳回 arbiter（【口頭】[29:14]）：這樣才能**隨時在尾巴加裝置**，所有裝置只要接到 release 線就好。
- 優點：可擴充。缺點：**慢**：20 個裝置、只有第 10 個想送，token 也要經過前 9 個。

---

## 5. Transaction、full duplex、half duplex

[30:38]–[34:31]　課本 §13.4.4（p.677–678）

**Transaction**（交易）：傳送端和接收端之間有**高階意義**的一連串訊息往返（傳送端送、接收端回、再送……）。

| bus | 意思 | 例子 |
|---|---|---|
| **Full duplex**（全雙工） | 兩邊**同時**都能送 | CPU 一直送資料和位址給記憶體，記憶體同時讀資料送回來 |
| **Half duplex**（半雙工） | 同一時間只有一邊送（S1 → S2 或 S2 → S1） | CPU 送位址，記憶體慢慢讀完再送回資料 |

full duplex 比較複雜；half duplex 要決定誰送，但可能由更上層的 transaction 決定。用哪種要看要支援的 transaction。

---

## 6. 例子：DRAM 讀取的 transaction

[34:31]–[40:32]　課本 p.678

假設是同步系統：CPU 和 DRAM 都跟同一個時脈同步；DRAM 是 2D 的記憶體陣列（[第 20 支](lecture20_note.md#8-dram-陣列與-refresh)）。

```
CPU → DRAM：RAS（row address strobe）＋ row 位址     ← 打開一整列
（等 1–3 個週期）
CPU → DRAM：CAS（column address strobe）＋ column 位址 ＋ read 命令   ← 只要這一列的某些欄
DRAM：pre-charge、啟動 bit line，讀出資料
DRAM → CPU：data ready ＋ 連續 3 個週期送出資料
```

**技巧**（【口頭】[38:27]）：DRAM 在送資料給 CPU 的**同時**，CPU 已經送出**下一個** transaction 的 RAS、row、CAS、column、read，讓 DRAM 先開始打開下一列。兩個 transaction **重疊** → 一組請求的總時間變短。

因為同一時間 CPU → DRAM 和 DRAM → CPU 都有資料在流，這條 bus **一定要是 full duplex**。

---

## 7. Split transaction bus

[40:32]–[45:10]　課本 §13.4.5（p.679）

一般的 transaction bus：S1（CPU）向 S2（裝置）要資料，S2 去實體裝置拿、再送回來。這段時間 S1 **鎖住** bus，S3、S4 想溝通也不行。transaction 中間可能有**很長的空檔**，bus 沒在用 → 浪費。

**Split transaction bus**（分離式交易匯流排）：

```
request sub-transaction：S1 → S2 送請求，**放掉 bus**
（這段時間 S3 ↔ S4、S1 ↔ 別人都可以用 bus）
response sub-transaction：S2 → S1 送回應
```

- 共享資源只在**需要的那段時間**鎖住。可以接更多裝置、頻寬更大、閒置更少。
- 「一個 transaction 的訊息要嘛全到、要嘛全沒到」的保證，交給 S1、S2 裡的專門模組處理（等全部訊息到了才繼續）。
- 【口頭】[44:26]「某種意義上簡單（bus 的設計簡單），某種意義上複雜（要確保 transaction 的行為）」。大部分系統都用 split transaction，除非有理由一定要用純 transaction bus。

---

## 8. Network layer：I/O port

[45:10]–[50:51]　課本 §13.5.1（p.680–682）

network layer：在不同的 I/O 裝置之間**路由**訊息，確保送到對的目的地。

主機板上每個裝置都提供一組 **I/O port**：

```
software 看到的 port：一組暫存器（有的輸入、有的輸出）
           ↕
port controller：處理 physical layer 和 data link layer（framing、錯誤更正、轉換格式）
           ↕
port connector：一組銅接腳，接外部的連接器
           ↕
I/O 裝置
```

- **硬體 port**：connector ＋ controller ＋ 存暫存器的空間。
- **軟體 port**：硬體 port 的包裝，軟體只看到「一組暫存器」。
- 寫暫存器 → port controller 自動轉成正確的電壓、經過 physical 和 data link layer 送到裝置。讀暫存器 → 拿到裝置送來的資料。

**Intel 的參考架構**（【口頭】[50:29]）：定義 **64K 個 8-bit I/O port**。64K = 65,536 = 2¹⁶，所以每個 port 有一個 **16-bit 的 port 編號**。每個裝置提供一組暫存器，每個暫存器分到一個唯一的 port 編號。

【課本補充】p.680：可以把 4 個連續的 8-bit port 合成一個 32-bit port。

---

## 9. x86 的 in、out 與 I/O-mapped I/O

[50:51]–[56:14]　課本 §13.5.1（p.681–682）

**I/O address space**：軟體（有特權的組合語言或作業系統程式碼）能存取的所有 I/O port。

x86 用兩條指令：

```
in  r1, <I/O port>     r1 ← I/O port 的內容     （從裝置讀進來）
out r1, <I/O port>     I/O port ← r1           （送到裝置）
```

【口頭】[51:40]「這也看得出組合語言的好處：沒有組合語言就沒辦法存取 I/O port 這種特權資源。我們要感謝組合語言的發明者。」

**I/O-mapped I/O**（用 port 編號定址）的路徑（【口頭】[54:14]）：
```
CPU（帶 port 編號的 in／out 請求）→ North Bridge → South Bridge →（可能再好幾跳）→ 裝置
```
- 裝置接上時，South Bridge 分配 port 編號給它（例如「我有 100 個 port，給我 100 個位址」）。
- North Bridge、South Bridge 各有一個小的 **routing table**：每個 entry 是「port 編號範圍 → 哪個裝置」。
- 回應走反方向；回應大多是送給 CPU 的，沿路的晶片很容易知道往哪送。

只靠 in 和 out 兩條指令，作業系統就能控制幾乎所有 I/O 裝置。

---

## 10. Memory-mapped I/O

[56:14]–[67:03]　課本 §13.5.2（p.683–684）

I/O-mapped I/O 的問題（【口頭】[56:28]）：
1. **不可攜**：程式設計師要知道 port 的位址，但同一個裝置在不同主機板上可能分到不同位址。例如滑鼠在一塊主機板上是 port 20、21、22，在另一塊是 50、51、52 → 為一塊主機板寫的程式到另一塊就不能用。
2. **大塊資料很慢**：一個 byte 一條 out 指令，送 1 KB 要 1000 條。

**Memory-mapped I/O**：在 I/O port 和應用程式之間加一層**虛擬層**。

- 作業系統用 **paging** 把一段 I/O port 對應到一段**記憶體位址**。
- 例：虛擬位址 4096–8191（一個 4 KB page）對應到某個 I/O 裝置。
- **TLB 的 entry 多一個 bit**：0 → 照常送到記憶體系統；1 → 換成 I/O port 位址，送到 **I/O 系統**。
- 軟體只用一般的 **load／store** 指令，**根本不知道**自己在讀寫 I/O 裝置。

**怎麼設定**（【口頭】[64:01]，以鍵盤為例）：程式請作業系統「把這個 page 連到鍵盤」→ 作業系統問 North／South Bridge 鍵盤的 port 位址 → 在 TLB 建立「這段虛擬位址 → I/O 系統的這些 port」的對應。之後就能用一般的 store 送「給我下一個按鍵」「Caps Lock 是什麼狀態」；鍵盤也用同一條路把按鍵寫進它的暫存器、送 interrupt。

**好處**（【口頭】[65:57]）：
- 對應關係是一層很有效的抽象，同一個程式原則上能在不同機器上跑。
- 讀寫 I/O 很簡單：設好對應後，用一般的 load／store。
- 大塊資料可以用迴圈，或 x86 的區塊搬移指令（第 11–14 支 x86 的字串指令），這些指令不知道資料其實在記憶體和 I/O 裝置之間流動。

---

## 11. Protocol layer：polling

[67:03]–[70:28]　課本 §13.6.1（p.685–686）

protocol layer 是最上層，假設下面三層都可靠：訊息能從任何節點送到任何節點、內容正確、到對的地方。它定義 CPU 和 I/O 裝置之間最高階的互動。三種方法：**polling**、**interrupt**、**DMA**。

**Polling**（輪詢）：程式要印一頁（實體的紙，不是記憶體 page），先要知道印表機有沒有空：
- 一直問印表機的狀態（寫印表機 port 的暫存器 → 訊息送到印表機 → 印表機透過暫存器回答）。
- 忙 → 等一下再問。「你有空嗎？你有空嗎？你有空嗎？」
- 有空了 → 送出這頁的內容。

簡單，但**沒效率**：多了很多流量、浪費電，問的時候 CPU 什麼別的事都不能做。

【課本補充】p.685–686 的 SimpleRisc polling 程式（假設送 0xDEADBEEF 到 port 0xFF00 問狀態，從 0xFF04 讀回答）：

```
movh r0, 0xDEAD
addu r0, r0, 0xBEEF
.loop:
        out r0, 0xFF00
        in  r1, 0xFF04
        cmp r1, 1
        beq .loop /* keep looping till status = 1 */
```

⚠️ 這段的註解和程式不一致：`cmp r1, 1` 接 `beq .loop` 是「狀態**等於** 1 就回去繼續轉」，也就是一直轉到狀態**不是** 1 為止；註解卻寫「轉到狀態 = 1」。要符合註解，條件應該反過來（狀態不等於 1 才回去）。已對照過 PDF 原頁面，原文就是這樣。

---

## 12. Interrupt

[70:28]–[71:44]　課本 §13.6.2（p.686–687）

CPU 告訴裝置「狀態改變時通知我」，然後就**不管**了，去做別的事。印表機空了，送一個 **interrupt** 給 CPU（經過 I/O 訊息的路，但當成 interrupt 交付），CPU 處理 interrupt（[第 33 支](lecture33_note.md)；課本 §10.8），再把列印工作送出去。

【口頭】[71:44] 借錢的比喻：
- **polling**：每隔一天就問欠錢的人「什麼時候還？還錢！還錢！」
- **interrupt**：只跟他說一次「你有錢了打電話給我」。
- **DMA**：錢很多時，如果他用現金還，我得一張一張數；不如請他從他的銀行帳戶**轉帳**到我的帳戶。

【課本補充】p.687：每個 interrupt 要帶著產生它的裝置的代碼；接在 USB 這種通用 port 上的裝置，代碼有兩部分：主機板上那個 port 的位址，和 port 分給這個裝置的 id。

---

## 13. DMA

[71:44]–[81:37]　課本 §13.6.3（p.687–688）

**問題**：印表機有空了，要送好幾 MB（例如一張 2 MB 的照片）。一個 byte 或 4 bytes 一條指令搬，CPU 整段時間都被綁住；就算用 memory-mapped I/O，也還是要很久。「就像朋友用小鈔還錢，他要數、我也要數。」

**DMA**（direct memory access，直接記憶體存取）：把搬資料的工作**外包**給 **DMA engine**（North Bridge 晶片裡的一個特殊單元），它同時能存取主記憶體和 I/O 裝置。

```
CPU 設定 DMA controller：「把記憶體某位址開始的 1 MB 送到印表機，做完送 interrupt 給我」
DMA engine：一次從記憶體讀 64 或 128 bytes，以印表機吃得下的速度送過去
做完 → interrupt → CPU
```

- **雙向**都可以：記憶體 → 裝置，或裝置 → 記憶體。
- 快很多：CPU 完全不用參與，可以去做別的事；DMA 到記憶體的 bus 很寬，一次 8 或 16 bytes。

**兩種模式**：

| | 做法 | 特性 |
|---|---|---|
| **Burst mode** | DMA 進行時把路徑**鎖住**（像 transaction bus）；看實作，可能連 CPU 到 North Bridge 的 front side bus 都被擋住，CPU 沒辦法送訊息給別的裝置 | 一次做完 |
| **Cycle stealing mode** | 用 **split transaction** 的方式，資料切小塊，每塊先仲裁 bus 再送 | 比較有彈性，其他流量可以同時進行 |

【口頭】[79:59] 大部分主機板上 DMA 流量的**優先權比較低**：資料量大，而繪圖、音效這種互動流量不能等，不然畫面和聲音會卡（鍵盤滑鼠可以忍受一點延遲）。

下一講挑兩個案例（PCI Express、USB），再講儲存媒體。（課本有五個案例。）

【課本補充】p.687–688：
- 一條 I/O 指令通常要**十幾個週期**以上（要經過好幾層），I/O bus 的頻率通常只有處理器的 1/3 到 1/4。所以要讓 device driver 和 interrupt handler 盡量短。
- 課本的比喻：教授改 100 多份考卷要一星期，交給助教改，教授就能做研究。
- device driver 把記憶體範圍和 I/O 裝置（port 位址）、方向告訴 DMA engine 之後就**暫停自己**，處理器去跑別的程式。
- **正確性問題**：DMA 直接寫主記憶體，但那個位置可能還在 **cache** 裡 → cache 裡的值變舊了，處理器卻不知道。所以要有 **DMA snoop circuit**：DMA 寫到的位置如果在 cache 裡，就把它趕出去（和[第 40 支](lecture40_note.md)的 snoopy 協定同一個想法）。

---

## 14. 【原始碼】Linux 的 inb／outb

Linux 核心存取 x86 I/O port 用的就是影片的 in、out 指令，`linux-src/arch/x86/include/asm/shared/io.h:7-20`：

```c
#define BUILDIO(bwl, bw, type)						\
static inline void __out##bwl(type value, u16 port)			\
{									\
	asm volatile("out" #bwl " %" #bw "0, %w1"			\
		     : : "a"(value), "Nd"(port));			\
}									\
									\
static inline type __in##bwl(u16 port)					\
{									\
	type value;							\
	asm volatile("in" #bwl " %w1, %" #bw "0"			\
		     : "=a"(value) : "Nd"(port));			\
	return value;							\
}
```

- `BUILDIO(b, b, u8)` 展開成 `__outb`、`__inb`（8 bit），`w` 是 16 bit、`l` 是 32 bit（同一個檔案第 22–24 行；第 27–30 行再 `#define inb __inb`、`outb __outb`……）。
- **port 的型別是 `u16`**：16 bit，正好 64K 個 port，和影片說的 Intel 架構一樣。
- `"a"(value)`：要送的值放在 **eax／al** 暫存器（x86 的 in、out 只能用這個暫存器）；`"Nd"(port)`：port 編號是 0–255 的常數就直接寫在指令裡，否則放在 **dx**。

---

## 15. 【實測】

程式在 `experiments/lecture44/`。

### 15.1 課本的 SEC、SECDED、CRC 都對

`ecc.py` 照課本 Table 13.2 的位置實作 (12,8) SEC、加 P5 的 SECDED，和 Example 156 的 CRC，全部**窮舉**檢查：

```
$ python3 -I ecc.py
parity (9,8)  ：code word 之間最小的 Hamming distance = 2
SEC (12,8)    ：code word 之間最小的 Hamming distance = 3
SECDED (13,8) ：code word 之間最小的 Hamming distance = 4
SEC：256 個資料 × 12 個位置的單一錯誤，syndrome 指出正確位置的有 3072 / 3072
SECDED：單一錯誤判成 single 的 3328 / 3328；雙重錯誤判成 double 的 19968 / 19968
CRC：訊息 10110011、除數 11001 → CRC = 0100（課本 Example 156：0100）
CRC 偵測 burst error： 長度1: 100%  長度2: 100%  長度3: 100%  長度4: 100%  長度5: 88%  長度6: 94%  長度7: 94%  長度8: 94%
```

- SEC：所有 3072 種單一錯誤，syndrome **都剛好指到出錯的位置**。
- SECDED：所有單一錯誤都判成「一個錯（可更正）」，所有 19,968 種雙重錯誤都判成「兩個錯（只偵測）」。
- 最小距離 2、3、4：距離 2 才能偵測 1 個錯、3 才能更正 1 個錯、4 才能再多偵測 2 個錯（§3 的 ⚠️）。
- CRC 算出 **0100**，和課本一樣。這個 4-bit CRC 對**長度 ≤ 4** 的 burst error **100%** 偵測得到，更長的就會漏掉一些（約 6–12%）。和課本「32-bit CRC 能偵測 32 bit 以內的 burst」是同一個規律：n-bit CRC 保證抓到 n bit 以內的 burst。

### 15.2 本機的 I/O port 和 interrupt

`/proc/ioports`（一般使用者看到的位址都被遮成 0，但名字看得到）：

```
0000-0000 : PCI Bus 0000:00
  0000-0000 : dma1
  0000-0000 : pic1
  0000-0000 : timer0
  0000-0000 : keyboard
  0000-0000 : rtc0
  0000-0000 : dma page reg
  0000-0000 : pic2
  0000-0000 : dma2
```

到今天，x86 電腦上還保留著傳統的 I/O port：**keyboard**、**dma1／dma2**（老式的 DMA controller）、**pic**（interrupt controller）、**timer**、**rtc**（即時時鐘）。

`/proc/interrupts`（20 個 CPU 加總，節錄）：

```
    1:        10056  IR-IO-APIC 1-edge i8042                  ← 鍵盤控制器
  126:       100605  IR-PCI-MSIX-0000:00:0e.0 2-edge vmd0     ← NVMe SSD（經過 Intel VMD）
  129:       189088  IR-PCI-MSIX-0000:00:0e.0 5-edge vmd0
  ...
```

- **i8042**（鍵盤控制器）收過 1 萬多次 interrupt：每按一個鍵就是一次，正是 §12 說的「鍵盤用 interrupt 通知 CPU」。
- SSD 的 interrupt 一共有十幾條（MSI-X，每條十萬次以上）：SSD 用 DMA 把資料搬進記憶體，做完就送 interrupt（§13）。

⚠️ 一般使用者不能執行 `in`／`out`（要 root 權限和 `ioperm`），所以這裡沒有真的去讀寫 I/O port。

---

## 16. 一頁速查表

| 主題 | 重點 |
|---|---|
| Frame | 一組 logical bit，data link layer 的單位 |
| 切 frame | demarcation（停頓，要 ternary）、bit count（漏 bit 就全錯位）、stuffing（特殊序列，內容出現就重複） |
| 錯誤來源 | 電磁干擾造成 bit flip |
| Parity | P = 全部 XOR；偵測 1 個錯；2 個錯偵測不到 |
| SEC | 4 個 parity bit 在 1、2、4、8 位；syndrome = 錯的位置 |
| SECDED | 再加 P5（總 parity）；更正 1 個、偵測 2 個 |
| CRC | modulo-2 除法的餘數；偵測 burst error；例：10110011 ÷ 11001 → 0100 |
| Arbitration | 選一個傳送端用 bus，要公平 |
| Centralized | request／grant／release；簡單，不好擴充 |
| Daisy chain | token 一路傳；好擴充，但慢；有優先順序 |
| Full／half duplex | 兩邊同時送／一次一邊 |
| DRAM transaction | RAS＋row → CAS＋column＋read → data ready＋資料；下一個 transaction 重疊 → 要 full duplex |
| Split transaction | 請求後放掉 bus，回應時再拿；大部分系統用 |
| I/O port | 軟體看到一組暫存器；controller 處理下兩層；connector 接裝置 |
| Intel | 64K 個 8-bit port，16-bit 編號 |
| in／out | in r1, port：讀；out r1, port：寫 |
| I/O-mapped I/O | 用 port 編號；North／South Bridge 的 routing table；不可攜、大塊資料慢 |
| Memory-mapped I/O | OS 用 paging 把 port 對到記憶體位址；TLB 的 bit 導到 I/O 系統；用 load／store |
| Polling | 一直問；簡單、浪費 |
| Interrupt | 叫裝置有事通知；省事 |
| DMA | DMA engine 在記憶體和裝置之間搬資料，做完送 interrupt；burst mode／cycle stealing mode；要 DMA snoop |
| 實測 | SEC、SECDED、CRC 窮舉全對；4-bit CRC 抓到所有 ≤ 4 bit 的 burst；/proc 看得到 keyboard、dma 等老 port 和 i8042 的 interrupt |

---

## 17. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| PCI Express、USB（split transaction、CRC、塞位元）、硬碟 | [第 45 支](lecture45_note.md)：PCI-X, USB, and hard disks |
| 光碟、SSD（SSD 也用 ECC） | [第 46 支](lecture46_note.md) |
| Interrupt 的處理流程 | [第 33 支](lecture33_note.md)（課本 §10.8） |
| TLB、paging（memory-mapped I/O 靠它） | [第 37 支](lecture37_note.md) |
| Snoopy 協定（DMA snoop 同一個想法）、bus 仲裁 | [第 40 支](lecture40_note.md) |
| DRAM 陣列 | [第 20 支](lecture20_note.md#8-dram-陣列與-refresh) |
| 實體層、四層架構 | [第 43 支](lecture43_note.md) |

---

## 附：本講一句話總結

> **data link layer 把 bit 組成 frame（用停頓、數 bit，或最常用的特殊序列 stuffing，內容裡出現就重複一次），再對付電磁干擾造成的 bit flip：parity bit 能偵測一個錯，SEC 把 4 個 parity bit 放在第 1、2、4、8 位、讓 syndrome 直接等於出錯的位置，SECDED 再加一個總 parity 分辨一個錯和兩個錯，CRC 用 modulo-2 除法的餘數偵測一長串的 burst error；多個裝置共用 bus 時要仲裁（簡單但不好擴充的 centralized request／grant／release，或好擴充但慢的 daisy chain token），transaction 可以用 full 或 half duplex（DRAM 讀取要重疊兩個 transaction 所以用 full duplex），而 split transaction bus 送完請求就放掉 bus。network layer 讓每個裝置提供一組 I/O port 暫存器：x86 用 in、out 指令配 16-bit 的 port 編號（I/O-mapped I/O），或讓作業系統用 paging 把 port 對應到記憶體位址、用一般的 load／store 存取（memory-mapped I/O，可攜又適合大塊資料）。protocol layer 有一直問的 polling、讓裝置主動通知的 interrupt，以及把大量資料交給 DMA engine 搬、搬完再送 interrupt 的 DMA（burst 或 cycle stealing 模式）。實測課本的 SEC、SECDED 和 CRC 例子窮舉全對，4-bit CRC 抓到所有 4 bit 以內的 burst；本機的 /proc 裡還看得到 keyboard、dma 這些老 I/O port 和鍵盤控制器 i8042 的上萬次 interrupt，Linux 的 inb／outb 也正是用 16-bit port 的 in、out 指令。課本有兩處小問題：polling 程式的 beq 和註解相反，SEC／SECDED 的「最小距離」比真正的少 1。**

---

*筆記依據 Smruti R. Sarangi「Basic Computer Architecture」Chapt 12: Data Link, Network, and Protocol Layers, Part 2/4 錄影逐字稿（YouTube 自動字幕，1:21:42）、Sarangi *Basic Computer Architecture* v3.09 Ch13 §13.4–13.6（p.667–688）、Linux v6.2.12 `arch/x86/include/asm/shared/io.h` 整理。實測為本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、Python 3.10）結果。*
