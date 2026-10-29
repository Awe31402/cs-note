# Basic Computer Architecture 第四十三支影片完整筆記：I/O 系統概論與實體層：訊號、編碼、同步（Chapter 12 I/O System, Part 1/4 → 課本 Ch13）

> **課程**：Basic Computer Architecture — Chapt 12: I/O system: overview and physical layer, Part 1/4（播放清單第 43 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：YouTube 錄影逐字稿（1:25:14，自動字幕）＋ 課本 *Basic Computer Architecture* v3.09 **Ch13** §13.1–13.3（p.641–666）
> **章號**：影片的 Chapter 12 是課本 v3.09 的 **Chapter 13**；影片說的「chapter 6」（D flip-flop）是課本 Ch7，「chapter 9」（working set）是課本 Ch11。
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
- [1. 電腦裡的 I/O 裝置](#1-電腦裡的-io-裝置)
- [2. 軟體這邊：system call 與 I/O stack](#2-軟體這邊system-call-與-io-stack)
- [3. 硬體這邊：網路卡、主機板、chipset](#3-硬體這邊網路卡主機板chipset)
- [4. North Bridge 與 South Bridge](#4-north-bridge-與-south-bridge)
- [5. I/O 系統的四層](#5-io-系統的四層)
- [6. Active high 與 active low](#6-active-high-與-active-low)
- [7. LVDS：用兩條線的電壓差](#7-lvds用兩條線的電壓差)
- [8. Physical bit 與 logical bit](#8-physical-bit-與-logical-bit)
- [9. Return to Zero（RZ）](#9-return-to-zerorz)
- [10. Manchester 編碼](#10-manchester-編碼)
- [11. NRZ 與 NRZI](#11-nrz-與-nrzi)
- [12. 同步子層：keep-out region](#12-同步子層keep-out-region)
- [13. Synchronous 與 mesochronous bus](#13-synchronous-與-mesochronous-bus)
- [14. Source synchronous bus](#14-source-synchronous-bus)
- [15. Asynchronous bus 與 clock recovery](#15-asynchronous-bus-與-clock-recovery)
- [16. Four-phase 與 two-phase handshake](#16-four-phase-與-two-phase-handshake)
- [17. 【原始碼】Linux 的 read system call](#17-原始碼linux-的-read-system-call)
- [18. 【實測】](#18-實測)
- [19. 一頁速查表](#19-一頁速查表)
- [20. 與後續講次的連結](#20-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

本章（影片 Ch12）講 I/O 系統和儲存裝置（硬碟、隨身碟、CD、DVD……）。

```
軟體：application →（system call）→ kernel → device driver → 處理器 → I/O 系統 → I/O 裝置
硬體：CPU ─ North Bridge（記憶體、GPU，高頻寬）─ South Bridge（硬碟、USB、網路、音效，低頻寬）
四層：physical（transmission ＋ synchronization）→ data link → network → protocol
transmission：active high／low、LVDS（差動）；RZ、Manchester、NRZ、NRZI
synchronization：keep-out region；synchronous、mesochronous、source synchronous、asynchronous
asynchronous bus：clock recovery；four-phase／two-phase handshake
```

---

## 1. 電腦裡的 I/O 裝置

[02:23]–[05:48]　課本 §13.1.1（p.642–644）

第一章說過：CPU 和記憶體溝通；working set 太大時資料可以放在硬碟。但這樣還不夠，要能**和電腦互動**：
- 輸入：鍵盤、滑鼠。
- 看結果：螢幕；要印就用印表機。（【口頭】「玩遊戲也是一種計算」。）

| I/O 裝置 | 說明 |
|---|---|
| 硬碟 | 【口頭】[03:45] 歸為 I/O 裝置：不是每台機器都有硬碟，可以用別的技術；而且和硬碟溝通用的都是 I/O 技術。「這有點勉強」 |
| 網路卡 | 一定是 I/O 裝置，用來和網路上其他電腦溝通 |
| 印表機、掃描器、相機、喇叭 | 傳統的 I/O 裝置 |

**I/O port**（I/O 埠）：一組金屬接腳，用來接外部的連接器。例如筆電側邊的 **USB port**（一個長方形的孔，中間有凸出的結構），或接顯示器的 **DVI port**。

---

## 2. 軟體這邊：system call 與 I/O stack

[05:48]–[12:17]　課本 §13.1.1、Fig 13.2（p.643–644）

```
user application
   ↓ system call（例如 read、write）
operating system（kernel）
   ↓
device driver
   ↓
processor
   ↓
I/O system（一整套晶片、線路、協定，負責把訊息送到對的 I/O 裝置）
   ↓
I/O device
```

**System call**（系統呼叫）：作業系統提供給應用程式的**特別函式**。【口頭】「作業系統課程會教，這裡講大概」。Linux 例子：

```c
read(file_descriptor, buffer, num_bytes);    /* 從 I/O 裝置讀 num_bytes 到記憶體的 buffer */
write(file_descriptor, buffer, num_bytes);   /* 把記憶體 buffer 的 num_bytes 送到 I/O 裝置 */
```

- **File descriptor**（檔案描述符）：I/O 裝置裡某個儲存區域的唯一 id。
- Linux 把**所有裝置都看成檔案**，放在 **`/dev`**（device）目錄下，都能用 read、write 讀寫。
- 兩種裝置：
  - **character device**（字元裝置）：一次讀寫一個字元，例如鍵盤、滑鼠。
  - **block device**（區塊裝置）：一次處理一大塊資料，例如硬碟、網路卡。

**I/O stack**：作業系統裡處理一個 I/O 請求的一連串模組。
- **請求**（藍線）：application → system call → **kernel**（作業系統的核心，和應用程式打交道）→ **device driver**（專門和裝置溝通的程式）→ 處理器 → I/O 系統 → I/O 裝置。
- **回應**（紅線）：反過來走一樣的路。例如鍵盤送回下一個讀到的字元；網路卡送出或收到網路訊息。

I/O 裝置可以在主機板上、在擴充卡上、或是 USB 裝置。

【課本補充】p.644：I/O 裝置也可以**主動**發起溝通：送一個 **interrupt**（中斷），打斷目前的程式、呼叫 interrupt service routine，再交給對應的 device driver 處理（interrupt 在第 44 支的 protocol layer）。處理器不需要知道裝置的細節，例如接 USB 相機時，它只要讓相機的 driver 和 USB port 順利溝通。

---

## 3. 硬體這邊：網路卡、主機板、chipset

[12:17]–[16:07]　課本 Fig 13.3、Definition 141–143（p.645–647）

**網路卡**（【口頭】[12:32]）：一塊卡，一邊的金手指接到主機板；上面黑色的方塊是晶片，有很多小晶片和一顆大的 **network processor**；network processor 提供一個 I/O port 接 **Ethernet 網路線**。「任何 I/O 系統都是這樣：外部的 port、一點處理、再接到主機板。」

**主機板**（【口頭】[13:43]，老師自己拍的照片）：
- 大塊綠色的板子是 **motherboard**（主機板）。
- 上面有主 CPU（上面有風扇散熱）、記憶體模組插槽、硬碟（接在這裡）、旁邊和上面有很多 I/O port（可能是 USB）。
- CPU 之外，主機板的其他部分大多是 **I/O 子系統**：在各個 I/O port、硬碟之間傳訊息，把資料拉近處理器和記憶體。
- 主機板上專門處理 I/O 訊息的那些晶片，合稱 **chipset**（晶片組）。

主機板 = chipset ＋ 主記憶體模組 ＋ 處理器 ＋ 外部 I/O 裝置和硬碟的連接器。

---

## 4. North Bridge 與 South Bridge

[16:07]–[20:56]　課本 Fig 13.4、Definition 144–145（p.647–649）

Intel 等廠商的參考架構：

```
                 GPU ─(PCI Express，很快)─┐
CPU ─(front side bus)─ North Bridge ─── DRAM 記憶體模組
                            │
                       South Bridge
                       ├─ 硬碟（SATA）
                       ├─ 音效、麥克風（Intel High Definition Audio，PCI）
                       ├─ 2–6 個 USB port（鍵盤、滑鼠……）
                       └─ 網路卡（PCI Express）
```

- **North Bridge**：接**高頻寬**的東西：記憶體模組、GPU。以前裡面有 **memory controller**。
- **South Bridge**：接**頻寬比較低**的東西：硬碟、網路卡、USB、音效。
- **Front side bus**：CPU 和 North Bridge 之間的匯流排。
- 【口頭】[17:21] 現在情況變了：**North Bridge 的功能已經搬進 CPU 晶片**，CPU 直接有 memory controller 控制記憶體模組；有些連 GPU 也搬進來了。South Bridge「即使在現在的處理器，也還在主機板上」。
- 【口頭】[20:11]「我只是先丟一些名字給大家（PCIe、SATA、USB……），讓下次聽到的時候覺得耳熟。」

**I/O bus**：一組線，在一組裝置之間傳資料和控制訊號，再加上一套協定。

【課本補充】p.646–649：
- Table 13.1（2012 年）各裝置的頻寬需求：顯示卡（PCIe 4）1–10 GB/s；硬碟（ATA／SCSI／SAS）150–600 MB/s；網路卡 10–100 MB/s；USB 60–625 MB/s；DVD 1–4 MB/s；音效 100 KB/s–3 MB/s；鍵盤滑鼠 **10–100 B/s**。差距非常大，所以要用不同的 bus。
- 從 **Intel Sandybridge** 開始，GPU 也搬進 CPU 晶片：因為有多的電晶體，而且晶片內溝通比晶片外快得多。
- 很多嵌入式處理器把大部分 South Bridge 和 port controller 都和 CPU 做在同一顆晶片：**SoC**（system on chip，系統單晶片）。

（本機的實際樣子見 [§18.1](#181-本機的-io-系統)。）

---

## 5. I/O 系統的四層

[20:56]–[24:54]　課本 §13.1.4、Fig 13.5（p.649–650）

【口頭】把通訊系統分層很常見，例如網際網路分成 **OSI 七層**。I/O 這個小網路也分層：

| 層 | 做什麼 |
|---|---|
| **Physical layer**（實體層） | 真正的訊號和電壓。兩個子層：**transmission**（電氣標準、用電壓編碼資料）、**synchronization**（訊號的時間，讓接收端讀得對） |
| **Data link layer**（資料連結層） | 假設能可靠地送一個 bit，那怎麼送一個 byte、一個大封包？把 bit 組成 **frame**、**buffering**（等整組 bit 到齊）、**錯誤修正**（雜訊可能讓一個 bit 出錯）、**transaction**（請求和回應、多則訊息的順序規則） |
| **Network layer**（網路層） | 怎麼**定址**、找到 I/O 裝置，把訊息送過去（**routing**，路由） |
| **Protocol layer**（協定層） | 在 I/O 系統上做複雜的大事：端到端的請求處理、interrupt 和 polling、**DMA**（direct memory access：讓 I/O 裝置不經過 CPU 直接寫記憶體） |

過了 data link layer，就可以確定訊息至少**跨過一跳**可靠地到了目的地。

---

## 6. Active high 與 active low

[24:54]–[28:08]　課本 §13.2.1（p.650–652）

時脈的負緣是一個週期的開始。要送 0 1 1 1 0 1 0 1 0 0 0：

| | 送 logical 1 | 送 logical 0 |
|---|---|---|
| **Active high** | 高電壓（V 伏特） | 0 伏特 |
| **Active low** | 0 伏特 | 高電壓 |

兩張圖剛好上下顛倒。電壓不一定是 0 和 1 伏特，可以是 −1、+1 伏特，或 −200 mV、+200 mV，只要清楚哪個電壓代表哪個狀態。

---

## 7. LVDS：用兩條線的電壓差

[28:08]–[36:45]　課本 §13.2.2、Fig 13.8（p.652–653）

**LVDS**（low voltage differential signaling，低電壓差動訊號）：用**兩條線**送一個訊號，看的是兩條線的**電壓差**。

- 一個固定的電流源打進 **3.5 mA**。
- 要送的值 A 決定哪兩顆電晶體打開：
  - A = 1：電流**往下**流過接收端的 **100 Ω** 電阻 → 電壓差 3.5 mA × 100 Ω = **+350 mV**。
  - A = 0：另外兩顆打開，電流**往上**流過同一個電阻 → **−350 mV**。
- 接收端用一個 **op-amp**（運算放大器）把這個小電壓差放大成其他電路用的電壓。
- 【口頭】[32:05]「這和 SRAM 的兩條平行 bit line 非常像」（[第 20 支](lecture20_note.md#5-解法pre-charge--sense-amplifier)）。

**為什麼電壓擺幅要小**：電路裡的電容要充放的電荷比較少，電流固定 → 花的時間少 → **快**。之後再放大就好。

**為什麼要兩條線**（【口頭】[33:22]）：
- 附近打開微波爐或手機，電磁**雜訊**會在線上感應出一點電壓（安培定律＋法拉第定律：電流變化產生變化的磁場，變化的磁場感應出電壓）。
- 只有一條線、在 ±350 mV 之間擺動時，如果雜訊多了 400 mV，就分不出是 0 還是 1。
- 兩條線靠得很近，雜訊會讓**兩條都差不多**升高，**電壓差**不太變。
- 這種一對線叫 **differential pair**（差動對），在 I/O 技術裡非常常見。

【課本補充】p.652：§13.2.1 的 **single-ended signaling**（單端訊號）就是只用一條線、相對地線的電壓；LVDS 的低擺幅讓它成為很快的實體層技術。

---

## 8. Physical bit 與 logical bit

[36:45]–[40:38]　課本 Definition 146–147（p.654–655）

| 詞 | 意思 |
|---|---|
| **Physical bit** | bus 上的 0 或 1，對應某個**電壓** |
| **Logical bit** | 真正要傳的訊息；可以是一串 physical bit 的**函數**（例如 logical 0 用 physical 00 表示） |
| **Bit period** | 傳一個 **logical** bit 的時間 |
| **I/O clock period** | I/O 時脈的一個週期 |
| **Binary signaling** | physical bit 只有 0、1 兩種 |
| **Ternary signaling** | 三種：0、1、**idle**（沒在傳任何東西） |

到目前為止都假設 physical bit = logical bit，接下來不是了。

LVDS 天生支援 ternary：電壓差的絕對值**小於門檻** → idle；大於門檻 → 正的是 1、負的是 0。

【課本補充】p.654–655：I/O clock 是 I/O 系統專用的時脈，通常和處理器時脈同步，但比較慢。大部分 I/O 協定的 bit period 和送的是 0 還是 1 無關，通常一個 bit period = 一個 I/O clock 週期。

---

## 9. Return to Zero（RZ）

[40:38]–[45:59]　課本 §13.2.4、Fig 13.9（p.655–656）

三種電壓：高、低、中（idle）。每個 bit period：

```
logical 1：前半格 高 → 後半格回到 idle
logical 0：前半格 低 → 後半格回到 idle
```

一個 logical bit 用**兩個 physical 狀態**表示，而且每個 bit period **一定有一次變化**：後半格一定「回到零」（idle），名字由來。

**為什麼要這麼麻煩**（【口頭】[43:32]）：
- active high 送一長串 1 時，電壓一直是高的。接收端只能拿**時間長度 ÷ 時脈週期**猜有幾個 1。
- 要讓傳送端和接收端用**完全一樣的時脈**非常難：時脈會有一點點差，還有 **clock skew**（同一個時脈送到兩個地方有時間差）。送 5 個 1，接收端可能讀成 4 個。
- RZ 每個 bit period 都有變化，接收端只要**數變化**就知道有幾個 bit，不用和傳送端 100% 同步。「時脈可以有點退到幕後。」

【課本補充】p.655–656：課本的 RZ 說法是送一個正（或負）的**脈衝**一小段時間，再讓線路回到 idle（讓電容放電，或加反向電壓）。用 binary 的話也可以「有脈衝 = 1、沒脈衝 = 0」，但接收端要看停頓多長來判斷 0，電路複雜。RZ 最大的缺點是**浪費頻寬**：每個 bit 都要停一下。

---

## 10. Manchester 編碼

[45:59]–[48:33]　課本 §13.2.5、Definition 148（p.656–657）

只用**兩種**電壓（比 RZ 簡單），用**變化的方向**表示 logical bit：

```
logical 1：physical 0 → 1（往上）
logical 0：physical 1 → 0（往下）
```

- 每個 bit period 都有一次變化 → 接收端從資料裡就能**推出時脈**，不用另外送時脈、也不用複雜的同步電路。
- 接收端只要有個電路記錄每次變化的方向。

【課本補充】p.656–657：
- 沒有變化 → 表示線路 idle。
- Definition 148：這種能從 physical bit 的變化裡取出傳送端時脈的訊號叫 **self clocked**（自帶時脈）。
- **IEEE 802.3**（今天 Ethernet 區域網路的基礎）用的就是 Manchester。
- 缺點：每個 bit 都有一次變化 → 每次都要對線路、driver 的電容充放電 → 電阻損耗變成熱 → **耗電**。

---

## 11. NRZ 與 NRZI

[48:33]–[56:15]　課本 §13.2.6–13.2.7（p.657–658）

**NRZ**（non return to zero，不歸零）：logical 0 = physical 0、logical 1 = physical 1，就是 active high，只是換個名字。

**變化的成本**（【口頭】[50:51]）：每次電壓改變都要有電流流動，電流流過電阻就有 **I²R** 的熱損耗。RZ 和 Manchester **每個 bit** 至少變一次，很耗電。NRZ 只有值改變時才變，但連續 100 個 1 時又回到「接收端可能讀成 99 或 101 個」的問題。

**NRZI**（non return to zero inverted，不歸零反轉）：介於 Manchester 和 NRZ 之間。

```
bit period 裡有變化（0→1 或 1→0 都算）→ logical 1
bit period 裡沒有變化               → logical 0
```

- logical 0 不是對應某個電壓，而是「這個 bit period 電壓沒變」。
- 和 Manchester 比，變化次數大約**少一半**（0 沒有變化）。
- 但還是有問題：連續 100 個 **0** 沒有任何變化，接收端一樣數不準。

**USB 的解法**（【口頭】[54:26]）：每傳 k 個原本的 bit，就**硬塞一個 1**（每 k + 1 個 bit 至少有一個 1，就至少有一次變化），接收端用這些變化校正自己的時脈，再把塞進去的 bit 丟掉。

⚠️ USB 的細節兩邊不一樣：影片說 USB 用 NRZI（變化 = 1）、每 k 個 bit 塞一個 1；課本 §13.7.4（p.695）說 USB 用的是**反過來**的 NRZI：**logical 0 才有變化**、logical 1 沒有變化，所以問題是連續的 1，做法是**每 6 個連續的 1 之後塞一個 0**。兩者想法一樣，都是保證每隔幾個 bit 一定有一次變化。USB 那段在第 45 支。

---

## 12. 同步子層：keep-out region

[56:15]–[59:40]　課本 §13.3（p.659）

接收端怎麼知道**什麼時候**讀資料？問題：
- 接收端和傳送端的時脈**頻率**可能不同，**相位**（phase）也可能不同（頻率一樣但錯開一點）。
- 訊號的延遲**不固定**（溫度會改變材料的特性）。
- **Keep-out region**（禁止區）：

```
          setup time │ hold time
    ─────────────────┼─────────────
                  負緣（時脈轉換）
    └──── keep-out region ────┘
```

- **setup time**：負緣**之前**的一小段時間；**hold time**：負緣**之後**的一小段時間。
- 輸入在這段時間裡改變，flip-flop 不保證存到新值，甚至可能進入 **metastable state**（亞穩態），值會**震盪**一陣子。
- 所以輸入的變化一定要避開 keep-out region。

---

## 13. Synchronous 與 mesochronous bus

[59:40]–[64:33]　課本 §13.3.1（p.659–662）

| | 假設 | 接收端 |
|---|---|---|
| **Synchronous bus**（同步匯流排） | 傳送端和接收端用**同一個**完美同步的時脈；訊號在接收端時脈進入 keep-out region **之前**就到 | 直接用 D flip-flop 存下來 |
| **Mesochronous bus**（中同步匯流排） | 時脈頻率相同，但有**固定、已知**的傳遞延遲，或已知的 **clock skew**（相位差）；訊號可能剛好落在 keep-out region | 加一個**可調的 delay element**，把變化推出 keep-out region |

**Delay element**（延遲元件）（【口頭】[61:40]）：
- 最簡單的：一串**反相器**（inverter），訊號反過來、再反回來……每經過一個就多一點延遲。⚠️ 老師先說「一串二極體」，馬上改口「對不起，是一串反相器」。
- **可調的**：在中間接一個多工器，選擇要不要**跳過**其中兩個反相器 → 延遲變小。

---

## 14. Source synchronous bus

[64:33]–[73:20]　課本 §13.3.2（p.662–663）

synchronous 和 mesochronous 都**不太實際**：它們假設兩邊的時脈同步得很好、延遲可以預測、相位差固定。實際上溫度變化、電磁干擾都會改變環境。

【口頭】[65:35] 老師正在用一台平板寫字，平板用大約 **1 公尺**的 USB 線接到桌機。以光速傳訊號來說 1 公尺很長，沒辦法保證兩個時脈一直同步，延遲稍微一變，可能就跳過幾個週期或落在 keep-out region。

**解法：傳送端把自己的時脈和資料一起送**（Tx clock）：
- 線路有延遲時，時脈和資料**一起**被延遲，兩者還是同步。（就像 LVDS 的兩條線一起受雜訊影響。）
- 接收端：
  1. 用**傳送端的時脈**把資料存進一個 D flip-flop：不會落在 keep-out region。
  2. 同時算出傳送端和接收端時脈的**相位差**，調整一個 tunable delay element。
  3. 經過延遲後，資料不會落在接收端時脈的 keep-out region，再用**接收端的時脈**存進另一個 flip-flop → 進入接收端的 **clock domain**（時脈域）。

「電路很簡單，但想法非常深遠。」大部分**高效能**的 bus 都是 source synchronous，因為延遲本來就不可預測。

---

## 15. Asynchronous bus 與 clock recovery

[73:20]–[75:44]　課本 §13.3.3（p.663）

最糟的情況：傳送端和接收端的時脈**完全沒有時間保證**，也**不送時脈**。

- 用 **Manchester**、**RZ**，或**加了塞位元的 NRZI** 這種有固定變化的編碼。
- 接收端有一個 **clock recovery** 電路：看資料裡的變化，**猜出**傳送端的時脈。
- 再把 physical bit 轉成 logical bit（圖上沒畫這個方塊）。
- 之後和 source synchronous 一樣：算相位差、可調延遲、用接收端時脈存進接收端的 clock domain。

---

## 16. Four-phase 與 two-phase handshake

[75:44]–[85:14]　課本 §13.3.3（p.665–666）

asynchronous bus 上另一種做法：多兩條控制線 **strobe**（選通，傳送端說「資料好了」）和 **ack**（確認，接收端說「讀到了」）。binary bus 沒有 idle 狀態，要先告訴接收端「資料來了」；而且兩邊完全不同步，傳送端要知道接收端讀了才能送下一筆。

**Four-phase handshake**（四相交握）：

```
① 傳送端把資料放到 bus，再拉高 strobe
② 接收端看到 strobe，讀資料，拉高 ack
③ 傳送端看到 ack，拿掉資料、放下 strobe
④ 接收端看到 strobe 放下，放下 ack
→ 傳送端看到 ack 放下，才開始下一筆
```

- 為什麼**先放資料、再拉 strobe**：資料要時間穩定；先拉 strobe，接收端醒來可能還看不到資料。
- 優點：完全不同步也能傳。缺點：訊息很多、中間還要停頓 → **沒效率**。只在同步程度非常差時用。

**Two-phase handshake**（兩相交握）：

```
① 傳送端放資料，**切換**（toggle）strobe
② 接收端讀資料，**切換** ack
→ 傳送端看到 ack 變了，馬上放下一筆資料、再切換 strobe……
```

- 關鍵：不用「拉高再放下」，**切換**（0 變 1 或 1 變 0）就能表示狀態改變。
- 每筆只要**兩個**階段，訊息比較少，而且資料之間**沒有空檔**。
- 代價：兩邊各要一個小狀態機，記住 strobe、ack 上次的值，「很簡單」。

【口頭】[84:23] 實體層讓我們能從傳送端把 bit 可靠地送到接收端：可能在不同的 clock domain、用不同的編碼和電壓，但最後所有的 logical bit 都到了接收端。過程中可能有錯，那是 data link layer 的事。

---

## 17. 【原始碼】Linux 的 read system call

影片 §2 說應用程式用 `read` 這個 system call 讀 I/O 裝置。Linux 裡它的入口，`linux-src/fs/read_write.c:621-624`：

```c
SYSCALL_DEFINE3(read, unsigned int, fd, char __user *, buf, size_t, count)
{
	return ksys_read(fd, buf, count);
}
```

`ksys_read`（`linux-src/fs/read_write.c:602-619`）用 **file descriptor** 找到檔案，再呼叫 `vfs_read`；`vfs_read` 最後呼叫這個檔案自己的讀取函式，`linux-src/fs/read_write.c:467-472`：

```c
	if (file->f_op->read)
		ret = file->f_op->read(file, buf, count, pos);
	else if (file->f_op->read_iter)
		ret = new_sync_read(file, buf, count, pos);
	else
		ret = -EINVAL;
```

- `file->f_op` 是這個「檔案」的操作函式表。如果檔案其實是一個裝置（`/dev/...`），這張表就是那個**裝置驅動程式**提供的。
- 這就是影片的 I/O stack：application → system call（`SYSCALL_DEFINE3(read, ...)`）→ kernel（`ksys_read`、`vfs_read`）→ **device driver**（`f_op->read`）。「所有裝置都是檔案」就是靠這層 `f_op` 做到的。

---

## 18. 【實測】

程式在 `experiments/lecture43/`。

### 18.1 本機的 I/O 系統

**字元裝置與區塊裝置**：

```
$ ls -l /dev/nvme0n1 /dev/sda /dev/input/event0 /dev/null /dev/urandom /dev/tty0
crw-rw---- 1 root input  13, 64 Oct 25 15:37 /dev/input/event0
crw-rw-rw- 1 root root    1,  3 Oct 25 15:37 /dev/null
brw-rw---- 1 root disk  259,  0 Oct 25 15:37 /dev/nvme0n1
brw-rw---- 1 root disk    8,  0 Oct 25 15:37 /dev/sda
crw--w---- 1 root tty     4,  0 Oct 25 15:37 /dev/tty0
crw-rw-rw- 1 root root    1,  9 Oct 25 15:37 /dev/urandom
```

第一個字母 **c** = character device（輸入裝置 event0、終端機 tty0、/dev/urandom），**b** = block device（NVMe SSD、硬碟 sda）。和影片的分類一樣。

**真的去看 read system call**：

```
$ strace -e trace=openat,read head -c 16 /dev/urandom
openat(AT_FDCWD, "/dev/urandom", O_RDONLY) = 3
read(3, "\331\2420\\\247\351\24\276\344:+\10\211\254\336\213", 16) = 16
```

`head` 打開 /dev/urandom 拿到 **file descriptor 3**，再 `read(3, buffer, 16)`，讀到 16 bytes：就是影片 §2 的 `read(file_descriptor, buffer, num_bytes)`。

**主機板上的 chipset**（`lspci`，節錄，加上我的註解）：

```
0000:00:00.0 Host bridge: Intel Corporation Device a72a
0000:00:01.0 PCI bridge: Intel Corporation Device a70d          ← 接到下面的 NVIDIA GPU
0000:00:02.0 VGA compatible controller: Intel Corporation Device a78b   ← CPU 內建的 GPU
0000:00:14.0 USB controller: Intel Corporation Device 7a60
0000:00:14.3 Network controller: Intel Corporation Device 7a70  ← Wi-Fi
0000:00:1c.0 PCI bridge: Intel Corporation Device 7a3e          ← 接到下面的 Ethernet
0000:00:1f.0 ISA bridge: Intel Corporation Device 7a0c
0000:00:1f.3 Multimedia audio controller: Intel Corporation Device 7a50
0000:01:00.0 VGA compatible controller: NVIDIA Corporation Device 28e0
0000:02:00.0 Ethernet controller: Realtek ... RTL8111/8168/8411 PCI Express Gigabit Ethernet Controller
10000:e1:00.0 Non-Volatile memory controller: Samsung Electronics Co Ltd Device a80b
```

- 裝置編號 **a7xx** 的一群（host bridge、內建 GPU、接 NVIDIA 的 PCIe bridge）看起來是 **CPU 晶片裡的部分**：就是影片說的「North Bridge 搬進 CPU」，獨立 GPU 直接接在 CPU 的 PCIe 上。
- 編號 **7axx** 的一群（USB、Wi-Fi、音效、ISA bridge、接 Ethernet 的 PCIe bridge）看起來是另一顆 **chipset 晶片**（相當於 South Bridge）：USB、網路、音效都在這邊，和影片的參考架構一致。
- ⚠️ 用裝置編號分哪些在 CPU、哪些在 chipset 是我的推測（本機的 lspci 名稱資料庫太舊，只顯示「Device xxxx」），「待補」。

### 18.2 編碼與時脈不準

`encode.py`：把 600 個 bit（中間有**連續 100 個 1** 和**連續 100 個 0**）用不同方式編成電壓，再交給一個**時脈慢了 2%** 的接收端。接收端每看到一次電壓變化就重新對齊，兩次變化之間只能靠自己不準的時脈數格子：

```
$ python3 -I encode.py
訊息：600 個 bit（中間有連續 100 個 1 和連續 100 個 0），接收端時脈慢 2%
  active-high  電壓變化  215 次 | 解出 596 個 bit | 錯了
  RZ           電壓變化 1199 次 | 解出 600 個 bit | 全對
  Manchester   電壓變化  984 次 | 解出 600 個 bit | 全對
  NRZI         電壓變化  296 次 | 解出 598 個 bit | 錯了
  NRZI+塞位元   電壓變化 315 次 | 多傳 19 個 bit | 解出 600 個 bit | 全對
```

- **active high（NRZ）**：100 個 1 中間沒有任何變化，接收端數成 98 個……總共少了 4 個 bit。正是影片說的「100 個 1 可能讀成 99 或 101 個」。
- **RZ**、**Manchester**：每個 bit 都有變化，接收端一直重新對齊，**全對**；代價是變化次數最多（RZ 1199 次、Manchester 984 次）→ 最耗電。
- **NRZI**：變化少很多（296 次），但連續 100 個 0 沒有變化，又少了 2 個 bit。
- **NRZI＋塞位元**（每 6 個連續 0 塞一個 1）：只多傳 19 個 bit、多 19 次變化，就**全對**了。這就是 USB 的做法：變化次數接近 NRZI，可靠度接近 Manchester。

---

## 19. 一頁速查表

| 主題 | 重點 |
|---|---|
| I/O 裝置 | 鍵盤、滑鼠、螢幕、印表機、網路卡、硬碟（算 I/O） |
| I/O port | 接外部連接器的金屬接腳（USB、DVI） |
| System call | 作業系統給應用程式的函式；read、write（file descriptor、buffer、bytes） |
| /dev | Linux 把裝置都當檔案；character device（一次一字元）、block device（一次一塊） |
| I/O stack | application → system call → kernel → device driver → 處理器 → I/O 系統 → 裝置 |
| Chipset | 主機板上處理 I/O 的晶片 |
| North Bridge | 記憶體、GPU（高頻寬）；現在多半搬進 CPU |
| South Bridge | 硬碟、USB、網路、音效（低頻寬） |
| 四層 | physical（transmission、synchronization）、data link、network、protocol |
| Active high／low | 高電壓 = 1／高電壓 = 0 |
| LVDS | 3.5 mA × 100 Ω = ±350 mV；兩條線看電壓差 → 快、抗雜訊 |
| Physical／logical bit | 電壓／真正的訊息；logical 可以是 physical 的函數 |
| Ternary | 0、1、idle |
| RZ | 前半格高或低、後半格回 idle；每 bit 一次變化；浪費頻寬 |
| Manchester | 0→1 = 1、1→0 = 0；self clocked；Ethernet 用；耗電 |
| NRZ | 電壓 = 值；變化最少；長串相同 bit 會數錯 |
| NRZI | 有變化 = 1；變化少一半；長串 0 會數錯 → 塞位元 |
| Keep-out region | setup time ＋ hold time；在這裡改輸入 → metastable |
| Synchronous | 同一個完美時脈 |
| Mesochronous | 頻率相同、已知延遲或 skew → 可調 delay element（一串反相器＋MUX） |
| Source synchronous | 傳送端連時脈一起送；高效能 bus 都用 |
| Asynchronous | 沒有時脈保證 → clock recovery |
| Four-phase | 資料＋strobe ↑ → ack ↑ → strobe ↓ → ack ↓ |
| Two-phase | 用切換（toggle）代替拉高放下；兩階段、沒有空檔 |
| 實測 | /dev 的 c／b；strace 看到 read(3, …, 16)；時脈慢 2% 時 NRZ、NRZI 掉 bit，Manchester、NRZI＋塞位元全對 |

---

## 20. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| Data link（frame、錯誤偵測與修正、仲裁）、network（I/O port、memory-mapped）、protocol（polling、interrupt、DMA） | [第 44 支](lecture44_note.md)：Data Link, Network, and Protocol Layers |
| PCI Express、SATA、SCSI、USB（反過來的 NRZI、塞 0）、FireWire、硬碟 | [第 45 支](lecture45_note.md)：PCI-X, USB, and hard disks |
| 光碟、SSD | [第 46 支](lecture46_note.md) |
| D flip-flop、setup／hold time | [第 18–20 支](lecture20_note.md)：Digital Logic（課本 Ch7） |
| SRAM 的兩條 bit line（差動） | [第 20 支](lecture20_note.md#5-解法pre-charge--sense-amplifier) |
| 匯流排、interconnect | [第 42 支](lecture42_note.md) |
| system call、device driver、kernel | 作業系統課程 |

---

## 附：本講一句話總結

> **電腦靠 I/O 裝置（鍵盤、滑鼠、螢幕、網路卡，甚至硬碟）和外界互動：軟體這邊，應用程式用 read、write 這種 system call 把請求交給 kernel，kernel 再交給 device driver，經過處理器和 I/O 系統到裝置（Linux 把裝置都當成 /dev 下的檔案，分 character 和 block device）；硬體這邊，主機板上的 chipset 分成接記憶體和 GPU 的 North Bridge（現在多半搬進 CPU）和接硬碟、USB、網路、音效的 South Bridge。I/O 系統分成 physical、data link、network、protocol 四層；physical layer 的 transmission 子層決定怎麼用電壓表示 bit：active high／low、用兩條線電壓差抗雜訊又快的 LVDS（±350 mV），以及每個 bit 都有變化、能從資料裡取出時脈但耗電的 RZ 和 Manchester，變化最少但長串相同 bit 會數錯的 NRZ，和用「有變化 = 1」再定期塞位元的 NRZI；synchronization 子層則要讓資料避開 flip-flop 的 keep-out region（setup＋hold time），從 synchronous、mesochronous（可調延遲）、source synchronous（連時脈一起送）到 asynchronous（clock recovery，或用 strobe／ack 做 four-phase、two-phase handshake）。實測 /dev 裡看得到 c 和 b 兩種裝置、strace 看得到 read(3, buf, 16)，模擬時脈慢 2% 的接收端在長串相同 bit 時 NRZ 和 NRZI 都會掉 bit，Manchester 和加了塞位元的 NRZI 則全對。**

---

*筆記依據 Smruti R. Sarangi「Basic Computer Architecture」Chapt 12: I/O system: overview and physical layer, Part 1/4 錄影逐字稿（YouTube 自動字幕，1:25:14）、Sarangi *Basic Computer Architecture* v3.09 Ch13 §13.1–13.3（p.641–666）、Linux v6.2.12 `fs/read_write.c` 整理。實測為本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、Python 3.10）結果。*
