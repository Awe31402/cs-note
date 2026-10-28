# Real Time Systems 第二十五支影片完整筆記：Windows CE（Lecture 26）

> **課程**：Real Time Systems — Lecture 26：Windows CE（播放清單第 25 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）；投影片與課本為 Rajib Mall（IIT Kharagpur）
> **來源**：YouTube 錄影逐字稿（28:22，自動字幕）＋ 課本 *Real-Time Systems*（Rajib Mall；IIT Kharagpur「Version 2 CSE」講義版 PDF）Ch5 §4.3、§7.8（PDF p.119、130）
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
- [1. Windows CE 是什麼](#1-windows-ce-是什麼)
- [2. CE 5 的記憶體模型：32 個 32 MB 的槽](#2-ce-5-的記憶體模型32-個-32-mb-的槽)
- [3. CE 6：每程序 2 GB 虛擬記憶體、服務搬進 kernel](#3-ce-6每程序-2-gb-虛擬記憶體服務搬進-kernel)
  - [3.1 使用者空間的配置](#31-使用者空間的配置)
  - [3.2 Kernel 空間的配置](#32-kernel-空間的配置)
  - [3.3 Kernel mode 與 user mode 驅動](#33-kernel-mode-與-user-mode-驅動)
- [4. Windows CE 當 RTOS](#4-windows-ce-當-rtos)
  - [4.1 OMAC 的需求](#41-omac-的需求)
  - [4.2 ISR 與 IST 的量測](#42-isr-與-ist-的量測)
  - [4.3 其他即時特性](#43-其他即時特性)
- [5. 【原始碼】Linux 的 UIO：小 ISR ＋ 使用者空間的 IST](#5-原始碼linux-的-uio小-isr--使用者空間的-ist)
- [6. 【實測】一般 Linux 能不能達到 OMAC 的要求](#6-實測一般-linux-能不能達到-omac-的要求)
- [7. 行動應用與 Android](#7-行動應用與-android)
- [8. 一頁速查表](#8-一頁速查表)
- [9. 與後續講次的連結](#9-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

1. Windows CE 的兩代記憶體模型：CE 5 的「槽」→ CE 6 的完整虛擬記憶體，並把驅動、檔案系統、GUI 等**搬進 kernel**。
2. 當 RTOS：中斷分成 **ISR**（kernel 的小 top half）和 **IST**（使用者模式的 bottom half，和即時程序一起競爭）；priority inheritance；單一 page table 讓 context switch 快。
3. 行動應用與 Android 的優先權。

---

## 1. Windows CE 是什麼

[00:00]–[02:30]

- **CE = Compact Embedded**；源自早期給 PDA 和手機的 Windows Phone OS。
- Windows 仍有嵌入式社群；最近推出 **ThreadX** 的 Azure 延伸（Azure RTOS）。和 CE 程式碼關係多深老師不清楚，但公司的新產品通常是舊產品的延伸。
- 老師也不知道 Microsoft Surface 跑的 OS 有多少來自 CE。
- **Windows CE 6**：完整的 **32 位元** OS，4 GB 虛擬記憶體分給 kernel 和 user mode。
- 目標：平板、智慧型手機、機上盒、thin client、汽車、機器人……（老師：CE 能用，但承認不太流行）。
- 有彈性、可設定、可調整、**小**；支援 ARM、MIPS、x86 等 ISA。
- 簡單的**驅動模型**：嵌入式處理器需要很多周邊和它們的驅動。要注意**功耗**。

---

## 2. CE 5 的記憶體模型：32 個 32 MB 的槽

[02:30]–[06:01]

- CE 5 的限制：最多 **32 個程序**、每個 **32 MB**——類似 base–limit 的模型。

```
 4 GB ┌──────────────────────┐
      │ Kernel（2 GB）         │
 2 GB ├──────────────────────┤
      │ 共享記憶體（1 GB）       │ ← 所有程序都能讀寫（共用資料、程式碼、IPC）
 1 GB ├──────────────────────┤
      │ 32 個槽 × 32 MB = 1 GB  │ ← 每個程序一個槽
      │ slot 0、1：目前執行中的程序、ROM 裡的 DLL       │
      │ 有些槽保留給系統：裝置驅動、檔案系統、系統服務…   │
      │ 其餘給使用者程序                               │
    0 └──────────────────────┘
```

- 對應 32 位元 Windows：2 GB kernel、2 GB user。
- User 的 2 GB 裡，私有部分其實只有 32 × 32 MB = **1 GB**；另外 1 GB 共享。
- 老師：不完全是嵌入式 / 即時、但想給使用者程式通用功能的 OS 常這樣做——不用純虛擬記憶體，而是給一塊連續區域、要程式待在裡面。

---

## 3. CE 6：每程序 2 GB 虛擬記憶體、服務搬進 kernel

[06:01]–[16:30]

CE 6 大改：

- 每個程序 **2 GB 虛擬記憶體**、最多 **32,000** 個程序（取 2 的冪大概是 32,768 = 2¹⁵）。
- **統一的 kernel**：不再是 microkernel ＋ executive ＋ user land 系統服務的設計，很多關鍵 OS 元件**搬進 kernel space** → 效能提高、安全提高、向後相容較好（使用者程式只要依賴簡單的系統呼叫介面，不必管「哪些在 user mode、哪些在 kernel mode」）。

⚠️ 老師說「2 GB = 2³¹ bytes，所以這是給 **64 位元** OS 的」[06:30]，但他前面才說 CE 6 是**完整的 32 位元** OS [01:30]。32 位元位址空間 4 GB，分成 2 GB kernel ＋ 2 GB user 正好合理；「64 位元」應是口誤。

- 老師：CE 6 已經很大，不算我們談的那種嚴格即時，但曾經在嵌入式很流行。Microsoft 的嵌入式產品「時盛時衰」。

### 3.1 使用者空間的配置

[09:00]–[11:03]

每個程序 2 GB 裡：

| 區域 | 內容 |
|---|---|
| Process space（1 GB） | 可執行碼與資料、虛擬記憶體配置、file-backed 映射檔 |
| User DLLs | 程式用到的程式碼與資料映射到這裡；**所有程序的對應相同**；資料頁各自獨立、程式碼頁共享 |
| RAM-backed 映射檔 | 系統資源與程式位址之間的映射 |
| **Shared system heap** | 和 kernel 溝通的區域：OS 元件可讀寫，使用者程序**只能讀** → 像一條從 kernel 到程序的安全單向管道 |

### 3.2 Kernel 空間的配置

[11:03]–[12:32]

Kernel 的 2 GB：

| 區域 | 內容 |
|---|---|
| 第一個 512 MB | kernel 程式碼與資料，**cached**（告訴 CPU 放進快取） |
| 下一段 | **uncached** 存取實體記憶體 |
| Kernel DLLs | — |
| **Object store** | 檔案系統：Windows 把一切（檔案、裝置）都當物件 |
| Kernel 虛擬記憶體 | 所有 server 和驅動共用 |
| CPU 特定的虛擬記憶體（最上面） | trap 區：中斷處理、例外處理 |

整個 kernel 空間是**固定映射**。

最有趣的是：一般 Windows 不這樣做，但 CE 把**驅動、檔案系統、GUI** 等全搬進 kernel，打包進同一個 DLL **coredll** → 大幅降低系統呼叫和 OS 基本服務之間的開銷、增加程式碼共享。

- 老師：和 eCos 等一樣的設計模式：把東西都包進 kernel 降低元件間的開銷。嵌入式 OS 為特定系統量身打造，不需要太多彈性（有也無妨，但不是重點）。

```
 Applications ＋ shell │ 少數 user mode 服務
 DLL：coredll、通訊 DLL、圖形 DLL、Winsock…
 ─────────────────────────────────────────── kernel
 Core kernel、檔案系統、網路、裝置驅動、核心服務（以 DLL 形式）
 Boot loader
 Hardware
```

**CE 5 vs CE 6 處理一個服務請求**：

- CE 5：很多服務在 user space → 應用進 kernel → kernel 叫 user space 的服務 → 回來。
- CE 6：幾乎所有服務都在 kernel → 在 kernel 裡做完就回來 → 開銷大減，代價是彈性。（第 19 支影片的 microkernel vs monolithic 取捨。）

### 3.3 Kernel mode 與 user mode 驅動

[15:00]–[16:30]

| | Kernel mode 驅動 | User mode 驅動 |
|---|---|---|
| 載入者 | `device.dll`，載入 kernel space 後常駐 | `udevice.exe` |
| 存取 | 可存取所有 kernel 資料結構與記憶體 | 不能存取 kernel 結構與記憶體 |
| 優點 | **效能** | **彈性、穩健**：當掉時影響被侷限 |
| 缺點 | 安全風險；驅動當掉整個 kernel 跟著倒 | 較慢 |
| 適合 | 高效能裝置 | 不需要高效能的裝置：USB、鍵盤、滑鼠 |

---

## 4. Windows CE 當 RTOS

### 4.1 OMAC 的需求

[16:30]–[18:00]

**OMAC 使用者團體**的分析（工業自動化）：

- **95%** 的即時應用需要 **0.5 ms（500 μs）到 10 ms** 的回應時間。
- 能容忍約 **10%** 的變動（jitter）：**50 μs 到 1 ms**。
- 中斷每 0.5–10 ms 來一次，這也是處理一個中斷的最長時間：每個中斷要在下一個來之前處理完。
- 就算號稱週期性的中斷，也可能有 10% 的變動。

### 4.2 ISR 與 IST 的量測

[18:00]–[20:01]

測試設定：Samsung 開發板、**200 MHz ARM**、Windows CE 5、完整使用者介面、同時播放 WMV 影片。

CE 把中斷分成：

- **ISR（interrupt service routine）**：top half，在 kernel。
- **IST（interrupt service thread）**：bottom half，在**使用者空間**的執行緒。

| | 最小 | 平均 | 最大 | 最大 / 最小 |
|---|---|---|---|---|
| ISR | 1.2 μs | 3.3 μs | 13.3 μs | 約 10 倍 |
| IST | 31.7 μs | — | 103 μs | 約 3 倍 |

老師：ISR 尾端變動相對大（約 10 倍）；IST 從倍數看變動沒那麼大（約 3 倍）。

（對照 OMAC：最壞 IST 103 μs，遠低於 0.5 ms 的最短需求。）

### 4.3 其他即時特性

[19:30]–[22:30]

**大小**：最小的 kernel 叫 **MinKern**，約 **350 KB**（沒有圖形和視窗支援）——比前一講幾十 KB 的 kernel 大得多，但支援多程序、多執行緒、DLL、虛擬記憶體、大量 I/O 驅動。

**中斷處理**（像 Linux 的 top / bottom half）：

1. 中斷發生 → kernel 存目前 user mode 程式的狀態。
2. kernel 呼叫 **ISR**：只做最低限度的服務，期間其他中斷被關閉 / 遮罩——要**盡量縮短**遮罩時間。
3. ISR 把 **IST** 排進佇列稍後執行。
4. User mode 的 IST 做大部分工作，而且**和使用者模式的即時程序一起競爭**——老師說這證明了它的即時性（和 Windows NT 的 DPC 不同，第 20 支影片）。

**資源共享**：從 **CE 3** 起支援 **priority inheritance**；更早的版本程序可能遭受 unbounded priority inversion。

**Context switch**：

- 要快：微秒甚至次微秒等級，不是毫秒。
- Windows 2000 起每個程序有自己的 page table。
- CE **讓所有程序共用一個 page table** → context switch 開銷變小。做法可能是 inverted page table，或把 address space ID 放進虛擬位址。（逐字稿沒說是哪一種。）

⚠️ 老師說「Windows 2000 也就是 Windows Millennium」[22:00]。Windows 2000 屬 NT 系、Windows Me 屬 9x（DOS）系，是不同的產品線（第 20 支影片課本的系譜）。

**CE vs 桌機 Windows**：不像 NT / 2000 / XP，CE 5 至少**不支援多處理器**；還有 32 個並行程序的限制等。很多 kernel 模組可能共用程式碼，但不確定是否完全相同。

**Windows CE .NET**：給行動應用；支援 Bluetooth、IPv6、安全協定、Kerberos。Microsoft 的行動平台從智慧型手機、筆電到平板，最新是 Surface / Surface Pro。

【課本補充】（課本 §7.8，p.130；§4.3，p.119）

- Windows CE 是精簡的 Windows，最小 footprint **400 KB**。
- **256** 個優先權等級。
- 為了效能，**所有執行緒都在 kernel mode 執行**。
- `sleep`、`wait` 相關 API 的計時器精度 **1 ms**。
- kernel 功能被拆成小的不可搶占區段 → 系統呼叫期間只關一小段時間搶占；中斷服務可搶占，支援**巢狀中斷**。課本 §4.3 把 Windows CE 列為 **preemption point** 做法的商用例子（第 22 支影片筆記）。
- 用 MMU 做虛擬記憶體管理。
- **priority inheritance 解決 NT 的 DPC 問題**：處理 page fault 的 kernel 執行緒（DPC）原本優先權高於 NORMAL；當優先權為 NORMAL 的執行緒發生 page fault，處理它的 kernel 執行緒優先權會被調到**引發 page fault 的執行緒**的優先權 → 執行緒不會被更低優先權的執行緒擋住，即使在 page fault 時。

⚠️ 大小：老師說 MinKern **350 KB**，課本說最小 **400 KB**；數字接近，版本可能不同。
⚠️ 執行模式：課本說「所有執行緒都在 kernel mode」，老師說 IST 在**使用者空間**、CE 6 還有 user mode 驅動與服務。兩者講的可能是不同版本（課本講義約 2007 年），待補。

---

## 5. 【原始碼】Linux 的 UIO：小 ISR ＋ 使用者空間的 IST

Linux 也有「kernel 裡只放很小的中斷處理、真正的驅動邏輯在使用者空間」的框架：**UIO（Userspace I/O）**。

`drivers/uio/uio.c:440`

```c
 * uio_interrupt - hardware interrupt handler
 * @irq: IRQ number, can be UIO_IRQ_CYCLIC for cyclic timer
 * @dev_id: Pointer to the devices uio_device structure
 */
static irqreturn_t uio_interrupt(int irq, void *dev_id)
{
	struct uio_device *idev = (struct uio_device *)dev_id;
	irqreturn_t ret;

	ret = idev->info->handler(irq, idev->info);
	if (ret == IRQ_HANDLED)
		uio_event_notify(idev->info);

	return ret;
}
```

對照：

| Windows CE | Linux UIO |
|---|---|
| ISR：kernel 裡最低限度的處理 | `uio_interrupt()`：呼叫驅動提供的極小 `handler`（通常只確認、關掉裝置中斷） |
| 把 IST 排進佇列 | `uio_event_notify()`：增加事件計數、喚醒等在 `/dev/uioX` 上的程序 |
| User mode 的 IST 做大部分工作、和即時程序競爭 | 使用者程序對 `/dev/uioX` 做 `read()`（阻塞直到中斷）後處理；它是一般執行緒，可以設成 SCHED_FIFO 和其他即時執行緒排優先權 |
| User mode 驅動：穩健、當掉影響被侷限 | 同左：使用者空間的驅動當掉不會拖垮 kernel |

（本機沒有 UIO 裝置：`ls /dev/uio*` 沒有結果；UIO 多用在工業 I/O 卡、FPGA 等。）

---

## 6. 【實測】一般 Linux 能不能達到 OMAC 的要求

用第 22 支影片筆記的 cyclictest 式程式（一般 SCHED_OTHER 任務、綁在 CPU 3），把週期設成 OMAC 範圍的兩端：

```
idle                       period  500 us: avg late   51.5 us, worst    213.7 us, OMAC jitter budget 50 us
idle                       period 10000 us: avg late  177.7 us, worst    273.8 us, OMAC jitter budget 1000 us
3 CPU hogs                 period  500 us: avg late  208.7 us, worst  24437.6 us, OMAC jitter budget 50 us
```

- **10 ms 週期**：最壞晚 274 μs < 1 ms 的 jitter 預算 → 一般 Linux 閒置時就達標。
- **0.5 ms 週期**：最壞晚 214 μs，**超過** 50 μs 的預算 4 倍（timer slack 50 μs ＋ 深度 C-state，第 22 支影片筆記）。
- **有 CPU 密集任務搶同一顆 CPU**：最壞 24 ms，連下一個週期都過了好幾十個——沒有即時優先權的一般任務完全不行。
- 對照 CE 5 在 200 MHz ARM 上 IST 最壞 103 μs：硬體慢得多，但有即時設計，反而比一般 Linux 上的一般任務穩。要在 Linux 上達到 0.5 ms 等級，就需要 SCHED_FIFO（timer slack 為 0）、mlock、關深度 C-state，甚至 PREEMPT_RT。

---

## 7. 行動應用與 Android

[24:01]–[28:01]

**行動應用的限制**：記憶體、緩衝、螢幕、處理能力都有限；常斷線 → 需要系統支援**斷線操作**（例如預先抓網頁、快取，斷線時還能瀏覽），都在大致是軟即時的框架下提供。

三類行動應用：

1. **獨立應用**：不用網路。
2. **簡單 client–server**：短暫的網路通訊。
3. **進階應用**：例如分散式遊戲，**即時通訊**很重要（之後的講次）。

老師：即時 OS 和即時通訊要一起看。

**Android**（現在拿走了 Windows 很多市佔）：

- 每個程序跑在一個虛擬機器上：類似 JVM，但叫 **Dalvik VM**；應用用 Java 寫，轉成 Dalvik 執行檔。
- 底下的 Linux kernel 處理安全、執行緒、程序、記憶體管理等低階功能。
- 多一層不一定是壞事，只要能建立某些保證和界限（老師不談 Android 的即時性）。

**App 的優先權**：應用**不能控制自己的生命週期** → 系統可以積極地拿走資源或殺掉應用，提高裝置回應性。

| 優先權 | 程序 | 例子 |
|---|---|---|
| Critical | Active process | 正在通話的電話程序 |
| High | Visible process | 正在看的影片 |
| Low | Background process | 背景清理頁面的程序 |
| — | Empty / idle process | 表示什麼都沒在做 |

下一章：RTOS 的 benchmark，然後是即時通訊。

---

## 8. 一頁速查表

| 主題 | 重點 |
|---|---|
| Windows CE | Compact Embedded；ARM / MIPS / x86；小、可設定；簡單驅動模型；省電 |
| CE 5 記憶體 | 2 GB kernel ＋ 2 GB user；32 槽 × 32 MB（1 GB 私有）＋ 1 GB 共享；最多 32 程序 |
| CE 6 記憶體 | 32 位元；每程序 2 GB 虛擬記憶體；32K 程序；shared system heap（kernel 可寫、user 只讀） |
| CE 6 架構 | 驅動、檔案系統、GUI 搬進 kernel（coredll）→ 開銷低、彈性低 |
| 驅動 | kernel mode（device.dll，快但危險）vs user mode（udevice.exe，穩健，給 USB、鍵盤滑鼠） |
| OMAC | 95% 應用要 0.5–10 ms 回應、10% jitter（50 μs–1 ms） |
| ISR / IST | 200 MHz ARM：ISR 1.2–13.3 μs（平均 3.3）、IST 31.7–103 μs；IST 在 user space 和即時程序競爭 |
| 其他 | MinKern 350 KB（課本 400 KB）；CE 3 起 PI；共用一個 page table 讓 context switch 快；課本：256 級、1 ms 計時器、preemption point、PI 處理 page fault DPC |
| Linux 對照 | UIO：`uio_interrupt()` 小 handler ＋ 使用者空間讀 `/dev/uioX` |
| 實測 | 一般 Linux：10 ms 週期達標、0.5 ms 週期超出 jitter 預算 4 倍、有 CPU hog 時最壞 24 ms |
| Android | Dalvik VM ＋ Linux kernel；app 不控制生命週期；critical > visible > background > empty |

---

## 9. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| **ISR / IST 延遲、context switch 等指標的量測** | **第 26 支**：Benchmarking RTOSs |
| **行動裝置的即時通訊** | **第 27–30 支**：Real Time Communication |

---

## 附：本講一句話總結

> **Windows CE（Compact Embedded）是給 PDA、手機、機上盒、汽車和機器人的小型可設定 Windows：CE 5 用 32 個 32 MB 的「槽」限制最多 32 個程序，CE 6 改成每個程序 2 GB 虛擬記憶體（仍是 32 位元）、最多 32K 個程序，並把驅動、檔案系統、GUI 都搬進 kernel 的 coredll 以降低開銷，只保留 user mode 驅動給 USB、鍵盤這類慢裝置以求穩健。當 RTOS 時，它把中斷分成 kernel 裡極短的 ISR 和在使用者空間、和即時程序一起競爭優先權的 IST（200 MHz ARM 上 ISR 1.2–13.3 μs、IST 31.7–103 μs，滿足 OMAC 所說 95% 工業應用需要的 0.5–10 ms 回應與 10% jitter），從 CE 3 起支援 priority inheritance（課本說它用 PI 讓處理 page fault 的 kernel 執行緒繼承引發者的優先權，解決 NT 的 DPC 問題），並讓所有程序共用一個 page table 來加快 context switch。Linux 的 UIO 正是同樣的「小 ISR 加使用者空間 IST」結構；而實測一般 Linux 的一般任務在 10 ms 週期能達到 OMAC 要求，0.5 ms 週期就超出 jitter 預算 4 倍、遇到 CPU 密集任務時更會晚到 24 ms。最後老師提到行動應用的斷線操作與 Android 依 critical、visible、background 分級殺程序的做法。**

---

*筆記依據 Smruti R. Sarangi「Real Time Systems」Lecture 26 錄影逐字稿（YouTube 自動字幕，28:22）、Rajib Mall *Real-Time Systems*（IIT Kharagpur Version 2 CSE 講義版）Ch5 §4.3、§7.8 整理。原始碼為 Linux v6.2.12；實測為本機（Ubuntu、kernel 6.5.0、x86-64）結果。*
