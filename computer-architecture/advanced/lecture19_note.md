# Advanced Computer Architecture 第十九支影片完整筆記：CUDA：kernel、thread／block／grid、記憶體、stream、graph 與 event（Chapter 6, Part II）

> **課程**：Advanced Computer Architecture — Chapter 6: GPUs (Part II) -- CUDA Programming Language（播放清單第 19 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：**whisper 轉錄**（本機 `large-v3-turbo`；YouTube 自動字幕是亂碼）（55:31）＋ 課本 *Next-Gen Computer Architecture* v3.2 **Ch6** §6.3（p.225–233）
> **章號**：影片 Chapter 6 = 課本 Ch6。
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄（本講沒有）
> - `【實測】`：在本機（Intel i7-13650HX、NVIDIA RTX 4060 Laptop GPU、driver 580.178.04、CUDA 13.0、Ubuntu、kernel 6.5.0）實際執行的結果
> - `⚠️`：逐字稿缺字、或影片／課本／實測說法不一致之處
> - `[mm:ss]`：影片時間點

⚠️ whisper 把「SASS」寫成「SAS」、「cudafe++」寫成「CUDA FE++」、「program order」寫成「program model」，筆記直接改正。

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. CUDA 是什麼](#1-cuda-是什麼)
- [2. PTX 與 SASS](#2-ptx-與-sass)
- [3. nvcc 的編譯流程](#3-nvcc-的編譯流程)
- [4. Kernel](#4-kernel)
- [5. Thread、block、grid](#5-threadblockgrid)
- [6. 2D 的 thread block](#6-2d-的-thread-block)
- [7. 2D 的 grid](#7-2d-的-grid)
- [8. Barrier：__syncthreads](#8-barrier__syncthreads)
- [9. 記憶體的種類](#9-記憶體的種類)
- [10. 完整的 vector addition](#10-完整的-vector-addition)
- [11. Streams](#11-streams)
- [12. CUDA graph](#12-cuda-graph)
- [13. Events](#13-events)
- [14. 【實測】](#14-實測)
- [15. 一頁速查表](#15-一頁速查表)
- [16. 與後續講次的連結](#16-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

```
CUDA = C++ 的擴充（NVIDIA，2006–2007）
nvcc：.cu → CPU 那邊（cudafe++、stub function）＋ GPU 那邊（cicc → PTX → ptxas）→ fat binary
執行時：PTX 可以 JIT 編成這顆 GPU 的 SASS
寫法：__global__ kernel；thread 在 block 裡、block 在 grid 裡（都可以 1D／2D／3D）
  kernel<<<grid, block>>>(…)；idx = blockIdx.x * blockDim.x + threadIdx.x
  __syncthreads() 是 block 內的 barrier
記憶體：__device__、__constant__、__shared__、__managed__
流程：cudaMalloc → cudaMemcpy 進去 → kernel → cudaMemcpy 出來 → cudaFree
加速：streams（複製和計算重疊）→ graph（DAG，預先排程）；events 量時間
```

---

## 1. CUDA 是什麼

[00:00] **CUDA**（Compute Unified Device Architecture），NVIDIA 2006–2007 年推出。

- 設計成 **C++ 的擴充**：當時 C++ 最流行；現在做也不錯，因為 GPU 程式用很多 pointer。
- 主要是 NVIDIA GPU 的語言。

---

## 2. PTX 與 SASS

[01:30] Fat binary 裡 GPU 的部分先編成**虛擬 ISA**：**PTX**。

- [01:44] 像 RISC 的指令集，假設有**無限多個暫存器**。
- 好處：有智慧財產的程式碼不想公開 → 編成中間語言，原始碼救不回來，再發佈。
- [02:20] GPU 有大有小、core 有多有少 → 要**動態重新編譯**：執行時用 **JIT**（just in time）compiler 編成 **SASS**（shader assembly），每顆 GPU 有自己的 SASS。

（本機看到的 PTX 和 SASS 見 [§14.1](#141-nvcc-的編譯流程與-ptxsass)。）

---

## 3. nvcc 的編譯流程

[02:48] CUDA 的 compiler 叫 **nvcc**（地位像 gcc），同時編 CPU 和 GPU 的程式碼，產生 **fat binary**。

```
              .cu
         ┌─────┴─────┐
     CPU 那邊        GPU 那邊
  C++ 前處理器       前處理器（換掉 macro）
   → .cpp4.ii           ↓
   cudafe++ ──────→   cicc → .ptx
   → .cudafe1.cpp       ↓
     .stub.c          ptxas → .cubin
   C++ compiler          ↓
   → .o           ←─  fatbinary
            ↓
      fat binary（CPU 程式碼 ＋ GPU 程式碼）
```

[04:22] CUDA 程式的運作方式：一個普通的 C++ 程式是入口 → 把資料傳到 GPU → 呼叫 GPU 上的函數 → 回來 → 把資料拿回來。

[05:45] **Stub function**：C++ 程式裡要呼叫 GPU 函數時，實際上呼叫的是一個 stub。它負責：設定參數、初始化 GPU、把參數送過去、告訴 GPU 要執行哪個函數……這些「水管工程」都是自動產生的函式庫程式碼。

[08:22] 執行時，runtime 決定哪些跑在 CPU、哪些跑在 GPU，還會 JIT 把 PTX 轉成 SASS，做很多 GPU 專屬的最佳化。

⚠️ [06:59] 老師說 ptxas「把 PTX 指令產生 PTX 機器碼，`.cubin` 是 GPU binary，但 ISA 是 PTX」。本機 `nvcc -arch=sm_89 -dryrun` 顯示 ptxas 產生的是 `*.sm_89.cubin`，裡面是 **SASS**（`cuobjdump -sass` 看得到 SASS 指令），PTX 則另外原封不動放進 fat binary 給 JIT 用。見 [§14.1](#141-nvcc-的編譯流程與-ptxsass)。

---

## 4. Kernel

[08:58] 在 GPU 上執行的函數叫 **kernel**。有 n 個 GPU thread，就跑 n 份 kernel：「想成一個平行的函數」。

```cuda
__global__ void vecAdd(float *A, float *B, float *C)
{
    int idx = threadIdx.x;
    C[idx] = A[idx] + B[idx];
}
```

- [09:40] **`__global__`**：告訴 CUDA compiler 這是 kernel，要在 GPU 上平行執行。不回傳東西。
- [10:47] 三個 pointer 指向 GPU 的記憶體（之後講）。
- [10:47] 最簡單的情況：n 個 thread、陣列有 n 個元素，thread 排在一條 x 軸上，編號 0 到 n−1。
- [11:51] 幾乎所有平行程式框架裡 thread 都有一個編號；但 **GPU 的 thread 編號是多維的**（x、y、z，最多三維）。
- [13:06] 每個 thread 用 `threadIdx.x` 拿到自己的編號，只做自己那一個加法。1000 個元素就跑 1000 個 thread。

---

## 5. Thread、block、grid

[13:06]

| 層 | 是什麼 | 排列 |
|---|---|---|
| **Thread** | 一份 kernel | |
| **Thread block** | 一群 thread | 1D、2D、3D |
| **Grid** | 一群 thread block | 1D、2D、3D |

[14:17] 怎麼選：

| 排列 | 適合 |
|---|---|
| 3D | 體積：例如飛機機翼 |
| 2D | 平面：例如在 2D 平面上解熱方程 |
| 1D | 陣列相加 |

[14:17] 一個 thread 的完整座標有 **6 個數**：在 block 裡的 (x, y, z) ＋ block 在 grid 裡的 (x′, y′, z′)。

[15:31] 這是 CUDA 跟傳統平行程式語言（OpenMP、MPI）很不一樣的地方：每個 thread 在 block 裡**至少有 3D 座標**，用不到的維度設成 0。

[16:39] 「GPU 很會排程、管理這麼多 thread，這就是 GPU 的力量。」

---

## 6. 2D 的 thread block

[16:39]

```cuda
__global__ void matAdd(float A[N][N], float B[N][N], float C[N][N])
{
    int idx = threadIdx.x;
    int jdx = threadIdx.y;
    C[idx][jdx] = A[idx][jdx] + B[idx][jdx];
}

int main()
{
    ...
    dim3 threadsPerBlock(N, N);
    matAdd<<<1, threadsPerBlock>>>(A, B, C);
    ...
}
```

- [19:15] 一個 thread block 裡 **thread 數有上限**。n 很大就把矩陣切成小 tile，每個 tile 給一個 block。
- [19:53] **`dim3`**：最多 3 個數。`dim3(N)` = N 個；`dim3(N, N)` = N² 個；`dim3(N, N, N)` = N³ 個。沒給的維度老師說預設 0。⚠️ 本機實測 `dim3 d(16, 16)` 的 `d.z` 是 **1**，不是 0（不然 16 × 16 × 0 = 0 個 thread），見 [§14.2](#142-本機-gpu-的限制)。
- [21:37] **`<<< , >>>`** 是特別的語法：
  - 第一個：block 在 grid 裡怎麼排（這裡 1 = 只有一個 block）。
  - 第二個：thread 在 block 裡怎麼排（這裡 N × N）。

【課本補充】課本 p.227：一個 thread block 通常不能超過 **768 或 1024** 個 thread（看 GPU）。課本的例子：N = 1024、最多 768 個 thread → 每維 16 個 thread。（本機的上限見 [§14.2](#142-本機-gpu-的限制)。）

---

## 7. 2D 的 grid

[22:45]

```cuda
__global__ void matAdd(float A[N][N], float B[N][N], float C[N][N])
{
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    int jdx = blockIdx.y * blockDim.y + threadIdx.y;
    C[idx][jdx] = A[idx][jdx] + B[idx][jdx];
}

int main()
{
    ...
    dim3 threadsPerBlock(16, 16);
    dim3 numBlocks(N / 16, N / 16);
    matAdd<<<numBlocks, threadsPerBlock>>>(A, B, C);
    ...
}
```

[25:02] 怎麼算座標：

```
x 方向：block 0 | block 1 | block 2 | …
         ←blockDim.x→
idx = blockIdx.x × blockDim.x  ＋  threadIdx.x
      └ 這個 block 的起點 ┘      └ 在 block 裡的位移 ┘
y 方向一樣
```

例：blockIdx.x = 2 → 2 × blockDim.x 是第 2 個 block 的起點，再加上 threadIdx.x。

[29:07] 總共還是 N² 個 thread，只是分成 (N/16)² 個 16 × 16 的 tile。兩層的組織：thread 在 block 裡、block 在 grid 裡。3D 也一樣，再加一個 z。

（本機跑了 4096 × 4096 的版本，見 [§14.3](#143-完整的-vector-addition-與-2d-grid)。）

---

## 8. Barrier：__syncthreads

[30:26] 這麼多 thread 怎麼同步？**Barrier**：一個集合點，**所有** thread 都到了，才一起放行。有的 thread 快、有的慢，到了 barrier 都要等。過了 barrier 的 thread 知道其他 thread 也都過了。

CUDA 的函數：**`__syncthreads()`**。

[31:26] 重要的設計：**thread block 之間完全獨立**。

- 整個計算是一個 grid，切成 thread block，每個 block 跟其他 block **互不相干**。
- CUDA 大部分的共享、同步功能都只在**一個 block 裡**有效（也有少數整個 grid 的）。
- block 裡要多細的同步都可以。
- 這對硬體怎麼排程 thread 有很大的影響（[第 20 支](lecture20_note.md)）。

[32:39] 【口頭】題外話：老師用 Camtasia Studio 錄影，最後輸出影片時用了 NVIDIA GPU，筆電整台變很燙、GPU 使用率 80–90%；不用 GPU 的話要好幾個小時。

（本機試了「拿掉 `__syncthreads`」會怎樣，見 [§14.5](#145-barrier-與-cuda-graph)。）

---

## 9. 記憶體的種類

[32:39]

| 修飾字 | 在哪 |
|---|---|
| **`__device__`** | GPU 的記憶體（device memory） |
| **`__constant__`** | GPU 的 constant cache |
| **`__shared__`** | **shared memory**：每個 thread block 一份，只有同一個 block 的 thread 共用 |
| **`__managed__`** | CPU 和 GPU **共用**：GPU 的 pointer 可以在 C 程式裡存取 CPU 記憶體，反過來也行（只要在 managed 空間裡） |

前三個都在 GPU 裡。

[34:30] 一般的計算流程：資料 CPU → GPU，在 GPU 上算，結果回來。

【課本補充】課本 p.229：還有 texture memory（放紋理的細節，給圖形用）。

---

## 10. 完整的 vector addition

[35:04]

```cuda
int main()
{
    float *a, *b, *c;                 /* CPU 上 */
    a = (float *)malloc(size); ...    /* 初始化 */

    float *ga, *gb, *gc;              /* GPU 上：在 CPU 上沒有用，但可以拿來複製 */
    cudaMalloc(&ga, size); cudaMalloc(&gb, size); cudaMalloc(&gc, size);

    cudaMemcpy(ga, a, size, cudaMemcpyHostToDevice);   /* host = CPU，device = GPU */
    cudaMemcpy(gb, b, size, cudaMemcpyHostToDevice);

    vecAdd<<<1, N>>>(ga, gb, gc);     /* 這次傳的是 GPU pointer（之前「作弊」了） */

    cudaMemcpy(c, gc, size, cudaMemcpyDeviceToHost);

    cudaFree(ga); cudaFree(gb); cudaFree(gc);
}
```

[37:37] `cudaMemcpy` 參數順序：目的、來源、大小、方向。Kernel launch 就是 stub function 指的那個地方。

[38:43] 三個階段：**資料傳到 GPU → 在 GPU 上算 → 結果傳回 CPU**。傳資料的時間「某種意義上是浪費」：沒有在算。

（本機量到傳資料比計算慢很多，見 [§14.3](#143-完整的-vector-addition-與-2d-grid)。）

---

## 11. Streams

[39:04] 解法：不要一次傳完，切成小塊，**像 pipeline 一樣**：算這一塊時同時傳下一塊。

[39:54] **Stream**：一串工作的 queue。

- **同一個 stream** 裡的操作照**程式順序**執行。
- **不同 stream** 之間可以重疊，沒有同步。

[40:30] 例子：

```
原本：  [傳進去][   算   ][傳出來]                     = 3 個單位
兩個 stream：
  stream 1：[傳一半][算一半][傳出來]
  stream 2：       [傳一半][算一半][傳出來]           = 2 個單位
```

[41:07] 只是把每件事切成兩半，重疊之後從 3 個單位變 2 個單位。

[42:26] 程式（老師說「程式碼來自 NVIDIA 的官方文件，不是我的」）：

```cuda
cudaStream_t stream[2];
for (int i = 0; i < 2; ++i) cudaStreamCreate(&stream[i]);
float *hostPtr;
cudaMallocHost(&hostPtr, 2 * size);
for (int i = 0; i < 2; ++i) {
    cudaMemcpyAsync(inputDevPtr + i * size, hostPtr + i * size, size, cudaMemcpyHostToDevice, stream[i]);
    MyKernel<<<100, 512, 0, stream[i]>>>(outputDevPtr + i * size, inputDevPtr + i * size, size);
    cudaMemcpyAsync(hostPtr + i * size, outputDevPtr + i * size, size, cudaMemcpyDeviceToHost, stream[i]);
}
```

- [42:26] NVIDIA 的命名：`cuda` 小寫開頭，後面每個字大寫，`_t` 表示型別。
- [43:42] **`cudaMemcpyAsync`**：非同步的複製，不綁住其他動作，這給了 stream 獨立性。
- [44:56] 三個操作（複製進去、kernel、複製出來）都**明確指定** stream：kernel 是 `<<<…, 0, stream[i]>>>` 的第四個參數。

（本機試了 1、2、4、8 個 stream，見 [§14.4](#144-streams)。）

---

## 12. CUDA graph

[46:03] 發現：**launch kernel 和傳資料的成本很高**。stream 有幫助，但不夠。

[47:10] 如果有很多**異質**的工作，用 **CUDA graph**：

| | Stream | Graph |
|---|---|---|
| 結構 | 像 linked list | **DAG**（有向無環圖） |
| 順序 | 程式順序 | **工作的相依順序** |

[47:43] 有一個自動的引擎：驗證 graph、預先算好排程、把工作排到 GPU 上；A → B 表示 A 先做、B 後做。

[48:23] 它還更聰明：

- 加入**預取**的程式碼：前一個工作在跑時，先把下一個工作的資料抓來。
- 分配資源：特別是同時有好幾個 graph、好幾個應用在跑時。

[49:37] Node 的種類：

| Node | 說明 |
|---|---|
| Kernel | GPU 函數 |
| CPU 函數 | |
| 等待／記錄 **event** | 像計時、效能計數器 |
| **Semaphore** 的 signal | 比 lock 更一般的訊號機制（lock 是 binary semaphore；也有 counting semaphore） |
| 另一個**子 graph** | 階層式、遞迴的結構 |

[50:40] 函數：`cudaGraphCreate`、`cudaGraphAddKernelNode`（其他種類的 node 也類似）、加相依……

[51:50] **Stream capture**：把一個現有的 stream 變成 graph，得到 graph 的好處。

[51:50] Graph 是「重量級」的東西（加了預取、驗證等等），適合計算量大、而且**異質、不規則**的情況。

（本機用 stream capture 試了 1000 個小 kernel，見 [§14.5](#145-barrier-與-cuda-graph)。）

---

## 13. Events

[51:50] CUDA 程式要 profile。在程式裡插 **event**，量兩個 event 之間的時間：

```cuda
cudaEvent_t start, stop;
cudaEventCreate(&start); cudaEventCreate(&stop);
cudaEventRecord(start, stream);
... 計算 ...
cudaEventRecord(stop, stream);
cudaEventSynchronize(stop);           /* 等到 stop 相關的工作都做完 */
float eTime;
cudaEventElapsedTime(&eTime, start, stop);
```

[53:00] **`cudaEventSynchronize`**：處理 event 本身是好幾步的動作，要等「記錄 stop 在 stream 的哪一點」這件事做完，時間才準。強烈建議呼叫。

[54:10] 這些 profiling 資訊讓程式設計師調整程式。

[54:10] 「2021 年最新的是 CUDA 11，歡迎觀眾去看。」

【課本補充】課本 p.231–233 §6.3.4 也講 streams、graphs、events，Figure 6.11 是一個有子 graph 的例子。

---

## 14. 【實測】

程式在 `experiments/lecture19/`，用 CUDA 13.0 的 `nvcc -arch=sm_89` 編譯（RTX 4060 Laptop 是 compute capability 8.9）。

### 14.1 nvcc 的編譯流程與 PTX／SASS

`nvcc -arch=sm_89 -dryrun -o tiny tiny.cu`（`tiny.cu` 只有一個 `vecAdd` kernel 和空的 `main`，`-dryrun` 只印不執行，所以沒有留下檔案） 會列出 nvcc 實際呼叫的每個工具（只印、不執行）。整理每一步產生的檔案：

```
gcc        -> tiny.cpp4.ii                       （C++ 前處理）
cudafe++   -> tiny.cudafe1.cpp、tiny.cudafe1.stub.c、tiny.module_id
gcc        -> tiny.cpp1.ii                       （GPU 那邊的前處理）
cicc       -> tiny.ptx
ptxas      -> tiny.sm_89.cubin
fatbinary
gcc        -> tiny.o
nvlink     -> tiny_dlink.sm_89.cubin
fatbinary
gcc        -> tiny_dlink.o
g++        -> tiny                               （連結成執行檔）
```

跟影片 [03:30] 的圖一樣：`.cpp4.ii` → `cudafe++`（產生 **stub**：`.stub.c`）、`cicc` → `.ptx` → `ptxas` → `.cubin` → `fatbinary`。

`cuobjdump --list-elf --list-ptx vecadd` 看 fat binary 裡有什麼：

```
ELF file    1: vecadd.1.sm_89.cubin
ELF file    2: vecadd.2.sm_89.cubin
PTX file    1: vecadd.1.sm_89.ptx
```

fat binary 裡**同時有 SASS（cubin）和 PTX**。`vecAdd` kernel 的 PTX（`nvcc -ptx`）和 SASS（`cuobjdump -sass`），兩者並排：

| PTX（虛擬 ISA） | SASS（RTX 4060 真的跑的） |
|---|---|
| `.reg .f32 %f<4>; .reg .b32 %r<6>; .reg .b64 %rd<11>;` | （用實體暫存器 R0–R9） |
| `mov.u32 %r3, %ctaid.x;` `mov.u32 %r4, %ntid.x;` `mov.u32 %r5, %tid.x;` | `S2R R6, SR_CTAID.X` `S2R R3, SR_TID.X` |
| `mad.lo.s32 %r1, %r3, %r4, %r5;` | `IMAD R6, R6, c[0x0][0x0], R3` |
| `setp.ge.s32 %p1, %r1, %r2;` `@%p1 bra $L__BB0_2;` | `ISETP.GE.AND P0, PT, R6, c[0x0][0x178], PT` `@P0 EXIT` |
| `ld.global.f32 %f1, [%rd8];` `ld.global.f32 %f2, [%rd6];` | `LDG.E R4, [R4.64]` `LDG.E R3, [R2.64]` |
| `add.f32 %f3, %f2, %f1;` | `FADD R9, R4, R3` |
| `st.global.f32 [%rd10], %f3;` | `STG.E [R6.64], R9` |

- PTX 用 `%r1`、`%rd10` 這種**虛擬暫存器**，每個值一個新名字（[§2](#2-ptx-與-sass) 說的「無限多個暫存器」）；SASS 只用了 R0–R9。
- `blockIdx.x * blockDim.x + threadIdx.x` 在兩邊都是一條乘加（`mad`／`IMAD`）。`blockDim.x` 在 SASS 裡是從 constant memory（`c[0x0][0x0]`）讀的，`n` 也是（`c[0x0][0x178]`）。
- `if (idx < n)` 變成 **predicate**：`@%p1 bra`／`@P0 EXIT`。這就是 [第 17 支](lecture17_note.md#12-predicated-execution) 的 predicated execution。

### 14.2 本機 GPU 的限制

`props.cu`（`cudaGetDeviceProperties`）：

```
NVIDIA GeForce RTX 4060 Laptop GPU，compute capability 8.9
SM 數：24；每個 SM 最多 1536 個 thread；warp 大小：32
每個 block 最多 1024 個 thread；block 的維度上限 (1024, 1024, 64)；grid 的維度上限 (2147483647, 65535, 65535)
每個 block 的 shared memory：48 KiB；每個 SM 的暫存器：65536 個（32-bit）；每個 block 最多 65536 個暫存器
L2：32768 KiB；顯示記憶體：7805 MiB，bus 128 bit；GPU 時脈 2370 MHz；copy engine 數：2
dim3 d(16, 16)：d.x = 16、d.y = 16、d.z = 1
```

- 每個 block 最多 **1024** 個 thread：跟課本 p.227 的「768 或 1024」一致。所以 [§6](#6-2d-的-thread-block) 的單一 block `dim3(N, N)` 只能到 N = 32。
- `dim3` 沒給的維度是 **1**（[§6](#6-2d-的-thread-block) 的 ⚠️）。
- SM、warp 是下一講的內容（[第 20 支](lecture20_note.md)）。
- **2 個 copy engine**：可以同時「複製進去」和「複製出來」，[§14.4](#144-streams) 會用到。

### 14.3 完整的 vector addition 與 2D grid

`vecadd.cu` 照 [§10](#10-完整的-vector-addition) 的流程，1600 萬個 float（64 MiB），用 event 量每一步：

```
  cudaMalloc × 3                 0.111 ms
  複製 a、b 到 GPU（128 MiB）   11.159 ms
  kernel（1600 萬個加法）    0.801 ms
  複製 c 回 CPU（64 MiB）   20.053 ms
  合計 32.125 ms；錯的元素：0
matAdd（4096 × 4096，256 × 256 個 block，每個 16 × 16）錯的元素：0
```

- **kernel 只佔 2.5%**，97% 的時間在搬資料。這就是 [38:43] 說的「傳資料的時間是浪費」。
- kernel：讀 128 MiB、寫 64 MiB，0.8 ms → 約 **250 GB/s**，就是顯示記憶體的頻寬。這個 kernel 每個元素只做一個加法，速度完全由記憶體決定。
- 複製回 CPU 比複製進去慢（約 3.3 GB/s 對 12 GB/s）：host 記憶體是一般的 `malloc`（pageable），驅動程式要多搬一手。用 pinned 記憶體會好很多（[§14.4](#144-streams) 的單獨複製約 12.7 GB/s）。
- 2D grid 的 `matAdd` 用 [§7](#7-2d-的-grid) 的 `blockIdx * blockDim + threadIdx` 算座標，4096 × 4096 全對。

### 14.4 Streams

`streams.cu`：128 MiB 的資料切成 S 份，每份「`cudaMemcpyAsync` 進去 → kernel → `cudaMemcpyAsync` 出來」放在自己的 stream。host 用 `cudaMallocHost`（pinned）。跑兩次：

| stream 數 | 第一次 | 第二次 |
|---|---|---|
| 1 | 22.11 ms | 21.96 ms |
| 2 | 16.89 ms | 16.98 ms |
| 4 | 14.52 ms | 14.51 ms |
| 8 | 13.24 ms | 13.25 ms |

單獨量：複製進去 10.5–10.7 ms、kernel 1.23 ms、複製出來 10.2 ms（加起來約 22 ms）。

- 1 個 stream：三步一個接一個，22 ms = 三者相加。
- **2 個 stream：17 ms**；越多越好，8 個時 13.2 ms，少了 40%。
- 為什麼能快？本機有 **2 個 copy engine**（[§14.2](#142-本機-gpu-的限制)）：一個 stream 在「複製出來」的同時，另一個 stream 可以「複製進去」。最好的情況大約是 max(進去, 出來) ＋ 一點點 ≈ 11 ms，切越細越接近。
- 跟影片 [41:07]「3 個單位變 2 個單位」的想法一樣；這裡 kernel 很短，所以重疊主要來自兩個方向的複製。

### 14.5 Barrier 與 CUDA graph

`graph.cu` 第一部分：每個 block 用 shared memory 把 256 個 1.0 加起來（樹狀相加，每一輪後面 `__syncthreads()`）；對照組把 `__syncthreads()` 全部拿掉：

```
有 __syncthreads：4096 個 block 裡，總和不是 256 的有 0 個
沒有 __syncthreads：4096 個 block 裡，總和不是 256 的有 4096 個
```

- 沒有 barrier，有的 thread 會在別的 thread 還沒把值寫進 shared memory 之前就去讀 → **每一個 block 都算錯**。這就是 [§8](#8-barrier__syncthreads) 說的：barrier 保證「大家都到了」才往下走。

第二部分：1000 個很小的 kernel（每個只讓 32 個 thread 各加 1）：

```
一個一個 launch 1000 個 kernel：1.304 ms（每個 1.30 µs）       第二次：1.415 ms（1.42 µs）
capture 成 graph（1000 個 node）再 launch：0.712 ms（每個 0.71 µs）  第二次：0.712 ms
```

- 用 **stream capture**（[§12](#12-cuda-graph)）把 1000 次 launch 變成一個 1000 個 node 的 graph，再一次 launch，每個 kernel 的額外負擔從約 **1.3–1.4 µs 降到 0.7 µs**，快了將近一倍。
- 這就是 [46:03]「launch kernel 的成本很高」：這些 kernel 本身幾乎不花時間，量到的幾乎全是 launch 的負擔；graph 事先驗證、排好，省掉了每次 launch 的工作。

---

## 15. 一頁速查表

| 主題 | 重點 |
|---|---|
| CUDA | NVIDIA，2006–2007；C++ 的擴充 |
| PTX／SASS | PTX：虛擬 ISA、無限暫存器、保護原始碼；執行時 JIT 成每顆 GPU 的 SASS |
| nvcc | CPU：cpp4.ii → cudafe++（stub）→ .o；GPU：cicc → .ptx → ptxas → .cubin；fatbinary 合起來 |
| Stub function | 設定參數、初始化 GPU、launch kernel 的自動產生的程式碼 |
| Kernel | `__global__`；n 個 thread 跑 n 份 |
| 階層 | thread → block → grid；各 1D/2D/3D；座標 6 個數 |
| Launch | `kernel<<<numBlocks, threadsPerBlock[, shmem, stream]>>>(…)`；`dim3` |
| 座標 | `idx = blockIdx.x * blockDim.x + threadIdx.x` |
| Block | 上限 768／1024 個 thread；block 之間獨立；同步、共享大多在 block 內 |
| Barrier | `__syncthreads()` |
| 記憶體 | `__device__`、`__constant__`、`__shared__`（每個 block 一份）、`__managed__`（CPU、GPU 共用） |
| 流程 | malloc → cudaMalloc → cudaMemcpy H2D → kernel → cudaMemcpy D2H → cudaFree |
| Stream | 同一個 stream 照順序、不同 stream 可重疊；`cudaMemcpyAsync`、pinned 記憶體 |
| Graph | DAG；預先驗證、排程、預取；node：kernel、CPU 函數、event、semaphore、子 graph；stream capture |
| Event | `cudaEventRecord` → `cudaEventSynchronize` → `cudaEventElapsedTime` |
| 實測 | kernel 只佔 2.5%、搬資料 97%；8 個 stream 22 → 13 ms；沒有 barrier 全錯；graph 讓每個 launch 1.3 → 0.7 µs |

---

## 16. 與後續講次的連結

| 本講觀念 | 在哪裡 |
|---|---|
| GPU 的動機、PTX／SASS 的由來 | [第 18 支](lecture18_note.md) |
| Predicated execution | [第 17 支](lecture17_note.md) |
| SM、warp、thread block 怎麼排到硬體上、shared memory 和 register file 的硬體 | [第 20 支](lecture20_note.md) |
| Barrier、lock、semaphore、平行程式 | [第 32 支](lecture32_note.md)、[第 37 支](lecture37_note.md)（Ch9） |
| 記憶體頻寬、roofline | [第 45 支](lecture45_note.md)（Ch10） |

---

## 附：本講一句話總結

> **CUDA 是 NVIDIA 在 C++ 上加的擴充：nvcc 把 .cu 拆成 CPU 部分（cudafe++ 產生 stub function）和 GPU 部分（cicc 編成假設有無限暫存器的虛擬 ISA PTX、ptxas 編成 cubin），合成 fat binary，執行時還能把 PTX JIT 成那顆 GPU 的 SASS；程式設計師寫 `__global__` kernel，用 `<<<grid, block>>>` 一次叫起很多 thread，thread 排在 1D／2D／3D 的 block 裡、block 排在 grid 裡，用 `blockIdx * blockDim + threadIdx` 算出自己負責哪一格，block 之間互相獨立、block 內用 `__syncthreads()` 當 barrier、用 `__shared__` 記憶體共享；典型流程是 cudaMalloc、複製進去、跑 kernel、複製出來，而因為搬資料很花時間，可以用 stream 讓複製和計算重疊，用 graph（DAG，可以從 stream capture 得到）降低大量 launch 的成本，用 event 量時間。本機在 RTX 4060 上實測：一個 vector addition 裡 kernel 只佔 2.5%、97% 在搬資料；切成 8 個 stream 讓總時間從 22 ms 降到 13 ms；拿掉 `__syncthreads()` 每個 block 都算錯；把 1000 個小 kernel capture 成 graph，每次 launch 的負擔從 1.3 µs 降到 0.7 µs。**

---

*筆記依據 Smruti R. Sarangi「Advanced Computer Architecture」Chapter 6: GPUs (Part II) -- CUDA Programming Language 的 whisper 轉錄稿（本機 `large-v3-turbo`，55:31）、Sarangi *Next-Gen Computer Architecture* v3.2 Ch6 §6.3（p.225–233）整理。實測為本機（Intel i7-13650HX、NVIDIA RTX 4060 Laptop GPU、driver 580.178.04、CUDA 13.0、Ubuntu、kernel 6.5.0）結果。*
