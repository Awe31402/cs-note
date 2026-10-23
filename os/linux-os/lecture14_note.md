# Operating Systems: A Linux Kernel-Oriented Approach 第十四講完整筆記：Library Call 與 System Call（Chapter 4, Part I）

> **課程**：Operating Systems: A Linux Kernel-Oriented Approach — Lecture 14（Chapter 4, Part I）
> **講者**：Smruti R. Sarangi
> **來源**：YouTube 錄影逐字稿（41:55，自動字幕）＋ 課本 *Operating Systems: A Linux Kernel-Oriented Approach*（Sarangi, v0.96）第 4 章開頭與 §4.1（p.115–121）、附錄 C.1（kernel 的 linked list）＋ Linux kernel **v6.2.12** 原始碼（`os/linux-src/`）
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
- [1. 所有打斷機制的共同架構](#1-所有打斷機制的共同架構)
- [2. 繞道：kernel 的通用 linked list](#2-繞道kernel-的通用-linked-list)
  - [2.1 問題：一般寫法不通用](#21-問題一般寫法不通用)
  - [2.2 解法：只放 `list_head`，不放資料](#22-解法只放-list_head不放資料)
  - [2.3 `container_of`：從成員找回外層結構](#23-container_of從成員找回外層結構)
  - [2.4 走訪串列](#24-走訪串列)
  - [2.5 不同型別的結構串在同一個串列上](#25-不同型別的結構串在同一個串列上)
  - [2.6 【原始碼】`list_head`、`container_of`、`list_entry`](#26-原始碼list_headcontainer_oflist_entry)
  - [2.7 【實測】混合型別的串列](#27-實測混合型別的串列)
- [3. 一個 library call 的一生：`printf`](#3-一個-library-call-的一生printf)
  - [3.1 `printf` 的簽名](#31-printf-的簽名)
  - [3.2 呼叫鏈：越往下越通用](#32-呼叫鏈越往下越通用)
  - [3.3 一切都是檔案：stdin、stdout、stderr](#33-一切都是檔案stdinstdoutstderr)
  - [3.4 Library 設計的一般概念](#34-library-設計的一般概念)
  - [3.5 【實測】`strace`：`printf` 最後就是一個 `write`](#35-實測straceprintf-最後就是一個-write)
- [4. System call 的格式](#4-system-call-的格式)
- [5. OS 這邊：`do_syscall_64`](#5-os-這邊do_syscall_64)
  - [5.1 進來之後](#51-進來之後)
  - [5.2 最常見的 12 個 system call](#52-最常見的-12-個-system-call)
  - [5.3 【原始碼】`write` system call 的實作](#53-原始碼write-system-call-的實作)
- [6. 從 system call 返回](#6-從-system-call-返回)
  - [6.1 Kernel 不是知恩圖報的朋友](#61-kernel-不是知恩圖報的朋友)
  - [6.2 `TIF_NEED_RESCHED`](#62-tif_need_resched)
  - [6.3 【課本補充】segment 暫存器怎麼處理](#63-課本補充segment-暫存器怎麼處理)
- [7. 一頁速查表](#7-一頁速查表)
- [8. 與後續講次的連結](#8-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 這堂課要做什麼

`[00:30]` 第 4 章：**應用程式和 OS 之間的溝通**。方法有很多：system call、interrupt、exception、signal。

【課本補充】課本的第 4 章結構（Fig 4.1）：

| 部分 | 內容 | 講次 |
|---|---|---|
| **System call** | library call、OS 的實作、context 還原 | **本講**（§4.1） |
| **Interrupt 與 exception** | APIC 架構、IRQ、中斷路徑、exception | 第 15–16 講（§4.2） |
| **Bottom half handler** | softirq、threaded IRQ、work queue | 第 16 講（§4.3） |
| **Signal handler** | signal 遞送、kernel 程式碼、使用者 process 和 kernel 的互動 | 第 17 講（§4.4） |

本講：

| # | 主題 | 對應課本 |
|---|---|---|
| 1 | 所有打斷機制的共同架構 | §4.1 前 |
| 2 | 繞道：kernel 的通用 **linked list**、`container_of` | 附錄 C.1 |
| 3 | 一個 library call 的一生：**`printf`** | §4.1.1 |
| 4 | System call 的格式、`do_syscall_64` | §4.1.2 |
| 5 | 從 system call 返回、`TIF_NEED_RESCHED` | §4.1.3 |

---

## 1. 所有打斷機制的共同架構

`[00:30]` 「大致上**全部走同一個機制**。」system call 現在有更有效率的方法（`syscall` 指令），但先看**傳統**的做法：

```
  exception ──┐
              │
  system call ┼──► 中斷向量（interrupt vector，一個號碼）
  （int 0x80）│          │
              │          ▼
  硬體中斷 ───┘    中斷描述表（IDT）     ← 起點存在 IDTR 暫存器（特權暫存器）
  （IRQ）              │
                      ▼ 第 N 格
                   handler 的位址
                      │
                      ▼
                   執行 handler（存剩下的 context）
```

`[01:00]` 

- **system call**（傳統做法）就是一個特別的中斷，號碼 **128**，十六進位 **0x80**（第 2 講）。
- **硬體中斷**：每個裝置分到一個 **IRQ（interrupt request）**。硬體中斷有一整套**硬體架構**來收集、排隊、緩衝（第 15 講）。

`[01:30]` 每個中斷有一個號碼，叫 **中斷向量（interrupt vector）**：

- 用它去查一張表：**中斷描述表（IDT，Interrupt Descriptor Table）**。
- 表裡存 **handler 的位址**。
- 類似地，也有 system call table、exception table。

`[02:00]` 用傳統機制發 system call（中斷 128）：

- 表在**記憶體**裡。
- **`IDTR` 暫存器**（又一個**特權暫存器**）存表的起點。
- 去第 128 格 → 那裡有一個位址 → system call handler 的**第一個 byte** → 跳過去執行 → handler 存剩下的 context。

`[03:00]` **分工**：

| | 誰做 |
|---|---|
| **填表** | **軟體**（kernel 開機時） |
| **查表、跳過去** | **硬體** |

> 「為什麼查表和跳轉要硬體做？因為你想讓 handler 去存剩下的 context，**context 要保持完好**。如果你在這裡跑一段程式，它可能會**覆寫** context。」（第 13 講：PC 一定要硬體存）

`[03:00]` **`syscall` 指令不用 IDT**：它**直接跳到 handler**，因為 handler 的位址存在一個**特權暫存器**（MSR）裡。

> 【口頭】`[27:31]`「`syscall` 不再被當作一般中斷的一種，因為它顯然是個**特別的中斷**——它拿到一點**商務艙服務**，直接到 handler。」

`[03:30]` exception 也一樣：有一個號碼 → 查表（軟體填、硬體查）→ 每一格指向一個 handler——「**handler 就是一個函式指標**」——跳到那個函式執行。

（第 2 講：「**全部都是透過函式指標表**」。）

---

## 2. 繞道：kernel 的通用 linked list

`[03:30]` 「我們在每一章只講附錄裡**這一章需要的**部分。這一章要看懂程式碼，就需要懂 Linux 的 linked list 怎麼實作。花兩張投影片。」

### 2.1 問題：一般寫法不通用

`[04:01]` linked list 的主要想法：有 **next** 指標和 **previous** 指標，「任何**環狀** linked list 的主要想法：每個人都知道左右鄰居是誰」。

但我們要的是**通用的** linked list：**任何物件**都能串起來。

`[05:01]` 資料結構課上的寫法：

```c
struct node {
	int index;              /* 資料（payload） */
	struct node *next;
	struct node *prev;
};
```

`[05:31]` 【口頭】「你看這個人（投影片上的圖示）很清楚地告訴你：**這行不通**。」

- 自訂資料結構這樣寫沒問題。
- 但要一個**對所有物件都能用**的 linked list：每種物件都要有 `物件型別 *next, *prev`。
- 就算每種都這樣做，**能有一個函式操作所有 linked list，不管物件是什麼嗎？**「似乎不可能。」
- 走訪串列的函式需要**物件的型別**。資料結構課不會遇到這問題，因為只有一種物件。
- `[06:30]` 假設除了 `node` 還有 `xnode`，也在一個 linked list 上，同一個函式要能處理兩種——怎麼做？

`[06:30]` C++ 和 Java 的做法：建一個 `LinkedList`，把**型別當參數**——這叫 **template**（範本）。「但 C **沒有**這些東西，要在 C 裡自己做一個通用的 linked list。」

### 2.2 解法：只放 `list_head`，不放資料

`[07:01]` 建一個很簡單的結構 **`list_head`**：

```c
struct list_head {
	struct list_head *next, *prev;
};
```

- 只有 next 和 prev，**沒有資料（payload）**。
- 「你似乎不覺得這有問題——原因等一下就知道。」

**反過來：把 `list_head` 嵌進你的資料結構裡**：

```c
struct node {
	int x;
	int y;
	struct list_head lh;    /* 嵌在裡面 */
};
```

```
   node A                   node B                   node C
 ┌─────────┐             ┌─────────┐             ┌─────────┐
 │ x       │             │ x       │             │ x       │
 │ y       │             │ y       │             │ y       │
 │ ┌─────┐ │             │ ┌─────┐ │             │ ┌─────┐ │
 │ │ lh  │◄┼─────────────┼►│ lh  │◄┼─────────────┼►│ lh  │ │
 │ └─────┘ │             │ └─────┘ │             │ └─────┘ │
 └─────────┘             └─────────┘             └─────────┘
       串列其實只串 list_head；node 的其他部分「掛」在上面
```

`[13:30]` 「linked list **只由 `list_head` 組成**。」

### 2.3 `container_of`：從成員找回外層結構

`[07:31]` 兩個巨集：

```c
#define list_entry(ptr, type, member)  container_of(ptr, type, member)
```

`list_entry` 只是包了 `container_of`，三個參數一樣：**指標、型別、成員名稱**。

`[08:00]` `container_of` 的做法：

1. 把傳進來的指標轉成 **`void *`**——「`void *` 的意思是：它**指向任何東西**，我**不在乎**。它就只是一個指標、一個記憶體位址：32 位元系統是 32 位元數字，64 位元系統是 64 位元數字。」
2. **減掉一個偏移量（offset）**——「真正的魔法在這裡」。
3. 轉型成 `type *`。

`[09:31]` 教授的例子：

```c
struct node {
	int x;                  /* 偏移 0 */
	int y;                  /* 偏移 4 */
	struct list_head lh;    /* 偏移 8 */
};
```

```
  位址:   92        96        100
         ┌─────────┬─────────┬──────────────────┐
         │    x    │    y    │       lh         │
         └─────────┴─────────┴──────────────────┘
         ↑ node 的起點                ↑ 傳進來的指標 = 100
         = 100 − offsetof(node, lh)
         = 100 − 8 = 92
```

| 步驟 | 值 |
|---|---|
| 傳進來的指標（指向 `lh`） | **100** |
| `type` | `struct node` |
| `member` | `lh` |
| `offsetof(struct node, lh)`：`lh` 在 `node` 裡的偏移 | `x` 4 bytes + `y` 4 bytes = **8** |
| 外層結構的起點 = 100 − 8 | **92** |
| 回傳 | `(struct node *) 92` |

`[12:32]` 總結：

> 「`list_entry` 做的事：**給一個指向 `list_head` 的指標，找出包住它的那個結構的位址**。找出偏移量、從指標減掉，就得到外層結構（這裡是 `struct node`）的基底位址。這代表：**不管你把 `list_head` 嵌在哪裡**，都能拿到外層結構的基底位址，**不用事先知道它是什麼**——靠前置處理器巨集的魔法。」

### 2.4 走訪串列

`[14:01]` 例如：

```c
struct abc {
	int x;
	struct list_head list;
};
```

- 走訪串列**沒問題**：給任何一個 `list_head`，都能加節點、刪節點、做任何事——「因為我只在操作 `list_head`，它是**通用**的。」
- 任何時候想到外層結構，就呼叫 `list_entry`。

`[14:32]` 例：目前節點是 `current`，想到**下一個節點**（不是下一個 `list_head`，是下一個 `abc`）：

```c
struct abc *next = list_entry(current->list.next, struct abc, list);
```

| 參數 | 值 |
|---|---|
| 指標 | `current->list.next`——從 `current` 走到它的 `list`、再到 `next` |
| 型別 | `struct abc` |
| 成員 | `list` |

（第 4 講看過的 `list_for_each_entry` 巨集，就是把這個包成一個 `for` 迴圈。）

### 2.5 不同型別的結構串在同一個串列上

`[15:01]` 「我們可以做一件**更巧妙、更反直覺**的事——一般 linked list 的實作**做不到**。」

```c
struct abc { int x; struct list_head list; };
struct def { int x; float f; struct list_head list; };
```

【口頭】「當然我加了 `float`，但 **kernel 程式碼沒有浮點數**。這只是示範。」

`[15:32]` **讓一個 `def` 指向一個 `abc`**——在這個方案裡**完全允許**：兩個都有 `list_head`，可以互相指。

```
     struct def                 struct abc
  ┌────────────┐             ┌────────────┐
  │ x          │             │ x          │
  │ f          │             │ ┌────────┐ │
  │ ┌────────┐ │             │ │  list  │ │
  │ │  list  │─┼────────────►│ └────────┘ │
  │ └────────┘ │             └────────────┘
  └────────────┘
```

`[16:00]` 怎麼走：

- 目前是 `struct def *current`。
- `current->list.next` 是指標。
- **關鍵**：「我知道下一個是 `abc`」，所以用 **`struct abc` 和 `list`** 算偏移量 → 得到正確的 `struct abc *`。

`[16:32]` 「這代表我可以把**不同型別的結構串在一起**。這非常重要——講 **signal handler** 時會看到它的實際應用：不同型別的結構被串在同一個串列上。」

`[17:02]` 前提：**要知道下一個的型別**。「假設 `def` 用某種方式知道下一個是什麼型別——也許 `def` 裡存了某些資訊。我們不在乎確切機制，只要能**事先**知道就行。」

`[18:30]` 【口頭】「背景知識很**基本**，但**不簡單**。linked list 這件事對初學者可能很複雜，直到抓到訣竅為止——因為我們用的是**巨集**，不是函式。」

【課本補充】附錄 C.1 對 kernel linked list 有完整介紹（`struct list_head`、單向串列 `hlist`）。第 10 講看過的 `struct pid` 裡的 `hlist_head tasks[]` 就是單向版。

### 2.6 【原始碼】`list_head`、`container_of`、`list_entry`

```c
// include/linux/types.h:178-180
struct list_head {
	struct list_head *next, *prev;
};
```

就這樣——教授投影片上的結構，原封不動。

```c
// include/linux/container_of.h:18-23
#define container_of(ptr, type, member) ({				\
	void *__mptr = (void *)(ptr);					\
	static_assert(__same_type(*(ptr), ((type *)0)->member) ||	\
		      __same_type(*(ptr), void),			\
		      "pointer type mismatch in container_of()");	\
	((type *)(__mptr - offsetof(type, member))); })
```

| 行 | 意思 | 對照教授說的 |
|---|---|---|
| 18 | `({ ... })` | GCC 擴充：**陳述式運算式（statement expression）**——大括號裡可以寫多行，最後一行的值就是整個運算式的值 |
| **19** | **`void *__mptr = (void *)(ptr);`** | 「建一個 `void *`，**指向任何東西**，值跟 `ptr` 一樣」——教授說的「內部變數 `__mptr`」 |
| 20–22 | **`static_assert`**：檢查 `*ptr` 的型別**真的**跟 `type` 裡 `member` 的型別一樣 | 教授沒講：**編譯時**的型別檢查。傳錯型別（例如成員名字寫錯）會直接編譯失敗，而不是執行時算出一個錯的位址 |
| **23** | **`((type *)(__mptr - offsetof(type, member)))`** | 「**真正的魔法**」：減掉偏移量，轉型成 `type *` |

`offsetof(type, member)` 是 C 標準的巨集（`<stddef.h>`）：成員在結構裡的偏移量，**編譯時**就算好了，所以 `container_of` 執行時只是**一個減法**。

```c
// include/linux/list.h:513-520
/**
 * list_entry - get the struct for this entry
 * @ptr:	the &struct list_head pointer.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the list_head within the struct.
 */
#define list_entry(ptr, type, member) \
	container_of(ptr, type, member)
```

註解：「取得這個 entry 的**結構**」——就是 `container_of` 換個名字。

```c
// include/linux/list.h:558-564
/**
 * list_next_entry - get the next element in list
 * @pos:	the type * to cursor
 * @member:	the name of the list_head within the struct.
 */
#define list_next_entry(pos, member) \
	list_entry((pos)->member.next, typeof(*(pos)), member)
```

這就是 §2.4 教授寫的那行：`list_entry(current->list.next, struct abc, list)`。`typeof(*(pos))` 自動拿到 `pos` 指向的型別，不用自己寫 `struct abc`——前提是**下一個跟目前的同型別**。§2.5 的混合型別情況就不能用這個巨集，要自己寫 `list_entry` 並給**下一個的型別**。

`container_of` 在 kernel 裡**到處都是**，不只用在 linked list。例如第 9 講的 maple tree、第 10 講的 `struct pid`，任何「嵌入式」的結構都靠它從成員找回外層。

### 2.7 【實測】混合型別的串列

把教授的 `abc` / `def` 例子寫成程式（我寫的，已在本機執行）：

```c
#include <stdio.h>
#include <stddef.h>

struct list_head { struct list_head *next, *prev; };

#define container_of(ptr, type, member) \
	((type *)((char *)(ptr) - offsetof(type, member)))
#define list_entry(ptr, type, member) container_of(ptr, type, member)

struct abc { int x; int y; struct list_head list; };
struct def { int x; char tag; struct list_head list; };

int main(void)
{
	struct abc a = { .x = 1, .y = 2 };
	struct def d = { .x = 3, .tag = 'D' };

	/* 把兩種不同型別的結構串在同一個 list 上：d -> a -> d */
	d.list.next = &a.list;  a.list.prev = &d.list;
	a.list.next = &d.list;  d.list.prev = &a.list;

	printf("offsetof(struct abc, list) = %zu\n", offsetof(struct abc, list));
	printf("&a = %p, &a.list = %p\n", (void *)&a, (void *)&a.list);

	/* 從 d 出發，拿到 next（指向 a.list），用 abc 的偏移量算出 a 的起點 */
	struct abc *pa = list_entry(d.list.next, struct abc, list);
	printf("list_entry(d.list.next) = %p -> x=%d y=%d\n", (void *)pa, pa->x, pa->y);

	struct def *pd = list_entry(pa->list.next, struct def, list);
	printf("back to def: x=%d tag=%c\n", pd->x, pd->tag);
	return 0;
}
```

輸出：

```text
offsetof(struct abc, list) = 8
&a = 0x7ffc35684540, &a.list = 0x7ffc35684548
list_entry(d.list.next) = 0x7ffc35684540 -> x=1 y=2
back to def: x=3 tag=D
```

| 觀察 | 印證 |
|---|---|
| `offsetof(struct abc, list) = 8` | 兩個 `int`（4 + 4）= 8，跟教授的例子一樣 |
| `&a.list` − `&a` = `0x...548` − `0x...540` = **8** | `list` 成員在 `a` 裡偏移 8 |
| `list_entry(d.list.next, ...)` 回傳 `0x...540` = **`&a`** | 從 `def` 的 next（指向 `a.list`）減掉 8，**正確找回 `a` 的起點** |
| 從 `abc` 再走回 `def`，`tag` 是 `D` | **兩種型別**的結構串在同一個串列上、可以來回走——只要每一步給對型別 |

（範例裡用 `(char *)` 而不是 kernel 的 `void *` 做減法：標準 C 不允許 `void *` 做算術，GCC 允許是擴充。）

---

## 3. 一個 library call 的一生：`printf`

### 3.1 `printf` 的簽名

`[19:00]`（課本 Listing 4.1）

```c
#include <stdio.h>
int main() {
	printf("Hello World\n");
}
```

- **`#include`**：把 header 的內容**複製貼上**。header 只有**簽名**、沒有程式碼，簽名是正確編譯所需要的（參數型別等）——第 4 講。
- `stdio.h` 是標準 C library **輸入輸出**函式的簽名。
- **`printf` 定義在一個動態 library 裡**，例如 `libc.so.6`，以 **DLL**（共享 library）的方式載入——不是程式碼的一部分，被呼叫時才去找（第 4 講）。

`[20:00]` 「現在**走進 glibc 的程式碼**，看看到底發生什麼，看 system call 怎麼發出、再**深入 kernel**。」（glibc 是**開源**的，可以去看。）

`[20:33]` `printf` 的簽名：

```c
int printf(const char *format, ...);
```

| 部分 | 意思 |
|---|---|
| `int` | 回傳碼 |
| `const char *format` | **格式字串**，例如 `"My name is %s, my number is %d"`。是**常數**（`const`），不能改 |
| **`...`** | 省略號：**數量可變的參數**——取決於格式字串裡有幾個 `%` |

### 3.2 呼叫鏈：越往下越通用

`[21:30]`（課本 p.117）

```
  printf
    → __printf               （內部版本）
      → vfprintf             （通用版：印到任何檔案）
        → printf_positional  （組出要印的字串）
          → outstring        （送到檔案）
            → PUT
              → ... new_do_write（glibc 的 fileops.c）
                → write system call
```

`[21:30]` 「簽名會**改變**、變得**越來越通用**，因為你預期其他函式也會在不同的點**加入**這條路。」

`[22:01]` 到了 **`vfprintf`**，東西變得更大：

```c
int vfprintf(FILE *s, const CHAR_T *format, va_list ap,
             unsigned int mode_flags);
```

| 參數 | 意思 |
|---|---|
| **`FILE *s`** | **印到哪個檔案**：可以是**終端機**（`stdout`），也可以是**一般資料檔** |
| `format` | 格式字串 |
| `va_list ap` | glibc 內部的資料結構：**`printf` 參數的清單** |
| `mode_flags` | 一些大略的旗標：怎麼寫（印的行為） |

`[23:01]` `printf` 和 `fprintf`（file printf，印到一般檔案）**最後都會走到這裡**。「既然要通用化，就寫成**印到一個檔案**：檔案可以是標準輸出、也可以是一般資料檔，**不重要**。」

### 3.3 一切都是檔案：stdin、stdout、stderr

`[22:01]` `printf` 到底印到哪裡？**終端機**。

> 「**終端機在 Linux 裡被表示成一個檔案**。Linux 裡檔案不只是一般資料檔，**裝置、終端機等等也被表示成檔案**。」

| 標準串流 | 是什麼 | 誰在用 |
|---|---|---|
| **`stdout`**（standard out） | 一般的**輸出**終端機 | `printf` |
| **`stdin`**（standard in） | 一般的**輸入**終端機 | `scanf` |
| **`stderr`**（standard error） | **錯誤**串流，預設也是終端機（可以改） | 錯誤訊息 |

（§3.5 實測的 `write(1, ...)`：**1** 就是 `stdout` 的檔案描述子。0 是 `stdin`、2 是 `stderr`。第 10 講：檔案描述子是 `task_struct->files` 的索引。）

### 3.4 Library 設計的一般概念

`[24:01]`（課本 Fig 4.2）「名字不重要，**精神**重要。」

```
  ┌────────────────────────────────────────────────────────────────────┐
  │ printf / fprintf / sprintf …   ← 程式設計師看得到的（一個家族）        │
  └──────────────────────┬─────────────────────────────────────────────┘
                         ▼
               vfprintf            ← 通用版：印到「一個檔案」
                         ▼                 （stdout 也是檔案）
               printf_positional   ← 組出要印的字串（"Value = 5"）
                         ▼
               outstring           ← 送到檔案；有好幾層抽象、函式指標表
                         ▼                 （因為有不同種類的檔案）
               準備 write system call 的參數（放進對的暫存器）
                         ▼
               syscall（以前是 int 0x80）
  ─────────────────────────── library 的工作到此結束 ───────────────────
                         ▼
                      kernel
```

`[24:32]` **library 最終的工作**：

> 「拿你的輸入，**整理、安排、管理、彙整**，全部清理好，然後**發一個 system call**。這就是 library 的工作：清理東西，讓發 system call 的過程更容易。這裡因為有一整個 `printf` 家族，就做一個**通用函式**，把整個家族**合併**起來。」

`[26:04]` `vfprintf` 時，**要印的東西還沒組出來**：

- 例如 `printf("Value = %d", x)`，最後要印的是 `"Value = 5"`。
- **`printf_positional`** 才把這個字串**組出來**。
- **`outstring`** 再把它送到檔案——中間有好幾層抽象、函式指標表，因為可以有**不同種類的檔案**。
- 最後如果是 system call，就**準備 `write` system call 的參數**：把所有東西放進對的暫存器。

`[27:01]` 「以前用 `int 0x80`，**現在用 `syscall` 指令**。最後發出 system call——**library 的工作到這裡結束**。」

`[27:31]` 「這就是 **library 和 system call 之間邏輯上的分界點**。」

【課本補充】課本的總結（p.116）：library call **包裝** system call：準備 system call 的參數、設好環境、發 system call、再適當地處理回傳值。Linux 上 **glibc** 包含標準 C library 的所有相關程式碼。

### 3.5 【實測】`strace`：`printf` 最後就是一個 `write`

`strace` 可以印出一個程式發的所有 system call。對上面的 Hello World（`printf("Hello World\n")`）：

```bash
$ strace ./hello
execve("./hello", ["./hello"], 0x7ffcca97f440 /* 79 vars */) = 0
brk(NULL)                               = 0x563ffb9c2000
arch_prctl(0x3001 /* ARCH_??? */, 0x7ffcc67a1230) = -1 EINVAL (Invalid argument)
mmap(NULL, 8192, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0) = 0x7f93481c3000
...
write(1, "Hello World\n", 12)           = 12
exit_group(0)                           = ?
+++ exited with 0 +++
```

| 觀察 | 印證 |
|---|---|
| **`write(1, "Hello World\n", 12) = 12`** | 整條 `printf → vfprintf → ... → outstring` 的呼叫鏈，最後**就是一個 `write` system call**：檔案描述子 **1**（`stdout`）、字串、長度 **12**。回傳 12（寫了 12 bytes） |
| 回傳值在 `= 12` | §4：回傳值放在 **`rax`** |
| 第一行 **`execve`** | 第 11 講：shell fork 出子 process，子 process **exec** 這個程式 |
| `mmap(... MAP_ANONYMOUS ...)` | 第 9 講：**anonymous** 記憶體區域 |
| `arch_prctl(...)` | 第 6 講：設定 `fs` base（TLS）的 system call |
| 最後 **`exit_group(0)`** | 第 7 講：`main` 回傳後，程式**一定**要發 exit system call（「硬體不知道程式什麼時候結束」）。`exit_group` 是「結束整個 thread group」的版本 |

**這個只印一行字的程式總共發了多少 system call？**

```bash
$ strace ./hello 2>&1 >/dev/null | grep -o '^[a-z_0-9]*' | sort | uniq -c | sort -rn
     23 newfstatat
     22 openat
      8 mmap
      4 pread64
      4 mprotect
      3 brk
      ...
      1 write
      ...
      1 exit_group
      1 execve
```

- 總共 **80 個**左右，**真正做事的 `write` 只有 1 個**。
- 其他大部分是**動態連結器**（第 4 講的 `ld-linux-x86-64.so.2`）在**找 library、開檔、映射到記憶體**：`openat`、`newfstatat`、`mmap`、`mprotect`。
- **`pread64`**：第 2 講說的 **17 號 system call**——動態連結器用它讀 ELF 檔頭。
- 這也是第 4 講說的**動態連結**的代價：開始執行前要做一堆準備工作。

---

## 4. System call 的格式

`[28:02]` Linux 的 system call 格式。

`[28:02]` 再強調：

| 機制 | 性質 |
|---|---|
| **System call** | **計畫好的**活動 |
| **Interrupt** | 隨時可能發生 |
| **Exception** | 「技術上不是計畫好的，但**介於計畫和沒計畫之間**」：不像中斷隨時都來，是你執行某條指令、那條指令才可能出 exception |

`[28:31]` 因為 system call 是計畫好的，**`rcx` 和 `r11`** 可以**空著**，不用在內部使用。

- 「某種意義上可以把它們當成 **caller saved**（呼叫者保存）。」
- 返回位址和 flags 會被硬體自動存進這兩個暫存器（第 13 講）。
- 所以 **library** 在開始發 system call 之前，要先把這兩個暫存器的內容**推到 stack** 上。

`[29:30]` 格式（課本 Table 4.1）：

| 用途 | 暫存器 |
|---|---|
| **system call 號碼** | **`rax`**——「最重要的參數」 |
| 第 1 個參數 | `rdi` |
| 第 2 個參數 | `rsi` |
| 第 3 個參數 | `rdx` |
| 第 4 個參數 | `r10` |
| 第 5 個參數 | `r8` |
| 第 6 個參數 | `r9` |
| **回傳值** | **`rax`** |

`[30:01]` system call **最多 6 個暫存器參數**。更多的話要**推到使用者 stack** 上，kernel 再從使用者 stack 取（kernel 可以讀使用者記憶體，第 12 講）。

【課本補充】課本補充：用 stack 傳參數**不是首選**，比用暫存器**慢很多**。

`[30:33]` library 函式的工作：「**讓 system call 的呼叫成為可能**」——設好參數、呼叫 `syscall` 指令。

（第 2 講 `entry_SYSCALL_64` 的註解有一模一樣的表；那裡也說明了為什麼第 4 個參數用 `r10` 而不是 `rcx`——`rcx` 被 `syscall` 指令拿去存返回位址了。）

---

## 5. OS 這邊：`do_syscall_64`

### 5.1 進來之後

`[30:33]` `syscall` 是一種**捷徑**，不走完整的中斷機制。

`[31:00]` x86 處理器的入口在 **`entry_64.S`**（第 13 講）：

1. 硬體把一部分 context 存進 `rcx` 和 `r11`。
2. 專門的軟體常式存剩下的 context。
3. **整個過程中斷是關的**。
4. 最後呼叫 **`do_syscall_64`**——「處理所有 system call 的**標準入口**」。

`[31:31]` `do_syscall_64`：

1. 第一件事：**在本地 CPU 上打開中斷**——「沒必要讓系統沒反應」。狀態一存好，中斷就打開。
2. 用 **`rax`** 的內容（system call 號碼）存取 **system call table**。
3. 出來的是一個**函式指標**——system call 的處理函式。

`[32:31]` 「我們在整個 kernel 裡一直看到：很多**只存函式指標**的表，這些函式指標就是這個過程的輸出。」

（第 2 講看過完整的程式碼：`do_syscall_64` → `do_syscall_x64` → `regs->ax = sys_call_table[unr](regs);`。）

【課本補充】課本說（p.118）：不管用舊方法（`int 0x80`）還是 `syscall` 指令，都會到達 system call 的入口 `do_syscall_64`。這時有一次 **ring level 切換**，中斷被關掉。「存 context 是很短的過程，這段時間遮罩中斷通常**不會**對處理關鍵任務造成效能問題。」

### 5.2 最常見的 12 個 system call

`[32:31]` x86 架構可以看 **`syscall_64.tbl`**，有所有 system call 的清單（第 2 講）。

`[33:00]` 其中一部分是**通用**的（跟架構無關），例如讀檔、寫檔：它們屬於 **uapi** 子系統，在 `unistd.h` 裡。但**完整**清單在 `syscall_64.tbl`。

`[33:30]` 前 12 格（課本 Table 4.2）——「任何系統裡**最常見**的 system call 的**老面孔**」：

| 號碼 | System call | 函式 | 教授的說明 |
|---|---|---|---|
| 0 | `read` | `sys_read` | **讀任何檔案** |
| 1 | `write` | `sys_write` | **寫任何檔案**。終端機（`stdout`、`stdin`）也被當成檔案 |
| 2 | `open` | `sys_open` | 讀寫之前要先**開** |
| 3 | `close` | `sys_close` | 做完要**關** |
| 4 | `stat` | `sys_newstat` | 查檔案的**狀態** |
| 5 | `fstat` | `sys_newfstat` | 同上（用檔案描述子） |
| 6 | `lstat` | `sys_newlstat` | 同上（不跟隨符號連結） |
| 7 | `poll` | `sys_poll` | 講中斷時再看 |
| 8 | `lseek` | `sys_lseek` | **移到檔案的某個位置** |
| 9 | `mmap` | `sys_mmap` | 把檔案的一部分**映射**到虛擬位址空間 |
| 10 | `mprotect` | `sys_mprotect` | 改 page 的**保護等級** |
| 11 | `munmap` | `sys_munmap` | 解除映射 |

`[34:00]` **`lseek`**：

> 「檔案是一串連續的 bytes。`lseek` 就是說：從 0 開始，好，現在我想從第 **512** 個 byte 開始讀——一個**虛擬的檔案指標**會移到那裡，之後一直讀，就從那些 bytes 開始讀。」

`[34:32]` **`mmap` / `munmap`**：

> 「硬碟上存了一個檔案，是一串 bytes，把它的**一部分映射到虛擬位址空間**。之後存取那段虛擬記憶體，會**自動翻成檔案存取**，反之亦然。檔案的一部分被映射到虛擬位址，就像被**鏡像**到那裡。」（第 9 講的 **file-backed** VMA）

`[35:02]` **`mprotect`**：「改 page 的保護等級：讓某些東西變成唯讀、只能寫等等。」（第 5 講的 PTE 保護位元）

### 5.3 【原始碼】`write` system call 的實作

`[35:31]` 教授說：從通用的 system call handler 到真正處理 `write` 的函式，中間「**一長串巨集**」，「要**天才**才搞得懂」。

> 「巨集在 kernel 程式碼裡**很多**，原因很清楚：巨集在**前置編譯階段**改寫程式碼，所以**很快**。最後會到 `ksys_write`，在那裡處理 system call。」

`write` 的 system call 定義：

```c
// fs/read_write.c:646-650
SYSCALL_DEFINE3(write, unsigned int, fd, const char __user *, buf,
		size_t, count)
{
	return ksys_write(fd, buf, count);
}
```

| 部分 | 意思 |
|---|---|
| **`SYSCALL_DEFINE3`** | 定義一個**有 3 個參數**的 system call。第 11 講的 `fork` 用 `SYSCALL_DEFINE0`（0 個參數）。這就是教授說的「一長串巨集」的起點：它會展開成 `__x64_sys_write(const struct pt_regs *regs)`——從 `pt_regs` 裡取出 `rdi`、`rsi`、`rdx`，轉型後再呼叫真正的函式 |
| 參數寫法 `型別, 名字, 型別, 名字…` | 巨集需要分開拿到型別和名字，才能產生轉型的程式碼 |
| `fd` | 檔案描述子（`rdi`，第 1 個參數） |
| **`const char __user *buf`** | 要寫的資料（`rsi`）。**`__user`** 是一個標記：「這是**使用者空間**的指標」，靜態檢查工具（sparse）會確保 kernel 不會直接解參考它，而是用專門的函式（`copy_from_user`）安全地複製 |
| `count` | 要寫幾個 byte（`rdx`） |
| `ksys_write(...)` | 真正做事的函式 |

```c
// fs/read_write.c:626-644
ssize_t ksys_write(unsigned int fd, const char __user *buf, size_t count)
{
	struct fd f = fdget_pos(fd);
	ssize_t ret = -EBADF;

	if (f.file) {
		loff_t pos, *ppos = file_ppos(f.file);
		if (ppos) {
			pos = *ppos;
			ppos = &pos;
		}
		ret = vfs_write(f.file, buf, count, ppos);
		if (ret >= 0 && ppos)
			f.file->f_pos = pos;
		fdput_pos(f);
	}

	return ret;
}
```

| 行 | 意思 |
|---|---|
| 628 | **`fdget_pos(fd)`**：用檔案描述子（例如 1）在 `current->files`（第 10 講）裡找到對應的 **`struct file`** |
| 629 | 預設回傳 **`-EBADF`**（Bad File descriptor）：找不到就回這個錯誤碼。kernel 的慣例：**負數 = 錯誤** |
| 632–636 | 拿到檔案目前的**位置**（`f_pos`）——就是 `lseek` 移動的那個「虛擬檔案指標」 |
| **637** | **`vfs_write(...)`**：交給 **VFS（虛擬檔案系統）**——它會根據檔案的種類（一般檔案、終端機、管線……）呼叫對應的**函式指標**。這就是教授說的「不同種類的檔案、函式指標表」在 kernel 這邊的對應 |
| 638–639 | 寫成功 → 更新檔案位置 |
| 643 | 回傳寫了幾個 byte（或負的錯誤碼）——最後會放進 `rax`，就是 `strace` 看到的 `= 12` |

---

## 6. 從 system call 返回

### 6.1 Kernel 不是知恩圖報的朋友

`[36:00]` 「進到 kernel，kernel **沒有義務**繼續跑被打斷的那個 process。kernel 會**趁這個機會**，根據 run queue 裡所有 process 和它們的優先權，排程**它喜歡的任何東西**。」

【課本補充】課本的標題是「**Kernel 很遺憾不是一個知恩圖報的朋友**」（The kernel is sadly not a very grateful friend）。process 一進 kernel，**不保證** system call 做完就會馬上被排程。kernel 可以決定做**自己的事**：例行記帳、更新資料結構、跑 kernel thread 服務裝置，也可以排程**其他使用者 process**。

（第 13 講教授的說法是「kernel 是**機會主義者**」。）

### 6.2 `TIF_NEED_RESCHED`

`[36:30]` 處理完中斷 / system call / exception 後，看**目前的 task** 是不是用了很多 CPU 時間：

- **用超過配額** → 不該再跑，排程器要找另一個 task。
- **有更高優先權的 thread 在等** → 也該讓它跑。

`[37:02]` 因為這些原因，**`TIF_NEED_RESCHED`** 旗標會被設起來：

- 它提供一個**粗略**的資訊：「需要跑排程器」，因為很可能有一個**更適合**在這顆 CPU 上跑的 process。
- `[37:31]` 這個旗標放在 **`thread_info`** 裡（第 7 講）。

`[38:00]` 教授回顧為什麼需要 `thread_info`：

> 「使用者 process 遇到 system call 這種感興趣的事件，context switch 進 OS，**同一個 process 變成 kernel thread**（不開新的），做完工作，然後**回到同一個 process**（總是好的），不然就要 context switch。它是 kernel thread 時用 **8 KB 的 kernel stack**——大小當然**有限**，所以要存更多資訊，就要放在**別的地方**：`thread_info`。它**不放在 stack 上**，而是放在 kernel 記憶體裡別的地方。」

（第 8 講：x86 v6.2.12 的 `thread_info` 嵌在 `task_struct` 裡，kernel stack 是 16 KB。）

`[39:00]` 檢查這個旗標，決定要不要呼叫 **`schedule`**——「**排程器的入口**」。

- 排程器跑了，**不知道**它會選哪個等待中的 task：可能選目前的，也可能選別的。
- 我們討論過很多理由，為什麼應該**偏好目前的 process**，或同一個 thread group 的另一個 thread，而不是一個**完全獨立**的 process——「那是**昂貴**的那種」（第 12 講）。
- 排程器會考慮所有這些和優先權，做出最好的決定。

`[40:00]` 不回到同一個使用者 process 的話，要做複雜的**狀態還原**（存狀態的反過程）。最後回到 user mode、開始一個使用者 process（同一個或不同的）用的指令是 **`sysret`**——「`syscall` 的**反向**」。

【課本補充】課本補充（p.119）：

- `TIF_NEED_RESCHED` 由**排程器**設：覺得目前 task 跑太久、要讓給其他 process，或有更高優先權的 process 在等。
- 有時 thread 會**主動要求**被搶占，讓其他 thread 有機會執行——透過 **`sched_yield`**。那時要讓出 CPU 的 thread 的 `TIF_NEED_RESCHED` 會被設起來。
- 排程器把 `TIF_NEED_RESCHED` 當成「**要跑排程器**」的指令。跑了之後它**獨立**做決定：繼續同一個 task 或開始新的 task，「**純粹是排程器的專屬權利**」。

（第 13 講看過實際的程式碼：`exit_to_user_mode_loop()` 裡的 `if (ti_work & _TIF_NEED_RESCHED) schedule();`。）

`[40:32]` 下一講：**中斷和 exception**，以及 kernel 執行 interrupt handler 的機制——**top half**、**bottom half**，以及實作 bottom half 的**很多種方法**。

`[41:01]` 本講總結：

> 「我們討論了中斷、exception、system call 處理的**大架構**；**很詳細地**看了一個 library——`printf`；看了 system call 參數怎麼傳、system call 怎麼處理；然後用 `TIF_NEED_RESCHED` 決定要不要呼叫排程器；最後還原 context、返回。context 的大部分上一章講過了。」

### 6.3 【課本補充】segment 暫存器怎麼處理

教授這一講沒講，課本 §4.1.3 補充了還原時 segment 暫存器的處理：

| 暫存器 | x86-64 上 | 存 / 還原嗎 |
|---|---|---|
| `ds`、`es`、`ss` | 通常**不用** | 通常不用存（第 13 講 `__switch_to` 的 `if (unlikely(next->es \| prev->es))`：都是 0 就跳過） |
| **`fs`** | 指向 **TLS** 區域 | **要**，當成一般暫存器，是 task context 的一部分 |
| **`gs`** | 指向 **per-CPU** 資料結構 | **要** |
| **`cs`** | **特別**：所有 kernel thread（ring 0）用**同一個**值，所有使用者 process 也用同一個值 | ring level 切換時，**硬體可以自動推斷**並設定 `cs` 的值 |

（第 6 講看過：GDT 裡 kernel 和使用者各只有一個 code segment——`KERNEL_CS` 和 `DEFAULT_USER_CS`。）

最後 kernel 呼叫 **`sysret`**：設 PC、把控制權交回使用者 process，同時**改變 ring level**（kernel mode → user mode）。

---

## 7. 一頁速查表

### 7.1 打斷機制的共同架構

| 項目 | 內容 |
|---|---|
| 中斷向量 | 每個中斷 / exception 一個號碼 |
| IDT | 中斷描述表，起點在 `IDTR` 暫存器 |
| 分工 | 軟體填表、硬體查表跳轉 |
| `int 0x80` | 傳統 system call，中斷 128 |
| `syscall` | 不查 IDT，handler 位址在 MSR，直接跳 |
| handler | 就是函式指標 |

### 7.2 Kernel linked list

| 項目 | 內容 |
|---|---|
| `struct list_head` | 只有 `next`、`prev`，**沒有資料** |
| 用法 | 把 `list_head` **嵌進**自己的結構 |
| `container_of(ptr, type, member)` | `ptr − offsetof(type, member)` → 外層結構的起點 |
| `list_entry` | = `container_of` |
| `list_next_entry(pos, member)` | 下一個同型別的元素 |
| 好處 | 一套函式處理所有串列；**不同型別**也能串在一起（要知道下一個的型別） |
| 原始碼 | `container_of` 還有編譯時的 `static_assert` 型別檢查 |

### 7.3 `printf` 的一生

| 階段 | 內容 |
|---|---|
| `printf` | 程式設計師看得到；家族：`fprintf`、`sprintf`…… |
| `vfprintf` | 通用版：印到任何 `FILE *`（`stdout` 也是檔案） |
| `printf_positional` | 組出要印的字串 |
| `outstring` | 送到檔案，經過函式指標表 |
| 準備參數 | 放進暫存器 |
| `syscall` | library 工作結束 |
| 實測 | 最後就是 `write(1, "Hello World\n", 12)`；整個程式 ~80 個 system call，只有 1 個 `write` |

### 7.4 標準串流

| 名稱 | 檔案描述子 | 預設 |
|---|---|---|
| `stdin` | 0 | 終端機輸入（`scanf`） |
| `stdout` | 1 | 終端機輸出（`printf`） |
| `stderr` | 2 | 終端機（錯誤） |

### 7.5 System call 格式

| 暫存器 | 用途 |
|---|---|
| `rax` | 號碼 / 回傳值 |
| `rdi`、`rsi`、`rdx`、`r10`、`r8`、`r9` | 第 1–6 個參數 |
| 更多參數 | 推到使用者 stack（慢） |
| `rcx`、`r11` | `syscall` 指令用掉（返回位址、flags）；library 要先存起來 |

### 7.6 前 12 個 system call

| # | 名稱 | # | 名稱 |
|---|---|---|---|
| 0 | `read` | 6 | `lstat` |
| 1 | `write` | 7 | `poll` |
| 2 | `open` | 8 | `lseek` |
| 3 | `close` | 9 | `mmap` |
| 4 | `stat` | 10 | `mprotect` |
| 5 | `fstat` | 11 | `munmap` |

### 7.7 返回

| 項目 | 內容 |
|---|---|
| kernel 的態度 | 不是知恩圖報的朋友 / 機會主義者 |
| `TIF_NEED_RESCHED` | 在 `thread_info`；排程器設；`sched_yield` 也會設 |
| 設了 | 呼叫 `schedule()`，排程器自己決定跑誰 |
| 回 user mode | `sysret`（`syscall` 的反向） |
| `cs` | 硬體依 ring level 自動推斷 |

### 7.8 本講看過的原始碼位置

| 檔案:行號 | 看什麼 |
|---|---|
| `include/linux/types.h:178-180` | `struct list_head` |
| `include/linux/container_of.h:18-23` | `container_of` |
| `include/linux/list.h:513-520` | `list_entry` |
| `include/linux/list.h:558-564` | `list_next_entry` |
| `fs/read_write.c:646-650` | `SYSCALL_DEFINE3(write, ...)` |
| `fs/read_write.c:626-644` | `ksys_write()` → `vfs_write()` |

---

## 8. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| **IDT、中斷向量、IRQ、硬體中斷的收集和排隊** | **第 15 講**：x86 中斷架構（APIC、IRQ、IDT 初始化、中斷路徑） |
| **Exception table、top half / bottom half** | **第 16 講**：exception、softirq、threaded IRQ、work queue |
| **不同型別的結構串在同一個 linked list 上** | **第 17 講**：signal（教授特別預告） |
| **`poll` system call** | 第 15 講之後（教授說「講中斷時再看」） |
| **`sched_yield`、`schedule()` 的決策** | **第 24–26 講**：排程 |
| **`mmap`、file-backed 映射** | **第 29–31 講**：記憶體管理 |
| **一切都是檔案、`vfs_write`、VFS** | 課本第 7 章 §7.3、§7.6.4（Virtual File System）。對應講次**待補** |

---

## 附：本講一句話總結

> **中斷、exception、system call 共用一套機制：用號碼查一張軟體填、硬體查的函式指標表（IDT），只有 `syscall` 指令走捷徑直接跳到 handler。從程式設計師的 `printf` 開始，glibc 一路把呼叫變得越來越通用（`vfprintf` 把終端機也當成檔案），最後只是把參數放進 `rdi`、`rsi`、`rdx`、號碼放進 `rax`，發一個 `write` system call——`strace` 實測整個 Hello World 發了約 80 個 system call，真正印字的只有一個 `write(1, "Hello World\n", 12)`。而要看懂 kernel 程式碼，得先懂 `list_head` + `container_of`：串列只串嵌在結構裡的 `list_head`，再用「指標減偏移量」找回外層結構，連不同型別的結構都能串在一起。**

---

*筆記依據 Smruti R. Sarangi「Operating Systems: A Linux Kernel-Oriented Approach」Lecture 14（Chapter 4, Part I）錄影逐字稿（YouTube 自動字幕，41:55）、課本 *Operating Systems: A Linux Kernel-Oriented Approach*（Sarangi, v0.96）第 4 章開頭與 §4.1、附錄 C.1，以及 Linux kernel v6.2.12 原始碼整理。實測為本機（Ubuntu、kernel 6.5.0）結果。*
