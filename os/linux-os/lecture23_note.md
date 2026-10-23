# Operating Systems: A Linux Kernel-Oriented Approach 第二十三講完整筆記：RCU（Chapter 5, Part VI）

> **課程**：Operating Systems: A Linux Kernel-Oriented Approach — Lecture 23（Chapter 5, Part VI：RCU）
> **講者**：Smruti R. Sarangi
> **來源**：YouTube 錄影逐字稿（46:40，自動字幕）＋ 課本 *Operating Systems: A Linux Kernel-Oriented Approach*（Sarangi, v0.96）§5.3.5（p.215–228）＋ Linux kernel **v6.2.12** 原始碼（`os/linux-src/`）
> **標記說明**：
> - `【口頭】`：教授在課堂上口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與教授回答
> - `【課本補充】`：課本有、但這堂課沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄（`檔名:行號`），用來對照觀念
> - `【實測】`：在本機（Ubuntu、kernel 6.5.0、x86-64、20 個 CPU）實際編譯執行的結果
> - `⚠️ 逐字稿缺字`：自動字幕在這裡掉字或聽不清，內容依上下文或課本補上；也用來標示課堂說法與原始碼不一致之處
> - `[mm:ss]`：影片時間點

---

## 目錄

- [0. 這堂課要做什麼](#0-這堂課要做什麼)
- [1. 問題：什麼時候可以 free？](#1-問題什麼時候可以-free)
  - [1.1 Reference count 太慢](#11-reference-count-太慢)
  - [1.2 寫的時候做一份新的](#12-寫的時候做一份新的)
  - [1.3 例子：刪掉 linked list 的一個節點](#13-例子刪掉-linked-list-的一個節點)
- [2. RCU 的五個函式](#2-rcu-的五個函式)
- [3. 怎麼用](#3-怎麼用)
  - [3.1 讀：走訪串列](#31-讀走訪串列)
  - [3.2 寫：換掉一個節點，等，再 free](#32-寫換掉一個節點等再-free)
- [4. 【原始碼】`rcu_assign_pointer`](#4-原始碼rcu_assign_pointer)
- [5. 【原始碼】`list_replace_rcu`](#5-原始碼list_replace_rcu)
- [6. `__rcu` 與 `rcu_dereference`](#6-__rcu-與-rcu_dereference)
  - [6.1 【原始碼】`__rcu` 其實是給 sparse 看的標記](#61-原始碼__rcu-其實是給-sparse-看的標記)
  - [6.2 【原始碼】`__rcu_dereference_check`](#62-原始碼__rcu_dereference_check)
  - [6.3 【原始碼】用它走訪串列](#63-原始碼用它走訪串列)
- [7. 為什麼關搶占就夠了](#7-為什麼關搶占就夠了)
  - [7.1 能跑別的 thread = 讀者已經離開](#71-能跑別的-thread--讀者已經離開)
  - [7.2 最簡單的做法：每顆 CPU 跑一個小程式](#72-最簡單的做法每顆-cpu-跑一個小程式)
  - [7.3 【原始碼】`rcu_read_lock` 真的只是關搶占](#73-原始碼rcu_read_lock-真的只是關搶占)
- [8. Tree RCU](#8-tree-rcu)
  - [8.1 Grace period 與 quiescent state](#81-grace-period-與-quiescent-state)
  - [8.2 四種「一定已經離開」的事件](#82-四種一定已經離開的事件)
  - [8.3 用樹收集每顆 CPU 的打勾](#83-用樹收集每顆-cpu-的打勾)
  - [8.4 【原始碼】`rcu_node` 與往上回報](#84-原始碼rcu_node-與往上回報)
  - [8.5 某顆 CPU 太慢怎麼辦](#85-某顆-cpu-太慢怎麼辦)
  - [8.6 【課本補充】為什麼記在 CPU 上而不是 thread 上](#86-課本補充為什麼記在-cpu-上而不是-thread-上)
- [9. Preemptible RCU](#9-preemptible-rcu)
- [10. 【實測】玩具版 RCU](#10-實測玩具版-rcu)
- [11. 一頁速查表](#11-一頁速查表)
- [12. 與後續講次的連結](#12-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 這堂課要做什麼

[00:33] **RCU（Read-Copy-Update，讀–複製–更新）** 是 Linux kernel 裡管理記憶體配置與釋放最重要的機制之一。

[01:00] 教授說它最像 **reader-writer lock**，但是「reader-writer lock **再加上**記憶體配置的部分」。

（上一講結尾教授說 RCU「可以說是 Linux 在同步方面最重要的輸出」。）

---

## 1. 問題：什麼時候可以 free？

### 1.1 Reference count 太慢

[01:00] 在 kernel 裡配置記憶體、之後再釋放，在並行系統裡很麻煩。

[01:33] 例如：一個物件，好幾個 thread 都拿著它的參照。要 free 它很難，因為**可能還有人在用**。

[01:33] 標準解法：**reference count（參照計數）**。

[02:01] 但這個計數本身是**共享變數**，讀寫它要鎖，鎖很貴（要 busy wait）。

【課本補充】課本說：每個 thread 用物件前要「登記」（計數加 1），用完「註銷」（減 1），減到 0 才能 free。在多處理器上這又慢又複雜。RCU **簡單得多**，「簡單的實作就是快的實作」。

[02:30] RCU 要做到：

- 如果一個資料結構裡有一個指標，要能**很快**知道「已經沒人在用這個指標指的東西了」。
- 這樣就能把指標停用，把那個資料結構回收。
- 換句話說：如果有好幾個讀者，要等**所有讀者**都讀完。

【課本補充】課本列出的三個要求：

1. 要能**停用**指向某資料結構的指標，之後就沒人能再存取它。
2. **不用 reference count**，也能知道什麼時候可以安全 free。
3. free 了之後空間可以回收。

最難的是第 2 點。

### 1.2 寫的時候做一份新的

[02:30] 教授說有一點「藏在細字裡」：

[03:00] **每次寫，都做一份新的副本**，而不是原地改。

[03:32] 這會讓設計簡單很多。

[04:02] 流程：

```
1. 要改物件 → 做一份新的副本，在副本上改
2. 把資料結構裡的指標改成指向新副本
3. 舊副本已經「不在資料結構裡」了，但可能還有讀者正在讀它
4. 等所有讀者都讀完
5. kfree 舊副本
```

[04:31] 所以要有一個機制：**等所有讀者讀完**。雖然資料結構已經更新了，舊物件**仍在使用中**；確定沒人在用，才能 free。

> **課本 Point 5.3.2**：
> - RCU 通常用來存取**包在一個「外層資料結構」裡的物件**（像第 14 講用 `list_head` 做的通用串列節點）。
> - 不直接改物件的欄位，而是在私有記憶體裡做一份副本、改完、再「**發布更新**」：改外層資料結構裡的指標，指向新副本。
> - 其他 thread 可能還拿著舊版本的參照，要等它們讀完。這段時間叫 **grace period（寬限期）**。讀取幾乎是**零成本**：不用拿鎖、不用等。
> - grace period 結束（所有讀者讀完），舊物件就能回收。RCU 最大的挑戰是**很快知道所有讀者都讀完了**。

### 1.3 例子：刪掉 linked list 的一個節點

[05:00] RCU 最經典的例子是 **linked list**（課本 Figure 5.19）：

```
刪除前：  A ──▶ B ──▶ C
                ▲
            讀者可能正在讀 B

步驟 1：  A ─────────▶ C       （把 A 的 next 直接指向 C）
                B ──▶ C        B 已經不在串列裡，但讀者手上還有它

步驟 2：  等所有讀者讀完（synchronize）

步驟 3：  free(B)，回收空間
```

[05:00] 教授說這個例子裡「改變是原地做的」：不需要做 B 的副本，只要把指向 B 的指標改成跳過它。

[05:30] 可能有好幾個讀者正在讀 B，要等它們全部讀完（這是一個同步操作），然後才能回收空間。

【課本補充】課本也說：在 C 裡，總是可以用指標運算回到物件的舊位址。這種情況不考慮，因為 RCU 本來就需要**有紀律的程式設計**。

---

## 2. RCU 的五個函式

[05:30] 三種操作：**寫（更新）**、**讀**、**回收記憶體**（確定沒有讀者在看舊版本時）。

課本 Table 5.1（`include/linux/rcupdate.h`）：

| 函式 | 意思 | 實際動作 |
|---|---|---|
| `rcu_read_lock()` | [06:02] 進入**讀端 critical section** | **關搶占** |
| `rcu_read_unlock()` | [07:00] 離開讀端 critical section | **開搶占** |
| `synchronize_rcu()` | [07:00] 讀的階段結束 | **等所有讀者讀完** |
| `rcu_assign_pointer()` | [07:33] 給 RCU 保護的指標賦值（**寫**） | 賦值 + 檢查 + memory barrier |
| `rcu_dereference()` | [08:30] 把 RCU 指標轉成一般 kernel 指標（**讀**） | 讀 → memory barrier（見 §6.2 ⚠️） |

[06:02] 因為它是「聰明的 reader-writer lock」，所以有 `rcu_read_lock`。教授說它**非常快**：只是關搶占。

[06:32] 回想第 22 講 spinlock：關搶占就是去 per-CPU 區域把一個變數加 1，就這樣。[07:00] `rcu_read_unlock` 則是減 1，減到 0 就允許搶占。

[08:01] 寫指標時**不用**一般的賦值，要用 `rcu_assign_pointer`：它做賦值、做一些檢查，還有一個 memory barrier，教授說這「相當重要」。

---

## 3. 怎麼用

[08:30] 教授說：「重點全在**怎麼用**。用過一次，就會懂為什麼它這麼有效率。」

### 3.1 讀：走訪串列

課本 Listing 5.27：

```c
rcu_read_lock();
list_for_each_entry_rcu(p, head, list) {
    t1 = p->a;
    t2 = p->b;
}
rcu_read_unlock();
```

[09:01] 從 `head` 開始走訪每個元素 `p`。`list_for_each_entry_rcu` 就是一個 `for` 迴圈，每次拿一個串列元素、讀它的值。

[09:31] 這是**純讀取**，所以包在 `rcu_read_lock` / `rcu_read_unlock` 之間。

【課本補充】課本說：`rcu_read_lock` 只是把 `pcpu_hot` 裡的 per-CPU 計數加 1（第 22 講），沒有 cache line bouncing，所有記憶體存取都在本地 CPU，非常有效率。

### 3.2 寫：換掉一個節點，等，再 free

課本 Listing 5.28：

```c
list_replace_rcu(&p->list, &q->list);
synchronize_rcu();
kfree(p);
```

[09:31] 要換掉串列中的某個元素，用 `list_replace_rcu` 這個巨集（[10:00] 它最後會呼叫我們之後看的那些函式）。

[10:00] 原本指向 p 的，現在指向 q。舊的 p 要被回收。

[10:31] `synchronize_rcu()`：等 p 的所有讀者離開。之後沒有讀者，就確定 p 沒人在用了。

[10:31] 為什麼之後不會有人再讀到 p？因為下次走訪串列時，**根本走不到 p**：從前一個節點會直接走到 q。[11:01] 「至少應該走不到。用 RCU 的人要確保這件事。」

[11:32] 資料結構必須**設計成**這樣。然後就能放心 `kfree(p)`。

【課本補充】課本強調：之後的讀者會檢查「這個指標指的節點**還在不在**串列裡」。**這不是 RCU 本身強制的**，是程式設計師要遵守的規則。

---

## 4. 【原始碼】`rcu_assign_pointer`

[11:32] 給一個 RCU 保護的指標賦值：

`include/linux/rcupdate.h:518-527`

```c
#define rcu_assign_pointer(p, v)					      \
do {									      \
	uintptr_t _r_a_p__v = (uintptr_t)(v);				      \
	rcu_check_sparse(p, __rcu);					      \
									      \
	if (__builtin_constant_p(v) && (_r_a_p__v) == (uintptr_t)NULL)	      \
		WRITE_ONCE((p), (typeof(p))(_r_a_p__v));		      \
	else								      \
		smp_store_release(&p, RCU_INITIALIZER((typeof(p))_r_a_p__v)); \
} while (0)
```

| 行 | 說明 |
|---|---|
| 518 | 目標：做 `p = v` |
| 520 | [12:00] 先把 `v` 轉成整數型別存起來（教授說這是「看起來很髒的內部變數」，kernel 風格就是一堆底線） |
| 521 | 給 sparse 檢查：`p` 必須是 `__rcu` 指標（§6.1） |
| 523-524 | [12:31] **簡單情況**：`v` 是編譯期常數 `NULL` → 直接 `WRITE_ONCE`。設成 NULL 不用排序：沒有新物件的內容需要先被看到 |
| 525-526 | [13:01] **一般情況**：`smp_store_release(&p, ...)`。傳 `&p` 是因為要改 `p` 的值 |
| 526 | [13:32] `RCU_INITIALIZER((typeof(p))...)`：把 `v` 轉成跟 `p` 一樣的型別（例如 `p` 是 `int *`，`v` 也轉成 `int *`）。教授說這是編譯期決定型別的巨集 |

[14:00] `smp_store_release` 保證在賦值這一刻，**同一個 thread 之前的所有指令都已經完成**（release fence，第 19、22 講）。

為什麼這很重要？寫者的流程是「先填好新物件的內容 → 再發布指標」。release 保證讀者一看到新指標，就一定看得到**填好的**內容，不會看到一個還沒初始化的物件。

`RCU_INITIALIZER` 的定義：

`include/linux/rcupdate.h:485`

```c
#define RCU_INITIALIZER(v) (typeof(*(v)) __force __rcu *)(v)
```

只是一個**型別轉換**：把一般指標轉成 `__rcu` 指標。`__force` 告訴 sparse「我是故意的」。

⚠️ 課本說「kernel 為 RCU 保護的變數定義了一塊**獨立的記憶體區域**，`RCU_INITIALIZER` 檢查指標是否指向這塊區域」。實際上**沒有**這樣的記憶體區域，也沒有執行期檢查。`__rcu` 是給靜態分析工具 sparse 看的**型別標記**（見 §6.1）。RCU 保護的物件就是一般 `kmalloc` 出來的記憶體。

---

## 5. 【原始碼】`list_replace_rcu`

[15:01] 用 `rcu_assign_pointer` 實作 `list_replace_rcu`：在雙向串列裡把 `old` 換成 `new`。

`include/linux/rculist.h:197-205`

```c
static inline void list_replace_rcu(struct list_head *old,
				struct list_head *new)
{
	new->next = old->next;
	new->prev = old->prev;
	rcu_assign_pointer(list_next_rcu(new->prev), new);
	new->next->prev = new;
	old->prev = LIST_POISON2;
}
```

逐步畫出來（P = old 的前一個，N = old 的後一個）：

```
一開始：        P ⇄ old ⇄ N

第 200-201 行： new.next = N，new.prev = P
                P ⇄ old ⇄ N
                 ↖ new ↗           （new 還不在串列裡，沒人看得到它）

第 202 行：     P.next = new        ← 關鍵的一步，用 rcu_assign_pointer
                P → new → N
                從 head 往後走的人，現在會走到 new，不會走到 old

第 203 行：     N.prev = new
                P ⇄ new ⇄ N

第 204 行：     old.prev = LIST_POISON2   （毒化，標示它已經不在串列裡）
```

| 行 | 說明 |
|---|---|
| 200-201 | [15:34] 「不關鍵」的操作：new 的 next、prev 設成跟 old 一樣。這時 new 還沒加進串列，別人看不到 |
| 202 | [16:33] 讓 P 的 next 指向 new。[17:00] 教授說：這一刻起可以說 new **已經在串列裡**了（雖然連結還沒全接好），這就是 **linearization point**（第 19 講），整個操作看起來就在這一點發生 |
| 202 | [17:32] 這一步在 RCU context 裡做，所以系統會追蹤還有多少 thread 在參照 old。[18:01] 數到 0，old 就可以回收 |
| 203 | [18:01] 剩下的連結：N 的 prev 指向 new |
| 204 | [18:30] 把 old 的 prev 設成一個「毒」值，告訴大家它不在串列裡了 |

[18:30] 教授說：這裡假設 **next 指標比較重要**，走訪串列用的是 next。所以只有設 next 那一步用 RCU；prev 是一般寫入。

（補充：所以 RCU 串列的讀者**只能往前走**（用 next）。往回走（prev）不受 RCU 保護。old 的 `next` 沒被毒化，因為正在讀 old 的讀者還可能要從它往後走到 N。）

⚠️ [17:32] 教授說「系統現在在追蹤有多少 thread 參照 old，數到 0 就回收」。實際上 RCU **不計數**（這正是它的重點），而是用 grace period 推斷（§8）。

---

## 6. `__rcu` 與 `rcu_dereference`

### 6.1 【原始碼】`__rcu` 其實是給 sparse 看的標記

[19:02] 宣告一個要給 RCU 保護的指標時，**一定要**加 `__rcu`：

```c
int __rcu *p;
```

[19:30] 不加的話，編譯器和靜態檢查工具不會把它當成 RCU 指標。

[20:01] 教授的說法：只有 RCU 的函式（`list_replace_rcu`、`rcu_assign_pointer` 等）可以存取它。[20:31] 不希望 RCU 指標被當成一般指標用。如果 RCU 存取和一般存取混在一起，就沒辦法保證 RCU 的性質。

[21:03] 直接 `*p` 不行：「編譯器會報錯，分析工具會報錯」。要先轉成一般的 kernel 指標。

定義：

`include/linux/compiler_types.h:19-25`

```c
/* sparse defines __CHECKER__; see Documentation/dev-tools/sparse.rst */
#ifdef __CHECKER__
/* address spaces */
# define __kernel	__attribute__((address_space(0)))
# define __user		__attribute__((noderef, address_space(__user)))
# define __iomem	__attribute__((noderef, address_space(__iomem)))
# define __percpu	__attribute__((noderef, address_space(__percpu)))
# define __rcu		__attribute__((noderef, address_space(__rcu)))
```

- 只有在 `__CHECKER__` 有定義時才有作用，也就是用 **sparse**（kernel 的靜態分析工具，`make C=1`）檢查時。
- `address_space(__rcu)`：把它標成另一個「位址空間」，跟 `__user`（使用者空間指標）、`__iomem`（I/O 記憶體）同一套機制。
- `noderef`：**不准直接解參照**。直接 `*p`，sparse 會警告。
- 用 GCC 一般編譯時，`__rcu` 展開成空的（或 BTF 標籤），**GCC 不會報錯**。

⚠️ 所以：
- 教授說「編譯器會報錯」：是 **sparse** 會報，一般 GCC 不會（教授 [28:00] 自己也說「GCC 不認識 RCU，但巨集寫得讓它能被正確解讀」）。
- 課本說「RCU 有獨立的記憶體區域」：這裡的「位址空間」是**型別系統的概念**，不是真的記憶體區域。`__user` 指標確實指向不同的空間（使用者空間），但 `__rcu` 指標指的就是一般 kernel 記憶體，只是標記成「要透過 RCU 函式存取」。

### 6.2 【原始碼】`__rcu_dereference_check`

[21:32] 把 RCU 指標轉成一般指標，目的是建一個**區域指標**，可以安全地用：

`include/linux/rcupdate.h:459-466`

```c
#define __rcu_dereference_check(p, local, c, space) \
({ \
	/* Dependency order vs. p above. */ \
	typeof(*p) *local = (typeof(*p) *__force)READ_ONCE(p); \
	RCU_LOCKDEP_WARN(!(c), "suspicious rcu_dereference_check() usage"); \
	rcu_check_sparse(p, space); \
	((typeof(*p) __force __kernel *)(local)); \
})
```

| 行 | 說明 |
|---|---|
| 462 | [21:32] 如果 `p` 是 `int *`，`typeof(*p)` 就是 `int`，所以定義 `int *local`。[22:00] `READ_ONCE(p)` 讀出 `p` 的值，強制轉成一般指標，存進 `local` |
| 463 | 開了 lockdep 時：檢查呼叫的地方確實在 `rcu_read_lock` 裡面，否則警告「可疑的 rcu_dereference」 |
| 464 | [22:31] 檢查 `p` 真的是 `__rcu` 指標。（只在 sparse 下有作用，見下面） |
| 465 | [23:03] 「回傳」`local`，強制轉成 `__kernel` 指標，kernel 就可以正常用了 |

`rcu_check_sparse` 的定義：

`include/linux/rcupdate.h:432-436`

```c
#define rcu_check_sparse(p, space) \
	((void)(((typeof(*p) space *)p) == p))
#else /* #ifdef __CHECKER__ */
#define rcu_check_sparse(p, space)
#endif /* #else #ifdef __CHECKER__ */
```

在 sparse 下，它把 `p` 轉成 `__rcu` 再跟 `p` 比；如果 `p` 不是 `__rcu`，位址空間不同，sparse 就警告。一般編譯時它是**空的**。

[23:30] 為什麼對 RCU context 這麼在意？因為這樣才能知道物件**什麼時候沒有讀者**，就能回收它。

[23:30] `READ_ONCE` 包含一個**編譯期 barrier**：編譯器不能把這次讀取跟別的指令重排。

【課本補充】課本：這**不需要**硬體 fence。x86 不是很弱的記憶體模型，它主要只會把「寫」和「之後對不同位址的讀」重排（第 19 講的 store buffering）。其他順序都保留。所以只要擋住編譯器就夠了。kernel 的通則：不需要硬體 fence 的地方就不加。

⚠️ 課本 Table 5.1 寫 `rcu_dereference` 是「讀 → memory barrier」。如上，它在 x86 上**沒有**硬體 barrier。第 461 行的註解「Dependency order」點出真正的原因：讀者接下來讀 `local->a`，這個讀取的**位址取決於**剛剛讀到的 `local`，CPU 不可能在知道位址之前就去讀（**位址依賴**）。幾乎所有 CPU 都保證這種順序，所以讀端不用 fence。（唯一的例外 DEC Alpha，kernel 在 `READ_ONCE` 裡替它加 barrier。）

### 6.3 【原始碼】用它走訪串列

[24:02] 用 `rcu_dereference` 寫出走訪串列的巨集：

`include/linux/rculist.h:688-691`

```c
#define __hlist_for_each_rcu(pos, head)				\
	for (pos = rcu_dereference(hlist_first_rcu(head));	\
	     pos;						\
	     pos = rcu_dereference(hlist_next_rcu(pos)))
```

[24:31] 兩種串列：`struct list_head`（第 14 講，環狀雙向）和 **hlist**。教授說 hlist 也是雙向串列，但**不是環狀的**，有點特別。（hlist 的頭只有一個指標，常用在 hash 表的桶子，省一半空間。）

- 從 head 開始，用 `rcu_dereference` 取第一個節點。
- [25:01] 每次往下一個節點走，也用 `rcu_dereference`。
- `pos` 變成 NULL 就停（hlist 不是環狀的，結尾是 NULL）。
- 串列本身宣告在 RCU context 裡，所以要用 RCU 函式；但 `pos` 是一般 kernel 指標，可以正常用。

`list_head` 版本（§3.1 用的）：

`include/linux/rculist.h:389-393`

```c
#define list_for_each_entry_rcu(pos, head, member, cond...)		\
	for (__list_check_rcu(dummy, ## cond, 0),			\
	     pos = list_entry_rcu((head)->next, typeof(*pos), member);	\
		&pos->member != (head);					\
		pos = list_entry_rcu(pos->member.next, typeof(*pos), member))
```

`list_entry_rcu` 裡面用的也是 `READ_ONCE`，再用 `container_of`（第 14 講）從 `list_head` 找回外層結構。結束條件是「繞回 head」，因為 `list_head` 是環狀的。

---

## 7. 為什麼關搶占就夠了

### 7.1 能跑別的 thread = 讀者已經離開

[25:32] 現在看它**怎麼運作**。

[26:01] 照理說 `rcu_read_lock` 應該要拿一個讀鎖，但**沒有**。[26:01] 教授說：「這就是 RCU 的秘方。**不用傳統的鎖。**」只是關搶占和開搶占，也就是 per-CPU 計數加 1、減 1。

[26:30] 問題：這麼簡單的操作，怎麼讓 `synchronize_rcu` 知道某個物件的讀者都讀完了？

[27:01] 前提（程式設計師的紀律）：`synchronize_rcu` 返回後，讀者下次要讀這個物件時，會先檢查它**還在不在資料結構裡**。

[27:30] 這就是 RCU 指標有用的地方：它只在 RCU context 裡用，自然就會「先檢查、再使用」。如果指標能在任何地方用，就會有 bug，所以編譯器（sparse）和靜態檢查不允許。

[28:30] 所以只要確定**現在沒有讀者**，就能說：節點已經從串列移除，現在沒有讀者，之後的讀者也找不到它，可以刪。

[29:01] 關鍵推理：

```
這顆 CPU 上能跑「別的 thread」
   ⟹ 這顆 CPU 可以被搶占（搶占是開著的）
   ⟹ 不在 RCU 讀端 critical section 裡（進去時會關搶占）
   ⟹ 原本在這顆 CPU 上的讀者，要嘛已經讀完離開，要嘛根本還沒開始
```

[29:30]–[30:33] 兩種情況：

- **還沒開始**：沒問題。它開始時一定從 head 走，走不到已經被移除的節點。
- **已經讀完**：它做了 `rcu_read_lock` … `rcu_read_unlock`。[30:33] 出了這個範圍，就不會再碰那個節點。

[31:00] 如果**所有** thread 都離開了讀端 critical section，節點的空間就能回收。

### 7.2 最簡單的做法：每顆 CPU 跑一個小程式

[31:00] 要確定沒有活躍的讀者：呼叫 `synchronize_rcu` 的那顆 CPU，對**所有其他 CPU** 送 **IPI（處理器間中斷）**，在每顆上跑一個很小的程式。

[31:30] 如果這個小程式**跑得起來**，就表示那顆 CPU 已經在 RCU critical section 外面了。代表兩件事：

1. 在那顆 CPU 上讀這個節點的讀者都讀完了。
2. [32:01] 之後任何 thread 走訪串列都看不到這個節點。

所以可以回收。

課本 Listing 5.33：

```c
foreach_cpu(cpu)
    run_curr_task_on_cpu(cpu);
```

[32:01] 教授：「這個想法很簡單，但很**慢**。」要送 IPI、要在每顆 CPU 上跑一段程式。例如 core 5 在 core 1 上跑一小段程式，跑得起來就知道 core 1 已經離開讀端 critical section 了。

【課本補充】課本的正確性論證：讀者在 Core i 上，拿著讀鎖，所以 Core i 的搶占是關的。Thread j 送 IPI 要在 Core i 上跑一個 task，但搶占關著時任何 task 都跑不了，要等讀者 `rcu_read_unlock` 開了搶占。所以那個小 task **跑起來的那一刻**，讀者一定已經讀完。

### 7.3 【原始碼】`rcu_read_lock` 真的只是關搶占

`include/linux/rcupdate.h:769-776`

```c
static __always_inline void rcu_read_lock(void)
{
	__rcu_read_lock();
	__acquire(RCU);
	rcu_lock_acquire(&rcu_lock_map);
	RCU_LOCKDEP_WARN(!rcu_is_watching(),
			 "rcu_read_lock() used illegally while idle");
}
```

除了第 771 行，其他都是 sparse 和 lockdep 的除錯工具（沒開時是空的）。真正的工作在 `__rcu_read_lock`。

沒開 `CONFIG_PREEMPT_RCU` 時（`rcupdate.h:92-102`）：

```c
static inline void __rcu_read_lock(void)
{
	preempt_disable();
}

static inline void __rcu_read_unlock(void)
{
	preempt_enable();
	if (IS_ENABLED(CONFIG_RCU_STRICT_GRACE_PERIOD))
		rcu_read_unlock_strict();
}
```

就是**一行 `preempt_disable()`**，跟教授和課本說的一模一樣。在不可搶占的 kernel 上（`CONFIG_PREEMPTION` 沒開），`preempt_disable` 甚至只是一個編譯器 barrier，`rcu_read_lock` 的成本**真的是零**。

⚠️ 但本機 kernel 的設定：

```
CONFIG_TREE_RCU=y
CONFIG_PREEMPT_RCU=y
```

本機用的是 **preemptible RCU**（因為 `CONFIG_PREEMPT_DYNAMIC=y`，開機時可以選成完全可搶占，RCU 就得用可搶占的版本）。這時 `__rcu_read_lock` 是另一個版本：

`kernel/rcu/tree_plugin.h:375-378`、`398-406`

```c
static void rcu_preempt_read_enter(void)
{
	WRITE_ONCE(current->rcu_read_lock_nesting, READ_ONCE(current->rcu_read_lock_nesting) + 1);
}
...
void __rcu_read_lock(void)
{
	rcu_preempt_read_enter();
	if (IS_ENABLED(CONFIG_PROVE_LOCKING))
		WARN_ON_ONCE(rcu_preempt_depth() > RCU_NEST_PMAX);
	if (IS_ENABLED(CONFIG_RCU_STRICT_GRACE_PERIOD) && rcu_state.gp_kthread)
		WRITE_ONCE(current->rcu_read_unlock_special.b.need_qs, true);
	barrier();  /* critical section after entry code. */
}
```

- **不關搶占**，改成把 **`current`（這個 task）** 的 `rcu_read_lock_nesting` 加 1。
- 計數放在 **task** 裡，不在 CPU 裡。正是課本 §5.3.5 最後「Preemptible RCU」講的：thread 可能被搶占、遷移到別顆 CPU，所以計數不能是 per-CPU 的（§9）。
- 一樣只是一次本地的加法，沒有共享變數，所以讀端還是很便宜。

---

## 8. Tree RCU

### 8.1 Grace period 與 quiescent state

[32:32] IPI 的做法太慢，所以有兩種有效率的實作：**Tree RCU** 和 **Tiny RCU**。**Tree RCU 是預設**。

【課本補充】Tiny RCU 是給**不可搶占的單處理器**用的，現在已經不太重要。

[33:02] 核心概念是 **grace period（寬限期）**：

```
           讀者  讀者
       讀者    讀者   讀者
  ─────┼────────────────────────┼─────▶ 時間
     移除                      回收
       └──── grace period ─────┘
```

（課本 Figure 5.20）

- [33:33] 從**更新物件**（例如把它從串列移除）到**回收空間**之間這段時間。
- 要夠長，讓所有「正在飛」的讀者都讀完、放掉對物件的參照。

[34:01] 問題是：grace period 要**多長**？怎麼知道它結束了？

[39:30] 讀者的讀端區塊結束後，就進入 **quiescent state（靜止狀態）**：不再讀那個物件了。**所有** thread 都進入 quiescent state，grace period 就結束，可以回收。

### 8.2 四種「一定已經離開」的事件

[34:31] 以下事件發生在某顆 CPU 上，就知道**這顆 CPU 的 grace period 結束了**：

| 事件 | 為什麼 |
|---|---|
| **thread 阻塞（block）** | [34:31] 等資源被擋住，排程器跑別的。讀端 critical section 裡不准搶占、不准跑別的 thread，所以一定已經在外面了 |
| **切到使用者模式** | [35:00] RCU 是純 kernel 的機制，回到使用者模式代表 kernel 的讀端區塊已經結束 |
| **進入 idle 迴圈** | [35:00] 在跑 idle task，目前的 RCU 區塊一定結束了 |
| **context switch 到別的 kernel thread** | [35:32] 讀端 critical section 裡不准搶占 |

[36:02] 這些都依賴「讀端 critical section 裡不准搶占」。

[37:09] 重點：**不用送 IPI、不用真的跑一段程式**。只要觀察這些「有興趣的事件」（阻塞、context switch 等），就能推斷那顆 CPU 的 grace period 一定結束了。[37:30] 只要在這四種事件上**插樁（instrument）**就好。

【原始碼】例如 context switch：排程器的 `__schedule()` 會呼叫

`kernel/sched/core.c:6512`

```c
	rcu_note_context_switch(!!sched_mode);
```

`rcu_note_context_switch` 定義在 `kernel/rcu/tree_plugin.h`（preemptible 版本在第 310 行，非 preemptible 版本在第 891 行）。每次 context switch 都順便告訴 RCU「這顆 CPU 經過了一個 quiescent state」。

[37:30] 教授說 `synchronize_rcu` 要像一個**雙重的 barrier**：

1. 等所有讀者讀完。
2. [38:02] 讀端區塊裡做的任何改變（教授說讀端區塊也可能改狀態，因為不一定每次都做新副本），[38:32] 在讀端 critical section 結束後要讓全世界看到。這只要插一道 fence 就好。

### 8.3 用樹收集每顆 CPU 的打勾

[39:30] 某顆 CPU 的 grace period 結束（發生了上面某個事件），它就**設一個位元**，表示「我進入 quiescent state 了」，也就是它之前的讀端 critical section 都結束了。

[40:01] **所有** CPU 都回報了，呼叫 `synchronize_rcu` 的 thread 就能回收。

[40:30] 怎麼有效率地收集？把所有 CPU 組成一棵**樹**（課本 Figure 5.21）：

```
                     struct rcu_state
                           │
                    struct rcu_node            ← 根
                  ┌────────┼────────┐
            rcu_node   rcu_node   rcu_node     ← 內部節點
             ┌──┴──┐    ┌──┴──┐    ┌──┴──┐
          rcu_data ... rcu_data ... rcu_data   ← 每顆 CPU 一個（per-CPU）
            CPU0  CPU1  CPU2  CPU3  CPU4  CPU5
```

- [40:30] 內部節點是 `struct rcu_node`。教授說這就是之前看過的**增廣樹（augmented tree）**，最下層是位元向量。
- [41:00] 用位元陣列，讓結構有效率、可擴展。
- [41:00] CPU 0 和 CPU 1 都打勾了，它們的父節點就打勾。兩個子節點都打勾了，再上一層打勾……
- 最後**根**打勾了，就知道所有 CPU 都經過 quiescent state 了。

[42:00] 每顆 CPU 都設了一個位元，用這棵樹就能很 scalable 地收集起來。根說「整棵樹都好了」，`synchronize_rcu` 就能返回，物件就能 free。

[42:31] 細節：

- 每顆 CPU 有一個 `struct rcu_data`，存它的狀態。
- [43:01] 可以**註冊一個 callback 函式**：整個系統的 grace period 結束時呼叫它（這就是 `call_rcu()`，不用等、非同步）。
- [43:31] callback 被呼叫後，`synchronize_rcu` 返回。

【課本補充】課本：為什麼用樹？**樹有天然的平行性，避免中心化的狀態。** 每顆 CPU 只要回答一個問題：「我是不是**最後一個**進入 quiescent state 的？」是的話就宣告 grace period 結束。這個動作不在關鍵路徑上，也沒有共享計數器。

【課本補充】實際上每顆 CPU 存的不只一個位元。可能有巢狀的 RCU 呼叫，也要區分「為什麼」進入 quiescent state。現代 kernel 用 **dynticks** 機制：一組計數器，看其中一個是**偶數還是奇數**，偶數表示 CPU 是 quiescent 的。還要記 grace period 經過了幾個、quiescent state 的種類、排程器活動、中斷處理等。

### 8.4 【原始碼】`rcu_node` 與往上回報

`kernel/rcu/tree.h:45-60`（節錄）

```c
struct rcu_node {
	raw_spinlock_t __private lock;	/* Root rcu_node's lock protects */
					/*  some rcu_state fields as well as */
					/*  following. */
	unsigned long gp_seq;	/* Track rsp->gp_seq. */
	unsigned long gp_seq_needed; /* Track furthest future GP request. */
	unsigned long completedqs; /* All QSes done for this node. */
	unsigned long qsmask;	/* CPUs or groups that need to switch in */
				/*  order for current grace period to proceed.*/
				/*  In leaf rcu_node, each bit corresponds to */
				/*  an rcu_data structure, otherwise, each */
				/*  bit corresponds to a child rcu_node */
				/*  structure. */
	unsigned long rcu_gp_init_mask;	/* Mask of offline CPUs at GP init. */
	unsigned long qsmaskinit;
				/* Per-GP initial value for qsmask. */
```

`kernel/rcu/tree.h:78-91`（節錄）

```c
	unsigned long grpmask;	/* Mask to apply to parent qsmask. */
				/*  Only one bit will be set in this mask. */
	int	grplo;		/* lowest-numbered CPU here. */
	int	grphi;		/* highest-numbered CPU here. */
	u8	grpnum;		/* group number for next level up. */
	u8	level;		/* root is at level 0. */
	...
	struct rcu_node *parent;
	struct list_head blkd_tasks;
				/* Tasks blocked in RCU read-side critical */
```

| 欄位 | 說明 |
|---|---|
| `lock` | 每個節點一把 spinlock。CPU 只鎖**自己那一支**，不用搶一把全域的鎖 |
| `gp_seq` | 目前是第幾個 grace period |
| `qsmask` | **還沒回報**的子節點位元。葉節點：每個位元是一顆 CPU；內部節點：每個位元是一個子 `rcu_node`。這就是教授說的「位元向量」，只是**位元是 1 表示還沒好**，清成 0 表示打勾了 |
| `qsmaskinit` | 每個 grace period 開始時 `qsmask` 的初始值 |
| `grpmask` | 自己在**父節點** `qsmask` 裡是哪一個位元 |
| `grplo`, `grphi` | 這個節點管第幾號到第幾號 CPU |
| `parent` | 父節點 |
| `blkd_tasks` | preemptible RCU 用：在讀端 critical section 裡被搶占的 task（§9） |

一顆 CPU 回報 quiescent state 時，往上走：

`kernel/rcu/tree.c:1898-1952`（節錄）

```c
static void rcu_report_qs_rnp(unsigned long mask, struct rcu_node *rnp,
			      unsigned long gps, unsigned long flags)
	__releases(rnp->lock)
{
	...
	/* Walk up the rcu_node hierarchy. */
	for (;;) {
		if ((!(rnp->qsmask & mask) && mask) || rnp->gp_seq != gps) {
			/*
			 * Our bit has already been cleared, or the
			 * relevant grace period is already over, so done.
			 */
			raw_spin_unlock_irqrestore_rcu_node(rnp, flags);
			return;
		}
		...
		WRITE_ONCE(rnp->qsmask, rnp->qsmask & ~mask);
		...
		if (rnp->qsmask != 0 || rcu_preempt_blocked_readers_cgp(rnp)) {

			/* Other bits still set at this level, so done. */
			raw_spin_unlock_irqrestore_rcu_node(rnp, flags);
			return;
		}
		rnp->completedqs = rnp->gp_seq;
		mask = rnp->grpmask;
		if (rnp->parent == NULL) {

			/* No more levels.  Exit loop holding root lock. */

			break;
		}
		raw_spin_unlock_irqrestore_rcu_node(rnp, flags);
		rnp_c = rnp;
		rnp = rnp->parent;
		raw_spin_lock_irqsave_rcu_node(rnp, flags);
		oldmask = READ_ONCE(rnp_c->qsmask);
	}

	/*
	 * Get here if we are the last CPU to pass through a quiescent
	 * state for this grace period.  Invoke rcu_report_qs_rsp()
	 * to clean up and start the next grace period if one is needed.
	 */
	rcu_report_qs_rsp(flags); /* releases rnp->lock. */
}
```

| 部分 | 說明 |
|---|---|
| `rnp->qsmask & ~mask` | 在這個節點把自己的位元**清掉**（打勾） |
| `if (rnp->qsmask != 0 ...) return` | 這一層還有別人沒好 → **我不是最後一個**，結束。（preemptible RCU 還要等 `blkd_tasks` 裡被搶占的讀者） |
| `mask = rnp->grpmask; rnp = rnp->parent` | 我是這一層最後一個 → 換成父節點，用自己在父節點裡的位元，**往上一層**繼續 |
| 換層時先放自己的鎖、再拿父節點的鎖 | 一次只拿一把鎖 |
| `rnp->parent == NULL` → `break` | 到根了 |
| `rcu_report_qs_rsp` | 「我是**整個系統**最後一個進入 quiescent state 的 CPU」→ grace period 結束，叫醒等待者、呼叫 callback、需要的話開始下一個 grace period |

這就是課本說的「只要回答：我是不是最後一個？」，而且大多數 CPU 只會碰到葉節點的鎖就結束。

【實測】本機 RCU 樹的形狀：

```
$ cat /sys/module/rcutree/parameters/rcu_fanout_leaf
16
```

葉節點最多管 16 顆 CPU（`kernel/rcu/Kconfig:162-168`，預設 16）。本機 20 顆 CPU → 2 個葉節點（16 + 4）加 1 個根。上層的扇出 `RCU_FANOUT` 在 64 位元預設是 64，所以要到 16 × 64 = 1024 顆 CPU 才需要第三層。

### 8.5 某顆 CPU 太慢怎麼辦

[43:31] 這看起來還是有點沒效率：**任何一顆** CPU 都能拖慢整個過程。它在睡怎麼辦？

[43:31] 如果某顆 CPU 一直不進入 quiescent state，機制可以**強迫**它：對它送一個 **IPI**，也就是最早那個又慢又沒效率的方法。[44:00] 沒辦法時只好用它，但它一定有效。

[44:00] 如果那顆 CPU 還在跑一個很長的讀端 critical section，IPI 會先排著，等讀端區塊結束才處理。IPI 的中斷 handler 在 quiescent state 中跑，就可以設那個位元，告訴 `synchronize_rcu` 的 thread「這顆 CPU 好了」。

（補充：v6.2.12 的 grace period kthread 會定期做「force quiescent state」（FQS）：例如發現某顆 CPU 處於 idle 或在使用者模式（看它的 dynticks 計數是偶數），就**直接替它回報**，連 IPI 都不用送。真的卡住才送 IPI 或印出「RCU stall」警告。）

### 8.6 【課本補充】為什麼記在 CPU 上而不是 thread 上

課本 Question 5.3.1：thread 比 CPU 多，可能每個 thread 都拿著物件的參照。為什麼 RCU 狀態只記在 CPU 層級？

答：

- 規則是：RCU 保護的物件**只能在 RCU 讀端區塊裡**存取，而且在區塊裡會先檢查它**還在不在**外層資料結構裡。
- 一進入讀端區塊，在區塊結束前**不能被搶占**。
- 所以 thread 不可能一直拿著參照、在區塊外繼續用。下次要用，又得在 RCU context 裡，那時會檢查物件還在不在；被移除了，參照就沒價值了。
- 同理，物件移除、某顆 CPU 進入 quiescent state 後，這顆 CPU 上**任何** thread 都找不到那個物件了。
- 因為讀端區塊裡不允許 thread 互相搶占，只要記錄「目前在這顆 CPU 上跑的 thread 進入 quiescent state」就夠了。

---

## 9. Preemptible RCU

[35:32] 教授提到：也有不靠關搶占的 RCU 變體，特別是在**即時系統**上。

[36:02] 一旦不用關搶占當武器，就得靠 **reference count**，而 reference count 需要鎖，很慢。

[36:30] 即時 kernel 希望**關搶占的時間越短越好**，所以 RCU 變得很難做。

[44:30] 如果讀端區塊可以被搶占（即時 kernel 的需求），機制就複雜多了：

- 永遠沒辦法確定。只有正在跑讀端區塊的 thread 自己知道。
- [45:00] 它做完時，要嘛減一個 reference count（讓別的核心透過 cache coherence 發現），要嘛呼叫一個 callback。兩者都慢。
- 教授：「即時系統的好處是提供保證，代價是效能。」

[45:30] 可搶占的 RCU 區域會造成問題：要加 reference counter，counter 要鎖，而那些傳統鎖的成本比我們現在的方案高。

【課本補充】課本 §5.3.5 最後：

- 古典 RCU 的讀端幾乎零成本；寫端只要讀目前的資料結構、做副本、修改、加回外層資料結構。唯一的挑戰是等讀者讀完，已經解決得很好。
- 讀端區塊如果可以搶占，區塊會被「**人為地拉長**」，之前偵測 quiescence 的方法不能用了。
- 讀者進入區塊時要**登記**、離開時**註銷**：`rcu_read_lock` 加一個計數，`rcu_read_unlock` 減一個計數。
- 這些計數器現在是 **process 的 context** 的一部分，**不是 CPU 的**。因為可能被搶占再遷移；一顆 CPU 上也可能有兩個讀端區塊同時進行（因為搶占）。
- 總結：這讓即時執行和搶占成為可能，代價是 RCU 變慢、變複雜。

⚠️ 跟原始碼對照（§7.3）：v6.2.12 的 preemptible RCU 確實把計數放在 task 裡（`current->rcu_read_lock_nesting`），但它**不是共享的 reference count，也不需要鎖**：每個 task 只改自己的計數，是本地操作，讀端仍然很便宜。複雜的部分在別處：

- 讀者在讀端區塊裡**被搶占**時，`rcu_note_context_switch` 會把它掛到那個 `rcu_node` 的 `blkd_tasks` 串列（§8.4 的欄位）。
- grace period 不只要等所有 CPU 打勾，還要等 `blkd_tasks` 裡這些被搶占的讀者都 `rcu_read_unlock`（§8.4 程式裡的 `rcu_preempt_blocked_readers_cgp(rnp)`）。
- 它們最後一個 `rcu_read_unlock` 時才做額外工作（"unlock special"），把自己從串列移除。

所以「preemptible RCU = 回到加鎖的 reference count」的說法太悲觀了；本機的發行版 kernel 預設就用它。

[46:00] 教授：看 kernel 程式碼，到處都是 RCU。很多函式和資料結構都受 RCU 保護，因為 tree RCU 效能很高。

---

## 10. 【實測】玩具版 RCU

寫一個使用者空間的玩具版 RCU（`toyrcu.c`），概念和 kernel 一樣，跟 `pthread_rwlock` 比較：

- 共享一個設定物件 `struct cfg { long a, b; }`，規則是 **b 一定等於 2a**。
- 8 個讀者不停讀，檢查 `b == 2*a`。
- 1 個寫者更新 2 萬次。

RCU 版本的寫者：

```c
struct cfg *old = atomic_load(&gp);
struct cfg *nw = malloc(sizeof *nw);               /* copy */
*nw = *old; nw->a++; nw->b = 2 * nw->a;           /* update the copy */
atomic_store_explicit(&gp, nw, memory_order_release); /* rcu_assign_pointer */
synchronize_rcu();
memset(old, 0x5a, sizeof *old);                    /* 毒化：讓 a、b 變成垃圾 */
free(old);
```

讀者：

```c
atomic_fetch_add(&rd_ctr[id], 1);                       /* rcu_read_lock：計數變奇數 */
struct cfg *p = atomic_load_explicit(&gp, memory_order_acquire); /* rcu_dereference */
if (p->b != 2 * p->a) b++;                               /* 讀到不一致就記一筆 */
atomic_fetch_add(&rd_ctr[id], 1);                       /* rcu_read_unlock：變偶數 */
```

`synchronize_rcu()`：拍下每個讀者的計數；正在讀（奇數）的，等到它的計數變了才算離開。（每個讀者有**自己的**計數，沒有共享計數。這接近 dynticks「偶數 = quiescent」的想法。）

結果：

```
toy RCU  updates=20000  reads=105018527  inconsistent reads=0
  wall 2.08s
rwlock   updates=20000  reads=49084490  inconsistent reads=0
  wall 2.85s
```

| | 讀取次數 | 時間 | 每秒讀取 | 不一致 |
|---|---|---|---|---|
| 玩具 RCU | 1.05 億 | 2.08 s | 約 5000 萬 | 0 |
| rwlock | 0.49 億 | 2.85 s | 約 1700 萬 | 0 |

- 兩者都**正確**。
- RCU 讀者快了約 **3 倍**：rwlock 的讀鎖要改**共享**的讀者計數，cache line 在 8 顆 CPU 間彈來彈去；RCU 讀者只碰自己的計數。

**把 `synchronize_rcu()` 拿掉**（寫者發布新指標後馬上毒化、free 舊的），跑 7 次：

```
inconsistent reads=0
inconsistent reads=46
inconsistent reads=655
inconsistent reads=14
inconsistent reads=187
inconsistent reads=21
inconsistent reads=94
```

有 grace period 的版本跑 3 次都是 0。

沒有 grace period 時，有讀者在寫者發布之前拿到了舊指標，寫者卻已經把舊物件毒化、free 掉，讀者讀到垃圾（**use-after-free**）。而且次數每次不一樣，有時甚至是 0，這種 bug 很難重現。這就是 `synchronize_rcu` 存在的理由。

---

## 11. 一頁速查表

### 11.1 RCU 的三個角色

| 角色 | 做什麼 | 成本 |
|---|---|---|
| 讀者 | `rcu_read_lock` → `rcu_dereference` → 讀 → `rcu_read_unlock` | 幾乎零（關搶占 / task 計數 +1） |
| 寫者 | 做副本、修改 → `rcu_assign_pointer` 發布 | 一次 release store |
| 回收者 | `synchronize_rcu()`（或 `call_rcu()` 註冊 callback）→ `kfree` | 要等一個 grace period |

### 11.2 五個函式

| 函式 | x86 上實際做的事 |
|---|---|
| `rcu_read_lock` | `preempt_disable()`；preemptible RCU：`current->rcu_read_lock_nesting++` |
| `rcu_read_unlock` | `preempt_enable()`；preemptible RCU：計數 −1 |
| `rcu_assign_pointer` | `smp_store_release`（NULL 時只是 `WRITE_ONCE`） |
| `rcu_dereference` | `READ_ONCE`（靠位址依賴，不用硬體 fence） |
| `synchronize_rcu` | 等所有 CPU 經過 quiescent state |

### 11.3 Quiescent state 的事件

阻塞、回到使用者模式、idle、context switch。都表示「不在讀端區塊裡」。

### 11.4 Tree RCU

- 葉：每顆 CPU 一個 `rcu_data`；內部：`rcu_node`，`qsmask` 記「誰還沒好」。
- 回報：清自己的位元；這層清空了就往上；到根 → grace period 結束。
- 太慢的 CPU：FQS 替它判斷，或送 IPI。
- 本機：葉扇出 16，20 CPU → 2 葉 + 1 根。

### 11.5 ⚠️ 課堂 vs 原始碼

| 說法 | 實際 |
|---|---|
| `__rcu` 指向「RCU 專用記憶體區域」 | sparse 的型別標記，不是記憶體區域 |
| 直接解參照「編譯器會報錯」 | sparse 會警告，GCC 不會 |
| `rcu_dereference` 有 memory barrier | 只有編譯期 barrier；靠位址依賴 |
| 「系統在追蹤有幾個 thread 參照 old」 | RCU 不計數，用 grace period 推斷 |
| preemptible RCU 要回到加鎖的 reference count | 計數在 task 裡、本地操作；被搶占的讀者掛在 `blkd_tasks` |

### 11.6 本講看過的原始碼位置

| 位置 | 內容 |
|---|---|
| `include/linux/rcupdate.h:769-776` | `rcu_read_lock` |
| `include/linux/rcupdate.h:92-102` | 非 preemptible：`preempt_disable` / `preempt_enable` |
| `kernel/rcu/tree_plugin.h:375-378, 398-406` | preemptible：`current->rcu_read_lock_nesting++` |
| `include/linux/rcupdate.h:518-527` | `rcu_assign_pointer` |
| `include/linux/rcupdate.h:485` | `RCU_INITIALIZER` |
| `include/linux/rcupdate.h:459-466` | `__rcu_dereference_check` |
| `include/linux/rcupdate.h:432-436` | `rcu_check_sparse` |
| `include/linux/compiler_types.h:19-25` | `__rcu` = sparse 位址空間 |
| `include/linux/rculist.h:197-205` | `list_replace_rcu` |
| `include/linux/rculist.h:389-393` | `list_for_each_entry_rcu` |
| `include/linux/rculist.h:688-691` | `__hlist_for_each_rcu` |
| `kernel/sched/core.c:6512` | context switch 時 `rcu_note_context_switch` |
| `kernel/rcu/tree.h:45-91` | `struct rcu_node` |
| `kernel/rcu/tree.c:1898-1952` | `rcu_report_qs_rnp`：沿樹往上回報 |

---

## 12. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| **context switch 是 quiescent state、`__schedule()`** | **第 24–26 講**：排程 |
| **即時 kernel 要縮短關搶占的時間** | **第 27 講**：Real-time schedulers |
| **RCU 保護的 VMA、page table、`mm` 結構** | **第 29–33 講**：記憶體管理（第 10 講的 maple tree 也用 RCU） |
| **`kfree`、slab 配置器** | **第 33 講**：Kernel memory allocation |
| **第 17 講 `sighand` 的 `__rcu`** | 回顧：signal handler 表是 RCU 保護的 |

（講次對應依播放清單標題推估。）

---

## 附：本講一句話總結

> **RCU 解決「物件從資料結構移除後，什麼時候才能 free」：寫者不原地改，而是做副本、改完再用 `rcu_assign_pointer`（一次 release store）發布新指標；讀者用 `rcu_read_lock`（只是關搶占）和 `rcu_dereference`（只是 `READ_ONCE`，靠位址依賴）讀，幾乎零成本；回收者呼叫 `synchronize_rcu` 等一個 grace period 再 `kfree`。關鍵推理是：讀端區塊裡不准搶占，所以一顆 CPU 只要發生過 context switch、阻塞、idle 或回到使用者模式，原本的讀者就一定讀完了；Tree RCU 把每顆 CPU 的「我好了」沿著 `rcu_node` 樹往上清 `qsmask`，最後一個到根的 CPU 宣告 grace period 結束，太慢的 CPU 再用 IPI 催。`__rcu` 其實是 sparse 的型別標記，不是獨立記憶體區域；本機用的 preemptible RCU 把計數放進 task、被搶占的讀者掛在 `blkd_tasks`。實測玩具版 RCU 讀取比 rwlock 快約 3 倍且完全正確，拿掉 grace period 就出現 use-after-free。**

---

*筆記依據 Smruti R. Sarangi「Operating Systems: A Linux Kernel-Oriented Approach」Lecture 23（Chapter 5, Part VI）錄影逐字稿（YouTube 自動字幕，46:40）、課本 *Operating Systems: A Linux Kernel-Oriented Approach*（Sarangi, v0.96）§5.3.5，以及 Linux kernel v6.2.12 原始碼整理。實測為本機（Ubuntu、kernel 6.5.0、x86-64）結果。*
