# Real Time Systems 第二十四支影片完整筆記：商用 RTOS（Lecture 25）

> **課程**：Real Time Systems — Lecture 25：Commercial RTOSs（播放清單第 24 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）；投影片與課本為 Rajib Mall（IIT Kharagpur）
> **來源**：YouTube 錄影逐字稿（45:00，自動字幕）＋ 課本 *Real-Time Systems*（Rajib Mall；IIT Kharagpur「Version 2 CSE」講義版 PDF）Ch5 §7.1–7.5、7.7（PDF p.126–130）
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
- [1. eCos](#1-ecos)
- [2. µC/OS-II](#2-µcos-ii)
- [3. VRTX](#3-vrtx)
- [4. VxWorks](#4-vxworks)
- [5. LynxOS](#5-lynxos)
- [6. QNX Neutrino](#6-qnx-neutrino)
- [7. OSEK/VDX](#7-osekvdx)
- [8. 【課本補充】pSOS](#8-課本補充psos)
- [9. 【實測】Linux 上的執行緒與 mutex 延遲](#9-實測linux-上的執行緒與-mutex-延遲)
- [10. 一頁速查表：總比較](#10-一頁速查表總比較)
- [11. 與後續講次的連結](#11-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

上一講是免費的 Linux 系（MontaVista、RTLinux、RTAI）；這講再看一個免費的 **eCos**，然後是一串商用 / 經典 RTOS：

| RTOS | 一句話 |
|---|---|
| eCos | 幾十 KB、單程序多執行緒、全在 kernel mode |
| µC/OS-II | 約 20 KB、5500 行 C、開源但商用要授權 |
| VRTX | 4 KB 起；第一個通過 FAA 認證的商用 RTOS |
| VxWorks | Wind River；host-target；火星任務、Boeing 787 |
| LynxOS | 28 KB microkernel；self-host |
| QNX Neutrino | microkernel ＋ software bus；被動式 kernel |
| OSEK/VDX | 汽車專用 |

老師的立場【口頭】[20:31]：產品都有缺點，後來的產品在前人的基礎上改進；工程師是在當時的時間、市場、資源下盡力而為。老師對這些 OS 都非常推崇。

---

## 1. eCos

[01:00]–[10:02]

- **embedded Configurable OS**。老師試過，連結（2022）還活著，也還有社群在維護。
- 給**記憶體極受限**的嵌入式系統：只需要**幾十 KB**。
- 多半是 **host-target**：在 Windows / Linux 桌機上開發，有一層虛擬化在 host 上跑或模擬 eCos，再部署到 target。
- 歷史：Cygnus Solutions 開發 → 被 Red Hat 收購 → Red Hat 2002 年初就停止開發 → 轉給 Free Software Foundation 繼續。
- **只有一個程序**、可以多執行緒；可設定（容易依應用需求客製）。
- API 相容 **POSIX** 和 **µITRON**（逐字稿聽成「metron」；另一個嵌入式、即時裝置的標準）。
- 目標：超大量的裝置（小而可靠）：消費性電子、電信硬體、**汽車**（智慧車有上百個電子單元、儀表板、GUI）。
- 和 RTAI 一樣，即時任務在 kernel mode——其實**根本沒有 user mode**，全在 kernel mode。

架構（由上而下，全在 kernel）：

```
 Applications
 C 函式庫、相容層（POSIX、µITRON）、簡單 web server、網路堆疊、檔案系統
 小 kernel
 ──────────── 以上與 target 無關
 boot ROM、HAL、基本驅動（flash、serial、ethernet）
 Target hardware
```

- 這麼小的空間能塞這麼多，老師覺得很驚人。
- HAL 仍然需要：不是為了跑另一個 kernel，而是硬體和主機板會變。
- 功能：多種排程演算法、計時器與計數器、中斷與例外（遮罩、中斷優先權——全在 kernel 裡所以容易做）、和 host 連線除錯（部署後很難除錯，所以要有專門的埠送除錯資訊回 host）。

**eCos vs RTLinux**（老師的數字，μs）：

| 指標 | RTLinux | eCos |
|---|---|---|
| 執行緒建立延遲 | 137 | 16–34 |
| 執行緒刪除延遲 | 稍多 | 5–6 |
| Mutex 延遲 | 5–46 | 5–25 |

老師：RTLinux 那麼大，其實表現不差；主要差在執行緒建立，大概因為 RTLinux 記帳比較多。

**功能比較**（F = 完整、P = 部分、N = 沒有；老師依投影片口述）：

| 功能 | RTAI | RTLinux | eCos |
|---|---|---|---|
| 即時執行緒 API | F | F | 稍少 |
| 即時訊息 | 相近 | 相近 | 相近 |
| 多種排程政策 | F | F | P（eCos 本來就很小；前兩者是附加在一般 Linux 上的） |
| 分散式時鐘同步 | N | N | N（用不到） |
| 和非即時程序溝通 | 有（共享記憶體、FIFO） | P | 意義不大（全在 kernel mode） |
| libc API | 有 | 較少 | 有 |
| C++ 標準函式庫、即時網路 | — | — | 不在表上 |
| 彈性存取中斷 | F | F | F |
| 文件 | — | — | 很好（老師實際去看過網站） |

⚠️ 老師前面說 eCos「用 **C++** 實作」[09:00]，比較表那段又說「eCos 仍然是 C 的東西，用 C 寫的，不是 C++，所以不支援 C++ 標準函式庫」[09:01]。兩處互相矛盾；講義沒有 eCos 的內容可對照，待補。

---

## 2. µC/OS-II

[10:02]–[15:00]（課本 §7.5，p.129）

- **Micro-Controller OS version 2**。
- 非常小：完整功能的 kernel 約 **20 KB**；原始碼約 **5500 行**，幾乎全是 ANSI C。
- **開源但商用不免費**：商用要付授權費或權利金；可授權給單一產品或整條產品線。由 **Micrium** 維護。
- 小到可用在 **8 位元**嵌入式平台；**可搶占的即時多工 kernel**。
- 記憶體管理不是虛擬記憶體，而是**固定大小的分割區（partition）**分配給任務。
- 支援超過 **100** 種微處理器。
- 功能豐富（很多是附加模組，不在 5500 行裡）：簡易檔案系統（FAT）、執行期監控、網路（完整 TCP/IP、HTTP）、GUI、USB、**CAN**（汽車用）、flash loader、RS-232、JTAG、shell、LCD。
- 可擴展：可以讓很多個一起運作。

架構：

```
 Application
 µC/OS-II（CPU 無關）：core、message mailbox、memory、mutex、semaphore、task、time
 CPU 相關的部分（像 CPU 驅動）
 BSP（board support package，主機板驅動）——同一顆 CPU 可能裝在不同主機板上
 Hardware
```

老師鼓勵觀眾去看 µC/OS 的程式碼。

【課本補充】（p.129）

- Micrium 公司；ANSI C ＋ 一點組合語言；組合語言盡量少，方便移植，已移植到超過 100 種架構（8 位元到 64 位元、微控制器、DSP）。
- 可以只用少數服務或全部服務，減少記憶體。
- **完全可搶占**的 kernel：永遠跑就緒任務中優先權最高的。
- 最多 **64** 個任務；每個任務要有**唯一**的優先權等級（共 64 級）→ **不支援 round robin**；優先權等級也當作任務 ID。
- **分割式記憶體管理**：每個分割區由數個固定大小的區塊組成；配置與釋放固定大小區塊是常數時間、確定性的。任務可以建立多個分割區，以使用不同大小的區塊。
- 通過 **FAA** 認證，可用於商用飛機（航電軟體標準）。

⚠️ 任務數：老師說可管理 **255** 個任務 [14:00]；課本說最多 **64** 個（64 個優先權等級）。兩者不同，可能是不同版本的差異；待補。

---

## 3. VRTX

[15:00]–[19:30]（課本 §7.2，p.128）

- **Versatile Real-Time Executive**；80 年代由 **Ready Systems** 開發，**1995** 年被 **Mentor Graphics**（頂尖 EDA 公司）收購，用於 SoC（多功能整合在一顆晶片）和傳統嵌入式系統。
- kernel 很小：一開始 **4 KB** → 功能不足，不能當完整 RTOS；後來版本增加功能。

| 版本 | 特點 |
|---|---|
| **VRTXmc**（microcontroller） | 低功耗、ROM / RAM 小，kernel 幾 KB；給手機等嵌入式應用（老師：20 年來手機都改用 Android 這種完整、非即時的 OS 了）；**第一個通過美國 FAA 認證、可用於飛機與安全關鍵應用（航電）的商用 RTOS**。早期產品：有線電視機上盒、數位相機、電玩、網路路由器、掌上電腦；Motorola 手機也從 VRTXmc 開始 |
| **VRTXsa**（scalable architecture） | 符合 **POSIX-RT**；給中大型應用；大量 I/O 支援、記憶體保護、降低中斷與任務切換延遲；**priority inheritance**、多處理器；系統呼叫**可搶占**且時間**確定**；例：**哈伯太空望遠鏡** |

老師：即時演算法有保證，對安全關鍵認證很有幫助。路由器現在多用更強的處理器和客製 Linux（軟即時就夠）。

【課本補充】（p.128）

- VRTX 是 Mentor Graphics 的 POSIX-RT 相容 OS，通過 FAA 認證可用於任務與生命關鍵應用（航電）。
- **VRTXsa**：支援虛擬記憶體、POSIX 函式庫、priority inheritance；系統呼叫在固定時間間隔內**完全確定**地完成，而且完全可搶占。
- **VRTXmc**：kernel 通常只要 **4–8 KB ROM、1 KB RAM**；不支援虛擬記憶體；給電腦玩具、手機、其他小型手持裝置。

---

## 4. VxWorks

[19:30]–[30:30]（課本 §7.3，p.128）

- **Wind River Systems** 開發（即時領域非常有代表性的公司），**2009** 年被 **Intel** 收購。
- 傳說【口頭】：80 年代初 VRTX 不太好用，Wind River 擴充它，所以 VxWorks = 「**VRTX that works**」。老師強調這是**未經證實的傳聞**（投影片原作者寫的），他不背書。
- VRTX 缺：能用的檔案系統、好的開發環境 → VxWorks 補上完整功能和 **host-target** 開發環境。
- kernel 仍是 **monolithic**；支援 POSIX-RT 和自己的 API。
- 應用：**火星任務**、Honda 機器人、網路交換器、**MRI** 掃描儀、汽車引擎控制。

**主要功能**：

| 功能 | 說明 |
|---|---|
| 多處理器、多工 | 很好的支援；可搶占排程、round robin |
| 記憶體保護 | 使用者（即時）應用和 kernel **隔離**——老師：這和 RTAI、Xenomai「即時任務和 kernel 平起平坐、全權」的做法不同，而且方向是對的（安全） |
| 優先權 | 可搶占優先權排程，**0–255** |
| **Task locking** | 使用者可以**鎖住**一個任務：不管其他任務優先權多高、有沒有中斷，它都繼續跑、不被搶占（前面沒看過的功能） |
| 同步 | 有 priority inheritance 的 mutex、semaphore |
| 工具 | shell 介面、檔案系統（和 VRTX 的差別）、完整模擬器（host-target 必需）、**symbolic debugging**、效能監控工具——非常重視市場 |

**縮短 context switch：register window**【口頭】[25:02]–[27:30]：

- 一般 context switch：把暫存器存到中斷處理常式 / kernel 堆疊，之後再還原。
- 當時的伺服器處理器（老師說是 Sun 的機器）有**很大的暫存器檔**：改變「架構暫存器 → 實體暫存器」的對應，讓每個任務有自己的一組暫存器（**register window**），切換時不必寫回記憶體再讀回。
- VxWorks 更聰明：對 register window 做 profiling；任務太多、空間不夠時，決定哪些留在暫存器檔、哪些存到記憶體；沒在用的任務整個暫存器 context 可以丟掉。
- 另外還有專門存溢出暫存器的 **register cache**（CPU 內的小記憶體）。

**記憶體保護的演進**：早期應用在 kernel mode、沒有隔離 → 後來有一個薄的虛擬記憶體函式庫 **VxVMI**（需要硬體 MMU）→ 最新版本都用某種虛擬記憶體。

**開發**：

- 在 Unix 或 Windows 上開發；**cross-compiler**（例如在 Intel Core i9 的 Linux 上編譯，產生 target 架構的程式碼）。
- 部署前用 **VxSim** 測試整個應用。
- **Tornado IDE**：以 VxWorks 為 target OS，含 cross-compiler、除錯器、host-target 通訊、執行期除錯監控工具、模擬器。

**用在哪**：火星任務與探測車、**Deep Impact** 等太空船、**Boeing 787**、Linksys 無線路由器、大量醫療設備（例如 **Siemens** Medical Solutions）、工業機器人。

【課本補充】（p.128）

- host 可以是 Windows 或 Unix；符合 POSIX-RT。
- Tornado 含 **VxSim**（沒有真實 target 板時模擬 VxWorks target）和 **WindView**（模擬環境的除錯工具）；**VxMP** 是多處理器版本。
- **Mars Pathfinder**（1997）：一直重置；地面工程師用 VxWorks 的追蹤、記錄、除錯工具遠端判定原因是 **unbounded priority inversion**——VxWorks 有 priority inheritance，但設定檔裡因疏忽**關掉了**；打開後修好。（第 10 支影片。）

---

## 5. LynxOS

[30:30]–[35:31]（課本 §7.7，p.130）

- 1988 年首次發布，開發歷史很長，至今很流行。
- **完全可搶占**、精簡、**可重入**的 kernel。
- **Microkernel**：只有 **28 KB**，提供基本服務：（最基本的）排程、中斷分派、同步（含多處理器同步）。
- 其他服務是 **KPI（kernel plug-in）**；KPI 可以多執行緒；提供 I/O、檔案系統、TCP 網路、stream、socket，可在多程序甚至多機器環境中運作。

老師談 microkernel（第 19 支影片）：小、容易移植維護延伸——只要 KPI 遵守和 microkernel 的介面契約。沒有 monolithic 那麼快，但考慮彈性（甚至執行期加減模組），用在即時系統很好。

- 加上 KPI 可以變成多用途 OS。
- **呼叫 KPI 不需要 context switch**：向檔案系統 KPI 發出服務請求，不在另一個 context 執行，幾乎像函式呼叫；KPI 之間互相呼叫也只要很少指令。
- **Self-hosted**：不限 host-target，可以在同一個系統上開發（第 22 支影片筆記）。
- kernel 受保護；和 VxWorks 一樣有軟硬體 MMU，隔離使用者應用和 kernel。
- **只有一種排程政策**：老師說這不合他意——連一般 Linux 都允許使用者層排程器。固定優先權、可搶占、**256** 級。
- **時鐘 tick 固定 100 Hz**（每 10 ms）→ 計時器解析度受限，和 RTAI、Xenomai 的奈秒級計時器比，不算最先進。

【課本補充】（p.130）

- Lynx 3.0 是 microkernel；早期版本是 monolithic。
- **和 Linux 完全相容**：Linux 程式的**二進位檔可以直接在 Lynx 上跑**；QNX 等其他 Linux 相容 OS 則要重新編譯。
- 完整設定下可以當多用途 Unix 機器，同時跑硬即時與軟即時任務。
- 和很多嵌入式 RTOS 不同，Lynx **支援記憶體保護**。

---

## 6. QNX Neutrino

[35:31]–[42:00]（課本 §7.4，p.128–129）

- 非常流行的專有 RTOS；符合 **POSIX**；最早的努力始於 **University of Waterloo**。
- **Microkernel**（和 LynxOS 一樣），加上 **software bus**：所有元件都接在匯流排上——程序管理器、各種檔案管理器、應用、圖形管理器、網路……

```
        File manager   Graphics   Network
             │            │          │
 ════════════╪════════════╪══════════╪════════  software bus（訊息傳遞）
             │            │          │
        Microkernel   Process mgr  Application
```

**Software bus**【口頭】：

- 很常見的設計：例如很多機器人系統用的 **ROS（Robot Operating System）**。
- 像 **publish–subscribe**：有人按了鍵盤，我「發布」按鍵值；有興趣的訂閱者（檔案管理器、程序管理器）去讀。（嚴格說 pub-sub 是你訂閱某些事件、發布者產生時自動拿到一份；bus 是更一般的多對多通訊架構。）
- 有開銷（不是直接呼叫函式），但彈性大，而且更容易保證某些性質（即時性質）。

**Kernel 是被動的**：

- kernel 提供執行緒相關服務（建立、刪除、通訊）和時間服務；開發者可以關掉不需要的功能。
- **kernel 從不自己跑、從不被排程**：總是處於**回應模式**——有應用或模組要服務、或有例外 / 中斷時，kernel 才執行、送回結果，然後退場。一般 OS 的 kernel 會自己排程自己、做記帳；這裡不會。看起來近乎無狀態（其實不是）。
- 介面：匯流排上有來自執行緒、排程器、同步、信號、訊息、通道、計時器、鎖、中斷的事件；狀態被改變的物件：執行緒、（同機或跨機）執行緒間的連線、排程與分派機制、計時器、中斷向量。

**其他**：

- 驅動、應用、協定、檔案系統都在 kernel **之外**、在受記憶體保護的使用者空間。
- 鬆耦合：任何元件隨時可以失敗、重新啟動，不影響其他元件。
- 應用可攜性好（POSIX-RT）；幾乎每個服務都是使用者程序；特殊程序 **proc** 負責建立程序，microkernel 協助記憶體管理。
- 老師：可以把它看成**分散式 OS**——kernel 只是接在匯流排上的一個元件，連內部都是分散的；元件甚至可以跑在別台機器上。
- （上一支影片筆記：老師說 QNX Neutrino 曾用在 BlackBerry 手機。）

【課本補充】（p.128–129）

- QNX Software Systems 的產品；用於任務關鍵應用：醫療儀器、網際網路路由器、遙測裝置、程序控制、**航管**。
- microkernel 可細粒度擴展、可設定到非常小——大量產品中，記憶體成本省 1% 就是數百萬美元。
- Neutrino 和它的 microGUI **Photon** 在很小的記憶體上跑得很快，用於網路家電、機上盒、MP3 播放器、工業與醫療設備。
- 已移植到 x86、MIPS、PowerPC、ARM；非商用版可以免費下載。

---

## 7. OSEK/VDX

[42:00]–[44:30]

- **OSEK** 是德文「開放系統」的縮寫（老師說他不懂德文，但意思是這樣）：汽車電子的開放系統。**VDX** = Vehicle Distributed eXecutive。
- 汽車裡有很多互連的處理單元；架構：應用軟體 → OSEK OS → 微控制器。
- 少數**專為車輛設計**的 OS；可以在 **ROM** 上執行。
- 兩種主要實體：**中斷**和**任務**。
- 優先權由高到低：
  1. 中斷：有的和 OS 服務綁在一起、有的沒有。中斷處理最高。
  2. OS（稍低於中斷處理）。
  3. 使用者層任務，支援不同程度的可搶占性。
- 老師：車上不做太多通用運算，主要是**記錄互動、對事件反應**——踩煞車、感測器輸入顯示可能碰撞、要打開安全氣囊——和 QNX 一樣是**反應式**的。

---

## 8. 【課本補充】pSOS

（課本 §7.1，p.126–128；第 22 支影片老師以 pSOS 為 host-target 的例子）

- 主要用在嵌入式應用；由 Wind River 提供；**host-target** 型。例：手機基地台。
- Host（Unix 或 Windows）：編輯器、cross-compiler、source-level 除錯器 **XRAY+**、函式庫。Target 板：處理器、ROM、RAM；ROM 上有 pSOS+ 和選用模組：
  - **PNA+**：網路管理器，經 Ethernet、FDDI 提供 host 與 target 間的 TCP/IP 通訊；相容 BSD 4.3 socket 語法、ftp、NFS。
  - **PHILE**（檔案系統）、**PROBE+**：target 除錯器；XRAY+ 叫用 PROBE+ 提供無縫除錯。
- 開發時程式下載到 target 的 RAM；跑好之後燒進 ROM。
- **32** 個優先權等級；最小設定下 target OS 只佔 **12 KB**。
- 支援 **priority inheritance** 和 **priority ceiling** 協定。
- 分段式記憶體管理、**不支援虛擬記憶體**（給中小型嵌入式應用）。**Memory region**：OS 依應用要求建立的實體連續記憶體區塊，可以分配給任務。
- 中斷：大多數 OS 中斷時跳進 kernel；pSOS 的**裝置驅動在 kernel 之外**、可執行期載入移除，中斷時處理器**直接**跳到向量表指向的 ISR——為了速度，也讓開發者完全控制中斷處理。

---

## 9. 【實測】Linux 上的執行緒與 mutex 延遲

用老師 eCos vs RTLinux 表的同樣指標，量本機 Linux（一般 kernel、SCHED_OTHER、綁在一顆 CPU；各 2000 次）：

```c
pthread_t t; double t0 = ns(); pthread_create(&t, 0, body, 0); double t1 = ns();
pthread_join(t, 0); double t2 = ns();
double c = started - t0, j = t2 - t1;           /* create->first instruction ; join(=deletion) */
```

```
thread creation (create -> thread runs): 4.0 .. 48.2 us
thread deletion (join after exit):       3.6 .. 18.3 us
mutex lock+unlock, uncontended:          11.2 ns
mutex hand-off (unlock -> waiter runs):  1.1 .. 46.3 us
```

看到什麼：

- 最好情況：執行緒建立 4 μs、刪除 3.6 μs、mutex 交接 1.1 μs——比老師表上的 eCos、RTLinux 還快（硬體快了 20 年）。
- 但**最壞情況**是最好的 10–40 倍（48 μs、18 μs、46 μs）。一般 Linux 沒有保證，jitter 大；RTOS 比的不是平均快，而是**最壞情況有上限**。
- 沒有競爭的 mutex 只要 11 ns（futex 在使用者空間用原子指令完成，不進 kernel）。
- 這些數字和 eCos / RTLinux 的數字**不能直接比**（硬體、量法都不同）；這正是第 26 支影片 benchmark 要處理的問題。

---

## 10. 一頁速查表：總比較

| RTOS | 架構 | 大小 | 開發方式 | 特色 |
|---|---|---|---|---|
| eCos | 全在 kernel mode、單程序多執行緒 | 幾十 KB | host-target | POSIX ＋ µITRON；可設定；文件好 |
| µC/OS-II | 可搶占 kernel | ~20 KB、5500 行 C | — | 固定大小分割區；FAA 認證；開源但商用收費 |
| VRTX | mc：極小；sa：可擴展 | 4–8 KB（mc） | — | 第一個 FAA 認證；sa：POSIX-RT、PI、確定性系統呼叫、哈伯望遠鏡 |
| VxWorks | monolithic（後來有記憶體保護） | — | host-target（Tornado、VxSim） | 0–255 優先權、task locking、PI、register window；火星、787、Pathfinder |
| LynxOS | microkernel ＋ KPI | 28 KB | self-host | 256 級、單一排程政策、100 Hz tick；Linux 二進位相容 |
| QNX Neutrino | microkernel ＋ software bus | 很小、可裁剪 | — | 被動 kernel；元件可重啟；航管、醫療 |
| OSEK/VDX | 汽車 | ROM | — | 中斷 > OS > 任務 |
| pSOS（課本） | 驅動在 kernel 外 | 12 KB | host-target | 32 級；PI 與 PCP；memory region |

---

## 11. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| **Windows CE（256 級、priority inheritance 處理 DPC）** | **第 25 支**：Windows CE |
| **執行緒建立、mutex、context switch 延遲的量測** | **第 26 支**：Benchmarking RTOSs |
| **CAN bus（µC/OS-II、OSEK 都支援）** | **第 27–28 支**：Real Time Communication |

---

## 附：本講一句話總結

> **商用與經典 RTOS 的共同點是小、可裁剪、確定性高，但架構各異：eCos 只要幾十 KB、單程序多執行緒、全部跑在 kernel mode，相容 POSIX 與 µITRON；µC/OS-II 約 20 KB、5500 行 ANSI C、用固定大小分割區管理記憶體、通過 FAA 認證（課本說每個任務獨佔 64 個優先權之一、不支援 round robin）；VRTX 從 4 KB 的 VRTXmc（第一個 FAA 認證的商用 RTOS）發展到符合 POSIX-RT、有 priority inheritance 與確定性系統呼叫的 VRTXsa（哈伯望遠鏡）；Wind River 的 VxWorks 是 host-target 的 monolithic RTOS，有 0–255 優先權、task locking、PI mutex、Tornado/VxSim 工具鏈和 register window 加速 context switch，用在火星任務、Boeing 787，也是 Pathfinder 事件的主角；LynxOS 是 28 KB 的 microkernel 加 kernel plug-in、self-host、能直接跑 Linux 二進位，但只有一種排程政策、tick 固定 100 Hz；QNX Neutrino 用 microkernel 加 software bus，kernel 是被動的、只回應訊息，元件都在受保護的使用者空間、可以獨立重啟；OSEK/VDX 則是專為汽車設計、中斷優先於 OS 與任務的反應式系統。實測一般 Linux 的執行緒建立、mutex 交接最好只要 1–4 μs、比老師表上的 eCos 和 RTLinux 還快，但最壞情況是最好的 10–40 倍——RTOS 要比的正是最壞情況。**

---

*筆記依據 Smruti R. Sarangi「Real Time Systems」Lecture 25 錄影逐字稿（YouTube 自動字幕，45:00）、Rajib Mall *Real-Time Systems*（IIT Kharagpur Version 2 CSE 講義版）Ch5 §7.1–7.5、7.7 整理。實測為本機（Ubuntu、kernel 6.5.0、x86-64）結果。*
