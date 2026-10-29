# Advanced Computer Architecture 第二十支影片完整筆記：GPGPU 的設計：GPC、SM、warp、SIMT 與 register file（Chapter 6, Part III）

> **課程**：Advanced Computer Architecture — Chapter 6: GPUs (Part III) -- Design of GPGPUs（播放清單第 20 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：YouTube 自動字幕（51:48）＋ 課本 *Next-Gen Computer Architecture* v3.2 **Ch6** §6.4–6.5（p.233–247）
> **章號**：影片 Chapter 6 = 課本 Ch6。
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄（本講沒有）
> - `【實測】`：在本機（NVIDIA RTX 4060 Laptop GPU、driver 580.178.04、CUDA 13.0、Ubuntu、kernel 6.5.0）實際執行的結果
> - `⚠️`：逐字稿缺字、或影片／課本／實測說法不一致之處
> - `[mm:ss]`：影片時間點

⚠️ 自動字幕把「Volta」聽成「voltage」、「PB」聽成「pp／pv」、「warp」聽成「award／bar／wall／walk」、「lockstep」聽成「log step／lobster」、「bank conflict」聽成「ban conflict」、「Itanium」聽成「titanium／italian」，筆記直接改正。

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. 整顆 GPU：NVIDIA Volta](#1-整顆-gpunvidia-volta)
- [2. GPC 與 TPC](#2-gpc-與-tpc)
- [3. SM](#3-sm)
- [4. SM 裡的四種記憶體](#4-sm-裡的四種記憶體)
- [5. Processing block（PB）](#5-processing-blockpb)
- [6. 能同時跑幾個 thread](#6-能同時跑幾個-thread)
- [7. Warp 與 lockstep](#7-warp-與-lockstep)
- [8. 有分支怎麼辦：predicated execution](#8-有分支怎麼辦predicated-execution)
- [9. 用 stack 記住在哪一條路上](#9-用-stack-記住在哪一條路上)
- [10. Lockstep 會 deadlock](#10-lockstep-會-deadlock)
- [11. Restricted lockstep](#11-restricted-lockstep)
- [12. GPU 的 pipeline](#12-gpu-的-pipeline)
- [13. Register file：bank 與 operand collector](#13-register-filebank-與-operand-collector)
- [14. L1 cache 的 bank conflict](#14-l1-cache-的-bank-conflict)
- [15. Chapter 6 總結](#15-chapter-6-總結)
- [16. 【實測】](#16-實測)
- [17. 一頁速查表](#17-一頁速查表)
- [18. 與後續講次的連結](#18-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

```
NVIDIA Volta：PCIe ↔ CPU；GigaThread engine（排 thread）；6 個 GPC；大 L2；NVLink；8 個 memory controller ↔ HBM2
GPC → 7 個 TPC（＋ raster engine）→ 每個 TPC 2 個 SM（＋ Polymorph engine）→ 每個 SM 4 個 PB
PB：L0 i-cache、warp scheduler、dispatch、64 KB register file、8 FP64、16 INT、16 FP32、2 tensor core、LD/ST、SFU
warp = 32 個 thread 共用一個 PC、lockstep；分支用 predication ＋ 每個 thread 一個 stack
lockstep 會 deadlock → Volta 的 restricted lockstep
pipeline：fetch → decode → warp scheduler（scoreboard）→ register file → 執行 → write back
一個 warp 一次要 1024 bit → 多 bank 的 register file ＋ crossbar ＋ operand collector；L1 也有 bank conflict
```

---

## 1. 整顆 GPU：NVIDIA Volta

[00:00] 以 **NVIDIA Volta**（GV100）為例。GPU 都是很大的晶片，有大量的計算單元：

| 部分 | 做什麼 |
|---|---|
| **PCI Express 3.0** | 跟 host（CPU）溝通的外部 bus |
| **GigaThread engine** | GPU 的 thread 管理和排程器：「GPU 的大腦」。【口頭】「不是人的大腦，比較像鳥的大腦，是個小腦袋。」 |
| **GPC**（graphics processing cluster） | 一大群小 core，Volta 有 **6 個** |
| **L2 cache** | 很大，GPU 的 cache 比 CPU 的大 |
| **NVLink** | 把好幾顆 GPU 接在一起的高速連線 |
| **8 個 memory controller** | 接到 **HBM2**（high bandwidth memory，Ch10） |

【課本補充】課本 p.233：課本的參考架構「大致類似 NVIDIA Volta，但不完全一樣」；有 6 個 NVLink controller。

---

## 2. GPC 與 TPC

[02:42] 每個 **GPC**：

- 一個 **raster engine**：[第 18 支](lecture18_note.md#12-rasterizer) 說過 rasterization 不適合平行化，要專用的硬體。
- **7 個 TPC**（texture processing cluster）：名字是歷史遺留的，不是只處理紋理。

[03:51] 設計哲學：**圖形專用的硬體越少越好**。

[04:22] 每個 **TPC**：

- 一個 **Polymorph engine**（[第 18 支](lecture18_note.md#11-polymorph-engine) 的 5 級）。它在現代 GPU 裡盡量把計算對到通用的單元上：從工程的角度，能重複使用 GPU 做一般計算是最好的。
- **2 個 SM**（streaming multiprocessor）。大部分計算都對到 SM 的 core 上。

---

## 3. SM

[05:46] 每個 **SM**：

| 元件 | 說明 |
|---|---|
| **L1 instruction cache** | 4 個 PB 共用。GPU 的一條指令可以做很多事：同一條**靜態**指令被很多 thread 執行，變成很多條**動態**指令 |
| **4 個 processing block（PB）** | 又多一層階層 |
| **128 KB shared L1** | 資料 |
| **4 個 texture unit／texture cache** | 圖形專用：遊戲裡紋理資料放在專用的地方比較好 |

[06:26] 算一下 Volta：6 GPC × 7 TPC × 2 SM = **84 個 SM**；× 4 = **336 個 PB**，「還沒到最底層的 core，但快了」。

---

## 4. SM 裡的四種記憶體

[08:02]

| 記憶體 | 放什麼 |
|---|---|
| **L1 instruction cache** | 像 CPU 的 i-cache |
| **L1 data cache** | 一般資料 |
| **Texture cache** | 紋理 |
| **Constant cache** | 常數 |

[08:38] CUDA 的 **`__shared__`**（[第 19 支](lecture19_note.md#9-記憶體的種類)）就是對到 **shared L1** 裡的一塊。

[09:06] 寫程式要注意 GPU 的資源：shared 陣列比 shared cache 大不算錯，但會變慢。

【課本補充】課本 p.236：shared memory 是 SM 裡所有 core 都能存取的一小塊記憶體（64–128 KB）。

---

## 5. Processing block（PB）

[09:39] 最底層。每個 **PB**：

| 元件 | 說明 |
|---|---|
| **L0 instruction cache** | SM 有 L1，PB 有 L0 |
| **Warp scheduler** | 每個 cycle 處理 32 個 thread（一個 **warp**），整個 warp 一次排程 |
| **Dispatch unit** | 把一個 warp 送到 core 上；warp 是不可分割的單位 |
| **Register file** | **16K 個 4-byte 暫存器 = 64 KB**。PTX 假設暫存器無限多，「離現實也不算太遠」。這麼大是為了資料的區域性和很高的平行度 |
| **8 個 FP64** 單元 | 雙精度少：一直用雙精度很貴 |
| **16 個 INT** 單元 | |
| **16 個 FP32** 單元 | |
| **2 個 tensor core** | 專做**矩陣乘法**，直接支援深度學習（Ch14） |
| **Load/store 單元** | 跟記憶體溝通 |
| **SFU**（special function unit） | sin、cos、tan、sigmoid 之類，主要用 **lookup table** 算 |

[11:49] 每個 core 就像一個「加了一點東西的 ALU」：很簡單的 pipeline，2–3 級，很多時候 1 級。

[12:24] 一個 warp 的整數指令：32 個 thread，只有 16 個 INT 單元 → **2 個 cycle**（前 16 個、後 16 個）。FP32 也一樣。

[13:30] Tensor core 一次處理一整個矩陣；兩個大矩陣相乘要先切成小的子矩陣。

---

## 6. 能同時跑幾個 thread

[14:34]

```
6 GPC × 7 TPC × 2 SM × 4 PB × 16 = 5376 個 thread 同時
```

[15:08] 現代 GPU 可以同時跑兩個 kernel，同時用 INT 和 FP32 單元 → 再 × 2，約 **10,000 個**。

[15:39] GPU 的時脈比較低（約 1.5 GHz，CPU 3–3.5 GHz），但平行度巨大：「就像把一台 CPU 做的超級電腦搬進一顆 GPU」。

[16:08] 【口頭】估算：一個 CPU core 最好的情況約 3 GFLOPS；× 1000 = 3 TFLOPS；再 × 10 = **30 TFLOPS** 大約是一顆 GPU。約 **33 顆 GPU** 就有 1 PFLOPS。用 CPU 難多了（雖然多核 CPU 也會乘上去）。

（本機的數字見 [§16.1](#161-每個-sm-每個-cycle-幾個-fma)。）

---

## 7. Warp 與 lockstep

[17:46] 能不能給**每個 thread 自己的 PC**，讓它們跑不同的指令？**不行**，負擔太大。

[18:15] 做法：32 個 thread 一組，叫 **warp**（thread block 的一個子集合），共用**同一個 PC**，**lockstep**（齊步）執行：

- 同一條指令，所有 thread 都做完，才換下一條。
- 16 個單元 → 先做 16 個、再做 16 個；8 個單元 → 4 個 cycle。

[19:20] 好處：簡單，不用每個 thread 一個 PC。

[19:52] 代價：一個 thread 因為長延遲的操作要等，**整個 warp 都等**。

[20:00] 聽起來沒效率，但 GPU 的程式碼通常**很規則、很好預測、很同質**，資料在記憶體裡也放在一起，所以這種情況很少發生。

【課本補充】課本 p.237：這叫 **SIMT**（single instruction, multiple thread）模型，現代 GPU 大多採用；warp 是 SIMT 的核心概念。課本 p.238：如果一個 warp 裡有的 thread cache miss，SIMT 會讓效能嚴重下降。

【課本補充】課本 p.241：warp scheduler 存著一組 warp，每幾個 cycle 選一個執行幾條指令。NVIDIA **Tesla** 最多存 24 個 warp、每個 cycle 選一個；**Fermi** 一次可以選兩個 warp 同時跑；**Kepler** 每個 SM 4 個 warp scheduler；Volta 每個 PB 一個。同時跑兩個不相關的 warp，一個用 ALU、一個用 load/store，就能讓更多功能單元忙起來。

---

## 8. 有分支怎麼辦：predicated execution

[20:28] if/else 的時候，32 個 thread 裡可能 10 個走 if、22 個走 else，怎麼 lockstep？

[20:58] **Predicated execution**（[第 17 支 §12](lecture17_note.md#12-predicated-execution)）：

- 兩個 thread **都處理** if 的身體。
- 對 thread 1 predicate 是真 → 執行、commit 結果。
- 對 thread 2 是假 → 拿到指令但忽略，不做任何事。
- [21:31] 重點：warp 裡所有 thread **還是 lockstep**；只是每條指令，每個 thread 都要判斷「這在我的正確路徑上嗎？」

[22:38] 例子：

```c
if (x > 0)      // 兩個 thread 都做比較
    …           // thread 1 做，thread 2 忽略
else
    …           // thread 1 忽略，thread 2 做
x = c;          // 兩個都做：point of reconvergence
```

[23:07] 兩邊又開始一起做的那一點叫 **point of reconvergence**（重新匯合點）。

（本機量了 warp 內分岔的代價，見 [§16.2](#162-warp-divergence)。）

---

## 9. 用 stack 記住在哪一條路上

[23:38] **每個 thread 一個 stack**（不是程式的 call stack，只存 predicate 的結果）：

- 進入 if 或 else 的身體 → push：在正確路徑 **1**，錯誤路徑 **0**。
- 離開 → pop。
- [24:11] 為什麼是 stack？**巢狀的 if**：最後進去的最先出來（LIFO）。

[25:52] 一條指令**執行**的條件：

- stack 是空的（不在任何分支裡），或
- stack 裡**全部都是 1**。

任何一個是 0 → 在某個分支的錯誤路徑上，不執行。

[26:51] 誰決定什麼時候 push/pop？**CUDA runtime** 加進特別的指令或 bit 給硬體；**stack 由硬體維護**。

[27:24] 三個 thread（1A、1B、2）的巢狀例子：

```c
                         1A        1B        2
if (x < 0) {          │ push 1  │ push 1  │ push 0
    if (y == 0) {     │ push 1  │ push 0  │ push 0（已經在錯的路上，可以不比）
        …             │ 1,1 做  │ 1,0 不做│ 0,0 不做
    }                 │ pop     │ pop     │ pop
    …                 │ 1 做    │ 1 做    │ 0 不做
} else {              │ pop, push 0 │ pop, push 0 │ pop, push 1
    …                 │ 不做    │ 不做    │ 做
}
x = c;                │ 全部 pop → 三個都做（point of reconvergence）
```

【課本補充】課本 p.240：每條指令前都讀整個 stack 算 AND 不實際，所以把結果存成一個 **32-bit 的 bit mask**（每個 thread 一個 bit），進入或離開條件敘述時才更新。

---

## 10. Lockstep 會 deadlock

[31:15] Lockstep 有它的問題。例子（x 是 **shared** 變數，4 個 thread 在同一個 warp）：

```c
__shared__ int x = 0;
if (threadIdx.x < 2) {
    while (x == 0);      // thread 0、1：等 x 變成非 0
} else {
    x = 1;               // thread 2、3：把 x 設成 1
    …
}
```

[32:31] thread 0、1 進 if；thread 2、3 也「進」（同一條指令，對它們是錯誤路徑），等著兄弟 thread 0、1 做完。

[32:58] 「好玩的來了」：thread 0、1 永遠在 while 裡等，因為**沒有人會把 x 設成 1**。照邏輯 thread 2、3 應該去設 x = 1，但它們在等 thread 0、1 離開 while，**永遠到不了**那一行。

[34:32] 只看程式邏輯沒問題，問題只出在 warp 裡 lockstep 的限制。這是刻意做出來的例子，但真實世界有很多類似的情況：程式 deadlock，開發者完全不知道為什麼。

【課本補充】課本 p.242：這種同步存取可以在**不同 warp** 的 thread 之間做，但**不能在同一個 warp 裡**。解法是讓每個 thread 有自己的執行狀態：自己的 **PC** 和 **call stack**，但這會破壞 lockstep、增加負擔。

---

## 11. Restricted lockstep

[35:00] 現代 GPU：**有限度、受控制的 thread divergence**。三個程式區塊：W（while 迴圈）、X（x = 1）、Y（後面的）。

[35:41] **Restricted lockstep execution**：

```
執行 W 一陣子 → 沒有進展
→ 換去執行 X、Y 一陣子（還是 lockstep，predicate 照樣算）→ x = 1 被設好了
→ 回來執行 W → thread 0、1 離開 while
→ if、else 都做完 → 在 point of reconvergence 匯合
```

[37:12] thread 還是一起執行（lockstep），只是**不是一路往下**，而是在 if 和 else 之間跳。[第 10 節](#10-lockstep-會-deadlock) 的 deadlock 就不會發生。

[37:43] 這是 **NVIDIA Volta** 的重要發明之一，之後的 GPU 都保留了。不然很多「黏人的」race condition 都是因為堅持 lockstep 造成的。

【課本補充】課本 p.242：也可以用 CUDA 函數 **`__syncwarp()`** 強迫 thread 重新匯合。

（本機在 RTX 4060 上跑了這個例子，見 [§16.3](#163-影片的-deadlock-例子)。）

---

## 12. GPU 的 pipeline

[38:12] 一個 PB 的 pipeline：

```
fetch（L0、L1 i-cache）→ decode → warp scheduler（scoreboard）→ register file → 功能單元 → write back
                                                                 ├ INT ALU
                                                                 ├ FP32、FP64
                                                                 ├ SFU（sin、cos…）
                                                                 └ LD/ST → shared L1 → L2 → GPU DRAM
```

- [38:41] Warp scheduler 用 **scoreboard**（[第 17 支 §11](lecture17_note.md#11-scoreboard)）：指令卡住，直到四種 hazard（RAW、WAR、WAW、structural）都沒了才發出。
- [40:20] 老師說圖上忘了畫 L2；GPU 的 DRAM 很重要：處理的是大資料，很多資料最後都放在 DRAM 裡。
- [40:54] 基本上是 **in-order pipeline**。In-order 通常不會有 WAR、WAW；但有 [§11](#11-restricted-lockstep) 那種執行方式，就有一點點 out-of-order，所以要 scoreboard 處理。

---

## 13. Register file：bank 與 operand collector

[41:57] 一個 warp 有 32 條 load（或 store），每個 thread 一條，怎麼讀暫存器？

```
32 個 thread × 32 bit = 1024 bit = 128 byte（在 2、3 個 cycle 內）
```

[42:34] 就算一次只送 16 個：一條指令有兩個運算元 → 16 × 2 × 32 = 還是 1024 bit；送 8 個也還要 512 bit。register file 的讀寫頻寬要非常高。

[43:08] 簡單但**不實際**的做法：一個 bank 每格 1024 bit，全部放在一起。不行：事先不知道會存取哪些暫存器、它們會不會放在同一個地方；一次讀這麼多也很慢。

[44:10] **有效率的做法**：

1. Register file 切成**很多個 bank**（子 register file），1024 bit 分散在各個 bank（例如每個 bank 128 bit → 8 個 bank）。
2. 一個 **crossbar network**，由 **arbiter**（仲裁電路）控制，存取有資料的那些 bank。
3. 讀出來的資料先放在 **operand collector**：一個暫存的 buffer，等資料從各個 bank 全部讀好。
4. 再從 operand collector 送到執行單元。

[45:16] 每個 bank 讀的資料比較少 → 快，而且可以平行。複雜度轉移到網路上，所以要聰明的 arbiter。

【課本補充】課本 p.244 §6.4.6 是同樣的設計。

---

## 14. L1 cache 的 bank conflict

[46:21] L1 cache 也一樣（L2 比較沒那麼嚴重）：寫 128 byte = 32 個 4-byte 的 word，分在不同的 bank。如果**兩個 word 在同一個 bank** → **bank conflict**，不能同時存取。

[47:22] 做法：一個專門的硬體先把**互不衝突**的存取送出去（例如 32 個裡找出 25 個），其他的在之後的 cycle 慢慢送。

[47:57] 寫的時候：從下一層搬一個 line 進來時，可以先**鎖住**這個 line；用一個像 **MSHR** 的結構（Ch7）：

- 例：L1 write miss，block 要從 L2 來。
- 這段時間來的讀寫都先放在這個暫存結構裡。
- block 一到，就把這些「在 miss 的陰影裡」到的 load/store 全部套用上去。

（本機量了 shared memory 的 bank conflict，見 [§16.4](#164-shared-memory-的-bank-conflict)。）

---

## 15. Chapter 6 總結

[48:49]

1. GPU 是現代數值、科學、高效能計算**不可或缺**的；也加速圖形應用。CPU 相比之下資源嚴重不足。
2. 早期 GPU 太專用；GPGPU 出現後非常通用。
3. CUDA 讓一般程式跑在 GPU 上；CUDA binary 是 fat binary，有 CPU 和 GPU 的程式碼。
4. GPU 有複雜的階層：GPC → TPC → SM → PB → core，現在有 5000–10000 個 core，一顆 GPU 就有好幾 TFLOPS。
5. 一個 warp 一次要 1024 bit → 需要複雜的多 bank register file 和多 bank L1。

[50:35] 書的第一部分（OOO pipeline 和 GPU）結束：可以塞進一大堆計算單元、提高某些程式的 IPC。但**記憶體系統**也要能高速供應資料、寫回結果，它可能成為瓶頸 → 第二部分（Ch7–10）講記憶體系統。

---

## 16. 【實測】

程式是 `experiments/lecture20/warp.cu`（`nvcc -O2 -std=c++17 -arch=sm_89`）。本機的 RTX 4060 Laptop 是 **Ada** 世代（compute capability 8.9），比影片的 Volta（7.0）新，24 個 SM（[第 19 支 §14.2](lecture19_note.md#142-本機-gpu-的限制)）。每個實驗跑兩次，數字完全一樣。

### 16.1 每個 SM 每個 cycle 幾個 FMA

每個 SM 一個 block、1024 個 thread，每個 thread 8 條互不相干的 FP32 FMA 鏈，用 GPU 上的 `clock64()` 直接數 cycle：

```
1. 每個 SM 每個 cycle 做了 106.9 個 FP32 FMA（24 個 SM → 整顆每 cycle 2566 個）
```

- 影片的 Volta：每個 PB 16 個 FP32 單元、每個 SM 4 個 PB → 每個 SM 每 cycle **64** 個。本機量到 **約 107** 個，比 64 多不少：這一代每個 SM 的 FP32 單元比 Volta 多。確切的單元數 NVIDIA 的資料這裡沒有，**待補**（107 不是整數倍，可能還有一點沒擠滿）。
- 整顆每 cycle 約 2566 個 FMA，每個 FMA 算 2 個浮點運算，以 [第 19 支](lecture19_note.md) 查到的 2370 MHz 估：約 **12 TFLOPS**。影片 [16:41] 說「一顆 GPU 約 30 TFLOPS」，是大顆的桌機／伺服器 GPU；筆電的 4060 少一些，但跟 CPU 比還是差很多（[第 5 支](lecture05_note.md) 本機一個 P-core 每 cycle 只能做幾個加法）。

### 16.2 Warp divergence

`branchy`：if 和 else 兩邊的工作量一樣（各 20000 次 FMA）。分兩種：

- 照 **thread 編號**的奇偶分：同一個 warp 裡一半走 if、一半走 else。
- 照 **warp 編號**的奇偶分：同一個 warp 的 32 個 thread 都走同一邊。

```
2. if/else 照「thread 編號」（同一個 warp 兩邊都要走）分：0.270 ms
2. if/else 照「warp 編號」（同一個 warp 走同一邊）分：0.137 ms
```

- 分岔在 warp **裡面**時剛好慢了 **2 倍**：每個 warp 兩邊都要走一次，一半的 thread 每次都被 predicate 關掉（[§8](#8-有分支怎麼辦predicated-execution)）。
- 分岔在 warp **之間**時沒有代價：每個 warp 只走自己那一邊。所以寫 CUDA 時，盡量讓同一個 warp 的 thread 走同一條路。

### 16.3 影片的 deadlock 例子

`spin`：照 [§10](#10-lockstep-會-deadlock) 的程式，一個 warp 裡 4 個 thread，thread 0、1 在 `while (x == 0)` 等，thread 2、3 設 `x = 1`（x 是 `__shared__ volatile`）。加了一個約 1 秒的逾時，免得真的卡死：

```
3. thread 0、1 等 x 變 1，thread 2、3 設 x = 1（同一個 warp）：順利結束，沒有卡住
```

- 在本機（Volta 之後的 GPU）**沒有 deadlock**：硬體真的會像 [§11](#11-restricted-lockstep) 說的，在 while 沒進展時先去執行 else 那邊，x 被設成 1 之後再回來。
- 影片說 Volta 以前的 GPU 會卡住；本機沒有那麼舊的 GPU，**沒辦法實測**舊的行為。

### 16.4 Shared memory 的 bank conflict

`bank`：一個 warp（32 個 thread），第 t 個 thread 一直讀 shared memory 第 `t × stride` 個 4-byte word（`volatile`，每次真的讀），每圈 8 次互不相干的讀取，用 `clock64()` 量平均每次多久：

| stride | 每次讀取（cycle） | 同一個 bank 被幾個 thread 搶 |
|---|---|---|
| 1 | 5.4 | 1 |
| 2 | 5.6 | 2 |
| 4 | 9.3 | 4 |
| 8 | 17.1 | 8 |
| 16 | 33.0 | 16 |
| 32 | **65.0** | **32**（全部同一個 bank） |
| 33 | 5.4 | 1 |

（最右欄假設 shared memory 有 32 個 bank、每個 bank 4 byte 寬，這是從 stride 33 和 stride 1 一樣快推出來的。）

- stride 32：32 個 thread 讀的 word 全部落在**同一個 bank**，要一個一個來，慢了 **12 倍**。stride 每加倍，時間差不多也加倍：這就是 [§14](#14-l1-cache-的-bank-conflict) 的 bank conflict，衝突的存取只能排到之後的 cycle。
- stride 2 跟 stride 1 幾乎一樣：2 路衝突的代價被藏起來了。
- **stride 33 跟 stride 1 一樣快**：33 除以 32 餘 1，32 個 thread 剛好分散到 32 個不同的 bank。這就是常見的「陣列每列多墊 1 格（padding）」的技巧（程式裡的 `s[32 * 33]` 就是這樣宣告的）。

---

## 17. 一頁速查表

| 主題 | 重點 |
|---|---|
| Volta 整顆 | PCIe 3.0、GigaThread engine、6 GPC、大 L2、NVLink、8 個 memory controller ＋ HBM2 |
| 階層 | GPC（raster engine）→ 7 TPC（Polymorph engine）→ 2 SM → 4 PB；84 SM、336 PB |
| SM | L1 i-cache、128 KB shared L1、texture cache、constant cache；`__shared__` 在 shared L1 |
| PB | L0 i-cache、warp scheduler、dispatch、64 KB register file、8 FP64、16 INT、16 FP32、2 tensor core、LD/ST、SFU |
| Thread 數 | 336 × 16 = 5376（× 2 ≈ 10,000）；約 30 TFLOPS |
| Warp | 32 個 thread、同一個 PC、lockstep（SIMT）；一個卡住整個 warp 等 |
| 分支 | predication；每個 thread 一個 stack（push 1／0、全 1 才執行）；課本用 32-bit mask；point of reconvergence |
| Deadlock | 同一個 warp 裡 thread 互等 → 卡死 |
| Restricted lockstep | Volta：沒進展就換去執行另一邊再回來；`__syncwarp()` |
| Pipeline | fetch → decode → warp scheduler（scoreboard）→ RF → 功能單元 → WB；基本上 in-order |
| Register file | 一個 warp 要 1024 bit → 多 bank ＋ crossbar ＋ arbiter ＋ operand collector |
| L1 | bank conflict：先送不衝突的；miss 時用像 MSHR 的結構暫存 |
| 實測（RTX 4060） | 每 SM 每 cycle 約 107 個 FMA；warp 內分岔慢 2 倍；deadlock 例子沒卡住；stride 32 慢 12 倍、stride 33 不衝突 |

---

## 18. 與後續講次的連結

| 本講觀念 | 在哪裡 |
|---|---|
| Rasterizer、Polymorph engine | [第 18 支](lecture18_note.md) |
| CUDA、thread block、`__shared__`、`__syncthreads` | [第 19 支](lecture19_note.md) |
| Predication、scoreboard | [第 17 支](lecture17_note.md) |
| Cache、bank、MSHR | [第 21 支](lecture21_note.md)–[第 25 支](lecture25_note.md)（Ch7） |
| HBM、記憶體頻寬 | [第 42 支](lecture42_note.md)–[第 45 支](lecture45_note.md)（Ch10） |
| Tensor core、深度學習 | [第 55 支](lecture55_note.md)–[第 58 支](lecture58_note.md)（Ch14） |

---

## 附：本講一句話總結

> **GPGPU 是一個很深的階層：以 NVIDIA Volta 為例，PCIe 接 CPU、GigaThread engine 排程、6 個 GPC（各有 raster engine）× 7 個 TPC（各有 Polymorph engine）× 2 個 SM（L1 i-cache、128 KB shared L1、texture／constant cache）× 4 個 PB（L0 i-cache、warp scheduler、64 KB register file、16 個 INT 和 16 個 FP32 單元、FP64、tensor core、LD/ST、SFU），約 5000–10000 個 thread 同時執行；32 個 thread 組成一個共用 PC、lockstep 執行的 warp（SIMT），分支用 predication 加上每個 thread 一個 1-bit stack（或 32-bit mask）處理，而 lockstep 可能讓同一個 warp 裡互等的 thread deadlock，所以 Volta 改用 restricted lockstep 在兩條路之間輪流跑；pipeline 基本上是 in-order 加 scoreboard，一個 warp 一次要 1024 bit，所以 register file 切成多個 bank、用 crossbar 和 operand collector 收集運算元，L1 也要處理 bank conflict。本機 RTX 4060 實測：每個 SM 每 cycle 約做 107 個 FMA（比 Volta 的 64 多），warp 內分岔剛好慢 2 倍，影片的 deadlock 例子在新 GPU 上順利跑完，shared memory 32 路 bank conflict 慢 12 倍、而 stride 33 的 padding 完全不衝突。**

---

*筆記依據 Smruti R. Sarangi「Advanced Computer Architecture」Chapter 6: GPUs (Part III) -- Design of GPGPUs 的 YouTube 自動字幕（51:48）、Sarangi *Next-Gen Computer Architecture* v3.2 Ch6 §6.4–6.5（p.233–247）整理。實測為本機（NVIDIA RTX 4060 Laptop GPU、driver 580.178.04、CUDA 13.0、Ubuntu、kernel 6.5.0）結果。*
