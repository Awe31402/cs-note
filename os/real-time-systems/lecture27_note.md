# Real Time Systems 第二十七支影片完整筆記：即時通訊（Lecture 28）

> **課程**：Real Time Systems — Lecture 28：Real Time Communication（播放清單第 27 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）；投影片與課本為 Rajib Mall（IIT Kharagpur）
> **來源**：YouTube 錄影逐字稿（49:43，自動字幕）＋ 課本 *Real-Time Systems*（Rajib Mall；IIT Kharagpur「Version 2 CSE」講義版 PDF）Ch6 §1–3.1（PDF p.139–145）
> **標記說明**：
> - `【口頭】`：老師在課堂上口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這堂課沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄（`檔名:行號`），用來對照觀念
> - `【實測】`：在本機（Ubuntu、kernel 6.5.0、x86-64）實際執行的結果
> - `⚠️`：逐字稿缺字、或課堂／課本／原始碼說法不一致之處
> - `[mm:ss]`：影片時間點

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. 什麼是即時通訊](#1-什麼是即時通訊)
- [2. QoS 參數](#2-qos-參數)
- [3. 不同應用對 QoS 的要求](#3-不同應用對-qos-的要求)
- [4. 延遲的組成與 jitter buffer](#4-延遲的組成與-jitter-buffer)
- [5. 三類流量：CBR、VBR、sporadic](#5-三類流量cbrvbrsporadic)
- [6. 網路種類](#6-網路種類)
- [7. CAN 與 LIN](#7-can-與-lin)
  - [7.1 CAN 的起源與結構](#71-can-的起源與結構)
  - [7.2 LIN](#72-lin)
  - [7.3 CAN 的協定](#73-can-的協定)
  - [7.4 非破壞性的位元仲裁](#74-非破壞性的位元仲裁)
  - [7.5 【原始碼】【實測】Linux SocketCAN 與仲裁模擬](#75-原始碼實測linux-socketcan-與仲裁模擬)
- [8. LAN 拓樸與 MAC](#8-lan-拓樸與-mac)
  - [8.1 網路卡與 DMA](#81-網路卡與-dma)
  - [8.2 拓樸](#82-拓樸)
  - [8.3 Token bus](#83-token-bus)
  - [8.4 CSMA/CD 與 Ethernet](#84-csmacd-與-ethernet)
  - [8.5 MAC 子層](#85-mac-子層)
- [9. 【原始碼】Linux 路由器的「排程」：pfifo_fast 的優先權帶](#9-原始碼linux-路由器的排程pfifo_fast-的優先權帶)
- [10. 一頁速查表](#10-一頁速查表)
- [11. 與後續講次的連結](#11-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

處理器即時了還不夠；分散式即時系統還要**通訊**即時。這講是入門：

1. 即時通訊 = 網路對 QoS（延遲、jitter、頻寬、遺失率）做**保證**。
2. 延遲的組成；queueing 延遲最麻煩；jitter buffer。
3. 流量分類：CBR、VBR、sporadic。
4. 網路：**CAN**（汽車）、LIN、LAN 拓樸、token bus、CSMA/CD。
5. 重點是 **MAC 子層**：誰、何時、傳多久——就像處理器排程。

---

## 1. 什麼是即時通訊

[00:00]–[01:30]（課本 p.139）

- 分散式即時平台越來越普及：便宜的現成硬體、一般 RTOS，還需要即時通訊。
- 傳統通訊是**盡力而為（best effort）**；即時通訊支援特定的 **QoS（quality of service）** 要求：例如某封包要在 2 ms 內到——限制最大延遲和最大封包遺失率。
- 網路一旦**接受**連線請求，就**保證**所要求的服務品質，不管網路上其他節點怎麼做。

【課本補充】（p.139）

- 為什麼分散式：用很多便宜硬體往往比一台昂貴的集中式機器划算；很多應用本來就分散（感測器、致動器在不同地點）；分散更容易容錯（安全關鍵應用需要）。
- 傳統網路協定（如 Ethernet）只追求好的平均效能、不管個別任務的 deadline，不適合即時應用。

---

## 2. QoS 參數

[01:30]–[03:00]（課本 §2.2，p.141–142）

| 參數 | 意思 |
|---|---|
| **頻寬（bandwidth）** | 網路服務一個連線的速率 |
| **最大傳輸延遲（delay）** | 超過延遲界限 → 硬即時應用失效、軟即時應用品質下降 |
| **Delay jitter** | 延遲的變動。老師的例子：傳輸時間 5 ms 有時變 10 ms → jitter 5 ms |
| **封包遺失率（loss rate）** | 允許丟多少封包 |
| **Blocking probability** | 網路（看起來）不回應、不接受新連線的機率：某資源被塞爆了 |

【課本補充】（p.141–142）

- Jitter 定義：同一 session 中訊息延遲的**最大變化** = 最大延遲 − 最小延遲。例：最小 1 ms、最大 10 ms → jitter 9 ms。jitter 在影片中會讓畫面停住一段時間。
- 遺失原因：違反延遲界限、違反 jitter 界限、緩衝溢位、資料損毀（光纖可忽略，無線很嚴重）。程序控制需要遺失率 0；多媒體可以忍受一些。
- Blocking probability：新連線被網路的**允入控制（admission control）**拒絕的機率。
- 語音封包故意做得小：減少封包化延遲、限制遺失的影響；ATM 標準 48 bytes 的 cell 大小主要就是為了語音。

---

## 3. 不同應用對 QoS 的要求

[03:00]–[09:00]

- 很難用硬 / 軟 / 堅定來分通訊系統；大致分成**非即時**、**軟即時**、**硬即時**通訊。

| 應用 | 在乎什麼 | 不在乎什麼 |
|---|---|---|
| 軟即時 / 傳統：email、web、FTP、remote login | **不能遺失**（FTP 掉一個 byte 整個檔案就壞了 → 要重傳）；**平均**延遲與平均吞吐量（軟界限） | 最壞延遲、jitter、最壞吞吐量 |
| 非互動電視、音訊廣播 | **jitter 非常嚴格**：每秒 30 張 → 下一張要在 33 ms 內到 | 延遲（管線化，一張晚 10 秒也無所謂，只要下一張 33 ms 後到） |
| Fly-by-wire 飛機 | **延遲**：感測訊號只容許毫秒（課本：次毫秒）延遲 | — |
| 堅定即時：影片、語音 | 遺失率、jitter（掉一兩個封包 = 畫面閃一下、聲音變差，千分之一還好，多了就聽得出來） | 延遲 |

老師：傳統網路的複雜度大多來自「網路可以丟封包，但端到端不能丟」。

**需要即時通訊的應用**（課本 §1）：

- **製造自動化**：機器人之間、機器人與控制器之間（無線即時協定）；訊息從非關鍵的 log 到硬即時控制資訊都有。
- **自動化化工廠**：感測器週期性送參數給控制器，控制器算出修正送給致動器；同時有 log、email、監控等非關鍵活動。
- **網路銀行**：要安全可靠；延遲要求嚴格（交易沒在時限內完成，伺服器逾時拒絕）。
- **多媒體 multicast**：串流影音，堅定即時。
- **網路電話（VoIP）**：堅定即時。

**VoIP 與 DiffServ**【口頭】[09:00]–[10:30]：

- 一般 IP 路由器預設**先來先服務**，不適合即時。
- **DiffServ** 讓 VoIP 封包有優先權。
- 路由器 / 交換器有多個輸入、輸出，每個都有多個**佇列**；**排程佇列中的封包、管理佇列緩衝**是建立即時協定的主要手段——和處理器的排程一樣。要給封包優先權，必要時插隊先送。
- 語音只佔網路負載的一小部分時（像今天的網際網路）效果很好。

---

## 4. 延遲的組成與 jitter buffer

[10:30]–[14:30]

| 延遲 | 說明 |
|---|---|
| **傳播延遲（propagation）** | 約 **0.5 μs / km**（訊號在介質中的速度）；固定，和負載無關 |
| **傳輸延遲（transmission）** | 封包大小 ÷ 鏈路速度（頻寬）。例：10 Mb ÷ 1 Mb/s = 10 s |
| **排隊延遲（queueing）** | 路由器裡多條流量要選，**比前兩者大得多** → 交換器要有好的封包選擇演算法 |

⚠️ 老師說「10 megabyte packet……10 megabits……1 megabit per second → 10 秒」[11:32]，前面說 megabyte、後面算 megabit。10 秒是用 10 Mb 算的。

**Jitter** 的來源：排隊、擁塞；擁塞時封包走不同路徑，時間不同。

**Jitter buffer**：接收端用緩衝吸收 jitter，但要吸收很多 jitter 就需要很大的緩衝。

> **緩衝大小 = 峰值速率 × delay jitter**

老師的例子：每秒 30 張、每張 2 Mb、jitter 5 s → 30 × 2 × 5 = **300**。

⚠️ 單位：老師說「300 mega**bytes**」[14:01]，但每張是 2 mega**bits**，300 應是 **300 Mb = 37.5 MB**。課本 p.142 的例子是 jitter **1 s** → **60 Mb**（課本寫「60Mb」）。

---

## 5. 三類流量：CBR、VBR、sporadic

[14:30]–[17:00]（課本 §2.3，p.143）

| 類別 | 說明 | 例子 |
|---|---|---|
| **CBR（constant bit rate）** | 每秒傳的 bytes 固定；固定大小的訊息週期性傳送；硬即時應用常產生；網路最容易排程 | 感測器的週期資料 |
| **VBR（variable bit rate）** | 不同時間速率不同 | 聊天（傳一陣、停一陣）；課本：壓縮音訊（靜音時不送，on-off）、壓縮影片（MPEG） |
| **Sporadic** | VBR 的重要子類：**一陣爆發後長時間安靜** | 課本：警報訊息 |

【課本補充】VBR 有兩種：一段時間以確定間隔產生固定大小封包、然後閒置；或週期性送出大小不一的封包。VBR 是為了在壓縮影音傳輸中更好地利用頻寬。

---

## 6. 網路種類

[16:31]–[21:01]（課本 §2.1，p.141）

| 網路 | 範圍 |
|---|---|
| **CAN（Controller Area Network）** | 很小：連接嵌入式控制器的元件（汽車的引擎、煞車……）；端到端通常 < 50 m；傳播時間很小 → 功能上像電腦裡的區域匯流排 |
| **LAN** | 一棟建築或校園內，通常私有；課本：10 Mbps 以上，很多已是 1 Gbps；多用廣播網路 |
| **Internet** | 很多 LAN 連起來（每個大學、城鎮、大樓有自己的私有網路，把它們連起來 → inter-net） |

**LAN 內怎麼連**：

- **點對點**：n 個節點要 n(n−1) 條連線（老師口頭先說 n(n−1)、更正後一樣）→ 佈線複雜度、成本、重量都太高。
- **Circuit switching**：送封包前先**預約**整條路徑——也就是預約沿途路由器佇列的**緩衝空間**和固定比例的頻寬。流量是爆發式時會浪費頻寬與資源；很多即時系統是爆發式的 → 不好。
- 所以要**共享介質**，例如匯流排：多個傳送者、多個接收者，同一個節點可以同時是兩者。

【課本補充】課本：Internet 起源於 1962 年美國軍方的 ARPANET 計畫，現在是超過 12,000 個電腦網路、共用一套定址方式的全球系統；小寫 internet 是「任何以共同協定連起來的網路集合」，大寫 Internet 是全球那一個。

---

## 7. CAN 與 LIN

### 7.1 CAN 的起源與結構

[21:01]–[24:31]

- **Bosch** 在 **80 年代中期**為汽車發明，目的是**減少佈線**（汽車裡電線很貴）、讓所有元件講共同語言。
- 典型的車：多條 CAN bus。

```
 主 CAN bus：引擎、懸吊、牽引力、變速箱、儀表板、安全氣囊……（重要的）
     │ gateway
     ├── 次級 bus：雨刷、燈、方向燈
     └── 次級 bus：後照鏡、車窗、門鎖等小開關
```

- CAN bus 像一條鏈：單一匯流排上接多個傳送者、接收者。現代車幾乎每樣東西都是小處理器（ECU），能收送訊息：引擎、安全氣囊、座位感測器、診斷電腦、收音機、CD 播放器……
- 可以是**平的**網路，或**階層式**：某些節點是另一條匯流排的 gateway。子網的節點只和姊妹節點競爭，贏了再透過 gateway 送上主匯流排——便宜地延伸匯流排的範圍。
- CAN 用**兩條線**；主要是**硬體**協定（也可以用軟體實作協定）。

【課本補充】（p.141）

- 早期汽車用點對點佈線，車越複雜就要好幾公里的電線 → 成本、重量、複雜度都高、可靠度很差 → 發展出 CAN。
- 特別要求：**處理雜訊**（電動馬達、點火系統、RF 傳輸都是強雜訊源）；用汽車的 **12 V** 電源。
- CAN 只規定 OSI 的**實體層和資料鏈結層**，上層由實作決定。
- 因為穩健，已擴展到工業自動化、火車、船、農機、家電、辦公自動化、電梯等。國際標準 **ISO 11898**、**ISO 11519-2**。

### 7.2 LIN

[24:31]–[27:01]

**LIN（Local Interconnect Network）**：CAN 的子網路，功能少、成本低。

| | CAN | LIN |
|---|---|---|
| 線數 | 2 條（差動訊號） | **1 條** |
| 主從 | **沒有** master / slave：每個節點都能收、都能主動送 | **單一 master**，多個 slave；決策都由 master；很多版本只有 master 能寫（或只有 master 能讀） |
| 仲裁 | 需要（非破壞性位元仲裁） | **不需要** |
| 速度 / 範圍 | 主網路；長度可到約 50 m（汽車、公車） | 約 20 kbit/s |
| 成本 | — | 非常低：用 **UART / SCI** 硬體就能做，幾乎任何微控制器都能接 |
| 延遲 | — | 因為簡單，**延遲有保證** |

LIN 子網的 master 負責連到 CAN bus。

### 7.3 CAN 的協定

[27:01]–[32:30]

- 汽車電子很複雜：引擎管理、噴油、主動懸吊、煞車、燈光……引擎運轉時元件之間大量交換訊息，CAN 承載大部分。
- **雜訊**：汽車電氣環境很吵（RF 傳輸、點火系統、電動車的馬達）→ **兩條線、差動訊號**：兩條線並排，雜訊對兩者影響相似，相減後雜訊大多消掉。
- MAC：以 **CSMA（carrier sense multiple access）** 為基礎；多個節點同時送 → 碰撞 → 用**非破壞性的位元仲裁**解決、選出贏家；贏家一直送到不需要為止。
- 每個送出的訊息有**唯一的 ID**，ID 代表**優先權**。
- OSI 模型：CAN 跑在**實體層和資料鏈結層**；網路、傳輸、session、表示層只有很有限的部分實作（用更高層的協定）；應用層由 ECU 處理 CAN 訊息。車上可以有一個主處理器跑 **CANopen**、**DeviceNet**，甚至自己的 RTOS（如 OSEK/VDX，第 24 支影片）來監看匯流排上的訊息。

**CAN frame 格式**：

```
| SOF | Arbitration（ID＝優先權）| Control | Data（payload）| CRC | ACK | EOF | 訊框間隔 |
```

### 7.4 非破壞性的位元仲裁

[32:30]–[35:32]

- 一般網路（老師以行動網路為例）：大家都送 → 發現碰撞 → 隨機退避 → 最後有人成功。
- CAN：匯流排本身計算 **wired AND**——一個送 1、一個送 0，匯流排上就是 **0**。0 是 **dominant（顯性）**、1 是 **recessive（隱性）**。
- 每個節點邊送邊聽：送 1 卻聽到 0 → 知道自己輸了，**停止傳送**。所以 **ID 數值越小，優先權越高**。
- 碰撞時沒有人「浪費」：贏家的位元本來就在匯流排上，繼續送就好 → **頻寬利用率高**。

老師的第一個例子：A 的 ID 010、B 的 ID 011。

```
bit:   0  1  2
A:     0  1  0
B:     0  1  1   ← 第 2 位送 1 卻聽到 0 → B 輸
bus:   0  1  0   → A 贏，A 的資料上匯流排
```

⚠️ 老師的第二個例子：A = 010、B = 000，他說「最後是 node A 贏、送出 A 的資料」[35:01]。照 wired AND，第 1 位 A 送 1、B 送 0、匯流排是 0 → **A 輸、B 贏**（000 是較小的 ID、較高優先權）。§7.5 的模擬確認。

### 7.5 【原始碼】【實測】Linux SocketCAN 與仲裁模擬

Linux 的 CAN 支援叫 **SocketCAN**，CAN 訊框就是一個 struct：

`include/uapi/linux/can.h:120`

```c
struct can_frame {
	canid_t can_id;  /* 32 bit CAN_ID + EFF/RTR/ERR flags */
	union {
		/* CAN frame payload length in byte (0 .. CAN_MAX_DLEN)
		 * was previously named can_dlc so we need to carry that
		 * name for legacy support
		 */
		__u8 len;
		__u8 can_dlc; /* deprecated */
	} __attribute__((packed)); /* disable padding added in some ABIs */
	__u8 __pad; /* padding */
	__u8 __res0; /* reserved / padding */
	__u8 len8_dlc; /* optional DLC for 8 byte payload length (9 .. 15) */
	__u8 data[CAN_MAX_DLEN] __attribute__((aligned(8)));
};
```

`include/uapi/linux/can.h:56`、`:61`

```c
#define CAN_EFF_FLAG 0x80000000U /* EFF/SFF is set in the MSB */
...
#define CAN_SFF_MASK 0x000007FFU /* standard frame format (SFF) */
```

- `can_id` 就是**仲裁欄位**：標準格式 11 位元（`CAN_SFF_MASK` = 0x7FF），延伸格式 29 位元（`CAN_EFF_FLAG`）。
- `data` 最多 8 bytes（`CAN_MAX_DLEN`）——CAN 訊框很短，符合「短訊息、低延遲」。
- 仲裁本身在 CAN 控制器**硬體**裡完成，kernel 不參與；應用只要用 `socket(PF_CAN, SOCK_RAW, CAN_RAW)` 收送 `struct can_frame`。（本機沒有 CAN 介面，`ip link show type vcan` 也沒有；建立 vcan 需要 root。）

【實測】Python 模擬位元仲裁（匯流排 = 所有傳送位元取 AND；送 1 卻讀到 0 就退出）：

```python
bus = min(sent.values())                       # wired AND
lost = [n for n, s in sent.items() if s == 1 and bus == 0]
```

```
IDs {'A': '010', 'B': '011'} -> winner ['A']  losers per bit: [(2, ['B'])]
IDs {'A': '010', 'B': '000'} -> winner ['B']  losers per bit: [(1, ['A'])]
IDs {'engine': '00010100000', 'airbag': '00001010000', 'radio': '01111110000'} -> winner ['airbag']  losers per bit: [(1, ['radio']), (3, ['engine'])]
```

- 第一個例子和老師一致（A 贏）。
- 第二個例子 **B 贏**（老師說 A）。
- 三個 11 位元 ID：收音機（0x3F0）第 1 位就輸、引擎（0x0A0）第 3 位輸，安全氣囊（0x050，最小）贏。設計時把最緊急的訊息給最小的 ID，就等於靜態優先權排程（RMA 的味道）。

**Jitter buffer 計算**：

```
  30 fps x 2 Mb/frame x 5 s jitter = 300 Mb = 37.5 MB
  30 fps x 2 Mb/frame x 1 s jitter = 60 Mb = 7.5 MB
```

---

## 8. LAN 拓樸與 MAC

### 8.1 網路卡與 DMA

[36:00]–[38:31]

- LAN 節點的五種基本連法：**bus、tree、star、ring、wireless**（無線沒有特定拓樸）。
- **NIC（network interface card）**：一邊和主機板溝通、一邊和網路溝通；把 LAN 和各種裝置的特性隔開，大家都講同一種網路語言；實體層的電壓、訊號等電氣與網路問題都由它處理，CPU 只看到標準介面。
- NIC 需要權限直接存取主記憶體：**DMA（direct memory access）**——不能一個 byte 一個 byte 傳，指定一塊區域，不經 CPU 整塊搬到 NIC（或從 NIC 搬回記憶體）；主記憶體裡有專門的區域做這種傳輸。

### 8.2 拓樸

[38:31]–[41:00]

| 拓樸 | 說明 |
|---|---|
| Tree | 以前有，現在少見 |
| **Star** | 單一 hub（有線或無線），所有機器連到中心。無線也可看成 star：工作站只要在 access point 的收發範圍內。最常見的無線標準 **IEEE 802.11** |
| **Ring** | 節點連成環，**輪流**傳送、每次傳預定的時間 → 傳輸**可預測** → 即時應用很喜歡（IEEE token ring、token bus 都源自 ring） |

Ring 的問題：任何一處斷掉整個網路就倒（不穩健）；而且實際的生產線多是**線性**拓樸，不是環。

【課本補充】（p.144–145）

- Bus 架構：節點用 T 形接頭接到網路纜線，兩端有終端器；單一共享通道，節點競爭使用。
- Ring 架構：MSAU（multistation access unit）是 hub / 集中器，把一群電腦接成 Token Ring；節點輪流傳送預定的時間 → 延遲可預測、可以做得很小 → 即時應用偏好。

### 8.3 Token bus

[41:00]–[43:32]

- 實體上是 **bus**，邏輯上排成 **ring**：每個站知道它左邊、右邊是誰。結合 bus 和 ring 的優點。
- 特殊的控制訊框 **token**：拿到 token 才能傳。
- 傳完預定的時間 → 把 token 交給左邊（或右邊）的鄰居；拿到 token 的站要傳就傳，不要就交下去。
- 任何時刻只有 token 持有者能傳 → **設計上就沒有碰撞**。
- 任何節點壞掉，協定就壞了（要處理）。老師：對即時系統來說，這是目前最好的想法（之後幾講詳談）。
- MAC 要支援：站的加入與移除、建立邏輯環、站不一定都在同一個環上（例：邏輯環 1 → 2 → … → 10，有些節點不在環上）。

課本 p.145：邏輯環初始化時，**編號最高**的站先傳。

### 8.4 CSMA/CD 與 Ethernet

[43:32]–[45:31]（課本 p.145）

沒有 token、所有節點共用一個通道時（例如無線網路）：

- **CSMA**：每個節點要**感測通道狀態**；通道閒著才傳。
- 兩個節點可能同時感測到閒置、同時開始傳 → 仍會碰撞。
- **傳播延遲越大，碰撞機率越高**（我佔用通道越久，別人越可能想傳）。
- **CD（collision detection）**：傳送時邊檢查，偵測到碰撞**立刻停止**。
- CAN 不會碰撞（wired AND 自動解決）；其他系統要寫碰撞解決協定。

**Ethernet**：

- 有線 LAN 協定，簡單、便宜、高速。
- **指數退避（exponential back-off）**：第一次在 0–k 間隨機等；再碰撞在 0–2k；再來 0–4k……
- 很多人嘗試延伸 Ethernet 支援即時通訊（之後的講次）。

課本：Ethernet 因為普及、高速、簡單、便宜成為最受歡迎的 LAN 協定；但它的延遲隨流量增加，高流量時吞吐量和最大延遲幾乎無法接受。邏輯環的**確定性**存取仲裁比 Ethernet 以碰撞為基礎的機制有顯著優勢。

### 8.5 MAC 子層

[45:31]–[49:32]（課本 §3，p.143）

- 資料鏈結層（L2）有兩個子層：**LLC（logical link control）**（控制鏈路的各種功能，有線或無線，和我們的討論關係不大）和 **MAC（medium access control）**。
- 從即時角度看，MAC 最重要：多個傳送者碰撞 → 大量**不確定的延遲** → 任務送不出東西 → 可能錯過 deadline。
- MAC 最重要的問題：**節點何時能傳、能傳多久**。

> 課本：**access arbitration** policy 決定節點**何時**能傳；**transmission control** policy 決定能傳**多久**；兩者合稱 **access control techniques**，構成 MAC 層協定。

- 錯誤更正、framing 等屬於 LLC，不在這裡談。
- LAN 典型的 MAC 是 Ethernet。之後會看很多 MAC 協定，它們決定了程式的即時性——就像處理器排程。
- 兩大類 MAC：

| 類別 | 說明 |
|---|---|
| **Contention-based（競爭式）** | 試著送、碰撞了再處理：CAN 自動解決；Ethernet 要用演算法（退避）解決 |
| **Round-robin（輪流式）** | 像 token ring：token 在節點之間移動，一個傳完換下一個 |

兩者的即時保證都要仔細分析——之後幾講。

---

## 9. 【原始碼】Linux 路由器的「排程」：pfifo_fast 的優先權帶

老師說路由器靠**佇列排程**給 VoIP 優先權。Linux 自己當路由器或送封包時，網路介面上的 **qdisc**（queueing discipline）就是這個排程器。傳統預設的 `pfifo_fast` 有三個優先權帶（band 0 最先送）：

`net/sched/sch_generic.c:697`

```c
static const u8 prio2band[TC_PRIO_MAX + 1] = {
	1, 2, 2, 2, 1, 2, 0, 0 , 1, 1, 1, 1, 1, 1, 1, 1
};
```

`include/uapi/linux/pkt_sched.h:20`

```c
#define TC_PRIO_BESTEFFORT		0
#define TC_PRIO_FILLER			1
#define TC_PRIO_BULK			2
#define TC_PRIO_INTERACTIVE_BULK	4
#define TC_PRIO_INTERACTIVE		6
#define TC_PRIO_CONTROL			7
```

- 封包優先權（由 IP 標頭的 TOS / DSCP 或 socket 的 `SO_PRIORITY` 決定）6（interactive）、7（control）→ **band 0**，最先送；一般流量（0）→ band 1；bulk、filler → band 2，最後送。
- 只要 band 0 有封包，band 1、2 就等——這就是**嚴格優先權**的封包排程，和 SCHED_FIFO 對執行緒做的事一樣。DiffServ 把 VoIP 標成 EF（expedited forwarding），就是想進到這種高優先權佇列。

【實測】本機 `tc qdisc show`：

```
qdisc noqueue 0: dev lo root refcnt 2
qdisc fq_codel 0: dev enp2s0 root refcnt 2 limit 10240p flows 1024 quantum 1514 target 5ms interval 100ms memory_limit 32Mb ecn drop_batch 64
qdisc noqueue 0: dev wlp0s20f3 root refcnt 2
```

- 現代 Ubuntu 的預設已經是 **fq_codel**：每條流一個佇列（公平），再用 CoDel 把排隊延遲壓在 **5 ms 目標**（`target 5ms`）——直接針對老師說的「排隊延遲是最大的延遲來源」。
- 要做嚴格優先權（例如給 VoIP），可以換成 `prio` 或 `mqprio` qdisc（需要 root）。

---

## 10. 一頁速查表

| 主題 | 重點 |
|---|---|
| 即時通訊 | 網路接受連線後保證 QoS；傳統是 best effort |
| QoS | 頻寬、最大延遲、jitter（最大 − 最小）、遺失率、blocking probability（被允入控制拒絕） |
| 應用 | 傳統：不遺失、平均值；廣播影音：jitter；fly-by-wire：延遲；影音（堅定）：遺失率、jitter |
| 延遲 | 傳播 ~0.5 μs/km、傳輸 = 大小/頻寬、**排隊**最大 |
| Jitter buffer | 峰值速率 × jitter（30 fps × 2 Mb × 5 s = 300 Mb；課本 1 s → 60 Mb） |
| 流量 | CBR（感測器）、VBR（壓縮影音、聊天）、sporadic（警報） |
| 網路 | CAN（< 50 m、像區域匯流排）、LAN、Internet；點對點與 circuit switching 太浪費 |
| CAN | Bosch 80 年代中；兩線差動抗雜訊；12 V；只有 L1、L2；ISO 11898；ID = 優先權；wired AND，0 顯性，**ID 越小越優先**；不浪費頻寬 |
| LIN | 單線、單 master、不需仲裁、~20 kbit/s、UART 即可、延遲有保證 |
| LAN | NIC ＋ DMA；bus / tree / star / ring / wireless；ring 可預測但脆弱 |
| Token bus | 實體 bus、邏輯 ring；持 token 才能傳 → 無碰撞 |
| CSMA/CD、Ethernet | 感測後傳、碰撞即停、指數退避；高負載時延遲不可接受 |
| MAC | access arbitration（何時）＋ transmission control（多久）；競爭式 vs 輪流式 |
| Linux | SocketCAN `struct can_frame`（11/29 位元 ID、8 bytes）；pfifo_fast 三個優先權帶；預設 fq_codel 目標 5 ms |

---

## 11. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| **LAN 上的即時 MAC 協定（token、priority、Ethernet 延伸）** | **第 28 支**：Real Time Communication in LANs |
| **流量特性化與整形（leaky / token bucket）** | **第 29 支**：Traffic Characterization and Shaping |
| **路由器的封包排程（service disciplines）** | **第 30 支**：Service Disciplines |
| **即時資料** | **第 31 支**：Real Time Data Stores |

---

## 附：本講一句話總結

> **即時通訊要求網路在接受連線後保證 QoS——頻寬、最大延遲、jitter（最大減最小延遲）、遺失率和 blocking probability——而不同應用在乎的不同：傳統檔案傳輸只在乎不遺失與平均值，廣播影音最在乎 jitter，fly-by-wire 最在乎延遲，影音通話則在乎遺失率與 jitter。延遲由傳播（約 0.5 μs/km）、傳輸（大小÷頻寬）與最麻煩的排隊延遲組成，jitter 可用「峰值速率 × jitter」大小的接收緩衝吸收（30 fps、每張 2 Mb、jitter 5 s 就要 300 Mb）；流量分成 CBR、VBR 和爆發式的 sporadic。汽車用 Bosch 的 CAN：兩條線差動傳輸抗雜訊、只定義實體與資料鏈結層、以訊息 ID 當優先權，匯流排做 wired AND（0 是顯性），送 1 卻聽到 0 的節點自動退出，所以 ID 越小越優先、碰撞也不浪費頻寬（模擬顯示老師第二個例子其實是 B 贏）；更便宜的 LIN 只有一條線和一個 master。LAN 方面，ring 和實體 bus、邏輯 ring 的 token bus 因為輪流傳送而延遲可預測，Ethernet 的 CSMA/CD 加指數退避則在高負載時延遲不可接受；關鍵在 MAC 子層決定誰何時傳、傳多久，就像處理器排程——Linux 的 SocketCAN 把 11/29 位元的仲裁 ID 放在 can_id，pfifo_fast 用三個優先權帶做嚴格優先，而本機預設的 fq_codel 則把排隊延遲壓在 5 ms。**

---

*筆記依據 Smruti R. Sarangi「Real Time Systems」Lecture 28 錄影逐字稿（YouTube 自動字幕，49:43）、Rajib Mall *Real-Time Systems*（IIT Kharagpur Version 2 CSE 講義版）Ch6 §1–3.1 整理。原始碼為 Linux v6.2.12；實測為本機（Ubuntu、kernel 6.5.0、x86-64）結果。*
