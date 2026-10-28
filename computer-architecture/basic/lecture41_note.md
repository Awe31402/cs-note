# Basic Computer Architecture 第四十一支影片完整筆記：Multithreading（coarse、fine、SMT）、vector 處理器與 GPU 入門（Chapter 11 Multithreading and Vector Processing, Part 4/5 → 課本 Ch12）

> **課程**：Basic Computer Architecture — Chapt 11: Multithreading and vector processing, Part 4/5（播放清單第 41 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：YouTube 錄影逐字稿（1:17:38，自動字幕）＋ 課本 *Basic Computer Architecture* v3.09 **Ch12** §12.4.8–12.5（p.608–620）
> **章號**：影片的 Chapter 11 是課本 v3.09 的 **Chapter 12**。
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄（本講沒有）
> - `【實測】`：在本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4）實際執行的結果
> - `⚠️`：逐字稿缺字、或影片／課本／實測說法不一致之處
> - `[mm:ss]`：影片時間點
>
> ⚠️ 影片最後 10 分鐘的 **GPU**（graphics processor）在課本 v3.09 裡**沒有**對應的章節（2015 年版有、新版拿掉了），那部分只有影片的內容。

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. Multithreading：多個 thread 共用一條 pipeline](#1-multithreading多個-thread-共用一條-pipeline)
- [2. Coarse-grained multithreading](#2-coarse-grained-multithreading)
- [3. Context switch 與 zero-overhead context switching](#3-context-switch-與-zero-overhead-context-switching)
- [4. 什麼時候切換、適合什麼工作](#4-什麼時候切換適合什麼工作)
- [5. Fine-grained multithreading](#5-fine-grained-multithreading)
- [6. Simultaneous multithreading（SMT）](#6-simultaneous-multithreadingsmt)
- [7. 三種 multithreading 的圖](#7-三種-multithreading-的圖)
- [8. Vector 處理器：SIMD](#8-vector-處理器simd)
- [9. Vector register 與 vector 加法](#9-vector-register-與-vector-加法)
- [10. Vector load／store：連續與 scatter-gather](#10-vector-loadstore連續與-scatter-gather)
- [11. 實例：用 SSE 加兩個陣列](#11-實例用-sse-加兩個陣列)
- [12. Vector 裡的 if：predicated instruction](#12-vector-裡的-ifpredicated-instruction)
- [13. Vector 處理器的設計](#13-vector-處理器的設計)
- [14. GPU 入門](#14-gpu-入門)
- [15. 【實測】](#15-實測)
- [16. 一頁速查表](#16-一頁速查表)
- [17. 與後續講次的連結](#17-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

```
multithreading：好幾個 thread 跑在同一條 pipeline 上
  coarse-grained：每 n 個週期（或遇到 L2 miss）換一個 thread
  fine-grained ：每 1–2 個週期就換
  SMT          ：同一個週期裡，把 issue slot 分給好幾個 thread（Intel 叫 hyperthreading）
vector 處理器（SIMD）：一條指令處理整個陣列
  vector register（SSE：XMM0–15，128 bit）、連續 load／store、scatter-gather
  SSE 範例：_mm_load_ps、_mm_add_ps、_mm_store_ps；strip mining
  vector 的 if：vector compare → vector flags → predicated 指令
GPU：很多很小的核心；vertex → rasterization → fragment → frame buffer；GPGPU
```

---

## 1. Multithreading：多個 thread 共用一條 pipeline

[00:00]–[03:28]　課本 §12.4.8、Definition 131（p.608–609）

【口頭】一種不同的多處理器設計：用的資源比傳統 multicore 少，但仍然很有效率，「至少十到十五年前就有了，大部分處理器業者都採用」。

複習：process 之間通常**不**共享位址空間，thread 之間可以。

**Multithreading**：讓**很多 thread 跑在同一條 pipeline 上**。pipeline 裡同時有 T1、T2、T3、T4 的指令，pipeline 知道怎麼分別執行它們。

為什麼有用（【口頭】[02:56]）：T1 在 L2 miss，要去主記憶體，**200–300 個週期**動不了。與其讓 pipeline 閒著，不如拿別的 thread 的指令來做 → **產出率（throughput）**變高。

【口頭】[02:06] 我們設計的 pipeline 一次只處理一條指令；現代處理器的 pipeline 一級可以同時處理 3、4、5、6 條指令，但基本想法一樣。

【課本補充】p.609：除了 coherence 和 consistency，現在還要多一個保證：**軟體看不出自己跑在 multithreaded 處理器上**，每個 thread 都覺得自己有一顆專屬的 CPU。

---

## 2. Coarse-grained multithreading

[03:28]–[05:34]　課本 Fig 12.17（p.609–610）

最簡單的一種。四個 thread、一條 pipeline（可以是一次處理多條指令的複雜 pipeline）：

```
T1 跑 n 個週期 → T2 跑 n 個週期 → T3 → T4 → T1 → …   （round robin，輪流）
```

- 例如 n = 10：從 T1 讀 10 個週期的指令，再從 T2 讀 10 個週期……
- 好處：**好做**。雖然 pipeline 裡同時會有不同 thread 的指令，但大多在不同的級，好管理。
- T1 卡住（長延遲的記憶體操作）時，可以很快換到別的 thread 做點事再回來。

---

## 3. Context switch 與 zero-overhead context switching

[05:34]–[09:41]　課本 p.610

換 thread 叫 **context switch**（換情境）。**context** = 這個 thread 的 **PC 和暫存器**。從 T1 換到 T2：要存下 T1 最後的 PC 和暫存器（像處理 interrupt 時那樣，但要快得多），再載入 T2 上次存的狀態。這有成本。

**4-way coarse-grained multithreaded 處理器**（4-way = 4 個 thread）要有：
- **4 個 PC**、**4 個暫存器檔**、**4 個 flags 暫存器**（每個 thread 一份）
- 一個 **context register**：記錄現在是哪個 thread（1–4）

```
PC1  PC2  PC3  PC4          暫存器檔1 … 暫存器檔4
         ↑                         ↑
    context register = 2  → 現在用 PC2、暫存器檔2
```

**Zero-overhead context switching**（零成本切換）：每個 thread 的狀態都在自己那份 PC 和暫存器檔裡。要從 thread 1 換到 2，只要把 context register 從 1 改成 2，處理器馬上就用 thread 2 的 PC 和暫存器檔。代價是多了硬體（4 份 PC、4 份暫存器檔），但已經證明能**大幅**提高整體產出率。

【課本補充】Important Point 20（p.610）：
- **Software thread**：程式語言的概念，共享部分位址空間、互相合作的子程式。
- **Hardware thread**：一個 software thread（或單 thread 程式）連同它的執行狀態，跑在 pipeline 上；跟 pipeline 的實體資源綁在一起。
- 一個 software thread 可以對到一個獨立的處理器，也可以對到一個 hardware thread，它自己不知道也不在乎。

---

## 4. 什麼時候切換、適合什麼工作

[09:41]–[12:02]　課本 p.610

例子：T1 L2 miss，要等 200 週期 → 換 T2；T2 也 L2 miss → 換 T3。

切換的策略可以很複雜：
- 每 n 個週期換一次，或
- 遇到**長延遲事件**（L2 miss、等很久的 I/O）就換，不一定要等滿 n 個週期。

目標：整個系統**閒置的週期最少、產出率最高**。

**很適合的工作：web server**（【口頭】[10:44]）：處理器、記憶體系統、一排硬碟存著網頁。很多使用者同時要網頁、圖片、影片。一個請求的指令很快處理完，送訊息去記憶體或硬碟拿檔案，這段時間那個 thread 閒著 → 換別的 thread。這樣可以用比較少的處理器得到很好的產出率。網路 I/O 也一樣：一個 thread 送了網路訊息在等回應，另一個 thread 接手用 pipeline。「所以很多伺服器處理器公司都採用了某種 multithreading。」

【課本補充】p.610：不需要換到一個也在等長延遲事件（例如也在 L2 miss）的 thread；要換到**有一堆準備好可以執行的指令**的 thread。可以設計很多種啟發式規則。

---

## 5. Fine-grained multithreading

[12:02]–[15:36]　課本 p.611

和 coarse-grained 幾乎一樣，只是切換的粒度**小很多**：每 **1–2 個週期**就換（coarse 是 10、20、30 個週期）。

好處：可以利用**短延遲**的事件：
- **除法**：【口頭】回想計算機算術那幾講，除法很慢，要 10–15 個週期（[第 24–26 支](lecture26_note.md)）。除法器忙的時候，加法器和乘法器是空的，可以拿來做別的 thread 的指令。
- **L1 cache miss**：也要 10–15 個週期。

**正確性的問題**（【口頭】[13:50]）：pipeline 裡同時有兩個不同 thread 的指令。「一定要記住」：
- **永遠不在不同 thread 的指令之間做 forwarding**。
- **永遠不因為不同 thread 之間的相依而 stall（interlock）**。
- 每個 thread 要有自己的 forwarding unit；forwarding 和 interlock 的邏輯會變得非常複雜，要判斷指令是不是同一個 thread。

（forwarding、interlock 見[第 32 支](lecture32_note.md)。）

---

## 6. Simultaneous multithreading（SMT）

[15:36]–[21:32]　課本 p.611–612

最有彈性、最有效率，也**最難做**。

**Multiple issue**（多發射）：現代處理器每個週期可以發好幾條指令到功能單元。例如 **3-issue** 處理器每週期 fetch、decode、execute 3 條。我們的 pipeline 每週期進 1 條、出 1 條；**n-issue** 每週期進 n 條、出 n 條。【口頭】怎麼做是進階課程的內容，課本 pipeline 那章有一點點。

**問題：ILP 低的程式**。**ILP**（instruction-level parallelism，指令層級平行度）：能同時執行的指令有多少。

```
add r1, r2, r3
add r4, r1, ...    ← RAW 依賴 r1
add r5, r4, ...    ← RAW 依賴 r4
```

三條都互相依賴，沒辦法平行執行 → ILP 低。這種情況 coarse-grained、fine-grained 都**幫不上忙**。

**SMT 的想法**：把 **issue slot**（發射槽）**分給好幾個 thread**。
- 【口頭】[18:57] 就算 thread 1 和 thread 2 都「表現很差」（每條都依賴前一條），也可以**同時**把兩個 thread 的指令送進 pipeline：它們屬於不同 thread，彼此沒有依賴，可以平行執行 → 產出率**加倍**。
- 同一個週期可以：thread 1 發 2 條、thread 2 發 1 條、thread 3 發 1 條。
- 4-way、4-issue SMT：同時管 4 個 thread、每週期發 4 條，怎麼分要讓整體產出率最大。

需要很聰明的**指令選擇邏輯**，還要兼顧**公平**（不偏心某個 thread）和產出率。

【口頭】[21:19]「所以從 **Intel Pentium 4** 之後、Intel **Xeon**、**IBM Power 7**，大部分商用處理器都實作某種 SMT。」

【課本補充】p.612：
- 實際上所有 thread 不會同時都處在 ILP 低的階段，一個卡住，其他的就多執行一點，所以 SMT 能讓大部分 issue slot 都忙。
- Intel 把 SMT 叫 **hyperthreading**。IBM Power 7（2012）有 8 核，每核 4-way SMT。
- n-way SMT 需要更多記憶體頻寬；fetch 邏輯要在同一個週期從 4 個 PC 抓指令；coherence、consistency 也更複雜。

---

## 7. 三種 multithreading 的圖

[21:32]–[29:13]　課本 Fig 12.18（p.612）

4-way、4-issue 處理器：每一列是一個週期（往下是時間），每一欄是一個 issue slot；四種顏色是四個 thread，**白格子是沒用到的 slot**。

| | 怎麼排 | 白格子 |
|---|---|---|
| (a) coarse-grained（n = 2） | 連續 2 個週期都是 T1，再 2 個週期 T2…… | 很多：一個 thread 找不到夠多同時可以執行的指令（它們的運算元還沒準備好） |
| (b) fine-grained | 每週期換一個 thread | 圖上和 (a) **一樣多** |
| (c) SMT | 每週期把 slot 分給好幾個 thread | **只剩 1 個** |

【口頭】[25:51] 圖上沒畫出短延遲／長延遲操作的影響，不然 (a)、(b) 會有點差別：fine-grained 晚一點回到 T1 時，可能又多了一條指令準備好了，所以 fine-grained 有可能比較好。

SMT 做得最快、產出率最高。

**作業系統怎麼看**（【口頭】[27:56]）：4-way multithreading 的實體 CPU，作業系統通常顯示成 **4 個虛擬 CPU**。程式設計師就照這章的方式寫程式（OpenMP、MPI，或自己寫），每個 hardware thread 對程式設計師和編譯器來說就像一顆 CPU。（本機的 14 核 20 thread 就是這樣，[第 38 支 §14.1](lecture38_note.md#141-本機是一個-multicore而且核心不一樣)。）

---

## 8. Vector 處理器：SIMD

[29:13]–[33:54]　課本 §12.5–12.5.1、Definition 132（p.612–614）

multithreading 還是 MIMD；**vector 處理器**是 **SIMD**。「對某些程式來說 vector 處理器**棒極了**」，很長一段時間是**超級電腦的主力**。

**Vector 指令**：一次處理**整個陣列**，而不是單一個資料。例如把兩個陣列逐項相加：

```
[1, 2, 3, 4] + [5, 6, 7, 8] = [6, 8, 10, 12]    ← 一條指令
```

以前要寫迴圈、至少 4 條 add。這種運算用在所有**陣列、矩陣、線性代數**運算，而線性代數是科學計算、繪圖、資料分析的核心。

歷史：
- 傳統上用在**超級電腦**，例如 **Cray-1**。
- 後來進入一般處理器：Intel 的 **MMX**、**SSE 1–4**、**AVX**；AMD 的 **3DNow!**。現在一顆一般的 Intel 處理器就能跑 vector 指令（例如 SSE2）。

【課本補充】p.613–614：
- SIMD 處理器主要用在科學計算、高強度遊戲、繪圖，**一般用途不大**，但對特定應用非常快。
- 遊戲裡的光影、陰影、動畫、深度、顏色處理，本質都是對點或像素矩陣做線性代數，所以一般處理器也開始支援一點 vector。ARM 有 **Neon**。
- 1990 年代以前的超級電腦大多是 vector 處理器，一直到 GPU 出現。

---

## 9. Vector register 與 vector 加法

[33:54]–[37:36]　課本 §12.5.2、Fig 12.19（p.614–616）

**Vector register**：可以放**好幾個值**（一個陣列）的暫存器。

x86 的例子：**16 個 128-bit** 暫存器 **XMM0–XMM15**，每個可以放：
- **4 個** 32-bit 浮點數，或
- **8 個** 16-bit short 整數（課本：也可以是 4 個 int 或 16 個 1-byte 字元）

⚠️ 影片 [34:07] 說這是「**MMX** 指令集的 128-bit 暫存器」；課本 p.614 說是 **SSE**（Streaming SIMD Extensions）的 XMM0–XMM15。照課本，XMM 是 SSE 的。

**Vector 加法 = 逐項相加**：

```
VR1 = [ 3 | 10 | … | … ]
VR2 = [ 2 | 11 | … | … ]
VR1 + VR2 = [ 5 | 21 | … | … ]   ← 各格之間**不傳進位**，每格是獨立的數
```

結果存到另一個同樣 128-bit 的 vector register。

在 SimpleRisc 加 **8 個 128-bit vector register：vr0–vr7**。

【課本補充】p.616：
- 不一定兩個運算元都是 vector：可以 vector 乘 scalar，也可以只有**一個** vector 運算元。
- Definition 133：一條 vector 指令能同時處理 n 個運算元，就說有 n 條 **data lane**（資料通道）。
- 有時需要在 scalar 和 vector 暫存器之間搬資料的指令，課本留作習題。

⚠️ 課本 p.616 說「SSE 有專門算 sin、cos 的指令」。我在本機 gcc 的 `xmmintrin.h` 等標頭檔裡找不到 `_mm_sin_ps`／`_mm_cos_ps`，只有 `_mm_sqrt_ps`（平方根）、`_mm_rcp_ps`（倒數近似）、`_mm_rsqrt_ps`（平方根倒數近似）這類單一運算元的 SSE 指令。sin、cos 的 vector 版本在 gcc 這邊看起來是函式庫，不是一條指令；這點「待補」。

---

## 10. Vector load／store：連續與 scatter-gather

[37:36]–[43:41]　課本 p.616

**① 連續**（比較簡單）：資料在**連續**的位置。

```
v.ld vr1, 12[r1]    → vr1[0] = mem[r1+12]、vr1[1] = mem[r1+16]、vr1[2] = mem[r1+20]、vr1[3] = mem[r1+24]
v.st vr1, 12[r1]    → 反過來，把 vr1 的四格寫到 r1+12 開始的 16 bytes
```

**② Scatter-gather**（分散－收集）：資料散在**任意**位置。

```
v.sg.ld vr1, vr2    → vr2 的四格是四個位址，到那四個地方拿資料填進 vr1
v.sg.st vr1, vr2    → 把 vr1 的四格分別寫到 vr2 給的四個位址
```

- 名字的意思：資料在記憶體裡是**分散**（scatter）的，load 要把它們**收集**（gather）起來放進一個 vector register。
- 缺點：硬體要做更多事，要送**四個**不同的記憶體請求。連續的版本很可能四筆都在**同一個 cache block** 裡，一次存取就夠。
- 好處：程式本來就是這種存取模式時很有用。

**其他 vector 運算**（【口頭】[43:12]）：vector 加、vector 乘（都是逐項）；vector 乘 scalar：[1, 2, 3] × 2 = [2, 4, 6]。

---

## 11. 實例：用 SSE 加兩個陣列

[43:41]–[50:48]　課本 Example 155、Definition 134（p.617–618）

```c
void sseAdd(const float a[], const float b[], float c[], int N)
{
    /* strip mining */
    int numIters = N / 4;
    for (int i = 0; i < numIters; i++) {
        __m128 val1 = _mm_load_ps(a);        /* 從 a 載入 4 個 float */
        __m128 val2 = _mm_load_ps(b);
        __m128 res = _mm_add_ps(val1, val2); /* 逐項相加 */
        _mm_store_ps(c, res);                /* 存到 c */
        a += 4; b += 4; c += 4;              /* 指標往前 4 個元素 */
    }
}
```

- 一個 128-bit 暫存器放 4 個 float → 迴圈只要跑 **N/4** 次。依 vector register 的大小切分迴圈叫 **strip mining**（條帶開採）。
- `_mm_load_ps`、`_mm_add_ps`、`_mm_store_ps`：gcc 提供的**內建函式**（intrinsic），每個都包著一條真正的組合語言指令。好處：不用寫組合語言；指令集改了，程式也不用改。【口頭】「mm 代表 MMX」。
- `a += 4`：a 是 float 陣列，C 會自動乘上 4 bytes，實際上位址加了 **16 bytes**。

【口頭】[49:46] 這個程式在幾乎所有 x86 的 Linux／Unix 系統上都能跑，**大約快 2 倍**，可能更多。現在的 AVX 有 **512-bit** 暫存器。「2 倍的加速如果要靠硬體進步，大概要等五、六個 Moore's law 週期，差不多 15 年；這裡只要改程式碼就有了。」

【課本補充】p.618：作者在四核 Intel Core i7 上比較，百萬個元素的陣列，SSE 版本快 **2–3 倍**。AVX 有 256 和 512 bit 的暫存器，鼓勵讀者自己用 AVX 寫寫看。（【實測】見 [§15.2](#152-sse-與-avx-加法)。）

---

## 12. Vector 裡的 if：predicated instruction

[50:48]–[65:03]　課本 §12.5.4、Definition 135（p.618–619）

要對 vector 的**每個元素**做：

```c
if (x < 10)
    x = x + 10;
```

例如 [1, 11, 2, 16] → [11, 11, 12, 16]。

**不能**在 vector 裡面跑一個小迴圈、每格分別判斷，那違反 vector 處理器的核心原則：每格都做**一樣**的事。

**步驟 1：vector compare**：把 VR1 的每格和 10 比，結果存進 **vector flags register**（每格一份結果）：

```
1 < 10 ✓   11 < 10 ✗   2 < 10 ✓   16 < 10 ✗
```

【口頭】[53:01] flags 可以像 pipeline 那樣有 E 和 GT 兩個欄位，也可以只存布林值，「怎麼實作都可以，重點是要記下每一格比較的結果」。

**步驟 2：predicated 指令**：靈感來自 **ARM** 的 `addeq r1, r2, r3`：上一次比較相等才做 r1 = r2 + r3，否則變成 **nop**（[第 9–10 支](lecture10_note.md)）。

定義 vector 版：`v.<p>.add`，p 是 **predicate**（條件），例如 **lt**（小於）、**gt**（大於）、**eq**（等於）。

```
v.cmp vr1, 10          ← 每格和 10 比，結果存在 vector flags
v.lt.add vr1, vr1, 10  ← 只有 flags 是「小於」的格子才加 10，其他格子當 nop
```

```
vr1 = [ 1 | 11 | 2 | 16 ]
        ✓    ✗   ✓    ✗
結果  [ 11 | 11 | 12 | 16 ]
```

⚠️ [61:36] 老師寫投影片時把 2 寫成 12，馬上改正。

**好處**（【口頭】[62:12]）：
- 完全**不用 for、while 迴圈**去逐格處理，整個 vector 當成一個不可分割的單位。
- flags 就像一個 **mask**（遮罩）：有些格子做、有些不做。**硬體保持簡單**。
- **消除了分支**：if、for、while 裡的分支都沒了。
- 代價：false 的格子也算了一下（多做一點事），但硬體簡單、沒有分支是「巨大的好處」。

（【實測】：沒有分支到底省了多少，見 [§15.3](#153-vector-的-if沒有分支就不怕猜錯)。）

---

## 13. Vector 處理器的設計

[65:03]–[66:02]　課本 §12.5.5（p.620）

- 要有 **vector 暫存器檔**。大部分一般用途處理器都有 vector 指令，所以也算 vector 處理器。
- 有 **scalar** 和 **vector** 兩種功能單元。
- 記憶體單元要更複雜：支援一般的 vector load／store，可能還有 scatter-gather。ALU 要支援 predicated 指令。

⚠️ forwarding：影片 [65:17] 說除非是 vector 和 scalar 互轉，「一般**不在** vector 和 scalar 之間 forward 值，因為通常不需要」；課本 p.620 則說 interlock 和 forwarding 邏輯要考慮 scalar 之間、vector 之間、**scalar 和 vector 之間**的衝突，forwarding 要在不同種類的功能單元之間傳值。

【課本補充】p.620：
- OF 級讀 vector 暫存器檔和 scalar 暫存器檔；EX 級把 scalar 運算元給 scalar ALU、vector 運算元給 vector ALU；MA 級加上 vector load／store 單元；RW 級分別寫回兩種暫存器檔。
- 大部分處理器的 cache block 大小是 vector register 的整數倍，所以連續的 vector load／store **不用跨 block**，幾乎和 scalar load／store 一樣快（cache 本來就是一次讀一整個 block，再用 MUX 挑需要的 byte）。L1 要改成一次能讀出更多資料。
- scatter-gather 的 vector load／store 比較慢；現代 out-of-order pipeline 本來就能處理延遲不固定的指令，vector 指令可以直接放進去。

---

## 14. GPU 入門

[66:02]–[77:38]　（課本 v3.09 沒有這部分）

GPU（graphics processor，繪圖處理器）有一些 multithreading 的特性、一些 vector 的特性，「是個混合」。

**為什麼需要 GPU**（【口頭】[69:16]）：
- 很多工作很吃繪圖：電腦遊戲、工程和建築的電腦輔助設計、高畫質影片、桌面特效（例如 Mac 上點一下圖示會放大）。關掉 GPU，桌面就會卡、不好看。
- 一般處理器不是為繪圖設計的，要非常大的運算量，而且會把處理器整個綁住、沒辦法做別的事。
- 所以交給**專門**的 GPU。可以在處理器的晶片裡，也可以在外面。
- 2015–2017 年的主要廠商：**NVIDIA**、**Intel**（自己做的 GPU）、**ATI／AMD**（AMD 收購了 ATI）。

**GPU 做什麼**（【口頭】[70:57]）：
- 遊戲開發者寫高階的規則：角色之間、角色和環境怎麼互動（例如一個角色在山之間跳、別的角色射子彈）。經過幾層軟體，比較高階的指令送到 GPU。
- GPU 處理遊戲裡的一組物件，產生一連串**畫面**；自動加陰影、光照效果。
- 把複雜的 **3D 場景**變成 **2D 影像**（螢幕只能顯示 2D，例如 1024 × 768 個像素），再加上顏色、**紋理**（texture：木頭和金屬看起來不一樣）。
- 也做一點**物理**：很多電影（《神鬼奇航》，還有很多好萊塢、寶萊塢電影）的水和地形特效是電腦產生的，劇組不用真的跑到海中央。
- 也做傳統的事，例如播影片，有 **MPEG-4** 編碼器和解碼器。

**傳統的繪圖 pipeline**（【口頭】[73:46]）：

```
形狀、物件、規則、特效
  → vertex processing（頂點處理）：對所有形狀做基本運算，把整個場景用一堆**三角形**表示
  → rasterization（光柵化）：把三角形切成一小塊一小塊的像素片段（fragment）
  → fragment processing（片段處理）：加上顏色和紋理（例如讓一塊長方形看起來像木頭）
  → frame buffer processing：多層 3D 場景加上深度資訊，變成最後的 2D 像素陣列
  → 螢幕
```

**GPGPU**（【口頭】[67:43]）：架構師發現 GPU 有**驚人的數值運算能力**，也很適合一般的線性代數和科學計算 → **GPGPU**（general purpose GPU，通用 GPU）。
- GPU 是各種模式的混合：可以跑好幾個工作（MPMD 風格）、很多 thread 執行完全一樣的指令（SIMD 風格）、還有 fine-grained multithreading。
- NVIDIA 用的詞是 **SIMT**（single instruction multiple threads）。
- 「分類是什麼不重要」，重點是 GPU 有**很多很多很小的核心**，例如 128 或 256 個。

下一講從 GPU 當成運算引擎開始講。

---

## 15. 【實測】

程式在 `experiments/lecture41/`。

### 15.1 SMT：ILP 低時 2 倍，ILP 高時沒用

`smt.c`：本機每個 P-core 有 2 個 hardware thread（hyperthreading，`/proc/cpuinfo` 有 `ht`；cpu2 和 cpu3 是同一個實體核心）。每個 thread 做 2 億次 `imul`，比較三種擺法：

- **chain**：每條 imul 都依賴上一條（ILP 低，影片 §6 的「表現很差的程式」）。
- **indep**：4 條互不相干的 imul 鏈輪流做（ILP 高）。

```
$ gcc -O2 -pthread -o smt smt.c && ./smt
chain: 1 thread  129 ms | 2 threads on the same core  128 ms (throughput x2.01) | on 2 cores  128 ms (x2.01)
indep: 1 thread   43 ms | 2 threads on the same core   92 ms (throughput x0.93) | on 2 cores   43 ms (x2.00)
```

- **ILP 低**：同一個核心跑兩個 thread，花的時間和跑一個一樣 → 產出率 **2 倍**，和用兩個實體核心一樣好。一個 thread 每 3 個週期才能發一條 imul（等上一條的結果），乘法器有 2/3 的時間是空的；第二個 thread 剛好填進去。這就是 SMT 把 issue slot 分給不同 thread 的效果。
- **ILP 高**：一個 thread 自己就把乘法器塞滿了（[第 30 支 §13.4](lecture30_note.md#134-真的-cpu乘法器本身也是-pipeline) 量過：每週期 1 條）。同一核心再加一個 thread 完全沒好處（0.93 倍），兩個 thread 搶同一組功能單元；換成兩個實體核心才有 2 倍。

### 15.2 SSE 與 AVX 加法

`simd.c` 把課本 Example 155 原樣寫出來，再加一個 AVX（256-bit，一次 8 個 float）版本。陣列各 4096 個 float（三個共 48 KB），重複 5 萬次，量的是運算不是記憶體。一般迴圈用 `-fno-tree-vectorize` 編譯，免得 gcc 自己把它變成 vector 指令。

```
$ gcc -O2 -mavx2 -fno-tree-vectorize -o simd simd.c && taskset -c 2 ./simd
add  scalar   44.9 ms | SSE (4 floats)   15.1 ms  x2.98 | AVX (8 floats)    6.4 ms  x7.00
```

- SSE 快 **3.0 倍**：比影片的「約 2 倍」多，在課本「2–3 倍」的上緣。
- AVX 一次 8 個，快 **7.0 倍**。暫存器越寬越快，和課本說的一樣。
- 本機 CPU 支援 SSE4.2、AVX、AVX2，**沒有** AVX-512（`/proc/cpuinfo` 沒有 `avx512f`）。

### 15.3 Vector 的 if：沒有分支就不怕猜錯

同一個檔案，`if (x < 10) x += 10` 對 4096 個元素做 2000 次（每次重新填資料，值在 0–19，一半小於 10）。SSE 版用 `_mm_cmplt_ps`（vector compare，結果當 mask）加上 `_mm_and_ps`（只有 mask 為真的格子加 10），就是 §12 的 predicated add。`experiments/lecture41/simd.c:27-35`：

```c
static void scalarIf(float *x, int n) { for (int i = 0; i < n; i++) if (x[i] < 10) x[i] += 10; }
static void ssePred(float *x, int n)
{
	__m128 ten = _mm_set1_ps(10);
	for (int i = 0; i < n; i += 4) {
		__m128 v = _mm_load_ps(x + i);
		__m128 mask = _mm_cmplt_ps(v, ten);                     /* vector compare：每格 x < 10 就全 1 */
		v = _mm_add_ps(v, _mm_and_ps(mask, ten));               /* 「predicated add」：只有 mask 為真的格子加 10 */
		_mm_store_ps(x + i, v);
```

```
if (x<10) x+=10, pattern data: scalar    2.7 ms | SSE compare+mask   0.6 ms  x4.8
if (x<10) x+=10, random  data: scalar   26.5 ms | SSE compare+mask   0.6 ms  x46.9
```

- 一般迴圈編出來真的有一條分支（`vcomiss` 後面接 `jbe`）。資料有規律（0, 1, …, 19 重複）時，硬體猜得到分支方向，2.7 ms。
- 資料**隨機**時，分支一半一半，猜錯很多次，每次猜錯 pipeline 要清掉重來（[第 30 支 control hazard](lecture30_note.md#11-control-hazard)），慢到 **26.5 ms**，差了 10 倍。
- SSE 的版本**沒有分支**，不管資料怎樣都是 **0.6 ms**，隨機資料時快 **47 倍**。這就是影片說的「消除分支是巨大的好處」。兩個版本的總和（checksum）一樣，結果正確。

---

## 16. 一頁速查表

| 主題 | 重點 |
|---|---|
| Multithreading | 很多 thread 共用一條 pipeline；等長延遲時讓別的 thread 用 |
| Coarse-grained | 每 n 週期（或 L2 miss）換；好做 |
| Context | PC＋暫存器；context switch 要存、載入 |
| Zero-overhead | 每個 thread 一份 PC、暫存器檔、flags，改 context register 就換 |
| 適合 | web server、file server：很多請求、很多 I/O 等待 |
| Fine-grained | 每 1–2 週期換；利用除法、L1 miss 這種短延遲 |
| 正確性 | 不同 thread 之間不 forward、不 interlock |
| ILP | 能同時執行的指令數；相依多 → ILP 低 |
| SMT | 同一週期把 issue slot 分給多個 thread；Intel 叫 hyperthreading |
| OS 的看法 | 4-way multithreading = 4 個虛擬 CPU |
| Vector 指令 | 一條指令處理整個陣列（SIMD） |
| 歷史 | Cray-1；MMX、SSE、AVX、3DNow!、Neon |
| Vector register | SSE：XMM0–15，128 bit，4 個 float 或 8 個 short；格子之間不傳進位 |
| Load／store | 連續（v.ld、v.st）；scatter-gather（v.sg.ld、v.sg.st，較慢） |
| SSE 範例 | _mm_load_ps、_mm_add_ps、_mm_store_ps；strip mining：迴圈 N/4 次 |
| Vector 的 if | vector compare → vector flags（mask）→ predicated 指令（v.lt.add） |
| 好處 | 沒有迴圈、沒有分支、硬體簡單 |
| GPU | 很多小核心；vertex → rasterization → fragment → frame buffer；GPGPU、SIMT |
| 實測 | SMT：低 ILP ×2、高 ILP ×0.93；SSE ×3、AVX ×7；隨機資料時 predicated 比分支快 47 倍 |

---

## 17. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| GPU 當成運算引擎、interconnection network | [第 42 支](lecture42_note.md)：GPUs and Interconnects |
| 計算機架構的未來 | **第 47 支** |
| Forwarding、interlock | [第 32 支](lecture32_note.md) |
| 乘法器的 pipeline、延遲 3 週期 | [第 30 支 §13.4](lecture30_note.md#134-真的-cpu乘法器本身也是-pipeline) |
| ARM 的條件執行（addeq） | [第 10 支](lecture10_note.md) |
| Out-of-order、superscalar（multiple issue 怎麼做） | Advanced Computer Architecture 課程 |

---

## 附：本講一句話總結

> **multithreading 讓好幾個 thread 跑在同一條 pipeline 上，一個 thread 等 L2 miss 或 I/O 時，pipeline 就拿別的 thread 的指令來做：coarse-grained 每 n 個週期（或遇到長延遲事件）換一次，靠每個 thread 各一份 PC、暫存器檔和一個 context register 做到零成本切換；fine-grained 每 1–2 個週期就換，連除法、L1 miss 這種短延遲都能利用，但不同 thread 之間絕不 forward、不 interlock；SMT 則在同一個週期把 multiple-issue 處理器的 issue slot 分給好幾個 thread，連 ILP 很低的程式都能填滿 pipeline（Intel 叫 hyperthreading，作業系統看到的是好幾個虛擬 CPU）。vector 處理器是 SIMD：一條指令處理整個陣列，靠 vector register（SSE 的 XMM0–15，128 bit 放 4 個 float）、連續或 scatter-gather 的 vector load／store、依暫存器寬度切迴圈的 strip mining，以及「vector compare 產生 mask、再用 predicated 指令只對成立的格子運算」來處理 if 而不用分支；GPU 則是很多很小的核心，從繪圖 pipeline 發展成 GPGPU。實測本機 SMT 對 ILP 低的程式讓產出率剛好加倍、對 ILP 高的程式毫無幫助，SSE 加法快 3 倍、AVX 快 7 倍，資料隨機時沒有分支的 vector if 比一般 if 快 47 倍。**

---

*筆記依據 Smruti R. Sarangi「Basic Computer Architecture」Chapt 11: Multithreading and vector processing, Part 4/5 錄影逐字稿（YouTube 自動字幕，1:17:38）、Sarangi *Basic Computer Architecture* v3.09 Ch12 §12.4.8–12.5（p.608–620）整理。實測為本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、gcc 11.4）結果。*
