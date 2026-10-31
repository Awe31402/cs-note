# Advanced Computer Architecture 第五十三支影片完整筆記：安全處理器架構（二）：SHA 雜湊、MAC、緩衝區溢位、TCB／TPM、記憶體加密引擎（MEE）與 Bonsai Merkle Tree（Chapter 13, Part II）

> **課程**：Advanced Computer Architecture — Chapter 13: Security (Part II/III) -- Secure Processors（播放清單第 53 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：**whisper 轉錄**（本機 `large-v3-turbo`；YouTube 自動字幕開頭誤判為印地語且辨識殘缺）（1:24:53）＋ 課本 *Next-Gen Computer Architecture* v3.2 **Ch13** §13.2–13.3.3（p.680–692）
> **章號**：影片 Chapter 13 = 課本 Ch13。
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄
> - `【實測】`：在本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、`gcc` 11.4）實際執行的結果
> - `⚠️`：逐字稿缺字、或影片／課本／實測說法不一致之處
> - `[mm:ss]`：影片時間點

⚠️ whisper 把「well-formed」寫成「well found」、「permute」寫成「per mute」、「224 and 512」聽成「four and five」、「replays／nonces」寫成「replace／nonsense」、「program's code」寫成「program score」、「Kerckhoffs's principle」寫成「kirchhoff's principle」（課本 p.685 亦少一個 `s` 拼作 `Kerckhoff's`）、「TCB／TEE」常寫成「tcp／t」、「Trusted Platform Module」口誤並被轉錄成「trusted platform manager」、「OS loader」寫成「os later」、「context switch」寫成「contact switch」、「ACIF」寫成「asif／SAF／as if」、「PUF」寫成「puff」、「SGX／MEE」寫成「sdx／me」、「arity／64-ary」寫成「addity／64 edit／64 added」，筆記一律校正。

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. 主動密碼攻擊與防護：MITM、Splicing、SHA 雜湊、MAC、Replay 與 ACIF 保證](#1-主動密碼攻擊與防護mitmsplicingsha-雜湊macreplay-與-acif-保證)
- [2. 傳統軟體攻擊與防禦：Buffer Overflow、NX 不可執行頁與 ASLR](#2-傳統軟體攻擊與防禦buffer-overflownx-不可執行頁與-aslr)
- [3. 硬體安全核心基石：TCB、威脅模型、TEE／Enclave、RoT／TPM、遠端證明與封存](#3-硬體安全核心基石tcb威脅模型teeenclaverottpm遠端證明與封存)
- [4. 安全處理器記憶體架構（MEE）：冷開機攻擊、Split-Counter CTR 與 Miss 陰影預計算](#4-安全處理器記憶體架構mee冷開機攻擊split-counter-ctr-與-miss-陰影預計算)
- [5. 保護計數器本身：傳統 8 元 Merkle Tree vs 64 元計數器樹（Bonsai Merkle Tree）](#5-保護計數器本身傳統-8-元-merkle-tree-vs-64-元計數器樹bonsai-merkle-tree)
- [6. 【原始碼】Linux `fs/verity/verify.c` 如何在受信任快取節點提前終止 Merkle Tree 驗證](#6-原始碼linux-fsverityverifyc-如何在受信任快取節點提前終止-merkle-tree-驗證)
- [7. 【實測】SHA-NI 硬體加速、MEE 四種攻擊攔截矩陣與 1 GiB 記憶體 Bonsai 樹壓縮](#7-實測sha-ni-硬體加速mee-四種攻擊攔截矩陣與-1-gib-記憶體-bonsai-樹壓縮)
- [8. 一頁速查表](#8-一頁速查表)
- [9. 與後續講次的連結](#9-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

```
單純加密只能擋被動竊聽（Confidentiality），擋不住主動攻擊者 Mallory！
  │
  ├── 四大主動攻擊與 ACIF 保證（§13.2，Definition 103）
  │   ├── Spoofing / Man-in-the-Middle（冒充身分）──► 數位簽章 / MAC 確保 Authenticity
  │   ├── Tampering（竄改位元）                   ──► SHA 單向雜湊 + 加密雜湊 MAC 確保 Integrity
  │   ├── Splicing（跨通道／跨記憶體位址拼接密文）──► 不同對象用不同金鑰（記憶體中把 Block Address 綁進 OTP）
  │   └── Replay（重送舊的合法 ⟨密文, MAC⟩）      ──► 一次性金鑰（CTR 計數器）/ 時間戳記 / Nonce 確保 Freshness
  │
  ├── 傳統系統安全邊界與硬體安全語彙（§13.3.1–13.3.2）
  │   ├── 軟體緩衝區溢位（覆寫堆疊 Return Address）：Code Injection（NX/W^X 頁攔截）vs Code Reuse/ROP（ASLR 隨機化攔截）
  │   └── 硬體語彙：TCB（Kerckhoffs 公開設計原則＋最弱環節定律）→ Attack Surface ⊇ Threat Model
  │                 TEE / Enclave ← 由 Root of Trust（TPM）建立 Chain of Trust（Measurement 雜湊 → Remote Attestation → Sealing）
  │
  └── 安全處理器參考架構（類 Intel SGX，§13.3.3）
      ├── 邊界：晶片內（Core + Cache + MEE）屬於 TCB；晶片外 DRAM 匯流排可遭竊聽、竄改與液態氮 Cold Boot Attack
      ├── MEE Split-Counter 加密：每 4KB 頁配 1 個 64b Major Counter + 64 個 6b Minor Counter（共 448 bits）
      │     • 128b CTR 輸入 = Major(64b) || Minor(6b) || BlockAddr(58b)；AES Key = f(PUF, BootRand, EnclaveID)
      │     • 讀取時在 100–400 cycles LLC Miss 陰影下預先算好 OTP（解密零關鍵路徑延遲！）；Minor 溢位(63→0)則遞增 Major 並重加密整頁
      └── 「誰來保護被逐出到 DRAM 的計數器？」（計數器一旦遭連同密文+MAC 一起回滾就會破防）
            • 傳統 Merkle Tree：父節點存子節點 8B Hash → 64B 快取行僅容納 8 個子節點（8-ary，1GB 需 7 層）
            • 64-ary 計數器樹（Bonsai Merkle Tree）：每個節點統一為「448b 計數器 + 閒置 64b 存自身 MAC（由父節點計數器加密）」
              → 分支度暴增 8 倍變 64-ary（1GB 僅需 4 層），樹根鎖在晶片內 MEE 暫存器，且遇到晶片內快取節點即可提前終止驗證！
