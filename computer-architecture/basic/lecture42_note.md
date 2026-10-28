# Basic Computer Architecture 第四十二支影片完整筆記：GPU 的結構與 CUDA、interconnection network 與各種拓樸（Chapter 11 GPUs and Interconnects, Part 5/5 → 課本 Ch12）

> **課程**：Basic Computer Architecture — Chapt 11: GPUs and Interconnects, Part 5/5（播放清單第 42 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：YouTube 錄影逐字稿（1:25:53，自動字幕）＋ 課本 *Basic Computer Architecture* v3.09 **Ch12** §12.6（p.620–630）
> **章號**：影片的 Chapter 11 是課本 v3.09 的 **Chapter 12**；影片說的「下一章」（bridge、bus、I/O）是課本 Ch13。
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄（本講沒有）
> - `【實測】`：在本機（Intel i7-13650HX、NVIDIA GeForce RTX 4060 Laptop GPU、CUDA 12.4、Ubuntu、Python 3.10）實際執行的結果
> - `⚠️`：逐字稿缺字、或影片／課本／實測說法不一致之處
> - `[mm:ss]`：影片時間點
>
> ⚠️ 前半段的 **GPU**（[00:00]–[41:11]）在課本 v3.09 裡**沒有**對應章節，只有影片的內容；後半段的 interconnect 對應課本 §12.6。

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. NVIDIA Tesla（GeForce 8800）的整體結構](#1-nvidia-teslageforce-8800的整體結構)
- [2. TPC 與 SM](#2-tpc-與-sm)
- [3. SM 裡面：SP、SFU、cache](#3-sm-裡面spsfucache)
- [4. Warp 與 SIMT](#4-warp-與-simt)
- [5. 一個 SM 怎麼排 warp](#5-一個-sm-怎麼排-warp)
- [6. GPU 的產出率](#6-gpu-的產出率)
- [7. CUDA 程式：向量加法](#7-cuda-程式向量加法)
- [8. Interconnect：multicore 晶片的樣子](#8-interconnectmulticore-晶片的樣子)
- [9. Router、tile、NoC](#9-routertilenoc)
- [10. 兩個指標：bisection bandwidth 與 diameter](#10-兩個指標bisection-bandwidth-與-diameter)
- [11. Chain 與 ring](#11-chain-與-ring)
- [12. Fat tree](#12-fat-tree)
- [13. Mesh](#13-mesh)
- [14. Torus 與 folded torus](#14-torus-與-folded-torus)
- [15. Hypercube](#15-hypercube)
- [16. Butterfly](#16-butterfly)
- [17. 拓樸總比較](#17-拓樸總比較)
- [18. 【實測】](#18-實測)
- [19. 一頁速查表](#19-一頁速查表)
- [20. 與後續講次的連結](#20-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

```
GPU（NVIDIA Tesla，GeForce 8800）：
  8 個 TPC × 2 個 SM × (8 個 SP + 2 個 SFU) = 160 個核心
  32 個 thread 一組叫 warp，同一個 warp 執行完全一樣的指令（SIMT）
  每個 SM 每 4 個週期處理一個 warp 的一條指令 → 每週期 8 條 → 整顆 128 條
  CUDA：cudaMalloc → cudaMemcpy → kernel<<<blocks, threads>>> → cudaMemcpy 回來
interconnect：
  bus 只能接 4–8 個；更大的要用網路（NoC）
  指標：bisection bandwidth（越大越好）、diameter（越小越好）
  chain、ring、fat tree、mesh、torus、folded torus、hypercube、butterfly
```

---

## 1. NVIDIA Tesla（GeForce 8800）的整體結構

[00:00]–[07:40]

【口頭】這個版本「大約 5 年前」的了，NVIDIA 之後出了很多新的 GPU，Intel 也有；但基本概念大致相同，設計本身會變。

```
host CPU ─(bridge／bus)─→ host interface
                              ├─ vertex、rasterization、fragment 等傳統繪圖單元  ┐
                              └─ compute work distribution（GPGPU 的工作）        ┘ 兩種工作的來源
                                           ↓
                       8 個 TPC（texture processor cluster），每個 2 個 SM
                                           ↓
                              interconnection network
                                           ↓
                    6 組：ROP ＋ L2 cache ＋ DRAM 模組
```

- **bridge**（橋接器）：讓 GPU 和 CPU、GPU 和記憶體、記憶體和 CPU 溝通的裝置，下一章（課本 Ch13）再講。
- 線的上面是不同的**工作來源**：傳統繪圖工作（上一講的 vertex → rasterization → fragment → frame buffer）或一般的運算工作；線的下面是比較通用（不是 100% 通用）的運算部分，兩種工作**共用**。
- **compute work distribution** 單元：分派一般運算（線性代數、矩陣乘法、反矩陣……）。架構師發現本來用來處理繪圖資料的大量電路，也可以拿來做一般運算 → GPGPU。
- 6 個 DRAM 模組，每個前面有一個 **L2 cache** 和一個 **ROP**（raster operations processor，光柵運算處理器）。位址空間分給 6 個 L2。這些 DRAM 是 GPU 自己的，和系統的主記憶體（也是 DRAM）分開。

---

## 2. TPC 與 SM

[04:33]–[09:18]

- **TPC**（texture processor cluster，紋理處理器叢集）：有處理紋理（texture）的單元，也有一般的處理器。這顆 GPU 有 **8 個**，別的 GPU 可以更多。
- 每個 TPC 有 **2 個 SM**。
- **SM**（streaming multiprocessor，串流多處理器）：資料像河水一樣從上面（host CPU 或記憶體）**串流**下來，SM 處理它們。每個 SM 裡有好幾個處理單元。
- TPC 裡還有 **geometry controller**（繪圖用：把運算和基本形狀轉成 multithreaded 程式碼）和 **SMC**（SM controller，把指令排到兩個 SM 上）。
- 【口頭】[08:13] GPU 不是收很低階的指令，而是比較高階的指令，再轉成 **PTX** 指令集（或其他內部表示）的低階指令。

---

## 3. SM 裡面：SP、SFU、cache

[09:18]–[15:04]

| 元件 | 做什麼 |
|---|---|
| **I-cache** | 指令 cache，每個 SM 一個。因為同一條指令會在整個 SM 的所有 SP、SFU 上執行（類似 vector），所以非常有效率 |
| **C-cache** | constant cache：演算法常用的常數（e、π……） |
| **MT issue** | multithreaded issue 單元，協調所有單元 |
| **8 個 SP** | streaming processor：**非常簡單**的核心，「比我們在 chapter 9 設計的 in-order 處理器簡單得多」，大概只有 1、2 級 pipeline。從 I-cache 拿指令、簡單解碼、執行，資料進出 shared memory 或 DRAM。只做加減乘除 |
| **2 個 SFU** | special function unit：比 SP 強，能算超越函數（三角函數）、log 等；一個 SFU 一個週期可以收 4 條指令 |
| **shared memory** | 給 SM 裡所有運算單元互相溝通 |
| **texture unit** | 處理紋理，有自己的 texture L1 cache |

⚠️ [12:57] 老師說 SFU「一個週期收 4 條……不對，2 個週期 8 條」，口誤後改正。

**數一數核心**（【口頭】[13:42]）：每個 SM 10 個（8 SP + 2 SFU）→ 每個 TPC 20 個 → 8 個 TPC 共 **160 個核心**。「就算是這顆比較舊、也滿簡單的 GPU，也有大約 160 個核心。」【口頭】[14:39] 老師先說「可以同時做 160 個加法」，又收回：「算法有點不一樣，下一張投影片再算。」

**串流的好處**（【口頭】[12:26]）：大多數資料只看**一次**：看一筆、處理、再看下一筆，通常不會重複看同一筆。

---

## 4. Warp 與 SIMT

[15:04]–[18:17]

- GPU 把 **32 個 thread** 分成一組，叫 **warp**（32 不是固定的，也可以 16、64，2 的次方）。
- 同一個 warp 的 thread 執行**完全一樣**的動態指令序列，只是資料不同 → 本質是 SIMD，但因為是跨 thread 的，叫 **SIMT**（single instruction multiple threads）。
- 「和 vector 處理器在概念上非常像，只是不是很多個功能單元，而是很多個 thread。」
- 和 vector 處理器一樣用 **predicated branch**：每個 thread 沒有自己的分支，thread 之間永遠不會**不同步**。
- 一個 warp 被分給一個 SM。
- warp 裡每條指令是 **atomic** 的：對外部觀察者來說，要嘛全部執行、要嘛完全沒執行。SM 裡所有單元先把 warp 裡**每個 thread 的第 i 條**指令做完，才考慮第 i + 1 條或別的 warp 的指令。

---

## 5. 一個 SM 怎麼排 warp

[18:17]–[19:58]

一個 SM 上可以同時有好幾個 warp。SM 的 multithreaded instruction scheduler 每次挑一個 warp、讓它的 32 個 thread 都前進一條指令：

```
warp 8 的指令 11
warp 1 的指令 42
warp 3 的指令 95
……（別的 warp）
warp 3 的指令 96    ← 接著上次的 95
warp 8 的指令 12
warp 1 的指令 43
```

（這就是上一講的 fine-grained multithreading，換的單位是 warp。）

---

## 6. GPU 的產出率

[19:58]–[25:43]

在 GPU 裡「一條指令」的意思是：一個 warp 的 32 個 thread 各執行一次同一條指令。每個 SM **每 4 個週期**完成一條這樣的指令：

```
8 個 SP：每 2 個週期排 8 個 thread（每個 SP 一個）→ 4 個週期 16 個 thread
2 個 SFU：每個每週期收 4 條 → 每 2 個週期排 8 個 thread → 4 個週期 16 個 thread
16 + 16 = 32 = 一個 warp
```

- 從**程式**的角度：每 4 個週期 1 條；從**總指令數**的角度：每 4 個週期 32 條 = **每週期 8 條／SM**。
- 8 個 TPC × 2 = **16 個 SM** → 整顆 GPU 每週期最多 8 × 16 = **128 條**指令。「IPC 是 128，真的非常非常高。」一般的 multicore 處理器沒有這麼高的產出率。
- 這些都是很簡單的指令（加、減、乘），但能處理大量資料、算出非常複雜的畫面。
- 所以 GPU 非常耗電；但高畫質遊戲、看電影、做電影就該用 GPU。

**記憶體**（【口頭】[22:45]）：
- 同一個 warp 的 thread 可以透過**該 SM 的 shared memory** 共用資料。
- 好幾個 warp 組成一個 **grid**；grid 裡不同的 warp 獨立執行，彼此**不透過** shared memory，而是透過 **global memory**（GPU 上或晶片外的 DRAM）溝通。

---

## 7. CUDA 程式：向量加法

[25:43]–[41:11]

【口頭】「這只是冰山一角。」GPU 有好幾種語言，最常見的是 **CUDA**（老師說是 common unified device architecture），主要用在 NVIDIA GPU；AMD 用 **OpenCL**（類似 CUDA，但有不同）。

- CUDA 是 C++ 的擴充：**CUDA 編譯器**另外編 GPU 的程式碼，一般的編譯器編 CPU 的程式碼，兩者一起工作。
- **kernel**：在 GPU 上**平行執行**的一段程式碼。
- **block**（也叫 **CTA**，cooperative thread array）：程式語言裡類似 warp 的東西，同一個 block 的 thread 一起執行。
- 好幾個 block 組成一個 **grid**。

**主程式（CPU 這邊）**（【口頭】[28:06]）：

```c
int a[N], b[N], c[N];                   /* N = 1024，「可以改成一百萬，才看得出好處」 */
int *gpu_a, *gpu_b, *gpu_c;
cudaMalloc((void **)&gpu_a, size);      /* 在 GPU 的記憶體裡配空間 */
cudaMalloc((void **)&gpu_b, size);
cudaMalloc((void **)&gpu_c, size);
/* 初始化 a、b */
cudaMemcpy(gpu_a, a, size, cudaMemcpyHostToDevice);   /* host（CPU）→ device（GPU） */
cudaMemcpy(gpu_b, b, size, cudaMemcpyHostToDevice);
vectorAdd<<<N/32, 32>>>(gpu_a, gpu_b, gpu_c);         /* N/32 個 block，每個 32 個 thread */
cudaMemcpy(c, gpu_c, size, cudaMemcpyDeviceToHost);   /* GPU → CPU */
cudaFree(gpu_a); cudaFree(gpu_b); cudaFree(gpu_c);
```

- `size` = N × sizeof(int)（4 或 8 bytes，看系統）。
- `<<< >>>` 裡面是 block 數和每個 block 的 thread 數：等於建一個 N/32 列 × 32 欄的 thread 矩陣，**每個 thread 加一對元素**。「這不是 C 的語法，所以要先給 CUDA 編譯器處理。」
- 等於 `for all i: c[i] = a[i] + b[i]`。

**kernel（GPU 這邊）**（【口頭】[34:56]）：

```c
__global__ void vectorAdd(int *gpu_a, int *gpu_b, int *gpu_c)
{
    int idx = threadIdx.x + blockIdx.x * blockDim.x;   /* 這個 thread 負責第幾個元素 */
    gpu_c[idx] = gpu_a[idx] + gpu_b[idx];
}
```

- `vectorAdd` 是**自己寫的**，不是 CUDA 內建；`__global__` 表示這是 GPU kernel、不回傳值。
- `threadIdx`、`blockIdx`、`blockDim` 是 CUDA 內建的：全域 id = block id × block 大小 + block 裡的 thread id。
- 【口頭】[38:03] 和這章所有平行程式（OpenMP、MPI 風格、SC 機器、WC 機器）一樣：開很多 thread，每個 thread 有 id，依 id 決定處理哪些元素。

【口頭】[40:41]「要寫出高效能的實作相當困難」，認真學 GPU 的人還有很多要學。

---

## 8. Interconnect：multicore 晶片的樣子

[41:11]–[43:50]　課本 §12.6.1、Fig 12.20（p.620–621）

我們一直說 multicore 的核心「用某種方式」接在一起（GPU 的 TPC 和記憶體模組也是）；一群晶片也可以接成 **cluster**（伺服器叢集）。這裡看各種**互連網路**（interconnection network）。

典型的 multicore 晶片（例子有 32 個核心）：
- **核心**（藍色）
- **cache bank**（大塊 SRAM，各存位址空間的一部分），散布在整顆晶片上
- **router**（路由元件）：把一小群核心和 cache bank 接到網路上
- **memory controller**（紅色，1、2 或 8 個）：和晶片外的 DRAM 模組溝通；DRAM 模組多、請求多，所以可以有好幾個

【口頭】[42:51] 像一座城市：一個小社區接到一條大馬路，其他社區也接到這條路。

---

## 9. Router、tile、NoC

[43:50]–[46:26]　課本 Definition 136（p.621）

- **NoC**（network on chip，晶片上的網路）：晶片上的網路。
- **tile**（磚）：一個 router 服務的那一小群核心和 cache bank；它們和網路之間只有一個連接，彼此之間可以用很簡單的方式互連。
- router 收發自己 tile 的所有訊息，也**轉送**別的 router 的訊息到目的地。
- NoC 的術語：router 叫 **node**（節點），相鄰的 node 之間用 **link**（連結，就是銅線）連起來。

**bus 不夠用**（【口頭】[45:55]）：bus 同一時間只能有一個人寫，不然訊號會互相干擾、兩則訊息都毀了 → 「最多 4 或 8 個處理器，超過就不行了」。

---

## 10. 兩個指標：bisection bandwidth 與 diameter

[46:26]–[50:14]　課本 §12.6.2、Definition 137–138（p.622–623）

**Bisection bandwidth**（二分頻寬）：要把網路切成**差不多相等**的兩半，**最少**要剪斷幾條 link。（節點數是奇數時，例如 17 個分成 8 和 9，這種小差別忽略。）

重要的原因：
1. **可靠度**：link 壞掉會怎樣。
2. **頻寬**：學過演算法的人知道 **max-flow min-cut** 定理，它告訴你從一邊到另一邊最多能流過多少資料。

【口頭】[47:37] 兩座一樣大的城市只靠一座窄橋相連 → bisection bandwidth = 1。橋斷了兩座城市就斷了聯繫、貿易停擺；就算沒壞，所有車都擠在這座橋上，大塞車。

**Diameter**（直徑）：所有節點對之間的**最短路徑**裡，**最長**的那一條有多長。代表最壞情況的**延遲**。

**目標**：bisection bandwidth **越大越好**（不會有某條 link 塞住），diameter **越小越好**（任兩點都能很快到）。

---

## 11. Chain 與 ring

[50:14]–[52:05]　課本 Fig 12.21–12.22（p.623）

| | 長相 | bisection | diameter |
|---|---|---|---|
| **Chain**（鏈） | 一個接一個排成一列 | **1** | **N − 1** |
| **Ring**（環） | chain 的頭尾接起來 | **2** | **N/2**（可以往兩邊走） |

chain 很簡單，但兩個指標都很差，「和我們的目標完全相反」。ring 比 chain 好，**4–8 個處理器**這種中小型系統可以用。

---

## 12. Fat tree

[52:05]–[56:02]　課本 Fig 12.23（p.623–624）

- 所有資料節點（藍色）都在**樹葉**；內部節點（綠色）是只負責轉送、不產生也不消耗資料的 **switch**（交換器）。
- 從 a 到 b：先往上到兩者最近的共同祖先，再往下。
- 問題：如果流量隨機，**一半**的訊息要經過**樹根** → 樹根塞車。【口頭】[53:52] 就像城市裡是一兩線道，但連接兩座城市的橋是 8 線道。
- 解法：**越靠近樹根，link 越「胖」**（能同時傳越多請求）：葉子 1 條、上一層 2 條、樹根 4 條 → 所以叫 **fat tree**（胖樹）。
- diameter 很小：**2 log N**（往上 log N、往下 log N）。

【口頭】[54:47] fat tree 在**晶片內**不流行，比較常用在**很多台機器**的地方，例如資料中心、叢集：switch 和線路的頻寬越往樹根越大。

【課本補充】p.624：要讓樹根完全不塞車，樹根到每棵子樹要有 N/2 條 link，bisection bandwidth 就是 **N/2**；但實際上不會真的給那麼多，因為一棵子樹裡所有節點同時送訊息的機率很低。

---

## 13. Mesh

[56:02]–[58:08]　課本 Fig 12.24（p.625）

晶片內網路**最常用**的拓樸，因為**很好做**：就是一個 √N × √N 的矩陣，相鄰的 tile 直接接起來。

| | 值 |
|---|---|
| diameter | 約 **2√N**（從一角到對角，橫走 √N、直走 √N；課本：2√N − 2） |
| bisection | **√N**（從中間切一刀，要剪斷 √N 條） |

diameter 不像 fat tree 那麼小（√N 對 log N），但比 chain、ring（N）好很多；bisection 也高，有很多條路可以走。

【課本補充】p.625：角落的節點有 2 個鄰居、邊上的有 3 個、其他有 4 個。mesh **不對稱**：邊上的節點彼此距離很遠。

---

## 14. Torus 與 folded torus

[58:08]–[65:25]　課本 Fig 12.25–12.26（p.625–627）

**Torus**（環面）：把 mesh 每一列、每一行的**頭尾接起來**（2D torus）。

| | mesh | torus |
|---|---|---|
| bisection | √N | **2√N**（同一刀要剪斷原本的 √N 條，再加上 √N 條頭尾的連線） |
| diameter | 2√N | **√N**（最遠的是角落到中心：√N/2 + √N/2） |

【口頭】[59:59]「最遠的兩個點**不是**兩個角落，那是錯的答案」：從角落一步就能繞到另一邊。

diameter 減半、bisection 加倍，代價是線比較多、routing 比較複雜。「能做 torus 就該做」，晶片內用 torus 並不少見。

**問題：線的長度不平均**（【口頭】[61:37]）：一般的 link 很短，但頭尾相連的那條是 **√N 倍長**。訊息走短的 link 要 1 個週期，走長的可能要好幾個，甚至會**限制時脈週期**。

**Folded torus**（摺疊環面）：每一列（行）本身就是一個 ring；把節點**交錯**放在 ring 的兩半（左、右、左、右……；上、下、上、下……）。
- 相鄰節點的距離從 1 單位變成 **2 單位**，但**沒有超長的線了**，所有距離都一樣。
- bisection 和 diameter 和 torus **完全一樣**。
- 【口頭】[64:27]「請讀者盯著這張圖看一陣子」，直到看懂為什麼要這樣排。

【課本補充】p.626–627：定義 diameter 時隱含假設每條 link 長度差不多；因為 router 的延遲通常比 link 的傳播時間大，這假設大致合理。但 torus 的長 link 讓它在**實務上**並沒有比 mesh 縮短多少 diameter，所以需要 folded torus。

---

## 15. Hypercube

[65:25]–[72:53]　課本 Fig 12.27（p.627–628）

比較適合**晶片外**的網路（把很多處理器接在一個機房裡），但是 interconnect 研究的核心拓樸。

**遞迴定義**：

| order | 怎麼做 | 樣子 |
|---|---|---|
| H0 | 一個節點 | 點 |
| H1 | 兩個 H0，對應節點連起來 | 線段 |
| H2 | 兩個 H1，對應節點連起來 | 正方形 |
| H3 | 兩個 H2，對應節點連起來 | **立方體**（hypercube 的「cube」） |
| H4 | 兩個 H3，對應節點連起來 | 每個節點 4 個鄰居 |

**編號**：H1 的節點是 0、1。做 H2 時，一份前面加 0（00、01），另一份前面加 1（10、11）。這樣**相鄰的兩個節點編號只差 1 個 bit**（00–01、00–10……）。H3：110 和 010、100、111 都只差一個 bit。

**性質**：

| | Hn（N = 2ⁿ 個節點） |
|---|---|
| bisection | 等於 H(n−1) 的節點數 = **N/2**（例：把立方體切成兩半要剪 4 條，= H2 的 4 個節點） |
| diameter | **n = log N**：編號有 n 個 bit，每走一條 link 改一個 bit，最多改 n 個 |

diameter 只有 log N，和 fat tree 一樣，「擴充性極好」。但需要**很多線**（每個節點 log N 條），所以叫 **high-radix**（高基數）拓樸，很難做在晶片裡。

---

## 16. Butterfly

[72:53]–[79:57]　課本 Fig 12.28（p.628–630）

比 hypercube **更適合做在晶片上**，NoC 有好幾種 butterfly 的實作。

8 個節點的例子：
- 概念上是一個**圓柱**，網路會繞回自己；但圓柱很難畫，所以切開攤平。圖上兩邊的「節點 1」其實是**同一個**節點，只是畫兩次比較好看（2、7 也是）。【口頭】「我重複講了好幾次，因為這不太直覺。」
- 藍色是資料節點，綠色是只轉送的 **switch**（和 fat tree 一樣）。
- **log N 欄**，每欄 **N/2 個 switch**；每個 switch 接到下一欄**同一列**的 switch，和**另一列**的 switch（**cross link**，交叉連結）。

**Routing 例子**（【口頭】[75:49]）：
- 節點 4 → 節點 7（目的 switch 是 11）：4 先到自己的 switch 01。第一個 bit 0 ≠ 1 → 走 cross link 變成 1x；第二個 bit 相同 → 直走 → 到 11 → 節點 7。
- 節點 4 → 節點 6（目的 switch 10）：01 → 第一個 bit 不同，走 cross link → 第二個 bit 又不同，再走 cross link → 10 → 節點 6。
- 每一步讓 switch 編號和目的地**多一個 bit 相同**，所以只要 log N 步。

| | 值 |
|---|---|
| switch 數 | **N log N / 2** |
| diameter | 約 **log N**（課本：log N + 1，最後一步從 switch 到目的節點） |
| bisection | **N/2**（在中間水平切，只會切到第一、二欄之間的 cross link） |

【課本補充】p.629：在 butterfly 裡 routing 和在 hypercube 裡幾乎一樣：每一步讓來源和目的 switch 編號的相同前綴多 1 個 bit。

---

## 17. 拓樸總比較

[79:57]–[85:53]　課本 Table 12.7（p.630）

| 拓樸 | switch 數 | link 數 | diameter | bisection |
|---|---|---|---|---|
| chain | 0 | N − 1 | N − 1 | 1 |
| ring | 0 | N | N/2 | 2 |
| fat tree | N − 1 | N log N ‡ | 2 log N | N/2 † |
| mesh | 0 | 2N − 2√N | 2√N − 2 | √N |
| torus | 0 | 2N | √N | 2√N |
| folded torus | 0 | 2N | √N | 2√N |
| hypercube | 0 | N log N / 2 | log N | N/2 |
| butterfly | N log N / 2 | N + N log N | log N + 1 | N/2 |

‡ 假設每條 link 的大小等於它下面子樹的大小；† 假設每條 link 的容量等於它子樹的葉子數。

- bus 不列：它不算傳統意義的網路。
- 【口頭】「chain、ring 在光譜的一端，hypercube、butterfly 在另一端，mesh、torus、folded torus 在中間（√N 的複雜度）。」hypercube 和 butterfly 線多，所以要剪更多線才能把網路切開。

**怎麼選**（【口頭】[85:06]）：

| 網路大小 | 用 |
|---|---|
| 很小 | chain、ring |
| 中等 | mesh、torus |
| 晶片外 | fat tree（torus 也可以） |
| 很大（幾十、幾百、幾千個節點） | hypercube、butterfly：線多，但最壞的端到端距離只有 log N |

多處理器這章到此結束，下一章是儲存裝置和 I/O 系統。

---

## 18. 【實測】

程式在 `experiments/lecture42/`。

### 18.1 本機的 GPU

```
$ nvidia-smi --query-gpu=name,memory.total,clocks.max.sm --format=csv
NVIDIA GeForce RTX 4060 Laptop GPU, 8188 MiB, 3105 MHz
```

`vecadd.cu` 也印出 CUDA 回報的硬體資訊：

```
NVIDIA GeForce RTX 4060 Laptop GPU: 24 SMs, warp size 32, max 1024 threads per block
```

| | 影片的 Tesla（GeForce 8800） | 本機 RTX 4060 Laptop |
|---|---|---|
| SM 數 | 16（8 TPC × 2） | **24** |
| warp 大小 | 32 | **32**（沒變） |
| 記憶體 | 6 個 DRAM 模組 | 8 GB |

warp 還是 32 個 thread，影片的觀念到今天都適用。⚠️ 每個 SM 裡有多少運算單元，CUDA 的這個 API 沒有直接回報，這裡「待補」。

### 18.2 影片的 CUDA 向量加法

`vecadd.cu` 就是 §7 的程式（`vectorAdd<<<N/32, 32>>>`），N 改成 2²⁴ = 1600 萬個 int（每個陣列 64 MB），用 CUDA event 計時：

```
$ nvcc -O2 -o vecadd vecadd.cu && ./vecadd
N = 16777216: copy to GPU 13.26 ms | kernel 32.628 ms | copy back 19.45 ms | wrong results: 0
    32 threads per block: kernel 0.843 ms
   128 threads per block: kernel 0.832 ms
   512 threads per block: kernel 0.884 ms
```

- 1600 萬個結果**全部正確**。
- **第一次**跑 kernel 要 32.6 ms，之後只要 **0.84 ms**：第一次包含 GPU 的初始化和載入程式的成本。
- 0.84 ms 讀 128 MB、寫 64 MB，等於每秒約 **228 GB**，比 CPU 在[第 38 支](lecture38_note.md#142-課本的-openmp-程式)量到的約 34 GB/s 快好幾倍。
- 但**搬資料**才是大頭：CPU → GPU 13 ms、GPU → CPU 19 ms，加起來是 kernel 的 **近 40 倍**。像向量加法這種每個元素只算一次的工作，光搬資料就把 GPU 的好處吃掉了。GPU 適合「資料搬過去一次、算很多次」的工作（影片說的線性代數、矩陣運算）。
- block 大小（32、128、512 個 thread）差別不大，都是被記憶體頻寬限制。

### 18.3 驗證 Table 12.7

`topo.py`：建出 N = 16 的各種拓樸，用 BFS 算 diameter；bisection bandwidth 用**暴力法**：把 16 個節點平分成兩半的所有方法（6435 種）都試一次，找要剪斷的最少 link 數。

```
$ python3 -I topo.py
N = 16            diameter（實測／課本）   bisection（實測／課本）   links
chain        15 / 15                   1 / 1                 15
ring          8 / 8                    2 / 2                 16
mesh          6 / 6                    4 / 4                 24
torus         4 / 4                    8 / 8                 32
hypercube     4 / 4                    8 / 8                 32
butterfly N= 8: diameter（節點到節點） 4，課本 log N + 1 = 4；switch 12，links 32
butterfly N=16: diameter（節點到節點） 5，課本 log N + 1 = 5；switch 32，links 80
butterfly N=64: diameter（節點到節點） 7，課本 log N + 1 = 7；switch 192，links 448
```

- 每一格都和課本 Table 12.7 **完全一樣**：mesh 的 diameter 2√N − 2 = 6、bisection √N = 4；torus 的 diameter 減半成 4、bisection 加倍成 8；hypercube 的 log N = 4、N/2 = 8。
- link 數也對：mesh 2N − 2√N = 24、torus 2N = 32、hypercube N log N / 2 = 32、butterfly N + N log N（32、80、448）。
- 16 個節點時 torus 和 hypercube 的數字一樣（N = 16 時 √N = log N = 4），節點變多 hypercube 才會拉開差距。

---

## 19. 一頁速查表

| 主題 | 重點 |
|---|---|
| GPU 結構 | host interface → 繪圖單元＋compute work distribution → TPC → interconnect → L2＋DRAM |
| TPC／SM | 8 個 TPC，每個 2 個 SM |
| SM | I-cache、C-cache、8 個 SP（很簡單）、2 個 SFU（超越函數）、shared memory、texture unit |
| 核心數 | 8 × 2 × 10 = 160 |
| Warp | 32 個 thread、同一條指令、不同資料（SIMT）；predicated branch |
| 排程 | SM 每次挑一個 warp，32 個 thread 一起前進一條指令 |
| 產出率 | 每 SM 每週期 8 條，整顆 128 條（IPC 128） |
| 記憶體 | 同 warp 用 SM 的 shared memory；不同 warp 用 global memory |
| CUDA | cudaMalloc、cudaMemcpy（HostToDevice／DeviceToHost）、kernel<<<blocks, threads>>>、cudaFree |
| Kernel | `__global__`；idx = threadIdx.x + blockIdx.x × blockDim.x |
| NoC | router（node）、link、tile |
| Bus 的極限 | 同一時間只有一人能寫；4–8 個處理器 |
| Bisection | 平分網路最少剪幾條（越大越好） |
| Diameter | 最長的最短路徑（越小越好） |
| Chain／ring | 1、N−1／2、N/2 |
| Fat tree | 越近樹根 link 越胖；diameter 2 log N；適合資料中心 |
| Mesh | 最常用、好做；√N、2√N |
| Torus | 頭尾相連；2√N、√N；長線問題 → folded torus |
| Hypercube | 相鄰節點編號差 1 bit；N/2、log N；線很多 |
| Butterfly | log N 欄 × N/2 個 switch；每步多一個 bit 相同；N/2、log N |
| 實測 | RTX 4060：24 SM、warp 32；kernel 0.84 ms，但搬資料 33 ms；Table 12.7 在 N = 16 全對 |

---

## 20. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| bridge、bus、I/O、PCI Express（GPU 怎麼接到 CPU） | [第 43 支](lecture43_note.md)：I/O System（課本 Ch13） |
| link 是怎麼傳訊號的（physical layer） | 第 43–44 支 |
| SIMD、vector、predication、fine-grained multithreading | [第 41 支](lecture41_note.md) |
| shared bus、snoopy 協定 | [第 40 支](lecture40_note.md) |
| 記憶體頻寬的限制 | [第 38 支](lecture38_note.md#142-課本的-openmp-程式) |
| GPU 架構的細節、NoC 的 routing、flow control | Advanced Computer Architecture 課程 |

---

## 附：本講一句話總結

> **影片用 NVIDIA Tesla（GeForce 8800）說明 GPU：CPU 經過 bridge 把工作交給 GPU，繪圖和一般運算（GPGPU）共用 8 個 TPC × 2 個 SM，每個 SM 有 8 個很簡單的 SP 和 2 個能算超越函數的 SFU（共 160 個核心），32 個 thread 一組成為 warp、一起執行同一條指令（SIMT），每個 SM 每 4 個週期完成一個 warp 的一條指令，整顆每週期最多 128 條；CUDA 程式要 cudaMalloc、cudaMemcpy 把資料搬到 GPU，再用 kernel<<<blocks, threads>>> 讓每個 thread 依 threadIdx、blockIdx 處理自己的元素，最後搬回來。後半講 interconnect：bus 只撐得住 4–8 個處理器，更大的要用 router 和 link 組成的網路，用 bisection bandwidth（越大越好）和 diameter（越小越好）衡量：chain、ring 最差，mesh 最好做（√N、2√N），torus 把 diameter 減半、bisection 加倍，folded torus 再消掉超長的線，hypercube 和 butterfly 線最多但 diameter 只有 log N，適合很大的網路。實測本機 RTX 4060 有 24 個 SM、warp 一樣是 32，影片的 CUDA 向量加法 1600 萬筆全對、kernel 只要 0.84 ms，但搬資料要 33 ms；用暴力法算的 N = 16 拓樸指標和課本 Table 12.7 每格都一樣。**

---

*筆記依據 Smruti R. Sarangi「Basic Computer Architecture」Chapt 11: GPUs and Interconnects, Part 5/5 錄影逐字稿（YouTube 自動字幕，1:25:53）、Sarangi *Basic Computer Architecture* v3.09 Ch12 §12.6（p.620–630）整理。實測為本機（Intel i7-13650HX、NVIDIA GeForce RTX 4060 Laptop GPU、CUDA 12.4、Ubuntu、Python 3.10）結果。*
