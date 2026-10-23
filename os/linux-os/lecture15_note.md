# Operating Systems: A Linux Kernel-Oriented Approach 第十五講完整筆記：x86 的中斷架構（Chapter 4, Part II）

> **課程**：Operating Systems: A Linux Kernel-Oriented Approach — Lecture 15（Chapter 4, Part II）
> **講者**：Smruti R. Sarangi
> **來源**：YouTube 錄影逐字稿（1:07:33，自動字幕）＋ 課本 *Operating Systems: A Linux Kernel-Oriented Approach*（Sarangi, v0.96）§4.2–4.2.6（p.121–138）＋ Linux kernel **v6.2.12** 原始碼（`os/linux-src/`）
> **標記說明**：
> - `【口頭】`：教授在課堂上口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與教授回答
> - `【課本補充】`：課本有、但這堂課沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄（`檔名:行號`），用來對照觀念
> - `⚠️ 逐字稿缺字`：自動字幕在這裡掉字或聽不清，內容依上下文或課本補上
> - `[mm:ss]`：影片時間點

---

## 目錄

- [0. 這堂課要做什麼](#0-這堂課要做什麼)
- [1. APIC：中斷控制器](#1-apic中斷控制器)
  - [1.1 兩種 APIC：LAPIC 與 I/O APIC](#11-兩種-apiclapic-與-io-apic)
  - [1.2 IRQ 是一條線（或一個訊息）](#12-irq-是一條線或一個訊息)
  - [1.3 APIC 的處理流程](#13-apic-的處理流程)
  - [1.4 IRQ 與中斷向量：多對多](#14-irq-與中斷向量多對多)
  - [1.5 Edge 與 level：要不要 acknowledge](#15-edge-與-level要不要-acknowledge)
  - [1.6 I/O APIC：路由器](#16-io-apic路由器)
  - [1.7 LAPIC：每顆核心的中斷控制器](#17-lapic每顆核心的中斷控制器)
  - [1.8 處理器間中斷（IPI）](#18-處理器間中斷ipi)
- [2. 中斷要送給哪顆核心](#2-中斷要送給哪顆核心)
- [3. `/proc/interrupts`](#3-procinterrupts)
  - [3.1 課本的例子](#31-課本的例子)
  - [3.2 【實測】這台電腦的 `/proc/interrupts`](#32-實測這台電腦的-procinterrupts)
- [4. IRQ 共用與中斷向量的範圍](#4-irq-共用與中斷向量的範圍)
  - [4.1 一條 IRQ 線、很多裝置](#41-一條-irq-線很多裝置)
  - [4.2 IRQ 與向量的對應](#42-irq-與向量的對應)
  - [4.3 中斷向量的範圍](#43-中斷向量的範圍)
  - [4.4 【原始碼】v6.2.12 的向量表](#44-原始碼v6212-的向量表)
- [5. CPU 內部：查 IDT](#5-cpu-內部查-idt)
- [6. Kernel 的中斷資料結構](#6-kernel-的中斷資料結構)
  - [6.1 `struct irq_desc`](#61-struct-irq_desc)
  - [6.2 Flow handler](#62-flow-handler)
  - [6.3 `struct irqaction`：裝置的 handler 串列](#63-struct-irqaction裝置的-handler-串列)
  - [6.4 三種回傳值](#64-三種回傳值)
  - [6.5 【原始碼】`irq_desc`、`irqaction`、`irqreturn`](#65-原始碼irq_descirqactionirqreturn)
- [7. IRQ domain](#7-irq-domain)
- [8. 開機時設定 IDT 和 APIC](#8-開機時設定-idt-和-apic)
  - [8.1 BIOS 先設一個陽春版](#81-bios-先設一個陽春版)
  - [8.2 `start_kernel` 裡的初始化](#82-start_kernel-裡的初始化)
  - [8.3 設定 LAPIC：MSR](#83-設定-lapicmsr)
  - [8.4 【原始碼】`x86_init.irqs`：架構專屬的函式指標](#84-原始碼x86_initirqs架構專屬的函式指標)
- [9. 中斷的路徑](#9-中斷的路徑)
  - [9.1 `common_interrupt`](#91-common_interrupt)
  - [9.2 走 `irqaction` 串列](#92-走-irqaction-串列)
  - [9.3 【原始碼】`common_interrupt` 與 `__handle_irq_event_percpu`](#93-原始碼common_interrupt-與-__handle_irq_event_percpu)
  - [9.4 【原始碼】driver 怎麼掛上 handler：`request_irq`](#94-原始碼driver-怎麼掛上-handlerrequest_irq)
- [10. Top half 的限制](#10-top-half-的限制)
- [11. 一頁速查表](#11-一頁速查表)
- [12. 與後續講次的連結](#12-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 這堂課要做什麼

`[00:34]` 第 4 章第 2 講：**x86 機器的中斷處理架構**，以及 Linux kernel 怎麼利用它。

```
  裝置 ──IRQ──► I/O APIC ──► LAPIC ──中斷向量──► CPU ──查 IDT──► kernel 的入口
                （路由）     （每核心）                         │
                                                              ▼
                                                    irq_desc → flow handler
                                                              │
                                                              ▼
                                                    走 irqaction 串列，
                                                    呼叫 driver 的 handler
```

| # | 主題 | 對應課本 |
|---|---|---|
| 1 | **APIC**：LAPIC、I/O APIC、IPI、中斷分配 | §4.2.1 |
| 2 | **IRQ**：`/proc/interrupts`、IRQ 共用、中斷向量的範圍 | §4.2.2 |
| 3 | kernel 的資料結構：`irq_desc`、`irqaction` | §4.2.3 |
| 4 | **IRQ domain** | §4.2.4 |
| 5 | 開機時怎麼設定 IDT 和 APIC | §4.2.5 |
| 6 | 一個中斷從進來到 driver 的完整路徑 | §4.2.6 |

---

## 1. APIC：中斷控制器

`[00:34]` **APIC** = **Advanced Programmable Interrupt Controller**（進階可程式化中斷控制器）。

`[01:00]` 

> 「可程式化中斷控制器已經存在很久了，但事情變得越來越複雜。APIC 有**非常階層化**的設計，而且**它們本身就變成了處理器**——光是處理和路由中斷，就需要相當的處理能力。特別是 Intel 處理器，有**專門的晶片**管理中斷。管理牽涉很多活動，**不只是**排隊、緩衝，**遠不止如此**。」

### 1.1 兩種 APIC：LAPIC 與 I/O APIC

`[01:31]`（課本 Fig 4.4）

```
                         ┌──────────────────────────┐
                         │  CPU  ┌──────┐ ┌──────┐   │
   裝置 1 ──IRQ──┐        │       │Core 0│ │Core 1│   │
   裝置 2 ──IRQ──┤   ┌────┴───┐   │LAPIC │ │LAPIC │   │
   裝置 3 ──IRQ──┼──►│I/O APIC│──►├──────┤ ├──────┤   │
   ...           │   └────┬───┘   │Core 2│ │Core 3│   │
   裝置 N ──IRQ──┘        │       │LAPIC │ │LAPIC │   │
                         │       └──────┘ └──────┘   │
                         └──────────────────────────┘
```

| | **LAPIC**（Local APIC） | **I/O APIC** |
|---|---|---|
| 數量 | **每顆 CPU 核心一個** | 傳統上**整個系統一個**；`[02:00]`「現在可以有不只一個」 |
| 在哪 | 核心裡 | 主機板上，晶片組的一部分 |
| 工作 | 把中斷轉成**中斷向量**送給 CPU；收 IPI；有一個小 timer | 管理 I/O 裝置產生的**外部中斷**，**路由**給 LAPIC |

（⚠️ 教授說現在可以有多個 I/O APIC；課本說「整個系統只有一個 I/O APIC」。§3.2 實測這台電腦用的是 `IR-IO-APIC`。）

### 1.2 IRQ 是一條線（或一個訊息）

`[02:31]` 每個硬體裝置產生一條 **IRQ 線**（IRQ = **Interrupt ReQuest**，中斷請求）：

| 方式 | 怎麼運作 |
|---|---|
| **傳統** | 一條**實體銅線**，電壓從 0 變 1（active high 的話）→ 有中斷；從 1 降回 0 → 沒中斷 |
| **現代：訊息式（MSI）** | `[03:01]` 送一個**訊息**給 APIC，**沒有真的線**在改變電壓。APIC 把訊息解讀成「**虛擬 IRQ 線**」從 0 變成 1 |

> 「不管怎麼實作——舊方式一條銅線有電壓變化，或者送一個訊息、APIC 解讀成虛擬 IRQ 線從 0 到 1——IRQ 基本上就是一條從裝置經過 APIC 到 CPU 的銅線。」

【課本補充】課本說：在現代 **MSI**（message-signaled interrupts，訊息信號中斷）架構裡，裝置還可以把**一些資料**寫到 APIC 暫存器，APIC 用這些資訊適當地處理中斷、把正確的中斷向量送給相關 CPU。

### 1.3 APIC 的處理流程

`[04:01]`（課本 Fig 4.5）

```
         IRQ 線                  APIC                         CPU
   ────────────────►  ① CPU 可以開 / 關所有中斷
                      ② 選優先權最高的中斷
                      ③ 緩衝中斷向量和資料
                      ④ 被 mask 了就不送，否則送出  ──中斷向量──►
                      ⑤ CPU 確認收到 ◄──────────────────────── acknowledge
```

| 步 | 內容 | 教授的說明 |
|---|---|---|
| 1 | **開 / 關中斷** | CPU 可以叫 APIC **暫時全部**開或關。關掉時，中斷資料封包**排在 APIC 內部的佇列**裡，不送出去 |
| 2 | **按優先權** | 中斷重新打開時，這些封包**依優先權順序**送進 CPU |
| 3 | **Masking（遮罩）** | `[05:01]` 就算中斷是開的，也可以**特別**關掉某幾個。例如關掉 IRQ 4 和 6，其他開著 |
| 4 | **指定中斷向量** | 通過開關和 mask 兩關之後，才指定**中斷向量** |
| 5 | **Acknowledge** | 有些中斷要明確確認（§1.5） |

**Disable vs mask**：

| | Disable（停用） | Mask（遮罩） |
|---|---|---|
| 範圍 | **全部**中斷 | **特定**中斷 |
| 課本比喻 | 「像一把**大鐵鎚**」 | 「比較**細緻**的動作」 |
| 被擋下的中斷 | 排在 APIC 的硬體佇列，重新打開時依優先權處理 | 排在 APIC 的佇列，unmask 後送出 |

【課本補充】課本補充（p.123）：中斷**通常不會遺失**，會排在 APIC 的硬體佇列裡。但**可能溢位**——很少見但可能發生，那時中斷就會遺失。CPU 確認收到之後，APIC **才**把中斷從內部佇列移除。

### 1.4 IRQ 與中斷向量：多對多

`[05:30]` 兩個概念要分清楚：

| | IRQ | 中斷向量（interrupt vector / interrupt number） |
|---|---|---|
| 是什麼 | 送到中斷控制器的**硬體信號**：「某個硬體單元需要被服務」 | 中斷的**唯一識別碼**，用來找要跑哪個中斷服務常式 |
| 誰用 | APIC | **CPU**：拿它查 IDT |
| 課本 | 「kernel 對硬體中斷來源的識別碼」 | 「識別**任何**一種打斷事件：中斷、system call、exception、fault……」 |

**LAPIC 送給 CPU 的是中斷向量，不是 IRQ。**

舊系統裡 IRQ 線和中斷向量是**一對一**的。有了 APIC，變得**很有彈性**，而且可以**動態**改：

| 對應 | 例子 | 教授的說明 |
|---|---|---|
| **多個 IRQ → 同一個向量** | `[05:30]` 一台電腦接了**好幾個滑鼠**，但最後滑鼠點擊就是要服務一次滑鼠點擊——「不管來源是哪個，要產生的都是同一個中斷向量」 | 課本：「這種情況很少見」 |
| **一個 IRQ → 多個向量** | `[06:31]` **網路卡**：一個中斷說「資料來了」、另一個說「你之前送的請求有回覆了」、再一個說「資料在等」 | 「每個向量對應一個 handler，所以可以跑多個 handler 處理同一個中斷——軟體工程裡叫**責任鏈（chain of responsibility）**」 |

> 課本：「一般來說是**多對多**的對應，而且可以動態程式化。產生中斷向量並送給 CPU 是 **LAPIC** 的工作。」

### 1.5 Edge 與 level：要不要 acknowledge

`[07:00]` 兩種中斷：

| | **Level-sensitive（準位觸發）** | **Edge-sensitive（邊緣觸發）** |
|---|---|---|
| 怎麼判斷有中斷 | 信號的**準位**升高 | 信號的**邊緣**（一次轉換，通常是負緣） |
| 怎麼結束 | 處理完，**另一邊把準位拉回 0** | 不用 |
| **Acknowledge** | **要明確確認**：把線從 1 拉回 0，代表 CPU 處理完了 | **大多不用** |
| `/proc/interrupts` 裡叫 | **`fasteoi`**（EOI = End Of Interrupt） | **`edge`** |

`[08:00]` 「APIC 做了**很多事**：很多時候要處理**非常過時的技術**，也要處理**依賴訊息的現代技術**，然後產生一個 **IRQ 封包**：有 IRQ 號碼，也帶一些**資料**（可以是送來的訊息）。」

### 1.6 I/O APIC：路由器

`[09:31]` I/O APIC 的工作是**監督性**的（系統裡只有一兩個，頂多幾個）：

- **IRQ 重新導向（redirection）**：四核心 CPU 的話，把某個 IRQ 路由到某一顆核心。
- 從週邊匯流排和它接的東西收所有中斷。
- `[10:00]` 歷史上 I/O APIC 有 **24 條**中斷線——「不是硬性規定，技術進步數字會增加」。
- 每個裝置分到一個 **IRQ 號碼**：傳統上**號碼越小，優先權越高**。
- **timer 中斷**的優先權相當高，是 **IRQ 0**。

`[10:31]` 例子：插入一個新 USB 裝置，USB 控制器送出中斷，**總要有一顆核心處理**——由 I/O APIC 決定**哪顆**。

【課本補充】I/O APIC 維護一張**重新導向表（redirection table）**：收各裝置的中斷請求、處理、分派給 LAPIC。「它本質上就是一個**中斷路由器**。」

### 1.7 LAPIC：每顆核心的中斷控制器

`[11:00]` LAPIC：

- 從 I/O APIC 收中斷，做一些處理：轉成**內部中斷封包**，產生一個**中斷向量**。
- CPU 想收這種中斷時，把向量送給 CPU，CPU 做剩下的中斷處理、呼叫 OS。

`[11:31]` 很多 LAPIC 有一個**小 timer**：

- 不是很精確，**不到奈秒等級**，但至少有**毫秒等級**，很多用途夠了。
- `[12:00]` 現代 kernel **不想很規律地**產生 timer 中斷，走向 **tickless（無週期 tick）**：

> 「你可以告訴 timer：『我要跑某個 task **接下來 27 毫秒**，你就在 27 毫秒後產生**一次**中斷，不要週期性地產生。』現代硬體也有這個選項。」

（第 3 講：這台電腦設定了 `CONFIG_NO_HZ_IDLE=y`。課本說這叫 **one-shot** 中斷，「像鬧鐘」。）

### 1.8 處理器間中斷（IPI）

`[12:31]` 情境：

- 兩顆 CPU：**CPU 1** 和 **CPU 5**。
- CPU 5 上跑著一個 **kernel thread**，它想讓 OS 在 **CPU 1** 上跑（想在 CPU 1 上跑一個 kernel thread），但 CPU 1 現在跑的是使用者 thread。

`[13:00]` 問題：

> 「CPU 5 **沒有直接的方法**影響 CPU 1。就算 OS（某個 kernel thread）在 CPU 5 上跑，它就繼續在 CPU 5 上跑，**它對 CPU 1 沒有管轄權**。這就需要**硬體支援**。」

`[13:30]` 做法：**送一個中斷給 CPU 1**——一種特別的中斷，叫 **處理器間中斷（IPI，inter-processor interrupt）**：

```
  CPU 5（kernel thread）                    CPU 1（使用者 thread 在跑）
        │                                         │
        │ 叫自己的 LAPIC 送 IPI                    │
        ▼                                         │
     LAPIC 5 ──────────── IPI ──────────────►  LAPIC 1
                                                  │ 中斷 CPU 1
                                                  ▼
                                         使用者 process 被搶占，
                                         kernel thread 開始跑
```

`[14:30]` 「在多 CPU 系統裡，一顆 CPU（或在上面跑的 kernel thread）要**控制另一顆 CPU 上發生的事**，**一定要送中斷**——要用外部機制，**有硬體支援**。除了這個機制，沒有直接的方法在 CPU 1 上跑任何 kernel thread。」

（第 3 講看過 `native_smp_send_reschedule()`：`apic->send_IPI(cpu, RESCHEDULE_VECTOR)`。）

【課本補充】課本補充（p.124）：CPU 1 被打斷後跑 IPI 的 interrupt handler，做完必要的記帳步驟，kernel thread 就會發現「它被叫進來是因為 CPU 5 上的 kernel thread 想把 CPU 1 上的 task 換掉」。「用這種方式，**一個 kernel thread 可以對所有 CPU 行使控制權**。」timer 晶片通常也放在 LAPIC 裡，中斷頻率可以依 kernel 需要設定、甚至動態改變。

---

## 2. 中斷要送給哪顆核心

`[14:30]` 三種中斷：

| 種類 | 說明 |
|---|---|
| **I/O 中斷** | 裝置產生 |
| **Timer 中斷** | |
| **IPI** | |

（教授刻意不包括 system call 的中斷，「因為 system call 現在有不同的機制」。）

`[15:00]` 中斷到了 I/O APIC，**要送給哪顆核心**？

**靜態分配（static）** `[15:00]`：

| 做法 | 說明 |
|---|---|
| 專用一顆核心 | 某種中斷固定給某一顆 |
| 專用一組核心 | 3、4 顆核心，從裡面挑一顆：**隨機**或**動態**（見下） |
| **廣播（broadcast）** | `[16:31]` 有時有道理：例如只有一個晶片外的 timer，想讓**每顆 CPU** 都處理 timer 中斷。但一般來說會**太多中斷** |

**動態分配（dynamic）** `[16:31]`：根據**執行時的狀況**決定。

`[17:03]` 做法：

- 每顆核心把**正在跑的 task 的優先權**寫進一個 **TPR（Task Priority Register，task 優先權暫存器）**——x86 的一個 **MSR**（特權暫存器）。
- 核心上跑的 kernel thread 寫入這個值，用它來做**中斷路由的決定**。

`[17:34]` 【口頭】

> 「某種意義上這**打破了抽象**：硬體要知道軟體層級的概念。一般來說這不是對的事——軟體知道硬體，但硬體不知道、也不該知道軟體層級的概念。這是那種**打星號**的例外。」

（第 12 講教授對 TLB 裡放 PID 也有一樣的評論。）

`[18:01]` 好處：中斷可以送給**正在跑最低優先權 task** 的核心，或**閒置**的核心：

- 第一優先：核心閒著 → 送給它。
- 或者它在跑很低優先權的 thread → 送給它。

【課本補充】課本補充（p.124–125）：

- IPI 沒有彈性（要送給誰就是誰）。
- timer 中斷通常每個 LAPIC 產生自己的週期性中斷，打斷自己的核心，但也可以改成 one-shot。
- 動態方案「**非常有效率**，因為讓高優先權的 process **不受打擾**地跑」。有閒置核心的話更好：它們可以處理**所有** I/O 中斷，有時甚至 timer 中斷（如果能重新路由）。

---

## 3. `/proc/interrupts`

### 3.1 課本的例子

`[18:01]` `/proc/interrupts` 這個檔案**每台機器都有**。**proc 檔案系統**是一個特殊的檔案系統，顯示 **kernel 層級或裝置層級**的資訊。

教授（作者）機器上的例子（課本 Table 4.3，2023 年）：

```
 IRQ#   CPU 0   CPU 1   CPU 2   CPU 3   HW IRQ type    Handler
  0:      7       0       0       0     2-edge         timer
  1:      0       0       0       0     1-edge         i8042
  8:      0       0       0       0     8-edge         rtc0
  9:      0       4       0       0     9-fasteoi      acpi
 12:      0       0       0       0     12-edge        i8042
 16:      0       0     252       0     16-fasteoi     ehci hcd:usb1
 23:      0       0       0      33     23-fasteoi     ehci hci:usb2
```

| 欄 | 意思 |
|---|---|
| 第一欄 | **IRQ 號碼**。第一個對應 **timer** |
| CPU 0–3 | 這個 IRQ 被送到**每顆 CPU** 處理了幾次 |
| HW IRQ type | **`edge`**（邊緣觸發）或 **`fasteoi`**（準位觸發），前面的數字是 IRQ 線 |
| Handler | 處理它的 handler 名字 |

`[19:01]` **timer 只有 7 次？**

> 【口頭】「7 次 timer 中斷不多——如果一毫秒一次，只有 7 毫秒的執行時間。答案是：現代晶片有**很多 timer**——低解析度、高解析度的。我在 IRQ 0 看的這個 timer，其實是跟 **I/O APIC** 相關的那個。它產生了 7 次被處理的中斷。但當然還有**其他 timer**，產生了別的中斷，沒顯示在這裡。」

`[19:31]` **重新對應（remapping）**：

- timer 那行寫 **`2-edge`**——IRQ **0** 被對應成 IRQ **2**。
- 「任何 APIC 都可以這樣做。I/O APIC 可以說：『好，我在 IRQ 0 收到中斷，但我告訴所有下游的 LAPIC 我從 IRQ 2 收到的。』」
- 目的：讓系統更有效率，**合併不同來源**的資訊。
- `[20:01]` 其他 IRQ 沒有這樣（1 → 1、8 → 8、9 → 9），但 timer 有，因為**有很多 timer**，想把所有 timer 相關資訊**集中在一處**。

`[21:43]` 最後一欄指向 handler：第一個是 timer handler，然後是各種裝置的 handler，最後兩個是 **USB** 裝置。

`[22:01]` 「proc 檔案系統的好處：它提供很多關於 process、硬體、kernel process 等**動態執行狀態**的資訊。」例如 `/proc/meminfo`（記憶體模組）、`/proc/cpuinfo`（CPU）。

【課本補充】課本說：資料是**低解析度的 LAPIC timer** 的；實際上用的是**更高解析度**的 timer。現代 kernel 偏好高解析度 timer，因為可以依正在執行的 process **動態設定**中斷間隔。很多現代 kernel 也是 **tickless** 的。

### 3.2 【實測】這台電腦的 `/proc/interrupts`

```bash
$ cat /proc/interrupts      # 節錄；這台有 20 顆 CPU，只顯示前 4 欄和最後幾欄
            CPU0       CPU1       CPU2       CPU3   ...
  1:           0          0          0          0   ... IR-IO-APIC    1-edge      i8042
  8:           0          0          0          0   ... IR-IO-APIC    8-edge      rtc0
  9:           0      21802          0          0   ... IR-IO-APIC    9-fasteoi   acpi
 12:           0          0          0          0   ... IR-IO-APIC   12-edge      i8042
 ...
124:           0          0          0          ... IR-PCI-MSIX-0000:00:0e.0    0-edge      vmd0
126:           0          0        390          ... IR-PCI-MSIX-0000:00:0e.0    2-edge      vmd0
 ...
NMI:          27         30         27         27   ... Non-maskable interrupts
LOC:      928481     607573     962954     266494   ... Local timer interrupts
RES:       20981       5759      17481       5416   ... Rescheduling interrupts
CAL:      148717      87978     108643      74167   ... Function call interrupts
TLB:       83132      54706      75125      51712   ... TLB shootdowns
```

| 觀察 | 對照 |
|---|---|
| `i8042`（IRQ 1、12）、`rtc0`（IRQ 8）、`acpi`（IRQ 9） | **跟課本的表一模一樣**：IRQ 1 / 12 是傳統的鍵盤 / 滑鼠控制器（第 6 講的 port `0x60`、`0x64`），IRQ 8 是即時時鐘 |
| `9-fasteoi acpi` | **準位觸發**（§1.5） |
| 前綴 **`IR-IO-APIC`** | 經過 **I/O APIC**；**IR = Interrupt Remapping**——§7 的「中斷重新對應 domain」 |
| `acpi` 的 21802 次**全在 CPU1** | **靜態分配**：這個 IRQ 被固定送給 CPU1 |
| **`IR-PCI-MSIX-...`** | **MSI-X**：§1.2 說的**訊息式中斷**，沒有實體 IRQ 線。這台有 **84 行** MSI、只有 **10 行** IO-APIC——**現代裝置大多用 MSI** |
| **`LOC`**：Local timer interrupts，每顆 CPU 幾十萬次 | 每顆核心**自己 LAPIC 的 timer**（§1.7）——教授說「還有其他 timer」就是這個 |
| **`RES`**：Rescheduling interrupts | **IPI**：第 3 講的 `RESCHEDULE_VECTOR`，叫別的核心重新排程 |
| **`CAL`**：Function call interrupts | **IPI**：叫別的核心執行某個函式 |
| **`TLB`**：TLB shootdowns | **IPI**：一個核心改了 page table，要叫其他核心**清掉它們 TLB 裡的舊翻譯**（第 6 講踢 frame 時「從所有可能有這筆映射的 TLB 移除」） |
| **`NMI`** | 不可遮罩中斷 |

這台沒有 IRQ 0 那一行——現代系統用 LAPIC timer 和高解析度 timer，傳統的 IRQ 0 timer 可能根本沒啟用。

---

## 4. IRQ 共用與中斷向量的範圍

### 4.1 一條 IRQ 線、很多裝置

`[22:30]` 「管理 IRQ 比我們想的**複雜得多**。」

`[22:30]` IRQ 是一條**實體線**（或虛擬線），有狀態、會在 0 和 1 之間轉換。問題：

- IRQ 線**有限**，裝置**很多**。
- 所以要**共用**：一條 IRQ 線接好幾個裝置。

```
              IRQ 線
  ──────────────┬──────────┬──────────┬────────► I/O APIC → … → CPU
                │          │          │
             裝置 A      裝置 B      裝置 C
              （任何一個都可以把線從 0 拉到 1）
```

`[24:01]` 問題：CPU 知道「某條 IRQ 線有中斷」，但**不知道是哪個裝置**。

**方法一**（看起來可行、實務上不行）`[24:30]`：

- 不事先建立裝置和 IRQ 的對應，**用到裝置時才建立**。
- 運氣好、一條 IRQ 只接一個裝置 → 那條 IRQ 響了就知道是它。
- 但**不只一個裝置**接在同一條 IRQ 上時，就難了。

**方法二** `[25:31]`：**一個一個問**。

```
  IRQ 響了
     │
     ▼
  問裝置 A：「是你嗎？」── 不是
     │
     ▼
  問裝置 B：「是你嗎？」── 是！→ 停
```

- 把這些裝置做成一個 **linked list**，走訪串列，問每個裝置有沒有發中斷。
- 沒有就往下，直到第一個說「是我」的。
- 「這解決了問題：IRQ 線少一點、裝置多一點也沒關係。」

【課本補充】課本補充（p.126–127）：可以盡量**動態**管理 IRQ（裝置沒在用時釋放 IRQ、第一次存取時才配置），但還是可能不夠。kernel 的中斷子系統要**跑所有共用這條 IRQ 的裝置的 handler**，這些 handler 去**查詢個別裝置**或**檢查中斷資料**，找出是誰發的。「這無可避免地會**拖慢系統**，但是必要的。」

### 4.2 IRQ 與向量的對應

`[26:31]` 流程：裝置發 IRQ → 進 APIC 子系統（I/O APIC、LAPIC）→ 產生中斷向量 → 送給 CPU → CPU 依中斷向量呼叫 handler。如果那個向量對應的 IRQ 只接一個裝置，沒問題；不然就要掃描找。

`[27:00]` 教授機器上的對應（課本 Table 4.5）——「**依機器而異**」：

| IRQ | 中斷向量 | 硬體裝置 |
|---|---|---|
| 0 | **32** | Timer |
| 1 | 33 | 鍵盤 |
| 8 | 40 | 系統時鐘 |
| 10 | 42 | 網路介面 |
| 11 | 43 | USB port |

`[27:31]` 「這個對應**非常重要**，OS 一定要知道：給中斷向量要找出 IRQ，從 IRQ 要找出裝置，才知道要服務哪個裝置。」

【課本補充】課本說分開 IRQ 和中斷向量的好處，在 **timer** 最明顯：可以有各種解析度的 timer，但都對應到**同一個中斷向量**。任何一個來的中斷都會呼叫 timer 的 interrupt handler。kernel 可以依需求和負載**動態**決定用哪個 timer。

### 4.3 中斷向量的範圍

`[28:01]` 中斷向量是 **8 位元**——「聽起來很舊，但這是**還在維持的歷史包袱**」。（課本 Table 4.4，作者 2023 年 Intel i7 機器）

| 向量範圍 | 意義 |
|---|---|
| **0–19** | **不可遮罩中斷（NMI）和 exception**。exception 是程式產生的錯誤情況；NMI **不能被遮罩，一定要服務** |
| 20–31 | Intel **保留**，目前沒用 |
| **32–127** | **I/O 裝置**（外部中斷） |
| **128**（`0x80`） | **system call**——「但如我們說過，這個方法**現在不用了**，用 `syscall` 指令直接進 system call 入口」 |
| **239** | **LOCAL APIC timer** 中斷 |
| **251–253** | **IPI**（3 個向量） |

【課本補充】課本的免責聲明：「這是作者 2023 年 Intel i7 系統上的範圍。很可能將來會變，或其他系統不一樣。請把這些資料當作**例子**。」

### 4.4 【原始碼】v6.2.12 的向量表

kernel 的 x86 向量定義，開頭的註解：

```c
// arch/x86/include/asm/irq_vectors.h:14-24
 * Linux sets the kernel code address each entry jumps to early during
 * bootup, and never changes them. This is the general layout of the
 * IDT entries:
 *
 *  Vectors   0 ...  31 : system traps and exceptions - hardcoded events
 *  Vectors  32 ... 127 : device interrupts
 *  Vector  128         : legacy int80 syscall interface
 *  Vectors 129 ... LOCAL_TIMER_VECTOR-1
 *  Vectors LOCAL_TIMER_VECTOR ... 255 : special interrupts
 *
 * 64-bit x86 has per CPU IDT tables, 32-bit has one shared IDT table.
```

各個特殊向量：

```c
// arch/x86/include/asm/irq_vectors.h（節錄，行號如註）
#define NMI_VECTOR			0x02    //  30
#define FIRST_EXTERNAL_VECTOR		0x20    //  36
#define SPURIOUS_APIC_VECTOR		0xff    //  61
#define ERROR_APIC_VECTOR		0xfe    //  69
#define RESCHEDULE_VECTOR		0xfd    //  70
#define CALL_FUNCTION_VECTOR		0xfc    //  71
#define CALL_FUNCTION_SINGLE_VECTOR	0xfb    //  72
#define THERMAL_APIC_VECTOR		0xfa    //  73
...
#define LOCAL_TIMER_VECTOR		0xec    // 107
#define NR_VECTORS			 256    // 109
```

跟課本的表對照：

| 課本 | v6.2.12 | 說明 |
|---|---|---|
| 0–19：NMI 與 exception；20–31：Intel 保留 | **0–31**：「系統 trap 和 exception，**寫死的事件**」 | `NMI_VECTOR` = **2**（NMI 其實只佔 2 號，其他是 exception） |
| 32–127：外部中斷 | **32–127**：裝置中斷 | `FIRST_EXTERNAL_VECTOR` = `0x20` = **32** |
| 128：system call | **128**：「**legacy**（舊式）int80 system call 介面」 | 第 2 講的 `IA32_SYSCALL_VECTOR` |
| 239：LAPIC timer | **236**（`0xec`） | ⚠️ **不一樣**。課本說「可能因系統不同」，v6.2.12 是 236 |
| 251–253：IPI | **251–253**（`0xfb`–`0xfd`）：`CALL_FUNCTION_SINGLE`、`CALL_FUNCTION`、**`RESCHEDULE`** | **一樣**。§3.2 實測的 `CAL` 和 `RES` 就是它們 |
| — | 254：`ERROR_APIC`、255：`SPURIOUS_APIC`（假的中斷） | 課本沒列 |
| — | `NR_VECTORS` = **256** | 8 位元 |

第 24 行：「**64 位元 x86 每顆 CPU 有自己的 IDT 表**」——§9.1 的 `vector_irq` 也是 per-CPU 的。

---

## 5. CPU 內部：查 IDT

`[29:00]` 前面看了中斷在**晶片外**怎麼處理，現在看**晶片內**：

1. **LAPIC** 決定中斷對應的**向量**：一個 0–255 的 8 位元數字，用來存取 **IDT** 對應的那一格。IDT 由特權暫存器 **IDTR** 指向。
2. `[30:02]` 每一格指向一個 **segment 描述子**（或 segment）和 **segment 裡的位址**——一般的分段定址，一個複合的記憶體位址（第 6 講）。
3. **檢查特權**：
   - 正在跑使用者 process → 要**切換模式**、存 context，再跑 interrupt handler。
   - `[30:30]` **已經在跑 interrupt handler** → 不用提升特權，要做適當的 context switch：把目前中斷的狀態存在**它專屬的中斷 stack** 上，處理新中斷（假設新的優先權更高）。
4. **返回位址**（第 2 講）：
   - `[31:01]` 無關的 I/O 中斷 → 做完回 **next PC**（PC + 4，或分支目標）。
   - `[31:32]` **page fault** → 回到**同一個 PC**：「第一次記憶體存取沒成功，修好狀態後重新嘗試同一條指令。」
5. `[32:30]` 存好狀態後，載入 interrupt handler。

`[32:30]` 「上一章討論過：中斷是**沒計畫的事件**，不能把一部分狀態存在 CPU 暫存器裡，**要存在 stack 上**。」（第 13 講）

【課本補充】課本補充（p.128）：硬體要把返回位址（適當算好的）和 flags 暫存器存到 stack 上；x86-64 還會把 **code segment 暫存器**推到 stack。這部分要在開始 interrupt handler **之前由硬體自動**完成。

---

## 6. Kernel 的中斷資料結構

### 6.1 `struct irq_desc`

`[33:04]` **`struct irq_desc`**（IRQ description，IRQ 描述）是 IRQ 的「**節點結構（nodal structure）**」：

| 欄位 | 內容 |
|---|---|
| **`irq_common_data`** | 一般來說**哪顆（或哪組）CPU** 該處理這個 IRQ（CPU affinity） |
| **`irq_data`** | 大部分的資料：**中斷向量、IRQ 號碼**等。IRQ 號碼和中斷向量的**對應**存在這裡 |
| **`handle_irq`** | 指向 **flow handler**（§6.2） |
| **`action`** | `struct irqaction *`：裝置 handler 的**串列**（§6.3） |

`[33:31]` 「IRQ 也像 process 的 PID 組成 namespace 一樣，組成 **domain**。」（§7）

【課本補充】課本補充（p.129）：`irq_desc` 是所有 IRQ 相關資料的節點結構：硬體裝置、中斷向量、CPU affinity、指向 flow handler 的指標、特殊旗標等。`irq_common_data` 還存**訊息信號中斷（MSI）**的特殊資訊。

### 6.2 Flow handler

`[33:31]` **`irq_flow_handler_t`**：一種**函式指標**的型別。

`[34:00]` 「它**不是** handler 本身」，而是一個 **interrupt flow handler**，負責**協調中斷處理的初步階段**：

| 工作 | 說明 |
|---|---|
| **Acknowledge** | 確認收到 |
| **Mask / unmask** | `[34:31]` 特別是**準位觸發**的中斷，確認的方式是把線從 1 拉到 0，有時就等於 unmask |
| **呼叫所有掛在這條 IRQ 線上的 handler** | `[35:02]` **一個接一個**呼叫 |
| **向上層回報** | `[35:38]` 收到中斷了，以及中斷的細節——有些中斷不是靠拉線，而是送**訊息** |

> 【口頭】「它基本上像**高階的管理者**。」

### 6.3 `struct irqaction`：裝置的 handler 串列

`[36:00]` **`struct irqaction`** 比較有趣：

- 基本上是一個 **linked list**。
- 「**不是**經典的 Linux linked list（`list_head`），是比較**簡單**的串列。」
- 每個元素有一個指標，指向一個**特定裝置的 handler**。

`[37:03]`（課本 Fig 4.6）

```
   struct irq_desc
  ┌──────────────────┐
  │ irq_common_data  │
  │ irq_data         │
  │ handle_irq ──────┼──► flow handler（例如 handle_level_irq）
  │ action ──────────┼──┐
  └──────────────────┘  │
                        ▼
                ┌──────────────┐     ┌──────────────┐     ┌──────────────┐
                │ irqaction    │     │ irqaction    │     │ irqaction    │
                │  handler ────┼─►A  │  handler ────┼─►B  │  handler ────┼─►C   ← 各裝置 driver 的函式
                │  dev_id      │     │  dev_id      │     │  dev_id      │
                │  next ───────┼────►│  next ───────┼────►│  next = NULL │
                └──────────────┘     └──────────────┘     └──────────────┘
```

`[37:31]` 兩個最重要的欄位：

| 欄位 | 意思 |
|---|---|
| **`next`** | 指向串列的下一個元素，最後一個指向 NULL |
| **`handler`** | **真正的 interrupt handler**——由**裝置 driver** 實作 |

【課本補充】課本補充（p.130）：`irqaction` 還有 **`dev_id`**（關聯的裝置），以及**選用**的 **`thread`**（`task_struct *`）和 **`thread_fn`**（函式指標）：有時需要開一個新 thread 做額外的中斷處理工作，用**較低優先權**晚一點完成。

### 6.4 三種回傳值

`[38:06]` 裝置 handler 會回傳一個**狀態**，三種：

| 回傳值 | 意思 | 之後 |
|---|---|---|
| **`IRQ_NONE`** | 「我**沒興趣**」——不是我發的 | 繼續掃：去串列的**下一個** `irqaction` |
| **`IRQ_HANDLED`** | 中斷**成功處理**了 | `[38:32]` handler 被呼叫時，如果真的是它發的中斷，它**不只**回一個確認，**也會處理**掉，然後才回確認 |
| **`IRQ_WAKE_THREAD`** | 做了一些基本處理，**剩下的**交給另一個 thread | `[39:01]` **當場**開一個新的 kernel thread 做剩下的工作（第 16 講的 threaded IRQ） |

### 6.5 【原始碼】`irq_desc`、`irqaction`、`irqreturn`

```c
// include/linux/irqdesc.h:55-67（節錄）
struct irq_desc {
	struct irq_common_data	irq_common_data;
	struct irq_data		irq_data;
	unsigned int __percpu	*kstat_irqs;
	irq_flow_handler_t	handle_irq;
	struct irqaction	*action;	/* IRQ action list */
	unsigned int		status_use_accessors;
	unsigned int		core_internal_state__do_not_mess_with_it;
	unsigned int		depth;		/* nested irq disables */
	...
	unsigned int		irq_count;	/* For detecting broken IRQs */
	unsigned long		last_unhandled;	/* Aging timer for unhandled count */
```

| 欄位 | 對照 |
|---|---|
| `irq_common_data`、`irq_data`、`handle_irq`、`action` | 課本 Listing 4.2 的四個欄位 |
| **`kstat_irqs`** | **per-CPU** 的計數器：每顆 CPU 處理了幾次——**就是 `/proc/interrupts` 每一欄的數字**（§3.2） |
| `depth` | 註解：「**巢狀**的 IRQ disable」——disable 兩次就要 enable 兩次才會真的打開 |
| `core_internal_state__do_not_mess_with_it` | 名字就是註解：「**別亂碰**」——kernel 開發者的幽默 |
| `irq_count`、`last_unhandled` | 註解：「**偵測壞掉的 IRQ**」——如果一條 IRQ 一直響、但每個 handler 都回 `IRQ_NONE`（沒人認領），kernel 會發現並停用它，避免整台機器被假中斷淹沒 |

```c
// include/linux/interrupt.h:118-132
struct irqaction {
	irq_handler_t		handler;
	void			*dev_id;
	void __percpu		*percpu_dev_id;
	struct irqaction	*next;
	irq_handler_t		thread_fn;
	struct task_struct	*thread;
	struct irqaction	*secondary;
	unsigned int		irq;
	unsigned int		flags;
	unsigned long		thread_flags;
	unsigned long		thread_mask;
	const char		*name;
	struct proc_dir_entry	*dir;
} ____cacheline_internodealigned_in_smp;
```

| 欄位 | 對照 |
|---|---|
| **`handler`** | **driver 的 handler**（函式指標） |
| **`dev_id`** | 是**哪個裝置**——呼叫 handler 時會傳進去，driver 用它區分自己管的多個裝置 |
| **`next`** | **單向串列**的下一個——教授說的「不是 `list_head`，比較簡單的串列」 |
| **`thread_fn`**、**`thread`** | 回 `IRQ_WAKE_THREAD` 時要喚醒的 thread 和它要跑的函式 |
| `name` | 顯示在 `/proc/interrupts` 最後一欄的名字（例如 `i8042`、`acpi`） |
| `____cacheline_internodealigned_in_smp` | 又是快取線對齊（第 8 講） |

```c
// include/linux/irqreturn.h:11-17
enum irqreturn {
	IRQ_NONE		= (0 << 0),
	IRQ_HANDLED		= (1 << 0),
	IRQ_WAKE_THREAD		= (1 << 1),
};

typedef enum irqreturn irqreturn_t;
```

三個回傳值是**位元旗標**（第 7 講的模式）：`IRQ_NONE` = 0、`IRQ_HANDLED` = 1、`IRQ_WAKE_THREAD` = 2。用位元的好處：§9.3 會看到可以把多個 handler 的回傳值 **OR 起來**。

---

## 7. IRQ domain

`[39:01]` 裝置很多、IRQ 很多，**裝置數量甚至可以超過 IRQ 數量**（所以要共用 IRQ）。還是會有**很多** IRQ，也可能有不只一個 I/O APIC。所以要用 **IRQ domain** 管理。

`[39:31]` 概念上：**每個中斷控制器一個 domain**——不是硬性規定：

- 有**虛擬化**的系統，一個中斷控制器裡可以切成好幾個 domain。
- 多個中斷控制器也可以屬於同一個 domain。

`[40:00]` 「為了理解，把 IRQ domain 想成像 **process namespace**：」

| Process namespace（第 10 講） | IRQ domain |
|---|---|
| PID 號碼在 namespace 裡唯一 | **IRQ 號碼在 domain 裡唯一** |
| PID 是**虛擬**的，只在 namespace 裡有意義 | IRQ 號碼也是虛擬的，只在 domain 裡有意義 |
| `struct pid`：跨 namespace 都一樣 | **`struct irq_desc`**：跨 domain 都一樣 |
| 每個 namespace 有一棵 **IDR tree**：PID 號碼 → `struct pid` | 每個 domain 有一個**對應結構**：IRQ 號碼 → `irq_desc` |
| 先把 process 加進 namespace | `irq_domain_add` 註冊 domain，之後再加中斷控制器 |

`[40:31]` 「是**一樣的軟體工程**。」

`[42:00]` 對應結構可以簡單一點：IRQ **不是動態增加**的，所以不需要 IDR tree 那麼複雜：

| 硬體 IRQ 數量 | 用什麼 |
|---|---|
| **少於 256**（通常如此） | **簡單的線性串列**——夠快 |
| **256 以上** | **radix tree** |

（課本：「兩全其美：IRQ 少時避開 radix tree 的開銷，用 linked list 的簡單。」）

`[42:31]` **domain 的階層**——順著**中斷的路徑**：

```
              （根：概念上的 CPU 層，拿到的是中斷向量）
                          ▲
                    LAPIC domain              ← 最後產生中斷向量送給 CPU
                          ▲
             中斷重新對應（interrupt remapping）domain   ← 重新對應中斷
                          ▲
                    I/O APIC domain           ← 最底層：中斷「入境」的第一站
                          ▲
                        IRQ
```

| 層 | 做什麼 |
|---|---|
| **I/O APIC domain**（最底層） | 中斷**第一個入境點**。可以**忽略、丟棄、合併、融合、重新對應**中斷，宣稱是別的 IRQ 產生的——「APIC 能做很多事」 |
| **中斷重新對應 domain** | 如名字所說：**重新對應**。例如好幾個滑鼠，你不在乎點的是哪個，對應到同一個來源 |
| **LAPIC domain** | 最後產生**中斷向量**送給 CPU |
| 根 | 概念上的，「不存在」，可以想成 CPU 層 |

【課本補充】課本指出 IRQ domain 和 namespace 的**差別**（p.131）：namespace 的目的是讓子 process 成為父 namespace 的成員，能存取父 namespace 擁有的資源；IRQ domain 則是**中斷從子流向父**，每一層都有一些**虛擬化和重新對應**。例如中間某個 domain 可以把**所有鍵盤中斷只送給一台虛擬機**，因為其他虛擬機不准接受鍵盤輸入——「這種政策可以用 IRQ domain 實施」。

（§3.2 實測的 **`IR-IO-APIC`**、**`IR-PCI-MSIX`**：**IR** 就是 Interrupt Remapping——中斷經過了重新對應那一層。）

---

## 8. 開機時設定 IDT 和 APIC

### 8.1 BIOS 先設一個陽春版

`[44:32]` IDT 有 **256 格**（中斷向量是 8 位元）。每一格基本上是指向 handler 的指標（還有一些後設資訊）。

`[45:05]` IDT **一開始由 BIOS 設定**：

> 【口頭】「BIOS 是 **Basic Input Output System**，它也像一個**奈米 OS（nano OS）**一樣在跑。很多人看過：OS 開機前，螢幕上就印了很多東西，可以進 BIOS、螢幕能用，可以選要開哪個 OS、從哪個裝置開機。BIOS 在處理鍵盤、滑鼠等中斷，所以需要一個**半初始化**的 IDT——至少它感興趣的那些中斷——**陽春版**。至少鍵盤要能用。」

`[46:00]` kernel 開機後**重新初始化**：現在是整個系統，每個接上的、會產生中斷的裝置都要。幾乎每一格都重新初始化、加上新的值。

`[46:30]` IDT 跟 page table 一樣：

> 「**軟體填、硬體查**。硬體知道這張表在哪，有固定的格式。所以**很快**，而且你**不需要中斷**來存取中斷表——這樣才合理。」

（課本：IDTR 暫存器存 IDT 的基底位址，「就像 **CR3** 存 page table 的位址」。）

`[47:31]` IDT 初始化好，是整個開機過程**非常重要的一部分**：「至少你可以說**系統收得到中斷了**。對很多人來說，這是開機過程中**很重要的里程碑**，因為系統**有反應了**。」

【課本補充】課本補充（p.132）：BIOS 需要跑一個 **nanokernel** 處理開機時的事件，例如使用者輸入，或電壓、溫度緊急狀況。kernel 一開機就要**覆寫** IDT、加上自己的項目，「只有選中的 kernel 能做，因為 BIOS 根本不知道它們——它們是 kernel 專屬的」。而且 interrupt handler 在 kernel 的位址空間裡，**只有 kernel 知道它們在哪**。interrupt handler 一般**不放在可以被搬動或換出去的記憶體區域**：那些 page 是 **locked 且 pinned** 在實體記憶體裡的（第 9 講）。

### 8.2 `start_kernel` 裡的初始化

`[47:31]` kernel 的入口是 **`start_kernel`**（`init/main.c`），裡面有很多函式初始化整個中斷子系統：

| 函式 | 做什麼 |
|---|---|
| **`early_irq_init`** | `[48:01]` **探測裝置**：看主機板上接了什麼，例如 PCI 匯流排上的裝置；為**幾乎每個硬體 IRQ 來源**建一個 `irq_desc` 的陣列 |
| **`init_IRQ`** | `[48:30]` 設好 **per-CPU 的中斷 stack**（第 8 講：每顆 CPU 7 個，不用 kernel stack）；設一個**基本版的 IDT** |
| 之後 | 設定 **LAPIC** 和 **I/O APIC** |

【原始碼】

```c
// init/main.c:1044-1045
	early_irq_init();
	init_IRQ();
```

第 1 講看過 `start_kernel()` 的開頭（第 940 行起）；這兩行在大約 100 行之後。

【課本補充】課本說：`early_irq_init` 的探測**只**為一般預期會有的裝置（例如顯示卡、網路卡）做設定，「這些裝置是現代系統運作必需的」。之後 `apic_bsp_setup` 設好 LAPIC、I/O APIC 和所有接上的裝置。

### 8.3 設定 LAPIC：MSR

`[50:32]` 設 LAPIC 的資料（包括它該怎麼處理中斷），Intel 定義了一組 **MSR**（處理器特定暫存器 / 特權暫存器），用來控制行為和初始化狀態。兩個簡單的指令：**`wrmsr`**（寫）、**`rdmsr`**（讀）。

`[51:02]` LAPIC 裡幾個重要的暫存器，**CPU 開機時可以設定**：

| 暫存器 | 說明 |
|---|---|
| **LDR**（Logical Destination Register，邏輯目的暫存器） | `[51:30]` Intel 處理器可以**分群**成兩層階層。例如 64 顆 CPU，每 16 顆一群。這個 **32 位元**暫存器：**16 位元群組 ID**（第一層）+ **16 位元群組內 ID**。「我們準備了**很多** CPU，實務上永遠不會有那麼多」 |
| **DFR**（Destination Format Register，目的格式暫存器） | `[52:31]` 表示**有沒有**用兩層分群 |
| **TPR**（Task Priority Register） | `[53:30]` §2 講過：寫入目前 task 的優先權，找負載最低的核心送中斷 |

`[52:31]` 【口頭】「分群是 Intel 硬體的**新穎功能**：至少**意圖**是做出包含**幾百甚至幾千顆核心**的大型可擴展系統。所以有階層的概念，也有 IRQ domain 的概念：很大的系統會有很多 IRQ，要適當管理。例如系統**橫跨兩塊主機板**怎麼辦？顯然是不同的 IRQ，不該在同一個 domain——兩邊都可能有兩個 timer 中斷，不能用同一個名字。所以要建 domain。」

`[54:01]` 初始化 LAPIC：

- **timer** 設成 0。
- 很多**效能計數器**（快取命中、失誤次數等各種統計）也設成 0。
- 狀態設成 **active**：LAPIC 開始接收和處理中斷。

**I/O APIC**：

- **每一支輸入中斷腳位**都要設好一個 IRQ。
- 每個 I/O APIC 建一個 **domain**（或註冊到現有的 domain），照 §7 的階層組織起來。

### 8.4 【原始碼】`x86_init.irqs`：架構專屬的函式指標

`[49:32]` 在標準 x86 硬體上（當然要在 `arch` 裡做）：

> 「有一個**裝函式指標的結構**。這就是讓它**跟架構無關**的關鍵：不同架構各自建一個這樣的結構，另一部分的程式碼只要呼叫所有函式指標。」

```c
// arch/x86/kernel/x86_init.c:77-83
	.irqs = {
		.pre_vector_init	= init_ISA_irqs,
		.intr_init		= native_init_IRQ,
		.intr_mode_select	= apic_intr_mode_select,
		.intr_mode_init		= apic_intr_mode_init,
		.create_pci_msi_domain	= native_create_pci_msi_domain,
	},
```

| 左邊（通用名字） | 右邊（x86 專屬函式） | 做什麼 |
|---|---|---|
| `pre_vector_init` | `init_ISA_irqs` | 舊式 ISA 匯流排的 IRQ（IRQ 0–15） |
| `intr_init` | `native_init_IRQ` | 中斷初始化（`init_IRQ` 會呼叫它） |
| `intr_mode_select` | `apic_intr_mode_select` | 選擇中斷模式 |
| **`intr_mode_init`** | **`apic_intr_mode_init`** | **設定 LAPIC 和 I/O APIC**——教授說「在目前的討論裡最重要」 |
| `create_pci_msi_domain` | `native_create_pci_msi_domain` | 建 **PCI MSI** 的 IRQ domain（§1.2 的訊息式中斷） |

`[50:00]` 「右邊這些是 **x86 硬體專屬**的函式，左邊是**通用**的名字。」

這是 kernel 裡「**跨架構抽象化**」的標準手法（第 1 講的 `arch` 目錄、第 13 講的「superset」）：通用程式碼呼叫 `x86_init.irqs.intr_init()`，不知道也不在乎背後是 x86 的 `native_init_IRQ`。

---

## 9. 中斷的路徑

### 9.1 `common_interrupt`

`[54:31]` 再看一次中斷的呼叫路徑。

> 「這個檔案（`entry_64.S`）我們**看過**了——在討論 **system call 的入口**時。Linux 對 system call 和中斷有一個**共同的入口**，目的是**存 context**。為中斷、exception、system call 各做一套存 context 的方法**沒什麼道理**，因為大部分程式碼一樣。只有小差異：system call 時 `rcx`、`r11` 用來存指令指標和 flags，因為 system call 是計畫好的；中斷是沒計畫的，所以用 stack。」

`[55:30]` 中斷來時：

1. **中斷向量**被硬體推到 stack 上，方便之後取回。
2. 基本的 context 存好後，跳到 **IDT 的入口**。
3. `[56:02]` 有一個標準的巨集定義 IDT 入口（`DEFINE_IDTENTRY_IRQ`）。

`[56:31]` 函式的核心部分：

- 一個 `struct irq_desc *`。
- `[57:00]` 用 **`this_cpu_read`** 巨集——「這我們也看過：讀 `current` task 時（第 8 講），存在 **`gs`** 指向的記憶體區域」。
- 「同樣的想法：有一張表放在 **per-CPU 區域**，所以**沒有跨核心或跨 thread 的競爭**。」
- 把表當**陣列**存取，**索引是中斷向量**，拿到的是 `struct irq_desc *`。

`[57:31]` 「有人會說：硬體不知道什麼是 `irq_desc`，怎麼運作？」

> 「有不同種類的架構。有些架構**跳到 handler 是硬體自動做的**，那就一定要是函式指標。但在我展示的這個實作裡，**很多事是軟體做的**：存一個指向 `irq_desc` 的指標，它有很多額外的東西。需要它的理由很明顯：**一個 IRQ 不是只對應一個指標**——那是簡單系統的情況，硬體查一下中斷表就夠了，像硬體走 page table 一樣。但這裡不是：中斷要 **acknowledge**、要存一些狀態、有 mask 和 unmask，最複雜的是要**走 `irqaction` 串列**找出發中斷的裝置。這很複雜，所以存一個 `irq_desc *` 比較合理。」

`[59:02]` 拿到指標（「不太可能是錯的或 NULL」）後，呼叫 **`handle_irq`**——各種中斷的**總括函式**（準位觸發、邊緣觸發都是）。

- 它收 `irq_desc` 和**所有 CPU 暫存器**（`regs`）。
- 為什麼要暫存器？「很多中斷，例如 **page fault**、**除錯中斷**，也需要暫存器狀態。」

【課本補充】`regs` 陣列存所有 CPU 暫存器的值，是存被打斷的 process 的 context 時填好的（第 2 講的 `pt_regs`）。

### 9.2 走 `irqaction` 串列

`[60:00]` IRQ handler 長什麼樣？

- `handle_irq` 依中斷類型呼叫不同函式：**`handle_level_irq`**（準位）、**`handle_edge_irq`**（邊緣）……
- 不管走哪條路，最後都會呼叫內部函式 **`__handle_irq_event_percpu`**。

`[60:32]` 為什麼是 **per-CPU**？

> 「我們**非常認真**看待效率，想要 per-CPU 的函式和資料結構，**不共享**。一共享，競爭、data race、false sharing 失誤全都來了。而中斷表**不常更新**（如果有的話），所以每顆 CPU 一份副本合理。」

`[61:00]` 邏輯：

1. 在**軟體**裡走 `irqaction` 串列。
2. 呼叫每個 handler，直到中斷被處理。
3. `[61:31]` 每個 handler 回傳 `IRQ_NONE`、`IRQ_HANDLED` 或 `IRQ_WAKE_THREAD`（§6.4）。
4. `[62:00]` 回 `IRQ_WAKE_THREAD`：喚醒一個 handler thread，它很可能是**較低優先權**的 kernel thread，處理需要很多工作的中斷。

`[62:00]` **這些個別的 handler 函式是 device driver 實作的**：

> 「去看 `drivers` 目錄，有各種東西的 device driver。它們**都要實作一個函式**，有中斷時 kernel 會呼叫它。裝置把自己**掛到**一條 IRQ 上。裝置來中斷時 IRQ 設成 1，kernel 最後透過它的 `irqaction` 呼叫這個裝置的 handler——這個 `irqaction` 一定是**事先**註冊給 kernel 的。`irq_handler_t` 是一個函式指標，函式**實體上在 driver 的程式碼裡**，被呼叫時 driver 的函式知道該做什麼。」

【課本補充】課本補充（p.137）：裝置以**隨插即用（plug-and-play）**方式接上，或開機時，kernel 找到它的 device driver。driver 向 kernel **註冊一組函式**，其中一個就是 interrupt handler，被包在一個 `irqaction` 結構裡，加進那個中斷向量對應的串列。

### 9.3 【原始碼】`common_interrupt` 與 `__handle_irq_event_percpu`

```c
// arch/x86/kernel/irq.c:240-264
DEFINE_IDTENTRY_IRQ(common_interrupt)
{
	struct pt_regs *old_regs = set_irq_regs(regs);
	struct irq_desc *desc;

	/* entry code tells RCU that we're not quiescent.  Check it. */
	RCU_LOCKDEP_WARN(!rcu_is_watching(), "IRQ failed to wake up RCU");

	desc = __this_cpu_read(vector_irq[vector]);
	if (likely(!IS_ERR_OR_NULL(desc))) {
		handle_irq(desc, regs);
	} else {
		ack_APIC_irq();

		if (desc == VECTOR_UNUSED) {
			pr_emerg_ratelimited("%s: %d.%u No irq handler for vector\n",
					     __func__, smp_processor_id(),
					     vector);
		} else {
			__this_cpu_write(vector_irq[vector], VECTOR_UNUSED);
		}
	}

	set_irq_regs(old_regs);
}
```

| 行 | 意思 | 對照 |
|---|---|---|
| 240 | `DEFINE_IDTENTRY_IRQ(common_interrupt)`：用巨集定義 IDT 入口 | 「標準巨集」 |
| 242 | 把 `regs`（被打斷的暫存器）記下來 | 給需要暫存器的 handler 用 |
| **248** | **`desc = __this_cpu_read(vector_irq[vector]);`** | **per-CPU 的陣列**，**用中斷向量當索引**，拿到 `irq_desc *`——課本說「`vector_irq` 陣列**就是** IDT 表」（精確說是 kernel 在 IDT 之後的軟體查表） |
| 249 | **`likely`**：通常不會是 NULL | 第 13 講 |
| **250** | **`handle_irq(desc, regs);`** | 總括函式 |
| 252 | 沒有 handler：至少要 **ack APIC**，不然 APIC 會一直等確認 | |
| 254–257 | 印「這個向量沒有 IRQ handler」 | |

`handle_irq` → `desc->handle_irq`（flow handler，例如 `handle_level_irq`）→ ... → 最後：

```c
// kernel/irq/handle.c:139-184（節錄）
irqreturn_t __handle_irq_event_percpu(struct irq_desc *desc)
{
	irqreturn_t retval = IRQ_NONE;
	unsigned int irq = desc->irq_data.irq;
	struct irqaction *action;

	record_irq_time(desc);

	for_each_action_of_desc(desc, action) {
		irqreturn_t res;
		...
		trace_irq_handler_entry(irq, action);
		res = action->handler(irq, action->dev_id);
		trace_irq_handler_exit(irq, action, res);

		if (WARN_ONCE(!irqs_disabled(),"irq %u handler %pS enabled interrupts\n",
			      irq, action->handler))
			local_irq_disable();

		switch (res) {
		case IRQ_WAKE_THREAD:
			/*
			 * Catch drivers which return WAKE_THREAD but
			 * did not set up a thread function
			 */
			if (unlikely(!action->thread_fn)) {
				warn_no_thread(irq, action);
				break;
			}

			__irq_wake_thread(desc, action);
			break;

		default:
			break;
		}

		retval |= res;
	}
```

```c
// kernel/irq/internals.h:165-166
#define for_each_action_of_desc(desc, act)			\
	for (act = desc->action; act; act = act->next)
```

| 行 | 意思 | 對照教授說的 |
|---|---|---|
| 141 | 預設回傳 **`IRQ_NONE`** | 「預設是沒被處理，但走完之後不一定是這個值」 |
| 142 | 從 `irq_data` 拿 **IRQ 號碼** | §6.1 |
| **147** | **`for_each_action_of_desc`**：展開成 `for (act = desc->action; act; act = act->next)`——**走 `irqaction` 單向串列** | 「這個巨集**基本上就是一個 for 迴圈**」 |
| **158** | **`res = action->handler(irq, action->dev_id);`**：**呼叫 driver 的 handler**，傳 IRQ 號碼和裝置 ID | 「對這個 IRQ 號碼和這個裝置，**請跑 handler**」 |
| 161–163 | 檢查 handler 有沒有**偷偷打開中斷**——top half 不該這樣做，有的話警告並關回去 | §10 的限制 |
| 166–177 | **`IRQ_WAKE_THREAD`** → **`__irq_wake_thread`**：喚醒 `irqaction->thread` | §6.4 |
| 171–174 | 檢查 driver 回了 `WAKE_THREAD` 卻沒設 `thread_fn` → 警告 | 防呆 |
| **183** | **`retval \|= res;`**：把每個 handler 的回傳值 **OR 起來** | 用位元旗標的好處（§6.5） |

**跟教授說的一個差異**：

- 教授說：「一個一個問，**直到找到負責的裝置**就停。」
- 原始碼：迴圈**沒有 `break`**——**每個 handler 都會被呼叫**，不管前面有沒有人回 `IRQ_HANDLED`。
- 課本說得對（p.136）：「**不管回傳值是什麼，整個 linked list 都會被走完，所有 handler 都會被呼叫**。」
- 為什麼？因為共用 IRQ 線時，**可能不只一個裝置同時發了中斷**。如果找到第一個就停，第二個裝置的中斷就會被漏掉（準位觸發的話，線會一直維持在 1，造成中斷風暴）。
- 所以 `retval |= res` 的意義是：「**至少有一個**人處理了」就是 `IRQ_HANDLED`。都沒人處理才是 `IRQ_NONE`——這會被 §6.5 的 `irq_count` / `last_unhandled` 統計。

### 9.4 【原始碼】driver 怎麼掛上 handler：`request_irq`

driver 註冊 interrupt handler 的函式：

```c
// include/linux/interrupt.h:164-169
static inline int __must_check
request_irq(unsigned int irq, irq_handler_t handler, unsigned long flags,
	    const char *name, void *dev)
{
	return request_threaded_irq(irq, handler, NULL, flags, name, dev);
}
```

```c
// include/linux/interrupt.h:71
#define IRQF_SHARED		0x00000080
```

| 參數 | 存到 `irqaction` 的哪裡 |
|---|---|
| `irq` | `irq` |
| `handler` | **`handler`**——driver 的函式 |
| `flags` | `flags`。要**共用** IRQ 線，就要給 **`IRQF_SHARED`** |
| `name` | `name`——顯示在 `/proc/interrupts` |
| `dev` | **`dev_id`**——handler 被呼叫時傳回來，讓 driver 知道是自己的哪個裝置 |

`request_irq` 就是 `request_threaded_irq` 的 `thread_fn = NULL` 版本。如果 driver 想要 threaded IRQ（回 `IRQ_WAKE_THREAD`），就直接呼叫 `request_threaded_irq` 並給一個 `thread_fn`——第 16 講。

共用 IRQ 時，driver 的 handler 一開始會**先問自己的硬體**：「是你發的中斷嗎？」（讀裝置的狀態暫存器，第 6 講的 PMIO / MMIO）。不是就馬上回 `IRQ_NONE`——這就是教授說的「一個一個問」，**由每個 driver 自己回答**。

---

## 10. Top half 的限制

`[65:30]` 「這基本上展示了串列的走訪，以及狀態是怎麼產生的。」

`[66:01]` handler（**top half**）有**很多限制**：

| 限制 | 說明 |
|---|---|
| **要很短** | 不能是長的程式碼 |
| **不能做會阻塞（blocking）的呼叫** | 不能讓 thread **睡覺**或被阻塞 |
| **不能印東西** | 不能做 I/O、不能印到 log |
| **只能做純計算** | |
| **不能存取使用者空間** | |

`[66:30]` 額外的工作要**延後**、交給另一個 thread——那個 thread 做的工作叫 **bottom half**：

- top half 把額外工作**排程**到之後的一個**延後函式呼叫（deferred function call）**。
- bottom half 才做**真正的粗活**，真正**服務**中斷。
- `[67:01]` 「top half 做一些基本處理，但它要**很快做完**，因為它優先權**超高**，在**壟斷**資源。bottom half 才做事。」

【課本補充】課本把 top half 執行的環境叫做 **interrupt context（中斷 context）**（p.138）：

| 中斷 context 裡**不能**做的事 |
|---|
| 阻塞呼叫，例如**拿鎖** |
| 被搶占（**preemption 是關的**） |
| stack 大小有限制（跟其他 kernel thread 類似） |
| **存取使用者空間的記憶體** |
| （大多數情況）**產生其他中斷** |
| **大量的記憶體配置** |
| **印資料** |
| 做 process 相關的函式 |

「這些顯然是**非常高優先權**的 thread 該有的屬性：要跑、要很快做完。」

bottom half 通常限制**比較少**：有些變體可以拿鎖、做複雜的同步、跑很久。而且 **bottom half 跑的時候中斷是開的**——因為它們優先權低，跑很久也不會讓系統不穩。

`[67:01]` 下一講：**exception**。

---

## 11. 一頁速查表

### 11.1 APIC

| | LAPIC | I/O APIC |
|---|---|---|
| 數量 | 每顆核心一個 | 系統一個（現在可以多個） |
| 工作 | 產生中斷向量送 CPU、收 IPI、有 timer | 路由 IRQ 給 LAPIC（重新導向表） |
| 設定 | MSR（`wrmsr` / `rdmsr`）：LDR、DFR、TPR | 每支腳位設一個 IRQ、建 domain |

### 11.2 名詞

| 名詞 | 意思 |
|---|---|
| IRQ | 裝置 → 中斷控制器的信號（一條線，或 MSI 訊息） |
| 中斷向量 | 0–255，CPU 拿來查 IDT |
| IRQ ↔ 向量 | 多對多，可動態程式化 |
| Disable | 全部關（大鐵鎚） |
| Mask | 關特定幾個 |
| Edge | 邊緣觸發，通常不用 ack |
| Level / `fasteoi` | 準位觸發，要 ack（拉回 0） |
| MSI / MSI-X | 訊息式中斷，沒有實體線（現代主流） |
| IPI | 一顆 CPU 叫另一顆 CPU 做事的唯一方法 |
| TPR | 寫入目前 task 優先權，讓中斷送給最閒的核心 |
| Tickless / one-shot | timer 只在需要時響一次 |

### 11.3 中斷向量範圍

| 範圍 | 課本 | v6.2.12 |
|---|---|---|
| 0–31 | 0–19 NMI / exception、20–31 保留 | 系統 trap 和 exception（NMI = 2） |
| 32–127 | 外部中斷 | 裝置中斷 |
| 128 | system call | legacy int80 system call |
| LAPIC timer | 239 | **236**（`0xec`） |
| 251–253 | IPI | `CALL_FUNCTION_SINGLE`、`CALL_FUNCTION`、`RESCHEDULE` |

### 11.4 Kernel 資料結構

| 結構 | 重點欄位 |
|---|---|
| `irq_desc` | `irq_common_data`（affinity）、`irq_data`（IRQ 號碼、向量）、`handle_irq`（flow handler）、`action`（串列）、`kstat_irqs`（per-CPU 計數） |
| `irqaction` | `handler`（driver）、`dev_id`、`next`（單向串列）、`thread_fn`、`thread`、`name` |
| `irqreturn_t` | `IRQ_NONE`（0）、`IRQ_HANDLED`（1）、`IRQ_WAKE_THREAD`（2） |
| IRQ domain | 像 namespace：IRQ 號碼在 domain 裡唯一；< 256 用線性串列，否則 radix tree |
| domain 階層 | I/O APIC → interrupt remapping → LAPIC → （CPU） |

### 11.5 中斷路徑

```
 裝置 → IRQ → I/O APIC → LAPIC → 中斷向量 → CPU 查 IDT（IDTR）
   → entry_64.S 存 context（跟 system call 共用入口）
   → common_interrupt：desc = vector_irq[vector]（per-CPU）
   → handle_irq → flow handler（handle_level_irq / handle_edge_irq）
   → __handle_irq_event_percpu：走 irqaction 串列，呼叫每個 driver 的 handler
   → IRQ_WAKE_THREAD → 喚醒 thread（bottom half）
```

### 11.6 開機

| 階段 | 做什麼 |
|---|---|
| BIOS | 陽春版 IDT（鍵盤等） |
| `early_irq_init` | 探測裝置、建 `irq_desc` 陣列 |
| `init_IRQ` | per-CPU 中斷 stack、基本 IDT |
| `x86_init.irqs.intr_mode_init` | `apic_intr_mode_init`：設 LAPIC、I/O APIC |

### 11.7 Top half

| 項目 | 內容 |
|---|---|
| 環境 | interrupt context |
| 不能 | 阻塞、拿鎖、睡覺、印東西、存取使用者空間、大量配置記憶體、打開中斷 |
| 要 | 很短；剩下的交給 bottom half |

### 11.8 本講看過的原始碼位置

| 檔案:行號 | 看什麼 |
|---|---|
| `arch/x86/include/asm/irq_vectors.h:14-109` | 向量範圍與特殊向量 |
| `include/linux/irqdesc.h:55-67` | `struct irq_desc` |
| `include/linux/interrupt.h:118-132` | `struct irqaction` |
| `include/linux/irqreturn.h:11-17` | `irqreturn_t` |
| `init/main.c:1044-1045` | `early_irq_init()`、`init_IRQ()` |
| `arch/x86/kernel/x86_init.c:77-83` | `x86_init.irqs` 函式指標 |
| `arch/x86/kernel/irq.c:240-264` | `common_interrupt` |
| `kernel/irq/handle.c:139-184` | `__handle_irq_event_percpu()` |
| `kernel/irq/internals.h:165-166` | `for_each_action_of_desc` |
| `include/linux/interrupt.h:164-169` | `request_irq()` |
| `include/linux/interrupt.h:71` | `IRQF_SHARED` |

---

## 12. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| **向量 0–31 的 exception、page fault 回到同一個 PC** | **第 16 講**：exception（下一講） |
| **`IRQ_WAKE_THREAD`、threaded IRQ、bottom half** | **第 16 講**：softirq、threaded IRQ、work queue |
| **中斷處理完要通知使用者 process** | **第 17 講**：signal |
| **IPI：`RES`、`CAL`、`TLB` shootdown** | **第 22 講**：kernel 裡的並行；**第 25 講**：多處理器排程；**第 30 講**：TLB 管理 |
| **top half 不能拿鎖、interrupt context** | **第 20–22 講**：鎖、spinlock 與中斷 |
| **TPR、把中斷送給最閒的核心** | **第 25 講**：多處理器排程 |
| **IRQ domain 把鍵盤中斷只送給一台 VM** | 課本第 8 章（虛擬化）。對應講次**待補** |
| **driver、`request_irq`、`drivers` 目錄** | 課本第 7 章 §7.4.2。對應講次**待補** |
| **即時系統的中斷延遲** | 另一門課 **Real Time Systems**：RTOS 基礎（第 18–19 講） |

---

## 附：本講一句話總結

> **x86 的中斷由兩層 APIC 處理：I/O APIC 把裝置的 IRQ（一條線，或現代的 MSI 訊息）路由給某顆核心的 LAPIC，LAPIC 把它轉成 0–255 的中斷向量送給 CPU（一顆 CPU 要叫另一顆做事，也只能靠 LAPIC 送 IPI——就是 `/proc/interrupts` 裡的 `RES`、`CAL`、`TLB`）。CPU 用向量查軟體填、硬體查的 IDT，進到跟 system call 共用的入口存好 context，再由 `common_interrupt` 用 per-CPU 的 `vector_irq[vector]` 找到 `irq_desc`；因為一條 IRQ 線可能接好幾個裝置，kernel 會走完整條 `irqaction` 串列、呼叫每個 driver 的 handler（`IRQ_NONE` / `IRQ_HANDLED` / `IRQ_WAKE_THREAD`）。這些 handler 就是 top half：要很短、不能阻塞，剩下的工作交給 bottom half。**

---

*筆記依據 Smruti R. Sarangi「Operating Systems: A Linux Kernel-Oriented Approach」Lecture 15（Chapter 4, Part II）錄影逐字稿（YouTube 自動字幕，1:07:33）、課本 *Operating Systems: A Linux Kernel-Oriented Approach*（Sarangi, v0.96）§4.2–4.2.6，以及 Linux kernel v6.2.12 原始碼整理。`/proc/interrupts` 為本機（Ubuntu、kernel 6.5.0、20 顆 CPU）實測。*