```

---

## 1. 主動密碼攻擊與防護：MITM、Splicing、SHA 雜湊、MAC、Replay 與 ACIF 保證

- `[00:29]` **被動竊聽 vs 主動攻擊（課本 §13.2.1，Figure 13.5，p.680–681）**：
  - 上一講（[lecture52_note.md](lecture52_note.md)）的加密解決了被動竊聽者讀取內容的問題（Confidentiality），但主動攻擊者 Mallory 可以攔截、刪除、修改或偽造通道上的訊息：
    1. **中間人／冒充攻擊（Man-in-the-Middle / Spoofing Attack）**：Mallory 對 Alice 假裝自己是 Bob、對 Bob 假裝自己是 Alice，在中間解密偷看並竄改後重加密轉傳。可用數位簽章確認發送者身分（Authenticity）來防範，但 RSA 簽章至少會膨脹訊息 $\ge 64$ bits（偽造機率 $2^{-k}$）且模指數運算太慢。
    2. **拼接攻擊（Splicing Attack，`[02:19]`）**：Mallory 收集過往合法通訊片段（影片舉 Alice 傳給表兄 Carlos 的簽章登入封包轉貼給 Bob；課本 p.681 舉將 Bob$\leftrightarrow$Carlos 合法加密訊息拼接到 Alice 標頭），若共用同一把金鑰就會受騙。
       - 【課本補充】**記憶體架構上的關鍵推論（p.681）**：把 Alice 與 Carlos 想成**兩個不同的實體記憶體位址**！若兩個位址共用完全相同的加密參數，攻擊者就能把位址 `C` 合法的 `⟨密文, 驗證碼⟩` 複製貼上到位址 `A` 而不被發現——因此**不同記憶體位址必須綁定各自的位址資訊（相當於不同金鑰）**。
- `[04:22]` **SHA 單向雜湊函數（SHA-based Hashing，課本 §13.2.2，p.681–682）**：
  - 無論輸入是幾 KB 或幾百 GB，**單向雜湊函數（One-way hash function）**都會將位元徹底攪拌，壓縮成固定長度的摘要（SHA 家族為 160–512 bits，其中 SHA-2 介於 **224 到 512 bits**）。單向性保證無法從雜湊反推原文，且長度 $l$ bits 的雜湊發生碰撞的機率約為極渺小的 $2^{-l}$（例如 $2^{-160}$ 或 $2^{-512}$）。
  - **SHA-2 內部結構**：與 AES 的多回合哲學類似，把訊息切成 **512-bit（64-byte）區塊**，每塊再拆成 **16 個 32-bit words**，經過 **64 個回合（rounds）**的排列、位移、旋轉與回合常數布林運算產出最終雜湊。
- `[07:20]` **訊息鑑別碼（MAC, Message Authentication Code，課本 §13.2.3，p.682）**：
  - `[15:31]` **為什麼單傳 `⟨Message, Hash⟩` 沒用？** 因為 SHA 演算法公開，中間人竄改訊息後自己重算一個新 Hash 一併送出，接收端照樣比對成功！
  - **MAC = 用共享秘密金鑰加密過的雜湊**（硬體常用做法：先快速算訊息的 SHA 雜湊 $H(M)$，同時平行用共享 Session Key 在 CTR 模式算出一塊短 OTP，令 $\text{MAC} = H(M) \oplus \text{OTP}$，即 **Carter–Wegman 結構**）。沒有共享金鑰的攻擊者既無法偽造對應新訊息的 MAC（確保 **Integrity**），接收端驗過 MAC 也同時確認訊息必出自持有該金鑰的發送端（確保 **Authenticity**），且計算速度遠快於 RSA 數位簽章。
- `[09:13]` **最難纏的重送攻擊（Replay Attack）與三種鮮度（Freshness）防線（課本 §13.2.4，p.682–683）**：
  - **攻擊手法**：Mallory 完全不改動任何位元，而是側錄過去合法的整組 `⟨密文, MAC⟩`（例如一小時前的銀行扣款指令或登入封包），事後原封不動重播；由於訊息未遭竄改且確實出自 Alice，密文與 MAC 天衣無縫吻合！
  - **三種確保資料鮮度（Freshness，拒絕陳舊資料）的解法**：
    1. **一次性工作階段金鑰（One-time Session Keys）**：每輪通訊都換金鑰——用第 52 講的 **CTR 模式每次遞增 minor counter**，效果就等價於每次都換一把新金鑰。
    2. **時間戳記（Timestamps）**：在加密訊息內嵌發送端時間，但需要收發雙方時鐘同步且欄位大、怕溢位。
    3. **Nonce（單調遞增整數計數器）**：每傳一筆訊息就把內嵌整數 `nonce` 加 1（如 $100 \to 101$），接收端只要維護一個狀態變數記住上次收到的 `nonce`（假設 FIFO 依序送達），重播的舊封包立刻因 `nonce` 過期被丟棄。
- `[14:48]` **ACIF 四大安全保證（課本 Definition 103，p.683）**：
  - 將上述四項屬性合稱 **ACIF 保證**：**A**uthenticity（真實性）、**C**onfidentiality（機密性）、**I**ntegrity（完整性）、**F**reshness（鮮度）。（【課本補充】一般資安常提的 Availability 可用性／阻斷服務攻擊與處理器微架構設計關聯較低，本章不納入）。

---

## 2. 傳統軟體攻擊與防禦：Buffer Overflow、NX 不可執行頁與 ASLR

在設計特殊安全硬體之前，課本 §13.3.1（p.684–685）先檢視一般處理器已具備的防線（虛擬記憶體隔離各行程位址空間、檔案系統權限控管），以及單純靠傳統虛擬記憶體擋不住的經典軟體漏洞——**緩衝區溢位（Buffer Overflow，課本 Figure 13.6）**：

```
高記憶體位址 ▲  ┌────────────────────┐
             │  │ Function Arguments │
