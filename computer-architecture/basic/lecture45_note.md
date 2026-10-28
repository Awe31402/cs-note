# Basic Computer Architecture 第四十五支影片完整筆記：PCI Express、USB、硬碟與 RAID（Chapter 12, Part 3/4 → 課本 Ch13）

> **課程**：Basic Computer Architecture — Chapt 12: PCI-X, USB, and hard disks, Part 3/4（播放清單第 45 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：YouTube 錄影逐字稿（1:13:42，自動字幕）＋ 課本 *Basic Computer Architecture* v3.09 **Ch13** §13.7.1、§13.7.4、§13.8–13.8.2（p.688–711）
> **章號**：影片的 Chapter 12 是課本 v3.09 的 **Chapter 13**；影片說的「chapter 3」（不以 bit 為單位存取）是課本 Ch3。
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
- [1. PCI Express 的由來](#1-pci-express-的由來)
- [2. PCI Express：lane 與 data striping](#2-pci-expresslane-與-data-striping)
- [3. PCI Express 的各層](#3-pci-express-的各層)
- [4. USB：拓樸與接頭](#4-usb拓樸與接頭)
- [5. USB 的各層](#5-usb-的各層)
- [6. 儲存媒體](#6-儲存媒體)
- [7. 硬碟怎麼存 bit：小磁鐵與 NRZI](#7-硬碟怎麼存-bit小磁鐵與-nrzi)
- [8. 硬碟的結構：platter、track、sector](#8-硬碟的結構plattertracksector)
- [9. 讀一次硬碟要多久](#9-讀一次硬碟要多久)
- [10. Logical block address 與壞軌](#10-logical-block-address-與壞軌)
- [11. RAID 0 與 RAID 1](#11-raid-0-與-raid-1)
- [12. RAID 2、3、4](#12-raid-234)
- [13. 小寫入只要改兩個 block](#13-小寫入只要改兩個-block)
- [14. RAID 5 與 RAID 6](#14-raid-5-與-raid-6)
- [15. 【原始碼】Linux 的 RAID 6](#15-原始碼linux-的-raid-6)
- [16. 【實測】](#16-實測)
- [17. 一頁速查表](#17-一頁速查表)
- [18. 與後續講次的連結](#18-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

```
PCI Express（主機板上的 bus）：point-to-point、1–32 條 lane、LVDS、8b/10b、source synchronous、
                             1 byte frame、32-bit CRC、split transaction、250 MB/s／lane、switch
USB（接外部裝置）：hub 組成的樹、127 個裝置、4 pin（5V、GND、D+、D−）、
                  NRZI（0 = 變化）＋每 6 個 1 塞一個 0、CRC、7-bit 裝置 + 4-bit endpoint、
                  control／interrupt／bulk／isochronous、message pipe／stream pipe
硬碟：小磁鐵＋NRZI；platter、track、sector（512 B）；T = seek + rotational latency + transfer
RAID：0（striping）、1（mirror）、2／3／4（parity 碟）、5（parity 輪流）、6（兩個 parity）
```

---

## 1. PCI Express 的由來

[00:00]–[02:33]　課本 §13.7.1（p.688–690）

主機板上有 CPU 晶片和很多 I/O 卡，需要一條 bus 把它們接到 CPU。

- 90 年代末流行兩種：**PCI**（peripheral component interconnect）和 **AGP**（accelerated graphics port，給顯示卡）。
- 現在大部分 GPU 都搬進 CPU 晶片，AGP 退流行了。【口頭】[02:00]「我要稍微收回這句話」：高效能遊戲機還是有獨立 GPU，也要接到 CPU。
- 標準化之後產生了 **PCI Express**（PCIe），PCI 和 AGP 被淘汰，新系統不再用。

---

## 2. PCI Express：lane 與 data striping

[02:33]–[05:22]　課本 p.689

**Lane**（通道）：一條高速**序列**（serial）bus，一次只送一個 bit。
- 為什麼不用平行的線：頻率非常高，連續兩個 bit 之間的時間很短；好幾條平行的線各有**不一樣的延遲**，很難同步（不是不可能，但通常要慢一點）。
- 一條 lane 是**單 bit、全雙工**的通道。

**拓樸**：**point-to-point**（點對點），兩個節點（例如 GPU 和 CPU）之間有 **1 到 32 條 lane**。
- 多條 lane：用 **data striping**（資料條帶化）把一個封包的 bit 分到不同的 lane 上平行送；接收端要把各 lane 的資料重新對齊。

【課本補充】p.689：
- 8 條 lane 送一個 8-bit 封包，可以每條 lane 送一個 bit。
- 這和傳統的平行 bus **不一樣**：平行 bus 所有線共用同一套同步和時序；PCIe 每條 lane 各自同步、各自計時，data link layer 負責把各 lane 的 bit 重組。
- 一條 lane 是兩條 LVDS 線（課本寫一條往一個方向、另一條往反方向）。
- 樹狀拓樸的內部節點是 **PCIe switch**，把多個裝置的流量多工。

---

## 3. PCI Express 的各層

[05:22]–[08:45]　課本 Table 13.4（p.690）

| 層 | PCI Express |
|---|---|
| Physical：訊號 | **LVDS** 差動訊號（每條 lane 兩條線，看電壓差，[第 43 支](lecture43_note.md#7-lvds用兩條線的電壓差)） |
| Physical：編碼 | **8b/10b**：每 8 bit 轉成 10 bit，多出的 bit 提供一點錯誤偵測與更正能力，對 bit flip 比較強 |
| Physical：時序 | **source synchronous**（時脈和訊號一起送） |
| Data link：frame | **1 byte** |
| Data link：錯誤 | **32-bit CRC**（對 burst error 很有用） |
| Data link：transaction | **split transaction**（高效能 bus 都要，不能一個 transaction 就鎖住 bus） |
| 頻寬 | **250 MB/s／lane** |
| Network | 用 **switch** 在點對點連結之間路由 |

【口頭】[05:39]「8b/10b 我們沒教，讀者可以上網查，很多 bus 都用。」

【口頭】[07:28] 250 MB/s 「不算超快，也不算慢」：一張 DVD 大約 5 GB = 5000 MB，以這個速度讀完要 5000 ÷ 250 = **20 秒**。還有更快的，例如光學 bus。

⚠️ 影片標題寫「**PCI-X**」，內容講的是 **PCI Express**（PCIe）。另外 250 MB/s 是最早的版本，本機的 PCIe 連結速度見 [§16.1](#161-本機的-pci-express-與-usb)。

---

## 4. USB：拓樸與接頭

[08:45]–[14:08]　課本 §13.7.4（p.693–695）

**USB**（universal serial bus，通用序列匯流排）：幾乎所有筆電、桌機、手機都有，已經是**接各種周邊**的實質標準：滑鼠、掃描器、印表機、手機、隨身碟……「到現在都還沒成為目前裝置的瓶頸」。它**不是**主機板上的 bus。

**樹狀拓樸**（【口頭】[09:55]）：
- 一個 USB port（host）可以接一個 **hub**，hub 有 4 個 port，可以再接裝置或別的 hub。
- 內部節點都是 hub。
- 一個 USB port 最多接 **127 個裝置**（包括 hub），最多 **5 層**深。

**接頭**：
| 接頭 | pin |
|---|---|
| 標準 USB | **4 個**：**VCC**（5 V 直流電源）、**GND**、**D+**、**D−**（差動對，像 LVDS；平均 3.3 V，擺幅約 3.3 V） |
| mini／micro USB（手機、平板） | **5 個**：多一個 **ID** pin |

- **為什麼 USB 要供電**（【口頭】[11:33]）：小滑鼠、繪圖板、隨身碟本身沒有電池，要靠 USB 供電才能動。附帶的好處：可以用筆電幫手機充電，「這本來不是設計的目的」。
- **ID pin**（【口頭】[13:14]，老師當場看自己手機的接頭）：分辨 **device** 和 **host**。例如滑鼠接電腦，滑鼠是 device、電腦是 host。在裝置樹裡要打破對稱、給資料一個方向（往 host 還是往 device）。

【課本補充】p.694–695：
- 設計者一開始把裝置分成三類：low speed（鍵盤、滑鼠）、full speed（高解析度音效）、high speed（掃描器、攝影機）。
- hub 大多是被動的；hub 可以由 host 供電，也可以自己供電。USB 一個 port 最多供 500 mA，以 100 mA 為單位分配。
- 裝置從 hub 拔掉時，hub 要偵測到並通知處理器，這時 hub 是主動的。

---

## 5. USB 的各層

[14:08]–[25:43]　課本 §13.7.4、Table 13.7（p.694–696）

| 層 | USB |
|---|---|
| Physical：訊號 | **LVDS** 差動 |
| Physical：編碼 | **NRZI**，但「**變化 = logical 0**」（和一般的 NRZI 相反） |
| Physical：時序 | **asynchronous**：從 NRZI 的變化回推時脈；連續 6 個 1 就塞一個 **0**（保證有變化） |
| Data link：frame | **46 到 1058 bit**，看流量種類 |
| Data link：錯誤 | **CRC** |
| Data link：transaction | **split transaction** |
| Network：位址 | 每個裝置 **7-bit id**（最多 2⁷ = 128，所以 127 個裝置）＋ **4-bit endpoint**（port）= **11-bit** 位址；每個裝置 16 個輸入、16 個輸出 endpoint |
| Network：拓樸 | hub 的樹：每個 hub 1 個 upstream（往 host）、最多 4 個 downstream |
| 雙工 | **half duplex**：傳送端和接收端不能同時說話 |

**四種流量**：

| 種類 | 用途 |
|---|---|
| **Control** | 剛接上時設定裝置，讓它能和電腦溝通 |
| **Interrupt** | 少量、要馬上處理的資料：例如按一個鍵 |
| **Bulk** | 大量資料、frame 很大：例如印表機 |
| **Isochronous** | 固定、事先知道的速率：例如網路攝影機 |

這些概念橫跨好幾層：physical layer 都一樣，從 data link layer 開始依流量種類有不同（frame 大小等）。

**三種速度**：

| | 速度 | 用途 |
|---|---|---|
| low speed | **192 KB/s** | 滑鼠、鍵盤（資料本來就少） |
| full speed | **1.5 MB/s** | 網路攝影機這類有一點影片的裝置 |
| high speed | **60 MB/s** | 高畫質裝置 |

**Protocol layer 的兩種 pipe**：

| | 結構 | 例子 |
|---|---|---|
| **Stream pipe** | 沒有結構，資料就這樣流進電腦，不在乎可靠或速率 | 感測器、簡單的單向網路攝影機 |
| **Message pipe** | 有結構、有 **handshake**（接收端能告訴傳送端收到了），比較常用 | 隨身碟這種要來回很多 transaction 的 |

- 驅動程式送訊息給裝置對應的 endpoint，就和 x86 主機板上「每個裝置定義一組 port」一樣（[第 44 支](lecture44_note.md#8-network-layerio-port)）。
- 【口頭】[20:51] 為什麼是**序列**的：和 PCIe 一樣，多個 bit 很難同步；而且外接的線**很長**，雜訊很多，平行送反而嚴重限制頻寬。

⚠️ NRZI 的方向：上一講（[第 43 支 §11](lecture43_note.md#11-nrz-與-nrzi)）老師說 USB「每 k 個 bit 硬塞一個 1」；這一講改成和課本一樣：**變化代表 0**，**每 6 個連續的 1 塞一個 0**。

【課本補充】p.695–696：USB 用 bit stuffing（專門的 frame 開始和結束符號）標出 frame 的邊界；USB hub 的仲裁很複雜，因為流量和裝置種類很多。USB 3.0 有 super speed（最多 384 MB/s，課本說 2012 年還不普及）。

---

## 6. 儲存媒體

[25:43]–[27:04]　課本 §13.8（p.697–700）

電腦關機時，處理器和 DRAM 裡的資料全部消失，但檔案、圖片還在，因為有**永久性**的儲存媒體：**硬碟**、**光碟**（CD、DVD）、**固態硬碟**（SSD，USB 隨身碟等）。這講講硬碟，下一講講光碟和 SSD。

---

## 7. 硬碟怎麼存 bit：小磁鐵與 NRZI

[27:04]–[30:52]　課本 §13.8.1（p.701–702）

硬碟用**一連串非常小的磁鐵**存資料，每個磁鐵有 N 極和 S 極。

- 讀取頭（一小塊金屬）在磁鐵上移動：金屬在磁場中移動會感應出電動勢（EMF）。
- 兩個相鄰磁鐵**方向一樣**：磁場沒有變化（或變化小）→ **logical 0**。
- 兩個相鄰磁鐵**方向相反**（N-S 接 S-N）：接面處磁場**反轉**，讀取頭經過時感應出比較大的電壓跳動 → **logical 1**。

這就是 **NRZI**：有變化是 1、沒變化是 0（[第 43 支](lecture43_note.md#11-nrz-與-nrzi)），也要加 dummy bit 避免失去同步。「NRZI 不一定是電壓訊號，也可以是**磁鐵的方向**。」

【口頭】[30:04] 一顆硬碟比手掌大一點，現在就能存 **1 TB**（1024 GB，大約 10¹² bytes）：磁鐵非常非常小，一個磁性表面上有數十億個。

---

## 8. 硬碟的結構：platter、track、sector

[30:52]–[40:35]　課本 Definition 160、Fig 13.29–13.32（p.702–706）

| 詞 | 意思 |
|---|---|
| **Platter**（盤片） | 一片金屬圓盤，中間有洞（甜甜圈形，「很像一片 CD」），洞是為了讓它旋轉 |
| **Track**（磁軌） | 盤片上的一圈**同心圓**，上面排著很多小磁鐵 |
| **Sector**（磁區） | track 的一段（某個角度範圍）；大小固定，常見 **512 bytes** |

**結構**（Fig 13.32）：
- 好幾片 platter 用一根 **spindle**（主軸）串在一起，由 **spindle motor** 帶動高速旋轉：**5400 RPM**（每秒 90 轉）或 **7200 RPM**，**角速度固定**（constant angular velocity）。
- 每個 platter 有上下兩個**記錄面**，每個記錄面有一支 **arm**（臂），尾端是 **read/write head**（讀寫頭）。讀：感測磁場變化；寫：產生磁場讓小磁鐵轉向。
- 讀寫頭的核心是一小段**金屬線圈**，磁場變化在兩端感應出小電壓，由讀寫頭的電子電路感測。
- 所有 arm 接到同一根軸，由 **actuator**（致動器）**順時針或逆時針轉一點**，讓讀寫頭對準正確的 track：讀寫頭是沿著**圓弧**移動，不是沿著半徑滑動。
- 一次只從**一個**記錄面讀：同時協調四個讀寫頭太難。
- **drive electronics** 控制馬達、處理資料，經過 **bus interface**（例如 PCIe）送到 CPU。

【口頭】[39:00] 圖上的 arm 應該畫長一點，讓讀寫頭能到最內圈的 track，「希望讀者原諒我」。

⚠️ 轉速：影片 [41:19] 說「幾乎 100% 的硬碟都是固定角速度」；課本 p.704 也說用固定角速度，但 p.705 又說 spindle motor「依要存取的 zone 調整速度」，兩處不太一致。

【課本補充】p.702–706：
- **Zoned-Bit Recording（ZBR）**：每圈 track 放一樣多的 sector 會浪費外圈的空間；所以把 track 分成好幾個 **zone**，同一個 zone 裡每圈的 sector 數一樣，外圈的 zone 放比較多 sector。
- 桌機硬碟大約每英吋 **10,000 條 track**，track 間距只有 **2.5 µm**，actuator 必須極度精確；sector 上有記號標示 track 編號，actuator 會微調對準，這叫 **servo control**。
- 讀寫頭停在 track 上後，一直讀 sector 上的編號記號，能準確預測想要的 sector 什麼時候轉到下面。
- 硬碟裡也有小處理器：收發 bus 上的資料、排程請求、做錯誤更正。「硬碟是人類工程不可思議的成就。」

---

## 9. 讀一次硬碟要多久

[40:35]–[42:50]　課本 Eq 13.8、Definition 161、Example 157（p.706–707）

```
T_disk access = T_seek + T_rot latency + T_transfer
```

| 部分 | 意思 |
|---|---|
| **Seek time**（尋軌時間） | actuator 轉動讀寫頭、對準正確 track |
| **Rotational latency**（旋轉延遲） | 等想要的 sector 轉到讀寫頭下面 |
| **Transfer time**（傳輸時間） | 從讀寫頭在 sector 的第一個 bit 開始：讀出資料、錯誤檢查與更正、組 frame、送到 CPU |

【課本補充】Example 157（p.707）：平均 seek time 12 ms、600 RPM、頻寬 10⁶ B/s，讀 10,000 個連續的 byte：
```
T_seek = 12 ms
轉一圈 100 ms，平均要等半圈 → T_rot latency = 50 ms
T_transfer = 10⁴ / 10⁶ s = 10 ms
總共 72 ms
```
（rotational latency 取半圈，因為讀寫頭和目標 sector 之間的角度差假設平均分布在 0°–360°。）

【實測】換成真實硬碟的轉速：5400 RPM 轉一圈 11.11 ms，平均 rotational latency **5.56 ms**；7200 RPM 轉一圈 8.33 ms，平均 **4.17 ms**。光是等磁區轉過來就要好幾毫秒，是 DRAM 存取（約 90 ns，[第 20 支](lecture20_note.md#13-實測)）的**五萬倍**以上。

---

## 10. Logical block address 與壞軌

[42:50]–[50:10]　課本 p.706

- 軟體用 **logical block address**（邏輯區塊位址）定址：一個 **block** = 一個 **sector**（軟體叫 block、硬碟叫 sector）。例如 128 GB 的硬碟，在軟體看來是一個從 0 到 128 GB − 1 的大 byte 陣列，切成 512-byte 的 block。
- 硬碟內部把 logical address 轉成 **physical address**：哪個**記錄面**、哪條 **track**、哪個 **sector**。
- 大部分硬碟拿**一個記錄面**專門存這張對應表；再加一個小的 **DRAM cache** 存最近用過的對應，減少查表時間。（「有點像 virtual memory」，[第 37 支](lecture37_note.md)。）
- **壞掉的 sector**（磁鐵失去磁性）：記錄在那個專用的記錄面上，把 logical block 改對應到別的健康的 sector。
- 外面只看得到「一個大 byte 陣列」的抽象，內部的記錄面、track、sector 完全藏起來，所以可以這樣偷偷換。

**硬碟的弱點**（【口頭】[47:49]）：
- 強電磁場可能讓資料消失。
- **機械零件很多**：馬達、spindle、platter、actuator、arm。讀寫頭離盤面只有**幾奈米**，摔到或溫度升高，故障機率就變得很高。
- 壞軌重新對應可以保住「位址」，但**資料**還是丟了。銀行的一顆硬碟壞了，不能讓客戶的帳戶資料就這樣不見 → **RAID**。

---

## 11. RAID 0 與 RAID 1

[50:10]–[55:34]　課本 §13.8.2、Fig 13.33–13.34（p.707–709）

**RAID**（redundant array of inexpensive disks，便宜磁碟的冗餘陣列）：不用一顆硬碟，用**好幾顆**。兩個好處：
- **可靠度**：一顆壞了還能復原。
- **頻寬**：一顆硬碟一次只能讀一個 block；好幾顆可以**平行**讀。

**RAID 0**：
```
碟 1：B1  B3  B5 …      （奇數 block）
碟 2：B2  B4  B6 …      （偶數 block）
```
- 這樣把資料分到好幾顆碟上叫 **data striping**。
- B3 和 B4 可以同時讀 → 頻寬加倍（可以用 DMA 直接寫進記憶體）。
- 缺點：**沒有**多的可靠度；剛好要同時讀兩個奇數 block 就不能平行。

**RAID 1**：
```
碟 1：B1  B2  B3  B4  B5
碟 2：B1  B2  B3  B4  B5      （完全一樣的鏡像）
```
- 一顆碟**整個壞掉**系統也照常跑。換一顆新的碟，把另一顆的內容全部複製過去。
- **任兩個** block 都能平行讀（B1 和 B2、B1 和 B3 都行）→ 頻寬加倍。
- 代價：**100%** 的額外儲存空間，總容量變一半。

---

## 12. RAID 2、3、4

[55:34]–[63:21]　課本 Fig 13.35（p.709–710）

不想付 100% 的代價：4 顆**資料碟**＋1 顆 **parity 碟**。

```
P1 = B1 ⊕ B2 ⊕ B3 ⊕ B4       （同一列的 4 個 block 做 XOR）
```

- 任何**一顆**碟壞了（例如碟 3）：B3 = B1 ⊕ B2 ⊕ B4 ⊕ P1，重建出來，換上新碟填回去。
- 假設 **fail-stop**（壞了就停、系統知道是哪顆壞）。【口頭】[59:05] 老師的儲存設備上，一顆碟壞了會**閃橘燈**，拔掉換新的，再用其他碟和 parity 重建。
- **兩顆**同時壞就救不回來。

三種只差在 block 多大：

| | striping 的單位 | 問題 |
|---|---|---|
| **RAID 2** | **1 bit** | 我們很少以 bit 為單位存取資料（第 3 章），存取變得很複雜 |
| **RAID 3** | **1 byte** | 寫 256 bytes 要寫到**所有**碟（第 1 byte 碟 1、第 2 byte 碟 2……）再算 parity → 所有碟的流量都很大 |
| **RAID 4** | **一個 block（512 bytes）** | 實用：寫 256 bytes 只要動一顆資料碟和 parity 碟 |

RAID 2、3 只存在於光譜上，實際上用的是 RAID 4。

---

## 13. 小寫入只要改兩個 block

[63:21]–[69:31]　課本 p.710

寫 B1（變成 B1′）時，要重算 parity，但**不用讀 B2、B3、B4**：

```
P  = B1 ⊕ B2 ⊕ B3 ⊕ B4
B1 ⊕ P = B1 ⊕ B1 ⊕ B2 ⊕ B3 ⊕ B4 = B2 ⊕ B3 ⊕ B4     （x ⊕ x = 0、0 ⊕ x = x）
P′ = B1′ ⊕ B2 ⊕ B3 ⊕ B4 = B1′ ⊕ (B1 ⊕ P)
   = P ⊕ B1 ⊕ B1′
```

【口頭】[61:29]「這是一個有點深刻的結果，我證明給你看。」

所以一次寫入 = **讀 2 個**（舊的 B1、舊的 P）＋**寫 2 個**（新的 B1、新的 P）。「確實有一點額外成本」，但換到：
- 任何一顆碟壞了都能重建；
- 讀的頻寬變高：B10 和 B12、B14 和 B4 都能平行讀（但同一顆碟上的 B18 和 B10 不行）。隨機的請求大多落在不同的碟，讀頻寬預期至少 **4 倍**。
- 讀的時候通常**不會**每次都去讀其他 block 和 parity 來驗證（為了效能）；block 本身有錯誤偵測碼，碟自己偵測到無法修復的錯就整顆宣告故障（fail-stop）。

**RAID 4 的問題**：**每次寫入**都要讀寫 parity 碟 → parity 碟變成**瓶頸**（single point of contention）。

---

## 14. RAID 5 與 RAID 6

[69:31]–[73:42]　課本 Fig 13.36（p.710–711）

**RAID 5**：不要專用的 parity 碟，把 parity block **輪流**放在每一顆碟上：

```
碟1  碟2  碟3  碟4  碟5
B1   B2   B3   B4   P1
P2   B5   B6   B7   B8
B9   P3   B10  B11  B12
B13  B14  P4   B15  B16
B17  B18  B19  P5   B20
```

- 第一列的 parity 在碟 5，第二列在碟 1、第三列在碟 2……輪流。
- 優點和 RAID 4 一樣，但**沒有**單一的爭用點。
- 5 顆碟時，只要兩個請求不在同一顆碟，頻寬就能到 **5 倍**。
- 「大部分商用系統都用 RAID 5 的某個變形」：可靠、高效能。

**RAID 6**：要防**兩顆**碟同時壞：
- 每一列有**兩個** parity block（P1a、P1b），用**不同的方法**算，也輪流分布在所有碟上。
- 壞兩顆（例如碟 3、4）時，插兩顆新的空碟，用其他四顆把資料和 parity 都重建。
- 代價：有效容量變少；改一個 block 要讀寫那個 block **和兩個** parity block → 每次寫的流量更大、效能更差。
- 【口頭】[71:44]「細節我沒辦法講，修過**域論**（field theory）的課就會知道。」

【課本補充】p.711：RAID 6 主要用在要求高可靠儲存的企業；兩個 parity **不是**簡單的 XOR，是資料的複雜函數。（實際的算法見 §15。）

---

## 15. 【原始碼】Linux 的 RAID 6

Linux 核心的軟體 RAID（md）就有 RAID 6，第二個 parity 用的正是「域論」：在 **GF(2⁸)**（256 個元素的有限域）裡做乘法。`linux-src/lib/raid6/mktables.c:21-33` 產生乘法表用的函式：

```c
static uint8_t gfmul(uint8_t a, uint8_t b)
{
	uint8_t v = 0;

	while (b) {
		if (b & 1)
			v ^= a;
		a = (a << 1) ^ (a & 0x80 ? 0x1d : 0);
		b >>= 1;
	}
	return v;
}
```

- 這就是我們在第 22 支學的「移位相加」乘法器（[第 22 支](lecture22_note.md#6-iterative-multiplier-的硬體)），只是**加法換成 XOR**、而且 a 左移溢位時要 XOR `0x1d`（等於對多項式 x⁸ + x⁴ + x³ + x² + 1 = 0x11d 取餘數），所以結果永遠留在 8 bit 裡。
- 兩個 parity：**P = D₀ ⊕ D₁ ⊕ … ⊕ Dₙ₋₁**（就是 RAID 5 的 parity）、**Q = 2⁰·D₀ ⊕ 2¹·D₁ ⊕ … ⊕ 2ⁿ⁻¹·Dₙ₋₁**（乘法是上面的 gfmul）。
- 兩顆資料碟 a、b 同時壞時（`linux-src/lib/raid6/recov.c:20` 的 `raid6_2data_recov_intx1`）：用剩下的碟算出 Da ⊕ Db 和 2ᵃ·Da ⊕ 2ᵇ·Db，兩個方程式兩個未知數，在 GF(2⁸) 裡解出 Da、Db。recov.c 裡用預先算好的乘法表和反元素表（`raid6_gfmul`、`raid6_gfinv`）做這件事。

---

## 16. 【實測】

程式在 `experiments/lecture45/`。

### 16.1 本機的 PCI Express 與 USB

**PCIe 連結**（`/sys/bus/pci/devices/*/current_link_speed`、`current_link_width`）：

| 裝置 | 目前 | 最大 |
|---|---|---|
| NVIDIA GPU（01:00.0） | 5.0 GT/s × **8** lane | 16.0 GT/s × 8 |
| Ethernet（02:00.0） | 2.5 GT/s × **1** lane | 2.5 GT/s × 1 |
| WD NVMe SSD | 16.0 GT/s × **4** lane | 16.0 GT/s × 4 |
| Samsung NVMe SSD | 16.0 GT/s × **4** lane | 16.0 GT/s × 4 |

- 每個裝置是**點對點**連結，lane 數各不同（x1、x4、x8），正是影片的「1 到 32 條 lane」。
- 網路卡只要 1 條 lane、2.5 GT/s（第一代 PCIe 的速度，也就是影片的 250 MB/s 那一代）；SSD 用 4 條 16 GT/s。
- GPU 閒著時，目前只跑 5.0 GT/s（最大是 16），省電用；忙的時候會升速。

**USB 樹**（`lsusb -t`，節錄）：

```
/:  Bus 02.Port 1: Dev 1, Class=root_hub, Driver=xhci_hcd/9p, 20000M/x2
    |__ Port 4: Dev 2, If 0, Class=Mass Storage, Driver=uas, 5000M
/:  Bus 01.Port 1: Dev 1, Class=root_hub, Driver=xhci_hcd/16p, 480M
    |__ Port 3: Dev 2, If 0, Class=Video, Driver=uvcvideo, 480M
    |__ Port 9: Dev 36, If 3, Class=Human Interface Device, Driver=usbhid, 12M
    |__ Port 14: Dev 3, If 0, Class=Wireless, Driver=btusb, 12M
```

- 樹狀結構：root hub 下面接裝置（Dev 2、3、36 就是 7-bit 的裝置編號）。
- 速度：**12M**（full speed，12 Mbit/s = 1.5 MB/s，影片的 full speed）給 HID（人機介面裝置，例如鍵盤）和藍牙；**480M**（high speed，60 MB/s）給攝影機（Video）；**5000M** 是 USB 3 的 super speed，給外接儲存（Mass Storage）。
- 每個裝置有好幾個 interface（If 0、If 1……），對應 USB 的 endpoint 群組。

### 16.2 RAID 的 parity 與重建

`raid.py`：

```
$ python3 -I raid.py
RAID 4／5：P = B1 ⊕ B2 ⊕ B3 ⊕ B4
  第 1 顆壞了 → 用其他 3 顆和 P 重建：一樣
  第 2 顆壞了 → 用其他 3 顆和 P 重建：一樣
  第 3 顆壞了 → 用其他 3 顆和 P 重建：一樣
  第 4 顆壞了 → 用其他 3 顆和 P 重建：一樣
  小寫入：P ⊕ B1 ⊕ B1' == 重算 B1' ⊕ B2 ⊕ B3 ⊕ B4（讀 2 個 block、寫 2 個 block）
RAID 5 的 parity 位置（5 顆碟，課本 Fig 13.36 的輪流方式）：
  B  B  B  B  P
  P  B  B  B  B
  B  P  B  B  B
  B  B  P  B  B
  B  B  B  P  B
RAID 6：4 顆資料碟任選 2 顆同時壞（6 種），用 P 和 Q 重建成功 6 種
隨機讀 4 個 block 時，平均同時有幾顆碟在讀（5 顆碟）：RAID 0 2.95、RAID 4 2.73、RAID 5 2.95
```

- 512-byte 的隨機資料，任何一顆碟壞了都能用 XOR **完全**重建。
- §13 的小寫入公式 P′ = P ⊕ B1 ⊕ B1′ 和「全部重算」結果一模一樣。
- RAID 5 的 parity 位置和課本 Fig 13.36 一樣。
- RAID 6 用 Linux 的 GF(2⁸) 算 Q：4 顆資料碟中**任兩顆**同時壞的 6 種情況，**全部**重建成功。只用 XOR 的話（RAID 5）兩顆壞就救不回來。
- 隨機讀 4 個 block：RAID 4 的 parity 碟讀的時候用不到，只有 4 顆碟分攤，平均 2.73 顆同時在讀；RAID 0 和 RAID 5 五顆碟都能用，2.95 顆。（寫的時候 RAID 4 每次都卡在 parity 碟，差距會更大。）

### 16.3 本機的儲存裝置

```
$ cat /sys/block/*/queue/rotational ...
sda     rotational=1 logical=512 physical=512 size=500GB  model=PNY PRO ELITE PS
nvme0n1 rotational=0 logical=512 physical=512 size=1024GB model=PC SN740 NVMe WD 1TB
nvme1n1 rotational=0 logical=512 physical=512 size=512GB  model=SAMSUNG MZVL4512HBLU-00BTW
```

- block 大小都是 **512 bytes**，和硬碟傳統的 sector 一樣（SSD 也假裝成 512-byte 的 sector）。
- 兩顆 NVMe SSD 是 rotational = 0（不會轉）。
- ⚠️ sda 回報 rotational = 1，但它接在 USB 3（5000M）的 Mass Storage 上（§16.1），看型號是外接的 USB 儲存裝置，不一定真的是轉動的硬碟：經過 USB 轉接時，核心常常拿不到正確的資訊。本機沒有可以確定是機械硬碟的裝置，所以沒有實測 seek time，「待補」。

---

## 17. 一頁速查表

| 主題 | 重點 |
|---|---|
| PCIe 由來 | 取代 PCI、AGP；主機板上的 bus |
| Lane | 單 bit 全雙工序列通道；一個連結 1–32 條；data striping |
| PCIe 各層 | LVDS、8b/10b、source synchronous；1 byte frame、32-bit CRC、split transaction；250 MB/s／lane；switch |
| USB | 外接周邊的標準；hub 樹，127 個裝置、5 層 |
| USB 接頭 | 5V、GND、D+、D−（mini／micro 多 ID：分 host／device） |
| USB 各層 | LVDS；NRZI（0 = 變化）、6 個 1 塞 0；46–1058 bit frame、CRC、split transaction；7-bit 裝置＋4-bit endpoint；half duplex |
| USB 流量 | control、interrupt、bulk、isochronous |
| USB 速度 | low 192 KB/s、full 1.5 MB/s、high 60 MB/s |
| Pipe | message（有 handshake）、stream（沒結構） |
| 硬碟存 bit | 小磁鐵；方向反轉 = 1、不變 = 0（NRZI） |
| 結構 | platter、track、sector（512 B）；spindle 5400／7200 RPM；arm＋head；actuator 轉圓弧 |
| 存取時間 | seek + rotational latency（平均半圈）+ transfer；Example 157：12 + 50 + 10 = 72 ms |
| LBA | 軟體看大陣列；硬碟內部轉成記錄面／track／sector；壞軌重新對應 |
| RAID 0 | striping；頻寬 ×2；沒有可靠度 |
| RAID 1 | 鏡像；壞一顆沒事；容量減半 |
| RAID 2／3／4 | parity 碟；bit／byte／block striping；只有 RAID 4 實用 |
| 小寫入 | P′ = P ⊕ B ⊕ B′；讀 2 寫 2 |
| RAID 5 | parity 輪流放；沒有瓶頸；最常用 |
| RAID 6 | 兩個 parity（GF(2⁸)）；壞兩顆也能救 |
| 實測 | GPU x8、SSD x4、網路 x1；USB 樹的 12M／480M／5000M；RAID 4／5／6 重建全對 |

---

## 18. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| 光碟（CD、DVD、Blu-ray）、flash memory 與 SSD | **第 46 支**：Optical Drives and Solid State Drives |
| 計算機架構的未來 | **第 47 支** |
| LVDS、NRZI、source synchronous、asynchronous | [第 43 支](lecture43_note.md) |
| CRC、split transaction、endpoint／port、DMA | [第 44 支](lecture44_note.md) |
| 移位相加的乘法器（GF 乘法同一招） | [第 22 支](lecture22_note.md#6-iterative-multiplier-的硬體) |
| 對應表、cache（硬碟的 LBA 對應） | [第 37 支](lecture37_note.md) |

---

## 附：本講一句話總結

> **PCI Express 是取代 PCI 和 AGP 的主機板 bus：點對點連結，由 1 到 32 條單 bit 全雙工的序列 lane 組成，把封包 striping 到各 lane 上，physical layer 用 LVDS、8b/10b 編碼和 source synchronous 時序，data link layer 用 1 byte frame、32-bit CRC 和 split transaction，每條 lane 250 MB/s，用 switch 路由；USB 是接外部周邊的標準，hub 組成的樹最多 127 個裝置，4 pin 接頭同時供 5V 電源和 D+／D− 差動資料，用「變化 = 0、每 6 個 1 塞一個 0」的 NRZI、CRC、7-bit 裝置加 4-bit endpoint 的位址，以及 control、interrupt、bulk、isochronous 四種流量。硬碟用一串小磁鐵的方向變化存 bit（又是 NRZI），盤片分成 track 和 512-byte 的 sector，存取時間 = 尋軌 + 等磁區轉過來（平均半圈）+ 傳輸，內部再把 logical block 對應到實際的記錄面／track／sector、壞軌時重新對應；因為硬碟機械零件多、容易壞，用 RAID 提高可靠度和頻寬：RAID 0 striping、RAID 1 鏡像、RAID 4 用 XOR parity 碟（小寫入只要 P′ = P ⊕ B ⊕ B′）、RAID 5 讓 parity 輪流放避免瓶頸、RAID 6 再加一個有限域上的 parity 防兩顆同時壞。實測本機 GPU 是 x8、SSD 是 x4、網路卡是 x1 的 PCIe，USB 樹上看得到 12M、480M、5000M 三種速度；RAID 4／5 壞一顆、用 Linux GF(2⁸) 算法的 RAID 6 壞兩顆，全部都能完整重建。**

---

*筆記依據 Smruti R. Sarangi「Basic Computer Architecture」Chapt 12: PCI-X, USB, and hard disks, Part 3/4 錄影逐字稿（YouTube 自動字幕，1:13:42）、Sarangi *Basic Computer Architecture* v3.09 Ch13 §13.7.1、§13.7.4、§13.8–13.8.2（p.688–711）、Linux v6.2.12 `lib/raid6/mktables.c`、`lib/raid6/recov.c` 整理。實測為本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、Python 3.10）結果。*
