# Operating Systems: A Linux Kernel-Oriented Approach 第十講完整筆記：PID 與 Namespace（Chapter 3, Part IV）

> **課程**：Operating Systems: A Linux Kernel-Oriented Approach — Lecture 10（Chapter 3, Part IV）
> **講者**：Smruti R. Sarangi
> **來源**：YouTube 錄影逐字稿（1:10:04，自動字幕）＋ 課本 *Operating Systems: A Linux Kernel-Oriented Approach*（Sarangi, v0.96）§3.1.11–3.1.13（p.83–90）與附錄 C.5–C.6（radix tree、augmented tree）＋ Linux kernel **v6.2.12** 原始碼（`os/linux-src/`）
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
- [1. 兩個資料結構](#1-兩個資料結構)
  - [1.1 Radix tree](#11-radix-tree)
  - [1.2 Augmented tree：在位元向量裡找下一個 1](#12-augmented-tree在位元向量裡找下一個-1)
  - [1.3 優化一：葉子存一個 word，不存一個位元](#13-優化一葉子存一個-word不存一個位元)
  - [1.4 優化二：m 元樹，父節點存子樹狀態的位元向量](#14-優化二m-元樹父節點存子樹狀態的位元向量)
  - [1.5 任何樹都能被 augment](#15-任何樹都能被-augment)
  - [1.6 【原始碼】`bsf` 指令](#16-原始碼bsf-指令)
- [2. Process ID](#2-process-id)
  - [2.1 PID 是 process 的身分證號碼](#21-pid-是-process-的身分證號碼)
  - [2.2 四層分組：process、thread group、process group、session](#22-四層分組processthread-groupprocess-groupsession)
  - [2.3 另一種分組：namespace](#23-另一種分組namespace)
- [3. Namespace 與容器](#3-namespace-與容器)
  - [3.1 PID 只在自己的 namespace 裡有意義](#31-pid-只在自己的-namespace-裡有意義)
  - [3.2 容器：可以搬家的迷你虛擬機](#32-容器可以搬家的迷你虛擬機)
  - [3.3 為什麼 PID 要跟著 namespace 走](#33-為什麼-pid-要跟著-namespace-走)
  - [3.4 `pid_t` vs `struct pid`：兩個都叫 pid](#34-pid_t-vs-struct-pid兩個都叫-pid)
- [4. `struct pid_namespace`](#4-struct-pid_namespace)
  - [4.1 四個主要欄位](#41-四個主要欄位)
  - [4.2 Pool：kernel 不用 malloc](#42-poolkernel-不用-malloc)
  - [4.3 【原始碼】`struct pid_namespace`](#43-原始碼struct-pid_namespace)
- [5. `struct pid`](#5-struct-pid)
  - [5.1 欄位](#51-欄位)
  - [5.2 一個 process 在每層 namespace 都有一個號碼](#52-一個-process-在每層-namespace-都有一個號碼)
  - [5.3 【原始碼】`struct upid` 與 `struct pid`](#53-原始碼struct-upid-與-struct-pid)
- [6. 配置一個 PID：`alloc_pid()`](#6-配置一個-pidalloc_pid)
  - [6.1 流程](#61-流程)
  - [6.2 【原始碼】`alloc_pid()`](#62-原始碼alloc_pid)
- [7. IDR tree：radix tree + augmented tree](#7-idr-treeradix-tree--augmented-tree)
  - [7.1 要回答的問題](#71-要回答的問題)
  - [7.2 為什麼用 radix tree 而不是 hash table](#72-為什麼用-radix-tree-而不是-hash-table)
  - [7.3 xa_node：64 個 slot + 64 位元的 mark](#73-xa_node64-個-slot--64-位元的-mark)
  - [7.4 例子：PID 0、1、3、4、7](#74-例子pid-01347)
  - [7.5 【原始碼】`struct idr` 與 `struct xa_node`](#75-原始碼struct-idr-與-struct-xa_node)
- [8. 檔案系統、I/O 與除錯欄位](#8-檔案系統io-與除錯欄位)
  - [8.1 檔案與開啟](#81-檔案與開啟)
  - [8.2 Signal handler](#82-signal-handler)
  - [8.3 Block device 與 character device](#83-block-device-與-character-device)
  - [8.4 ptrace：process 的監護人](#84-ptraceprocess-的監護人)
  - [8.5 【原始碼】`task_struct` 裡的這些欄位](#85-原始碼task_struct-裡的這些欄位)
- [9. 【實測】PID namespace 與 PID 配置](#9-實測pid-namespace-與-pid-配置)
- [10. 一頁速查表](#10-一頁速查表)
- [11. 與後續講次的連結](#11-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 這堂課要做什麼

`[00:33]` 從**單一** process 轉到**一群** process：process ID 和管理一群 process 的方法。

| # | 主題 | 對應課本 |
|---|---|---|
| 1 | 先繞道看兩個資料結構：**radix tree**、**augmented tree** | 附錄 C.5–C.6 |
| 2 | **PID**、四層分組 | §3.1.11 |
| 3 | **Namespace** 與容器 | §3.1.12 |
| 4 | `struct pid_namespace`、`struct pid`、`alloc_pid()` | §3.1.12 |
| 5 | **IDR tree**：怎麼找下一個可用的 PID | §3.1.12 |
| 6 | `task_struct` 裡的檔案、I/O、ptrace 欄位 | §3.1.13 |

`[00:33]` 「你會看到這兩個資料結構被**大量使用**，所以先快速看一下很重要。」

---

## 1. 兩個資料結構

### 1.1 Radix tree

`[01:00]`（課本 Fig C.2）要存一堆 **key**，而且這些 key **有關聯**：例如英文單字 travel、truck、tram、trust、trick、tread、tractor、try、trip……都以 **tr** 開頭。

```
                         tr
        ┌────┬────┬──────┼──────┬──────┐
        a    u    i      y      ead
       ╱│╲   │╲   │╲     │
   vel ctor m ck st ck m  st*
```

（`*` 表示「到這裡是一個完整的字」，例如 try 和 tryst。）

`[01:30]` 做法：

- 共同前綴 **tr** 放在根。
- 後面每個字母（或一段字母）建一個內部節點。
- 例如 **tread**：t-r → e-a-d。
- 加一個新字 **treat**：`ead` 要拆成 `ea` → 再分 `d`（tread）和 `t`（treat）。
- **try** 和 **tryst**：t-r → y → st。所以**每個節點要多一個位元**：「走到這個節點為止，是不是一個完整的字？」（try 是，tr 不是。）

`[02:31]` 查詢：**tractor** 在不在？tr → a → ctor ✓。**trampoline** 在不在？tr → a → m，但下面沒有 poline ✗。

> 「**複雜度就是字母數**。hash table 是另一種做法，但有它自己的問題；這種情況 radix tree **快非常多**。」

`[03:30]` 為什麼跟 PID 有關？**PID 是 0、1、2、3……這種數字，大部分情況下它們的高位元（MSB）都一樣**——共享前綴。所以用 radix tree 存 PID 是很好的主意。

【課本補充】課本補充（p.553–554）：

- radix tree 的查詢時間**只跟 key 的長度有關，跟 key 的數量無關**。
- **Trie（字典樹）**：每條邊一個字母。radix tree 把**只有一個子節點**的節點壓縮起來，一條邊可以多個字母（例如 `tra`）。
- **Patricia trie**：字母只有 0 和 1 的 radix tree（二進位）。只有兩個不同的 key 時，不管它們差幾個位元，Patricia trie 都只有 3 個節點：根存共同前綴，兩個子節點存不同的後綴。Patricia = *Practical Algorithm To Retrieve Information Coded In Alphanumeric*。

### 1.2 Augmented tree：在位元向量裡找下一個 1

`[04:02]` 第二個資料結構，在 OS（特別是 Linux kernel）裡**很有價值**。

> 【口頭】「它有很多名字。我們用 **augmented tree（增強樹）**，但要承認**這不是標準名稱**。同一個想法出現在很多不同的資料結構裡，沒有公認的名字，這只是這門課的**工作名稱**。而且**任何樹**都可以用同一招 augment。」

`[04:30]` **問題**：

- 一個**很長的位元向量**，n 個位元，每個是 0 或 1。
- 從某個位置開始，**往某個方向掃，找到第一個 1** 的位置。

`[05:30]` 直接掃是 **O(n)**——一百萬個位元的話，「**超級**沒效率」。要建一棵樹，變成 **O(log n)**。

（課本 Fig C.3）

```
                          1                        ← 根：整個範圍有沒有 1？
              ┌───────────┴───────────┐
              1                       1
        ┌─────┴─────┐           ┌─────┴─────┐
        1           1           1           0      ← 這一段有沒有 1？
      ┌─┴─┐       ┌─┴─┐       ┌─┴─┐       ┌─┴─┐
      1   0       1   1       1   0       0   0    ← 葉子：位元向量本身
```

`[06:00]` 建法：

- 兩個相鄰位置（例如 0 和 1）建一個父節點，存：「**我這棵子樹（涵蓋的範圍）裡有沒有 1？**」
- 再上一層看更大的範圍——但**不用看範圍裡的每個位元**，只要看兩個子節點，取**邏輯 OR**。
- 有一段「0、0」的父節點存 0：「整個範圍都沒有 1。」

`[07:32]` **找下一個 1**：

1. 從起點開始，目前位置是 0（是 1 就直接拿了）。
2. 問父節點：父節點說 1 → 看父節點的**另一個子節點**，往下找。
3. 父節點說 0 → 代表我來的這棵子樹沒有 1 → 問**祖父節點**；祖父說 1 → 去**另一邊**的子樹找。
4. 最多**往上走一次、往下走一次**：**O(log n)**。

`[09:00]` **修改**也是 O(log n)，像 heap 一樣往根走、翻位元：

| 修改 | 做法 |
|---|---|
| 0 → 1 | 往上走，路上的 0 全部改成 1 |
| 1 → 0 | 往上走，每到一個節點看**其他子節點**：都是 0 → 這個節點也改成 0，繼續往上；有一個是 1 → 停（父節點還是 1） |

【課本補充】課本 C.6 舉的例子：系統有一百萬個 frame，用一百萬位元的向量表示，第 i 位是 1 代表第 i 個 frame **空著**。常見操作就是「找第一個沒被配置的 frame」——正是這個問題。

### 1.3 優化一：葉子存一個 word，不存一個位元

`[10:02]` 讓 augmented tree **實用**的兩個優化。第一個：

> 「現實中你**不會在一個位置存一個位元**，因為任何機器最小的存取單位是一個 **word**，可能是 64 位元。暫存器存 64 位元就是這樣。」

- 每個葉子存 **64 位元**。
- 父節點（例如涵蓋兩個葉子）的範圍就是 **128 位元**，它只存：「這 128 個位元裡有沒有任何一個 1？」

`[12:00]` x86 的設計者**猜到**你會做這種事，提供了一個**快速硬體指令**：

> 給一個 64 位元暫存器，**直接找出第一個 1 的位置**。例如「第一個 1 在第 27 位」，或者「全部是 0」。

### 1.4 優化二：m 元樹，父節點存子樹狀態的位元向量

`[12:30]` 第二個優化：

- 從漸近複雜度看，父節點只存「整棵子樹有沒有 1」就夠了。
- 但**沒效率**：走到一個內部節點、知道底下有 1，還要再去問**每個子節點**哪一個有 1。
- 而且**二元樹**不是有效率的組織，要 **m 元樹**。

`[13:32]` 改成：**父節點自己存一個位元向量**，第 i 位代表「**第 i 棵子樹**有沒有 1」。

```
  父節點：  [ 0  1  0  1  0 ]     ← 5 個子樹各一位：0 = 整棵沒有 1、1 = 至少一個 1
             │  │  │  │  │
             ▼  ▼  ▼  ▼  ▼
           子樹 子樹 子樹 子樹 子樹
```

`[14:33]` 從根往下走時：

1. 到一個內部節點，看它的向量，例如 `0 1 1 0 ...`。
2. 找**最左邊的 1**——這就是**第一棵有 1 的子樹**。
3. 這又是**同一個問題的縮小版**：在一個小向量裡找第一個 1！
4. 如果用 **64 元樹**，這個向量剛好 **64 位元 = 一個暫存器**，用 x86 的 **ffs**（find first set）指令一下就找到。例如在第 3 位，就去第 3 棵子樹。

`[16:31]` Linux kernel 的設計者**充分利用**這個模式和這個硬體指令，做了一個結構叫 **xarray**。

`[17:03]` 【口頭】教授的總結：

> 「你看**快取**怎麼進來了（所以不用二元樹而用 m 元樹）、**暫存器**怎麼進來了、**特殊指令**怎麼進來了、**理論計算機科學**怎麼進來了（全部在對數時間內完成）。**時間複雜度從來不是全貌**。對數時間當然比線性好，但實際上**要走幾步**？這需要更多**工程**。」

`[18:01]` 「設計一個**工程用**的資料結構，比我們以為的資料結構設計**難得多**。」

### 1.5 任何樹都能被 augment

`[18:31]` 任何樹都可以加上這種資訊。例如一棵 radix tree：

- 有一個大的位元向量。
- 每棵子樹對應位元向量的某個範圍。
- 加上 augmented tree 的資訊 → 變成 **augmented radix tree**：搜尋照 key 走，同時每棵子樹也知道「我的範圍裡有沒有 1」。

> 「我們會看到怎麼 augment 一棵 radix tree 來**配置 PID**，那會很有趣。」

### 1.6 【原始碼】`bsf` 指令

教授說的「找第一個 1 的硬體指令」，在 kernel 裡：

```c
// arch/x86/include/asm/bitops.h:250-256
static __always_inline unsigned long variable__ffs(unsigned long word)
{
	asm("rep; bsf %1,%0"
		: "=r" (word)
		: "rm" (word));
	return word;
}
```

| 部分 | 意思 |
|---|---|
| `__ffs` | **f**ind **f**irst **s**et：找第一個是 1 的位元 |
| **`bsf`** | **B**it **S**can **F**orward：x86 指令，從最低位往上掃，回傳第一個 1 的位置 |
| `rep; bsf` | 加了 `rep` 前綴。在支援的新 CPU 上，`rep bsf` 會被解讀成 **`tzcnt`**（count trailing zeros），一樣結果、更快；舊 CPU 會忽略 `rep`，當成一般 `bsf` |
| `"rm" (word)` | 輸入可以是暫存器或記憶體 |

一條指令取代「迴圈 64 次、每次位移再檢查」。課本說 `bsf` 只要 **2–3 個 cycle**。

---

## 2. Process ID

### 2.1 PID 是 process 的身分證號碼

`[20:02]` 系統裡**每個 process** 都有一個**唯一**的 **process ID（PID）**。跑 `ps -ef` 或 `ps -la` 就看得到。

- 所有 system call、任何跟 kernel 的互動，都**只要給這個號碼**。
- 【口頭】「就像學生的學號；在印度就像 **Aadhaar 號碼**（國民身分號碼），足以識別你。」

`[20:31]` 已經看過的：

- process 組成**階層**（樹）：每個 process 有父 process，最上面有一個全系統的根。
- process 可以組成 **thread group**：共享資源的 thread，有一個**領袖**，領袖的 PID 就是整群的 **thread group ID（tgid）**。

【課本補充】`pid_t` 在大多數架構上是 `unsigned int`（教授說 32 位元無號整數）。`/proc/sys/kernel/pid_max` 存系統最多能有幾個 PID，**預設 32,768**。

### 2.2 四層分組：process、thread group、process group、session

`[21:00]` 教授在白板上畫的階層：

```
  ┌────────────────────────── Session（一個 shell 開的所有東西）────────────────────┐
  │  ┌──────────── Process group（一個指令產生的）────────────┐                      │
  │  │  ┌── Thread group ──┐                                  │                      │
  │  │  │ thread  thread   │   獨立 process   獨立 process     │   另一個 process group│
  │  │  └──────────────────┘                                  │                      │
  │  └────────────────────────────────────────────────────────┘                      │
  └──────────────────────────────────────────────────────────────────────────────────┘
```

| 層 | 是什麼 | 例子 |
|---|---|---|
| **Process** | Linux 裡**每個執行中的實體**都是 process。連 thread 也是——只是光譜一端、跟兄弟姊妹 thread 共享很多資源的特殊 process | |
| **Thread group** | 一群 thread，通常叫**多執行緒 process**。「技術上不是單一 process，是一群 process」 | |
| **Process group** | **一個命令列指令**產生出來的所有東西 | `[22:01]` 從命令列跑 Chrome 或 Firefox：它產生一堆 process、thread group；開一個網站又產生一堆。全部來自同一個源頭 |
| **Session** | **一個 shell** 開出來的所有 process group | `[23:30]` 開一個命令視窗（終端機），裡面處理指令的程式是 **shell**。從 shell 跑 5 個瀏覽器實例等等 → shell 建立一個 session |

`[23:03]` process group 的用處：

> 「某個網站當掉了、或你瀏覽完了，回到命令列按 **Ctrl+C**——**全部消失**。一個指令產生的所有東西，按 Ctrl+C 全部被終止。這是我們想要的功能。按 Ctrl+Z（送 `SIGTSTP`）整群被暫停。」

`[24:00]` session 的用處：關掉 shell，如果啟動時說「shell 關掉程式就跟著死」，它們**全部被終止**——session 結束。

【課本補充】Point 3.1.5：process 可以屬於一個 thread group，thread group 的 ID 是領袖的 PID。一群 process 和 thread group 叫 **process group**，可以從 shell 一次送 `SIGINT`（Ctrl+C）和 `SIGTSTP`（Ctrl+Z）。一群 process group 組成一個 **session**，例如同一個**登入 shell** 啟動的所有 process。

### 2.3 另一種分組：namespace

`[24:30]` 組織 process 有很多方式：

1. 上面那種階層（process → thread group → process group → session）。
2. **父子**關係（process 樹）。
3. **Namespace（命名空間）**。

`[25:02]` namespace 也組成**一棵樹**：有一個**根 namespace**，下面有子 namespace，子還可以有子……

`[25:30]` 「你可以把 session 想成 namespace 的**子集**：namespace 比 session 大，是包在外面的另一個大圈。」

namespace 的目的：讓我們能建立 **容器（container）**——「**現代雲端運算的驅動力**」。

---

## 3. Namespace 與容器

### 3.1 PID 只在自己的 namespace 裡有意義

`[26:01]` 每個 process 有一個唯一的整數 ID：**32 位元無號整數**，型別是 **`pid_t`**（`_t` 代表 type）。

`[26:31]` 問題：process 有很多種組織方式。

> **PID 只在 process 自己的 namespace 裡有意義。**

- namespace 是「**process 名字（ID）的空間**」。
- 在一個隔離的 process 環境裡，PID 有意義；**一出去就沒意義了**。

### 3.2 容器：可以搬家的迷你虛擬機

`[27:00]` 為什麼要這樣設計？

> 「大家花很多力氣下載軟體、安裝、設定。之後機器當了就麻煩了。所以把軟體裝在 **Docker 容器**裡比較好——它像一個**迷你虛擬機**，有自己專屬的**檔案系統**和**虛擬網路裝置**。機器當了，把容器拿到別台機器上跑，**整套照樣能動**。」

`[27:30]` 容器最棒的地方：**可以暫停，然後在別的地方恢復**。

`[31:00]` 【口頭】教授的例子：

> 「你在訓練一個神經網路，要訓練一週。6 天 12 小時的時候，有人說：『抱歉，這台機器**馬上**要關機。』一小時內你要做完該做的事。你會失去那麼多工作嗎？**不會**。你可以**暫停容器**（Docker、LXC、Podman 都行）——等於暫停容器裡所有 process。用一個叫 **CRIU** 的軟體收集它的狀態，放進一個映像檔，傳到別台機器，**從同一點重新開始**。就像一次 context switch：容器裡的 process **根本不知道**它們在一台機器上被暫停、在另一台被恢復。你的工作會完成。」

`[32:00]` 虛擬機也能做到，但**容器是輕量級**的做法。當然需要 kernel 支援，其中一個就是 **namespace**——「namespace 是 process 的**隔離視角**」。

`[34:03]` 容器的好處：

- 帶著自己的**虛擬檔案系統**走，不用存取原生檔案系統（除非需要）。
- 不用重新安裝軟體、找 library、設路徑——「交作業前那些辛苦的事，全部不用」。
- 不只能離線搬，**正在跑的**也能搬。

`[34:32]` 「這是現代雲端運算的驅動力。**虛擬機**——一整個機器狀態和 OS——**非常重**；**容器**輕量得多。」

【課本補充】課本補充（p.84–85）：

- 傳統雲端運算正被 **microservices、容器、serverless** 等新技術補充（第 8 章）。
- **虛擬機（VM）**：完整虛擬化的環境，CPU 和記憶體等所有處理器資源都被虛擬化。可以暫停、搬到新機器、重啟，但是**重量級**的。
- **容器**：輕量級，**環境沒有被虛擬化**。在一台機器裡建立一個小的隔離環境，是「迷你虛擬機」。容器裡的 process 看到一個隔離的環境：**自己的一組 process、網路堆疊和檔案系統**。也提供很強的**安全保證**。
- 容器在它的 process 和系統其他部分之間建立**屏障**：使用者可以安全地在遠端系統上執行 process，使用者的程式碼和遠端系統**不用完全互信**——容器確保 process 不能對遠端系統造成太大傷害，遠端系統也不能過度干預 process 的執行。

### 3.3 為什麼 PID 要跟著 namespace 走

`[32:32]` 關鍵：

> 「假設我是容器裡的一個 process，正在跟 **PID 5** 的 process 互動。如果搬到另一台機器，對方的 PID **還是要是 5**，不能變。變了，我**正在跑的程式碼就會壞**。」

所以搬容器時，**容器的 namespace 也一起搬**：「我是 7，到另一台機器我**還是 7**。」

`[33:00]` 【口頭】「想成**護照號碼**：護照號碼在全球都有意義，因為有全球標準，這個國家認、那個國家也認。」

`[29:01]` 另一個比喻（Aadhaar 號碼）：

> 「在印度，你有一個唯一的號碼很有意義；但去別的國家，那個號碼**就失去價值**。process 也一樣：在這個 process 社會裡有意義，去別處就沒價值。但我身上有些東西是**我固有的**，不會失去價值——所以有一個叫 **`struct pid`** 的結構。」

【課本補充】Point 3.1.4：**每個 process 在自己的 namespace 看得到，在所有祖先 namespace 也看得到。任何 process 在子 namespace 都看不到。**

課本也說：可以讓系統管理員只給父 namespace 一組資源，父 namespace 再分給子 namespace——**細粒度的資源管理和追蹤**。

### 3.4 `pid_t` vs `struct pid`：兩個都叫 pid

`[27:30]`、`[29:30]` 另一個問題：

- process 結束了，PID 應該**回收**再用。
- 但如果 PID 被回收了，**父 process 想檢查**怎麼辦？
- 說「父 process 檢查之前不准回收」——那可能會有**太多死掉的 zombie**。
- 所以 process 的資料要存在一個**不依賴號碼**的地方，因為號碼只在某些地方有意義。

`[29:30]` 這就是**容易搞混**的地方：

| 名稱 | 型別 | 是什麼 | 跟 namespace 的關係 |
|---|---|---|---|
| **pid（PID 號碼）** | `pid_t` | 一個**數字** | 只在某個 namespace 裡有意義 |
| **struct pid** | `struct pid` | 一個**結構**，存 process 的很多額外資料 | **不依賴 namespace** |

> 【口頭】「process 的號碼叫 pid，跟它關聯的結構**也叫** pid——這部分很令人困惑。我們說 **PID 號碼**指數字，說 **struct pid** 指結構，讓意思清楚。」

【課本補充】Point 3.1.3：「不幸地，Linux 裡兩個不同的東西**共用同一個名字** pid。我們**別無選擇**，只能從上下文推斷。」課本補充：`struct pid` 在 process 終止、變成 zombie 之後，**資訊還會保留**。

---

## 4. `struct pid_namespace`

### 4.1 四個主要欄位

`[35:30]`（課本 Listing 3.6）

| 欄位 | 說明 |
|---|---|
| **`struct idr idr`** | **IDR tree**：把 PID 號碼對應到 `struct pid`、找最小的未配置 PID、PID 號碼的記帳（§7） |
| **`struct kmem_cache *pid_cachep`** | **pool**：預先配置好的 `struct pid`（§4.2） |
| **`level`** | 這個 namespace 在樹裡的**層級**（教授說根是 1，課本說是層級；原始碼裡根是 **0**，見 §6.2） |
| **`parent`** | 指向**父 namespace**——「就像每個 process 指向它的父 process」 |

`[37:31]` 「**最重要的**：每個 namespace 都有**自己的 IDR tree**。因為 PID 號碼只在**目前的** namespace 有意義，所以每個 namespace 都要把它的一組 PID 號碼對應到 `struct pid`。」

【課本補充】課本說為什麼要這麼精密的資料結構：原則上一個 namespace 可以包含**非常大量**的 process，需要很快的結構來存和索引。

### 4.2 Pool：kernel 不用 malloc

`[36:02]` **Pool（池）**：一組**預先配置好的物件**，這裡是 `struct pid`。

為什麼需要？`[36:02]`、`[38:31]`

| 理由 | 說明 |
|---|---|
| kernel 不用 `new`、`malloc`、`free` | 那些是**使用者空間**的 library 函式，因為使用者空間的記憶體配置模式比較複雜 |
| **快** | `malloc` 很慢。要一個物件就從 pool 拿一個，用完還回去 |
| **容易追蹤** | 知道目前有幾個物件在用。**pool 空了 → 可能有記憶體洩漏** |
| 記憶體管理簡單 | 不用管什麼被 malloc 了、哪裡有空洞 |

kernel 裡的 pool 叫 **`kmem_cache`**。

`[39:01]` 【口頭】

> 「很多 pooling library，像 **Apache** 專案就有一個 pool library，使用者空間軟體也常用。建議是：寫大型軟體時，偶爾用 malloc/free 沒關係，但**為了效能，連使用者層級的軟體都應該用 pool**，它也讓記憶體問題很容易被抓到。不管 kernel 還是使用者空間都是這樣建議——只是從使用者空間到 kernel，建議就變成**嚴格的規則**，因為那裡的程式紀律更重要。」

【課本補充】課本補充（p.87）：

- pool 是**一維陣列**的同型物件。
- 初始化 pool 時，程式設計師要大概知道**同時最多會有幾個**物件，加上安全邊際。
- 估計錯了也可以自動**放大 pool**，到某個上限。
- 一個 pool **只能放一種物件**（幾乎所有情況）；物件大小一樣時才可能例外。

（`kmem_cache` 就是 **slab allocator**，第 33 講會細講。）

### 4.3 【原始碼】`struct pid_namespace`

```c
// include/linux/pid_namespace.h:19-34
struct pid_namespace {
	struct idr idr;
	struct rcu_head rcu;
	unsigned int pid_allocated;
	struct task_struct *child_reaper;
	struct kmem_cache *pid_cachep;
	unsigned int level;
	struct pid_namespace *parent;
#ifdef CONFIG_BSD_PROCESS_ACCT
	struct fs_pin *bacct;
#endif
	struct user_namespace *user_ns;
	struct ucounts *ucounts;
	int reboot;	/* group exit code if this pidns was rebooted */
	struct ns_common ns;
} __randomize_layout;
```

| 行 | 欄位 | 說明 |
|---|---|---|
| 20 | **`idr`** | **IDR tree**——第一個欄位 |
| 22 | `pid_allocated` | 已經配了幾個 PID |
| 23 | **`child_reaper`** | 課本沒列，但很重要：這個 namespace 的「**收屍者**」——就是這個 namespace 裡的 **PID 1**。第 7 講看過：父 process 死了，子 process 要交給別人；在容器裡就是交給容器的 PID 1，而不是整台機器的 `init` |
| 24 | **`pid_cachep`** | **pool** |
| 25 | **`level`** | 層級 |
| 26 | **`parent`** | 父 namespace |
| 30 | `user_ns` | 屬於哪個 **user namespace**（權限的 namespace） |

課本 Listing 3.6 列的四個欄位（`idr`、`pid_cachep`、`level`、`parent`）都在。

---

## 5. `struct pid`

### 5.1 欄位

`[39:31]` 「`struct pid` 不難，但有些有趣的特點。」

| 欄位 | 說明 |
|---|---|
| **引用計數（reference count）** | 有多少實體（其他 process 等）在用這個結構。要釋放它，計數要是 0，不然別人手上的指標會指到已經被重用的結構 |
| **`level`** | 預設層級：「預設你住在這個 namespace，層級就是 3」 |
| **`tasks[]`** | **一個 linked list 的陣列**（`hlist_head` 就是 kernel 的單向 linked list） |
| **`numbers[]`** | 每一層 namespace 的 PID 號碼（§5.2） |

`[41:05]` `tasks[]` 的重要元素：

| 元素 | 串列內容 |
|---|---|
| `tasks[PIDTYPE_TGID]` | 如果我是 thread group 領袖：我 thread group 裡的所有 `task_struct` |
| `tasks[PIDTYPE_PGID]` | 我 **process group** 裡的所有 task |
| `tasks[PIDTYPE_SID]` | 我 **session** 裡的所有 task |

`[42:00]` 有什麼用？要**殺掉整個 shell** 時，就要殺掉它產生的**所有** process——這種 linked list 就很有用。

`[42:31]` 【口頭】**取捨**：

> 「存這麼多資訊，好處是要用時**隨手可得**。壞處是建立或殺掉 process 時，**記帳開銷也很高**。就像在每個房間都放一份聯絡人清單：在廚房想查某人電話很方便——但效率和開銷之間有**取捨**。建一個新 process 或殺一個 process，都要**修這些串列**。而 kernel 裡這些串列是**平行存取**的，平行存取就要用**鎖**，不然會有 **race condition（競爭條件）**導致錯誤。」

### 5.2 一個 process 在每層 namespace 都有一個號碼

`[43:30]` 每個 `struct pid` 也是**所有祖先 namespace** 的成員：

```
   根 namespace（level 0）      這個 process 在這裡是 PID 3
         │
   子 namespace（level 1）      在這裡是 PID 4
         │
   孫 namespace（level 2）      在這裡是 PID 17   ← 預設住在這裡
```

`[44:01]` 所以對**每一層**——包括自己這層和上面所有層——都要存「**我在那個 namespace 的 PID 是多少**」。

- 一個 **`upid`** 只有兩個欄位：**PID 號碼**和**哪個 namespace**。
- `struct pid` 存一個 **upid 的陣列**：`[(3, 根), (4, 子), (17, 孫)]`。

`[44:33]` 「這要放在**情境**裡才容易懂：PID 號碼隨 namespace 變。我身為一個 `struct pid`，要讓**所有祖先 namespace** 都看得到，在那些地方我有不同的號碼，所以存一個 (號碼, namespace) 二元組的陣列。有四個組合，就存四個。」

### 5.3 【原始碼】`struct upid` 與 `struct pid`

```c
// include/linux/pid.h:9-16
enum pid_type
{
	PIDTYPE_PID,
	PIDTYPE_TGID,
	PIDTYPE_PGID,
	PIDTYPE_SID,
	PIDTYPE_MAX,
};
```

```c
// include/linux/pid.h:54-71
struct upid {
	int nr;
	struct pid_namespace *ns;
};

struct pid
{
	refcount_t count;
	unsigned int level;
	spinlock_t lock;
	/* lists of tasks that use this pid */
	struct hlist_head tasks[PIDTYPE_MAX];
	struct hlist_head inodes;
	/* wait queue for pidfd notifications */
	wait_queue_head_t wait_pidfd;
	struct rcu_head rcu;
	struct upid numbers[1];
};
```

| 欄位 | 對照 |
|---|---|
| `enum pid_type` | 四種：**PID**（單一 task）、**TGID**（thread group）、**PGID**（process group）、**SID**（session）——正好是 §2.2 的四層。`PIDTYPE_MAX` = 4，是陣列大小 |
| `upid.nr` | PID **號碼** |
| `upid.ns` | 在**哪個** namespace |
| `count` | **引用計數**。型別 `refcount_t`：專門的計數器型別，會偵測溢位 |
| `level` | 預設層級 |
| `lock` | 教授說的「平行存取要用鎖」——這裡就有一把 |
| `tasks[PIDTYPE_MAX]` | **4 個 linked list**（`hlist_head` 是單向串列頭） |
| `wait_pidfd` | **pidfd**：較新的機制，可以用一個**檔案描述子**指向 process，避免「PID 被回收後指錯人」的問題——正是 §3.4 講的困擾的另一種解法 |
| **`numbers[1]`** | **upid 陣列**。宣告寫 `[1]`，實際配置時會依 namespace 層數多配。這是彈性陣列的舊寫法（課本寫 `numbers[]`）。因為 namespace 層數不固定，所以每層 namespace 用的 `pid_cachep` pool **大小不同**——這也是為什麼 pool 放在 `pid_namespace` 裡 |

> 原始碼裡 `tasks[]` 有 4 個（PID、TGID、PGID、SID），課本列了 TGID、PGID、SID 三個。`tasks[PIDTYPE_PID]` 是「用這個 PID 的那個 task 本身」。

---

## 6. 配置一個 PID：`alloc_pid()`

### 6.1 流程

`[45:32]` `alloc_pid()` 函式，如名字所說，配置一個 PID：

```
 1. 從 pool（pid_cachep）拿一個 struct pid        ← 非常快
 2. 從自己的 namespace 開始，往上走過每一個祖先 namespace：
      在那個 namespace 的 IDR tree 裡：
        「請給我最小的未配置 PID 號碼」     ← 可能這層給 5、那層給 7、再上層給 11
        建立對應：號碼 → struct pid
```

`[46:32]` 「我先配 `struct pid`（從 pool 拿），再走過每一個祖先 namespace，找一個 PID 號碼、建立一個對應——例如在這裡建立 11 → p。」

`[47:00]` 現在最重要的問題：**怎麼從 IDR tree 拿到最小的未配置 PID？**（§7）

### 6.2 【原始碼】`alloc_pid()`

```c
// kernel/pid.c:159-247（節錄）
struct pid *alloc_pid(struct pid_namespace *ns, pid_t *set_tid,
		      size_t set_tid_size)
{
	...
	pid = kmem_cache_alloc(ns->pid_cachep, GFP_KERNEL);
	if (!pid)
		return ERR_PTR(retval);

	tmp = ns;
	pid->level = ns->level;

	for (i = ns->level; i >= 0; i--) {
		int tid = 0;

		if (set_tid_size) {
			tid = set_tid[ns->level - i];
			...
		}

		idr_preload(GFP_KERNEL);
		spin_lock_irq(&pidmap_lock);

		if (tid) {
			nr = idr_alloc(&tmp->idr, NULL, tid,
				       tid + 1, GFP_ATOMIC);
			...
		} else {
			int pid_min = 1;
			/*
			 * init really needs pid 1, but after reaching the
			 * maximum wrap back to RESERVED_PIDS
			 */
			if (idr_get_cursor(&tmp->idr) > RESERVED_PIDS)
				pid_min = RESERVED_PIDS;

			/*
			 * Store a null pointer so find_pid_ns does not find
			 * a partially initialized PID (see below).
			 */
			nr = idr_alloc_cyclic(&tmp->idr, NULL, pid_min,
					      pid_max, GFP_ATOMIC);
		}
		spin_unlock_irq(&pidmap_lock);
		idr_preload_end();
		...
		pid->numbers[i].nr = nr;
		pid->numbers[i].ns = tmp;
		tmp = tmp->parent;
	}
```

| 行 | 對應教授說的 |
|---|---|
| 180 | **`kmem_cache_alloc(ns->pid_cachep, ...)`**：**從 pool 拿一個 `struct pid`** |
| 184–185 | 從自己的 namespace（`ns`）開始 |
| 187 | **`for (i = ns->level; i >= 0; i--)`**：從自己的層級往下數到 **0**——也就是從自己往上走到**根**（所以根 namespace 是 level **0**） |
| 209 | 上鎖：`pidmap_lock`——平行存取要鎖 |
| 233–234 | **`idr_alloc_cyclic(&tmp->idr, ...)`**：在**這一層** namespace 的 IDR tree 裡配一個號碼 |
| 244–245 | **`pid->numbers[i] = {nr, tmp}`**：把 (號碼, namespace) 存進 upid 陣列 |
| 246 | `tmp = tmp->parent`：往上一層 |

**兩個跟教授 / 課本不一樣的地方**：

**① 不是「最小的」，是「循環的（cyclic）」**

- 教授和課本說「找**最小的**未配置 PID」。
- 原始碼用的是 **`idr_alloc_cyclic`**：從**上次配置的位置之後**開始找，到 `pid_max` 再**繞回來**。
- 所以 PID 是一直**往上加**的，不會馬上重用剛釋放的小號碼（§9 實測可以看到）。
- 為什麼？如果馬上重用，一個 process 剛死、同號碼立刻給了新 process，別人手上的舊 PID 就會**指錯人**——§3.4 說的問題。循環配置讓同一個號碼**很久之後**才會被重用。
- 但在 IDR tree 內部，「從某位置開始找下一個空位」用的正是 augmented tree 的技巧——教授說的機制還是對的，只是**起點**不是 0。

**② `RESERVED_PIDS`**

```c
// kernel/pid.c:64
#define RESERVED_PIDS		300
```

第 222–227 行的註解：「**init 一定需要 PID 1**，但到最大值繞回來時，從 `RESERVED_PIDS` 開始。」所以繞回來之後，**1–299** 不會再被配出去——保留給開機時建立的系統 process。

另外 `set_tid`（第 190–206 行）讓呼叫者**指定**要某個號碼——這就是給 **CRIU** 用的：把容器搬到新機器時，要讓 process **拿回原本的 PID**（§3.3）。第 203 行檢查要有 `checkpoint_restore` 權限。

---

## 7. IDR tree：radix tree + augmented tree

### 7.1 要回答的問題

`[47:00]` kernel 有一個特殊的 **proc 檔案系統**，告訴你系統的細節：

- `/proc/cpuinfo`：所有 CPU 和設定。
- `/proc/meminfo`：所有記憶體。
- `/proc/sys/kernel/pid_max`：**任何一個 namespace 最多能有幾個 PID**——因為每個 PID 在根 namespace 都有一筆，所以這自動就是**整個系統**最多能有幾個 PID。

`[48:00]` 需要一個資料結構回答（課本列了四個）：

| # | 問題 |
|---|---|
| 1 | 給一個 PID 號碼，**找到它的 `struct pid`** |
| 2 | 從某個位置開始，找**下一個空的** PID |
| 3 | **快速釋放**一個 PID |
| 4 | 某個 PID **有沒有被配置** |

`[48:31]` 「我們要一個把**號碼對應到結構**的東西，也要一個能在概念上的位元向量裡**快速掃描**的東西。」

### 7.2 為什麼用 radix tree 而不是 hash table

`[49:02]` 把 PID 存成 radix tree：從**最高位元**開始，依照位元 0、1、0、1……往下走，葉子是 `struct pid`。葉子是 NULL → 沒配置。

`[49:31]` 「它像 hash table，但**實務上比 hash table 快**，因為它做得很**節省記憶體**。」

`[50:01]` 當然不是二元的——「我們一直在**反對**二元樹」——是 **64 元樹**。而且要能**平行存取**：「kernel 裡建任何東西，平行存取都是重要需求。」

【課本補充】課本解釋為什麼不用 hash table（p.84）：kernel 開發者做了**很多實驗**，發現大部分時候 PID **共享前綴**（高位數字），因為活躍的 process 通常 PID 差不多（大約同時建立的）。查過一個 process 的 radix tree 路徑之後，相關的部分**還在處理器快取裡**，查相關 process 時可以利用。所以實務上 radix tree **比 hash table 快**。

### 7.3 xa_node：64 個 slot + 64 位元的 mark

`[50:30]` 怎麼找空的 PID？假設有一個**概念上的位元向量**（實際上不是存在一個地方，是**分散存放**的），要找下一個 1。

【口頭】`[51:00]` 教授插了一段：

> 「我研究了很久 **indices** 和 **indexes** 這兩個複數。很多來源說法不一，常常互相矛盾。說『股市指數』（stock market indices）或『經濟成長指標』用 indices 沒問題，但在我們這裡用比較少人用的 **indexes**——因為從 index 來看，它比較像複數。」

`[51:30]` 做法：**augment 一棵 radix tree**。

`[52:01]` 不要每次只看 1 個位元（只有 2 個選擇），**一次看 6 個位元**（64 個選擇）：

- radix tree 變成 **64 元樹**。
- 跟 B 樹、B+ 樹、maple tree 一樣的好處：**快取友善**。

`[55:01]` 整個結構叫 **xarray**，每個節點是一個 **xa_node**（課本 Fig 3.7）：

```
              xa_node
  ┌──────────────────────────────────┐
  │ marks: [1][0][1][0]...[0]  （64 位元）│  ← 第 i 位 = 第 i 棵子樹有沒有空的 PID
  ├──────────────────────────────────┤
  │ slots: [→][→][→][→]...[→]  （64 個指標）│  ← 指向子 xa_node 或 struct pid
  └──────────────────────────────────┘
```

| 陣列 | 大小 | 內容 |
|---|---|---|
| **slots** | 64 個指標 | 指向下一層的 xa_node，或（葉子層）指向 `struct pid` |
| **marks** | 64 個位元 | 第 i 位是 1 → **第 i 棵子樹有空的 PID** |

`[55:31]` 「最有效率存 64 個位元的方式？存在一個 **64 位元 word** 裡——一個 64 位元暫存器。」然後用 x86 的 **`bsf`** 指令找第一個 1：

> 「它在硬體裡掃描，**2 到 3 個 cycle**，非常快。如果用軟體做要很久：位移、取最低位、檢查……這叫 `xas_find_chunk`。」

`[56:03]` 【口頭】「其他機器可能沒有這個指令——有些可能有，但我不知道，這是**給你們的作業**。沒有的話就只能走慢路，沒有立即的選擇。」

`[56:31]` 遞迴往下：到一個節點，看 marks，去第一個有空位的子樹，一直到葉子。

- **查詢**：走到葉子，就是 `struct pid`。
- **配置**：走到第一個未配置的 PID，把它從 1 改成 0，然後**往上修狀態**（§1.2 的 1 → 0 規則）。
- 要能**平行**做——「去看 xarray 的程式碼，他們用鎖等等**解決得很好**，這是已解決的問題。」

`[57:31]` 「新的元素有兩個：**不是二元的，是 64 元**（理由在講 B 樹時很清楚了）；用了一個**機器指令**——這是**軟硬體協作**。硬體設計者知道軟體工程師會需要，所以做了這麼精密的指令，不然真的會花很多時間。」

【課本補充】課本補充（p.89）：

- **Definition 3.1.1**：IDR tree 是 key-value 儲存結構，key 是 PID 號碼、value 是 `struct pid`，value 存在葉子，是 **radix tree 和 augmented tree 的組合**。
- IDR = **ID Radix**。
- 從 PID 的**最高位**往**最低位**走，這保證用前序走訪時葉子是**排序過的**。
- **Trivia 3.1.1**：位元向量**不是明確存在一個地方**，而是**分散在倒數第二層的所有內部節點**裡（指向葉子的那一層）。

### 7.4 例子：PID 0、1、3、4、7

`[58:02]`（課本 Fig 3.6）因為 64 元樹畫不進投影片，用**二元**示範。

已配置：0、1、3、4、7。二進位：`000`、`001`、`011`、`100`、`111`。

```
                         根 [1 1]                     ← 兩邊都有空位
                 ┌───────────┴────────────┐
             [0 1]                       [1 1]
          ┌────┴────┐               ┌────┴────┐
       [0 0]      [1 0]          [0 1]      [1 0]     ← marks：1 = 那個位置空著
       ╱   ╲      ╱   ╲          ╱   ╲      ╱   ╲
     000   001  010   011      100   101  110   111
     PID0  PID1 (空)  PID3     PID4  (空) (空)  PID7
```

`[58:33]` 逐層看：

| 節點 | marks | 為什麼 |
|---|---|---|
| 左下 `[0 0]` | 0 和 1 都**已配置**（000、001） | 兩個都不空 |
| 左中下 `[1 0]` | 010 **空**、011 已配置 | 左邊空 |
| 左上 `[0 1]` | 左子樹全滿、右子樹有空 | 「父節點會**勸大家別往左走**，因為那邊什麼都拿不到」 |
| 右下兩個 | 100 已配、101 空；110 空、111 已配 | 各有一個空 |
| 右上 `[1 1]` | 兩邊都有空 | |
| 根 `[1 1]` | 兩邊都有空 | 往哪邊都找得到 |

`[60:32]` **配置**（偏好左邊）：

1. 根 `[1 1]`：往**左**。
2. 左上 `[0 1]`：左邊是 0（沒空位），往**右**。
3. 左中下 `[1 0]`：往**左**。
4. 到 **010 = PID 2**——配給它。

「這就是 IDR tree 的實際例子。」

### 7.5 【原始碼】`struct idr` 與 `struct xa_node`

```c
// include/linux/idr.h:19-23
struct idr {
	struct radix_tree_root	idr_rt;
	unsigned int		idr_base;
	unsigned int		idr_next;
};
```

```c
// include/linux/radix-tree.h:25-26
#define radix_tree_root		xarray
#define radix_tree_node		xa_node
```

| 部分 | 意思 |
|---|---|
| `idr_rt` | 一棵 **radix tree**——而 `radix_tree_root` **就是** `xarray`（第 25 行的 `#define`） |
| `idr_base` | 號碼從幾開始 |
| **`idr_next`** | **下一次從哪裡開始找**——這就是 §6.2 `idr_alloc_cyclic` 用的「游標（cursor）」 |
| 第 25–26 行 | 舊名字 radix tree 現在**就是** xarray 的別名。「IDR tree = radix tree = xarray」 |

xa_node 的實際結構：

```c
// include/linux/xarray.h:1126-1131
#define XA_CHUNK_SHIFT		(CONFIG_BASE_SMALL ? 4 : 6)
...
#define XA_CHUNK_SIZE		(1UL << XA_CHUNK_SHIFT)
...
#define XA_MAX_MARKS		3
#define XA_MARK_LONGS		DIV_ROUND_UP(XA_CHUNK_SIZE, BITS_PER_LONG)
```

```c
// include/linux/xarray.h:1140-1156
struct xa_node {
	unsigned char	shift;		/* Bits remaining in each slot */
	unsigned char	offset;		/* Slot offset in parent */
	unsigned char	count;		/* Total entry count */
	unsigned char	nr_values;	/* Value entry count */
	struct xa_node __rcu *parent;	/* NULL at top of tree */
	struct xarray	*array;		/* The array we belong to */
	union {
		struct list_head private_list;	/* For tree user */
		struct rcu_head	rcu_head;	/* Used when freeing node */
	};
	void __rcu	*slots[XA_CHUNK_SIZE];
	union {
		unsigned long	tags[XA_MAX_MARKS][XA_MARK_LONGS];
		unsigned long	marks[XA_MAX_MARKS][XA_MARK_LONGS];
	};
};
```

| 部分 | 值 | 對照教授說的 |
|---|---|---|
| `XA_CHUNK_SHIFT` | **6** | **一次看 6 個位元** |
| `XA_CHUNK_SIZE` | 1 << 6 = **64** | **64 元樹** |
| **`slots[64]`** | 64 個指標 | **slots** |
| **`marks[3][1]`** | 3 組 mark，每組 64 位元（1 個 `unsigned long`） | **marks**：每組剛好一個 64 位元 word，可以直接用 `bsf` |
| `XA_MAX_MARKS` | **3** | 每個節點有 **3 種**不同用途的 mark（不只一種） |
| `shift` | | 「每個 slot 還剩幾個位元」——這個節點在樹的哪一層 |
| `parent` | | 指向父節點，往上修狀態時用 |
| `CONFIG_BASE_SMALL ? 4 : 6` | | 很小的嵌入式系統用 16 元樹省記憶體 |

IDR 用哪一種 mark 表示「空位」？

```c
// include/linux/xarray.h:256
#define XA_FREE_MARK		XA_MARK_0
```

第 0 組 mark 就是**「這棵子樹有空位」**——教授說的那個位元向量。

---

## 8. 檔案系統、I/O 與除錯欄位

`[61:01]` 結構、namespace、process 講得差不多了，最後看一點檔案系統、I/O 和除錯相關的欄位。

### 8.1 檔案與開啟

`[61:01]`

- **檔案**：儲存裝置上一串連續的 bytes。
- **檔案系統**：檔案的**樹狀**組織，每個**內部節點是目錄**、**葉子是檔案**，是一棵「**分支數變化很大的 m 元樹**」。

`[61:31]` 一個 process 可以**擁有**很多檔案。擁有 = **開啟（open）**它：

> 「開啟就是告訴 kernel：『我想用這個檔案』，讓 kernel 建立一些資料結構，讓你能存取它。你用**名字**識別檔案，就像用 ID 識別 process，但你不能直接那樣存取，要先跟 kernel 表達意圖。kernel 會檢查檔案存不存在、你有沒有權限……有的話就在內部建資料結構，讓你方便讀寫。」

所以處理任何檔案都多兩個操作：

```
 open（開啟） → read / write / ...（做你要做的） → close（關閉：拆掉 kernel 建的資料結構）
```

【課本補充】不同檔案格式（`.mp4`、`.docx`）看起來很不一樣，但對 OS 來說，**檔案就只是一串 bytes**。不同的檔案系統依讀寫模式、循序 vs 隨機存取、可靠性需求等，用不同方式組織檔案和目錄。task 層級只需要一個**檔案系統的通用 handle** 和**開啟檔案的清單**。

### 8.2 Signal handler

`[63:00]` **Signal handler**：process 向 kernel **註冊**的函式：

> 「『如果你要送這個 signal 給我（例如 `SIGTSTP` 或別的），就呼叫我位址空間裡的這個函式。』例如 **`SIGUSR1`** 是自訂 signal，process 要向 kernel 註冊一個 handler。」

- 註冊在 `task_struct` 裡（一個 signal 結構的陣列，下一章看細節）。
- handler 是 **process 自己的函式**，在**使用者空間**被呼叫。

### 8.3 Block device 與 character device

`[64:03]` 第 1 講講過，Linux 有兩種裝置：

| | Block device | Character device |
|---|---|---|
| 最小 I/O | 一個 block（512 bytes，半 KB） | 一個字元 |
| 例子 | 硬碟、隨身碟——任何儲存裝置 | 滑鼠、鍵盤 |
| 處理 | 需要精密的 kernel 資料結構 | 「頻寬這麼低，沒問題」 |

（⚠️ 教授這裡把**網路卡**也舉為 block device 的例子；一般網路卡在 Linux 是另一類「網路裝置」，不屬於 block device。這段待第 7 章確認。）

`[64:30]` I/O context 在 I/O 那章再講，但至少要在 `task_struct` 裡有**指向這些資料結構的指標**。

【課本補充】課本說 block 大小 **512 B 到 4 KB**。block device 的 I/O 請求資訊要保存；I/O 完成或 character device 送資料來時，要呼叫 signal handler。

### 8.4 ptrace：process 的監護人

`[65:01]` 最後是 **ptrace** 機制：

- 你在**除錯器（debugger）**裡跑一個 process，需要另一個 process（例如它的父 process）**盯著它**——「拿放大鏡看它在做什麼」。
- 這是**非常關鍵的技術**：讓除錯器能運作，也讓程式能在**受保護的沙盒（sandbox）**裡跑。

`[65:31]` 沙盒的例子：

> 「在 Android 上跑一個 App，它想存取相機，Android 擋下來，問你：『要不要給這個 App 相機權限？』為了這種受保護的沙盒，有時要讓一個 process **接管**另一個 process——這就是 ptrace。」

`[66:02]` 流程：

1. 被追蹤的 process 要**通知 kernel**：「我願意被追蹤、被控制。」
2. 定義規則：**可以做什麼、不能做什麼**。
3. 定義一個**感興趣的事件**：`fork`、system call……
4. process 一**越界** → 被追蹤的 process **停下來** → kernel 送 **`SIGTRAP`** signal 給**追蹤者（tracer）**。
5. 追蹤者的 signal handler 執行，用 ptrace **檢查**被追蹤者在做什麼——是哪個操作違規了。
6. 追蹤者可以**分析 system call**，甚至**改 system call 的參數**：「你沒有寫這個檔案的權限，但想讀的話可以——要開的話不能寫，但可以用唯讀模式開。」

`[67:00]` 【口頭】教授的比喻：

> 「如果你在**宿舍**搗蛋，會怎樣？學校會寄訊息給你的**家長**：『你兒子或女兒做了這些事。』一違規，訊息就送出去。這裡一樣：被追蹤的 process 一搗蛋，訊息就送給**監護人 process**，就像寄訊息給你爸媽。這應該讓你們很有共鳴吧。」

`[69:01]` 「把被追蹤的 process 想成**頑皮的小孩**、追蹤者是**監護人**、kernel 是**學校**。」

【課本補充】課本補充（p.90）：

- 我們特別關心 **system call**，因為這是 process 跟 OS 互動的主要機制。使用者如果有惡意，會表現在可能錯誤或惡意的 system call 上，所以要徹底審查 process 跟 kernel 的互動。
- 改 system call 參數有用：可以為除錯放進額外資訊；也可以把可能有害的 system call 改成**比較無害的版本**，限制它造成傷害的能力。

`[69:01]` 下一講：Linux 怎麼**建立、管理、銷毀** process。「這裡有一些**獨特、很有趣**的機制，Unix 和 Linux 這類系統用，但其他 OS 不用。」

### 8.5 【原始碼】`task_struct` 裡的這些欄位

課本 Listing 3.8 列的欄位，在 v6.2.12 的 `task_struct` 裡：

```c
// include/linux/sched.h（節錄，行號如註）
	unsigned int			ptrace;          // 762：ptrace 旗標（第 7 講看過在開頭附近）
	...
	struct pid			*thread_pid;     // 997：指向我的 struct pid
	struct hlist_node		pid_links[PIDTYPE_MAX];  // 998：掛在 struct pid 的 tasks[] 串列上的節點
	...
	struct fs_struct		*fs;             // 1085：檔案系統資訊（根目錄、目前目錄）
	...
	struct files_struct		*files;          // 1088：開啟的檔案
	...
	struct nsproxy			*nsproxy;        // 1095：我屬於哪些 namespace
	...
	struct signal_struct		*signal;         // 1098：signal 相關
	...
	struct bio_list			*bio_list;       // 1179：block I/O（bio = block I/O）
	...
	struct io_context		*io_context;     // 1189：I/O 裝置的 context
```

| 欄位 | 說明 |
|---|---|
| `ptrace` | 允許什麼樣的追蹤（旗標） |
| **`thread_pid`** | **`task_struct` → `struct pid`** 的指標 |
| **`pid_links[4]`** | 反方向：我是 `struct pid` 的 `tasks[]` 那 4 個串列裡的哪一個節點。兩個結構就這樣**雙向**串起來 |
| `fs` | 檔案系統的 handle |
| **`files`** | **開啟的檔案清單**——`open()` 回傳的檔案描述子（0、1、2……）就是這裡的索引 |
| **`nsproxy`** | 課本沒列：指向一組 namespace（PID、網路、掛載點、IPC……）。PID namespace 只是**其中一種** |
| `signal` | 課本註解說「註冊的 signal handler 清單」；實際上 handler 存在另一個欄位 `sighand`，`signal` 存的是整個 thread group 共享的 signal 狀態 |
| `bio_list` | 正在處理的 block I/O |
| `io_context` | I/O 排程用的 context |

---

## 9. 【實測】PID namespace 與 PID 配置

**① 一個 process，兩個 PID**

用 `unshare` 建一個新的 PID namespace（加 `-U` 用 user namespace，所以一般使用者也能做）：

```bash
$ unshare -Urpf --mount-proc sh -c 'echo "inside: $$"; ps -o pid,cmd'
inside: 1
    PID CMD
      1 sh -c echo "inside: $$"; ps -o pid,cmd
      3 ps -o pid,cmd
```

- 在新 namespace 裡，`sh` 的 PID 是 **1**——它就是這個 namespace 的 `child_reaper`（§4.3）。
- `ps` 只看得到這個 namespace 裡的 process（1 和 3），**看不到外面**——「任何 process 在子 namespace 都看不到」的反面：子 namespace 看不到父 namespace 的 process。

從**外面**看同一個 process：

```bash
$ unshare -Urpf --mount-proc sleep 5 &
$ grep NSpid /proc/$(pgrep -n -x sleep)/status
NSpid:	210701	1
```

- **同一個 process**，在根 namespace 是 **210701**，在新 namespace 是 **1**。
- 這行 `NSpid` 就是把 `struct pid` 的 **`numbers[]` 陣列**印出來：`numbers[0] = (210701, 根)`、`numbers[1] = (1, 新 namespace)`。正是 §5.2 的「每層 namespace 一個號碼」。

**② PID 是循環配置的**

```bash
$ for i in 1 2 3; do sh -c 'echo $$'; done
210707
210708
210709
```

每個 `sh` 跑完就結束、PID 就釋放了，但下一個**不會**重用剛釋放的號碼，而是一直往上加——印證 §6.2 的 **`idr_alloc_cyclic`**，不是「最小的空號碼」。

**③ PID 上限**

```bash
$ cat /proc/sys/kernel/pid_max
4194304
```

- 課本說預設 **32,768**——那是 kernel 的編譯預設值 `PID_MAX_DEFAULT`（`include/linux/threads.h:28`，`0x8000`）。
- 這台 Ubuntu 是 **4,194,304**（2²²）：開機時被 **systemd** 調大了，接近 kernel 的上限 `PID_MAX_LIMIT`。
- 所以上面的 PID 才會到 21 萬這麼大。

---

## 10. 一頁速查表

### 10.1 兩個資料結構

| | Radix tree | Augmented tree |
|---|---|---|
| 解決什麼 | 存共享前綴的 key | 在位元向量裡找下一個 1 |
| 時間 | 跟 key 長度成正比 | O(log n) |
| 優化 | 一次看 6 位元 → 64 元樹 | 葉子存一個 word；父節點存子樹狀態的位元向量 |
| 硬體 | — | x86 `bsf`：一個 64 位元暫存器裡找第一個 1，2–3 cycle |
| kernel 結構 | xarray | xarray 的 marks |

### 10.2 四層分組

| 層 | 例子 | `pid_type` |
|---|---|---|
| Process | 一個 task | `PIDTYPE_PID` |
| Thread group | 多執行緒 process | `PIDTYPE_TGID` |
| Process group | 一個指令產生的（Ctrl+C 全殺） | `PIDTYPE_PGID` |
| Session | 一個 shell 產生的 | `PIDTYPE_SID` |
| （Namespace） | 一個容器 | 另一套機制 |

### 10.3 Namespace

| 項目 | 內容 |
|---|---|
| 是什麼 | process 名字（ID）的空間；組成一棵樹 |
| 規則 | process 在自己和所有祖先 namespace 看得到，子 namespace 看不到 |
| 用途 | **容器**：搬家時 PID 不變 |
| 容器 vs VM | 容器輕量；VM 虛擬化整台機器、重量級 |
| 搬容器 | CRIU；`alloc_pid` 的 `set_tid` 讓 PID 能恢復 |
| 根 namespace | level 0（原始碼） |
| PID 1 | 每個 namespace 的 `child_reaper` |

### 10.4 結構

| 結構 | 重要欄位 |
|---|---|
| `pid_t` | 一個數字，只在某個 namespace 有意義 |
| `struct pid` | `count`、`level`、`tasks[4]`、`numbers[]`（每層一個 upid） |
| `struct upid` | `nr`（號碼）、`ns`（namespace） |
| `struct pid_namespace` | `idr`、`pid_cachep`（pool）、`level`、`parent`、`child_reaper` |
| `struct idr` | `idr_rt`（xarray）、`idr_next`（循環游標） |
| `struct xa_node` | `slots[64]`、`marks[3][1]`（每組 64 位元） |

### 10.5 `alloc_pid()`

| 步驟 | 內容 |
|---|---|
| 1 | 從 pool 拿一個 `struct pid` |
| 2 | 從自己的 namespace 往上到根，每層用 IDR tree 配一個號碼 |
| 3 | 存成 `numbers[i] = (號碼, namespace)` |
| 配置方式 | 教授 / 課本：最小的空號碼；原始碼：**循環**（`idr_alloc_cyclic`），繞回時從 300 開始 |

### 10.6 Pool

| 好處 | 說明 |
|---|---|
| 快 | 不用 malloc / free |
| 好追蹤 | pool 空了 → 可能記憶體洩漏 |
| kernel 名稱 | `kmem_cache`（slab） |

### 10.7 其他欄位

| 欄位 | 用途 |
|---|---|
| `files` | 開啟的檔案（open → read/write → close） |
| `fs` | 檔案系統 handle |
| `signal` / `sighand` | signal 狀態 / handler |
| `bio_list`、`io_context` | block I/O |
| `ptrace` | 被追蹤：越界 → 停下、送 `SIGTRAP` 給追蹤者 |
| `nsproxy` | 屬於哪些 namespace |
| `thread_pid`、`pid_links[]` | 跟 `struct pid` 雙向串接 |

### 10.8 本講看過的原始碼位置

| 檔案:行號 | 看什麼 |
|---|---|
| `arch/x86/include/asm/bitops.h:250-256` | `bsf` 指令 |
| `include/linux/pid_namespace.h:19-34` | `struct pid_namespace` |
| `include/linux/pid.h:9-16` | `enum pid_type` |
| `include/linux/pid.h:54-71` | `struct upid`、`struct pid` |
| `kernel/pid.c:64` | `RESERVED_PIDS` = 300 |
| `kernel/pid.c:159-247` | `alloc_pid()` |
| `include/linux/idr.h:19-23` | `struct idr` |
| `include/linux/radix-tree.h:25-26` | radix tree = xarray |
| `include/linux/xarray.h:1126-1156` | `XA_CHUNK_SHIFT`、`struct xa_node` |
| `include/linux/xarray.h:256` | `XA_FREE_MARK` |
| `include/linux/threads.h:28` | `PID_MAX_DEFAULT` |
| `include/linux/sched.h:762-1189` | `ptrace`、`thread_pid`、`fs`、`files`、`nsproxy`、`signal`、`bio_list`、`io_context` |

---

## 11. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| **建立 process 時呼叫 `alloc_pid`、`fork`** | **第 11 講**：process 建立（下一講） |
| **`files`、`fs` 在 fork 時要不要共享** | **第 11 講**：`clone` 的旗標 |
| **Signal handler、`SIGTRAP`、`SIGUSR1`** | **第 17 講**：signal |
| **Thread group、`PIDTYPE_TGID`** | **第 18 講**：多執行緒 |
| **鎖（`pidmap_lock`、`struct pid` 的 `lock`）、race condition** | **第 20–22 講**：鎖、kernel 裡的並行 |
| **`__rcu`、`rcu_head`** | **第 23 講**：RCU |
| **Augmented tree 找空的 frame** | **第 30、33 講**：kernel 記憶體架構、buddy allocator |
| **Pool、`kmem_cache`** | **第 33 講**：kernel 記憶體配置（slab / slub） |
| **Block device、`bio_list`、檔案系統** | 課本第 7 章（I/O 系統）。對應講次**待補** |
| **容器、namespace、cgroup** | 課本第 8 章 §8.2（Containers）。對應講次**待補** |

---

## 附：本講一句話總結

> **每個 process 都有一個 PID 號碼，但號碼只在它自己的 namespace 裡有意義——為了讓容器能搬家，一個 process 在自己和每個祖先 namespace 各有一個號碼，存在 `struct pid` 的 `numbers[]` 陣列裡（實測：`NSpid: 210701 1`）。每個 namespace 用一棵 IDR tree 管號碼：它是 radix tree 加上 augmented tree 的 64 元 xarray，每個節點有 64 個 slot 和 64 位元的 mark，靠 x86 的 `bsf` 指令一條就找到哪棵子樹還有空號碼。**

---

*筆記依據 Smruti R. Sarangi「Operating Systems: A Linux Kernel-Oriented Approach」Lecture 10（Chapter 3, Part IV）錄影逐字稿（YouTube 自動字幕，1:10:04）、課本 *Operating Systems: A Linux Kernel-Oriented Approach*（Sarangi, v0.96）§3.1.11–3.1.13 與附錄 C.5–C.6，以及 Linux kernel v6.2.12 原始碼整理。實測為本機（Ubuntu、kernel 6.5.0）結果。*