向下成長的堆疊│  ├────────────────────┤
(Stack)      │  │ Return Address     │ ◄── `buffer[20]` 越界寫入 `buffer + 80`，精準覆寫返回位址！
             │  ├────────────────────┤
             │  │ Local Variables    │
低記憶體位址 ▼  │ int buffer[10]     │ ──► 陣列索引往高記憶體位址遞增，C/C++ 不做邊界檢查
                └────────────────────┘
```

- `[17:30]` **成因與兩種攻擊變體**：
  - C/C++ 為求效能不做陣列邊界檢查；宣告 `int buffer[10]` 卻寫入 `buffer[20]` 時，會直接寫到 `&buffer[0] + 20 * 4` 位址，覆寫堆疊訊框（activation block）上方的函式**返回位址（Return Address）**：
  1. **程式碼注入攻擊（Code Injection Attack，`[20:35]`，Figure 13.6b）**：攻擊者在輸入字串裡塞入惡意機器碼，並把返回位址改指回 `buffer` 內部，函式 `ret` 時直接跳進堆疊執行惡意指令。
     - **硬體對策——資料頁標記不可執行（Non-Execute / NX bit，`[21:35]`）**：只要在分頁表（Page Table）與 TLB 加一個位元區分程式碼頁與資料頁，禁止執行任何位於堆疊／堆積資料頁上的指令，即可徹底封殺堆疊注入。
  2. **程式碼重用攻擊（Code Reuse Attack / ROP，`[22:14]`，Figure 13.6c）**：既然資料頁不能執行，攻擊者改把返回位址指到**主程式或函式庫原本就合法存在的函式片段（課本稱 gadgets）**，像串珠一樣把多段既有程式碼串接起來拼出惡意邏輯。
     - **作業系統＋架構對策——位址空間配置隨機化（ASLR, Address Space Layout Randomization，`[24:31]`）**：每次程式啟動時，OS 隨機設定 stack、heap、動態連結庫與 mmap 區段的起始基底位址（實際位址 $=$ 隨機 base $+$ offset），讓攻擊者事先猜不到既有函式與返回位址落在哪裡。

---

## 3. 硬體安全核心基石：TCB、威脅模型、TEE／Enclave、RoT／TPM、遠端證明與封存

當雲端租戶連**作業系統（OS）、Hypervisor 與機房管理員都不信任**時，傳統虛擬記憶體便無能為力。`[26:34]`（課本 §13.3.2，p.685–687）建立整套硬體安全術語體系：

| 概念 | 定義與架構角色 |
|---|---|
| **TCB（Trusted Computing Base）** | 系統中假設絕對安全、共同維持 ACIF 保證的硬體／軟體元件集合。遵循兩大鐵律：<br>1. **Kerckhoffs 原則**：TCB 的硬體與演算法設計必須公開，**唯一保密的只有金鑰**（靠藏匿設計遲早因離職員工或逆向工程破功）。<br>2. **鎖鏈最弱環節定律（Chain-link principle）**：安全是串聯失效系統，TCB 中只要一個元件被攻破就全盤淪陷。 |
| **Attack Surface（攻擊面）** | 針對 TCB 所有潛在漏洞可發動的一切被動／主動攻擊超集。**TCB 愈肥大，攻擊面就愈大**（因此安全處理器極力把肥大的 OS 踢出 TCB！）。 |
| **Threat Model（威脅模型）** | 攻擊面的子集——架構師預期實務上會發生並願意付出硬體成本去防禦的特定攻擊向量集合。 |
| **TEE 與 Enclave** | **TEE（Trusted Execution Environment）** 是硬體提供的安全執行環境框架；每個受保護程式在 TEE 內運行的隔離沙箱實例稱為 **Enclave（飛地）**，連惡意 OS 都無法偷看或竄改其執行。 |
| **Root of Trust（RoT）／TPM** | `[30:55]` 「如果連 OS 都是惡意的，誰來保證 TEE 不是 OS 偽造的假沙箱？」系統必須有一顆假設絕對不會被攻破的信任起點 **RoT**（實務上常做成獨立安全協同晶片 **TPM, Trusted Platform Module**），負責在開機時驗證 BIOS、保管公私鑰（可由 PUF 衍生）並提供簽章／加密服務。 |
| **Measurement 與 Chain of Trust** | `[33:38]` RoT 計算 BIOS 的雜湊（稱為 **Measurement 量測值**）$\to$ BIOS 量測 OS Loader $\to$ Loader 量測 OS $\to$ 直到 TCB 最後一棒，串成**信任鏈（Chain of Trust）**（若 OS 不在 TCB 內，信任鏈就只驗到硬體／韌體與 Enclave 本身！）。 |
| **Remote Attestation（遠端證明）** | `[35:01]` 處理器／TPM 用硬體私鑰對 TEE 的 Measurement 做數位簽章送給遠端伺服器；遠端向第三方取得該處理器公鑰驗簽並比對合法雜湊白名單後，才放心把機密資料用處理器公鑰加密傳進 Enclave。 |
| **Sealing（資料封存）** | `[35:50]` Enclave 關機、排程切換或把暫存資料寫入硬碟前，向 RoT 取得由**當前 Measurement 衍生出的加密金鑰**將資料加密落盤；下次唯有在相同硬體、相同韌體／程式碼量測值一致的合法環境下才能解開（Unseal）。 |

---

## 4. 安全處理器記憶體架構（MEE）：冷開機攻擊、Split-Counter CTR 與 Miss 陰影預計算

`[37:25]` 進入本講核心（課本 §13.3.3，p.687–690，以 **Intel SGX** 為靈感的參考架構）：

- **TCB 與威脅模型（Figure 13.7）**：
  - **TCB 邊界就是處理器封裝（CPU Package）**：核心、NoC、L1/L2/LLC 快取與記憶體控制器內的 **MEE（Memory Encryption Engine）** 皆在晶片內受保護；**晶片外的主記憶體（DRAM）與記憶體匯流排完全不可信**。
  - `[38:01]` **液態氮冷開機攻擊（Cold Boot Attack）**：攻擊者可直接對主機板噴灑液態氮降溫，讓 DRAM 電容在拔離插槽後仍維持電荷數秒至數分鐘，再把 DIMM 插到另一台跑惡意 OS 的主機板上任意讀寫整條記憶體！此外匯流排探針也能竊聽、偽造、拼接或重播封包。因此**每筆跨出 LLC 的記憶體流量都必須由 MEE 提供完整 ACIF 保護**。
- `[43:10]` **Split-Counter CTR 記憶體加密設計（課本 Figure 13.8–13.9，p.688–690）**：
  - 為了防止相同明文產生相同密文以及防範 Replay，每次快取行寫回 DRAM 都必須換新的 OTP。若每個區塊各自存一把 128-bit 金鑰會爆量，因此善用空間局部性，以 **4 KB 實體頁（含 $64$ 個 64-byte 快取行）** 為單位配置**分離式計數器（Split Counters）**：
    - **1 個 64-bit Major Counter**（整頁共用，初始可設隨機值，實務上 $2^{64}$ 永遠不會溢位）。
    - **64 個 6-bit Minor Counters**（頁內每個 64B 快取行各配 1 個，初始為 $0$，每次該行從 LLC 髒寫回 DRAM 時 $+1$）。
    - **頁計數器總空間** $= 64 + 64 \times 6 = \mathbf{448\text{ bits}}$！放進一條標準 **64-byte（512-bit）** 快取行（可存在晶片內專用 **Counter Cache** 或一般快取）剛好還剩 **$512 - 448 = 64\text{ bits}$ 閒置空間**（下一節的伏筆！）。
  - `[45:48]` **128-bit AES 輸入與三重綁定硬體金鑰**：
    - 假設 64-bit 實體位址空間、64B 區塊位移佔 6 bits，則**實體區塊位址（Block Address）剛好是 58 bits**！將三者串接成精準的 128 bits 送進 AES：
      $$\text{Seed}_{128\text{b}} = \underbrace{\text{Major Counter}}_{64\text{ bits}} \,\|\, \underbrace{\text{Minor Counter}}_{6\text{ bits}} \,\|\, \underbrace{\text{Block Address}}_{58\text{ bits}}$$
      - **為什麼一定要拼入 58-bit `Block Address`？** 防止跨位址的 **Splicing 攻擊**與相同內容區塊跨頁撞上相同計數器值而洩漏明文相等性！
      - 【課本補充（p.689–690）】一條 64B 快取行要切成 **4 個 16B 區塊**平行加密；為避免同一條快取行內兩個 16B 區塊內容相同卻拿到同一個 OTP，會先把上述 128-bit Seed 與 Enclave 建立時隨機產生的 **4 組 chunk 專屬 IV** 做 $\text{XOR}$，再各自送進 4 個平行 AES 引擎。
    - **AES 硬體秘密金鑰由三項晶片內秘密合成**：
      $$K_{\text{AES}} = f(\text{PUF}, \text{Boot-time Random Number}, \text{Enclave ID})$$
      綁定 **PUF** 確保搬去別顆 CPU 解不開；綁定**開機亂數**確保重開機跑同一支程式密文全變；綁定單調遞增的 **Enclave ID** 確保同一機器上不同 Enclave 彼此無法偷讀。
- `[51:15]` **關鍵效能創舉：在 LLC Miss 陰影下隱藏解密延遲（Shadow of an LLC Miss，p.690）**：
  - **LLC 讀取未命中流程**：
    1. 發出兩筆記憶體請求讀取 64B 密文與其 MAC（⚠️ 影片 `[52:30]` 一度口頭說 128-bit MAC，但課本 p.690 與影片後半 `[76:04]` 統一為 **64-bit / 8-byte Carter–Wegman MAC**），等待 DRAM 回傳需 **100–400 CPU cycles**。
    2. 在這漫長的等待期間（**shadow of the miss**），MEE 早就從 Counter Cache 讀出計數器並用 AES 算好 64B OTP！
    3. 密文一抵達晶片，只需 **1 拍 `XOR`** 就能還原明文並**投機地直接送進 LLC／CPU（不在關鍵路徑上乾等 MAC 驗完！）**；MEE 背景同步算 Hash 並核對 MAC——為何敢投機送資料？因為一旦 MAC 不符代表遭受主動硬體攻擊，屬於**災難性安全事件（catastrophic event）**，處理器會直接停機／中止 Enclave 並丟棄所有揮發狀態，完全不怕錯誤資料被外部觀測到。
  - **LLC 髒行逐出（Evict）與 Minor Counter 溢位邊界情況（`[55:51]`，p.690）**：
    - 每次髒行寫回 DRAM：對應的 6-bit Minor Counter $+1 \to$ 算新 OTP 加密 64B 資料並產生新 MAC 寫回。
    - **當某個 Minor Counter 從 `63`（$2^6-1$）溢位歸零怎麼辦？** 絕不能直接繞回 `0` 重用舊計數器！此時必須將**整頁的 Major Counter 加 1、頁內全部 64 個 Minor Counters 歸零**，並把該 4KB 頁目前不在晶片內快取、而躺在 DRAM 裡的所有區塊（影片舉例 20 塊在 DRAM、44 塊在晶片快取，則只需處理那 20 塊）**全部讀回、用新計數器重新加密再寫回 DRAM**——代價昂貴，所幸每寫回同一行 64 次才觸發一次。

---

## 5. 保護計數器本身：傳統 8 元 Merkle Tree vs 64 元計數器樹（Bonsai Merkle Tree）

- `[60:25]` **把安全問題收斂成一句話：「只要保護好計數器，ACIF 就萬無一失！」（Important Point 21，p.690）**：
  - 攻擊者沒有晶片內的 $K_{\text{AES}}$，單改密文或單改 MAC 必被抓包；把別處位址的 `⟨密文, MAC⟩` 搬過來會因 `Block Address` 不符失敗；只重播舊的 `⟨密文, MAC⟩` 會因晶片記住最新計數器而失敗。
  - **唯一的破口（Triple Replay，`[64:51]`）**：Counter Cache 容量有限，計數器本身也必須逐出到不可信的 DRAM！如果攻擊者把過去某個時間點的 **`⟨舊密文, 舊 MAC, 舊計數器⟩` 三者一起在 DRAM 裡回滾覆寫**，MEE 讀進舊計數器一算，舊密文與舊 MAC 就會完美吻合！因此「保護龐大資料」的問題被濃縮成「**如何保護體積小得多的計數器不被竄改或重播**」。
- `[71:56]` **傳統做法：8 元 Merkle Tree（課本 §13.3.3 Figure 13.10，p.691）**：
  - 葉節點是各個 4KB 頁的 448-bit 計數器行；每個內部父節點儲存 $k$ 個子節點內容的 64-bit（8-byte）雜湊，層層往上直到**樹根（Root）鎖死在晶片內 MEE 暫存器（永不離開 TCB）**。
  - **重要快取最佳化（`[75:49]`）**：內部節點也可快取在晶片內；讀取驗證或寫入更新往上爬樹時，**只要碰到任何一個已經躺在晶片內快取的祖先節點即可立即停止**（因為晶片內 TCB 保證其正確無誤），不必每次都爬到樹根。
  - **致命瓶頸——分支度（Arity）只有 8**：一條 64B 快取行只能塞 $64 / 8 = \mathbf{8}$ 個 8-byte 子雜湊（8-ary tree）。以 **1 GB 安全記憶體**（含 $2^{30} / 2^{12} = 2^{18} = 262,144$ 個 4KB 頁）為例，需要 $\log_8(2^{18}) = 6$ 層內部雜湊節點外加 1 層葉節點，共 **7 層**！樹太深導致每次 Counter Cache Miss 要連讀好幾次 DRAM。
- `[77:08]` **顛覆性改良：64 元對稱計數器樹（Bonsai Merkle Tree，課本 Figure 13.11，p.691–692）**：
  - 還記得前面每條 512-bit 葉節點計數器行（`64b Major + 64 × 6b Minor = 448b`）還空著 **64 bits** 嗎？不能只塞自己的純 Hash（那樣切斷了父子連結，擋不住整行回滾），而是塞入**由父節點計數器加密保護的 64-bit Carter–Wegman MAC**！

    ```
    父節點（Level i−1，512 bits）：[ Major_P (64b) | Minor_P[0..63] (64×6b = 384b) | MAC_P (64b) ]
                                           │               │（取第 c 個 6-bit minor counter）
                                           └───────┬───────┘
                                                   ▼ 產生 OTP 加密子節點 Hash！
    子節點（Level i，  512 bits）：[ Major_C (64b) | Minor_C[0..63] (64×6b = 384b) | MAC_C (64b) ]
                                   └───────────────┴── 先對這 448 bits 算 64b Hash ──► ⊕ OTP 存入 MAC_C！
    ```

  - **兩大絕妙設計（`[79:48]`）**：
    1. **全樹節點結構完全統一，分支度從 8 暴增為 64（64-ary tree）**：父節點的 64 個 6-bit Minor Counters 剛好對應 **64 個子節點**！每次子節點更新逐出時，父節點對應的 Minor Counter 就 $+1$ 並重算子節點 MAC。同樣 **1 GB 安全記憶體**，樹高從 7 層驟降為 $\log_{64}(2^{18}) + 1 = 3 + 1 = \mathbf{4\text{ 層}}$（含葉節點）！
    2. **MAC 就地嵌在節點尾端閒置的 64 bits**：讀一條 64B 計數器快取行同時拿到計數器與其 MAC，不必像資料塊那樣發兩筆獨立記憶體請求。（【課本補充 p.692】靠近樹根的高層節點更新較頻繁、Minor Counter 較易溢位，實作上可在高層酌情調降分支度）。

---

## 6. 【原始碼】Linux `fs/verity/verify.c` 如何在受信任快取節點提前終止 Merkle Tree 驗證

本講 `[75:49]` 與課本 p.691 強調 Merkle Tree 在硬體／系統實作中最關鍵的效能技巧：**根雜湊鎖在 TCB 內，而爬樹驗證只要碰到已在受信任快取中驗證過的內部節點就立刻停下，不必爬到底**。在 Linux v6.2.12 的 `fs-verity` 子系統（`fs/verity/verify.c`）中，核心正是用一模一樣的演算法逐頁保護唯讀檔案不受底層儲存媒體竄改：

```c
/* fs/verity/verify.c:96-129 — 從葉節點往上爬 Merkle Tree，遇已驗證快取頁（PageChecked）立即停下！ */
	/*
	 * Starting at the leaf level, ascend the tree saving hash pages along
	 * the way until we find a verified hash page, indicated by PageChecked;
	 * or until we reach the root.
	 */
	for (level = 0; level < params->num_levels; level++) {
		pgoff_t hindex;
		unsigned int hoffset;
		struct page *hpage;

		hash_at_level(params, index, level, &hindex, &hoffset);
		hpage = inode->i_sb->s_vop->read_merkle_tree_page(inode, hindex,
				level == 0 ? level0_ra_pages : 0);
		if (IS_ERR(hpage)) { ... goto out; }

		if (PageChecked(hpage)) {                 /* 命中受信任 Page Cache 中的已驗證內部節點！ */
			memcpy_from_page(_want_hash, hpage, hoffset, hsize);
			want_hash = _want_hash;
			put_page(hpage);
			goto descend;                     /* 不必繼續往上爬到 root_hash！ */
		}
		hpages[level] = hpage;
		hoffsets[level] = hoffset;
	}
	want_hash = vi->root_hash;                        /* 整條路徑都沒快取時，才由 TCB 內的 root_hash 保底 */
