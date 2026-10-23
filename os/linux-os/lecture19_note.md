# Operating Systems: A Linux Kernel-Oriented Approach 第十九講完整筆記：記憶體模型（Chapter 5, Part II）

> **課程**：Operating Systems: A Linux Kernel-Oriented Approach — Lecture 19（Chapter 5, Part II：Memory models）
> **講者**：Smruti R. Sarangi
> **來源**：YouTube 錄影逐字稿（51:13，自動字幕）＋ 課本 *Operating Systems: A Linux Kernel-Oriented Approach*（Sarangi, v0.96）§5.1.6–5.1.7（p.180–189）＋ Linux kernel **v6.2.12** 原始碼（`os/linux-src/`）
> **標記說明**：
> - `【口頭】`：教授在課堂上口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與教授回答
> - `【課本補充】`：課本有、但這堂課沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄（`檔名:行號`），用來對照觀念
> - `【實測】`：在本機（Ubuntu、kernel 6.5.0、x86-64、20 個 CPU）實際編譯執行的結果
> - `⚠️ 逐字稿缺字`：自動字幕在這裡掉字或聽不清，內容依上下文或課本補上
> - `[mm:ss]`：影片時間點

---

## 目錄

- [0. 這堂課要做什麼](#0-這堂課要做什麼)
- [1. 一個操作的三個時間點](#1-一個操作的三個時間點)
  - [1.1 讀：start ≤ completion ≤ end](#11-讀start--completion--end)
  - [1.2 寫：像把信丟進郵筒](#12-寫像把信丟進郵筒)
  - [1.3 需要一份「合約」](#13-需要一份合約)
- [2. Atomicity](#2-atomicity)
- [3. 把平行執行變成循序執行](#3-把平行執行變成循序執行)
  - [3.1 假設知道完成時間](#31-假設知道完成時間)
  - [3.2 Legal sequential execution](#32-legal-sequential-execution)
- [4. Linearizability](#4-linearizability)
- [5. Sequential consistency（SC）](#5-sequential-consistencysc)
  - [5.1 定義](#51-定義)
  - [5.2 例子：寫了 1，六個月後讀到 0](#52-例子寫了-1六個月後讀到-0)
  - [5.3 SC = 不打亂各自順序的交錯](#53-sc--不打亂各自順序的交錯)
- [6. 真實機器：weak memory model](#6-真實機器weak-memory-model)
  - [6.1 Store buffering：`t1 = t2 = 0`](#61-store-bufferingt1--t2--0)
  - [6.2 【實測】在這台電腦上跑 100 萬次](#62-實測在這台電腦上跑-100-萬次)
  - [6.3 【原始碼】kernel 自己的 litmus test](#63-原始碼kernel-自己的-litmus-test)
- [7. Fence](#7-fence)
  - [7.1 Fence 的規則](#71-fence-的規則)
  - [7.2 【原始碼】x86 的 barrier 家族](#72-原始碼x86-的-barrier-家族)
  - [7.3 【課本補充】Acquire 與 release](#73-課本補充acquire-與-release)
  - [7.4 【原始碼】spinlock 的 unlock 用 release](#74-原始碼spinlock-的-unlock-用-release)
- [8. 最重要的定理：沒有 data race ⟹ SC](#8-最重要的定理沒有-data-race--sc)
- [9. 預告：safety 與 liveness](#9-預告safety-與-liveness)
- [10. 【課本補充】Progress guarantees](#10-課本補充progress-guarantees)
- [11. 一頁速查表](#11-一頁速查表)
- [12. 與後續講次的連結](#12-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 這堂課要做什麼

[00:02] 這講是**非阻塞演算法的理論**。上一講看到 CAS、fetch-and-add 可以不用 lock。但要寫對這種演算法，得先搞懂「操作之間怎麼互動」。

[00:31] 只關心**記憶體操作**：讀和寫。其他指令都只在 CPU 內部，不會跨核心互動。

本講的路線：

```
操作有 start / completion / end 三個時間
        │
        ▼
Atomicity（只有一個大家同意的完成時間）
        │
        ├── + 完成時間在 start 和 end 之間 + 合法 ──▶ Linearizability（最嚴）
        ├── + 同一 thread 內順序不變 + 合法 ─────▶ Sequential consistency
        └── 連這個也放寬 ─────────────────────▶ Weak memory model（真實機器）
                                                    │
                                    用 fence 管秩序  ▼
                       定理：沒有 data race 的程式 → 在任何機器上看起來都是 SC
```

---

## 1. 一個操作的三個時間點

[01:00] 每個操作都有 **start time**、**end time**、**completion time**。

### 1.1 讀：start ≤ completion ≤ end

[01:31] 「記憶體系統」指 L1 cache 以下的所有東西。對 CPU 來說它就是一個整體：送讀出去，等值回來。

| 時間點 | 讀操作 |
|---|---|
| **Start** | 讀離開 pipeline、送進記憶體系統 |
| **Completion** | 值**確定、之後不會再變**的那一刻（值可能還沒回到 CPU） |
| **End** | 值回到 CPU、離開 pipeline，處理完畢 |

[03:31] 讀的情況很單純：**start < completion < end**。

[04:01] 但 completion 通常**不知道**確切時間。做硬體的人知道，跑程式的人看不到。只能確定它存在。

### 1.2 寫：像把信丟進郵筒

[04:31] 寫就複雜多了。教授的比喻：**把信丟進郵筒，你不知道它什麼時候到。**

[05:00] 寫入甚至還沒進 cache，先進 **write buffer（寫入緩衝區）**：

- 一個小緩衝區，放最近的寫入。
- cache line 是 64 bytes，一次寫只有 4 bytes。同一條 cache line 的寫可以先在這裡**合併**。
- 慢慢再排到 cache，再傳到別的核心的 cache。這要花多久不一定。

（課本 Figure 5.11：CPU →（寫入）→ write buffer →（延遲）→ L1 cache。）

[05:32] 寫的三個時間點：

| 時間點 | 寫操作 | 郵筒比喻 |
|---|---|---|
| **Start** | 寫被交給記憶體系統 | 出門 |
| **End** | 寫離開 pipeline，pipeline 忘記它 | 回家 |
| **Completion** | 寫**到達所有核心**、大家都看得到 | 信到收件人手上 |

[06:01] 寫要經過 write buffer、cache、晶片上的網路才到。可能馬上到，也可能**很久以後**。所以寫的 completion 可以**在 end 之後**。

### 1.3 需要一份「合約」

[06:30] 可能性這麼多，所以要定一份**規格（specification）／合約**：程式、程式設計師、編譯器、硬體、不同核心上的 thread，大家都照合約走，才知道什麼結果是「預期的」。

[07:31] 這不只適用於記憶體。記憶體本身就是一個**並行物件（concurrent object）**：它像一個大家可以同時讀寫的 byte 陣列。

[08:00] 同樣的道理可以推到**並行 queue、stack、linked list**。例如 queue 的 `enqueue` 也有 start、end、completion：end 時發出它的 thread 覺得做完了，但其他 thread 要到 completion 才看得到。

---

## 2. Atomicity

[09:30] 如果一個操作只有**一個**完成時間，而且**所有 thread 都同意**是這個時間，這個操作就是 **atomic**：

> **Atomic**：操作**看起來**在完成時間那一瞬間執行完，從 start 到 end 之間看不到任何中間狀態。

[10:00] 例如 `enqueue` 內部會改好幾個欄位，但別人看不到這些中間狀態。別人只看到「做完了」或「還沒做」。

[10:32] **完成時間不用明確知道**。像數學家一樣，只要證明：

1. 存在一個完成時間；
2. 所有 thread 都同意這個時間。

就夠了。

[11:31] **Non-atomic（非 atomic）**：不同 thread 覺得它在不同時間完成。thread 1 覺得在這裡，thread 2 覺得在那裡……大家沒有共識。

【口頭】[11:31] 教授說：手機上跑的 ARM v7 以前就是 non-atomic 的，還有很多伺服器架構也是。幸好近年很多架構都設計成 atomic。這個概念在硬體和軟體都適用。

---

## 3. 把平行執行變成循序執行

### 3.1 假設知道完成時間

[12:31] 三個 thread 一起對一個並行 queue 做 `enqueue`、`dequeue`。光看值，能說什麼？

[13:03] 先注意：**單一 thread 內**，前一個操作 end 之後才能 start 下一個。thread 本身是循序的。

[14:01] 先**假設**知道每個操作的完成時間（怎麼知道先別問）。把所有操作照完成時間排成一條線（課本 Figure 5.10）：

```
Thread 1           enq:3       deq:3     deq:2        deq:4
Thread 2      enq:1            deq:1           enq:4
Thread 3              enq:2                enq:5

照完成時間排成一列，每步後 queue 的內容：
enq:1  enq:3   deq:1  deq:3  enq:2  deq:2  enq:4  deq:4  enq:5 ...
{1}    {3,1}   {3}    {}     {2}    {}     {4}    {}     {5} ...
```

[14:32] 想像有一個**假想的循序 thread**，照這個順序一個一個處理這些操作：

- enq 1 → `{1}`
- enq 3 → `{3,1}`
- deq → 拿到 head，是 1 ✓ → `{3}`
- deq → 3 ✓ → `{}`
- enq 2 → `{2}`
- ……

### 3.2 Legal sequential execution

[15:32] 這個循序執行**符合 queue 的語意**（先進先出）嗎？符合，就叫 **legal（合法）**。

[16:32] 整個想法：

1. 照完成時間把所有操作排成一列。
2. 假設一個循序 thread 在跑它們（原本是不同 thread 發的，不管）。
3. 如果這個循序順序合法，就說**這個平行執行等價於一個合法的循序執行**。

【課本補充】課本說：這個對應很有價值，因為人腦很會推理循序執行，卻很難理解平行執行。

---

## 4. Linearizability

[17:00] 現實是：**start 和 end 知道，completion 不知道。**

[17:30] 那能不能**猜**一組完成時間，讓平行執行等價於合法的循序執行？猜的時候要守規則。

[18:02] 規則：**每個操作的完成時間，一定在它自己的 start 和 end 之間。**

[19:00] 不是每次都猜得出來（後面有例子），但猜得出來的話，就滿足以下規格：

> **Linearizability（可線性化）** = 三者同時成立：
> 1. **Atomic**：每個操作有唯一的完成時間。
> 2. 完成時間在 **start 和 end 之間**。
> 3. 照完成時間排出來的循序執行是 **legal**。

[20:32] 說法有很多種：「這個執行是 linearizable」「符合 linearizability」「和 linearizability 一致」，都是同一件事。

[21:02] 這是**平行執行的一種「正確性」定義**。你說「這個平行執行是對的」，人家問「對是什麼意思？」，就可以說「依 linearizability 是對的」，然後去找每個操作裡的唯一完成點。

[26:30] 這個完成點在文獻裡叫 **linearization point**。很多理論演算法不會明確說它在哪裡，只證明它**存在**。

【口頭】[27:02] 教授的類比：就像矩陣，有時只能說「解存在」（看 rank），但不真的去求解。

[21:31] 問題：linearizability 對**記憶體這種基本操作**太難做到。

[22:02] 原因：寫要等到**真的完成**（傳到所有核心）才能結束，pipeline 要一直記著它。這**太慢、不實際**。

對 queue、stack 這種本來就要做很多步的**高階資料結構**，linearizability 可行。對單一讀寫，太貴。

---

## 5. Sequential consistency（SC）

### 5.1 定義

[22:32] 放寬一點：**sequential consistency（SC，循序一致性）**。

[23:01] SC 也是 atomic，但完成時間**不必**在 start 和 end 之間，可以在任何地方。條件改成：

> **SC** = 三者同時成立：
> 1. **Atomic**。
> 2. **同一個 thread** 發出的操作，完成時間的順序 = 程式中的順序（program order）。
> 3. 排出來的循序執行是 **legal**。

[24:00] 換句話說：**同一個 thread 的操作不可以互相超車。**

[24:32] 但學過**亂序（out-of-order）處理器**的人不會喜歡這條。現代處理器：

- 寫通常很晚才送，要等確定這道指令在正確的路徑上（沒有分支預測錯）。
- 讀會**提早**送，因為讀常在關鍵路徑上。
- 不同位址的操作互相超車很常見，不然效能會很差。

[25:00] 所以連 SC 也不實際。實際的是更寬鬆的 **weakly consistent（弱一致性）**：連「同一 thread 內不超車」都不要求，改用其他（沒那麼綁的）規則。

[25:30] 比較：

| 模型 | 唯一完成時間 | 完成時間在 start–end 之間 | 同 thread 順序不變 | 合法 |
|---|---|---|---|---|
| Linearizability | ✓ | ✓ | ✓（自然成立） | ✓ |
| SC | ✓ | ✗ | ✓ | ✓ |
| Weak | 不一定 | ✗ | ✗（有其他規則） | — |

【課本補充】課本 Point 5.1.4：所有 linearizable 的執行都是 SC；所有 SC 的執行都符合 weak memory model 的要求。反過來不成立。

### 5.2 例子：寫了 1，六個月後讀到 0

[27:32] `x`、`y` 都是 global，初始為 0。

```
Thread 1          Thread 2
x = 1
                  （等 6 個月）
                  讀 x → 0
```

[28:31] **Linearizable 嗎？**

```
Thread 1:  [── W x=1 ──]
Thread 2:                    （6 個月）           [── R x=0 ──]
           完成點一定在這裡面 ↑                    完成點一定在這裡面 ↑
```

完成點都要在各自的 start–end 之間，兩者差了 6 個月，所以順序一定是「W x=1」→「R x=0」。讀到 0 不合法。**不是 linearizable。**

[30:41] **SC 嗎？** SC 不要求完成點在 start–end 之間，寫的完成點可以拖到很晚（某些架構甚至可以排在 start 之前）。

排成「R x=0」→「W x=1」：先讀到初始值 0，再寫 1，**合法**。每個 thread 只有一個操作，沒有 thread 內順序可違反。**是 SC。**

[31:32] 所以這個執行**不屬於** linearizability，但**屬於** SC。

### 5.3 SC = 不打亂各自順序的交錯

[32:00] 另一個看 SC 的方法：SC 就是**把各個 thread 的操作交錯（interleave）起來，但不打亂各 thread 自己的順序**。

【口頭】[32:00] 教授用手指比劃：兩隻手的手指可以互相穿插，怎麼穿插都行，但同一隻手的手指不能換位置。

[33:01] 每種交錯都把平行執行化成循序執行。只要那個循序執行合法，就是 SC。

[33:01] 怎麼證明一個平行執行是對的？選一個標準（linearizability 或 SC），看它滿不滿足，就說「依這個標準是對的」。

---

## 6. 真實機器：weak memory model

### 6.1 Store buffering：`t1 = t2 = 0`

[33:31] `x`、`y` 是 global，初始為 0。`t1`、`t2` 是區域變數（在暫存器裡）。

```
Thread 1        Thread 2
x = 1           y = 1
t1 = y          t2 = x
```

[34:01] 每個 thread 都是寫一個位址、讀**另一個**位址。

[34:33] **`t1 = 0, t2 = 0` 可能嗎？**

在 **SC** 下不可能：

- 交錯中，第一個完成的一定是 `x = 1` 或 `y = 1`（各 thread 的第一個操作是寫）。
- 不失一般性，設 `x = 1` 最先完成。那 `t2 = x` 一定排在它後面，所以 `t2 = 1`。
- 反過來（`y = 1` 最先）也一樣，`t1 = 1`。所以 `t1`、`t2` 至少有一個是 1。

[35:01] 但**在任何真實機器上跑，包括教授的筆電，都會看到 `0, 0`**。

[35:32] 一個平行程式會有很多可能的結果。**哪些結果允許、哪些不允許，由機器的記憶體模型決定。** 看到 `0, 0`，表示這台機器不是 SC。

[36:01] 為什麼？要做到 SC，`t1 = y` 這個讀得**等** `x = 1` 這個寫做完。但讀通常提早送，寫很晚才送。所以實際上兩個讀「先發生」，都讀到 0。

[37:02] 結論：SC 是理論模型，實務上不提供。

[38:02] 【口頭】如果是單一 thread，這個讀和寫（不同位址）**怎麼重排都沒差**。同一位址當然不能重排，不同位址就無所謂。只有在多處理器下，才會出現這種不符合 SC 的結果。

【課本補充】課本說：編譯器和硬體**都可能**重排存取不同位址的指令。現代亂序處理器允許「後面的讀（不同位址）超過前面的寫」。

### 6.2 【實測】在這台電腦上跑 100 萬次

寫一個 store-buffering 測試（`sb.c`）。兩個 thread 綁在 CPU 0 和 CPU 1，主 thread 每輪把 `x`、`y` 清 0 再同時放行：

```c
/* Thread 1 */                          /* Thread 2 */
x = 1;                                  y = 1;
if (mode) asm volatile("mfence" ::: "memory");
else      asm volatile(""       ::: "memory");   /* 只擋編譯器 */
t1 = y;                                 t2 = x;
```

- `plain`：只放 compiler barrier（`asm volatile("" ::: "memory")`）。編譯器不會重排，所以下面看到的重排**全是硬體做的**。
- `mfence`：放一道真正的 fence。

`gcc -O2` 編出來的 Thread 1（`objdump`）：

```
1392:	movl   $0x1,0x2c98(%rip)        # 4034 <x>      ← x = 1
139c:	mov    0x2c72(%rip),%eax        # 4014 <mode>
13a2:	test   %eax,%eax
13a4:	je     13e8 <th1+0xb8>
13a6:	mfence
13a9:	mov    0x2c81(%rip),%eax        # 4030 <y>      ← t1 = y
```

結果（每次 100 萬輪，跑兩次）：

```
plain : t1t2=00:61753 01:553110 10:376585 11:8552
mfence: t1t2=00:0 01:6737 10:120 11:993143
plain : t1t2=00:415307 01:291314 10:293250 11:129
mfence: t1t2=00:0 01:2876 10:212 11:996912
```

- **沒有 fence：`0, 0` 出現了 6 萬到 41 萬次。** SC 說不可能的結果，真的發生了。
- **加了 `mfence`：`0, 0` 一次都沒有。**
- 數字每次差很多，因為跟兩個 thread 剛好多同步有關。重點是有和沒有。

x86 的記憶體模型叫 **TSO（Total Store Order）**，它**只**允許這一種重排：後面的讀超過前面還在 write buffer 裡的寫（store → load）。這正是教授講的 write buffer。

### 6.3 【原始碼】kernel 自己的 litmus test

Linux kernel 有一個正式的記憶體模型（LKMM，`tools/memory-model/`），還附了一堆 **litmus test**（測試記憶體模型用的小程式）。教授的例子就在裡面，叫 **SB（store buffering）**：

`tools/memory-model/litmus-tests/SB+poonceonces.litmus:1-9`

```
C SB+poonceonces

(*
 * Result: Sometimes
 *
 * This litmus test demonstrates that at least some ordering is required
 * to order the store-buffering pattern, where each process writes to the
 * variable that the preceding process reads.
 *)
```

沒加 barrier 的版本：結果是 **Sometimes**（`0, 0` 有時會出現）。

加了 `smp_mb()` 的版本：

`tools/memory-model/litmus-tests/SB+fencembonceonces.litmus:1-32`

```
C SB+fencembonceonces

(*
 * Result: Never
 *
 * This litmus test demonstrates that full memory barriers suffice to
 * order the store-buffering pattern, where each process writes to the
 * variable that the preceding process reads.  (Locking and RCU can also
 * suffice, but not much else.)
 *)

{}

P0(int *x, int *y)
{
	int r0;

	WRITE_ONCE(*x, 1);
	smp_mb();
	r0 = READ_ONCE(*y);
}

P1(int *x, int *y)
{
	int r0;

	WRITE_ONCE(*y, 1);
	smp_mb();
	r0 = READ_ONCE(*x);
}

exists (0:r0=0 /\ 1:r0=0)
```

| 行 | 說明 |
|---|---|
| 4 | `Result: Never`：加了 full barrier，`0, 0` 永遠不會出現 |
| 8 | 「Locking 和 RCU 也行，但其他的大概都不行」：正好呼應本講的定理（§8） |
| 14-21 | P0 = 教授的 Thread 1：`x = 1; t1 = y` |
| 18, 20 | `WRITE_ONCE` / `READ_ONCE`：叫編譯器老實地做一次讀寫，不要合併或省略 |
| 19 | `smp_mb()`：full memory barrier |
| 32 | `exists`：問「`0:r0=0` 且 `1:r0=0`」（就是 `t1 = t2 = 0`）可不可能 |

這兩個檔案的結論，和 §6.2 本機實測完全一致。

---

## 7. Fence

### 7.1 Fence 的規則

[37:31] 怎麼在混亂中建立秩序？定義一組 **atomic 指令**，也叫**同步指令（synchronizing instruction）**。例如 fetch-and-increment、CAS。

[38:30] 現代記憶體模型都是 **weak memory model**：允許重排不同位址的存取，**但**遇到同步指令（教授叫它 `sync`）要守規矩：

```
 同一 thread 中：

   A   B   C          ← 這些可以互相亂序完成
 ─────────────
     sync              ← A、B、C 全部完成，sync 才能完成
 ─────────────
   D   E   F          ← 這些要等 sync 完成後才能完成（彼此可亂序）
```

[39:31] 這種指令叫 **fence**。**所有同步指令都內建 fence。** fence 也有不同種類，這裡講最簡單的（full fence）。

[40:01] fence 很耗時，但工作很清楚：把它兩邊的存取**隔開**。所有 weak memory model 都必須支援 fence。

### 7.2 【原始碼】x86 的 barrier 家族

【口頭】[46:30] 教授提到：Linux kernel 習慣叫 **memory barrier**，不叫 fence，但它們是同一個東西。

`arch/x86/include/asm/barrier.h:14-25`

```c
#ifdef CONFIG_X86_32
#define mb() asm volatile(ALTERNATIVE("lock; addl $0,-4(%%esp)", "mfence", \
				      X86_FEATURE_XMM2) ::: "memory", "cc")
#define rmb() asm volatile(ALTERNATIVE("lock; addl $0,-4(%%esp)", "lfence", \
				       X86_FEATURE_XMM2) ::: "memory", "cc")
#define wmb() asm volatile(ALTERNATIVE("lock; addl $0,-4(%%esp)", "sfence", \
				       X86_FEATURE_XMM2) ::: "memory", "cc")
#else
#define __mb()	asm volatile("mfence":::"memory")
#define __rmb()	asm volatile("lfence":::"memory")
#define __wmb()	asm volatile("sfence" ::: "memory")
#endif
```

| 巨集 | 意思 | x86-64 指令 |
|---|---|---|
| `mb()` | full barrier：讀寫都不能越過 | `mfence` |
| `rmb()` | read barrier：讀不能越過 | `lfence` |
| `wmb()` | write barrier：寫不能越過 | `sfence` |
| 32 位元 | 舊 CPU 沒有這些指令（SSE2 之前），用 `lock; addl $0` 代替；`ALTERNATIVE` 在開機時依 CPU 功能挑一個 | |

這組 `mb()` 是給**裝置 I/O** 用的（註解第 9-11 行：「單 CPU 時跟裝置溝通也可能需要」）。CPU 之間的同步用 `smp_` 開頭那組：

`arch/x86/include/asm/barrier.h:57-80`

```c
#define __smp_mb()	asm volatile("lock; addl $0,-4(%%" _ASM_SP ")" ::: "memory", "cc")

#define __smp_rmb()	dma_rmb()
#define __smp_wmb()	barrier()
#define __smp_store_mb(var, value) do { (void)xchg(&var, value); } while (0)

#define __smp_store_release(p, v)					\
do {									\
	compiletime_assert_atomic_type(*p);				\
	barrier();							\
	WRITE_ONCE(*p, v);						\
} while (0)

#define __smp_load_acquire(p)						\
({									\
	typeof(*p) ___p1 = READ_ONCE(*p);				\
	compiletime_assert_atomic_type(*p);				\
	barrier();							\
	___p1;								\
})

/* Atomic operations are already serializing on x86 */
#define __smp_mb__before_atomic()	do { } while (0)
#define __smp_mb__after_atomic()	do { } while (0)
```

| 行 | 說明 |
|---|---|
| 57 | `smp_mb()`：用一道「加 0 的 locked 指令」當 full fence（上一講看過） |
| 60 | `smp_wmb()` 只是 `barrier()`，**只擋編譯器**，不產生任何指令。因為 x86（TSO）本來就不會把寫和寫重排 |
| 61 | `smp_store_mb`：寫完馬上 fence。用 `xchg`，x86 上 `xchg` 對記憶體自帶 `lock` |
| 63-68 | `smp_store_release`：先 `barrier()` 再寫。x86 上寫不會越過前面的讀寫，所以只要擋編譯器 |
| 70-76 | `smp_load_acquire`：先讀再 `barrier()`。x86 上讀不會被後面的讀寫超過，所以也只擋編譯器 |
| 78-80 | 註解直接說了：「x86 上 atomic 操作本來就有序列化效果」。所以 atomic 前後不用再加 barrier |

重點：在 x86 上，**唯一需要真正 fence 指令的，就是 store → load 這種順序**（`smp_mb`）。這正是 §6.2 的 store-buffering 例子。其他的 barrier 在 x86 上都只是「叫編譯器別亂動」。在 ARM 這種更弱的機器上，同樣的巨集會變成真正的指令。

### 7.3 【課本補充】Acquire 與 release

課本 §5.1.6 介紹了 **RC（relaxed consistency）** 模型，它把 fence 分成兩種：

| 類型 | 對應 | 規則 |
|---|---|---|
| **Acquire** | 拿 lock | 它**之後**的操作，要等它完成才能完成。先拿到 lock，才能碰 critical section |
| **Release** | 放 lock | 它**之前**的操作都完成了，它才能完成。放 lock 時，critical section 裡的修改要讓大家看得到 |

acquire 比 full fence 弱：full fence 還要求它**之前**的操作也有全域順序。

### 7.4 【原始碼】spinlock 的 unlock 用 release

kernel 的 spinlock unlock 正好就是一個 release：

`include/asm-generic/qspinlock.h:123-129`

```c
static __always_inline void queued_spin_unlock(struct qspinlock *lock)
{
	/*
	 * unlock() needs release semantics:
	 */
	smp_store_release(&lock->locked, 0);
}
```

- unlock = 把 `locked` 寫成 0（上一講：「unlock 就是把 lock 位址設回 0」）。
- 但不是普通的寫，是 `smp_store_release`：保證 critical section 裡的所有讀寫，都在這個「寫 0」之前完成。
- 上一講教授說「unlock 還要加一個 memory barrier，下一講再說」，指的就是這個。
- 在 x86 上它只是 `barrier()` + 一次普通的寫（§7.2），一道 fence 指令都沒有。

qspinlock 本身留到第 22 講。

---

## 8. 最重要的定理：沒有 data race ⟹ SC

[40:32] 我們講了 linearizability、SC（理論上好但不實際）、fence。它們用一個定理連起來。

[41:00] 為什麼需要它：你不希望同一個程式，在 SC 的機器上跑一種結果，在非 SC 的機器上跑另一種。你想寫**一個**程式，在**任何**記憶體模型上都能跑。

[41:32] 教授說這是**計算機結構和作業系統裡最重要的定理之一**。

先回顧 properly labeled（PL）程式：每個共享變數都包在 critical section 裡。

[42:01] 新加的條件：**lock 和 unlock 要用內建 fence 的同步指令。**

- lock 之前的東西，要在拿到 lock 前全部完成；lock 之後的東西，不能提早開始。
- [42:30] unlock 之後，critical section 裡做的事要**一瞬間**讓大家都看得到。這就是 fence 的性質。

[43:30] 定理（課本 **Theorem 5.1.1**）：

> **沒有 data race（properly labeled）的程式，在所有機器上都有 sequentially consistent 的執行，不管底下的記憶體模型是什麼。**

【口頭】[43:30] 證明在教授的《Advanced Computer Architecture》書裡。

[44:02] 意義：

- 寫程式時，你知道哪些是共享變數。
- 每個共享變數都用**同一把鎖**包起來。
- lock 函式庫（例如 pthreads）內部會用同步指令，這由函式庫保證。
- 這樣，**不管機器的記憶體模型是什麼，你的執行都是 SC。**

[45:00] 這解決了問題：

- SC 太慢，機器不提供。機器用 weak memory model，允許重排不同位址的存取（對單一 thread 沒差）。
- 但只要你用 lock，[45:32] 執行**看起來**就是 SC。換句話說，外部觀察者光看結果，**分辨不出**你是跑在 weak 機器還是 SC 機器上。

【口頭】[45:32] 教授說定義還要再細調一下，但那屬於計算機結構課的範圍。

[46:00] 為什麼成立？

- data race 是「並行且衝突」的存取。
- 但 fence 讓**依賴關係**得以形成：critical section 裡的東西一定在 unlock 之前發生。
- [46:30] **同步指令彼此之間是 SC 的**：只看那些 atomic 位址的存取，它們是循序一致的。硬體要保證這點。

[48:01] 例子（教授在投影片上畫圖）：

```
Thread 1                   Thread 2
lock
x = 1
unlock ─┐
        │ fence 之間是 SC，
        │ 有 happens-before
        └──────────────▶ lock
                          x = 2
                          unlock
```

- `x = 1` → unlock：fence 的定義，它之前的都要先完成。
- unlock → lock：fence 指令彼此之間是 SC 的，一定有順序。
- lock → `x = 2`：fence 的定義，它之後的都要晚完成。
- happens-before 有遞移性 → `x = 1` 和 `x = 2` 之間有順序，**不再是並行的**。

[49:00] 所以重點只有一個：**在對的地方放 fence**。實務上就是：每個共享變數配一把（或一組）鎖，存取前 lock。

[49:31] 只要程式是 properly labeled（＝沒有 data race），機器的記憶體模型（Intel、ARM 都一樣）就**不重要了**。

---

## 9. 預告：safety 與 liveness

[50:00] 確定了「正確」之後，下一步要問：操作**最後會做完嗎**？光是正確還不夠。

[50:30] 教授用紅綠燈舉例：

| 性質 | 意思 | 紅綠燈的例子 |
|---|---|---|
| **Safety（安全性）** | 壞事永遠不會發生 | 兩個垂直方向不能同時綠燈，不然會撞 |
| **Liveness（活性）** | 好事最後一定會發生 | 紅燈最後一定會變綠 |

safety 不保證 liveness：「永遠紅燈」很安全，但沒人過得去。

[50:30] 下一講講 liveness，以及更進階的主題。

---

## 10. 【課本補充】Progress guarantees

本講最後只預告了 liveness。課本 §5.1.7 接著講**進度保證（progress guarantees）**，這裡先整理，詳細看下一講。

先換掉「實際時間」：課本改用 **internal step（內部步驟）**來量時間：讀一個變數、寫一個變數、做一道基本指令，都算一步。不同 thread 的步數之間沒有關係（除非有因果關係），因為兩步之間的延遲可以任意長。

| 保證 | 定義 | 例子 |
|---|---|---|
| **Obstruction-free** | 讓其他 n−1 個 thread 都睡著，剩下那一個能在有限步內做完 | 不能用 lock：拿著 lock 的睡著了，別人永遠拿不到 |
| **Lock-free** | 整個系統的總步數超過某個門檻，**至少有一個**操作完成。不會 deadlock、不會 livelock，但個別 thread 可能 starve | 上一講的 CAS 迴圈（Listing 5.6） |
| **Wait-free** | **每個** thread 都在有限步內做完。沒有 starvation | 上一講的 `atomic_fetch_add`（Listing 5.4） |

關係（課本 Figure 5.12，文氏圖）：

```
┌───────────── Obstruction-free ─────────────┐
│     ┌──────────── Lock-free ────────────┐  │
│     │       ┌──── Wait-free ────┐       │  │
│     │       └───────────────────┘       │  │
│     └───────────────────────────────────┘  │
└────────────────────────────────────────────┘
```

- **Wait-free ⟹ lock-free**：每個 thread 都在 k 步內完成 → 系統走 nk 步時，依鴿籠原理至少有一個 thread 走了 k 步、完成了。
- **Lock-free ⟹ obstruction-free**：其他 n−1 個都睡著時，系統所有的步都是那一個 thread 走的，夠多步它一定會完成。
- 反過來都不成立。

這三類都不用 lock，統稱 **non-blocking 演算法**。

為什麼不全用 wait-free？理論上任何演算法都能轉成 wait-free（Herlihy & Shavit 的 universal construction），但 wait-free 通常**很慢**，又很難寫、很難驗證。obstruction-free 又太弱。所以實務上大多用 **lock-free**，在效能、正確性、複雜度之間取得平衡。

---

## 11. 一頁速查表

### 11.1 時間點

| | Start | Completion | End |
|---|---|---|---|
| 讀 | 送出 | 值確定不變 | 值回到 CPU |
| 寫 | 送出 | 所有核心都看得到 | 離開 pipeline |
| 關係 | 讀：start < completion < end；寫：completion 可以在 end 之後 |

### 11.2 正確性模型

| 模型 | 條件 | 實際？ |
|---|---|---|
| Atomic | 唯一、大家同意的完成時間 | — |
| Linearizability | atomic + 完成在 start–end 之間 + legal | 高階資料結構用；單一讀寫太慢 |
| SC | atomic + 同 thread 不超車 + legal | 理論標準；硬體不提供 |
| Weak | 可重排不同位址；用 fence 管 | 所有真實機器 |

Linearizable ⊂ SC ⊂ Weak 允許的結果。

### 11.3 Store buffering

| | `t1 = t2 = 0`？ |
|---|---|
| SC | 不可能 |
| x86 沒 fence | 會（本機 100 萬次中 6 萬～41 萬次） |
| x86 加 `mfence` / `smp_mb()` | 不會（0 次） |

### 11.4 Fence

- fence 之前的全部完成，fence 才完成；fence 之後的要等 fence 完成。
- 所有同步指令（CAS、fetch-and-add、lock/unlock）都內建 fence。
- Acquire（lock）：擋住後面的。Release（unlock）：等前面的。
- Linux 叫 memory barrier。

### 11.5 定理

> 沒有 data race（properly labeled + lock 用同步指令）的程式，在任何機器上都是 SC。

### 11.6 本講看過的原始碼位置

| 位置 | 內容 |
|---|---|
| `tools/memory-model/litmus-tests/SB+poonceonces.litmus` | store buffering，沒 barrier：Sometimes |
| `tools/memory-model/litmus-tests/SB+fencembonceonces.litmus:1-32` | store buffering，`smp_mb()`：Never |
| `arch/x86/include/asm/barrier.h:14-25` | `mb`/`rmb`/`wmb` → `mfence`/`lfence`/`sfence` |
| `arch/x86/include/asm/barrier.h:57-80` | `smp_mb`、`smp_store_release`、`smp_load_acquire`（x86 上多半只擋編譯器） |
| `include/asm-generic/qspinlock.h:123-129` | `queued_spin_unlock` = `smp_store_release(&lock->locked, 0)` |

---

## 12. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| **liveness、progress guarantees（obstruction/lock/wait-free）** | **第 20 講**：RW locks and queues |
| **linearizability 用來證明並行資料結構** | **第 21 講**：Concurrent queues |
| **`smp_store_release`、acquire/release、spinlock** | **第 22 講**：Concurrency in the kernel |
| **「Locking 和 RCU 也能排序 SB」、`READ_ONCE`/`WRITE_ONCE`** | **第 23 講**：RCU |
| **`rmb`/`wmb` 給裝置用** | 第 7 章 I/O（本課程播放清單未涵蓋，待補） |

（講次對應依播放清單標題推估。）

---

## 附：本講一句話總結

> **每個操作有 start、completion、end 三個時間；讀的 completion 在中間，寫的 completion 卻可能遠在 end 之後（write buffer 像郵筒）。atomic 是「只有一個大家都同意的完成時間」；再要求完成時間落在 start–end 之間且排出來合法，就是 linearizability（適合 queue 這類高階資料結構，對單一讀寫太慢）；只要求同一 thread 不超車，就是 sequential consistency（理論標準，但硬體不提供）。真實機器是 weak memory model：store-buffering 測試 `x=1; t1=y || y=1; t2=x` 在本機 100 萬次中出現 SC 不允許的 `0,0` 達 6 萬～41 萬次，加一道 `mfence` 就變 0 次，和 kernel 自己的 litmus test（Sometimes / Never）一致。秩序靠 fence（Linux 叫 memory barrier）建立，所有同步指令都內建 fence；而最重要的定理是：沒有 data race 的程式在任何機器上看起來都是 SC，所以只要用鎖把共享變數包好，就不用管底下的記憶體模型。**

---

*筆記依據 Smruti R. Sarangi「Operating Systems: A Linux Kernel-Oriented Approach」Lecture 19（Chapter 5, Part II）錄影逐字稿（YouTube 自動字幕，51:13）、課本 *Operating Systems: A Linux Kernel-Oriented Approach*（Sarangi, v0.96）§5.1.6–5.1.7，以及 Linux kernel v6.2.12 原始碼整理。實測為本機（Ubuntu、kernel 6.5.0、x86-64）結果。*
