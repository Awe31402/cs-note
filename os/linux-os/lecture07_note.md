# Operating Systems: A Linux Kernel-Oriented Approach 第七講完整筆記：Linux 的 Process 狀態（Chapter 3, Part I）

> **課程**：Operating Systems: A Linux Kernel-Oriented Approach — Lecture 7（Chapter 3, Part I）
> **講者**：Smruti R. Sarangi
> **來源**：YouTube 錄影逐字稿（43:15，自動字幕）＋ 課本 *Operating Systems: A Linux Kernel-Oriented Approach*（Sarangi, v0.96）第 3 章開頭與 §3.1.1–3.1.4（p.63–72）＋ Linux kernel **v6.2.12** 原始碼（`os/linux-src/`）
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
- [1. Process 是什麼](#1-process-是什麼)
  - [1.1 程式的執行實例](#11-程式的執行實例)
  - [1.2 Process 擁有的東西](#12-process-擁有的東西)
- [2. Process 的種類：獨立 process 與 thread](#2-process-的種類獨立-process-與-thread)
  - [2.1 Process 樹](#21-process-樹)
  - [2.2 Thread：共享什麼、不共享什麼](#22-thread共享什麼不共享什麼)
  - [2.3 為什麼要 thread](#23-為什麼要-thread)
- [3. Process 描述子：`task_struct`](#3-process-描述子task_struct)
  - [3.1 難的是「表示資訊」，不是演算法](#31-難的是表示資訊不是演算法)
  - [3.2 主要欄位](#32-主要欄位)
  - [3.3 Kernel stack：在 kernel 裡的小公寓](#33-kernel-stack在-kernel-裡的小公寓)
  - [3.4 【原始碼】`task_struct` 的開頭與關係欄位](#34-原始碼task_struct-的開頭與關係欄位)
- [4. `thread_info`：低階資料結構](#4-thread_info低階資料結構)
  - [4.1 問題：被我打斷的是誰？](#41-問題被我打斷的是誰)
  - [4.2 高階 vs 低階資料結構](#42-高階-vs-低階資料結構)
  - [4.3 把旗標塞進一個整數](#43-把旗標塞進一個整數)
  - [4.4 【原始碼】`thread_info` 與 TIF 旗標](#44-原始碼thread_info-與-tif-旗標)
- [5. Task 的狀態](#5-task-的狀態)
  - [5.1 `TASK_RUNNING`：跑中和準備好是同一個狀態](#51-task_running跑中和準備好是同一個狀態)
  - [5.2 程式怎麼結束：硬體不知道](#52-程式怎麼結束硬體不知道)
  - [5.3 Zombie：等父 process 收屍](#53-zombie等父-process-收屍)
  - [5.4 Stopped：`SIGSTOP` 與 Ctrl+Z](#54-stoppedsigstop-與-ctrlz)
  - [5.5 【Q&A】按了 Ctrl+Z 之後呢？](#55-qa按了-ctrlz-之後呢)
  - [5.6 兩種睡眠：interruptible 與 uninterruptible](#56-兩種睡眠interruptible-與-uninterruptible)
  - [5.7 狀態圖總整理](#57-狀態圖總整理)
  - [5.8 【原始碼】狀態常數與 `ps` 的字母](#58-原始碼狀態常數與-ps-的字母)
  - [5.9 【原始碼】`exit_notify()`：變成 zombie](#59-原始碼exit_notify變成-zombie)
  - [5.10 【實測】製造一個 zombie、暫停一個 process](#510-實測製造一個-zombie暫停一個-process)
- [6. 一頁速查表](#6-一頁速查表)
- [7. 與後續講次的連結](#7-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 這堂課要做什麼

`[00:32]` 第 3 章：**process**，「基本上是 kernel 的心臟」。

`[01:00]` 本章分四部分：

| # | 主題 | 本講有沒有講 |
|---|---|---|
| 1 | 概念 | ✅ |
| 2 | **process 描述子**（Linux 怎麼存一個 process 的所有資訊） | ✅ 開頭（`task_struct`、`thread_info`、狀態） |
| 3 | process 的建立與銷毀（「獨特、有人說非傳統」的方式）；zombie、dead process | 第 11 講 |
| 4 | **context switch**（各種切換方式） | 第 12–13 講 |

【口頭】`[02:01]` 教授說 context switch 就像**肝臟移植**：

> 「非常非常精細的過程，很多小動作要按**精確的順序**做。順序裡就算兩件事對調，整個過程都可能崩潰。而且 Linux 跑在**每一種硬體**上——今天設計的任何新硬體，不相容 Linux 是不可想像的。所以它的 context switch 機制要非常通用。這本身就是**工程奇蹟**。」

【課本補充】課本把本章分成三大部分（Fig 3.1）：**process 描述子**（`task_struct`、kernel stack、虛擬記憶體資訊、PID 與 namespace）、**process 建立與銷毀**（fork、exec、thread）、**context switch**（硬體 context、切換類型、細節）。

---

## 1. Process 是什麼

### 1.1 程式的執行實例

`[00:32]` **process = 程式在執行中（program in action），或程式的一個執行實例（running instance）**。

- 點一次記事本圖示 → 產生一個記事本 process。
- 再點一次 → 又一個。

【課本補充】課本講得更完整（p.63）：

- 程式（執行檔）是硬碟上一個 **ELF 格式**的檔案，在檔案系統裡**靜靜躺著**，不會執行。
- 使用者下指令執行它，**loader** 把程式碼和資料載入記憶體，把 PC 設成 text section 第一條指令的位址——process 就開始執行了。
- 它可以活很久：發 system call、收 signal、被換入換出**無數次**。銷毀前可能經歷**上千次** context switch。

### 1.2 Process 擁有的東西

`[03:00]` process 有「**自己的生命**」，也有「**自己的財產**」：

| 擁有的東西 | 說明 |
|---|---|
| **CPU 時間** | 未來保證拿到的 CPU 時間（確保它能跑）；過去用掉的 CPU 時間（記帳用） |
| **記憶體** | |
| **開著的檔案** | |
| **資料庫連線、網路連線** | |
| 很多相關的資料結構 | |

`[03:33]` process 被殺掉時，它擁有的**所有財產都要釋放**。難的是**追蹤**它到底擁有什麼。

`[04:01]` 跟 OS 溝通的方式第 1、2 講講過：**system call**（假中斷，請 OS 服務）和 **signal**（反向 system call，OS 通知 process）。

---

## 2. Process 的種類：獨立 process 與 thread

### 2.1 Process 樹

`[04:30]` 【口頭】「process 某種意義上**有關係**，某種意義上**沒關係**。」

Linux 把所有 process 組織成一棵**樹**：

```
                 init（「母親」process）
               /        |         \
          子 process  子 process   子 process
           /    \                    |
       孫子     孫子                 孫子
```

- 最上面是一個「**母親**」process，叫 **`init`**。
- `init` 有很多子 process，它們又有自己的子 process……
- 樹的性質：**每個 process 只有一個父 process**。

### 2.2 Thread：共享什麼、不共享什麼

`[05:30]` 樹裡的任何一個子 process，可以是：

| 類型 | 說明 |
|---|---|
| **獨立的（standalone）** | 來、跑、走。也叫**單執行緒 process（single-threaded process）**：在一個核心上跑，做完就結束 |
| **一群 process 的一員** | 跟另外 5、6、10 個 process 有關係，**合作**解決一個問題，**共享一部分位址空間** |

`[06:02]` 一群合作的 process 共享什麼？

| 共享 ✅ | 不共享 ❌ |
|---|---|
| **程式碼（code / text）** | **Stack**（每人一個私有 stack） |
| **Heap**（`new`、`malloc` 的資料） | **Thread local storage（TLS）**：「名字就叫 thread *local*」，用 segment 暫存器（`fs`）索引的一小塊（第 6 講） |
| **全域變數**（data、bss） | **暫存器等核心相關狀態**（本來就不可能共享） |

```
   Thread 1        Thread 2        Thread 3
 ┌─────────┐    ┌─────────┐    ┌─────────┐
 │ Stack 1 │    │ Stack 2 │    │ Stack 3 │   ← 私有
 │ TLS 1   │    │ TLS 2   │    │ TLS 3   │   ← 私有
 │ 暫存器   │    │ 暫存器   │    │ 暫存器   │   ← 私有
 └────┬────┘    └────┬────┘    └────┬────┘
      └──────────────┼──────────────┘
         ┌───────────┴───────────┐
         │  Code、Heap、Data、BSS  │           ← 共享
         └───────────────────────┘
```

`[07:00]` 為什麼共享？「讓大家能**合作**解決問題：所以共享程式碼；不共享資料怎麼合作？資料就放在 heap。」

`[07:30]` 這樣一群 process 裡的**每一個**叫 **thread（執行緒）**，更精確地說是 **software thread（軟體執行緒）**。

`[07:30]` 教授的定義：

- 可以說任何獨立 process 都是「一個 thread」，這樣說也行，但它**不屬於多執行緒 process**。
- **多執行緒 process（multi-threaded process）** = 一群 process，**每一個都是完整的 process**，但 OS 記錄「它們屬於同一群」，共享某些記憶體區域和其他資源（可能包括 CPU 時間，看排程器怎麼設計）。
- thread 也叫 **輕量級 process（lightweight process）**。

`[08:31]` 【口頭】「process 一般是程式的執行實例；**thread 就像 process 再加一點東西**：它屬於一個群組，群組裡共享部分記憶體空間，群組本身可能擁有一些資源，分給所有成員。」

【課本補充】課本補充（p.66–67）：

- process 有不同特權等級：**使用者 process、kernel process、middleware process**（跑在 ring 1、2）。
- thread 除了共享記憶體，也共享**開著的檔案、網路連線、環境變數、資源限制**。
- 同一群 thread 共用一個 **thread group id（tgid）**。通常有一個**群組領袖（group leader）**——通常是產生其他 thread 的那個 thread，它的 **pid 等於整群的 tgid**。
- **Point 3.1.1**：thread 和獨立 process 其實是**一個光譜的兩端**。真正完全獨立的 process 很少，因為動態連結 library 的程式碼頁通常是共享的。建立 process 時可以**精確指定**子 process 要跟父 process 共享哪些資源。thread 是光譜的極端：**盡可能共享**。
- Linux 裡同一群 thread **共享整個虛擬位址空間**，每個 thread 的 stack 和 TLS 只是放在這個共享空間裡**各自不同的位置**。

### 2.3 為什麼要 thread

`[09:30]` thread 讓你能寫**平行程式**：

- 不然你只能用一個核心。四核心處理器，怎麼四個都用上？要**合作**。
- 例子：**網頁伺服器**。它的工作是提供網站，但不該是完全分開的 process，因為它們**有太多共同的東西**——多執行緒架構就有好處。

`[10:02]` 【口頭】「這只是簡短介紹，書裡有更多。我們之後會**反覆講（ad nauseam）**，別擔心。」（⚠️ 逐字稿寫成「AD naum ... add infinum」）

---

## 3. Process 描述子：`task_struct`

### 3.1 難的是「表示資訊」，不是演算法

`[01:00]` 教授在本章開頭就強調：

> 「我們會看到**資料結構設計**在 OS 設計裡扮演非常關鍵的角色——**不是像大家以為的那麼關於演算法**。更多是**資訊怎麼表示**。我們從沒覺得這是很嚴重的問題，因為課堂作業都很簡單。但像 OS 這麼大的東西，**光是表示資訊就是挑戰**，有時比演算法本身難得多。演算法（樹、stack、queue）大家都會，可以**整套直接拿來用**。」

`[10:02]` Linux 的核心資料結構是 **process 描述子（process descriptor）**。挑戰是：怎麼表示這麼多資訊——一個 process 可能是獨立的，也可能是多執行緒群組的一員。

`[13:04]` Linux 用 **`struct task_struct`**，在 **`include/linux/sched.h`**。

- Linux 是 **C** 寫的，不是 C++，所以**沒有 class 和物件**，只有 struct。
- Linux 內部把 process 叫做 **task**，所以叫 `task_struct` 而不是 `process_struct`。`[25:02]`

`[11:01]` 教授說重點**不是**每個欄位做什麼、是 `int` 還是 `float`，而是**表示資訊的策略**：要存很多

- **記帳資訊（bookkeeping）**
- **一般資訊**
- **關係資訊**：父 process、一堆子 process、是不是某個 thread group 的成員、group 的領袖是誰……

> 「我們想把所有 process 相關的資訊放在**同一個地方**。它會變成很大的資料結構，但沒關係。」

`[12:02]` 課本**附錄 C**（資料結構）在這裡會很有用。

`[17:30]` 【口頭】「東西都放一個地方，很容易失控。我們會看到 Linux 怎麼**不讓這件事發生**。這就是研究開源 OS 的好處：你會看到事情**實際上**怎麼做，而不是**理論上**該怎麼做。」

### 3.2 主要欄位

`[13:30]`（課本 Table 3.1）

| 欄位 | 說明 | 哪一講 |
|---|---|---|
| `struct thread_info thread_info` | **低階資訊**。以前很重要，**現在用得不多**，但體現了一種設計模式 | 本講 §4 |
| `state` | **process 狀態** | 本講 §5 |
| `void *stack` | **kernel stack** | 第 8 講 |
| `prio`、`static_prio`、`normal_prio` | **優先權** | 第 8 講 |
| `struct sched_info sched_info` | 排程資訊 | 第 24–26 講 |
| `struct mm_struct *mm, *active_mm` | **記憶體資訊**（「會講一些新穎的資料結構」） | 第 9 講 |
| `pid_t pid` | **process ID**：管號碼、也管資料結構，「很複雜」 | 第 10 講 |
| `struct task_struct *parent` | **父 process** | 第 10 講 |
| `struct list_head children, sibling` | **子 process、兄弟 process**（樹的關係） | 第 10 講 |
| 其他 | 檔案系統、I/O、同步、除錯 | |

`[14:01]` 「有些欄位現在沒在用了。」例如 `thread_info`，但「歷史上怎麼用的還是值得看，因為它體現了一種**設計模式**」：

> 【口頭】「我們平常寫程式不在乎效率，不會想快取線邊界、page 邊界。但寫真正的程式時，跟計算機結構的互動就很重要。因為最後重要的是：我開始一個程式就按碼表，結束時看花了多久。減少執行時間至關重要，**任何效率來源都歡迎**。」

### 3.3 Kernel stack：在 kernel 裡的小公寓

`[15:01]` 每個使用者程式進 kernel 時（例如處理 system call），在 kernel 裡需要有一些狀態，要有地方放。

【口頭】教授的比喻：

> 「假設我常常在德里和孟買之間跑。偶爾去一次就住旅館；但如果**很常去**，就值得在那裡買間**小公寓**，放點家具和東西。我大部分時間在使用者空間，但是 kernel 的**常客**，所以應該在 kernel 裡有個小家，放一點狀態——這個小家就是 **kernel stack**。」

`[16:01]` 還有一個 **kernel thread**，像你在 kernel 裡的**化身（avatar）**，幫你做所有 system call 的工作，「像你在 kernel 裡的**鏡像**」。（第 5 章再講。）

### 3.4 【原始碼】`task_struct` 的開頭與關係欄位

`task_struct` 在 v6.2.12 從第 737 行開始，**非常長**（好幾百行）。開頭：

```c
// include/linux/sched.h:737-762
struct task_struct {
#ifdef CONFIG_THREAD_INFO_IN_TASK
	/*
	 * For reasons of header soup (see current_thread_info()), this
	 * must be the first element of task_struct.
	 */
	struct thread_info		thread_info;
#endif
	unsigned int			__state;

#ifdef CONFIG_PREEMPT_RT
	/* saved state for "spinlock sleepers" */
	unsigned int			saved_state;
#endif

	/*
	 * This begins the randomizable portion of task_struct. Only
	 * scheduling-critical items should be added above here.
	 */
	randomized_struct_fields_start

	void				*stack;
	refcount_t			usage;
	/* Per task flags (PF_*), defined further below: */
	unsigned int			flags;
	unsigned int			ptrace;
```

| 行 | 欄位 | 對照課本 Table 3.1 |
|---|---|---|
| 743 | `thread_info` | **第一個欄位**。註解說「**必須**是 `task_struct` 的第一個元素」——這樣 `task_struct` 的位址就等於 `thread_info` 的位址，兩者可以互相轉換 |
| 745 | `__state` | **process 狀態**（課本寫 `uint state`，v6.2.12 改名成 `__state`，加底線表示「不要直接存取，用函式」） |
| 753–756 | `randomized_struct_fields_start` | 從這裡以下的欄位**順序會被隨機打亂**（編譯時）。攻擊者就算能寫 kernel 記憶體，也猜不到某欄位在哪個偏移。註解說「只有**排程關鍵**的欄位才放在上面」——放上面是為了效能（固定位置、快取友善） |
| 758 | `stack` | **kernel stack** |
| 761 | `flags` | 每個 task 的旗標（`PF_*`），例如 `PF_EXITING`（正在結束）、`PF_KTHREAD`（是 kernel thread） |

x86 有沒有開 `CONFIG_THREAD_INFO_IN_TASK`？

```text
# arch/x86/Kconfig:288
	select THREAD_INFO_IN_TASK
```

有。所以在 x86 上，`thread_info` **就嵌在 `task_struct` 裡面**（不是放在 kernel stack 底部，那是舊做法，第 8 講會講）。

**關係欄位**（教授說的「父、子、thread group」）：

```c
// include/linux/sched.h:961-985（節錄）
	pid_t				pid;
	pid_t				tgid;
	...
	struct task_struct __rcu	*real_parent;
	...
	struct task_struct __rcu	*parent;
	...
	struct list_head		children;
	struct list_head		sibling;
	struct task_struct		*group_leader;
```

| 欄位 | 意思 |
|---|---|
| `pid` | 這個 task 的 ID |
| `tgid` | **thread group ID**。單執行緒 process：`tgid == pid`。多執行緒：所有 thread 的 `tgid` 相同，等於領袖的 `pid`。（使用者程式呼叫 `getpid()` 拿到的其實是 **`tgid`**） |
| `real_parent` | **真正的**父 process（建立我的那個） |
| `parent` | 收 `SIGCHLD`、負責 `wait` 的那個。平常等於 `real_parent`；被除錯器（ptrace）掛上時會變成除錯器 |
| `children` | 我的子 process 串列的**頭** |
| `sibling` | 我在**父 process 的 `children` 串列**裡的節點 |
| `group_leader` | thread group 的領袖 |

`children` 和 `sibling` 都是 `list_head`——第 4 講看過的 kernel linked list。走訪所有子 process 就是 `list_for_each_entry(child, &task->children, sibling)`。樹就是這樣用 linked list 串起來的。

---

## 4. `thread_info`：低階資料結構

### 4.1 問題：被我打斷的是誰？

`[18:30]` 情境：

1. 我在跑一個使用者 process，**中斷來了**。
2. 進入 kernel，照慣例：收集整個狀態、放進 context、存起來。
3. 跑 interrupt handler。
4. 決定接下來做什麼：恢復那個 process？還是跑別的？

`[19:00]` 問題：處理完中斷後，我想知道**剛剛被我打斷的 task 是誰**——它的 ID，或指向它 `task_struct` 的指標。

- 收集和儲存 context 時要用到。
- 而且**很多次**都會用到：我打斷了誰？它的東西放哪？
- 「這個指標必須**隨手可得**。」

`[19:30]` 教授說要介紹一個**舊的解法**（現在不用了），先體會它的巧妙之處：**`thread_info`**。

（新的解法是第 6 講看過的：用 `gs` 存 per-CPU 的 `current`。）

### 4.2 高階 vs 低階資料結構

`[20:00]`

| | 高階資料結構 | 低階資料結構 |
|---|---|---|
| 怎麼寫 | 一般的 struct：`int a; int b; float c;` | 一樣是 struct |
| 考慮什麼 | 只從**自己的角度**想：要存兩個 int、三個 float，名字隨便取 | 同樣的東西，但**清楚知道**：**怎麼存、存在哪、裡面存什麼**——為了在真實硬體上跑得有效率 |
| 不在乎 | 怎麼存、存在哪、有沒有對齊快取線、有沒有對齊 page | — |

`[21:00]` 「`thread_info` 看起來像一般的 struct，但**花了很多心思設計**。」

【課本補充】課本的定義（p.68）：**低階資料型別**是基本型別，或是**知道硬體**的 C struct。它的設計深受「怎麼有效率地存和取」影響。例如欄位安排得**對齊快取線邊界**，減少 **false sharing**（偽共享）造成的快取失誤。

另一個例子：如果知道某資料結構**一定從 4 KB page 邊界開始**，就很容易算出其他欄位的位址，或反過來——**給一個欄位的位址，找出資料結構的起點**。`thread_info` 就是經典例子（第 8 講細講）。

### 4.3 把旗標塞進一個整數

`[21:00]` `thread_info` 有兩個 `long` 欄位，存**旗標（flags）**：process 目前的狀態、system call 的狀態、同步相關資訊。每個旗標是 **1 位元**的欄位。

`[21:30]` 問題：要存 19 個旗標、每個 1 位元，**最有效率**的方法是什麼？

| 做法 | 空間 |
|---|---|
| ❌ 一個 19 個 `int` 的陣列 | 19 × 32 位元——「**太浪費**」 |
| ✅ 一個 `uint32`（32 位元整數），**第 i 個位元就是第 i 個旗標** | 32 位元 |

`[22:31]` 32 位元可以存 32 個 1 位元旗標，每一個都能用**簡單的布林運算和位移**取出來——OS 程式碼**大量使用**這招。

`[23:01]` 旗標**不一定是 1 位元**，也可以是 3 位元。本質上是一堆位元的集合：例如 32 位元裡，這 3 個位元是一個欄位、第 27 位元是另一個……全都能放進一個普通整數，不需要特殊資料結構。

> 【口頭】「這是一個**持久的模式（enduring pattern）**，你會到處看到它。」

`[23:33]` 另外還有一個 32 位元整數存**狀態（status）**，和這個 thread **在哪顆 CPU 上跑**。

`[24:02]` 「C++ 的高級資料結構最後也是用這種技巧，但 Linux 是 C 寫的，你可以看到它**赤裸裸的實作**。」

【課本補充】課本補充（p.70）：`thread_info` 存 thread 的**目前狀態、正在執行的 system call 的狀態、同步資訊**，還有一個重要資訊：**它在（或將在）哪顆 CPU 上跑**。找出目前 CPU 的 id 是**非常頻繁**的操作，`thread_info` 的做法「有點次佳」，後面會看到更快的方法。

### 4.4 【原始碼】`thread_info` 與 TIF 旗標

課本 Listing 3.1 引用的就是這個：

```c
// arch/x86/include/asm/thread_info.h:56-63
struct thread_info {
	unsigned long		flags;		/* low level flags */
	unsigned long		syscall_work;	/* SYSCALL_WORK_ flags */
	u32			status;		/* thread synchronous flags */
#ifdef CONFIG_SMP
	u32			cpu;		/* current CPU */
#endif
};
```

| 欄位 | 型別 | 教授說的 |
|---|---|---|
| `flags` | `unsigned long`（64 位元） | 兩個 long 之一：低階旗標 |
| `syscall_work` | `unsigned long` | 兩個 long 之二：system call 要做的額外工作（例如追蹤） |
| `status` | `u32` | 32 位元的狀態 |
| `cpu` | `u32` | **在哪顆 CPU 上跑**（只有多核心 `CONFIG_SMP` 才需要） |

整個 struct 只有 **24 bytes**：兩個 8 bytes + 兩個 4 bytes。注意 `u32`——第 1 講看過的 kernel 自訂型別，保證 32 位元。

`flags` 裡每個位元的意義：

```c
// arch/x86/include/asm/thread_info.h:81-83, 105-106
#define TIF_NOTIFY_RESUME	1	/* callback before returning to user */
#define TIF_SIGPENDING		2	/* signal pending */
#define TIF_NEED_RESCHED	3	/* rescheduling necessary */
...
#define _TIF_SIGPENDING		(1 << TIF_SIGPENDING)
#define _TIF_NEED_RESCHED	(1 << TIF_NEED_RESCHED)
```

| 常數 | 值 | 意思 |
|---|---|---|
| `TIF_SIGPENDING` | **2** | 第 2 位元：**有 signal 在等著處理** |
| `TIF_NEED_RESCHED` | **3** | 第 3 位元：**需要重新排程**（該換人了） |
| `_TIF_NEED_RESCHED` | `1 << 3` = `0b1000` | 同一個旗標的**遮罩（mask）**版本 |

這就是教授說的「第 i 個位元是第 i 個旗標」：

- `TIF_xxx` 是**位元編號**（第幾位）。
- `_TIF_xxx` 是**遮罩**（`1 << 編號`），用來做 `&`、`|` 運算。
- 檢查：`if (flags & _TIF_NEED_RESCHED)`；設定：`flags |= _TIF_NEED_RESCHED`。

實際設定的函式：

```c
// include/linux/sched.h:2041-2044
static inline void set_tsk_need_resched(struct task_struct *tsk)
{
	set_tsk_thread_flag(tsk,TIF_NEED_RESCHED);
}
```

第 3 講看過 timer interrupt 會呼叫 `update_process_times()` 檢查要不要換人——如果要，就是用這個函式把 `TIF_NEED_RESCHED` 這**一個位元**設起來。等到從 kernel 回使用者空間前，kernel 檢查這個位元，是 1 就去排程。

---

## 5. Task 的狀態

`[24:32]` process 的狀態**至關重要**，是**非常基本**的概念，所以任何 process 描述子都把它放在**最上面**。

> 【口頭】「我們會一再回來看這些狀態。如果現在沒有 100% 懂，**不要怪自己**，有些概念有點難。你會一再看到它們，有足夠時間複習。」

【課本補充】課本說（p.70）：排程的世界裡，單執行緒 process 或多執行緒 process 裡的一個 thread，都叫 **task**。**task 是排程的基本單位。**

### 5.1 `TASK_RUNNING`：跑中和準備好是同一個狀態

`[25:32]` **`TASK_RUNNING`** 表示：**正在跑**，或**準備好要跑**。

- Linux 為**每顆 CPU** 維護一個 task 佇列，叫 **run queue（執行佇列）**。
- 在 run queue 裡 = 「我準備好了」，但一顆 CPU 一次只能跑一個 task，所以我準備好、但沒在跑——狀態還是 **`TASK_RUNNING`**。
- 排程器讓我執行、我在跑——狀態**還是** `TASK_RUNNING`。
- 【口頭】「我們會一直用 scheduler 這個字……（英式發音）」

`[26:31]` 有其他變數會記錄我是否真的在跑，但**用同一個狀態**有好處：**不用改很多次狀態**。

`[26:31]` 被**搶占（preempt）**：

- timer interrupt 來了，排程器覺得我跑太久了，該給別人機會。
- 我被搶占，回到 run queue。
- **狀態還是 `TASK_RUNNING`**，完全沒變。

【課本補充】課本補充（p.70–71）：

- 這有**歷史原因**，也有**效率上的常識原因**。
- 跑中和準備好的 task 用同一個狀態，就可以用**同一個佇列**管理。簡化很多設計，減少狀態更新。
- 特別是 context switch 時，**被換出去的 task 不用改狀態**。
- 有人會說這造成**歧義**。某種程度是真的，但它簡化了很多事，實務上不是大問題。

### 5.2 程式怎麼結束：硬體不知道

`[27:00]` 程式執行完了，會怎樣？

> 【口頭】「硬體很**簡單、天真、笨**——三者都是。它只看 PC、讀指令、照指令做。是分支就跳。**硬體永遠不知道程式什麼時候結束**。就像飛機在跑道上：你可以說不停下來，讓它衝出去撞毀——但那永遠是個壞主意。所以要在跑道盡頭前停下來。」

`[28:00]` 你會說：「我寫了 `main`，放了左大括號、右大括號，到這裡就結束了。」

- 但**編譯器**會加很多**你沒寫的程式碼**：`main` **之前**和 `main` **之後**都有。
- `main` 之前：設定 memory map、把常數複製到記憶體映像。
- `main` 之後：最重要的——發一個 **`exit` system call**。

`[29:01]` **程式用 system call 通知 OS「我做完了」**，因為硬體沒辦法知道。

另一種情況：發生 exception，OS 決定終止我（「我是壞程式」）。

`[30:31]` `main` 結尾寫 `return 0;` 會怎樣？`main` 只是**程式自己的觀點**裡的主函式，不是**實際執行的程式碼**的觀點。編譯器和連結器加了很多函式（`_start` 等，第 4 講），`main` 回傳後，最後一定會呼叫 `exit` system call。

### 5.3 Zombie：等父 process 收屍

`[29:32]` 兩種結束方式最後都會變成 **`TASK_ZOMBIE`（殭屍）**：

> 【口頭】「zombie 像鬼：不完全死、也不完全活。」

為什麼要留著？

`[30:01]` 子 process 結束時，**父 process 會被通知**。

- `exit` system call 會傳一個**整數**參數：**exit code（結束碼）**。
- 慣例：**0 = 一切正常**；其他值代表各種錯誤（負值、有時正值也可以）。
- **父 process 的工作是收這個整數**，了解子 process 成不成功。

`[30:31]` 【口頭】「只有在那之後，task 才**得到解脫（salvation）**，從 zombie 被釋放。」

`[35:31]` 流程：

```
 子 process 呼叫 exit(code)
        │
        ▼
 變成 zombie ──── SIGCHLD signal ───► 父 process
        │                                │
        │                                ▼
        │                        呼叫 wait system call
        │                        讀子 process 的 exit code
        │                                │
        ▼                                ▼
 子 process 的狀態完全清除          父 process 從 wait 之後繼續
```

1. 子 process 呼叫 `exit`，變成 zombie。
2. 父 process 收到 **`SIGCHLD`** signal。
3. 父 process 要呼叫 **`wait`** system call 讀子 process 的值。
4. 讀到了 → 父 process 繼續（`wait` 之後），子 process 的狀態**完全清除**。

`[37:01]` 這樣父子之間還有一點**同步**：就算子 process 結束了，它的回傳值還是會到父 process 手上。這也凸顯 process 是**有階層**的：使用者層最上面是 `init`，你產生（spawn）的新 process 就是你的子 process。

`[37:32]` **exit code 的慣例**：

| exit code | 意思 |
|---|---|
| **0** | 一切正常，執行成功 |
| **1** | **非特定錯誤**：出事了，但不告訴你是什麼 |
| 其他 | 精確的錯誤原因。例如 19 對應某個錯誤碼。可以看 **`errno.h`**（error number）：很多標準呼叫都定義了「這個值代表發生了什麼」——非法指令、非法記憶體位址等 |

教授說這「介於規則和慣例之間」：很多 kernel task 把它當規則用。

`[39:01]` 「除非父 process 讀了子 process 的狀態，否則子 process **得不到解脫**。」

> 【口頭】「你看，process 的世界裡也有解脫。結束後你還是 zombie，直到把狀態送出去、父 process 記下來，OS 才能清理這個 process 的狀態。」

### 5.4 Stopped：`SIGSTOP` 與 Ctrl+Z

`[32:01]` 另一種：**明確地停下**一個 process。

| 方式 | Signal | 能被忽略嗎？ |
|---|---|---|
| `kill -STOP pid`（或 `kill` system call） | **`SIGSTOP`** | **不能**——特殊 signal，優先權很高 |
| 終端機按 **Ctrl+Z** | **`SIGTSTP`**（⚠️ 教授一度說「Sig STP」，正確名稱是 `SIGTSTP`） | **可以**——process 可以明確選擇忽略 |

`[32:31]` process **當場凍結**，進入 **`TASK_STOPPED`**。它會**永遠**待在那裡，直到收到 **`SIGCONT`**（continue）才恢復。

`[40:00]` **`kill`** 既是**命令列指令**，也是 **system call**，做同一件事：**送一個 signal 給一個 process**。

- 給 process ID（一個數字）和要送的 signal。
- 「我是使用者，當然不能停 kernel process。但我自己跑的 process，可以讓它**當場停下**。」

`[41:01]` 恢復：

- 用同樣的 `kill` 機制送 **`SIGCONT`**。
- 或用 **`fg`** 指令（foreground，前景）：把暫停的 process 拉回執行狀態。

### 5.5 【Q&A】按了 Ctrl+Z 之後呢？

`[33:02]`

**學生問**：按了 Ctrl+Z 之後會怎樣？

**教授答**：送出 `SIGSTOP` 或相關 signal 的那一刻，process **當場凍結**。`SIGCONT` 是讓它恢復的方式。

為什麼要凍結？`[33:32]`

- process 在**等別的事**，那件事還沒發生。
- 這比較像**使用者**產生的操作。

【課本補充】課本補充（p.71）：

- 想等有足夠 CPU 和記憶體時再跑某個 task，可以先暫停它。
- `SIGSTOP` 不能被丟棄或被 process 攔截，「這比較像給 **kernel** 的訊息，叫它真的暫停這個 task」。
- 恢復後 task 從**暫停的同一點**繼續。正確性不受影響，除非它依賴的環境在暫停期間改變了。

### 5.6 兩種睡眠：interruptible 與 uninterruptible

`[33:32]` process 發起一個 I/O（例如讀硬碟），會花很久：

- 處理器每秒跑**幾十億**個 cycle。
- 硬碟延遲是**毫秒**級——**幾百萬**個 cycle。

所以發起長延遲的操作時，**要去睡覺**。兩種：

| 狀態 | 比喻 | 能回應 signal 嗎？ | 怎麼醒來 |
|---|---|---|---|
| **`TASK_INTERRUPTIBLE`** | **淺眠** | **能**。在等，但你送 signal 來，我能處理，「至少部分有反應」 | 等的事情發生，**或**收到 signal |
| **`TASK_UNINTERRUPTIBLE`** | 深眠 | **不能**。「你送什麼 signal 來我都不理」 | **只有 OS 叫醒我**——也就是我要的東西到了 |

`[42:00]` 什麼時候用 uninterruptible？

- 做長時間 I/O、而且**不預期**會有 signal。
- 或者：就算有 signal，**signal handler 也要靠等待的結果**才能做事——那也得 uninterruptible。

`[35:00]` 【口頭】這些狀態是所有 OS 的基本概念，特別是 Linux，幾乎所有 Unix 也是，Windows 某種程度也是。

> 「但 Windows 是專有的，我必須承認我**知道得不多**，只知道 Google 告訴我的，而那不多。所以我說任何專有 OS 的事，都要**打好幾折**聽。開源 OS 遵循 **POSIX** 標準——一個全球標準，定義介面的標準，所以很多 OS 遵循同樣的 system call 結構和互動方式。」

【課本補充】課本說：interruptible 狀態下，其他 task 可以（透過 OS）送訊息給它，它會呼叫 **signal handler**。signal handler 是程式裡定義的函式，概念上像 interrupt handler，**差別是它在使用者空間實作**。

### 5.7 狀態圖總整理

`[41:31]`（課本 Fig 3.2）

```
                                                              執行結束
  新 task 建立                                              ┌──────────► TASK_ZOMBIE
       │                                                   │            （結束或被終止）
       ▼            排程器讓它跑                              │
  TASK_RUNNING  ─────────────────────►  TASK_RUNNING  ──────┤
  （準備好、沒在跑）◄─────────────────────  （正在跑）         │   SIGSTOP
       ▲            被搶占                    │              └──────────► TASK_STOPPED
       │                                     │              ◄──────────
       │  被 OS 叫醒                           │ 去睡覺           SIGCONT
       │                                     ▼
       └──────────────  TASK_INTERRUPTIBLE 或 TASK_UNINTERRUPTIBLE
```

| 狀態 | 什麼時候 |
|---|---|
| `TASK_RUNNING` | 準備好或正在跑 |
| `TASK_INTERRUPTIBLE` | 睡覺，可被 signal 叫醒 |
| `TASK_UNINTERRUPTIBLE` | 睡覺，只有等的事發生才醒 |
| `TASK_STOPPED` | 被 `SIGSTOP` / `SIGTSTP` 暫停，等 `SIGCONT` |
| `TASK_ZOMBIE` | 已經 `exit`，等父 process `wait` |

`[42:32]` 下一講：kernel 怎麼管它的 stack，然後是 process 優先權。

### 5.8 【原始碼】狀態常數與 `ps` 的字母

狀態常數定義在 `sched.h` 開頭：

```c
// include/linux/sched.h:84-96
/* Used in tsk->state: */
#define TASK_RUNNING			0x00000000
#define TASK_INTERRUPTIBLE		0x00000001
#define TASK_UNINTERRUPTIBLE		0x00000002
#define __TASK_STOPPED			0x00000004
#define __TASK_TRACED			0x00000008
/* Used in tsk->exit_state: */
#define EXIT_DEAD			0x00000010
#define EXIT_ZOMBIE			0x00000020
#define EXIT_TRACE			(EXIT_ZOMBIE | EXIT_DEAD)
/* Used in tsk->state again: */
#define TASK_PARKED			0x00000040
#define TASK_DEAD			0x00000080
```

**又是位元旗標**——每個狀態是**一個位元**：

| 常數 | 值（二進位） | 課本的名字 |
|---|---|---|
| `TASK_RUNNING` | `0`（沒有任何位元） | `TASK_RUNNING` |
| `TASK_INTERRUPTIBLE` | `0b0001` | 同 |
| `TASK_UNINTERRUPTIBLE` | `0b0010` | 同 |
| `__TASK_STOPPED` | `0b0100` | `TASK_STOPPED` |
| `__TASK_TRACED` | `0b1000` | （被除錯器暫停） |
| `EXIT_DEAD` | `0x10` | （完全死了，正在清理） |
| `EXIT_ZOMBIE` | `0x20` | **`TASK_ZOMBIE`** |

**跟課本不一樣的地方**：

1. 課本叫 `TASK_ZOMBIE`，v6.2.12 叫 **`EXIT_ZOMBIE`**。
2. 註解寫得很清楚：`EXIT_*` 是「**Used in tsk->exit_state**」——zombie **不放在 `__state`**，而是放在**另一個欄位 `exit_state`**（`sched.h:873`）。所以 Linux 實際上有**兩個**狀態欄位：一個管「活著時在幹嘛」，一個管「死了沒、怎麼死」。

用位元旗標的好處：可以**組合**。

```c
// include/linux/sched.h:115-116, 122
#define TASK_KILLABLE			(TASK_WAKEKILL | TASK_UNINTERRUPTIBLE)
#define TASK_STOPPED			(TASK_WAKEKILL | __TASK_STOPPED)
...
#define TASK_NORMAL			(TASK_INTERRUPTIBLE | TASK_UNINTERRUPTIBLE)
```

- **`TASK_KILLABLE`** = uninterruptible + 「但**致命 signal**（例如 `SIGKILL`）可以叫醒」。這是介於教授說的兩種睡眠**之間**的第三種：一般 signal 不理，但 `kill -9` 還是能殺掉它。這是較新的設計，解決「uninterruptible 的 process 怎樣都殺不掉」的老問題。
- `TASK_NORMAL` = 兩種睡眠都算，`wake_up()` 用它一次叫醒兩種。

**`ps` 指令顯示的字母**從哪來？

```c
// fs/proc/array.c:125-135
static const char * const task_state_array[] = {

	/* states in TASK_REPORT: */
	"R (running)",		/* 0x00 */
	"S (sleeping)",		/* 0x01 */
	"D (disk sleep)",	/* 0x02 */
	"T (stopped)",		/* 0x04 */
	"t (tracing stop)",	/* 0x08 */
	"X (dead)",		/* 0x10 */
	"Z (zombie)",		/* 0x20 */
	"P (parked)",		/* 0x40 */
```

| `ps` 字母 | 對應 | 教授的名字 |
|---|---|---|
| **R** | `0x00` | `TASK_RUNNING` |
| **S** | `0x01` | `TASK_INTERRUPTIBLE`（sleeping） |
| **D** | `0x02` | `TASK_UNINTERRUPTIBLE`——「**disk sleep**」，因為最常見的原因就是**等硬碟** |
| **T** | `0x04` | `TASK_STOPPED` |
| **Z** | `0x20` | zombie |

每個註解的值就是 §5.8 開頭那些常數。`ps` 和 `/proc/<pid>/status` 的 `State:` 那一行就是查這張表印出來的。

### 5.9 【原始碼】`exit_notify()`：變成 zombie

`exit` system call 最後會走到 `do_exit()` → `exit_notify()`：

```c
// kernel/exit.c:723-763（節錄）
static void exit_notify(struct task_struct *tsk, int group_dead)
{
	bool autoreap;
	struct task_struct *p, *n;
	LIST_HEAD(dead);

	write_lock_irq(&tasklist_lock);
	forget_original_parent(tsk, &dead);

	if (group_dead)
		kill_orphaned_pgrp(tsk->group_leader, NULL);

	tsk->exit_state = EXIT_ZOMBIE;
	if (unlikely(tsk->ptrace)) {
		...
	} else if (thread_group_leader(tsk)) {
		autoreap = thread_group_empty(tsk) &&
			do_notify_parent(tsk, tsk->exit_signal);
	} else {
		autoreap = true;
	}

	if (autoreap) {
		tsk->exit_state = EXIT_DEAD;
		list_add(&tsk->ptrace_entry, &dead);
	}
	...
	write_unlock_irq(&tasklist_lock);

	list_for_each_entry_safe(p, n, &dead, ptrace_entry) {
		list_del_init(&p->ptrace_entry);
		release_task(p);
	}
}
```

| 行 | 意思 | 對應教授說的 |
|---|---|---|
| 729 | 鎖住整個 task 串列 | 要改 process 樹，不能讓別人同時改（鎖是第 5 章） |
| 730 | `forget_original_parent`：我要死了，**我的子 process 怎麼辦？** 把它們交給別人（通常是 `init` 或指定的「收養者」） | 樹不能斷：每個 process 都要有父 process |
| **735** | **`tsk->exit_state = EXIT_ZOMBIE;`** | **變成 zombie**——設的是 `exit_state`，不是 `__state` |
| 742–744 | 如果我是 thread group 的領袖、而且群組裡其他 thread 都結束了 → **`do_notify_parent`**：通知父 process | 父 process 收到 **`SIGCHLD`**（`exit_signal` 通常就是 `SIGCHLD`） |
| 745–746 | 不是領袖的 thread → `autoreap = true` | 非領袖 thread **不用等 `wait`**，直接清理 |
| 749–752 | 如果可以自動收屍（例如父 process 設定忽略 `SIGCHLD`）→ 直接變 `EXIT_DEAD` | 跳過 zombie 階段 |
| 759–762 | 對 `dead` 串列裡每個 task 呼叫 `release_task` | 真正釋放 |

所以教授說的「zombie 等父 process `wait`」，在原始碼裡有兩個例外：**非領袖 thread** 和**父 process 不在乎 `SIGCHLD`** 的情況，會跳過 zombie 直接清理。

### 5.10 【實測】製造一個 zombie、暫停一個 process

我寫了一個小程式（已在本機編譯執行）：子 process 用 exit code **19** 結束，父 process 先**不** `wait`，看看子 process 變成什麼。

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void)
{
	pid_t pid = fork();
	if (pid == 0)
		exit(19);                 /* 子行程：馬上用 exit code 19 結束 */

	sleep(1);                         /* 父行程：先不 wait，子行程變 zombie */
	char cmd[64];
	snprintf(cmd, sizeof(cmd), "ps -o pid,ppid,stat,cmd -p %d", pid);
	system(cmd);

	int status;
	waitpid(pid, &status, 0);         /* 收屍：讀 exit code */
	printf("child exit code = %d\n", WEXITSTATUS(status));
	system(cmd);                      /* 再看一次：已經不見了 */
	return 0;
}
```

輸出：

```text
    PID    PPID STAT CMD
 209619  209618 Z    [zombie] <defunct>
    PID    PPID STAT CMD
child exit code = 19
```

- 第一次 `ps`：子 process 的 STAT 是 **`Z`**，名稱後面有 **`<defunct>`**（已故）——它已經結束了，但還在 process 表裡，等父 process 收屍。
- `waitpid` 拿到 exit code **19**。
- 第二次 `ps`：**什麼都沒有了**——子 process 得到解脫。

**暫停與恢復**：

```bash
sleep 100 &                         # 背景跑一個 sleep
grep State /proc/$!/status          # State:  S (sleeping)
kill -STOP $!                       # 送 SIGSTOP
grep State /proc/$!/status          # State:  T (stopped)
kill -CONT $!                       # 送 SIGCONT
grep State /proc/$!/status          # State:  S (sleeping)
```

- `sleep` 在睡覺，所以是 **S**（interruptible）。
- `SIGSTOP` 之後變 **T**（stopped），bash 還印出 `[1]+ Stopped sleep 100`。
- `SIGCONT` 之後回到 **S**。

---

## 6. 一頁速查表

### 6.1 Process 與 thread

| 觀念 | 一句話 |
|---|---|
| Process | 程式的執行實例；擁有 CPU 時間、記憶體、檔案、連線 |
| Process 樹 | 最上面是 `init`；每個 process 只有一個父 process |
| Thread | 一群 process 的一員；共享 code、heap、data、bss |
| Thread 私有 | stack、TLS、暫存器 |
| Thread 別名 | lightweight process、software thread |
| 為什麼要 thread | 寫平行程式、用滿多核心（例如網頁伺服器） |
| tgid | thread group ID；`getpid()` 回傳的是 tgid |

### 6.2 `task_struct`

| 項目 | 內容 |
|---|---|
| 位置 | `include/linux/sched.h`，第 737 行起 |
| Linux 叫法 | task（所以是 `task_struct`） |
| 第一個欄位 | `thread_info`（x86 嵌在裡面） |
| 狀態 | `__state`（活著）＋ `exit_state`（zombie / dead） |
| Kernel stack | `stack`：在 kernel 裡的「小公寓」 |
| 關係 | `real_parent`、`parent`、`children`、`sibling`、`group_leader` |
| 安全 | `randomized_struct_fields_start` 以下欄位順序隨機 |
| 教授的重點 | 難的是**表示資訊**，不是演算法 |

### 6.3 `thread_info`

| 項目 | 內容 |
|---|---|
| 性質 | **低階**資料結構：在乎對齊、存放位置 |
| 欄位 | `flags`、`syscall_work`（long）、`status`、`cpu`（u32） |
| 技巧 | 很多旗標塞進一個整數，第 i 位元 = 第 i 個旗標 |
| 例子 | `TIF_SIGPENDING`（2）、`TIF_NEED_RESCHED`（3） |
| 現況 | 舊解法；新的用 `gs` 存 `current` |

### 6.4 狀態

| 狀態 | `ps` | 意思 | 怎麼離開 |
|---|---|---|---|
| `TASK_RUNNING` | R | 準備好或正在跑 | 睡覺、停止、結束 |
| `TASK_INTERRUPTIBLE` | S | 淺眠 | 事件發生或收到 signal |
| `TASK_UNINTERRUPTIBLE` | D | 深眠（常是等硬碟） | 只有事件發生 |
| `TASK_KILLABLE` | D | 深眠但致命 signal 能叫醒 | 事件或 `SIGKILL` |
| `TASK_STOPPED` | T | 暫停 | `SIGCONT` / `fg` |
| zombie（`EXIT_ZOMBIE`） | Z | 已 `exit`，等父 `wait` | 父 process `wait` |

### 6.5 結束流程與 signal

| 項目 | 內容 |
|---|---|
| 結束方式 | 呼叫 `exit` system call（`main` 回傳後編譯器加的程式碼會呼叫） |
| 為什麼要 `exit` | 硬體不知道程式何時結束 |
| exit code | 0 = 成功、1 = 非特定錯誤、其他看 `errno.h` |
| 通知父 process | `SIGCHLD` |
| 父 process 收屍 | `wait` / `waitpid` |
| 暫停 | `SIGSTOP`（不能忽略）、`SIGTSTP` / Ctrl+Z（可以忽略） |
| 恢復 | `SIGCONT`、`fg` |
| 送 signal | `kill` 指令 / `kill` system call |

### 6.6 本講看過的原始碼位置

| 檔案:行號 | 看什麼 |
|---|---|
| `include/linux/sched.h:737-762` | `task_struct` 開頭 |
| `include/linux/sched.h:961-985` | `pid`、`tgid`、父子兄弟欄位 |
| `include/linux/sched.h:873` | `exit_state` |
| `arch/x86/Kconfig:288` | x86 選 `THREAD_INFO_IN_TASK` |
| `arch/x86/include/asm/thread_info.h:56-63` | `struct thread_info` |
| `arch/x86/include/asm/thread_info.h:81-106` | `TIF_*` 旗標位元 |
| `include/linux/sched.h:2041-2044` | `set_tsk_need_resched()` |
| `include/linux/sched.h:84-122` | 狀態常數、`TASK_KILLABLE` |
| `fs/proc/array.c:125-135` | `ps` 的狀態字母 |
| `kernel/exit.c:723-763` | `exit_notify()`：變成 zombie、通知父 process |

---

## 7. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| **Kernel stack、`thread_info` 放在 stack 底部的舊做法、優先權** | **第 8 講**：kernel stack 與優先權（下一講） |
| **`mm_struct`、記憶體區域** | **第 9 講**：`mm_struct` 與 maple tree |
| **PID、tgid、process 樹、namespace** | **第 10 講**：PID 與 namespace |
| **`fork`、`exec`、`exit`、`wait`、zombie** | **第 11 講**：process 建立 |
| **run queue、搶占、`TIF_NEED_RESCHED`** | **第 12–13 講**：context switch；**第 24–26 講**：排程 |
| **Signal（`SIGCHLD`、`SIGSTOP`、`SIGCONT`）、signal handler** | **第 17 講**：signal |
| **Thread、共享 heap、TLS** | **第 18 講**：多執行緒 |
| **kernel thread 是 process 的「化身」** | **第 22 講**：kernel 裡的並行（第 5 章） |
| **`tasklist_lock`、`write_lock_irq`** | **第 20–22 講**：讀寫鎖、kernel 裡的並行 |

---

## 附：本講一句話總結

> **Process 是程式的執行實例，Linux 叫它 task，把它的一切（狀態、kernel stack、優先權、記憶體、PID、父子關係）都塞進一個巨大的 `task_struct`；難的不是演算法而是怎麼表示這些資訊——像 `thread_info` 把一堆旗標塞進一個整數。task 的狀態裡，跑中和準備好同樣是 `TASK_RUNNING`，睡眠分可被 signal 叫醒和不可叫醒兩種，而結束的 process 會先變成 zombie，等父 process 用 `wait` 讀走 exit code 才得到解脫。**

---

*筆記依據 Smruti R. Sarangi「Operating Systems: A Linux Kernel-Oriented Approach」Lecture 7（Chapter 3, Part I）錄影逐字稿（YouTube 自動字幕，43:15）、課本 *Operating Systems: A Linux Kernel-Oriented Approach*（Sarangi, v0.96）第 3 章開頭與 §3.1.1–3.1.4，以及 Linux kernel v6.2.12 原始碼整理。實測為本機（Ubuntu、kernel 6.5.0）結果。*