```

接著往下驗證剛讀進來的未快取內部節點，並標上 `SetPageChecked(hpage)` 讓後續兄弟節點直接受惠（`fs/verity/verify.c:138-157`）：

```c
/* fs/verity/verify.c:138-156 — 由上往下逐層核對雜湊，驗證通過即標記 PageChecked 納入受信任快取 */
descend:
	for (; level > 0; level--) {
		struct page *hpage = hpages[level - 1];
		unsigned int hoffset = hoffsets[level - 1];

		err = fsverity_hash_page(params, inode, req, hpage, real_hash);
		if (err) goto out;
		err = cmp_hashes(vi, want_hash, real_hash, index, level - 1);
		if (err) goto out;
		SetPageChecked(hpage);                    /* 標記為已驗證，下次爬樹碰到此節點即可提前終止 */
		memcpy_from_page(_want_hash, hpage, hoffset, hsize);
		want_hash = _want_hash;
		put_page(hpage);
	}
```

（另外 Linux 核心也內建了本講 §13.2.2 的硬體加速版 SHA-256 組語 `arch/x86/crypto/sha256_ni_asm.S:100-170`，用 `sha256rnds2` / `sha256msg1` / `sha256msg2` 指令執行 64 回合雜湊，我們直接在下方實測它的真實週期數！）

---

## 7. 【實測】SHA-NI 硬體加速、MEE 四種攻擊攔截矩陣與 1 GiB 記憶體 Bonsai 樹壓縮

我們在 [experiments/lecture53/merkle_and_shani.c](experiments/lecture53/merkle_and_shani.c) 實作三組實驗並於本機 i7-13650HX 單顆 P-core 執行：(1) 純軟體 64 回合 SHA-256 vs Intel 硬體 `SHA-NI` 指令集對單條 64B 快取行的雜湊延遲，以及 4 塊 16B CTR OTP 預計算所需的週期數；(2) 完整建構 Split-Counter MEE（`64b Major || 6b Minor || 58b BlockAddr`）與 Bonsai 父子計數器節點，實際注入 Spoofing、Splicing、Data+MAC Replay 與連同葉節點計數器一起回滾的 Triple Replay；(3) 精算 1 GiB 安全記憶體下傳統 8-ary Merkle Tree 與 64-ary Bonsai Tree 的層數、內部節點容量與 32 KiB 樹快取下的平均 DRAM 讀取次數。

### 實驗程式碼重點節錄

```c
/* experiments/lecture53/merkle_and_shani.c:139-160 — MEE 128b 計數器種子與 64B 快取行加密 + MAC */
static inline __m128i make_ctr_seed(uint64_t major, uint8_t minor6, uint64_t block_addr58) {
    uint64_t lo = (((uint64_t)(minor6 & 0x3fu)) << 58) | (block_addr58 & ((1ULL << 58) - 1ULL));
    return _mm_set_epi64x((long long)major, (long long)lo);
}
static void mee_encrypt_line(const uint8_t pt[64], uint8_t ct[64], uint64_t *mac_out,
                             uint64_t major, uint8_t minor6, uint64_t block_addr58) {
    __m128i base = make_ctr_seed(major, minor6, block_addr58);
    for (int q = 0; q < 4; q++) {
        __m128i chunk_iv = _mm_set1_epi32(0x13579BDF * (q + 1)); /* 區隔同一行內 4 個 16B chunk */
        __m128i otp = aes_ni_10(_mm_xor_si128(base, chunk_iv));
        _mm_storeu_si128((__m128i *)(ct + 16 * q),
                         _mm_xor_si128(_mm_loadu_si128((const __m128i *)(pt + 16 * q)), otp));
    }
    uint64_t h = hash64_ni(ct);
    __m128i mac_otp = aes_ni_10(_mm_xor_si128(base, _mm_set1_epi32(0xCAFEBABE)));
    *mac_out = h ^ (uint64_t)_mm_cvtsi128_si64(mac_otp);        /* Carter-Wegman 64-bit MAC */
}
```

### 執行指令與實測輸出

```bash
gcc -O3 -msha -maes -mssse3 -msse4.1 -Wall -Wextra computer-architecture/advanced/experiments/lecture53/merkle_and_shani.c -o /tmp/merkle_and_shani
taskset -c 0 /tmp/merkle_and_shani && rm -f /tmp/merkle_and_shani
```

```text
=== 1. SHA-256 Verification (SW vs Intel SHA-NI) & 64B Line Latency ===
NIST "abc" digest: ba7816bf8f01cfea...f20015ad (SW==SHA-NI: YES)
SW SHA-256 (64 rounds) :  694.8 TSC-cycles / 64B line
HW SHA-NI  (64 rounds) :  105.2 TSC-cycles / 64B line (6.61x faster)
4x16B CTR OTP precomp  :   16.7 TSC-cycles / 64B line (fits easily inside ~250-cycle DRAM miss shadow! sink=f)

