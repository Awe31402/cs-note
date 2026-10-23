# Operating Systems: A Linux Kernel-Oriented Approach 第一講完整筆記：導論（Introduction）

> **課程**：Operating Systems: A Linux Kernel-Oriented Approach — Lecture 1（Chapter 1）
> **講者**：Smruti R. Sarangi
> **來源**：YouTube 錄影逐字稿（49:20，自動字幕）＋ 課本 *Operating Systems: A Linux Kernel-Oriented Approach*（Sarangi, v0.96）第 1 章 §1.0–1.3（p.9–20）＋ Linux kernel **v6.2.12** 原始碼（`os/linux-src/`）
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
- [1. OS 是什麼](#1-os-是什麼)
  - [1.1 一個有特權的程式：班長比喻](#11-一個有特權的程式班長比喻)
  - [1.2 OS 在系統裡的位置](#12-os-在系統裡的位置)
- [2. 為什麼需要 OS](#2-為什麼需要-os)
  - [2.1 硬體是共用的 → 會起衝突 → 要有人管](#21-硬體是共用的--會起衝突--要有人管)
  - [2.2 OS 要管的東西](#22-os-要管的東西)
  - [2.3 【課本補充】OS 的正式定義](#23-課本補充os-的正式定義)
- [3. 兩個核心工作：仲裁與控管存取](#3-兩個核心工作仲裁與控管存取)
  - [3.1 【Q&A】硬體不同，OS 也要不同嗎？](#31-qa硬體不同os-也要不同嗎)
  - [3.2 仲裁（Arbitration）vs 控管存取（Regulating Access）](#32-仲裁arbitrationvs-控管存取regulating-access)
- [4. OS 的種類](#4-os-的種類)
- [5. Linux 的歷史](#5-linux-的歷史)
- [6. GPL 授權：為什麼它讓 Linux 贏](#6-gpl-授權為什麼它讓-linux-贏)
  - [6.1 GPL 的規則](#61-gpl-的規則)
  - [6.2 經濟邏輯：大家合養一個 OS](#62-經濟邏輯大家合養一個-os)
  - [6.3 禁止 Tivoization](#63-禁止-tivoization)
  - [6.4 缺點與 BSD 授權](#64-缺點與-bsd-授權)
  - [6.5 【原始碼】kernel 的授權檔](#65-原始碼kernel-的授權檔)
- [7. 版本號怎麼看](#7-版本號怎麼看)
  - [7.1 x.y.z 與 release candidate](#71-xyz-與-release-candidate)
  - [7.2 【原始碼】版本號寫在哪裡](#72-原始碼版本號寫在哪裡)
  - [7.3 【Q&A】LTS 是什麼？](#73-qalts-是什麼)
- [8. Kernel 原始碼地圖](#8-kernel-原始碼地圖)
  - [8.1 程式碼量與分布](#81-程式碼量與分布)
  - [8.2 【原始碼】自己數一次：v6.2.12 各目錄行數](#82-原始碼自己數一次v6212-各目錄行數)
  - [8.3 `arch`：把硬體差異關起來](#83-arch把硬體差異關起來)
  - [8.4 【原始碼】`u64` 是怎麼來的](#84-原始碼u64-是怎麼來的)
  - [8.5 其他重要目錄](#85-其他重要目錄)
  - [8.6 【原始碼】`init`：開機的第一個 C 函式](#86-原始碼init開機的第一個-c-函式)
  - [8.7 Block device vs Character device](#87-block-device-vs-character-device)
  - [8.8 【Q&A】sound 不是 driver 嗎？](#88-qasound-不是-driver-嗎)
- [9. 整門課的地圖](#9-整門課的地圖)
  - [9.1 八章內容](#91-八章內容)
  - [9.2 三個附錄](#92-三個附錄)
  - [9.3 收尾：OS 的四大塊](#93-收尾os-的四大塊)
- [10. 一頁速查表](#10-一頁速查表)
- [11. 與後續講次的連結](#11-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 這堂課要做什麼

`[00:02]` 這是第 1 章「導論」。目標只有一個：搞懂 **OS 是什麼、為什麼需要它**，再看一眼整門課要走的路。

`[01:30]` 教授列的先備知識：

| 先備知識 | 為什麼需要 |
|---|---|
| **C / C++** | 系統軟體大量用指標運算（pointer arithmetic） |
| **計算機結構**（computer architecture） | 硬體很「生」（raw），OS 就是蓋在它上面、讓它能用的那層軟體 |
| **資料結構** | OS 要在硬體和軟體之間搭橋，會用到不少複雜的資料結構 |

【口頭】教授的比喻：電腦系統像一個三明治，上面還有虛擬機、middleware 等很多層，但 **OS 是中間那塊肉（patty）**，最重要的一層。

---

## 1. OS 是什麼

### 1.1 一個有特權的程式：班長比喻

`[00:30]` OS 本質上**也是一個程式**，但它很特別：

- 它可以**控制其他一般程式**。
- 它可以**控制硬體**——一般程式不行（之後會看到為什麼一般程式**不應該**可以）。

教授（和課本）用的比喻是 **班長（class monitor）**：大家都是學生，但班長多了一點權力，負責管其他人。用英文說法是「more than an equal among equals」。沒有這個角色，現代電腦根本跑不起來。

【課本補充】課本把理由講得更直白（p.9）：

1. 設計硬體時，目標是**省電和效能**，不是「好用」。一次只專心做一件事比較好。
2. 不能讓一般程式碰到所有硬體功能：一是**安全**問題，二是就算程式沒惡意，一個不小心的改動也可能讓整台機器掛掉。
3. 所以需要一個專門的機制，讓「可能影響安全、可能弄垮系統」的操作，在**嚴格控制**下執行。這就是 OS。

### 1.2 OS 在系統裡的位置

`[02:33]` 先看一張「超級簡化」的圖（課本 Fig 1.1）：

```
   ┌──────┐   ┌────────┐   ┌──────────┐
   │ CPUs │   │ Memory │   │ Hard disk│
   └──┬───┘   └───┬────┘   └────┬─────┘
      └───────────┴──── I/O devices ──┘
```

- CPU 可以想成「核心（core）」，有很多個。
- 這些硬體**原始狀態下軟體沒辦法直接用**，需要有人管理。OS 管好了，一般程式就可以「放心，一切都好（all is well）」。

`[03:33]` OS 的位置（課本 Fig 1.2）：

```
     P1      P2      P3        ← 一般程式（你執行的 binary）
   ─────────────────────────
         Operating System      ← 中間層：仲介、保全、資源管理者
   ─────────────────────────
   CPUs │ Memory │ I/O & Storage   ← 各種硬體
```

OS 像個**仲介（broker）**。連你插上去的一支 USB 滑鼠，都需要 OS 來管。

---

## 2. 為什麼需要 OS

### 2.1 硬體是共用的 → 會起衝突 → 要有人管

`[04:02]` 70 年代早期其實沒有 OS，但大家很快發現需要。80 年代起，從 Unix 開始出現各種 OS。

【口頭】寫 C 語言的 Kernighan 和 Ritchie 也寫了 Unix——教授說他們是「多才多藝的人」。（⚠️ 逐字稿寫成「kingan and Richie」，依上下文判斷）

`[05:01]` 核心理由是一條鏈：

> **硬體是共用的** → 共用就可能**起衝突** → 有衝突就需要**衝突管理者** → 這個管理者就是 **OS**。

連智慧手錶都有 OS。

### 2.2 OS 要管的東西

`[05:30]`–`[08:32]`

| 資源 | OS 要做什麼 |
|---|---|
| **CPU** | 20 個程式、只有 4 個核心 → 要**排程**（scheduling）。計組學過的 **timer interrupt**（計時器晶片發出的中斷）在排程裡扮演關鍵角色 |
| **裝置** | 不同裝置講不同「語言」：USB 裝置講 USB、網路裝置講 Ethernet。OS 要把**應用層的呼叫轉成裝置層的呼叫** |
| **儲存** | 硬碟、隨身碟要管理 |
| **記憶體** | 現代系統可以插拔記憶體條，也要管理 |
| **安全** | 非常重要，尤其是嵌入式裝置、穿戴裝置 |
| **分散式** | OS 不再只管一台機器（見下方） |

【口頭】分散式系統的例子：你把檔案放上 Google Drive / OneDrive，跑到地球另一端打開，還是看得到。顯然不是同一台機器，是**很多台機器合作，讓它們看起來像一台**。這也是現代 OS 的工作。

**整體哲學**：不管底下是普通記憶體、一支手錶、還是一整個分散式系統，OS 都要讓它**好用**。

### 2.3 【課本補充】OS 的正式定義

課本 Summary 1.0.1 多講了一點：

- 不同程式要依使用者設定的**優先權**，**公平**分到硬體資源。
- 多個程式同時存取同一個資源時，要有**紀律**。
- 和裝置溝通要靠 OS 裡的專門程式：**device driver（裝置驅動程式）**。這個責任不能丟給一般程式。
- 很多程式存取同一塊記憶體，可能是**安全違規**，也可能是**正當的共享記憶體通訊**，OS 要能分辨。
- **功耗、溫度、安全**近十年變得非常重要。OS 要能在手機、手錶、甚至未來的智慧眼鏡上跑，所以要很「省電意識（power-aware）」。

> **Definition 1.0.1（課本）**：OS 是 **CPU 管理者、記憶體管理者、裝置管理者、儲存管理者**。它的工作是**仲裁**對這些資源的存取，確保程式**安全**地執行，並在功耗與溫度限制下**最大化效能**。

---

## 3. 兩個核心工作：仲裁與控管存取

`[08:32]` 教授總結 OS 的基本概念：**仲裁（arbitration）** 和 **控管存取（regulating accesses）**。

### 3.1 【Q&A】硬體不同，OS 也要不同嗎？

`[08:52]`

**學生問**：OS 跟硬體互動，那硬體不同，OS 是不是也不同？（⚠️ 逐字稿這句不完整，依教授回答整理）

**教授答**：**不一定**。

- Linux 可以跑在 ARM、x86、PowerPC 上；Windows 可以跑在各種筆電上。
- 做法是：OS 有給各種硬體和 CPU 的 **driver**，由 driver 把硬體**抽象化**。
- OS 本身是**多層結構**：有**通用部分**，也有**架構相關部分**（architecture-specific）。架構相關的部分負責把底層架構藏起來。
- 當然，沒有一個 OS 能跑在**所有**硬體上，每個 OS 都有自己的目標硬體。

（這就是後面 [§8.3 `arch` 目錄](#83-arch把硬體差異關起來) 的伏筆。）

### 3.2 仲裁（Arbitration）vs 控管存取（Regulating Access）

`[10:32]` 這兩件事「有點像，但不完全一樣」。

先定義：**process（行程）= 程式正在執行的一個實例**。同一個 binary（例如記事本）開三次，就是三個 process。

| | 仲裁（Arbitration） | 控管存取（Regulating Access） |
|---|---|---|
| **問題** | 大家都想用，**誰先用？** | 你**有沒有資格**用？ |
| **例子 1** | 多個 process 搶 CPU | — |
| **例子 2** | 多個 process 同時要印東西：不能這台印一頁、那台印一頁混在一起。要讓一個印完，再換下一個 | 公司裡的印表機有**列印權限**。印表機在網路上大家都看得到，但「看得到」不代表「能印」——我不能去 CEO 的印表機印東西 |
| **例子 3** | — | 我能不能讀另一個使用者的信用卡資料？**不行** |
| **出發點** | 避免衝突：n 個人搶，讓 1 個用、停掉 n−1 個 | 安全 |

---

## 4. OS 的種類

`[12:02]` OS 的範圍非常大：

| 平台 | 重點（課本 §1.1 補充） |
|---|---|
| **高效能運算、伺服器** | 跑大量科學運算、參與分散式運算 |
| **桌機、筆電** | 要支援非常多樣的程式和裝置 → 彈性高 → 容易中毒 → **安全是第一要務** |
| **手機**（iOS、Android） | 基本上是一台完整電腦，但資源少，**電池**是最大限制 |
| **智慧手錶** | 程式不多，但要很順；功耗限制嚴格，OS 的**程式碼大小和記憶體用量**要很小 |
| **智慧眼鏡、護目鏡**（未來） | — |

【口頭】教授想像的智慧眼鏡：在工廠裡看著熔爐，眼鏡直接顯示熔爐的規格、溫度等資訊。

---

## 5. Linux 的歷史

`[13:00]` 為什麼這門課用 Linux？

【口頭】教授說：「如果你問我過去三、四十年，什麼東西真正造成了影響——不只是影響，是遠遠超過影響——那就是 **Linux 的開發和普及**。」

`[13:31]` 故事：

1. **1991 年**，芬蘭學生 **Linus Torvalds** 想做 OS。他先玩一個教學用的小 OS 叫 **Minix**（可以想成 mini Unix）。
   - 【口頭】「就像你們現在在玩 xv6 一樣。」
   - ⚠️ 年份不一致：教授說 1991；課本 p.12 說「It all started in **1990**」，並說第一版在 **1992** 以 GPL 發布。兩邊都記下。
2. 他很快發現 Minix 的問題，不想繼續，決定自己來。
3. 關鍵設計：**外部介面跟 Unix 一樣，但程式碼全部重寫**。

【課本補充】課本多講的歷史（p.12–13）：

- 當時的 Unix 大多是商業授權，學生買不起；Minix 主要給學術用。
- 很多開發者加入，對他們來說這比較像**興趣**而不是工作。
- 同時 **GNU**（GNU is Not Unix）和自由軟體基金會（**FSF**）正在發展各種工具程式。Linux 社群和他們一拍即合，很多 GNU 工具就成了 Linux 的一部分。
- **2003 年**：kernel **2.6** 發布，是個分水嶺。之後學界和業界都開始認真看待 Linux，出現 Red Hat、SUSE、Ubuntu（Canonical）等公司。

`[14:30]` 教授認為 Linux 成功**最聰明的地方不是技術，而是商業模式**——也就是 GPL。

---

## 6. GPL 授權：為什麼它讓 Linux 贏

### 6.1 GPL 的規則

`[14:30]` Linus 是最早大量使用 **GNU General Public License（GPL）** 的人之一。

【口頭】GNU 是**遞迴定義**：GNU = **G**NU is **N**ot **U**nix。

`[15:03]`、`[20:03]` GPL 的內容：

| 給你的自由 | 限制 |
|---|---|
| 使用（use） | 任何**衍生作品**（derivative work）也必須是 GPL |
| 修改（modify） | 不能拿我的程式碼去賣，然後把修改**藏起來** |
| 散布（distribute） | 你做的任何修改，都要**回饋**到開源程式碼庫 |

教授補一句：自己私下用沒問題；**一旦拿去賣，就必須回饋**。

【課本補充】課本的說法更精確：GPL 跟當時其他自由授權最大的差別是——**重新散布的人不能用更嚴格的授權**。所以所有改進都留在社群裡，沒有「私有圍牆」，社群進步得很快。

### 6.2 經濟邏輯：大家合養一個 OS

`[15:32]` 【口頭】教授的算法：

> 假設做一個 OS 要 1000 人。我是一家公司，為什麼要自己養 1000 人？
> 我出 50 人、別人出 50 人……大家一起做出一個**屬於社群**的東西。
> 如果 OS 不是我的主業，我**花很少人力，就有一個很棒的 OS**。

結果：商業公司、業餘愛好者、學術界都有興趣加入。Linux 成了**伺服器和高效能運算的事實標準**，而且免費。

> 【口頭】「現在任何大型部署，你可以有 95% 的把握說它是 Linux。」

【課本補充】Point 1.2.1 講的是同一件事：硬體公司主業是處理器或軟體服務，何必自己做 OS？買幾十萬份商業 OS 授權也太貴。不如派**幾十個工程師**參與 Linux，很多公司加起來就等於一支專職 OS 團隊，成本只有一小部分，而且**沒有長期商業風險**。課本也提到：Linux 大約到專案開始**十年後**才在效能上站穩腳步，前十年能撐下去，是因為大家看好它的未來。

`[16:31]` 衍生出來的系統：**Android**、Samsung 的 **Tizen**、智慧手錶的 OS……

> 【口頭】「現在你看到的東西，只要不是 Windows、不是 macOS，基本上都是某種 Linux 變體。」

而且因為 GPL，這些全都**必須開源**。

### 6.3 禁止 Tivoization

`[20:31]` **Tivoization**（⚠️ 逐字稿寫成「too isation / tization」）：

- 意思是：我做一台硬體，它**只能跑我給的那一版**開源軟體（例如用金鑰加密鎖住），不能跑其他版本的 Linux。
- 這等於**把我的修改綁死在硬體上**，變相藏起來。

GPL（教授說的版本）**禁止**這樣做：

> 【口頭】假設那台攝影機跑 Linux，廠商不能說「這是特製版 Linux，原始碼不給你」。我應該要能拿到那一版、隨便改、再灌回攝影機。例如我想讓攝影機做機器學習，就可以自己改 kernel。

`[38:01]` 教授後面又提到一次：智慧電視跑 Tizen、webOS，你可以去官網下載原始碼，看看你的電視到底在跑什麼；因為禁止 Tivoization，理論上你也可以改電視上的 OS。

> ⚠️ 補充說明（不是教授講的，查證後再確認）：教授這段把「禁止 Tivoization」直接歸給 GPL。但 kernel 用的是 **GPL 第 2 版**（見 [§6.5](#65-原始碼kernel-的授權檔)），一般認為明文禁止 Tivoization 的是 GPL **第 3 版**。這點待補，之後若教授或課本有更多說明再更新。

### 6.4 缺點與 BSD 授權

`[22:30]` GPL 不是人人都喜歡：

- 修改**一定要公開**，所以很難做「產品」。例如我賣一台跑 Linux 的電視，裡面有我花大錢做的修改（智慧財產），卻沒辦法藏。

【口頭】教授以前在某公司實習，那家公司就因為這個限制，改用 **FreeBSD**。BSD 授權沒有這個限制，可以改 kernel、賣改過的版本、賺錢。

【課本補充】Linux 不是唯一的免費開源 OS。BSD 家族（FreeBSD、OpenBSD、NetBSD）也很好，授權比 GPL 寬鬆，各有自己的市場。課本作者和學生有一篇論文比較過 Linux、FreeBSD、OpenBSD 的效能 [Singh and Sarangi, 2020]。

`[23:30]` 【口頭】教授的提醒：授權**不是點一下「我同意」就算了的小事**，它對軟體的成長和發展影響很大。

### 6.5 【原始碼】kernel 的授權檔

kernel 根目錄的 `COPYING` 寫明了授權：

```text
// COPYING:1-12
The Linux Kernel is provided under:

	SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note

Being under the terms of the GNU General Public License version 2 only,
according with:

	LICENSES/preferred/GPL-2.0

With an explicit syscall exception, as stated at:

	LICENSES/exceptions/Linux-syscall-note
```

逐行看：

| 行 | 意思 |
|---|---|
| `SPDX-License-Identifier:` | SPDX 是一套「授權名稱的標準寫法」。kernel 每個檔案開頭都有這行，機器就能自動判讀授權 |
| `GPL-2.0` | **GPL 第 2 版**，而且是「version 2 **only**」——不是「第 2 版或更新版」 |
| `WITH Linux-syscall-note` | 加一條**例外條款**（見下） |

這條例外很重要，它回答了「我寫的程式在 Linux 上跑，要不要也開源？」：

```text
// LICENSES/exceptions/Linux-syscall-note（License-Text 開頭）
   NOTE! This copyright does *not* cover user programs that use kernel
 services by normal system calls - this is merely considered normal use
 of the kernel, and does *not* fall under the heading of "derived work".
```

意思：**一般程式透過 system call 使用 kernel，只算「正常使用」，不算「衍生作品」**。所以你在 Linux 上寫的閉源程式不用開源；只有**改 kernel 本身**才要遵守 GPL。

這也跟教授說的「Android 必須開源」對得起來：被要求開源的是 Android 用的 **Linux kernel** 部分；跑在上面的 App 透過 system call 使用 kernel，不受 GPL 約束。（後半句是依授權檔推論，不是教授原話。）

---

## 7. 版本號怎麼看

### 7.1 x.y.z 與 release candidate

`[19:31]` 教授說課程會用 **6.2.12**（大約一年前的版本），到 2025 年也「完全不過時」，有小改動他會指出來。

`[24:31]` 版本號格式（課本 Fig 1.3）：

```
   6  .  2  .  12   -rc<num>
   │     │     │      └─ release candidate（測試版）
   │     │     └─ patch number（修補號）
   │     └─ minor version（次版本號）
   └─ major version（主版本號）
```

| 部分 | 教授的說法 | 課本的說法 |
|---|---|---|
| **major** | 一大組功能改變，例如新檔案系統、新排程器 | 重要的**架構**改變 |
| **minor** | 沒那麼大，但仍是實質改變 | 重要的 bug 修正和新功能 |
| **patch** | 例行改進；**大約每兩週**一次 | 小問題和**安全性**修正 |
| **release candidate** | 不是給正式環境用的。我做了東西分享給社群，大家評估，根據回饋出下一個 rc，最後成熟成下一個版本 | 編號 `-rc1, -rc2, …`；主要給其他 kernel 開發者測試、討論；討論收斂後被穩定版取代 |

【課本補充】2004 年以前，minor 是**偶數 = 穩定版、奇數 = 開發版**。從 3.0 開始不再這樣，**每個版本都是穩定版**，開發版就是 release candidate。

### 7.2 【原始碼】版本號寫在哪裡

版本號就寫在 kernel 根目錄 `Makefile` 的最前面：

```makefile
# Makefile:1-6
# SPDX-License-Identifier: GPL-2.0
VERSION = 6
PATCHLEVEL = 2
SUBLEVEL = 12
EXTRAVERSION =
NAME = Hurr durr I'ma ninja sloth
```

| 變數 | 對應 | 本版的值 |
|---|---|---|
| `VERSION` | major | 6 |
| `PATCHLEVEL` | minor | 2 |
| `SUBLEVEL` | patch | 12 |
| `EXTRAVERSION` | 額外字尾，例如 `-rc3` | 空的（代表這是正式穩定版） |
| `NAME` | 這一版的代號（開發者取好玩的） | — |

注意名詞對不上：教授和課本說的「**minor**」，在 Makefile 裡叫 `PATCHLEVEL`；說的「**patch**」，在 Makefile 裡叫 `SUBLEVEL`。翻原始碼時別搞混。

### 7.3 【Q&A】LTS 是什麼？

`[25:30]`

**學生問**：有時候會看到 LTS？（⚠️ 逐字稿只有「sir sometimes LTS」，問題不完整）

**教授答**：LTS = **Long-Term Support（長期支援）**，主要是**發行版（distribution）** 的事，不是 kernel 的事。

- **kernel** 有自己的週期，由 **Linux Foundation** 維護。
- **發行版**（Red Hat、SUSE、Ubuntu……）是不同公司。他們把 kernel 加上**套件**和**套件管理器**打包起來。有些套件只能配某版 kernel，所以發行版要長期維護、支援、提供套件庫——這就是 LTS。
- Linux 允許任何人散布，所以才有這麼多發行版。

---

## 8. Kernel 原始碼地圖

### 8.1 程式碼量與分布

`[19:03]` 原始碼的正式倉庫是 **kernel.org**，但課堂上用 **elixir.bootlin.com**（⚠️ 逐字稿寫成「lx. boot Lin」）。它的好處是**可以點連結跳轉**。

【口頭】教授說：你不用為了這門課讀很多 kernel 程式碼，但要知道 kernel 長什麼樣——「看看一個 2500 萬行的程式碼庫長什麼樣子，總是好的。」

`[24:00]` 數字（教授口頭 vs 課本）：

| 項目 | 教授 | 課本（p.14） |
|---|---|---|
| 開發歷史 | 33 年 | — |
| 總行數 | 約 **2500 萬**行 | v6.2 約 **2300 萬**行 |
| 每個版本改動 | 約 25 萬行 | 每次版本變更約**新增** 25 萬行 |

`[26:31]`–`[32:30]` 教授投影片上的分布（課本 Fig 1.4 是圓餅圖，PDF 抽不出文字，數字依教授口頭）：

| 目錄 | 佔比（教授） | 說明 |
|---|---|---|
| `drivers` | **70%** | 鍵盤、滑鼠、印表機、掃描器……每種裝置都要 driver |
| `arch` | — | 架構相關（課本：超過 170 萬行） |
| `fs` | — | 檔案系統 |
| `include` | — | header 檔 |
| `kernel` | **1%** | 核心程式碼。「但 2500 萬的 1% 還是很多」（⚠️ 教授接著說「kernel code is 250,000 lines」，跟 1% 對得上） |
| `net` | — | 網路協定堆疊 |
| `sound` | **4%** | 聲音、影像、codec |
| 其他 | **4%** | 雜項 |

**為什麼 driver 要放在 kernel 裡？** `[27:01]`

- 你裝好 OS，希望東西**插上就能用（out of the box）**，不想每樣都上網找 driver。
- 所以**熱門裝置**的 driver 最好直接放在 kernel 裡。
- 冷門的新掃描器沒有內建 driver？那就上網找、自己裝。

【課本補充】課本補了另一面：也不能把全世界每個 driver 都塞進來，不然 kernel 會大到不行。冷門和過時的裝置可以不收。kernel 開發者要挑：夠**熱門**、而且被認為**安全**（沒有安全漏洞）的 driver。

【口頭】教授說課程重點會放在 `kernel` 目錄，再加一點 `fs` 和 `arch`；`net`、`sound` 裡有一大堆小目錄，對應各種協定和 codec。

### 8.2 【原始碼】自己數一次：v6.2.12 各目錄行數

我用下載的 v6.2.12，數每個頂層目錄裡 `.c`、`.h`、`.S`、`.rs` 檔的**總行數**（包含空行和註解）：

```bash
# 在 os/linux-src/ 下執行
for d in */; do
  n=$(find "$d" -type f \( -name '*.c' -o -name '*.h' -o -name '*.S' -o -name '*.rs' \) \
      -print0 | xargs -0 cat | wc -l)
  echo "$n ${d%/}"
done | sort -rn
```

結果（前 12 名）：

| 目錄 | 行數 | 佔比 |
|---|---:|---:|
| `drivers` | 22,059,817 | **68.6%** |
| `arch` | 2,574,418 | 8.0% |
| `fs` | 1,472,945 | 4.6% |
| `sound` | 1,428,599 | 4.4% |
| `net` | 1,233,461 | 3.8% |
| `include` | 1,205,089 | 3.7% |
| `tools` | 901,495 | 2.8% |
| `kernel` | 440,687 | **1.4%** |
| `lib` | 229,287 | 0.7% |
| `mm` | 182,750 | 0.6% |
| `crypto` | 107,875 | 0.3% |
| `security` | 107,336 | 0.3% |
| **總計**（全部目錄） | **32,147,523** | 100% |

跟教授、課本對照：

- **比例對得上**：`drivers` 68.6% ≈ 教授說的 70%；`sound` 4.4% ≈ 4%；`kernel` 1.4% ≈ 1%。
- **總數對不上**：我數到約 3200 萬行，教授說 2500 萬、課本說 2300 萬。原因應該是**算法不同**：我連空行、註解都算進去了；常見的統計工具會扣掉這些。課本沒說它怎麼算的，所以只能說「算法不同」，不能說誰錯。
- **`drivers` 為什麼這麼大？** 光 `drivers/gpu/drm/amd`（AMD 顯示卡）就有約 485 萬行，其中 `drivers/gpu/drm/amd/include/asic_reg/` 這種**硬體暫存器定義的 header** 就佔約 387 萬行。也就是說，「70% 是 driver」裡有一大塊是自動產生的暫存器表，不是手寫邏輯。（這段是我自己量的，教授沒講。）
- `kernel` 目錄 44 萬行，比教授說的 25 萬行多。同樣可能是算法不同（空行、註解）。

### 8.3 `arch`：把硬體差異關起來

`[27:31]` **這個目錄很重要。**

- kernel 大部分程式碼是**通用的**：跑在 x86、ARM、PowerPC 上都一樣。
- 跟機器有關的部分——**組合語言**、機器的**特殊指令**——全部關在 `arch` 目錄裡。
- 其餘的 kernel 就**不用知道**自己跑在什麼架構上（architecture-agnostic）。

教授的例子 `[28:30]`：

> 你的機器可能不支援 64 位元整數，但我（寫通用程式碼的人）就是要 64 位元整數。那我定義一個型別 `u64`（unsigned 64-bit）。**怎麼在你的機器上做出 64 位元整數，是你 `arch` 目錄的工作。** 在某些架構上 64 位元整數是 `long long`，在另一些架構上可能是別的東西。

為什麼要這樣？因為 Linux 要跑在**從智慧手錶到伺服器**的所有東西上：不同晶片、不同指令集（ISA）、32 位元、64 位元，甚至 16 位元。

【課本補充】課本列了 `arch` 要藏起來的差異：16 位元或 32 位元、**little endian 或 big endian**（位元組順序）、**CISC 或 RISC**。開機、driver、存取特權硬體操作，都要靠處理器相關的程式碼，這些全打包在 `arch`。`arch` 裡每個子目錄對應一種架構（x86、ARM、SPARC……）。

v6.2.12 的 `arch/` 底下有這些架構：

```text
alpha  arc  arm  arm64  csky  hexagon  ia64  loongarch  m68k  microblaze
mips  nios2  openrisc  parisc  powerpc  riscv  s390  sh  sparc  um  x86  xtensa
```

### 8.4 【原始碼】`u64` 是怎麼來的

教授說的 `u64`，在原始碼裡長這樣。kernel 內部用的短名字定義在：

```c
// include/asm-generic/int-ll64.h:16-23
typedef __s8  s8;
typedef __u8  u8;
typedef __s16 s16;
typedef __u16 u16;
typedef __s32 s32;
typedef __u32 u32;
typedef __s64 s64;
typedef __u64 u64;
```

- `u` = unsigned（無號）、`s` = signed（有號），後面的數字是**位元數**。
- 所以 `u64` 就是「無號 64 位元整數」，kernel 程式碼到處都用它。
- 它只是 `__u64` 的別名。`__u64` 又是什麼？往下追：

```c
// include/uapi/asm-generic/int-ll64.h:29-35
#ifdef __GNUC__
__extension__ typedef __signed__ long long __s64;
__extension__ typedef unsigned long long __u64;
#else
typedef __signed__ long long __s64;
typedef unsigned long long __u64;
#endif
```

| 行 | 意思 |
|---|---|
| 29 | 如果是用 GCC 編譯…… |
| 31 | `__u64` 就是 `unsigned long long`。檔名的 `ll64` 就是「**l**ong **l**ong 是 **64** 位元」 |
| 30、33 | `__signed__` 是 GCC 的關鍵字，等於 `signed` |
| `__extension__` | 告訴 GCC：「我知道 `long long` 在舊 C 標準裡不算標準，別警告我」 |

**架構差異在哪裡？** 還有另一個檔案 `int-l64.h`（**l**ong 是 **64** 位元），用 `long` 而不是 `long long`：

```c
// include/uapi/asm-generic/int-l64.h:29-30
typedef __signed__ long __s64;
typedef unsigned long __u64;
```

各架構自己選要用哪一個。例如 **ia64**（Itanium）：

```c
// arch/ia64/include/uapi/asm/types.h:18-20
#ifndef __KERNEL__
#include <asm-generic/int-l64.h>
#endif
```

這就是教授說的「在某些架構上是 `long long`，在另一些架構上是別的東西」。

**但要注意現況**（v6.2.12）：

```c
// include/uapi/asm-generic/types.h:4-7
/*
 * int-ll64 is used everywhere now.
 */
#include <asm-generic/int-ll64.h>
```

- 註解說「**現在到處都用 int-ll64**」。
- ia64 那段有 `#ifndef __KERNEL__`，意思是只有**編譯使用者程式**時才用 `int-l64`；**kernel 內部**現在所有架構的 `u64` 都是 `unsigned long long`。
- x86 自己沒有 `types.h`，直接用這個通用版本（`include/uapi/asm-generic/Kbuild:35` 把 `types.h` 列為 `mandatory-y`，沒有自己版本的架構會自動拿通用版）。

所以「每個架構自己定義 `u64`」這個**機制**還在，但 kernel 內部的實際定義已經統一了。（這是我對照原始碼的觀察，教授沒細講。）

### 8.5 其他重要目錄

`[33:01]` 教授逐一介紹（課本 Fig 1.5）：

| 目錄 | 內容 | 硬體相關？ |
|---|---|---|
| `init` | **開機**程式碼 | 是 |
| `arch` | 所有架構相關的組合語言 | 是 |
| `drivers` | 所有 driver：把通用 OS 呼叫翻成硬體專用指令 | 是 |
| `kernel` | **核心**：process 排程、同步、時間管理、事件管理、除錯 | 否 |
| `mm` | **記憶體管理**（memory manager）：實體和虛擬記憶體 | 否 |
| `fs` | **檔案系統** | 否 |
| `block` | 所有 block 儲存裝置（硬碟等）的**通用層** | 否 |
| `io_uring` | I/O 支援用的資料結構 | 否 |
| `virt` | **虛擬化**：讓一個 OS 當成一般程式跑另一個 OS | 否 |

各目錄的口頭重點：

- **`init`** `[33:01]`：OS 怎麼開機本身就很有趣。裡面有一個 start 函式，可以一步一步看 OS 做了什麼。這是**編排好的固定順序**——假設有 200 步，順序**不能亂**。課程之後也會走一遍。（原始碼見 [§8.6](#86-原始碼init開機的第一個-c-函式)）
- **`drivers`** `[33:32]`：【口頭】「大部分 OS 開發者其實是 **driver 開發者**。」每出一個新裝置就要寫新 driver，所以 OS 的工作大多在這裡；kernel 本身的改動相對少。
- **`io_uring`** `[35:01]`：做 I/O 很難——你往裝置推資料有一個速率，裝置讀資料有一個速率，兩者要配合。（⚠️ 逐字稿這段斷斷續續）
- **`kernel`** `[35:01]`：課程**大部分注意力**都在這裡。【口頭】教授說他會用英式發音讀 scheduling（"shed-yoo-ling"）。
- **`virt`** `[35:32]`：課程最後會講**虛擬化**——一個 OS 把另一個 OS 當程式跑，像 **KVM、VirtualBox、VMware**。一台 AWS 機器可以跑 5 個 OS，每個 OS 上面再跑應用程式，「OS 上的 OS」。這是**雲端運算的核心技術**。虛擬化有兩種：完整的虛擬機，和輕量的 **Docker container**，兩者不一樣，之後會講。

`[37:30]` 課本把這些目錄分成三大類：

```
           所有程式碼
     ┌────────┼─────────────┐
 Process 管理  記憶體與 I/O     硬體相關
  kernel      mm, fs,        arch, drivers,
  virt        block,         init
              io_uring
```

### 8.6 【原始碼】`init`：開機的第一個 C 函式

教授說的「start 函式」是 `init/main.c` 裡的 `start_kernel()`。架構相關的組合語言把機器準備好之後，就會跳到這裡。開頭幾行：

```c
// init/main.c:940-972
asmlinkage __visible void __init __no_sanitize_address start_kernel(void)
{
	char *command_line;
	char *after_dashes;

	set_task_stack_end_magic(&init_task);
	smp_setup_processor_id();
	debug_objects_early_init();
	init_vmlinux_build_id();

	cgroup_init_early();

	local_irq_disable();
	early_boot_irqs_disabled = true;

	/*
	 * Interrupts are still disabled. Do necessary setups, then
	 * enable them.
	 */
	boot_cpu_init();
	page_address_init();
	pr_notice("%s", linux_banner);
	early_security_init();
	setup_arch(&command_line);
	setup_boot_config();
	setup_command_line(command_line);
	setup_nr_cpu_ids();
	setup_per_cpu_areas();
	smp_prepare_boot_cpu();	/* arch-specific boot-cpu hooks */
	boot_cpu_hotplug_init();

	build_all_zonelists(NULL);
	page_alloc_init();
```

這一講只要看懂「大方向」，細節之後的講次會一個一個拆：

| 行 | 在做什麼 | 跟本講的關係 |
|---|---|---|
| 940 函式宣告 | `asmlinkage`：這個函式是**從組合語言呼叫**的。`__init`：開機完這段程式碼就可以丟掉、釋放記憶體 | 開機的起點是組合語言（在 `arch` 裡），然後才進到 C |
| 945 `set_task_stack_end_magic(&init_task)` | 在第一個 task 的 stack 底部放一個「魔術數字」，之後可以偵測 stack 有沒有溢位 | `task` 就是 process，第 3 章主角 |
| 952–953 `local_irq_disable()` | **先關掉中斷** | 開機時一切都還沒準備好，不能讓中斷打斷。中斷是第 4 章 |
| 955–958 註解 | 「中斷還是關著的。先做必要設定，再打開。」 | 印證教授說的「順序固定、不能亂」 |
| 961 `pr_notice("%s", linux_banner)` | 印出 Linux 版本橫幅。你開機時看到（或 `dmesg` 第一行）的 `Linux version 6.2.12 ...` 就是這行印的 | 版本號來自 [§7.2](#72-原始碼版本號寫在哪裡) 的 Makefile |
| 963 `setup_arch(&command_line)` | **呼叫架構相關的設定**。這個函式每個架構各寫一份（x86 的在 `arch/x86/kernel/setup.c`） | 就是 [§8.3](#83-arch把硬體差異關起來) 說的：通用程式碼呼叫 `arch` 提供的函式 |
| 968 `smp_prepare_boot_cpu()` | 註解寫「arch-specific boot-cpu hooks」，又是架構相關 | 同上 |
| 971–972 `build_all_zonelists`、`page_alloc_init` | 開始設定**記憶體配置** | `mm` 目錄，第 6 章 |

`start_kernel()` 很長（後面還有幾百行），這裡只節錄開頭。

### 8.7 Block device vs Character device

`[34:02]` Linux 支援兩種裝置：

| | Block device（區塊裝置） | Character device（字元裝置） |
|---|---|---|
| **一次讀寫** | 一整個 **block**（教授說 512 bytes，半 KB） | **一個字元**（一兩個 byte） |
| **例子** | 硬碟、隨身碟 | 鍵盤、滑鼠 |
| **難度** | 不容易處理 → 所以有 `block` 這個**通用層** | 很容易處理 |

#### 【Q&A】Character device 是什麼？

`[36:32]`

**學生問**：character device 是什麼？跟 block device 一樣嗎？（⚠️ 逐字稿只有「what is character... character devices the block Devices」）

**教授答**：**不一樣**。block device 一次讀或寫一整塊 512 bytes，像硬碟。character device 像滑鼠、鍵盤，一次讀寫一兩個 byte。按一個鍵就是一個 ASCII 字元，就是 1 byte。

#### 【原始碼】512 bytes 在哪裡

```c
// include/linux/blk_types.h:24-34
 * The basic unit of block I/O is a sector. It is used in a number of contexts
 * in Linux (blk, bio, genhd). The size of one sector is 512 = 2**9
 * bytes. Variables of type sector_t represent an offset or size that is a
 * multiple of 512 bytes. Hence these two constants.
 */
#ifndef SECTOR_SHIFT
#define SECTOR_SHIFT 9
#endif
#ifndef SECTOR_SIZE
#define SECTOR_SIZE (1 << SECTOR_SHIFT)
#endif
```

| 行 | 意思 |
|---|---|
| 24–27 註解 | 「block I/O 的**基本單位是 sector（磁區）**，大小是 512 = 2⁹ bytes。」 |
| 30 `SECTOR_SHIFT 9` | 用「左移幾位」表示大小。2⁹ = 512 |
| 33 `SECTOR_SIZE (1 << SECTOR_SHIFT)` | `1 << 9` = 512。用位移而不是直接寫 512，是因為 kernel 常要做「位元組數 ÷ 512」，寫成 `>> SECTOR_SHIFT` 比除法快 |
| `#ifndef ... #endif` | 如果別的地方已經定義過就不重複定義 |

名詞對照：教授說「block = 512 bytes」；原始碼裡 512 bytes 叫 **sector**。block 和 sector 的關係，第 7 章（I/O 系統）會再講。

### 8.8 【Q&A】sound 不是 driver 嗎？

`[31:31]`

**學生問**：sound 的一部分應該在 device driver 裡吧？（⚠️ 逐字稿只有「sir it Dev sound part of it will be in device drivers」）

**教授答**：對，但 Linux 有一套**通用的聲音架構**。

- 你需要一個通用的「匯流排式」架構，把聲音和 **codec** 放進去。
- 聲音很大一部分是 codec：WAV 是原始位元檔，但還有 MP3 等格式；影片也一樣，有 MP4。**codec 的部分放在 `sound` 這裡**，不在 `drivers`。

---

## 9. 整門課的地圖

### 9.1 八章內容

`[38:34]`（課本 §1.3、Fig 1.6）

| 章 | 主題 | 教授的重點 |
|---|---|---|
| **2** | 計算機結構基礎 | 「從 OS 角度看的計組」：一般計組課會教，但對 OS 很重要所以再強調 |
| **3** | **Process** | 「最重要的一章」。process = 程式的執行實例，像開三個 PowerPoint 就是三個 process |
| **4** | 跟 OS 溝通 | system call、interrupt、signal（見下方） |
| **5** | 同步與排程 | 多個 process 搶同一個資源 → 競爭、公平性、**deadlock**（死結：我拿了一個資源，又想拿另一個）。排程分**一般** process 和**即時（real-time）** process，演算法很不一樣 |
| **6** | 記憶體系統 | 【口頭】「記憶體是**最**關鍵的資源。我刻意不說『最關鍵的資源**之一**』。」 |
| **7** | I/O 系統與 device driver，含檔案系統 | 業界 Linux 工程師最常見的工作就是**寫 driver** |
| **8** | 虛擬化與安全 | 雲端運算：你的資料交給第三方處理，信任和隱私就是問題 |

【口頭】關於寫 driver 的工作 `[42:02]`：driver 工程師還是要懂 kernel 內部。這份工作很有成就感、薪水好、技術含量高，因為**全世界都很缺會寫 driver 的人**，「不只印度，是全世界」。

#### 跟 OS 溝通：放火燒房子才能叫消防隊

`[39:02]` 跟 OS 溝通**一點都不簡單**——**不能用普通的函式呼叫**。

> 【口頭】「唯一能叫消防隊來的方法，就是**放火燒你的房子**。」

意思是：你想跟 OS 說話，就要**產生一個中斷（interrupt）**——一個**假的**中斷。這樣 OS 才會進來。

`[39:31]` **最重要的觀念**：OS **平常是不在跑的**。它就像其他程式一樣，只有**被叫醒**才跑。叫醒它的方式：

1. 你產生一個**假中斷**（system call）
2. 發生**真中斷**（例如按了鍵盤）
3. 你做了不該做的事（例如執行非法指令）

`[40:00]` 三種訊息的**方向**：

```
                  system call
   Process  ─────────────────────►
                                      OS
   Hardware ─────────────────────►
                  interrupt

   Process  ◄─────────────────────  OS
                  signal（反向的 system call）
```

| 機制 | 方向 | 例子 |
|---|---|---|
| **System call** | process → OS | 程式要讀檔 |
| **Interrupt** | 硬體 → OS | 按了一個鍵、點了滑鼠 |
| **Signal** | OS → process | 你點了選單按鈕：OS 先知道，但你的程式也得知道，才能彈出選單。OS 就用 signal 告訴它 |

【口頭】signal 可以想成「**反向的 system call**」。

### 9.2 三個附錄

`[43:30]`

| 附錄 | 內容 | 為什麼要看 |
|---|---|---|
| **A** | x86 組合語言（64 位元） | 課程會出現很多 x86 組語片段；OS 很多關鍵程式還是用組語寫，為了速度和效率 |
| **B** | 編譯、連結、載入 | 100 個 `.c` 檔怎麼變成一個執行檔？`.c` 和 `.h` 有什麼不同？header 是什麼、為什麼需要？ |
| **C** | 資料結構 | 課本上學的資料結構太簡單，真實世界的要加很多**工程調整**，而且常是**複合式**的（例如三種資料結構合在一起） |

`[30:01]` 教授在講 `include` 目錄時也順便解釋了 header：**`.h` 檔只放函式的簽名（signature），不放程式碼**，目的是讓編譯能成功。kernel 有很多子系統要互相溝通，所以 header 很多。

【課本補充】附錄 C 會介紹的資料結構：通用 linked list、red-black tree、B / B+ tree、**maple tree**、radix tree、augmented tree。

`[37:30]` 【口頭】教授說真正的 OS 用的資料結構「**有一點 linked list、一點 hash table、一點 tree，全部混在一起**」。這門課的目標是用 Linux 當**載具**教通用 OS 觀念：觀念是通用的，但放在 Linux 的背景下教，讓你知道真正實作時是什麼樣子——而不是只會玩具 OS。

### 9.3 收尾：OS 的四大塊

`[45:30]` 最後一張投影片，OS 有四個部分：

| 部分 | 內容 |
|---|---|
| **基礎（架構面）** | OS 是硬體和 process 之間的介面，所以要懂核心的特殊功能和暫存器。【口頭】每份處理器規格書都有 **Part 1**（給一般程式設計師）和 **Part 2**（給 OS 設計者：特權暫存器、特權指令、中斷規格）。一般計組課會漏掉的，這裡變重要了 |
| **Process** | process 像人，會起爭執，要管理和排序 |
| **記憶體、虛擬化、安全** | 這三個綁在一起，因為「OS 上跑 OS」其實是**記憶體問題**：guest OS 以為整塊記憶體和所有裝置都是它的，host OS 也這麼以為，抽象層打架，所以要在虛擬記憶體上**再加一層虛擬**。安全：不能讓一個 guest OS 偷看另一個 guest 的資料（你的信件、保單、財務、醫療資料） |
| **溝通** | 跟外部裝置和程式溝通不簡單，因為 OS 是**另一個程式**，不能直接函式呼叫 |

`[48:30]` **本講最重要的一句話**（教授原話整理）：

> **OS 平常不在跑。它只在被叫醒時才跑，而叫醒它有非常特定的方法。工作做完，它就離開畫面。**

`[49:01]` 下一講進入第 2 章：基礎。

---

## 10. 一頁速查表

### 10.1 核心觀念

| 觀念 | 一句話 |
|---|---|
| **OS** | 有特權的程式，能控制硬體和其他程式（班長） |
| **為什麼需要 OS** | 硬體共用 → 會衝突 → 要有管理者 |
| **仲裁** | 大家都想用，誰先？（CPU、印表機） |
| **控管存取** | 你有沒有資格用？（權限、安全） |
| **Process** | 程式的執行實例 |
| **Device driver** | 把通用 OS 呼叫翻成硬體專用指令的程式 |
| **OS 何時跑** | 只有被叫醒時：system call、interrupt、例外 |
| **System call / Interrupt / Signal** | process→OS / 硬體→OS / OS→process |

### 10.2 Linux 與 GPL

| 項目 | 內容 |
|---|---|
| 起源 | Linus Torvalds，芬蘭，從 Minix 出發（教授：1991；課本：1990 開始、1992 首版 GPL 發布） |
| 設計 | 介面跟 Unix 一樣，程式碼全新 |
| 授權 | `GPL-2.0 WITH Linux-syscall-note`（只限第 2 版） |
| GPL 規則 | 可以用、改、散布；衍生作品也要 GPL；賣了就要公開修改 |
| syscall note | 透過 system call 用 kernel 的程式**不算**衍生作品 |
| 為什麼成功 | 公司各出一點人，合養一個共有的 OS |
| BSD | 授權較寬鬆，可以改了拿去賣 |
| 2003 | kernel 2.6，Linux 被認真看待的分水嶺 |

### 10.3 版本號

| 格式 | `6.2.12-rc<num>` |
|---|---|
| 6 | major（Makefile 的 `VERSION`） |
| 2 | minor（Makefile 的 `PATCHLEVEL`） |
| 12 | patch，約兩週一次（Makefile 的 `SUBLEVEL`） |
| -rc | release candidate，測試版 |
| LTS | 發行版的事，不是 kernel 的事 |

### 10.4 Kernel 目錄

| 目錄 | 內容 | v6.2.12 實測佔比 |
|---|---|---:|
| `drivers` | 裝置驅動程式 | 68.6% |
| `arch` | 架構相關、組合語言 | 8.0% |
| `fs` | 檔案系統 | 4.6% |
| `sound` | 聲音、codec | 4.4% |
| `net` | 網路協定 | 3.8% |
| `include` | header | 3.7% |
| `kernel` | 排程、同步、時間（**本課重點**） | 1.4% |
| `mm` | 記憶體管理 | 0.6% |
| `block` | block 裝置通用層 | 0.2% |
| `init` | 開機（`start_kernel()`） | <0.1% |
| `virt` | 虛擬化 | <0.1% |

### 10.5 本講看過的原始碼位置

| 檔案:行號 | 看什麼 |
|---|---|
| `COPYING:1-12` | kernel 授權 |
| `LICENSES/exceptions/Linux-syscall-note` | system call 例外條款 |
| `Makefile:1-6` | 版本號 |
| `include/asm-generic/int-ll64.h:16-23` | `u8`…`u64` 定義 |
| `include/uapi/asm-generic/int-ll64.h:29-35` | `__u64` = `unsigned long long` |
| `include/uapi/asm-generic/int-l64.h:29-30` | `__u64` = `unsigned long`（另一種選擇） |
| `arch/ia64/include/uapi/asm/types.h:18-20` | 架構自己選型別 |
| `include/uapi/asm-generic/types.h:4-7` | 「int-ll64 is used everywhere now」 |
| `init/main.c:940` | `start_kernel()`，開機的第一個 C 函式 |
| `include/linux/blk_types.h:24-34` | sector = 512 bytes |

---

## 11. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| **特權、班長比喻**、規格書的 Part 2 | **第 2 講**：特權暫存器、ring level（Ch 2 Part I） |
| **timer interrupt 與排程** | **第 3 講**：timer interrupt 與 x86（Ch 2 Part II） |
| **附錄 B：編譯、連結、載入** | **第 4 講**：靜態與動態連結（Ch 2 Part III） |
| **記憶體是最關鍵的資源**、虛擬記憶體 | **第 5–6 講**：虛擬記憶體、segmentation 與 I/O（Ch 2 Part IV–V） |
| **Process**、`init_task`、`start_kernel()` 裡的 task | **第 7–13 講**：process 狀態、kernel stack、`mm_struct` 與 maple tree、PID 與 namespace、process 建立、context switch（Ch 3） |
| **system call、interrupt、signal**；`start_kernel()` 先關中斷 | **第 14–17 講**：library call 與 system call、x86 中斷架構、softirq / threaded IRQ / work queue、signal（Ch 4） |
| **仲裁、deadlock、排程** | **第 18–28 講**：多執行緒、memory model、鎖、並行佇列、RCU、排程、多處理器排程、Linux 排程、即時排程（Ch 5） |
| **`mm` 目錄**、`page_alloc_init()` | **第 29–33 講**：頁面置換、kernel 記憶體架構、reverse mapping、MGLRU、kernel 記憶體配置（Ch 6） |
| **block / character device、sector**、`drivers`、`fs` | 課本第 7 章（I/O 系統）。⚠️ 目前播放清單只到第 33 講（Ch 6），第 7 章對應講次**待補** |
| **虛擬化、Docker、`virt`** | 課本第 8 章。對應講次**待補** |
| **即時排程** | 另一門課 **Real Time Systems**（Rajib Mall）會深入講 |

---

## 附：本講一句話總結

> **OS 是一個有特權的程式，因為硬體是共用的、會起衝突，所以需要它來仲裁和控管存取；它平常不在跑，只有被 system call、interrupt 或例外叫醒時才跑。這門課用 Linux（v6.2.12）當載具來教通用的 OS 觀念——Linux 能贏，靠的是 GPL 讓大家合養一個共有的 OS。**

---

*筆記依據 Smruti R. Sarangi「Operating Systems: A Linux Kernel-Oriented Approach」Lecture 1 錄影逐字稿（YouTube 自動字幕，49:20）、課本 *Operating Systems: A Linux Kernel-Oriented Approach*（Sarangi, v0.96）第 1 章，以及 Linux kernel v6.2.12 原始碼整理。*
