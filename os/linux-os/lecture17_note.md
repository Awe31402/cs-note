# Operating Systems: A Linux Kernel-Oriented Approach 第十七講完整筆記：Signal（Chapter 4, Part IV）

> **課程**：Operating Systems: A Linux Kernel-Oriented Approach — Lecture 17（Chapter 4, Part IV）
> **講者**：Smruti R. Sarangi
> **來源**：YouTube 錄影逐字稿（54:37，自動字幕）＋ 課本 *Operating Systems: A Linux Kernel-Oriented Approach*（Sarangi, v0.96）§4.4–4.5（p.145–158）＋ Linux kernel **v6.2.12** 原始碼（`os/linux-src/`）
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
- [1. Signal：方向相反的訊息](#1-signal方向相反的訊息)
- [2. 範例程式](#2-範例程式)
  - [2.1 程式碼](#21-程式碼)
  - [2.2 逐行說明](#22-逐行說明)
  - [2.3 `signal` 與 `kill`：名字取得不好](#23-signal-與-kill名字取得不好)
  - [2.4 【實測】跑一次](#24-實測跑一次)
- [3. 常見的 signal](#3-常見的-signal)
  - [3.1 【原始碼】signal 編號](#31-原始碼signal-編號)
- [4. 送 signal 的規則](#4-送-signal-的規則)
  - [4.1 `kill`、`tkill`、`tgkill`](#41-killtkilltgkill)
  - [4.2 排隊、封鎖、同類型只能有一個](#42-排隊封鎖同類型只能有一個)
  - [4.3 Thread group：誰來處理](#43-thread-group誰來處理)
  - [4.4 【原始碼】「同類型只能一個」只限一般 signal](#44-原始碼同類型只能一個只限一般-signal)
  - [4.5 【實測】送 5 次，處理幾次？](#45-實測送-5-次處理幾次)
- [5. 收到 signal 之後：四種處理方式](#5-收到-signal-之後四種處理方式)
  - [5.1 忽略、終止、core dump、停止](#51-忽略終止core-dump停止)
  - [5.2 SIGKILL 與 SIGSTOP：不能擋、不能忽略、不能處理](#52-sigkill-與-sigstop不能擋不能忽略不能處理)
  - [5.3 【原始碼】kernel 專屬的 signal 與會產生 core dump 的 signal](#53-原始碼kernel-專屬的-signal-與會產生-core-dump-的-signal)
- [6. Kernel 的資料結構](#6-kernel-的資料結構)
  - [6.1 `task_struct` 裡的欄位](#61-task_struct-裡的欄位)
  - [6.2 Signal context：使用者空間的中斷](#62-signal-context使用者空間的中斷)
  - [6.3 `signal_struct`](#63-signal_struct)
  - [6.4 `sighand_struct` 與 `k_sigaction`](#64-sighand_struct-與-k_sigaction)
  - [6.5 `sigpending` 與 `sigqueue`：混合型別的串列](#65-sigpending-與-sigqueue混合型別的串列)
  - [6.6 【原始碼】這些結構](#66-原始碼這些結構)
- [7. 進入與離開 signal handler](#7-進入與離開-signal-handler)
  - [7.1 存 context 到使用者 stack：signal frame](#71-存-context-到使用者-stacksignal-frame)
  - [7.2 關鍵：偷改返回位址](#72-關鍵偷改返回位址)
  - [7.3 `__restore_rt` 與 `rt_sigreturn`](#73-__restore_rt-與-rt_sigreturn)
  - [7.4 【原始碼】`x64_setup_rt_frame` 與 `rt_sigreturn`](#74-原始碼x64_setup_rt_frame-與-rt_sigreturn)
  - [7.5 【實測】`strace` 看整個流程](#75-實測strace-看整個流程)
- [8. 第 4 章總結](#8-第-4-章總結)
- [9. 一頁速查表](#9-一頁速查表)
- [10. 與後續講次的連結](#10-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 這堂課要做什麼

`[00:34]` 第 4 章最後一講：**signal handler**。

| # | 主題 | 對應課本 |
|---|---|---|
| 1 | signal 是什麼、範例程式 | §4.4.1 |
| 2 | 送 signal 的規則、常見 signal、處理方式 | §4.4.2 |
| 3 | kernel 的資料結構 | §4.4.3 |
| 4 | 進入和離開 signal handler（signal frame、`sigreturn`） | §4.4.4 |
| 5 | 第 4 章總結 | §4.5 |

---

## 1. Signal：方向相反的訊息

`[00:34]` signal 跟之前學的**很不一樣**：

| 機制 | 方向 | 誰發起 |
|---|---|---|
| System call | 應用程式 → **OS** | 應用程式送命令或資訊給 OS |
| Exception | 應用程式 → **OS** | 應用程式做了什麼、出錯了 |
| Interrupt | 硬體 → **OS** | 硬體送訊息給 OS |
| **Signal** | **kernel → 應用程式** | **方向反過來** |

> 「前面那些都是**送進 OS**（這裡 OS 指 kernel）的訊息。signal handler 的資訊流向**相反**：從 kernel 到應用程式。某種意義上我們**反轉**了資訊流，應用程式要想辦法處理這個 signal。」

`[01:30]` 這需要一個新的 context：**signal context**——「**第五種** context」（第 16 講有四種：user、process、interrupt、softirq）。

`[02:01]` 例子（第 1、2 講也舉過）：

```
  滑鼠移動 ──中斷──► OS：算出水平位移、垂直位移
                       │
                       │ 包成 signal
                       ▼
                     應用程式：移動游標，或做它想做的事
```

`[02:32]` 「**大部分圖形化應用程式都大量使用 signal**——它們不得不。任何要跟硬體打交道的東西都要大量用 signal。」

---

## 2. 範例程式

### 2.1 程式碼

`[03:00]`（課本 Listing 4.12）

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

void handler(int signum) {
	printf("In the signal handler of process %d\n", getpid());
	exit(0);
}

int main() {
	pid_t child_pid, wpid; int status;

	signal(SIGUSR1, handler);          /* Register the handler */
	child_pid = fork();                /* Create the child */

	if (child_pid == 0) {
		printf("I am the child and I am stuck\n");
		while (1) {}
	} else {
		sleep(2);                      /* Wait for the child to get initialized */
		kill(child_pid, SIGUSR1);      /* Send the signal */
		wpid = wait(&status);          /* Wait for the child to exit */
		printf("Parent exiting, child = %d, wpid = %d, status = %d\n",
		       child_pid, wpid, status);
	}
}
```

### 2.2 逐行說明

`[03:00]` 逐行看：

| 行 | 做什麼 | 教授的說明 |
|---|---|---|
| 宣告變數 | `child_pid`、`wpid`、`status` | 「沒做什麼，就宣告幾個變數，之後會知道意思」 |
| **`signal(SIGUSR1, handler)`** | **註冊 signal handler** | `[03:31]` 「**`SIGUSR1`** 是一個**自訂** signal，不是系統定義的，可以用在任何自訂用途。把**函式指標** `handler` 跟 `SIGUSR1` 關聯起來——應用程式告訴 kernel：『如果 `SIGUSR1` 被觸發，請呼叫**我使用者空間裡**的 `handler` 函式。』」 |
| `handler` 函式 | 印 PID、`exit` | `[04:00]` 「**每個 signal handler 都有同樣的簽名**：不回傳東西，只有一個整數參數——**signal 號碼**」。`getpid` 是包著 system call 的 library call，取得目前 process 的 PID |
| **`fork()`** | 建子 process | `[05:01]` 父子做不同的事 |
| 子 process | 印一行，然後 **`while (1) {}`** | `[05:31]` 「印完訊息就開始**無窮迴圈**，子 process 卡住了」 |
| 父 process：**`sleep(2)`** | 睡 2 秒 | `[05:31]` 「告訴 OS：至少 2 秒後叫醒我。OS 可以設 timer 或鬧鐘。」為什麼要睡？`[06:00]` 「要讓子 process **設定好**、進入無窮迴圈。不然有風險子 process 還沒準備好。2 秒也**不是 100% 保證**，但這是代表性的例子，以很高的機率子 process 會設定好」 |
| **`kill(child_pid, SIGUSR1)`** | **送 signal** 給子 process | §2.3 |
| **`wait(&status)`** | 等子 process 結束 | `[07:30]` 父 process 想看子 process 的 exit status（第 7 講） |
| 印出結果 | | `[08:30]` `child_pid` 和 `wpid` 應該一樣 |

`[08:00]` 發生了什麼：

1. signal 送到子 process。**`fork` 時 signal handler 也被複製了**（第 11 講 `copy_sighand`），所以子 process 進入 `handler`。
2. 印出那行，子 process `exit`。
3. 子 process 一結束，父 process 的 `wait` 就**被釋放**（父 process 在等子 process）。
4. 父 process 印出：子 PID、`wait` 回傳的 PID（兩個一樣）、exit status。

`[09:02]` 「signal 當然是**經過 OS** 送的，不是直接送。所以可以說是一個 process **透過 kernel** 給另一個 process 送 signal，或是 kernel **直接**送——兩者等價。基本上就是某些資訊從 kernel 到使用者 process。」

【課本補充】課本補充（p.150）：

- 如果 handler **不呼叫 `exit`**、直接返回：被打斷的使用者 thread 從**暫停的地方繼續**執行。「從 thread 的觀點，這就像一次一般的 context switch。」
- 但像 **`SIGSEGV`**、**`SIGABRT`** 這種 signal，**不建議**從 handler 返回：它們通常代表程式運作有**嚴重問題**。好的做法是存狀態、清理、釋放資源、**終止**程式。
- 用 **`WEXITSTATUS`** 巨集從 `status` 取出 exit 值（低 8 位元）。

### 2.3 `signal` 與 `kill`：名字取得不好

`[06:31]` 【口頭】

> 「送 signal 是用 **`kill`** system call。你可能會跟我爭：送 signal 應該用 `signal` system call 才對吧？但因為**歷史和遺留**的原因，不是這樣。**`signal` 呼叫其實是註冊 signal handler**——理想上它應該叫『register signal handler』。**`kill` 呼叫才是送 signal**。我會比較喜歡 `register_signal` 和 `send_signal`，但事實不是這樣。」

| 呼叫 | 名字聽起來像 | 實際上做的 |
|---|---|---|
| **`signal()`** | 送 signal | **註冊** handler |
| **`kill()`** | 殺掉 process | **送** signal（任何 signal，不一定是殺掉） |

【課本補充】課本說：「`kill` 並不是字面上殺掉 process 的意思，它應該叫 `send_signal` 才對。我們決定接受這種異常吧 ☹。」命令列格式：`kill -signal pid`。

### 2.4 【實測】跑一次

```text
I am the child and I am stuck
In the signal handler of process 213154
Parent exiting, child = 213154, wpid = 213154, status = 0
```

| 觀察 | 對照 |
|---|---|
| 子 process 印了「stuck」 | 進入無窮迴圈 |
| 2 秒後，**子 process（213154）** 印出「In the signal handler」 | 正在無窮迴圈的子 process 被 signal **打斷**，跑了 handler |
| `child = 213154, wpid = 213154` | 兩個一樣：`wait` 等到的就是那個子 process |
| `status = 0` | handler 裡 `exit(0)` → exit status 0，「一切都好」 |

（教授課堂上的輸出 PID 是 1078，教授說「你們可以自己跑看看」。）

---

## 3. 常見的 signal

`[10:30]`（課本 Table 4.9）

| Signal | 號碼 | 說明 |
|---|---|---|
| `SIGHUP` | 1 | 啟動這個 process 的**終端機被關掉**時送出 |
| **`SIGINT`** | 2 | 按 **Ctrl+C** 時產生。預設動作可以被 handler 覆寫 |
| `SIGQUIT` | 3 | 終止 process，並產生 **core dump** 檔 |
| `SIGILL` | 4 | 執行了**無效的指令**，或沒有足夠權限執行那個指令。教授：「格式不正確的指令」 |
| `SIGTRAP` | 5 | 除錯用。除錯器可以設定除錯暫存器，在條件滿足時（如 breakpoint）產生這個 signal（第 16 講） |
| `SIGABRT` | 6 | 通常由 **library** 產生，表示程式內部錯誤。可以處理，但回到同一個執行點**沒意義**——錯誤很可能會再發生 |
| `SIGBUS` | 7 | 存取**無效記憶體**，通常是**對齊**問題（例如某些架構上從奇數位址讀整數） |
| **`SIGFPE`** | 8 | **算術 exception**，例如溢位或除以零。教授：「我們已經看過了，除法錯誤是其中之一」（第 16 講） |
| **`SIGKILL`** | 9 | **非常高優先權**：立即終止程式。**不能被封鎖、忽略或處理** |
| **`SIGUSR1`** | 10 | 給一般程式設計師**隨意使用** |
| **`SIGSEGV`** | 11 | 跟 `SIGBUS` 類似但**通用得多**：解參考 **NULL 指標**，或存取沒映射給 process 的記憶體。教授：「C 程式裡**經常看到的 segmentation fault**」。課本：「C/C++ 程式設計師最常遇到的記憶體錯誤」 |
| `SIGUSR2` | 12 | 跟 `SIGUSR1` 一樣，給程式設計師用 |
| `SIGPIPE` | 13 | 跟**管線（pipe）**有關：管線一端斷掉（process 結束或從沒加入）時 OS 送出 |
| **`SIGALRM`** | 14 | **timer 到期**：process 用硬體 timer 設鬧鐘，時間到 OS 送出。「像一般的鬧鐘」 |
| `SIGTERM` | 15 | 終止 process，是**禮貌地**請程式終止。**可以**被封鎖、忽略、處理 |
| `SIGSTKFLT` | 16 | 很少用了：stack fault。已被 `SIGSEGV` 取代 |
| **`SIGCHLD`** | 17 | **子 process 結束**時送給父 process（第 7 講） |
| **`SIGCONT`** | 18 | 恢復停止的 process |
| **`SIGSTOP`** | 19 | 停止 process。優先權很高，**不能被攔截、處理或忽略** |
| **`SIGTSTP`** | 20 | `SIGSTOP` 的**禮貌版**（Ctrl+Z），可以處理：應用程式做完一些記帳動作再優雅地停下 |

`[11:31]` 「我們看過**兩種**停止：`SIGSTOP` 和 `SIGTSTP`，一個是**硬停**、一個是**軟停**。很多 signal 在前幾講都看過了。」

`[11:31]` 有兩個 signal **沒有任何預設含義**：**`SIGUSR1`** 和 **`SIGUSR2`**——「可以用在任何你想要的地方，包括 process 之間互相送 signal」。

### 3.1 【原始碼】signal 編號

```c
// arch/x86/include/uapi/asm/signal.h:23-63（節錄）
#define SIGHUP		 1
#define SIGINT		 2
#define SIGQUIT		 3
#define SIGILL		 4
#define SIGTRAP		 5
#define SIGABRT		 6
#define SIGIOT		 6
#define SIGBUS		 7
#define SIGFPE		 8
#define SIGKILL		 9
...
#define SIGSEGV		11
...
#define SIGCHLD		17
#define SIGCONT		18
#define SIGSTOP		19
#define SIGTSTP		20
...
#define SIGSYS		31
...
#define SIGRTMIN	32
#define SIGRTMAX	_NSIG
```

```c
// arch/x86/include/asm/signal.h:11
#define _NSIG		64
```

| 範圍 | 是什麼 |
|---|---|
| **1–31** | **一般（標準）signal**——課本表裡的那些 |
| **32–64**（`SIGRTMIN`–`SIGRTMAX`） | **即時 signal（real-time signals）**——課本沒講，§4.4 會看到它們規則不一樣 |
| `_NSIG` = **64** | 總共 64 個——教授說的「64 個 handler」 |

`SIGIOT` 跟 `SIGABRT` **同號**（6）：歷史上的別名，又是向下相容（第 3 講）。

---

## 4. 送 signal 的規則

### 4.1 `kill`、`tkill`、`tgkill`

`[12:01]` 送 signal 有三種主要變體：

| 呼叫 | 送給誰 | 說明 |
|---|---|---|
| **`kill`** | 整個 **thread group** | 由 thread group 裡**某一個** thread 處理（§4.3） |
| **`tkill`**（thread kill） | **特定的一個** thread | |
| **`tgkill`** | 特定的一個 thread，**再加上** thread group ID | `[12:33]` 「thread ID 可能會**被重用**。`tgkill` 比較明確，是 `tkill` 的**安全版**：就算 ID 被重用了還是安全」 |

【課本補充】課本解釋 `tgkill` 的角落情況（p.152）：`tkill` 指定的 thread ID 可能被**回收**——那個 thread 結束了，然後一個新 thread 拿到同一個 PID。signal 就會送錯 thread。`tgkill` 多一個參數：**thread group ID**。「兩個**都**被回收、而且還一樣的機率**很低**。」（第 10 講：PID 是循環配置的，就是為了降低這種風險。）

課本也舉了要送給**特定** thread 的例子：某個 thread 發生算術 exception，要呼叫**那個** thread 的 handler，不可能也不應該呼叫同一個 thread group 裡另一個 thread 的 handler。

### 4.2 排隊、封鎖、同類型只能有一個

`[13:02]` signal 處理的**細微之處**：

| 規則 | 說明 | 類比 |
|---|---|---|
| **process 沒在跑時** | signal **排隊**，晚點遞送 | 「跟中斷**完全一樣**：中斷也是排隊晚點遞送」（第 15 講的 APIC 佇列） |
| **可以封鎖（block）和解除封鎖** | 被封鎖的 signal 就排隊；解除封鎖後才遞送 | 「就像中斷可以被**封鎖 / mask**」 |
| **同類型不能有兩個 pending** | `[14:00]` 「process 在睡覺，你一直送**同類型**的 signal——不能有多個同類型的」 | 「**跟 softirq 類似**」（第 16 講：每種 softirq 一個位元）。「把它想成一種**保險**：限制 kernel 裡的儲存大小」 |
| **handler 執行時，自己那種 signal 被封鎖** | `[14:33]` 例如 `SIGUSR1` 的 handler 在跑時，`SIGUSR1` 被封鎖 | 「**不鼓勵同一種 signal 並行處理**：避免 race condition、讓事情簡單」 |

`[14:33]` 所以有**兩組**被封鎖的 signal：

| 集合 | 意思 |
|---|---|
| **`real_blocked`** | **真正**不感興趣的 signal |
| **`blocked`** | 執行 handler 時，**再加上**正在處理的那種 signal（可能還有更多） |

```
  blocked = real_blocked ∪ （正在處理的 signal）∪ （sa_mask 額外指定的）

  real_blocked ⊆ blocked
```

【課本補充】Point 4.4.2：「**同類型的 signal 不能同時有兩個 pending。而且 signal handler 執行時會封鎖對應的 signal。**」handler 執行完，kernel 把 `blocked` 設回 `real_blocked`。

### 4.3 Thread group：誰來處理

`[15:31]` 

| Signal | 誰處理 |
|---|---|
| **`SIGKILL`**（要結束整群） | 送給 thread group 的**所有** thread |
| **其他** signal | 由**任何一個** thread 處理 |

`[16:01]` 【口頭】教授**更正**自己：

> 「我想稍微更正我剛才講 `kill` 的部分：送 signal 給 thread group 時，它是**選一個 thread** 來跑。我剛說『每個人都處理』**不對**。只有 thread group 裡的**一個** thread 處理 signal。每個人都處理沒什麼道理，除非你想**終止**整群 thread。」

`[16:30]` signal handler 在 thread group 裡是**共享**的——「反正整個虛擬記憶體空間都共享，很多資源都共享，signal handler 是其中之一」。

`[16:30]` 教授的三個重點：

1. **不鼓勵同一種 signal 並行處理**。
2. 送給 thread group 時，**一個** thread 處理，除非你**明確**指定是哪一個（`tkill` / `tgkill`；ID 會被重用，所以 `tgkill` 比較安全）。
3. 要**終止**整個 thread group，`SIGKILL` 會送給**每一個** thread。

【課本補充】課本說：一般 signal 送給 thread group 裡的特定 thread，但 **`SIGKILL` 和 `SIGSTOP`** 送給**所有** thread，因為它們要影響**整個** thread group：要嘛全部消滅、要嘛全部停止。「**thread group 在 signal 處理上是一個單位**」（Point 4.4.1）。

### 4.4 【原始碼】「同類型只能一個」只限一般 signal

kernel 送 signal 時的檢查：

```c
// kernel/signal.c:1074-1077
static inline bool legacy_queue(struct sigpending *signals, int sig)
{
	return (sig < SIGRTMIN) && sigismember(&signals->signal, sig);
}
```

```c
// kernel/signal.c:1099-1101（__send_signal_locked 裡）
	result = TRACE_SIGNAL_ALREADY_PENDING;
	if (legacy_queue(pending, sig))
		goto ret;
```

| 部分 | 意思 |
|---|---|
| **`sig < SIGRTMIN`** | 只有**一般 signal**（1–31）才適用 |
| `sigismember(&signals->signal, sig)` | 這種 signal 的位元**已經設了**（已經有一個 pending） |
| `result = TRACE_SIGNAL_ALREADY_PENDING;` `goto ret;` | **已經 pending 了 → 直接丟掉**這次的 signal |

**跟教授 / 課本對照**：

- 教授和課本說「**同類型不能有兩個 pending**」——對**一般 signal（1–31）**完全正確。
- 函式名字 **`legacy_queue`**（**舊式**排隊）就透露了：這是**傳統 Unix** 的行為。
- **即時 signal（32–64）不受這個限制**：它們會**真的排隊**，送幾次就處理幾次。這是 POSIX 後來加的，就是為了解決「signal 會被合併、資訊遺失」的問題。

### 4.5 【實測】送 5 次，處理幾次？

我寫的程式（已在本機執行）：先**封鎖** `SIGUSR1` 和 `SIGRTMIN`，各送 **5 次**，再解除封鎖：

```c
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

static volatile sig_atomic_t n_usr1, n_rt;
static void h_usr1(int s) { n_usr1++; }
static void h_rt(int s)   { n_rt++; }

int main(void)
{
	signal(SIGUSR1, h_usr1);
	signal(SIGRTMIN, h_rt);

	sigset_t set, old;
	sigemptyset(&set);
	sigaddset(&set, SIGUSR1);
	sigaddset(&set, SIGRTMIN);
	sigprocmask(SIG_BLOCK, &set, &old);   /* 先擋住，讓 signal 堆在 pending 裡 */

	for (int i = 0; i < 5; i++) {
		kill(getpid(), SIGUSR1);          /* 一般 signal 送 5 次 */
		kill(getpid(), SIGRTMIN);         /* 即時 signal 送 5 次 */
	}

	sigprocmask(SIG_SETMASK, &old, NULL); /* 解除封鎖，pending 的全部遞送 */
	printf("sent 5 each: SIGUSR1 handled %d time(s), SIGRTMIN handled %d time(s)\n",
	       (int)n_usr1, (int)n_rt);
	return 0;
}
```

輸出：

```text
sent 5 each: SIGUSR1 handled 1 time(s), SIGRTMIN handled 5 time(s)
```

| Signal | 送幾次 | 處理幾次 | 為什麼 |
|---|---|---|---|
| **`SIGUSR1`**（一般） | 5 | **1** | 第 2–5 次送時已經有一個 pending，被 `legacy_queue` **丟掉** |
| **`SIGRTMIN`**（即時） | 5 | **5** | 不受 `legacy_queue` 限制，**每次都排進佇列** |

教授說的「同類型只能有一個 pending」在一般 signal 上**親眼看到**了；即時 signal 是例外。

---

## 5. 收到 signal 之後：四種處理方式

### 5.1 忽略、終止、core dump、停止

`[17:30]` signal handler 是 process **向 kernel 註冊**的：「如果這個 signal 來了，請呼叫這個函式。」

> 【口頭】「你看，最主要進場的東西又是**函式指標**。signal handler 是什麼？一個函式指標。我們這整門課、特別是最近五講看到的：**函式指標已經成為 kernel 設計裡的一等公民**——到目前為止**最大**的概念。」

`[18:01]` signal 送來時：有 handler 就處理，**沒有**就做**預設動作**——「跟 exception 一樣」：

| 預設動作 | 什麼時候 | 例子 |
|---|---|---|
| **忽略（ignore）** | 不重要的 signal、沒註冊 handler | `[18:30]` 例如 `SIGUSR1` 沒有 handler 就忽略（⚠️ 實際上 `SIGUSR1` 的預設動作是**終止**；被預設忽略的是 `SIGCHLD`、`SIGURG`、`SIGWINCH` 等，見 §5.3） |
| **終止（terminate）** | 重要、一定要結束 | 「有些 signal 你**不能**忽略：要終止就得終止」 |
| **終止 + core dump** | `[18:30]` 有些情況要產生 **core dump 檔** | 例如 **segmentation fault** |
| **停止（stop）/ 繼續** | `[20:01]` | `SIGSTOP` / `SIGTSTP` 停止，`SIGCONT` 恢復 |

`[19:00]` **core dump 檔**是什麼？

> 「你一定看過 segmentation fault 之後產生這些檔案。它們包含 process **整個記憶體映像和狀態**，讓你可以用**除錯器**打開這個 core 檔、檢查 process 的狀態。process 當掉時，你會想打開它看看你的變數當時長什麼樣子。」

`[19:30]` 「任何 C++ 程式當掉、產生 core dump 檔，可以在除錯器裡打開，**有啟用符號**的話（第 16 講的 `-g`），就能看到為什麼當掉、導致當機的變數值是什麼——一定有某個 **NULL 指標**，你可以找出什麼是 NULL、其他可能牽連的變數值是什麼，幫你找到 bug。」

（第 16 講實測：除以零沒有 handler → `Floating point exception (core dumped)`。）

### 5.2 SIGKILL 與 SIGSTOP：不能擋、不能忽略、不能處理

`[20:30]` 有些**非常高優先權**的 signal，**不能**定義 handler 來忽略它：

> 「一個空的 handler 就**等於**忽略這個 signal。但如果每個 process 都能處理所有終止 signal，那**我的 process 就永遠不會結束**，整個系統就會不穩定。」

`[21:00]` **`SIGKILL`** 和 **`SIGSTOP`**：

| | 不能被忽略 | 不能被處理 | 不能被封鎖 |
|---|---|---|---|
| **`SIGKILL`** | ✅ | ✅ | ✅ |
| **`SIGSTOP`** | ✅ | ✅ | ✅ |

`[21:30]` 「它們有**所有 signal 中最高的優先權**。道理很簡單：一個是不管怎樣都要**整個 process 終止**——那忽略、處理、封鎖都沒意義。另一個 `SIGSTOP` 是要整個 process **停止**——一樣不能忽略、處理、封鎖。」

【課本補充】Point 4.4.1：「不能被處理的 signal（例如立即終止 process）是**專門由 kernel 處理**的。」課本補充（p.153）：這些 signal 被 **kernel 攔截**，thread group 裡對應的 thread 全部被終止或停止 / 恢復。

課本還提到 handler **不一定**能真正處理問題：如果 signal 是**exception** 造成的，handler 返回後**同一條**造成 exception 的指令又會執行、又出 exception。這時終止或停止出錯的 thread 是好選擇。有時 handler 可以改變造成 exception 的環境（例如重新映射一個記憶體 page、改造成 exception 的變數值）——「在 signal handler 裡做這種修改來修正執行中程式的狀態**相當危險**，只適合**黑帶程式設計師** ☺」。

### 5.3 【原始碼】kernel 專屬的 signal 與會產生 core dump 的 signal

```c
// include/linux/signal.h:418-419
#define SIG_KERNEL_ONLY_MASK (\
	rt_sigmask(SIGKILL)   |  rt_sigmask(SIGSTOP))
```

```c
// include/linux/signal.h:444-446
#define sig_kernel_only(sig)		siginmask(sig, SIG_KERNEL_ONLY_MASK)
#define sig_kernel_coredump(sig)	siginmask(sig, SIG_KERNEL_COREDUMP_MASK)
#define sig_kernel_ignore(sig)		siginmask(sig, SIG_KERNEL_IGNORE_MASK)
```

| 巨集 | 意思 |
|---|---|
| **`SIG_KERNEL_ONLY_MASK`** | **只有 `SIGKILL` 和 `SIGSTOP`**——名字就說了：「**kernel 專屬**」。程式想對它們設 handler，`sigaction` 會回錯誤 |
| `sig_kernel_only(sig)` | 檢查一個 signal 是不是 kernel 專屬的 |
| `sig_kernel_coredump(sig)` | 預設動作是否**產生 core dump** |
| `sig_kernel_ignore(sig)` | 預設動作是否**忽略** |

又是**位元遮罩**：`rt_sigmask(SIGKILL)` 就是 `1 << (SIGKILL - 1)`，每個 signal 一個位元，`|` 起來成一個集合。

會產生 core dump 的 signal：

```c
// include/linux/signal.h:425-430（節錄）
#define SIG_KERNEL_COREDUMP_MASK (\
        rt_sigmask(SIGQUIT)   |  rt_sigmask(SIGILL)    | \
	rt_sigmask(SIGTRAP)   |  rt_sigmask(SIGABRT)   | \
        rt_sigmask(SIGFPE)    |  rt_sigmask(SIGSEGV)   | \
	rt_sigmask(SIGBUS)    |  rt_sigmask(SIGSYS)    | \
        rt_sigmask(SIGXCPU)   |  rt_sigmask(SIGXFSZ)   | \
```

**`SIGFPE`** 和 **`SIGSEGV`** 都在裡面——第 16 講除以零的實測，以及教授說的「segmentation fault 產生 core dump」，都是因為這個遮罩。

---

## 6. Kernel 的資料結構

`[22:02]` 「看過應用程式這邊，現在看 **kernel 那邊**。」

> 「signal 是一個 **kernel 機制**：kernel 裡需要資料結構，**library 層級**也需要資料結構，才能寫出像上面那樣的程式。library 層級要是沒有 `kill`、`signal` 這些包著 system call 的 library call，就會很不一樣。我們會看到 library call 也做了**很多額外的工作**——包裝 system call 時都要做相當多的額外工作，標準 C library 幫我們做了。」

`[22:30]` 「C、C++ 程式設計師看到的是應用程式的觀點。JavaScript、Java 大家都用**同樣的機制**，只是包裝不同，可能看不出來是在包裝 system call。但底層程式碼一樣。每個語言都需要一個 library，每個 library 做的事大致一樣。」

### 6.1 `task_struct` 裡的欄位

`[23:32]`（課本 Listing 4.13）

| 欄位 | 型別 | 意思 |
|---|---|---|
| **`signal`** | `struct signal_struct *` | `[23:32]` **signal 資訊的頂層（apex）資料結構** |
| **`sighand`** | `struct sighand_struct *` | `[24:01]` **signal handler** 的資訊。「一個是 signal 的資訊、一個是 signal handler 的資訊，**分開**存」 |
| **`blocked`** | `sigset_t` | 目前被封鎖的 |
| **`real_blocked`** | `sigset_t` | 原本就封鎖的（`real_blocked ⊆ blocked`） |
| **`pending`** | `struct sigpending` | `[25:02]` 所有**還沒遞送**的 signal |
| `sas_ss_sp`、`sas_ss_size` | | **另一個 stack**（alternative stack）的起點和大小（§6.2） |

`[24:31]` `blocked` 和 `real_blocked`：

> 「`real_blocked` 是你**真正**不感興趣的 signal。但執行 signal handler 時，你**額外**封鎖了一些。例如 handler 在處理 `SIGUSR1`，它額外封鎖了 `SIGUSR1`，也可以多封鎖一些。所以 `blocked` 是 `real_blocked` 的**超集合**。一般不在處理 signal 時兩個可以一樣，但處理時 `blocked` 會多一些 `real_blocked` 沒有的。」

【課本補充】`sigset_t` 是**一個位元向量**：每個 signal 一個位元（第 7 講的模式）。

### 6.2 Signal context：使用者空間的中斷

`[25:32]` 「這是我們要理解的 **signal context** 很重要的部分。先用簡單的方式解釋。」

假設一個**單執行緒**的使用者 process，註冊了一個 signal handler。signal 來了，你期待它做什麼？

> 「某種意義上，它現在要**扮演 OS 的角色**。OS 會在一個獨立的 thread、用獨立的 stack 跑某個 handler，對吧？記得 OS 會分配一個 kernel stack，然後跑 handler。使用者 process 某種意義上要做**類似的事**。」

`[26:30]` 步驟：

```
  使用者程式碼在跑
        │  signal 來了
        ▼
  ① 存使用者 process 的 context（在使用者空間做一種 context switch）
        │
        ▼
  ② 執行 signal handler（在使用者空間）
        │
        ▼
  ③ 還原 context（要靠 kernel）
        │
        ▼
  使用者程式碼繼續，好像什麼都沒發生
```

`[27:01]` 「這**基本上就像使用者空間裡的一次中斷**：停下來、跑某個東西、還原。」

`[27:01]` 執行 signal handler 時，**用誰的 stack**？兩個選項：

| 選項 | 說明 |
|---|---|
| **① 同一個使用者 stack** | 讓它長大、再縮回來 |
| **② 另一個 stack** | `[27:32]` 「**完全獨立**的 stack——就像 OS 用完全獨立的 kernel stack 一樣」。指定 stack pointer 和允許的大小（`sas_ss_sp`、`sas_ss_size`）。【口頭】「**alternative** stack，不是 alternate」 |

`[28:01]` 對照：

| | Kernel 空間（system call） | 使用者空間（signal） |
|---|---|---|
| 誰 | 同一個使用者 thread，**升級**成 kernel thread | 同一個使用者 thread |
| 存 context | 存在 kernel stack | 存在**使用者** stack（§7.1） |
| 跑什麼 | system call handler | **signal handler** |
| 用的 stack | 獨立的 kernel stack | 同一個使用者 stack，或 alternative stack |
| 最後 | 還原 context、回使用者程式 | 還原 context、繼續使用者程式碼 |

`[28:31]` 「註冊 signal handler 時，可以指定用**同一個** stack 還是**另一個**。所以你現在理解的是：**signal 處理系統的架構，跟 kernel 的某些部分類似**。」

【課本補充】課本說：用另一個 stack「**沒有正確性問題**，因為原本的 thread 反正是停住的，用哪個 stack 都沒關係」。

### 6.3 `signal_struct`

`[29:00]`（課本 Listing 4.14）signal 是**整個 thread group 共享**的。

> 「signal 架構的很大一部分，**thread group 的存在扮演核心角色**，因為 thread group 裡所有 thread 都共享它。Linux 一般**不在乎** thread——每個 thread 都當成獨立的 process。但 signal 是**例外**：只有在這裡才**非常認真**看待 thread group 的成員關係。」

| 欄位 | 意思 |
|---|---|
| **`live`**（`atomic_t`） | `[30:00]` thread group 裡**活著的** thread 數量。`atomic_t` 就是**原子整數** |
| **`thread_head`** | 所有 thread 組成的**雙向 linked list** |
| **`wait_chldexit`** | `[30:31]` **wait queue**：thread 裡有誰在呼叫 `wait`（等子 process 結束），就加進這個佇列 |
| **`curr_target`** | `[31:01]` **上一個**收到並處理 signal 的 thread |
| **`shared_pending`** | `[31:01]` 整個 thread group **共享**的 pending signal 清單 |

`[31:31]` 「送給 thread group 時，一般只有一個 thread 會處理，你記住**上一次是哪個** thread 處理的。」

【課本補充】為什麼要記 `curr_target`？課本說（p.155）：這是**負載平衡**資訊。下次可以指派**另一個** thread 處理，讓所有 thread **平均地**被拖慢。也注意：pending signal 的資訊存在 **thread group 層級**，不是個別 thread。

### 6.4 `sighand_struct` 與 `k_sigaction`

`[32:00]`（課本 Listing 4.15）也是 thread group 所有 thread **共享**的。

| 欄位 | 意思 |
|---|---|
| **`count`** | `[32:32]` **引用計數** |
| **`signalfd_wqh`** | `[33:31]` 等待 **signalfd** 的佇列（見下） |
| **`action[_NSIG]`** | `[34:31]` **signal handler 表**：64 格 |

**引用計數** `[32:32]`：

> 「引用計數是 kernel 裡**非常重要**的概念：它記錄**有誰指向你**。計數是 0，就知道你不再相關——沒人指向你，可以被**垃圾回收**。」`count` 記錄有幾個 `task_struct` 指向這個 handler 結構（在幾個 `task_struct` 裡註冊了）。

**兩種送 signal 的方式** `[33:31]`：

| 方式 | 說明 |
|---|---|
| **一般方式**（非同步） | 告訴 kernel 要送 signal 給某 process，或 kernel 收到中斷決定送 signal。kernel **直接呼叫** handler（callback） |
| **特殊方式：signalfd**（同步） | `[34:00]` process P1 **寫東西到一個檔案**，這被當成一個 signal，從檔案傳給 P2（當然是 kernel 做的，但中間有一個檔案）。**FD = file descriptor**（檔案描述子）。有一個等待佇列放等著這個檔案的 process |

【課本補充】課本補充 signalfd（p.156）：這是**同步**的機制，signal 處理是**計畫好的**，不是隨時都可能來、要馬上處理。process 向 OS 註冊一個檔案描述子（signalfd 檔），OS 把 signal 的細節**寫到**這個檔案。thread 通常會**等** signal 來（排在 `signalfd_wqh`），signal 來了就被叫醒。非同步機制比較常用，因為 thread **馬上**被通知，反應比較快，特別是圖形介面。

**`action` 陣列** `[34:31]`：

> 「這是你的 **signal 處理表**：用 signal 號碼當索引，拿到一個 `k_sigaction`，它**包著**指向 handler 的指標。這**原則上跟中斷處理表、exception 處理表、system call 表一樣**——全都用**完全一樣的架構**。懂了基本概念，到處都用得上。」

`[35:31]` 核心概念：

```
  一張表（64、128、256 格）
       │ 用號碼當索引（例如 signal 37）
       ▼
  第 37 格：某個結構
       │
       ▼
  結構裡有一個函式指標 ──► 被呼叫
```

> 「函式指標在這整個故事裡成為**一等公民**，當然它被包在某個結構裡、存在一個陣列裡——那是一點**軟體工程**，不用太在意，但這是**核心**的想法。」

**`sigaction`** `[36:30]`（課本 Listing 4.16）：`k_sigaction` 包著 `sigaction`，它有三個欄位：

| 欄位 | 意思 |
|---|---|
| **`sa_handler`** | **指向 handler 的函式指標**（在 thread 的使用者空間記憶體裡） |
| **`sa_flags`** | 怎麼呼叫 signal、怎麼進入 signal context（例如要不要用另一個 stack） |
| **`sa_mask`** | `[36:30]` handler 執行時**額外遮罩**的 signal：「正在處理的那個一定被遮罩，但你可能想多遮罩幾個，因為不想在 handler 中途被打斷」 |

`[37:00]` 「基本的遮罩集合加上 `sa_mask`（做邏輯 OR），就得到 `blocked` 集合。」

### 6.5 `sigpending` 與 `sigqueue`：混合型別的串列

`[38:03]` 最後一個：存所有 **pending** signal 的結構。

> 【口頭】「這裡事情變複雜了。如果你看不懂，我**不會怪你**：一部分是因為**有點不傳統的命名**，一部分是因為 Linux kernel 建 linked list 的方式。」

`[39:03]`（課本 Fig 4.11）

```
     struct sigpending                struct sigqueue     struct sigqueue     struct sigqueue
  ┌───────────────────┐            ┌──────────────┐    ┌──────────────┐    ┌──────────────┐
  │ list ◄────────────┼────────────┼─► list ◄─────┼────┼─► list ◄─────┼────┼─► list       │
  │ （list_head）      │            │ flags        │    │ flags        │    │ flags        │
  │                   │            │ info         │    │ info         │    │ info         │
  │ signal（位元向量）  │            │（signal 19…） │    │（signal 2…）  │    │ ...          │
  └───────────────────┘            └──────────────┘    └──────────────┘    └──────────────┘
     串列的「頭」                              串列的「元素」（不同型別！）
```

`[39:31]` 「先不看投影片，看 `struct sigpending` 的**第一個成員**：它指向一個 linked list。Linux 的通用 linked list 有 next 和 prev 指標——**串列裡的元素不一定要是同一種型別**，只要你知道**鄰居是什麼型別**就好。」

`[40:01]` 這個串列的元素是 **`sigqueue`**：

> 【口頭】「**`sigqueue` 不是一個佇列（queue）**！這是混淆的主要來源。它應該叫 **sig entry**，但 kernel 不是我寫的，不能怪我。我會叫它 **sigqueue entry**——它是串列裡的一個**項目**。『queue』暗示一個串列，但它**不是**，它是一個項目。每個項目要成為串列的一部分，都要有一個 **`list` 成員**。」

`[40:31]` 「還記得講 Linux linked list 那堂課（第 14 講）嗎？有 `abc` 和 `def`，型別不同，但在**同一個串列**上。**這正是同一件事——那個例子就是為了這裡才介紹的。**」

**`sigpending.signal`：位元向量** `[41:01]`

> 「跟 softirq 類似，有 64 個定義好的 signal，資料結構要說『第 27 個 signal 有沒有設成 1』。標準做法是用一個 **unsigned long 整數**：`sigset_t` 就是一個 **64 位元**的 unsigned long。第 27 位元設成 1，代表 **signal 27 pending**。任何時刻某種類型**只能有一個** pending——跟 softirq 一樣，所以用位元向量，第 i 位元是 1 代表 signal i pending。」

`[41:30]` 位元和串列的關係：

> 「假設 `signal` 裡有 **n 個位元**是 1，串列裡就有 **n 個項目**，每個設了的位元一個。例如 signal 19 pending：第 19 位元是 1，其中一個 `sigqueue` 就是 signal 19 的。有 5 個位元是 1，就有 5 個 pending signal，串列裡就有 5 個 `sigqueue`。`sigpending` 和 `sigqueue` 是**同一個環狀雙向串列**的一部分：一個是 `sigpending`，其他都是 `sigqueue`。」

（課本 Trivia 4.4.1 說的一樣。§4.4 的即時 signal 可以排多個，所以嚴格說是「一般 signal 每個位元對應一個 `sigqueue`」。）

**`kernel_siginfo_t`（`info`）** `[43:02]`：

| 欄位 | 意思 |
|---|---|
| **signal 號碼** | 例如 19 |
| **錯誤號碼** | 如果是 segmentation fault 之類的錯誤造成的，有唯一的錯誤號碼 |
| **來源代碼** | 來源可能是 **OS**（你做了什麼、某個裝置發了中斷、OS 決定送 signal），也可能是**另一個 process** 透過 kernel 送的 |
| **送出者的 PID** | 另一個 process 送的話，**誰**送給你的 |

`[44:03]` **為什麼要包一層 `sigqueue`？**

> 「我們想建一個 **signal 資訊結構的 linked list**，但**不想在 `siginfo` 裡放 `list_head`**。所以把它包在 `sigqueue` 裡，在那裡放 `list_head`。`sigqueue` 的工作就是當**包裝**，提供 **linked list 的功能**。」

`[45:00]` 【口頭】軟體設計的評論：

> 「做這種選擇**比較像藝術而不是科學**：一個設計者建了某個東西，**其他 kernel 開發者要用它**。效率當然是首要考量，但**優雅**也是：你想讓別人用得**順**、用得**優雅**，用的方式讓他們**引入 bug 的機會很小**。就算他們不是很厲害的開發者，犯錯的機會也不多。這是任何好的程式系統的**標誌**：系統設計得讓你**就算想引入 bug 都做不到**。」

### 6.6 【原始碼】這些結構

`task_struct` 裡的欄位：

```c
// include/linux/sched.h:1098-1106（節錄）
	struct signal_struct		*signal;
	struct sighand_struct __rcu		*sighand;
	sigset_t			blocked;
	sigset_t			real_blocked;
	...
	struct sigpending		pending;
	unsigned long			sas_ss_sp;
	size_t				sas_ss_size;
```

跟課本 Listing 4.13 一模一樣。`sighand` 有 `__rcu` 標記（第 23 講）。

**handler 表**：

```c
// include/linux/sched/signal.h:20-25
struct sighand_struct {
	spinlock_t		siglock;
	refcount_t		count;
	wait_queue_head_t	signalfd_wqh;
	struct k_sigaction	action[_NSIG];
};
```

| 欄位 | 對照 |
|---|---|
| `siglock` | 課本沒列：保護 signal 相關資料的**自旋鎖**（第 5 章） |
| **`count`** | 引用計數 |
| **`signalfd_wqh`** | signalfd 的等待佇列 |
| **`action[_NSIG]`** | **64 格的 handler 表**——「跟中斷表、system call 表一樣的架構」 |

```c
// include/linux/signal_types.h:37-49
struct sigaction {
#ifndef __ARCH_HAS_IRIX_SIGACTION
	__sighandler_t	sa_handler;
	unsigned long	sa_flags;
#else
	unsigned int	sa_flags;
	__sighandler_t	sa_handler;
#endif
#ifdef __ARCH_HAS_SA_RESTORER
	__sigrestore_t sa_restorer;
#endif
	sigset_t	sa_mask;	/* mask last for extensibility */
};
```

```c
// include/linux/signal_types.h:51-56
struct k_sigaction {
	struct sigaction sa;
#ifdef __ARCH_HAS_KA_RESTORER
	__sigrestore_t ka_restorer;
#endif
};
```

| 欄位 | 意思 |
|---|---|
| **`sa_handler`** | **函式指標** |
| `sa_flags` | 旗標 |
| **`sa_restorer`** | 課本沒列在 `sigaction` 裡，但**非常重要**：handler 返回後要跳去的地方——§7.2 的 `__restore_rt`。x86 有定義 `__ARCH_HAS_SA_RESTORER` |
| **`sa_mask`** | 額外遮罩。註解：「mask 放最後，方便擴充」——`sigset_t` 大小可能會變，放最後才不會影響前面欄位的偏移 |
| `IRIX` 那段 | 不同架構（IRIX 是 SGI 的舊 Unix）欄位**順序不一樣**——又是歷史包袱 |

**pending 串列**：

```c
// include/linux/signal_types.h:32-35
struct sigpending {
	struct list_head list;
	sigset_t signal;
};
```

```c
// include/linux/signal_types.h:22-27
struct sigqueue {
	struct list_head list;
	int flags;
	kernel_siginfo_t info;
	struct ucounts *ucounts;
};
```

| 結構 | 欄位 | 角色 |
|---|---|---|
| **`sigpending`** | `list` + **`signal`**（位元向量） | 串列的**頭** |
| **`sigqueue`** | `list` + `flags` + **`info`** | 串列的**元素**——**不同型別** |

兩個都有 `struct list_head list`，所以可以串在**同一個**環狀串列上；走訪時從頭開始，用 `list_entry(..., struct sigqueue, list)`（第 14 講的 `container_of`）找回每個 `sigqueue`。`sigqueue` 的 `ucounts` 是課本沒列的：記錄**這個使用者**目前排了多少 signal，用來限制每個使用者能排的即時 signal 數量（`ulimit -i`）——不然即時 signal 可以無限排隊，會被拿來耗盡 kernel 記憶體。

---

## 7. 進入與離開 signal handler

### 7.1 存 context 到使用者 stack：signal frame

`[46:00]` 呼叫 `kill` 送 signal、OS 送出 signal——需要 **process、library 函式和 kernel 之間大量的協調**。

`[46:30]` 使用者 process 在跑時，它的 context 要存起來：

> 「把 context 存在 **kernel** 沒有意義，因為**整個工作都發生在使用者領域**。就存在**使用者 stack** 上，或如果原本就打算用另一個 stack，就存在另一個 stack。」

`[47:00]` 「這不像硬體 context 那樣要存很多東西、還要那麼小心，但還是要存。」

【課本補充】課本把存在使用者 stack 上的這個結構叫 **signal frame**（p.157）。

### 7.2 關鍵：偷改返回位址

`[47:00]` 要存的第一個東西：**返回位址**。

> 「呼叫 signal handler 時——signal handler 是一個**函式**——函式要返回到哪裡？**不應該**返回到使用者 process 被打斷的那個點，因為還有一些**額外處理**要做。」

```
  使用者 process 被打斷
       │
       ▼
  signal handler 執行
       │ return —— 返回到哪？
       ▼
  ✗ 不是使用者 process 被打斷的地方
  ✓ 一段「自訂程式碼」：還原 context 等等
       │
       ▼
  使用者 process 才繼續
```

`[48:02]` 「呼叫函式時，你想讓它返回到**某個別的點**——你指定的那個點。這做得到：」

| 指令集 | 怎麼指定返回位址 |
|---|---|
| 用**返回位址暫存器**的（如 RISC-V、ARM） | 把那個暫存器設成你想讓它返回的地方 |
| 用 **stack** 的（如 **x86**） | 把返回位址**放在 stack 上** |

`[48:33]` 返回位址通常設成 **glibc** 的一個特定函式：**`__restore_rt`**（restore return：還原 signal 的返回路徑）。

`[48:33]` 「**你呼叫一個函式，函式本身不在你的控制下，但它要返回到哪裡在你的控制下**——那是你**事先指定**的。這是**重要的洞見**。x86 上放在 stack 上，就等於事先指定好了，它就會返回到你想要的任何地方。」

`[49:00]` 存的 context（**`ucontext`**，user-level context）：

| 欄位 | 內容 |
|---|---|
| **`uc_flags`** | 一些旗標 |
| **`uc_stack`** | **stack pointer** |
| **`uc_mcontext`** | **所有暫存器和處理器狀態**的快照 |

再加上 **`siginfo`**：「跟 kernel 的 siginfo 一樣：**為什麼**要做這件事？因為收到了一個 signal：signal 號碼、細節，全在這裡。」

`[50:01]` 教授的重點：

> 「**要記住的重點**：收集並存好 context——這我們知道一定要做，不然怎麼跑 signal handler。**真正的重點是：你在偷改（hacking）返回位址，把它送到你選的地方**——因為你的返回語意允許你這樣做。」

【課本補充】課本對這個技巧的評論（p.158）：「signal 處理函式**不用用任何特別的方式寫**，它完全不知道這些改變。透過修改 stack 或返回位址暫存器來**偷改返回位址**，是一個**標準技巧**，在 OS kernel 裡有很多用途。例如常常需要在獨立的 thread 上執行一個函式，它返回到一段特別的程式碼，記錄回傳值、拆掉 thread。」（第 13 講的 **`ret_from_fork`** 就是同一招。）

### 7.3 `__restore_rt` 與 `rt_sigreturn`

`[50:32]` 從 signal handler 返回：

1. signal handler 執行完。
2. 返回到 **`__restore_rt`**（因為事先設好了）。
3. `__restore_rt` 是一個通用函式：看 handler 執行期間有沒有 exception、handler 執行得怎樣……「**最大的工作是還原 context**」。
4. `[51:00]` 但**使用者空間不能自己還原 context**——要 kernel 做。所以發一個 system call：**`sigreturn`**。
5. `sigreturn` 的工作：還原 context，讓使用者 process **從停下的地方**繼續。

`[51:30]` 細節：

- 照**相反**的順序做。
- 用 **`copy_from_user`** 函式把 context **從使用者空間複製到 kernel 空間**，讓 kernel 能還原它（kernel 知道怎麼還原）。
- 還原使用者 process 的 context：填好暫存器、設 stack pointer。
- 「就跟從 system call 返回一樣，從 signal 返回。」使用者 process 執行完這段自訂程式碼後，**從完全相同的點**開始。

```
  使用者空間                                kernel
  ──────────────────────────────────────────────────────────────
  程式在跑 ───── signal 要遞送 ──────────►  在使用者 stack 上建 signal frame：
                                            ・存 context（ucontext）
                                            ・存 siginfo
                                            ・pretcode ← __restore_rt
                                            ・rip ← handler，rdi ← signal 號碼
                ◄──────── 回到使用者空間 ───
  handler(sig) 執行
       │ ret（從 stack 取返回位址 = pretcode）
       ▼
  __restore_rt（glibc）
       │ syscall rt_sigreturn ──────────────►  copy_from_user：從 signal frame 取回 context
                                              還原所有暫存器、stack pointer、blocked
                ◄──────── sysret / iret ────
  程式從原本被打斷的地方繼續
```

【課本補充】課本的總結（p.158）：「**signal handler 是 process 裡的一個小 process**。它的生命很短，signal 處理函式執行完就不存在了，然後原本的 thread 恢復。」

### 7.4 【原始碼】`x64_setup_rt_frame` 與 `rt_sigreturn`

**signal frame 的結構**：

```c
// arch/x86/include/asm/sigframe.h:59-64
struct rt_sigframe {
	char __user *pretcode;
	struct ucontext uc;
	struct siginfo info;
	/* fp state follows here */
};
```

| 欄位 | 對照 |
|---|---|
| **`pretcode`** | **返回位址**——放在**最前面**（stack 頂端），handler 執行 `ret` 時就會從這裡取。名字是「pointer to return code」 |
| **`uc`** | **`ucontext`**：context |
| **`info`** | **`siginfo`** |
| 註解 | 後面還接著**浮點狀態** |

（課本 Listing 4.17 把 `pretcode` 列在最後，實際 x86-64 是在最前面——因為 x86 從 stack 頂端取返回位址。）

**kernel 建 signal frame、準備跳到 handler**：

```c
// arch/x86/kernel/signal_64.c:164-209（節錄）
int x64_setup_rt_frame(struct ksignal *ksig, struct pt_regs *regs)
{
	sigset_t *set = sigmask_to_save();
	struct rt_sigframe __user *frame;
	...
	/* x86-64 should always use SA_RESTORER. */
	if (!(ksig->ka.sa.sa_flags & SA_RESTORER))
		return -EFAULT;

	frame = get_sigframe(ksig, regs, sizeof(struct rt_sigframe), &fp);
	...
	/* Create the ucontext.  */
	unsafe_put_user(uc_flags, &frame->uc.uc_flags, Efault);
	unsafe_put_user(0, &frame->uc.uc_link, Efault);
	unsafe_save_altstack(&frame->uc.uc_stack, regs->sp, Efault);

	/* Set up to return from userspace.  If provided, use a stub
	   already in userspace.  */
	unsafe_put_user(ksig->ka.sa.sa_restorer, &frame->pretcode, Efault);
	unsafe_put_sigcontext(&frame->uc.uc_mcontext, fp, regs, set, Efault);
	unsafe_put_sigmask(set, frame, Efault);
	...
	/* Set up registers for signal handler */
	regs->di = ksig->sig;
	/* In case the signal handler was declared without prototypes */
	regs->ax = 0;
	...
	regs->si = (unsigned long)&frame->info;
	regs->dx = (unsigned long)&frame->uc;
	regs->ip = (unsigned long) ksig->ka.sa.sa_handler;

	regs->sp = (unsigned long)frame;
```

| 行 | 意思 | 對照教授說的 |
|---|---|---|
| 171–173 | **x86-64 一定要有 `SA_RESTORER`**，沒有就失敗 | 返回位址**一定**要被偷改 |
| 175 | `get_sigframe`：在使用者 stack（或 alternative stack）上**挖一塊空間**放 frame | 「存在使用者 stack 上，或另一個 stack」 |
| 182–184 | 填 **`ucontext`**：旗標、stack 資訊 | `uc_flags`、`uc_stack` |
| **188** | **`unsafe_put_user(ksig->ka.sa.sa_restorer, &frame->pretcode, ...)`** | **偷改返回位址**：把 `sa_restorer`（glibc 的 `__restore_rt`）寫進 frame 最前面。註解：「設好從使用者空間返回的方式。如果有提供，就用**已經在使用者空間的 stub**」 |
| 189 | `unsafe_put_sigcontext(&frame->uc.uc_mcontext, ..., regs, ...)` | **把所有暫存器（`pt_regs`）存進 `uc_mcontext`**——「所有暫存器和處理器狀態的快照」 |
| 190 | 存 signal mask | 還原時要把 `blocked` 設回去 |
| **199** | **`regs->di = ksig->sig;`** | handler 的**第 1 個參數**（`rdi`）= **signal 號碼**——「每個 handler 都有同樣的簽名：一個整數參數，signal 號碼」 |
| 205–206 | `rsi` = `&frame->info`、`rdx` = `&frame->uc` | 用 `SA_SIGINFO` 註冊的 handler 會收到這兩個額外參數（第 16 講實測的 `siginfo_t *info`） |
| **207** | **`regs->ip = (unsigned long) ksig->ka.sa.sa_handler;`** | **改 `pt_regs` 裡的 PC**：回到使用者空間時，**不是回到原本被打斷的地方，而是跳到 handler** |
| **209** | **`regs->sp = (unsigned long)frame;`** | stack pointer 指向 frame——所以 handler 執行 `ret` 時，取到的就是 `pretcode` |

**整個技巧的核心**：kernel **改寫** `pt_regs`（第 2 講：使用者程式進 kernel 時存的 context）。之後照常 `sysret` / `iret` 回使用者空間（第 13 講），CPU 就「以為」使用者程式本來就是要跑 handler。

**返回**：

```c
// arch/x86/kernel/signal_64.c:243-271
SYSCALL_DEFINE0(rt_sigreturn)
{
	struct pt_regs *regs = current_pt_regs();
	struct rt_sigframe __user *frame;
	sigset_t set;
	unsigned long uc_flags;

	frame = (struct rt_sigframe __user *)(regs->sp - sizeof(long));
	if (!access_ok(frame, sizeof(*frame)))
		goto badframe;
	if (__get_user(*(__u64 *)&set, (__u64 __user *)&frame->uc.uc_sigmask))
		goto badframe;
	if (__get_user(uc_flags, &frame->uc.uc_flags))
		goto badframe;

	set_current_blocked(&set);

	if (!restore_sigcontext(regs, &frame->uc.uc_mcontext, uc_flags))
		goto badframe;

	if (restore_altstack(&frame->uc.uc_stack))
		goto badframe;

	return regs->ax;

badframe:
	signal_fault(regs, frame, "rt_sigreturn");
	return 0;
}
```

| 行 | 意思 | 對照教授說的 |
|---|---|---|
| 243 | **`SYSCALL_DEFINE0(rt_sigreturn)`**：一個**沒有參數**的 system call | 「`sigreturn` system call」（x86-64 用 `rt_` 版本，rt = real-time，支援即時 signal 的那套介面） |
| **250** | `frame = regs->sp - sizeof(long)`：從**使用者 stack pointer** 找到 signal frame | handler `ret` 時把 `pretcode` 彈掉了，所以 frame 在 `sp` 往回一個 `long` 的位置 |
| 251–252 | **`access_ok`**：檢查這塊記憶體**真的是使用者空間**的 | 安全：不能讓使用者騙 kernel 去讀 kernel 記憶體 |
| 253 | `__get_user(...)`：從使用者空間讀出存好的 signal mask | 教授說的 **`copy_from_user`** 這一類函式 |
| **258** | **`set_current_blocked(&set)`** | 把 `blocked` **設回**原本的——handler 期間額外封鎖的那些解除了（§4.2） |
| **260** | **`restore_sigcontext(regs, &frame->uc.uc_mcontext, ...)`** | **把存好的暫存器寫回 `pt_regs`**——包括原本的 PC 和 stack pointer |
| 263 | 還原 alternative stack 設定 | |
| 266 | 回傳 `regs->ax`——被打斷前的那個 `rax` 值 | 讓使用者程式**完全感覺不到**被打斷過 |
| 268–270 | frame 有問題（例如被竄改）→ 送 `SIGSEGV` | |

之後照常從 system call 返回（`sysret` / `iret`），但 `pt_regs` 已經被換回**被打斷前**的值——程式就從原本的地方繼續。

### 7.5 【實測】`strace` 看整個流程

一個最小的程式：註冊 `SIGUSR1` 的 handler，送給自己：

```c
#include <signal.h>
#include <unistd.h>
void h(int s){}
int main(){signal(SIGUSR1,h);kill(getpid(),SIGUSR1);return 0;}
```

`strace` 輸出（節錄）：

```text
rt_sigaction(SIGUSR1, {sa_handler=0x55b3aa781189, sa_mask=[USR1],
             sa_flags=SA_RESTORER|SA_RESTART, sa_restorer=0x7fa65a042520},
             {sa_handler=SIG_DFL, sa_mask=[], sa_flags=0}, 8) = 0
kill(213172, SIGUSR1)                   = 0
--- SIGUSR1 {si_signo=SIGUSR1, si_code=SI_USER, si_pid=213172, si_uid=1000} ---
rt_sigreturn({mask=[]})                 = 0
```

| 觀察 | 對照 |
|---|---|
| **`signal()` 底下是 `rt_sigaction`** system call | §2.3：`signal` 是**註冊**；glibc 用 `rt_sigaction` 實作 |
| **`sa_handler=0x55b3aa781189`** | 我的 handler `h` 的位址（在程式的 text section） |
| **`sa_mask=[USR1]`** | glibc **自動**把 `SIGUSR1` 加進 `sa_mask`——handler 執行時**封鎖自己那種 signal**（§4.2） |
| **`sa_flags=SA_RESTORER\|...`** | §7.4 第 171 行：x86-64 **一定要** `SA_RESTORER` |
| **`sa_restorer=0x7fa65a042520`** | **glibc 的 `__restore_rt`**——位址在 `0x7fa6...`，是**共享 library（libc.so.6）**的位址範圍（第 5 講的 memory map 中間那塊），不是程式自己的。**這就是會被寫進 `pretcode` 的值** |
| 後面的 `{sa_handler=SIG_DFL ...}` | 舊的設定：**預設動作** |
| **`kill(213172, SIGUSR1)`** | §2.3：**`kill` 才是送 signal** |
| `--- SIGUSR1 {... si_code=SI_USER, si_pid=213172 ...} ---` | signal 遞送了。`si_code=SI_USER`：**來源是使用者 process**；`si_pid`：**送出者的 PID**（§6.5 的 `kernel_siginfo_t` 欄位） |
| **`rt_sigreturn({mask=[]})`** | handler 執行完、返回到 `__restore_rt`，它發了 **`rt_sigreturn`** system call；`mask=[]` 是要還原成的 `blocked`（空的） |

handler 本身是空的，`strace` 看不到它執行——但看得到它**前後**的兩個 system call：kernel 遞送 signal，以及 `__restore_rt` 的 `rt_sigreturn`。

---

## 8. 第 4 章總結

`[52:02]` 教授回到第 4 章的大綱：

| 主題 | 內容 |
|---|---|
| **System call** | 追蹤一個 library call 的完整路徑：`printf` 到底做了什麼、最後怎麼變成 system call、那個 system call 做什麼、參數怎麼傳（暫存器、stack） |
| **中斷架構** | 相當精細：APIC、LAPIC、I/O APIC |
| **Exception** | |
| **處理中斷的方法** | 「更精細」：top half、softirq、threaded IRQ、work queue——各有不同的取捨 |
| **Signal handler** | 「比較像**使用者空間的中斷**」：使用者 process 在跑，在某點停下，跑一些 handler，做完工作，在 kernel 幫忙下還原 context，再繼續 |

`[53:32]` signal 最常用在 **GUI** 軟體：

> 「如果我在寫東西——我在做什麼？這是一個滑鼠裝置，對吧？中斷被 OS 攔截，透過 **signal** 機制送給——這裡是 **PowerPoint**——PowerPoint 就把它畫在螢幕上。**這就是 signal。**」

`[54:02]` 下一章：**process 的排程**——「OS 的**第二個心臟**」。

【課本補充】課本的第 4 章摘要（Summary 4.5.1，重點整理）：

| # | 摘要 |
|---|---|
| 1 | library call 包裝 system call：準備參數、呼叫、處理回傳值、通知應用程式 |
| 2 | `printf` 這種 library call 實作成好幾層，每層處理前一層的輸出，最後產生**一個字串**當 system call 的參數 |
| 3 | x86 上 system call 號碼放 `rax`，其他 6 個參數放其他暫存器，更多的放使用者 stack |
| 4 | 用 `syscall` 指令進 kernel；切換模式後先存 context，再開始處理 system call |
| 5 | 中斷和 exception 有一個 8 位元的號碼叫**中斷向量**，用來存取 **IDT**，每格存一個指向 `irq_desc` 的指標 |
| 6 | 每個裝置有一條中斷線 **IRQ**（實體銅線或虛擬的），接到 **APIC**。APIC 偵測到電壓變化或收到訊息時觸發 IRQ |
| 7 | APIC 階層化組織（**domain**）：少數全系統的 I/O APIC，每個核心一個 LAPIC。LAPIC 把中斷向量送給 CPU，也提供 timer |
| 8 | 中斷向量只有 256 個，但主機板可能有更多裝置和 IRQ → 要動態對應 IRQ 到向量，也要多個裝置**共用** IRQ |
| 9 | 中斷來時，從 IDT 拿到 IRQ 的指標，**詢問每個**接在這條 IRQ 上的裝置有沒有發中斷，是的話呼叫它 driver 的 handler |
| 10 | interrupt handler 叫 **top half**，優先權**比所有一般和即時 task 都高**。不能拿鎖、存取使用者空間、動態配置記憶體；執行時中斷是關的。一般性的工作要延後給 **bottom half** |
| 11 | exception 時 kernel 可以：(a) 送 signal；(b) `printk` 寫 log；(c) double fault → panic；(d) 動態二進位轉譯；(e) `notify_die` |
| 12 | bottom half 三種：**softirq**（可以在 top half 結束後馬上在 interrupt context 執行，中斷是開的，優先權比所有 top half 低）、**threaded IRQ**（即時優先權通常 50）、**work queue**（最通用，worker thread 用一般 kernel process 優先權） |
| 13 | **`signal` 註冊 signal handler，`kill` 送 signal** |
| 14 | 任何時刻某種 signal **只能有一個**被觸發；kernel 有精細的資料結構存所有 signal 資訊：等待的 thread、pending 的 signal、註冊的 handler |
| 15 | 每個 signal handler 都**返回到一個自訂位置**、執行 `__restore_rt`，它發 **`sigreturn`** system call：把存在使用者 stack 的 context 複製到 kernel 空間，由 kernel 還原使用者 process 的 context 並恢復它 |

---

## 9. 一頁速查表

### 9.1 Signal 的本質

| 項目 | 內容 |
|---|---|
| 方向 | **kernel → 應用程式**（其他機制都是 → kernel） |
| 本質 | 使用者空間的中斷；第五種 context |
| 用途 | GUI（滑鼠、鍵盤事件）、process 間溝通、通知錯誤 |
| handler 簽名 | `void handler(int signum)` |
| fork 時 | handler 被複製（`copy_sighand`） |

### 9.2 API

| 呼叫 | 實際做的 |
|---|---|
| `signal()` / `sigaction()` | **註冊** handler（底下是 `rt_sigaction`） |
| `kill(pid, sig)` | **送** signal 給 thread group（一個 thread 處理） |
| `tkill(tid, sig)` | 送給特定 thread |
| `tgkill(tgid, tid, sig)` | 安全版 `tkill`，防 ID 回收 |
| `sigprocmask()` | 封鎖 / 解除封鎖 |
| `sigreturn` / `rt_sigreturn` | handler 結束後還原 context（glibc 的 `__restore_rt` 呼叫） |

### 9.3 重要 signal

| # | 名稱 | 預設 | 備註 |
|---|---|---|---|
| 2 | `SIGINT` | 終止 | Ctrl+C |
| 8 | `SIGFPE` | core dump | 除以零 |
| **9** | **`SIGKILL`** | 終止 | **不能擋 / 忽略 / 處理** |
| 10, 12 | `SIGUSR1/2` | 終止 | 自訂用途 |
| 11 | `SIGSEGV` | core dump | segmentation fault |
| 14 | `SIGALRM` | 終止 | 鬧鐘 |
| 15 | `SIGTERM` | 終止 | 禮貌地請結束 |
| 17 | `SIGCHLD` | 忽略 | 子 process 結束 |
| **19** | **`SIGSTOP`** | 停止 | **不能擋 / 忽略 / 處理** |
| 20 | `SIGTSTP` | 停止 | Ctrl+Z，可處理 |
| 32–64 | 即時 signal | 終止 | **會真的排隊** |

### 9.4 規則

| 規則 | 內容 |
|---|---|
| 沒在跑 | 排隊、晚點遞送 |
| 封鎖 | 排隊，解除封鎖再遞送 |
| 同類型 | 一般 signal（1–31）只能一個 pending，多的**丟掉**（`legacy_queue`）；即時 signal 會排隊。實測：送 5 次，`SIGUSR1` 處理 1 次、`SIGRTMIN` 處理 5 次 |
| handler 執行時 | 自己那種 signal 被封鎖；`blocked = real_blocked ∪ 自己 ∪ sa_mask` |
| thread group | 一個 thread 處理（記 `curr_target` 做負載平衡）；`SIGKILL` / `SIGSTOP` 給所有 thread |
| 預設動作 | 忽略、終止、終止 + core dump、停止 |

### 9.5 Kernel 資料結構

| 結構 | 重點欄位 | 範圍 |
|---|---|---|
| `task_struct` | `signal`、`sighand`、`blocked`、`real_blocked`、`pending`、`sas_ss_sp/size` | 每個 thread |
| `signal_struct` | `live`、`thread_head`、`wait_chldexit`、`curr_target`、`shared_pending` | thread group 共享 |
| `sighand_struct` | `count`（引用計數）、`signalfd_wqh`、**`action[64]`** | thread group 共享 |
| `k_sigaction` / `sigaction` | **`sa_handler`**、`sa_flags`、`sa_mask`、**`sa_restorer`** | 每個 signal 一個 |
| `sigpending` | `list`（頭）、**`signal`**（位元向量） | |
| `sigqueue` | `list`、`info`（號碼、錯誤碼、來源、送出者 PID） | 串列元素（不同型別） |

### 9.6 Signal frame 與返回

| 步驟 | 誰 | 做什麼 |
|---|---|---|
| 1 | kernel | 在使用者 stack 建 `rt_sigframe`：`pretcode` ← `sa_restorer`、`uc`（所有暫存器）、`info` |
| 2 | kernel | 改 `pt_regs`：`ip` ← handler、`di` ← signal 號碼、`sp` ← frame |
| 3 | 使用者 | handler 執行，`ret` → 取 `pretcode` → 跳到 glibc 的 `__restore_rt` |
| 4 | 使用者 | `__restore_rt` 發 `rt_sigreturn` |
| 5 | kernel | 從 frame 讀回 context（`access_ok`、`__get_user`）、還原 `blocked` 和 `pt_regs` |
| 6 | | `sysret` / `iret`：程式從原本的地方繼續 |
| 關鍵 | | **偷改返回位址**：函式不在你控制下，但它返回到哪裡在你控制下 |

### 9.7 本講看過的原始碼位置

| 檔案:行號 | 看什麼 |
|---|---|
| `arch/x86/include/uapi/asm/signal.h:23-63` | signal 編號、`SIGRTMIN` |
| `arch/x86/include/asm/signal.h:11` | `_NSIG` = 64 |
| `kernel/signal.c:1074-1077, 1099-1101` | `legacy_queue`：一般 signal 只能一個 pending |
| `include/linux/signal.h:418-446` | `SIG_KERNEL_ONLY_MASK`、`SIG_KERNEL_COREDUMP_MASK` |
| `include/linux/sched.h:1098-1106` | `task_struct` 的 signal 欄位 |
| `include/linux/sched/signal.h:20-25` | `struct sighand_struct` |
| `include/linux/signal_types.h:22-56` | `sigqueue`、`sigpending`、`sigaction`、`k_sigaction` |
| `arch/x86/include/asm/sigframe.h:59-64` | `struct rt_sigframe` |
| `arch/x86/kernel/signal_64.c:164-209` | `x64_setup_rt_frame`：偷改返回位址、跳到 handler |
| `arch/x86/kernel/signal_64.c:243-271` | `rt_sigreturn` |

---

## 10. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| **排程是「OS 的第二個心臟」** | **第 18–28 講**：第 5 章（同步與排程） |
| **thread group 共享 signal handler、`signal_struct`** | **第 18 講**：多執行緒 |
| **`siglock`（spinlock）、`atomic_t live`、`refcount_t count`** | **第 20–22 講**：鎖、kernel 裡的並行 |
| **`sighand` 的 `__rcu`** | **第 23 講**：RCU |
| **`SIGALRM`、timer** | **第 27 講**：即時排程器 |
| **`SIGSEGV`：page fault 找不到 VMA** | **第 29–31 講**：記憶體管理 |
| **`SIGPIPE`、pipe、signalfd（檔案描述子）** | 課本第 7 章 §7.6.10（Pipes）。對應講次**待補** |
| **即時 signal、POSIX** | 另一門課 **Real Time Systems**：POSIX 標準（第 22 講） |

---

## 附：本講一句話總結

> **Signal 是方向反過來的訊息：kernel → 應用程式，本質上是使用者空間的中斷。程式用名字取得很爛的 `signal()` 註冊 handler（存進 thread group 共享的 `sighand_struct.action[64]`，又是一張函式指標表），用 `kill()` 送 signal；pending 的 signal 記在一個位元向量加一串混合型別的 linked list（`sigpending` 頭 + `sigqueue` 元素），所以一般 signal 同類型只能 pending 一個（實測送 5 次只處理 1 次，即時 signal 則 5 次）。遞送時 kernel 把 context 存成使用者 stack 上的 signal frame，改 `pt_regs` 讓 CPU 跳到 handler，並把返回位址偷改成 glibc 的 `__restore_rt`；handler 一返回就呼叫 `rt_sigreturn`，由 kernel 把 context 讀回來，程式從原處繼續。`SIGKILL` 和 `SIGSTOP` 例外：kernel 專屬，不能擋、不能忽略、不能處理。**

---

*筆記依據 Smruti R. Sarangi「Operating Systems: A Linux Kernel-Oriented Approach」Lecture 17（Chapter 4, Part IV）錄影逐字稿（YouTube 自動字幕，54:37）、課本 *Operating Systems: A Linux Kernel-Oriented Approach*（Sarangi, v0.96）§4.4–4.5，以及 Linux kernel v6.2.12 原始碼整理。實測為本機（Ubuntu、kernel 6.5.0）結果。*