=== 2. MEE ACIF Attack Verification Matrix ===
Legitimate latest read (v2, minor=2)                : PASS
Spoofing attack (1-bit flip in DRAM ciphertext)     : DETECTED (MAC mismatch)
Splicing attack (copy valid (CT,MAC) ADDR_C->ADDR_A): DETECTED (block_addr in OTP)
Data+MAC Replay (old v1 with protected minor=2)     : DETECTED (freshness fail)
Triple Replay (CT+MAC+LeafCtr) WITHOUT Merkle Tree  : BYPASSED (vulnerable!)
Triple Replay (CT+MAC+LeafCtr) WITH Bonsai Tree     : DETECTED (parent ctr MAC fail)

=== 3. 1 GiB Secure Memory (262,144 Pages): 8-ary vs 64-ary Bonsai Tree ===
Classical 8-ary Merkle : 7 levels |  37449 internal nodes ( 2340.6 KiB) | avg 2.086 DRAM tree reads/miss
64-ary Bonsai Merkle   : 4 levels |   4161 internal nodes (  260.1 KiB) | avg 1.145 DRAM tree reads/miss
Internal tree compression: 9.00x fewer internal nodes, 1.82x fewer DRAM tree reads!
```

### 實測觀察與課本對照

1. **「LLC Miss 陰影預計算」在真實硬體上綽綽有餘**：算完一條 64B 快取行所需的 4 組 16B AES-CTR OTP 僅花 **`16.7` TSC cycles**，遠小於課本說的 `100–400 cycles` DRAM 存取延遲——當 DRAM 資料抵達晶片時，OTP 早已躺在暫存器裡等著做 1 拍 `XOR`；且硬體 `SHA-NI` 將 64 回合 SHA-256 從純軟體 `694.8 cycles` 壓到 `105.2 cycles`（快 **`6.61x`**），背景驗 MAC 也極為輕快。
2. **親眼見證為何沒有 Merkle Tree 就會被 Triple Replay 攻破**：單改密文（Spoofing）、跨位址搬移合法密文對（Splicing）與只重送舊密文對（Replay）全被 MEE 擋下；但一旦攻擊者連 DRAM 裡的葉節點計數器一起回滾（`minor: 2 -> 1`），沒有樹保護時立刻出現 `BYPASSED (vulnerable!)`，加上 Bonsai 父節點計數器 MAC 後則當場 `DETECTED`！
3. **Bonsai 64 元樹對照課本 p.691–692 完全吻合**：同樣保護 1 GiB（$2^{18}$ 頁），傳統 8-ary 樹需 **7 層**、`37,449` 個內部節點（`2,340.6 KiB`）；64-ary Bonsai 樹僅需 **4 層**、`4,161` 個內部節點（**僅 `260.1 KiB`，縮小整整 `9.00x`**），連 32 KiB 樹快取幾乎都能裝下大宗熱點內部節點，使每次未命中的平均 DRAM 爬樹讀取減少近一半（`2.086 → 1.145` 次）！

---

## 8. 一頁速查表

| 機制／攻擊 | 核心設計與位元配置 | 防禦目標與效能代價 |
|---|---|---|
| **SHA-2 + Carter–Wegman MAC** | 512b 區塊跑 64 回合單向雜湊；$\text{MAC} = H(\text{Data}) \oplus \text{OTP}$ | 同時提供 **Integrity**（防竄改）與 **Authenticity**（無金鑰算不出合法 MAC），免除昂貴 RSA 簽章 |
| **Buffer Overflow 防線** | NX bit（資料頁禁執行）＋ ASLR（隨機化 stack/heap/code 基底） | NX 擋住堆疊 Code Injection；ASLR 大幅提高 ROP Code Reuse 猜位址難度 |
| **TCB / TEE / TPM** | 晶片內 TCB 建立 Enclave；TPM 算 Measurement 簽章做遠端證明與 Sealing | 將不可信的龐大 OS 排除在 TCB 之外，縮小 Attack Surface |
| **MEE Split-Counter CTR** | 每 4KB 頁：`64b Major + 64×6b Minor = 448b`；AES 輸入拼入 `58b BlockAddr` | 擋住 Cold Boot、竊聽、Splicing（靠位址）與一般 Replay（靠遞增計數器）；OTP 藏在 LLC miss 陰影下算完 |
| **Minor Counter 溢位** | 任一 6-bit Minor 達 63 再加 1 $\to$ `Major++`、全部 `Minor=0` | 必須把該 4KB 頁在 DRAM 中的所有區塊讀回重加密寫回（每行寫回 64 次才發生一次） |
| **8-ary vs 64-ary Bonsai Tree** | 利用計數器行剩餘 `64b` 存父節點計數器加密的 MAC，分支度 $8 \to 64$ | 防止計數器連同密文一起回滾；1 GB 記憶體樹高 $7 \to 4$ 層、內部節點縮小 $9\times$，遇晶片內快取節點即停 |

---

## 9. 與後續講次的連結

- **往回對照 [lecture52_note.md](lecture52_note.md)**：第 52 講埋下的「為何 CTR 模式是現代安全硬體基石」在本講完全兌現——正因為 CTR 的 OTP 不依賴明文，MEE 才能在等待 DRAM 回傳的 300 拍空檔把解密工作做完！
- **往下接 [lecture54_note.md](lecture54_note.md)（第 54 講：Enclave 軟體介面、旁道攻擊與 ORAM）**：本講結尾留下了兩個致命問題——(1) 既然分頁表是由「不可信的惡意 OS」管理，OS 會不會偷改分頁表把 Enclave 的安全虛擬頁指到非安全實體頁（Address Translation Attack）？這需要第 54 講的 `ECREATE`/`EADD`/`EINIT` 與反向分頁表（IPT / EPCM）；(2) 就算 DRAM 資料全部用 AES+Bonsai Tree 鎖得密不透風，**記憶體位址匯流排上的讀寫位址序列、快取命中時間差（Flush+Reload / Prime+Probe）與亂序投機執行（Spectre / Meltdown）** 仍會透過旁道把秘密漏光！

---

## 附：本講一句話總結

**安全處理器將信任邊界收縮至晶片封裝內，在 LLC 與不可信 DRAM 之間部署記憶體加密引擎（MEE），利用「Major+Minor 計數器拼實體位址」的 CTR 模式在快取未命中陰影下無延遲解密並搭配 MAC 防竄改與拼接，最後再以節點內嵌 64-bit MAC 的 64 元 Bonsai Merkle Tree 將全記憶體鮮度保證收斂至晶片內單一樹根暫存器。**

---

*筆記整理自 Smruti R. Sarangi 教授 Advanced Computer Architecture 第 53 講影片與 Next-Gen Computer Architecture v3.2 第 13 章 §13.2–13.3.3，搭配 Linux v6.2.12 原始碼與本機實測驗證。*
