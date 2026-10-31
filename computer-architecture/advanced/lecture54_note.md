# Advanced Computer Architecture 第五十四支影片完整筆記：安全處理器架構（三）：Intel SGX 飛地生命週期、反向分頁表（IPT）、Oblivious RAM（Path ORAM）與旁道攻擊（Chapter 13, Part III）

> **課程**：Advanced Computer Architecture — Chapter 13: Secure Processor Architectures (Side channel attacks and ORAM) -- Part III/III（播放清單第 54 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：**whisper 轉錄**（本機 `large-v3-turbo`；YouTube 字幕下載不到）（1:07:26）＋ 課本 *Next-Gen Computer Architecture* v3.2 **Ch13** §13.3.4–13.5（p.693–702）
> **章號**：影片 Chapter 13 = 課本 Ch13。
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄
> - `【實測】`：在本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、`gcc` 11.4）實際執行的結果
> - `⚠️`：逐字稿缺字、或影片／課本／實測說法不一致之處
> - `[mm:ss]`：影片時間點

⚠️ whisper 把「Merkle tree(s)」寫成「Muggle tree／market trees」、「quoting enclave」寫成「coating enclave」、「page faults」寫成「page falls」、「SGX」常寫成「SDX」、「ACIF」寫成「ASIF」、「version tree」聽成「waiting tree」、「`EENTER`」寫成「ENTER」、「`ECALLs`」寫成「recalls」、「Oblivious RAM」寫成「Obligious RAM」、「TCB／DRAM」寫成「TCP／NAMB」、「$\log_2 N$」聽成「log N to the pixel」、「`posMap`」寫成「post map」、「miss information」寫成「misinformation」、「RSA」寫成「RSC」、「nanosecond-level」寫成「nanometer level」、「side channel」寫成「side jam」、「a miss」寫成「amiss」，筆記直接改正。

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. Enclave 軟體環境與生命週期：`ECREATE`、`EADD`、`EINIT`、`EENTER`/`EEXIT` 與 `EREMOVE`](#1-enclave-軟體環境與生命週期ecreateeaddeiniteentereexit-與-eremove)
- [2. 不信任 OS 下的分頁安全：位址轉譯攻擊、反向分頁表（IPT）與頁置換](#2-不信任-os-下的分頁安全位址轉譯攻擊反向分頁表ipt與頁置換)
- [3. 隱藏記憶體位址流：Oblivious RAM（ORAM）與 Path ORAM 演算法](#3-隱藏記憶體位址流oblivious-ramoram與-path-oram-演算法)
- [4. 旁道攻擊（Side-Channel Attacks）分類學與 RSA 模指數洩密範例](#4-旁道攻擊side-channel-attacks分類學與-rsa-模指數洩密範例)
- [5. 快取、DRAM 與投機執行旁道：Prime+Probe、Flush+Reload、Rowhammer 與 Spectre/Meltdown](#5-快取dram-與投機執行旁道primeprobeflushreloadrowhammer-與-spectremeltdown)
- [6. 旁道防禦對策：資源分割、注入雜訊與常數時間填充](#6-旁道防禦對策資源分割注入雜訊與常數時間填充)
- [7. 【原始碼】Linux x86 `sgx/encls.h` 與 `ioctl.c` 的 Enclave 指令封裝](#7-原始碼linux-x86-sgxenclsh-與-ioctlc-的-enclave-指令封裝)
- [8. 【實測】真實硬體 Flush+Reload 竊取 RSA 金鑰、Path ORAM 位址熵與本機漏洞緩解狀態](#8-實測真實硬體-flushreload-竊取-rsa-金鑰path-oram-位址熵與本機漏洞緩解狀態)
- [9. 一頁速查表](#9-一頁速查表)
- [10. 與後續講次的連結](#10-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

```
上一講用 MEE + Bonsai Merkle Tree 保住了 DRAM 資料與計數器的 ACIF，但還剩三大缺口：
  │
  ├── 缺口一：OS 不可信，卻要靠 OS 建立飛地、管理分頁表與處理中斷／系統呼叫！（§13.3.4）
  │   ├── 記憶體切分：開機保留安全實體區（SGX v1 PRM 128 MB：約 92 MB 可用 EPC + 約 36 MB Metadata）
  │   ├── 生命週期指令：ECREATE（建控制結構）→ EADD（拷貝程式／資料並填 IPT）→ EINIT（TPM/Quoting Enclave 簽驗 Measurement 啟動）
  │   ├── 分頁防護：防 OS 發動 Address Translation Attack → 每次 TLB 填入安全映射前，硬體必查安全區內的反向分頁表（IPT / EPCM）
  │   │             Page Fault 抹除低 12-bit 頁內位移（但頁序列仍對 OS 可見）；換頁出安全區需配 Eviction/Version Tree + Nonce MAC
  │   └── 進出與拆除：外部中斷 AEX 或主動 EEXIT（OCALL 代辦 syscall）將暫存器存入安全區並 **Flush TLB**；EENTER（ECALL）進入；EREMOVE 清零拆除
  │
  ├── 缺口二：DRAM 資料雖已加密，但記憶體匯流排上的「實體位址流」仍是明文！（§13.3.5）
  │   └── Oblivious RAM（ORAM）：結合 Redundancy（一次讀寫多個區塊）＋ Permutation（每次存取後搬家）
  │       → Path ORAM：DRAM 排成二元樹（每節點 Bucket 含 B 塊）＋ 晶片內 Stash 與 posMap
  │         每次存取 addr a：查 pos=posMap[a] → 重抽隨機新葉 → readPath(pos) 全進 Stash → 由葉往根用 getCousins+trim(B) 寫回（慢 2–10×）
  │
  └── 缺口三：旁道攻擊（Side-Channel Attacks，§13.4）——不直接偷資料，靠「資料相依行為＋共享資源競爭／物理訊號」推敲秘密！
      ├── 案例：RSA 二進位模指數 `if (e % 2 == 1) z = (z*x)%n;` 的條件分支洩漏私鑰 e 的每一個 bit
      ├── 三大類型：
      │   • Type 1（觀測自身競爭）：Prime+Probe（填滿快取看誰被踢）、Flush+Reload（清共享行看誰回來）、執行單元／分支預測器競爭、Rowhammer
      │   • Type 2（軟體觀測／操控受害者）：Evict+Time、效能計數器、Page-fault 窺探、亂序投機暫態攻擊（Spectre / Meltdown / Foreshadow）
      │   • Type 3（實體接觸量測）：功耗波形、電磁輻射（EM）、溫度、匯流排位址探針
      └── 防禦三招：硬體資源分割（Cache Partitioning）、加入虛假存取雜訊、關閉高解析度計時器／投機執行或將敏感存取補齊至最差常數延遲
```

---

## 1. Enclave 軟體環境與生命週期：`ECREATE`、`EADD`、`EINIT`、`EENTER`/`EEXIT` 與 `EREMOVE`

在安全處理器（如 Intel SGX，課本 §13.3.4 p.693–694）的威脅模型裡，**作業系統（OS）並不被信任**，卻又必須負責發特權指令管理飛地（Enclave）、維護分頁表與處理中斷——這看似矛盾的分工透過專用 ISA 指令與硬體檢查達成：

- `[00:52]` **實體與虛擬記憶體雙重切分**：
  - 開機時將實體記憶體切成**非安全區（Unsecure / Non-secure region）**與**安全保留區（Secure region / PRM）**；每個安全行程的虛擬位址空間也同樣切為安全區與非安全區。
  - 【口頭】**SGX v1 vs 新版 Scalable SGX 的演進（`[01:24]–[02:14]`、`[14:04]`）**：早期 Intel SGX v1（⚠️ 老師口頭舉第 9、10 代為例，實務上自第 6 代 Skylake 起導入用戶端、第 11/12 代起從消費級 CPU 移除並改由伺服器級 Xeon 支援 SGX v2）的處理器保留記憶體（PRM）總共只有 **128 MB**，其中約 **36 MB（約 28% + 8%）用於儲存安全 Metadata**，真正留給 Enclave 程式碼與資料的空間（EPC）只有約 **92 MB**！新一代伺服器版大幅擴充了安全記憶體容量，但也因此捨棄了高成本的硬體 Merkle Tree 防重播機制。
- `[02:47]` **建立與啟動三部曲（`ECREATE` $\to$ `EADD` $\to$ `EINIT`）**：
  1. **`ECREATE`**：在安全 Metadata 區建立並初始化 Enclave 的控制資料結構（SGX 稱為 SECS），同時指派專屬的安全虛擬頁區段與安全實體頁框（frames）。
  2. **`EADD`**：將程式碼與初始資料從非安全頁複製進安全頁，並在安全 Metadata 區同步建立反向分頁表項目。
  3. **`EINIT`**：正式啟動前，由 TPM（或專門負責軟體密碼簽章的系統級姊妹飛地 **Quoting Enclave**，`[04:57]`）計算整個初始狀態的雜湊（**Measurement**）並做數位簽章；本機或遠端驗證簽章與量測值無誤後，才放行啟動。
- `[12:54]` **執行期進出、系統呼叫與拆除（`EENTER` / `EEXIT` / AEX / `EREMOVE`，p.694）**：
  - **外部中斷——非同步飛地退出（Asynchronous Enclave Exit, AEX，`[13:11]`）**：核心正在跑 Enclave 時若收到硬體中斷，絕不能像一般行程把暫存器與 PC 存進 OS 可讀的 PCB！硬體必須自動將完整安全執行緒狀態（暫存器、PC、Enclave ID 等）存進**安全 Metadata 區（SGX 稱 SSA）**，並且**強制清空（Flush）TLB 中的安全映射**（避免接手的 OS 透過殘留 TLB 偷讀安全頁，這是 SGX 重要的效能開銷來源），才把控制權交給 OS。
  - **主動進出與代辦系統呼叫（`EENTER` / `ECALL` 與 `EEXIT` / `OCALL`，`[14:41]`–`[17:50]`）**：
    - 宿主非安全行程透過包裝了 **`EENTER`** 指令的 **`ECALL`** 進入 Enclave 執行機密函式；要返回非安全區則執行包裝了 **`EEXIT`** 的 **`OCALL`**（離開時同樣儲存安全狀態並 **Flush TLB**；反之 `EENTER` 進入時不必清非安全 TLB）。
    - **Enclave 內部嚴禁直接呼叫 `syscall`**：因為系統呼叫會把狀態暴露給不可信的 OS。若需檔案／網路 I/O，必須透過共享非安全頁把參數傳出去，發 **`EEXIT`（`OCALL`）** 請宿主行程或代理執行緒（proxy thread）代跑系統呼叫，再由 **`EENTER`（`ECALL`）** 帶回結果。
  - **拆除飛地（`EREMOVE`，`[16:24]`）**：由 OS 觸發，硬體專用單元負責抹除並清零記憶體與磁碟交換區中的所有安全頁狀態。

---

## 2. 不信任 OS 下的分頁安全：位址轉譯攻擊、反向分頁表（IPT）與頁置換

- `[06:50]` **位址轉譯攻擊（Address Translation Attack）與反向分頁表（IPT，課本 p.693–694）**：
  - **威脅**：分頁表由不可信的 OS 維護；惡意 OS 大可偷偷修改分頁表，把 Enclave 的「安全虛擬頁」指到非安全區的實體頁框，誘騙 Enclave 把私鑰明文寫到 OS 看得見的地方（或讓兩個不同 Enclave 指到同一個實體頁框）！
  - **硬體防線——反向分頁表（Inverted Page Table, IPT；Intel SGX 實作稱為 EPCM）**：
    - 硬體在安全 Metadata 區維護一張**以「安全實體頁框編號（Physical Frame Number）」為索引**的反向分頁表，記錄哪一個 Enclave 的哪一個安全虛擬頁合法擁有該頁框。
    - `EADD` 加頁時填入 IPT，`EINIT` 前全面檢查所有安全頁是否皆落在合法安全區且無非法共用；啟動後，**CPU 絕不盲信 OS 的分頁表——每當任何涉及安全虛擬頁或安全實體頁框的映射要寫入 TLB 時，硬體必先查核 IPT**！一旦發現 OS 偷改映射（虛擬頁與 IPT 記載不符、或安全虛擬頁指往非安全實體頁框），硬體當場觸發 Fault 中止執行。
- `[05:38]` **Page Fault / TLB Miss 的資訊洩漏與頁置換（Swapping，p.694）**：
  1. **抹除頁內低 12-bit 位移（Page-offset zeroing，`[06:17]`）**：發生 Page Fault 交由 OS 處理時，硬體會把故障位址最低 12 bits（4KB 頁內 byte/word 位移）**全部清為 `0`**，只回報頁號（Page ID）。
     - ⚠️ **殘留的 Page-Fault 旁道漏洞**：即便遮住了頁內位移，惡意 OS 仍可故意頻繁清 TLB 或把頁換出，藉此觀察完整的**頁存取序列（page-access sequence）**推敲程式控制流——大多數安全處理器（含早期 SGX）無法單靠硬體擋住這類 **Page-fault snooping attack**。
  2. **安全頁換出（Eviction / Version Tree，`[11:15]`–`[12:53]`）**：當 Enclave 用量超過 92 MB 安全區需要把安全頁換出到一般記憶體或磁碟時，硬體會建立類似 Merkle Tree 的**逐出／版本樹（Eviction / Version Tree）**，用 `⟨Key, Nonce⟩` 加密頁面並計算維持 ACIF 保證的 MAC（把 `⟨Key, Nonce⟩` 留在安全區，甚至連儲存這些 Nonce 的 Metadata 頁本身也能遞迴保護換出），換入時嚴格驗證無誤才放行。

---

## 3. 隱藏記憶體位址流：Oblivious RAM（ORAM）與 Path ORAM 演算法

- `[17:58]` **為什麼加密了資料還不夠？（課本 §13.3.5，p.695）**
  - 每次 LLC Miss，記憶體控制器送上匯流排的**實體位址（Address）完全是明文**！因為標準商用 DRAM 數十年來拒絕在 DIMM 端加裝位址解密晶片。攻擊者光靠探針側錄位址序列（甚至惡意每跑幾條指令就發中斷逼出精細存取軌跡），就能看穿程式走了哪個分支、查了哪格表格並偷走金鑰。
- `[19:18]` **Oblivious RAM（ORAM，無感記憶體）的兩大核心支柱**：
  - 想要讓晶片外觀察到的位址流與真實程式邏輯徹底脫鉤，必須**同時結合兩件事**（Goldreich & Ostrovsky, 1996）：
    1. **冗餘存取（Redundancy）**：每次真正只想讀寫 1 個區塊，卻故意去讀寫 $K$ 個區塊（極端天真做法是每次讀寫整個 DRAM，絕對安全但慢到無法使用）。
    2. **動態重排搬家（Permutation）**：每次存取某個邏輯位址後，立刻把它隨機搬到新的實體位置（晶片內維護 Logical $\to$ Physical 映射表），讓同一個變數連續讀一萬次在 DRAM 匯流排上看起來像一萬次毫無關聯的隨機漫步！
- `[22:40]` **實用化突破：Path ORAM（Stefanov et al., 2013；課本 p.695–696）**：
  - 將效能懲罰從早期天文數字壓低到約 **2–10 倍減速**，由三大結構組成：
    1. **DRAM 二元桶樹（Binary Tree of Buckets）**：假設安全記憶體有 $N$ 個邏輯區塊（位址 $0 \dots N-1$），在主記憶體建立一棵具有 $2^L$ 個葉節點、共 $L+1$ 層（根為第 $0$ 層、葉為第 $L$ 層）的完整二元樹；樹上每個節點是一個可容納 **$B$ 個區塊**的**桶子（Bucket）**，桶內可放真實資料塊或填充用的虛假塊（dummy blocks）。
       - ⚠️ **課本符號小瑕庛**：課本 p.695 第 1521 行誤將葉節點層號寫成 `level L + 1`，但緊接著 p.696 虛擬碼第 16 行迴圈 `for l ∈ (L, L-1, ..., 1, 0)` 正確使用 `0..L` 共 $L+1$ 層；另影片 `[32:55]` 口頭偶爾把節點桶子（Bucket，含 $B$ 個 blocks）說成單一 block，閱讀時以虛擬碼 `|W| = B` 為準。
    2. **`posMap[0..N-1]`（位置映射表，晶片內）**：記錄每個邏輯區塊 $a$ 目前被指派到哪一片葉節點 $k \in \{0, \dots, 2^L-1\}$。**核心不變量（Path ORAM Invariant）**：邏輯區塊 $a$ 的實體資料，保證落在**從樹根（Level 0）走到第 `posMap[a]` 號葉節點（Level $L$）這條路徑 $P(\text{posMap}[a])$ 上的某一個桶子裡**！
    3. **`Stash`（晶片內暫存快取 $S$）**：位於記憶體控制器內的小型緩衝區，暫存剛讀上來或一時塞不回樹路徑的溢位區塊。

```
存取邏輯位址 a 的四步曲 — access(op, a, new_data)：
  Step 1 [查舊葉、抽新葉]：pos ← posMap[a]；立刻重新指派 posMap[a] ← random(0 .. 2ᴸ−1)！
  Step 2 [整條路徑讀入 Stash]：S ← S ∪ readPath(pos)（把根到舊葉 pos 沿途 L+1 個桶子共 (L+1)B 塊全讀進晶片）
  Step 3 [在晶片內完成讀寫]：old_data ← S[a]；若是 write 則更新 S[a] ← new_data
  Step 4 [由葉 l=L 往根 l=0 貪婪寫回原路徑 P(pos)]：
         對每一層 l = L, L−1, ..., 0：
           • W ← getCousins(pos, l)：從 Stash 挑出所有「新指派路徑 P(posMap[a']) 在第 l 層仍與舊路徑 P(pos) 共用同一節點」的區塊 a'
           • W ← trim(W)：取最多 B 個區塊（不足 B 個則補加密後的 dummy 區塊）寫回桶子 P(pos)[l]，並將真實區塊從 Stash 移除！
```

  - **為什麼既安全又不會撐爆 Stash？** 每次存取都會把目標塊 $a$ 重抽一條隨機新葉路徑，下次讀 $a$ 時走的是全新隨機路徑；而寫回時雖然只能沿著剛讀過的**舊路徑 `P(pos)`** 寫回（不額外洩漏新葉位置！），但因為所有樹路徑至少在靠近樹根的上層節點（尤其 Level 0 樹根）交會，把區塊儘量往深層（$l = L \to 0$）塞回，就能讓晶片內 `Stash` 維持極小容量（當桶大小 $B=4$ 時幾乎不會溢位）。

---

## 4. 旁道攻擊（Side-Channel Attacks）分類學與 RSA 模指數洩密範例

- `[35:25]` **什麼是旁道攻擊？（課本 §13.4，p.696–697）**
  - 【課本補充】作者舉了一個真實刑案故事：綁匪打 WhatsApp 勒索電話時竟能精準說出被害人正從客廳走向廚房，被害人嚇得以為家裡被裝針孔——其實綁匪認識她家格局，只是從**通話音質強弱推算出她離 Wi-Fi 分享器的遠近**！這種不破解加密演算法本身、而是透過執行過程無意間外洩的微弱訊號（延遲、功耗、電磁波、快取競爭）反推機密的手法，就是**旁道攻擊（Side-Channel Attack）**。（若是由安全區內部的惡意木馬「刻意」調變共享硬體把機密偷渡給外部共犯，則稱為**隱蔽通道 Covert Channel**）。
  - **構成旁道的兩大必要條件（`[52:33]`、`[57:39]`）**：
    1. **資料相依行為（Data-dependent behavior）**：程式的指令流、記憶體位址或運算資源需求隨秘密資料位元而變。
    2. **可觀測的干擾或物理特徵**：該行為改變了攻擊者量得到的時間、計數器或物理量（奈秒級**高解析度計時器**如 x86 `rdtsc` 正是放大快取命中與未命中百拍差距的頭號幫兇，但因網卡驅動與遊戲需要又無法輕易廢除）。
- `[37:46]` **貫穿全節的受害者範例：RSA 二進位平方乘模指數（Figure 13.12b，p.697）**：
  - 計算 $z = x^e \bmod n$ 時（⚠️ 影片 `[38:08]`、`[38:21]` 口誤說「$e, d, n$ 都是大質數」，實則 $p, q$ 為大質數、$n=pq$ 為合數、$e, d$ 僅與 $\lambda(n)$ 互質；另課本 p.697 圖 13.12b 第 3 行誤印為 `while (e = 0)`，應為 `while (e != 0)`）：
    ```c
    z = 1;
    while (e != 0) {
        if (e % 2 == 1)          // 檢查私鑰指數 e 目前最低位元（LSB）是否為 1！
            z = (z * x) % n;     // ★ 只有當前 bit == 1 時才會執行這段模乘法（Modular Multiply）！
        e = e >> 1;
        x = (x * x) % n;         // 每輪固定執行模平方（Modular Square）
    }
    ```
  - **致命破綻**：只要攻擊者能用任何間接手段偵測出每一輪迴圈「有沒有執行 `z = (z * x) % n` 這行指令（I-cache 行是否被抓進快取、乘法器是否忙碌、功耗是否跳升）」，就能**逐 bit 把整把私鑰指數 $e$（或解密私鑰 $d$）完整抄出來**！
- `[46:05]` **Szefer 旁道攻擊分類矩陣（課本 Figure 13.12a，p.697）**：

  | 觀測對象 \ 攻擊者性質 | **Logical（純軟體攻擊者）** | **Physical（具實體接觸權限的外部探針）** |
  |---|---|---|
  | **Self（只量測攻擊者自身執行受干擾程度）** | **Type 1**：共享微架構資源破壞性競爭<br>• 功能單元（ALU/FPU/乘法器）、分支預測器／BTB<br>• 快取系統（Prime+Probe、Flush+Reload）、DRAM Rowhammer | （課本留白——實體探針直接量目標即可） |
  | **Other（直接量測或操控受害者執行環境）** | **Type 2**：軟體監測受害者指標<br>• 總執行時間（Evict+Time）、硬體效能計數器（PMC）<br>• OS 窺探 Page-fault 序列、亂序投機暫態執行（Spectre/Meltdown） | **Type 3**：物理旁道量測<br>• 瞬時功耗波形（SPA/DPA）、電磁輻射（EM）<br>• 晶片溫度變化、DRAM 匯流排實體探針 |

---

## 5. 快取、DRAM 與投機執行旁道：Prime+Probe、Flush+Reload、Rowhammer 與 Spectre/Meltdown

- `[48:52]` **Type 1：攻擊者只量測自己（課本 §13.4.2，p.697–698）**：
  1. **`Prime+Probe`（填滿＋探測，不需共享記憶體頁）**：
     - **Prime**：攻擊者先讀寫自己的陣列，把目標 L1/L2 快取（或特定 4-way set 的全部 4 個 ways）填滿自己的快取行。
     - **Wait**：讓出 CPU 給受害者跑一小段（例如 RSA 的一輪迴圈）。
     - **Probe**：攻擊者拿回 CPU，用高解析度計時器重讀剛才填進去的每一條快取行——若某個 set 讀取變慢（Cache Miss），代表受害者剛才存取了映射到同一個 set 的位址（例如執行了 `z = (z*x)%n` 的指令行），把攻擊者的資料踢走了！
  2. **`Flush+Reload`（沖刷＋重載，`[50:20]`，精準度最高、適用於共享函式庫如 OpenSSL）**：
     - 當攻擊者與受害者共用唯讀程式碼頁（shared library）時，兩者的虛擬位址指向**完全相同的實體快取行**。
     - **Flush**：攻擊者用處理器提供的快取清除指令（如 x86 `clflush`）把目標函式（如模乘法指令行）從所有快取層踢回 DRAM。
     - **Reload**：等受害者跑完一輪後，攻擊者計時讀取該共享位址——**若極快命中（Hit），代表受害者剛才替我們把它抓回快取了（該 bit 為 `1`）；若極慢未命中（Miss），代表受害者沒走該分支（該 bit 為 `0`）**！
  3. **功能單元／分支預測器競爭與 `Rowhammer` 旁道（`[53:24]`，p.698）**：
     - 同核心超執行緒（SMT）下狂發乘法指令測試結構危障減速，或製造分支預測器別名（aliasing）碰撞。
     - **Rowhammer 讀取干擾旁道**：高頻反覆讀取（hammer）DRAM 某列 $R_0$ 會加速相鄰列 $R_1$ 電容漏電並誘發位元翻轉（觸發較慢的 ECC 修正）。課本與影片描述如何將鄰列逼近翻轉臨界點再排程受害者：一旦受害者資料相依地存取了目標列，就會把鄰列推過臨界點產生可由計時器或位元翻轉偵測到的延遲訊號。
- `[56:37]` **Type 2：軟體觀測受害者與投機暫態執行攻擊（課本 §13.4.3，p.698–699）**：
  - **`Evict+Time` 與效能計數器**：先量受害者正常總耗時，接著踢掉某組快取 set 再重跑一次受害者——若受害者整體變慢，即知其用到了該快取 set；或直接讀取硬體效能計數器（PMC）統計指令數推估私鑰中 `1` 的總個數（Hamming weight）。
  - **`[59:25]` 震撼近代計算機結構的暫態執行攻擊（Transient / Speculative Execution Attacks：Spectre、Meltdown、Foreshadow）**：
    - 回顧第 4 章亂序執行管線：分支指令抵達 ROB 頂端正式確認（commit）前，處理器會依據分支預測器**投機執行錯誤路徑上的記憶體讀取**；雖然猜錯後會沖刷管線與暫存器（架構狀態看似乾淨），但**投機期間抓進快取的痕跡卻不會復原**！
    - 看課本 p.699 經典 **Spectre v1（Bounds-Check Bypass）** 程式碼片段：
      ```c
      if (val < threshold)          // 攻擊者先用合法小 val 訓練分支預測器「總是跳入」，再傳入越界大 val！
          v = array1[array2[x]];    // 亂序核心在發現越界前，已投機讀出秘密值 s = array2[x]，再去讀 &array1[s]！
      ```
    - 攻擊者若能控制越界索引 `x`，使其指向行程內任意機密位址（如私鑰位元組），投機執行就會去存取位址 `array1_base + array2[x] * 4`（實務上常乘以快取行／頁大小如 `4096` 以放大訊號）。事後管線雖被沖刷，攻擊者只要對 `array1` 跑一輪 `Prime+Probe` 或 `Flush+Reload` 找出哪一格變成了 **Cache Hit**，就能直接反推出任意記憶體位址裡的秘密數值 `array2[x]`！（課本 p.702 補充：**Meltdown** 用類似暫態機制越權偷讀 OS 核心記憶體，**Foreshadow** 則直接偷讀 Intel SGX Enclave 快取資料！）。
- `[47:42]` **Type 3：實體旁道（課本 §13.4.4，p.699）**：量測模乘法與模平方截然不同的瞬時功耗波形、電磁輻射（EM）頻譜，或多次執行累積的溫度差異。

---

## 6. 旁道防禦對策：資源分割、注入雜訊與常數時間填充

課本 §13.4.5（p.699）與影片 `[64:13]–[66:25]` 總結三類通用防禦與其沉重代價：

1. **硬體資源嚴格分割（Resource Partitioning）**：把快取 sets／ways、分支預測器或運算單元依執行緒靜態切開，徹底消滅跨執行緒的破壞性干擾（代價：快取利用率下降）。
2. **刻意注入雜訊與消除資料相依存取（Adding Noise / Data-oblivious code）**：不論條件真假都存取固定位置或額外存取虛假位址（例如密碼學標準的 **Constant-time 實作**），抹平存取特徵（代價：額外執行與頻寬開銷）。
3. **限制計時器／投機執行，或【口頭】最差情況常數時間填充（Worst-case Constant-Time Padding，`[65:26]`）**：
   - 直接關掉高解析度計時器或關閉亂序投機執行會讓一般程式效能崩盤，實務上常針對邊界插入序列化屏障（如 `lfence`）。
   - 【口頭】老師補充自己參與的資訊理論研究結論：在保留計時器的前提下，要從理論上切斷敏感資料的時序旁道，硬體可將敏感快取區標記起來，**強制讓每一次敏感讀取（無論實際 Hit 或 Miss）都固定延遲到最差未命中時間 $N$ cycles 才回傳**——把所有命中全部偽裝成未命中，讓攻擊者的計時器量不出任何時間差！

---

## 7. 【原始碼】Linux x86 `sgx/encls.h` 與 `ioctl.c` 的 Enclave 指令封裝

本講前半 §13.3.4 介紹的 Intel SGX 特權管理指令（`ECREATE`、量測雜湊、載入頁面 `EADD`、驗簽啟動 `EINIT`、換出寫回 `EWB`、換入驗證 `ELDU` 與拆除 `EREMOVE`），在 x86 硬體中全部統一編碼成單一特權指令 **`ENCLS`（opcode `0x0f 0x01 0xcf`）**，由 `%eax` 葉號（leaf number）區分功能。打開 Linux v6.2.12 的 `arch/x86/kernel/cpu/sgx/encls.h` 即可看到一對一的硬體指令介面：

```c
/* arch/x86/kernel/cpu/sgx/encls.h:139-170 — 對應課本 §13.3.4 建立、量測、載入、啟動與拆除 Enclave */
/* Initialize an EPC page into an SGX Enclave Control Structure (SECS) page. */
static inline int __ecreate(struct sgx_pageinfo *pginfo, void *secs) {
	return __encls_2(ECREATE, pginfo, secs);                 /* 建立 Enclave 控制結構 SECS */
}
/* Hash a 256 byte region of an enclave page to SECS:MRENCLAVE. */
static inline int __eextend(void *secs, void *addr) {
	return __encls_2(EEXTEND, secs, addr);                   /* 累加計算 Enclave 的 Measurement 雜湊！ */
}
static inline int __eadd(struct sgx_pageinfo *pginfo, void *addr) {
	return __encls_2(EADD, pginfo, addr);                    /* 拷貝程式／資料進安全 EPC 頁並登記 EPCM(IPT) */
}
/* Finalize enclave build, initialize enclave for user code execution. */
static inline int __einit(void *sigstruct, void *token, void *secs) {
	return __encls_ret_3(EINIT, sigstruct, secs, token);     /* 核對數位簽章與 Measurement 後正式解鎖啟動 */
}
static inline int __eremove(void *addr) {
	return __encls_ret_1(EREMOVE, addr);                     /* 解除並清理安全 EPC 頁 */
}
```

而在同檔案下方 `arch/x86/kernel/cpu/sgx/encls.h:190-216` 則是課本 p.694「將安全頁安全換出至不可信記憶體」的三劍客——`EPA` 建立存放 Nonce 版本號的 **Version Array（VA）頁**、`EWB` 加密產出 MAC 換出、`ELDU` 驗 MAC 與版本號換回！

---

## 8. 【實測】真實硬體 Flush+Reload 竊取 RSA 金鑰、Path ORAM 位址熵與本機漏洞緩解狀態

我們撰寫 [experiments/lecture54/sidechannel_and_oram.c](experiments/lecture54/sidechannel_and_oram.c)，在本機 Intel Core i7-13650HX 上實測三項結果：(1) 不靠任何模擬器，直接用 x86 硬體 `_mm_clflush` 與 `__rdtscp` 對課本圖 13.12b 的 RSA 模指數發動真實 **`Flush+Reload` 快取旁道攻擊**，偷取 16-bit 私鑰 `0xB5A3`，並對照常數存取修正版；(2) 完整實作課本 p.696 的 **Path ORAM**（$N=4096$ 邏輯區塊、$L=10$ 即 $1024$ 葉、每桶 $B=4$ 塊），對**同一個秘密位址 `a = 42` 連續讀寫 10,000 次**，驗證資料正確性與 DRAM 位址匯流排的資訊熵（Shannon Entropy）；(3) 讀取本機 Linux 核心 `/sys/devices/system/cpu/vulnerabilities/*` 檢視真實第 13 代 Intel CPU 對 Meltdown / Spectre / Foreshadow(L1TF) 的硬體與軟體防護。

### 實驗程式碼重點節錄

```c
/* experiments/lecture54/sidechannel_and_oram.c:35-56 — 圖 13.12b 有漏洞的 RSA 步驟 vs 常數存取修正版 */
static uint64_t rsa_step_vulnerable(uint64_t *z, uint64_t *x, uint64_t n, int bit) {
    if (bit == 1) {
        volatile uint8_t g = *MUL_GADGET_LINE; /* 只有私鑰 bit == 1 時才觸碰共享快取行！ */
        (void)g;
        *z = (uint64_t)(((__uint128_t)(*z) * (*x)) % n);
    }
    *x = (uint64_t)(((__uint128_t)(*x) * (*x)) % n);
    return *z;
}
/* 攻擊者：_mm_clflush(MUL_GADGET_LINE) -> 讓受害者跑一輪 -> timed_read(MUL_GADGET_LINE) < thresh 即知 bit==1！ */
```

### 執行指令與實測輸出

```bash
gcc -O2 -Wall -Wextra computer-architecture/advanced/experiments/lecture54/sidechannel_and_oram.c -o /tmp/sidechannel_and_oram -lm
taskset -c 0 /tmp/sidechannel_and_oram && rm -f /tmp/sidechannel_and_oram
```

```text
=== 1. Hardware Flush+Reload Side-Channel Attack on RSA Key ===
Cache Hit latency  :  51 TSC-cycles | clflush Miss latency: 278 TSC-cycles (threshold=164)
True RSA key e     : 0xB5A3
Vulnerable modexp  : 0xB5A3 (16 / 16 bits leaked = 100.0% accuracy!)
Constant-access fix: 0xFFFF ( 9 / 16 bits matched = 56.2%, leaks only all-1s mask)

=== 2. Path ORAM (N=4096, L=10 [1024 leaves], B=4): 10,000 Accesses to a=42 ===
Data integrity check  : 100% EXACT
Address-stream entropy: Plain DRAM = 0.000 bits (100% addr=42) vs Path ORAM = 9.925 / 10.000 bits!
Bandwidth & Stash     : 88 DRAM blocks/op (= 2*(L+1)*B) | Stash avg=0.00, max=0 blocks

=== 3. Local Host CPU Side-Channel Mitigations (/sys/devices/system/cpu/vulnerabilities) ===
  meltdown    : Not affected
  spectre_v1  : Mitigation: usercopy/swapgs barriers and __user pointer sanitization
  spectre_v2  : Mitigation: Enhanced / Automatic IBRS, IBPB: conditional, RSB filling, PBRSB-eIBRS: SW sequence
  l1tf        : Not affected
  mds         : Not affected
```

### 實測觀察與課本對照

1. **51 拍 vs 278 拍的鴻溝讓私鑰 100% 裸奔**：在本機 i7-13650HX 上，快取命中含柵欄僅 `51` TSC cycles，被 `clflush` 踢到 DRAM 後飆升到 `278` cycles（相差 `5.5x`）。攻擊者完全不需特權，光靠量自己的 `timed_read` 就把 16-bit 私鑰 `0xB5A3` **一字不差（`16/16 = 100.0%`）** 全數還原；改用無分支常數存取版後，攻擊者永遠只讀到全命中 `0xFFFF`，徹底切斷私鑰相關性（另呼應第 46 講我們實測時發現非 root 讀取 `/sys/class/powercap/intel-rapl` 被拒——那正是 Linux 防堵軟體功耗旁道攻擊 **PLATYPUS** 的真實防線！）。
2. **Path ORAM 將 `0.000 bits` 完全洩密的熱點存取變成接近滿熵 `9.925 / 10.000 bits` 白噪音**：儘管 CPU 連續一萬次都在讀寫同一個位址 `a = 42`，匯流排探針看到的卻是均勻散落在 $2^{10}=1024$ 條葉路徑上的隨機序列，且桶大小 $B=4$ 讓寫回後的殘留 `Stash` 始終為 `0`；代價則是每次邏輯存取需搬動 $2 \times (10+1) \times 4 = \mathbf{88}$ 個區塊，印證課本所言高昂的頻寬成本。
3. **真實第 13 代矽晶片的硬體演化**：本機 Raptor Lake 矽晶片已在硬體層修復了 `meltdown`、`l1tf`（Foreshadow）與 `mds`（皆顯示 `Not affected`），但跨分支預測與越界投機的 **`spectre_v1` 與 `spectre_v2` 因深植於亂序預測架構本質**，至今仍須仰賴指標遮罩柵欄與硬體 `Enhanced / Automatic IBRS` 協同防禦！

---

## 9. 一頁速查表

| 模組／攻擊 | 核心機制 | 架構重點與代價 |
|---|---|---|
| **SGX 飛地建立與進出** | `ECREATE` $\to$ `EADD` $\to$ `EINIT`；執行期用 `EENTER`(`ECALL`) / `EEXIT`(`OCALL`) / AEX | Enclave 內禁跑 `syscall`；每次 `EEXIT` 或中斷 AEX 必須把狀態存入安全區並 **Flush TLB** |
| **反向分頁表（IPT / EPCM）** | 以安全實體頁框反查合法虛擬頁，監控所有寫入 TLB 的安全映射 | 即使不可信 OS 管分頁表，也無法發動 Address Translation Attack 把安全頁導向非安全頁框 |
| **Path ORAM** | 二元桶樹（每桶 $B$ 塊）＋ 晶片內 `posMap` 與 `Stash`；每次讀整條路徑並重抽隨機新葉 | 同時具備 Redundancy 與 Permutation，隱藏明文 DRAM 位址流；每次存取放大為 $2(L+1)B$ 塊 I/O |
| **Prime+Probe vs Flush+Reload** | 前者填滿快取 set 偵測被受害者踢走（慢）；後者 `clflush` 共享行偵測被受害者載入（快） | 利用高解析度計時器將資料相依的指令／資料存取轉化為私鑰位元流 |
| **Spectre / Meltdown** | 亂序核心在錯誤預測路徑投機載入越界記憶體 `array1[array2[x]]`，沖刷後殘留快取痕跡 | 架構狀態雖回滾，微架構快取狀態卻已洩密；需靠硬體隔離、指標遮罩與序列化屏障防禦 |
| **旁道防禦三原則** | (1) 快取／資源分割、(2) 常數時間無分支＋雜訊存取、(3) 敏感位址補齊至最差 Miss 延遲 | 安全性與效能／資源利用率正面衝突，須依威脅模型取捨 |

---

## 10. 與後續講次的連結

- **總結第 13 章（第 52–54 講）完整防禦縱深**：第 52 講用 **AES-CTR + Session Key** 保住機密性與管線吞吐 $\to$ 第 53 講用 **Carter–Wegman MAC + 64-ary Bonsai Merkle Tree** 保住晶片外 DRAM 資料與計數器的完整性、真實性與鮮度 $\to$ 本講用 **IPT + Enclave 指令集** 隔離惡意 OS，並揭示即便密碼學毫無破綻，微架構共享快取、明文位址匯流排與亂序投機執行仍會開啟旁道戰場。
- **往下接 [lecture55_note.md](lecture55_note.md)（第 14 章：機器學習硬體架構）**：完成效能（Ch2–10）、功耗與溫度（Ch11）、可靠度（Ch12）與安全性（Ch13）四大支柱後，全課程最後四講（第 55–58 講）將進入當代計算機結構最火熱的領域專用架構（DSA）——**深度學習與神經網路加速器（GPU / TPU / Systolic Arrays / PIM）**！

---

## 附：本講一句話總結

**安全處理器透過反向分頁表（IPT）與飛地指令集在不信任作業系統下守住虛擬記憶體隔離，以結合冗餘讀取與隨機搬家的 Path ORAM 掩護明文位址匯流排，並正面迎戰由「資料相依存取＋微架構資源競爭／投機執行」所引發的快取與暫態旁道攻擊。**

---

*筆記整理自 Smruti R. Sarangi 教授 Advanced Computer Architecture 第 54 講影片與 Next-Gen Computer Architecture v3.2 第 13 章 §13.3.4–13.5，搭配 Linux v6.2.12 原始碼與本機實測驗證。*
