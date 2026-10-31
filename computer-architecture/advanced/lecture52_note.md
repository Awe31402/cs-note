# Advanced Computer Architecture 第五十二支影片完整筆記：安全處理器架構（一）：密碼學基礎、AES 區塊加密、CTR 模式、RC4、RSA 與 Diffie–Hellman（Chapter 13, Part I）

> **課程**：Advanced Computer Architecture — Chapter 13:  Secure Processor Architectures (Part I) -- Cryptographic Fundamentals and Encryption（播放清單第 52 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：**whisper 轉錄**（本機 `large-v3-turbo`；YouTube 字幕下載不到）（1:10:14）＋ 課本 *Next-Gen Computer Architecture* v3.2 **Ch13** §13.1（p.671–680）
> **章號**：影片 Chapter 13 = 課本 Ch13。
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄
> - `【實測】`：在本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、`gcc` 11.4）實際執行的結果
> - `⚠️`：逐字稿缺字、或影片／課本／實測說法不一致之處
> - `[mm:ss]`：影片時間點

⚠️ whisper 把「encrypted text」寫成「encrypted test」、「XOR／XORed」寫成「bitwise or／solve／zor／zord／as OR」、「plaintext」寫成「clean text」、「avatars」寫成「avtars」、「OFB mode」寫成「OFP mode」、「pseudo-random」寫成「sudo random」、「Rivest, Shamir, and Adleman」寫成「river shamir and adelman」、「RSA」偶爾寫成「RAC」、Diffie–Hellman 的「$\bmod\ p$」全部聽成「mod B」、「pre-sharing」寫成「three sharing」，筆記直接改正；另 `[70:11]–[70:18]` 為片尾音樂期間 whisper 幻覺出的亂碼詞句，予以略過。

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. 安全處理器導論：機密性、完整性與混淆／擴散（Confusion & Diffusion）](#1-安全處理器導論機密性完整性與混淆擴散confusion--diffusion)
- [2. AES 區塊加密演算法：狀態矩陣、四種回合運算與金鑰排程](#2-aes-區塊加密演算法狀態矩陣四種回合運算與金鑰排程)
- [3. 長訊息加密模式比較：ECB、CBC、OFB 與硬體首選的 Counter Mode（CTR）](#3-長訊息加密模式比較ecbcbcofb-與硬體首選的-counter-modectr)
- [4. RC4 串流加密與硬體管線化對比（AES vs RC4）](#4-rc4-串流加密與硬體管線化對比aes-vs-rc4)
- [5. 非對稱式密碼學：RSA 協定、數位簽章與物理不可複製函數（PUF）](#5-非對稱式密碼學rsa-協定數位簽章與物理不可複製函數puf)
- [6. 工作階段金鑰（Session Keys）與 Diffie–Hellman 金鑰交換](#6-工作階段金鑰session-keys與-diffiehellman-金鑰交換)
- [7. 【原始碼】Linux x86 `aesni-intel_asm.S` 如何用四路交錯餵滿 AES-NI 硬體管線](#7-原始碼linux-x86-aesni-intel_asms-如何用四路交錯餵滿-aes-ni-硬體管線)
- [8. 【實測】混淆／擴散雪崩效應、CBC vs CTR 錯誤傳播與單核 10 GiB/s AES-NI 管線加速](#8-實測混淆擴散雪崩效應cbc-vs-ctr-錯誤傳播與單核-10-gibs-aes-ni-管線加速)
- [9. 一頁速查表](#9-一頁速查表)
- [10. 與後續講次的連結](#10-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

```
前 12 章假設硬體只是「偶爾出錯但本性誠實」；第 13 章起假設存在蓄意竊聽或竄改的攻擊者
  │
  ├── 核心安全目標（Definition 100–101）
  │   ├── Confidentiality（機密性，防竊聽）vs Integrity（完整性，防竄改）＋ Authenticity（真實性，防冒充）
  │   └── 密碼品質指標：Confusion（改 1 bit Key → 密文大改，切斷 Key–Ciphertext 統計相關）
  │                     Diffusion（改 1 bit Plaintext → 統計上約 50% 密文位元翻轉，切斷明密文相關）
  │
  ├── 對稱式加密（Symmetric Ciphers：加解密共用同一把秘密金鑰，硬體執行速度快）
  │   ├── 區塊加密 AES（§13.1.1）：16B 排成 4×4 狀態矩陣；128/192/256-bit 金鑰分別跑 10/12/14 回合
  │   │     • 回合運算：SubBytes（S-box 查表）→ ShiftRows（列左旋）→ MixColumns（GF(2⁸) 矩陣乘）→ AddRoundKey（XOR）
  │   │     • 完整流程：第 0 回合僅 AddRoundKey（Pre-whitening）；第 1..N−1 回合四步全做；第 N 回合跳過 MixColumns
  │   │     • 長訊息模式：ECB（相同明文同密文，擴散低）→ CBC（加密循序、解密平行、單 bit 錯毀後續）
  │   │                   → OFB（加解密皆循序、錯誤隔離）→ CTR（Major+Minor 計數器產生 OTP：加解密皆平行＋錯誤隔離！）
  │   ├── 串流加密 RC4（§13.1.2）：256-byte 狀態陣列 S 先以 Key 洗牌（KSA），再逐 byte 交換輸出 keystream XOR 明文
  │   └── 硬體實作（§13.1.3）：AES 各回合結構一致，極易展開並管線化（pipelined）；RC4 強狀態相依，本質循序難管線化
  │
  └── 非對稱式加密與金鑰分配（§13.1.4–13.1.5）
      ├── RSA：公開金鑰 (n, e) 與私密金鑰 (n, d) 滿足 E_K(D_K(m)) = D_K(E_K(m)) = m（可交換性）
      │        支援公開金鑰加密與私鑰「數位簽章」（Authenticity），但大數模指數比 AES 慢 1–2 個數量級以上
      └── Session Key 啟動：開機／連線初期用 RSA 或 Diffie–Hellman（qᵃ mod p, qᵇ mod p → qᵃᵇ mod p）協商共享金鑰，
                            後續執行期全部交給高速硬體 AES（如 CTR 模式記憶體加密）處理
```

---

## 1. 安全處理器導論：機密性、完整性與混淆／擴散（Confusion & Diffusion）

- `[00:00]` **為什麼計算機結構需要談安全？**
  - 前十二章（例如 [lecture49_note.md](lecture49_note.md) 到 [lecture51_note.md](lecture51_note.md)）假設所有硬體元件皆可信任——它們可能因宇宙射線或老化而出錯，但不會「蓄意」注入故障或偷竊密碼與信用卡號。現代系統（雲端多租戶、外包 SoC IP、實體接觸 DRAM 匯流排的攻擊者）打破了這項假設。
  - 【口頭】本章只需要第 4 章亂序管線（issue / execute / commit，攸關第 54 講 Spectre/Meltdown）、快取與記憶體階層，以及鏈結串列與樹（Merkle tree）的基礎，不需要先修密碼學。全章分為四塊：(1) 資料加密、(2) 雜湊與資料完整性、(3) 安全處理器架構、(4) 旁道攻擊（side-channel attacks）。
- `[01:26]` **兩大基本安全屬性（課本 Definition 100，p.671）**：
  1. **機密性（Confidentiality）**：Alice 傳給 Bob 的明文（**plaintext**，注意英文連寫無空白）經加密盒轉成密文（**ciphertext**）；竊聽者（eavesdropper）即使攔截密文，也無法推回原始資料。
  2. **完整性（Integrity）**：防止中間人竄改（tampering）；無論傳送 100 B 或 100 GB，只要任何 1 個位元被翻轉，接收端都能察覺。
- `[03:27]` **克勞德·夏農的兩大密碼設計準則（課本 Definition 101，p.672）**：
  - 演算法本身通常公開（Kerckhoffs 原則），真正保密的是參數化加密行為的**金鑰（Key，典型為 128–256 bits）**。一個好的加密器必須滿足：
    1. **混淆（Confusion，Key $\leftrightarrow$ Ciphertext 關係）**：只改動 **Key 的 1 個位元**，絕大多數（統計上約一半）密文位元都會改變。這消除了金鑰與密文之間的統計相關性；【口頭】就算同一份明文在兩把不同金鑰下碰巧產生相似密文，攻擊者也無法據此推論兩把金鑰相近。
    2. **擴散（Diffusion，Plaintext $\leftrightarrow$ Ciphertext 關係）**：只改動 **Plaintext 的 1 個位元**，統計上應有**一半（50%）密文位元翻轉**（反之改密文 1 bit 解密後亦影響約半數明文位元）。這確保微小的明文差異會造成密文雪崩式巨變，攻擊者無法從兩段密文的相似度推知明文結構。
- `[08:08]` **區塊加密（Block Cipher）vs 串流加密（Stream Cipher）**：
  - **Block cipher**：把長訊息切成固定大小區塊（影片說 128–256 bits，課本 p.672 寫泛指區塊密碼可為 128–1024 bits），逐塊加密，通常**易於平行化**（例如 AES）。
  - **Stream cipher**：把金鑰當作偽亂數產生器（PRNG）的種子，產生無限長的偽亂數序列（keystream），第 $n$ 個明文 byte 直接與第 $n$ 個亂數 byte 做 $\text{XOR}$ 得到密文；解密時用同種子產生同序列再 $\text{XOR}$ 一次即可還原，但本質偏向**循序（sequential）**（例如 RC4）。

---

## 2. AES 區塊加密演算法：狀態矩陣、四種回合運算與金鑰排程

- `[09:55]` **AES（Advanced Encryption Standard）規格（課本 Table 13.1，p.672）**：
  - 支援三種金鑰長度，對應不同的加密回合數（rounds）：

    | 金鑰長度（Key size） | 回合數 $N$（Rounds） | 區塊大小（Block size） |
    |---|---|---|
    | 128 bits（16 B，本講範例） | 10 | 128 bits（16 B） |
    | 192 bits（24 B） | 12 | 128 bits（16 B） |
    | 256 bits（32 B） | 14 | 128 bits（16 B） |

  - ⚠️ **區塊大小勘誤**：影片 `[10:17]` 口誤說「把明文切成與金鑰等長的區塊」，課本 p.674 也寫「An AES block can only encrypt 128 to 256 bits」。在 NIST FIPS-197 標準中，雖然底層 Rijndael 演算法家族支援可變區塊長度，但正式標準化的 **AES 不論金鑰是 128、192 或 256 bits，區塊大小一律固定為 128 bits（16 bytes）**。
- `[11:04]` **狀態矩陣（State Matrix）**：
  - 將 16 bytes 明文 $B_0, B_1, \dots, B_{15}$ 以**行優先（column-major）**排成 $4 \times 4$ 矩陣，稱為演算法的 **state**：
    $$\text{State} = \begin{bmatrix} B_0 & B_4 & B_8 & B_{12} \\ B_1 & B_5 & B_9 & B_{13} \\ B_2 & B_6 & B_{10} & B_{14} \\ B_3 & B_7 & B_{11} & B_{15} \end{bmatrix}$$
- `[12:25]` **每回合的四個基本步驟（課本 p.673）**：
  1. **`SubBytes`（位元組代換）**：讀取每個位元組 $B_i$（$0\dots 255$），查一張 256 格的 **S-box（Substitution box）** 替換成 $C_i$。【口頭】若只做位元搬移，全 `0` 明文搬來搬去還是全 `0`；S-box 透過代數多項式反元素映射（執行期只需查表）打破線性結構並拉開明密文距離。
  2. **`ShiftRows`（列移位／左旋）**：第 $i$ 列（$i \in \{0, 1, 2, 3\}$）向左循環旋轉（left rotate）$i$ 個位元組（例如第 2 列左旋 2 格：原本前兩格繞到最右邊）。
  3. **`MixColumns`（行混合）**：將每一行（column）的 4 個位元組視為向量，在有限體 $\text{GF}(2^8)$ 下乘上一個固定 $4\times 4$ 矩陣產生新行。`ShiftRows`（橫向）搭配 `MixColumns`（縱向）專職**擴散（diffusion）**位元。
  4. **`AddRoundKey`（加入回合金鑰）**：將整個 128-bit 狀態矩陣與當前回合專屬的 128-bit **round key** 做逐位元 $\text{XOR}$，把金鑰混入資料。
- `[18:42]` **金鑰排程（Key Schedule，課本 Eq. 13.1，p.673）**：
  - 定義作用於 4-byte word（$B_0 B_1 B_2 B_3$）的三個基本算子：
    - `RotWord`（記為 $R$）：左旋一格 $B_0 B_1 B_2 B_3 \to B_1 B_2 B_3 B_0$。
    - `SubWord`（記為 $S$）：對 4 個 byte 分別套用 S-box 查表。
    - `XORWord`（記為 $X$）：將第一個位元組改為 $B_0 \oplus RC[i]$，其中回合常數 $RC[1]=1$，後續每回合乘 2。
      - ⚠️ **有限體細節**：影片與課本寫 $RC[i] = 2 \times RC[i-1]$；此乘法定義在 $\text{GF}(2^8)$ 不可約多項式 $x^8+x^4+x^3+x+1$（`0x11b`）之下，因此 $i=1\dots 8$ 為 `0x01..0x80`，但 $i=9, 10$ 會模簡化為 `0x1b, 0x36` 而非整數溢位成 `0x100`。
  - 把原始 16-byte 金鑰排成第 0 回合的 $4\times 4$ 金鑰矩陣 $K^0$，以 $K_j^i$ 表示第 $i$ 回合（$1 \le i \le 10$）第 $j$ 個位元組（$0 \le j \le 15$）：
    $$K_j^i = \begin{cases} X\!\left(S\!\left(R\!\left(K_{12}^{i-1} K_{13}^{i-1} K_{14}^{i-1} K_{15}^{i-1}\right)\right)\right) \oplus K_0^{i-1} K_1^{i-1} K_2^{i-1} K_3^{i-1}, & j \in \{0,1,2,3\} \text{（首行）} \\ K_j^{i-1} \oplus K_{j-4}^i, & 4 \le j \le 15 \text{（其餘三行）} \end{cases}$$
    - ⚠️ 影片 `[24:36]` 口頭把 $\oplus$ 唸成「plus」，實為逐位元 $\text{XOR}$（即 $\text{GF}(2)$ 加法）；且 Eq. 13.1 專指 AES-128（AES-192/256 的矩陣寬度不同，排程略有差異）。
- `[27:16]` **完整 AES 流程與解密（課本 p.674）**：
  - **第 0 回合（Pre-whitening）**：只做 `AddRoundKey`（明文直接 $\oplus K^0$）。
  - **第 $1 \dots N-1$ 回合**（AES-128 為第 1–9 回合）：依序執行 `SubBytes` $\to$ `ShiftRows` $\to$ `MixColumns` $\to$ `AddRoundKey`。
  - **第 $N$ 回合（最後一回合）**：**跳過 `MixColumns`**，只做 `SubBytes` $\to$ `ShiftRows` $\to$ `AddRoundKey`，最終 state 即為 16-byte 密文。
  - 【課本補充】AES 每一步皆可逆；解密時只需先展開並存好所有回合金鑰，再從最終狀態反向執行逆運算即可。

---

## 3. 長訊息加密模式比較：ECB、CBC、OFB 與硬體首選的 Counter Mode（CTR）

當資料長達數 MB 甚至整個 DRAM 容量時，必須將明文切成多個 16-byte 區塊，選擇適當的**加密模式（Encryption Modes，課本 §13.1.1 p.674–676）**：

```
(1) ECB：PT_k ──► [AES_K] ──► CT_k                     （全平行，但 PT_i = PT_j ⇒ CT_i = CT_j！）
(2) CBC：PT_k ──► (⊕ CT_{k-1}) ──► [AES_K] ──► CT_k    （加密串列、解密平行；單 bit 明文錯毀掉後面所有區塊）
(3) OFB：OTP_k = AES_K(OTP_{k-1})；CT_k = PT_k ⊕ OTP_k （加解密皆串列；單 bit 錯不擴散、可平行做明文 ECC）
(4) CTR：OTP_k = AES_K(Major || Minor+k)；CT_k = PT_k ⊕ OTP_k （加解密全平行＋單 bit 錯隔離＋預先算 OTP！）
```

- `[28:41]` **Electronic Codebook（ECB）**：每塊獨立用同一把 Key 加密。混淆度高，但**擴散度極低**——只要兩塊明文相同，密文就一模一樣（著名例子是加密點陣圖仍看得出輪廓），洩漏結構資訊。
- `[30:47]` **Cipher Block Chaining（CBC，課本 Figure 13.1）**：
  - 第一塊先與公開的初始向量 **IV（Initialization Vector）** 做 $\text{XOR}$ 再進 AES；之後每一塊明文都先與**前一塊密文**做 $\text{XOR}$ 再加密：$C_k = \text{AES}_K(P_k \oplus C_{k-1})$。
  - **優缺點**：擴散極強（改 1 bit 明文，後續所有密文塊全變）；**解密可平行**（因為 $P_k = \text{AES}_K^{-1}(C_k) \oplus C_{k-1}$，所有 $C_k$ 都已在手邊！），但**加密嚴格循序**，且明文若出現 1-bit 錯誤會毀掉後面整份密文。
- `[33:21]` **Output Feedback Mode（OFB，課本 Figure 13.2）**：
  - 不直接把明文送進 AES，而是把 `IV` 反覆送進 AES 串接加密產生一系列 16-byte **一次性密碼本（OTP, One-Time Pad）**：$O_1 = \text{AES}_K(\text{IV}),\ O_k = \text{AES}_K(O_{k-1})$，最後才用極快的 $C_k = P_k \oplus O_k$ 產生密文。
  - **優缺點**：明文 1-bit 錯誤只影響對應密文的 1 個 bit（刻意壓低跨區塊擴散以換取容錯），且產生 OTP 的同時可平行檢查／修正明文錯誤；但致命傷是 $O_k$ 仰賴 $O_{k-1}$，導致**加密與解密都變成循序運算**。
- `[36:29]` **Counter Mode（CTR，課本 Figure 13.3）——現代安全處理器架構的基石**：
  - 徹底拔除區塊間的串聯鏈，改將每個 AES 區塊的輸入設為 **`(Major Counter, Minor Counter + k)`**：
    - **Major counter**：對應整項加密任務（或整個記憶體頁／快取行世代），所有區塊共用同一個值。
    - **Minor counter**：依區塊位置設為 $0, 1, 2, \dots, k-1$，執行期直接由區塊位移算出，**不必額外儲存**。
  - AES 用硬體秘密金鑰加密 `(Major || Minor + k)` 產生 $\text{OTP}_k$，再與明文做 $\text{XOR}$：
    $$\text{OTP}_k = \text{AES}_K(\text{Major} \,\|\, (\text{Minor} + k)), \qquad C_k = P_k \oplus \text{OTP}_k$$
  - **四大架構優勢**：
    1. **加解密完全平行**：各區塊毫無相依性。
    2. **解決相同明文洩漏**：即便 $P_i = P_j$，因 minor counter 不同，AES 混淆／擴散保證 $\text{OTP}_i \neq \text{OTP}_j$，密文截然不同。
    3. **錯誤隔離與平行糾錯**：明文不在 AES 關鍵路徑上，可在算 OTP 的同時平行做 ECC（這也是下一講記憶體讀取「邊讀 DRAM 邊算 OTP 隱藏解密延遲」的關鍵！）。
    4. **只需儲存一個 Major Counter**，大幅節省 metadata 空間。
  - ⚠️ **Major Counter 需要保密嗎？** 課本 p.675 寫「The value of the major counter should remain confidential」，但影片 `[36:48]` 只說它是隨機產生的任務編號。在標準密碼學與第 53 講 Intel SGX／記憶體加密實作中，真正保密的是晶片內的 **AES Key**，而 `(Major, Minor)` 計數器通常以**明文**存在晶片外記憶體／counter tree 中（否則讀計數器又要先解密會陷入雞生蛋死結）；CTR 模式真正的鐵律是**同一把 Key 下絕不可重用相同的 `(Major, Minor)` 配對（Freshness / Nonce uniqueness）**，一旦重用會導致 $C_1 \oplus C_2 = P_1 \oplus P_2$ 直接洩漏明文差！

| 模式 | 相同明文塊密文不同？ | 加密可平行？ | 解密可平行？ | 明文 1-bit 錯影響範圍 |
|---|---|---|---|---|
| **ECB** | 否（洩漏結構） | 是 | 是 | 單一 16B 區塊 |
| **CBC** | 是 | **否（循序）** | 是 | 當前與後續所有區塊 |
| **OFB** | 是 | **否（OTP 串列）** | **否（OTP 串列）** | 僅對應 1 bit |
| **CTR** | 是 | **是** | **是** | 僅對應 1 bit |

---

## 4. RC4 串流加密與硬體管線化對比（AES vs RC4）

- `[39:15]` **RC4（Rivest Cipher 4）串流加密（課本 §13.1.2，p.676–677）**：
  - 適用於事先不知道訊息長度、逐 byte 到達的場景（如早期無線通訊）。核心資料結構為 **256-byte 狀態陣列 `S[0..255]`**，金鑰長度可變（5–16 bytes）。
  - **階段一：初始化（KSA）**——⚠️ 影片 `[41:13]` 口頭說以已知 IV 初始化，課本 p.676 給出精確定義 `S[i] = i`（$0 \le i \le 255$），接著用金鑰打亂排列（只做交換 permutation，不改數值集合）：
    ```c
    j = 0;
    for (i = 0; i <= 255; i++) {
        j = (j + S[i] + key[i % key_length]) % 256; // 注入 Key 增加 Confusion
        swap(S[i], S[j]);                           // 交換位置增加 Diffusion
    }
    ```
  - **階段二：偽亂數產生（PRGA）**——此階段不再讀 Key，每輪更新指標 `i, j`、交換 `S[i], S[j]`，並輸出第三個位置 `S[(S[i] + S[j]) % 256]` 作為 1-byte OTP 與明文 $\text{XOR}$（⚠️ 課本 p.677 第 5 行印漏右括號寫成 `swap(S[i], S[j];`，應補上 `)`）：
    ```c
    i = 0; j = 0;
    while (1) {
        i = (i + 1) % 256;
        j = (j + S[i]) % 256;
        swap(S[i], S[j]);
        output(S[(S[i] + S[j]) % 256]);
    }
    ```
- `[46:42]` **硬體實作與管線化比較（課本 §13.1.3，Figure 13.4，p.677–678）**：
  - **AES 天生適合硬體管線化**：128-bit 區塊的 10 個回合結構高度一致，硬體直接將迴圈展開（loop unrolling）切成 5 級管線：`[Init] → [3 Rounds] → [3 Rounds] → [3 Rounds] → [Last Round (無 MixColumns)]`。搭配 CTR 模式時，不同區塊毫無相依性，每一拍都能推進一個 128-bit 區塊，吞吐量極高。
  - **RC4 本質抗拒管線化**：每產生 **1 byte** 都必須依序完成「讀 `S[i], S[j]` $\to$ 寫回 `swap` $\to$ 算 `S[i]+S[j]` 再讀 `S`」，下一輪的位址 `j` 完全取決於上一輪剛交換完的陣列狀態，無法跨 byte 管線重疊（課本補充：僅在極低功耗 IoT 場景才會研究輕量化串流／區塊密碼；且現代網路協定已淘汰有統計偏差的 RC4）。

---

## 5. 非對稱式密碼學：RSA 協定、數位簽章與物理不可複製函數（PUF）

- `[48:36]` **對稱 vs 非對稱加密（課本 §13.1.4，p.678–679）**：
  - AES 與 RC4 為**對稱式密碼（Symmetric ciphers）**，收發雙方必須事先共享同一把私密金鑰；**RSA（Rivest–Shamir–Adleman）** 則是**非對稱式密碼（Asymmetric cipher）**，擁有一對公開金鑰（Public Key）與私密金鑰（Private Key）。
- `[50:25]` **RSA 金鑰產生與加解密範例（課本 p.678）**：
  1. 選兩個相異質數 $p = 59,\ q = 67$（實務上為數百位十進位大質數），計算模數 $n = p \cdot q = 3953$。
  2. 計算卡邁克爾函數 $\lambda(n) = \text{lcm}(p-1, q-1) = \text{lcm}(58, 66) = 1914$。
  3. 選與 $\lambda(n)$ 互質（coprime）的整數 $e < \lambda(n)$，此處取 $e = 31$。
  4. 求模反元素 $d$ 使得 $d \times e \equiv 1 \pmod{1914}$，得 $d = 247$（因 $247 \times 31 = 7657 = 4 \times 1914 + 1$）。
  5. **公開金鑰（Public Key）** $= (n, e) = (3953, 31)$，公開加密運算 $E_K(m) = m^e \bmod n$。
  6. **私密金鑰（Private Key）** $= (n, d) = (3953, 247)$，私鑰解密運算 $D_K(m) = m^d \bmod n$。
     - ⚠️ **投影片與課本筆誤**：影片 `[53:35]` 老師當場發現投影片把私鑰寫成 $(n, e)$ 並口頭更正為 $(n, d)$，但課本 p.678 第 6 點仍誤印為 `The private key is (n = 3953, e = 247)`，正確應為 `d = 247`。
- `[55:15]` **RSA 的核心靈魂——可交換性（Commutativity，Eq. 13.2–13.3）**：
  - 在數學上，RSA 並沒有兩套不同演算法，「解密」其實就是「用私鑰做模指數運算」，且兩者**可交換**：
    $$D_K(E_K(m)) = m = E_K(D_K(m))$$
  - 以明文 $m = 54$ 驗證：
    - **保密傳輸（公鑰加密 $\to$ 私鑰解密）**：$54^{31} \bmod 3953 = 3574$，收件人用私鑰算 $3574^{247} \bmod 3953 = 54$（【口頭】如同瀏覽器用 Gmail 公開金鑰加密信件，只有握有私鑰的伺服器解得開）。
    - **數位簽章（Digital Signature，私鑰簽署 $\to$ 公鑰驗證，`[58:04]`）**：Alice 用只有自己擁有的私鑰加密一段雙邊約定格式的已知訊息（$54^{247} \bmod 3953 = 1011$）；全世界任何人都能拿 Alice 的公鑰算 $1011^{31} \bmod 3953 = 54$，一旦解出預期內容，就證明發送者確實是 Alice 本人——這項確立發送者身分的性質稱為**真實性（Authenticity）**。
- 【課本補充】**硬體身分認證：物理不可複製函數（PUF, Physically Unclonable Function，p.679）**：
  - 呼應第 51 講（[lecture51_note.md](lecture51_note.md)）的**製程變異（Process Variation）**：每一顆製造出來的晶片，其電晶體漏電流、溫度或延遲分布都帶有獨一無二的微觀物理偏差。晶片內可部署感測器量測這些參數，提煉出穩定的 32–128 bit 硬體指紋（PUF），用於裝置認證、軟體授權綁定與安全通訊。

---

## 6. 工作階段金鑰（Session Keys）與 Diffie–Hellman 金鑰交換

- `[63:20]` **為什麼 CPU 硬體內部不用 RSA 直接加密資料？（課本 §13.1.5，p.679–680）**
  - RSA 需要對幾千位元的大整數做模指數運算（modular exponentiation），速度比硬體管線化 AES **慢 1 到 2 個數量級以上**（實務 2048-bit RSA 甚至慢上數千倍），完全無法承受每個記憶體讀寫或內部匯流排封包的延遲要求。
  - **解法：兩階段混合架構（Session Key）**——在開機（boot time）或連線建立初期，才用較慢的信任網路或 RSA 驗證身分並協商出一把生命週期僅限本次開機／連線的**工作階段金鑰（Session Key）**，隨後所有高速資料流一律改由 **AES** 加密（特別是多廠商 IP 組成的現代 SoC 內部安全通道）。
- `[65:55]` **免傳送秘密的金鑰協商：Diffie–Hellman Key Exchange（課本 p.680）**：
  - 若 Alice 與 Bob（例如兩顆硬體模組在出廠製造時）已預先共享兩個公開質數 $p$ 與 $q$，即使通道上有被動竊聽者，也能不靠沉重的 RSA 憑證體系，直接算出共享 Session Key：
    1. Alice 產生私密亂數 $a$，計算公開值 $A = q^a \bmod p$ 傳給 Bob。
    2. Bob 產生私密亂數 $b$，計算公開值 $B = q^b \bmod p$ 傳給 Alice。
    3. Alice 收到 $B$ 後計算 $K = B^a \bmod p = (q^b)^a \bmod p = q^{ab} \bmod p$。
    4. Bob 收到 $A$ 後計算 $K = A^b \bmod p = (q^a)^b \bmod p = q^{ab} \bmod p$。
  - 通道上只出現過 $p, q, A, B$；竊聽者在離散對數難題保護下無法從 $A, B$ 算出 $q^{ab} \bmod p$（⚠️ 前提是竊聽者僅被動偷聽；若遇到主動竄改的 Man-in-the-Middle，就是下一講 §13.2.1 要處理的問題！）。

---

## 7. 【原始碼】Linux x86 `aesni-intel_asm.S` 如何用四路交錯餵滿 AES-NI 硬體管線

課本 §13.1.1 與 §13.1.3 指出：AES-128 由「第 0 回合 `XOR` $\to$ 9 回合四步全做 $\to$ 第 10 回合跳過 `MixColumns`」組成，且硬體一定做成多級管線；但 CBC 加密因區塊間循序相依無法填滿管線，CTR（或 CBC 解密）則能同時把多個獨立區塊塞進管線。打開 Linux v6.2.12 的 `arch/x86/crypto/aesni-intel_asm.S`，對照單區塊 `_aesni_enc1` 與四區塊平行交錯 `_aesni_enc4` 看得一清二楚：

```asm
/* arch/x86/crypto/aesni-intel_asm.S:1994-2016 — 單一 128-bit 區塊加密（循序 CBC 加密被迫走這條路） */
.Lenc128:
	movaps -0x20(TKEYP), KEY
	aesenc KEY, STATE          /* Round 1: SubBytes + ShiftRows + MixColumns + AddRoundKey */
	movaps -0x10(TKEYP), KEY
	aesenc KEY, STATE          /* Round 2（必須乾等上一條 aesenc 跑完硬體管線！） */
	movaps (TKEYP), KEY
	aesenc KEY, STATE          /* Round 3 ... 中間省略 Rounds 4-8 ... */
	...
	movaps 0x60(TKEYP), KEY
	aesenc KEY, STATE          /* Round 9 */
	movaps 0x70(TKEYP), KEY
	aesenclast KEY, STATE      /* Round 10: 跳過 MixColumns 的專用指令！ */
	RET
SYM_FUNC_END(_aesni_enc1)
```

再看緊接在下方的四路平行版 `arch/x86/crypto/aesni-intel_asm.S:2036-2124`（Linux 另外還有 8 路 AVX 版 `arch/x86/crypto/aes_ctrby8_avx-x86_64.S:112-195` 專門加速 CTR 模式）：

```asm
/* arch/x86/crypto/aesni-intel_asm.S:2036-2082（節錄 128-bit 路徑）— 四個獨立區塊交錯餵入硬體管線 */
SYM_FUNC_START_LOCAL(_aesni_enc4)
	movaps (KEYP), KEY		# key
	mov KEYP, TKEYP
	pxor KEY, STATE1		# round 0（Pre-whitening：只做 AddRoundKey XOR！）
	pxor KEY, STATE2
	pxor KEY, STATE3
	pxor KEY, STATE4
	...
.L4enc128:
	movaps -0x20(TKEYP), KEY
	aesenc KEY, STATE1		# 四個區塊共用同一把回合金鑰，連發 4 條獨立 aesenc 填滿管線深度
	aesenc KEY, STATE2
	aesenc KEY, STATE3
	aesenc KEY, STATE4
	...				# Rounds 2..9 同樣四路交錯，Round 10 連發四條 aesenclast
```

- **原始碼對照課本的三個亮點**：
  1. x86 硬體直接把課本的兩類回合變成兩條機器指令：`aesenc`（含 `MixColumns`）與 `aesenclast`（跳過 `MixColumns`），第 0 回合 pre-whitening 就是一條 `pxor`。
  2. 在現代 Intel P-core 上，單條 `aesenc` 延遲（latency）為 **3–4 cycles**，但吞吐率（reciprocal throughput）可達 **0.5–1 cycle/op**。循序 CBC 加密每次只能算 `STATE1`，被迫吞下 $10 \times 4 \approx 40$ cycles 串列延遲；CTR 模式同時把 `ctr+0..ctr+7` 交錯送入 `aesenc`，不僅藏住管線延遲，連回合金鑰 `movaps` 載入次數都攤提了 4–8 倍！

---

## 8. 【實測】混淆／擴散雪崩效應、CBC vs CTR 錯誤傳播與單核 10 GiB/s AES-NI 管線加速

我們撰寫 [experiments/lecture52/crypto_modes_bench.c](experiments/lecture52/crypto_modes_bench.c)，在本機 Intel Core i7-13650HX 單顆 P-core（`taskset -c 0`）上實測四件事：(1) 純軟體 `SubBytes/ShiftRows/MixColumns/KeySchedule` 與硬體 `AES-NI` 對照 FIPS-197 標準測試向量，並跑 10,000 次隨機單 bit 翻轉驗證 Confusion / Diffusion；(2) 比較 ECB / CBC / CTR 的相同區塊洩漏與單 bit 明文錯誤擴散；(3) 在 16 MiB 緩衝區比較純軟體 AES、RC4、循序 `AES-NI` CBC 與 8 路管線化 `AES-NI` CTR 的吞吐量；(4) 驗證課本 RSA（$n=3953$）與 Diffie–Hellman 數值例子。

### 實驗程式碼重點節錄

```c
/* experiments/lecture52/crypto_modes_bench.c:89-121 — 循序 CBC vs 8 路平行 CTR 餵入 AES-NI 管線 */
static void aes128_ni_cbc_enc(const uint8_t *in, uint8_t *out, size_t nbytes,
                              const __m128i rk[11], __m128i iv) {
    __m128i prev = iv;
    for (size_t off = 0; off < nbytes; off += 16) {
        __m128i pt = _mm_loadu_si128((const __m128i *)(in + off));
        prev = aes128_ni_enc_block(_mm_xor_si128(pt, prev), rk); /* 嚴格串列相依 */
        _mm_storeu_si128((__m128i *)(out + off), prev);
    }
}
/* CTR 模式每次展開 8 個獨立 minor counter (minor+0 .. minor+7) 交錯執行 _mm_aesenc_si128 */
```

### 執行指令與實測輸出

```bash
gcc -O3 -maes -msse4.1 -Wall -Wextra computer-architecture/advanced/experiments/lecture52/crypto_modes_bench.c -o /tmp/crypto_modes_bench
taskset -c 0 /tmp/crypto_modes_bench && rm -f /tmp/crypto_modes_bench
```

```text
=== 1. FIPS-197 AES-128 Verification & Avalanche Effect ===
SW Ciphertext : 69c4e0d86a7b0430d8cdb78070b4c55a
NI Ciphertext : 69c4e0d86a7b0430d8cdb78070b4c55a (match=YES)
Confusion (1-bit key flip)      : 64.06 / 128 bits changed (50.05%)
Diffusion (1-bit plaintext flip): 63.95 / 128 bits changed (49.96%)

=== 2. Mode Comparison (Block0 == Block2; 1-bit flip in Block0) ===
Identical PT blocks (B0==B2) -> CT equal?  ECB=YES (leaks!) | CBC=NO | CTR=NO
1-bit PT flip in B0 -> Flipped CT bits per block [B0, B1, B2, B3]:
  CBC mode : [ 67,  62,  70,  57] (cascades to all subsequent blocks)
  CTR mode : [  1,   0,   0,   0] (error isolated to 1 bit)

=== 3. Throughput & Pipeline Parallelism on 16 MiB Buffer ===
SW AES-128 (S-box + GF(2^8))   :      72.2 MiB/s  (37.040 TSC-cycles/B)
RC4 Stream Cipher (byte-serial):     360.9 MiB/s  ( 7.407 TSC-cycles/B)
HW AES-NI CBC Encrypt (serial) :    1692.6 MiB/s  ( 1.579 TSC-cycles/B)
HW AES-NI CTR by8 (pipelined)  :    9978.4 MiB/s  ( 0.268 TSC-cycles/B) -> 5.90x over CBC, 138.3x over SW

=== 4. Textbook RSA & Diffie-Hellman Verification ===
RSA (n=3953, e=31, d=247): m=54 -> E(m)=3574 -> D(E(m))=54 | Sig D(m)=1011 -> E(D(m))=54
Diffie-Hellman (p=2147483647, q=5): A=723883728, B=1231185971 -> K_alice=1440356847, K_bob=1440356847 (match=YES)
64-bit ModExp throughput (toy 64b vs real 2048b is ~1000x slower still): 16.63 MiB/s (sink=152)
```

### 實測觀察與課本對照

1. **完美貼合 Definition 101 的 50% 雪崩效應**：翻轉 1 bit 金鑰平均改變 `64.06 / 128` bits（`50.05%`），翻轉 1 bit 明文平均改變 `63.95 / 128` bits（`49.96%`）。
2. **軟體 vs 專用硬體的反轉現象（RC4 vs AES）**：在純軟體沒有專用指令時，逐 byte 查陣列的 RC4（`360.9 MiB/s`）確實比軟體模擬 $\text{GF}(2^8)$ 的 AES（`72.2 MiB/s`）快 `5.0x`；但一旦做成硬體管線指令（`AES-NI`），可管線化且一次處理 16B 的 **AES-CTR 暴衝到 `9,978.4 MiB/s`（單核近 10 GiB/s，每 byte 僅 `0.268` TSC cycle）**，反超 RC4 達 **`27.6x`**、比純軟體 AES 快 **`138.3x`**！
3. **為什麼安全記憶體架構非用 CTR 不可**：同樣使用硬體 `AES-NI`，循序相依的 CBC 加密受困於管線氣泡，只有 `1,692.6 MiB/s`；改用無相依性的 **CTR 8 路平行加密直接加速 `5.90x`**，足以跟上單通道 DDR4/DDR5 頻寬，且明文 1-bit 錯只影響 `[1, 0, 0, 0]` 位元。
4. **課本 RSA 數值完全吻合**：$54^{31} \bmod 3953 = 3574$、簽章 $54^{247} \bmod 3953 = 1011$ 與課本 p.678–679 一字不差；且連區區 64-bit 玩具模指數都只有 `16.63 MiB/s`（比 AES-CTR 慢 `600x`），證明 RSA 只適合開機／連線初期協商 Session Key。

---

## 9. 一頁速查表

| 主題 | 核心公式／機制 | 架構意義與關鍵取捨 |
|---|---|---|
| **安全目標** | Confidentiality（機密）＋ Integrity（完整）＋ Authenticity（真實） | 加密本身只防偷看（機密性），**不防竄改**（完整性留待第 53 講 SHA/MAC/Merkle Tree） |
| **Confusion / Diffusion** | 改 1 bit Key / Plaintext $\to$ 約 50% 密文位元翻轉 | 消除 Key–CT 與 PT–CT 統計相關性 |
| **AES-128** | $4\times 4$ 狀態矩陣；Round 0 `XOR` + 9 回合四步 + Round 10 省 `MixColumns` | 固定 128-bit 區塊（128/192/256-bit key 對應 10/12/14 回合）；極易硬體展開與管線化 |
| **CTR 模式** | $\text{OTP}_k = \text{AES}_K(\text{Major} \,\|\, \text{Minor}+k),\ C_k = P_k \oplus \text{OTP}_k$ | 加解密雙向平行、相同明文密文不同、單 bit 錯誤不擴散、可預先算 OTP 藏記憶體延遲 |
| **RC4 串流密碼** | 256B `S` 陣列經 Key 洗牌後逐 byte `swap` 輸出 OTP | 軟體簡單，但強狀態相依導致硬體無法管線化 |
| **RSA 與數位簽章** | $E_K(m)=m^e \bmod n,\ D_K(m)=m^d \bmod n$；$E_K(D_K(m))=m$ | 私鑰加密已知訊息即為數位簽章；比 AES 慢數百至數千倍，不用於處理器執行期資料路徑 |
| **Diffie–Hellman** | 公開傳 $A=q^a\bmod p,\ B=q^b\bmod p \implies K=q^{ab}\bmod p$ | 預先共享 $(p, q)$ 即可在不傳送私密值下協商出供 AES 使用的 **Session Key** |

---

## 10. 與後續講次的連結

- **往回對照 [lecture51_note.md](lecture51_note.md)（製程變異）**：第 51 講視為時序毒藥的隨機摻雜與漏電變異，在本講課本 §13.1.4 搖身一變成為每顆晶片無法仿造的硬體身分證 **PUF（Physically Unclonable Function）**。
- **往下接 [lecture53_note.md](lecture53_note.md)（第 53 講：雜湊、Merkle Tree 與安全處理器 Intel SGX）**：本講最後埋下兩個伏筆——(1) 單靠 CTR 加密擋不住主動攻擊者的重送（replay）與拼接（splicing）竄改，必須加上 **SHA 雜湊、MAC 與 Bonsai Merkle Tree**；(2) 本講介紹的 `(Major, Minor)` **Split-Counter CTR 模式**正是第 53 講硬體記憶體加密引擎（MEE）能夠在等待 DRAM 資料回傳時「預先平行算出 OTP、將解密延遲壓到單一 XOR」的核心武器！
- **往下接 [lecture54_note.md](lecture54_note.md)（第 54 講：旁道攻擊與 ORAM）**：為什麼現代處理器寧可做專用 `AES-NI` 硬體指令，也不願在軟體裡查 `SBOX[256]` 陣列？因為軟體查 `SBOX[s[i]]` 的記憶體位址取決於秘密狀態，會透過快取命中／未命中時間差把 AES 金鑰洩漏給旁道攻擊者！

---

## 附：本講一句話總結

**處理器硬體安全以混淆與擴散為理論根基，在開機或連線初期透過慢速但靈活的 RSA／Diffie–Hellman 建立工作階段金鑰與身分認證，執行期則全面交棒給高度管線化的硬體 AES Counter Mode（Major+Minor 計數器產生一次性密碼本），兼顧雙向平行高吞吐、錯誤隔離與密碼強度。**

---

*筆記整理自 Smruti R. Sarangi 教授 Advanced Computer Architecture 第 52 講影片與 Next-Gen Computer Architecture v3.2 第 13 章 §13.1，搭配 Linux v6.2.12 原始碼與本機實測驗證。*
