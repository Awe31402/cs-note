# 《Engineering Secure Devices: A Practical Guide for Embedded System Architects and Developers》讀書筆記

- **作者**：Dominik Merli
- **出版**：No Starch Press，2024 年，ISBN-13 (print): 978-1-7185-0348-9 / ISBN-13 (ebook): 978-1-7185-0349-6
- **實驗／示範平台**：STMicroelectronics STM32MP157F-DK2 (Dual Cortex-A7 @ 650 MHz + Cortex-M4 @ 209 MHz), OpenSTLinux (Linux 5.15), Yocto Project
- **技術審校**：Colin O'Flynn（ChipWhisperer 開源專案發起人、Dalhousie University 助理教授）
- **閱讀說明**：本筆記以中文（繁體）整理重點，引用保留英文原文（附原書印刷頁碼）。本書專為「嵌入式系統架構師、韌體與 Linux 開發者、產品安全工程師」編寫，重點聚焦於安全開發流程（Secure Development Process）、硬體與密碼學積木（RNG、Crypto 實作、機密儲存、裝置身分、安全通訊）、以及進階裝置安全架構（Secure Boot、韌體更新 SWUpdate、即時強健性架構、存取控制 DAC/MAC、系統監控與日誌）。書中未提到的內容一律標註「書中未提」，不做補充推測。

---
## 第 1 章：Secure Development Process（安全開發流程與成熟度管理）

> 對應原書 p.34–68

### 這章在幹嘛

這是全書的組織與工程方法論基礎章。作者開門見山打破了傳統硬體與韌體工程師「只要把功能寫好、最後再加一把鎖」的迷思。作者以其十餘年在西門子（Siemens）與學術界的安全工程經驗指出：**技術防護功能可以標記為「完成」，但安全開發流程永遠沒有「完成」的一天**——它必須持續維護、迭代與改善，因此安全工程流程的衡量指標被稱為「成熟度（Maturity）」。

整章骨架分為八大核心模組：
1. **安全開發指引的多樣性與選型**：微軟 SDL、OWASP SAMM 與工業認證標準 IEC 62443-4-1 的定位對比。
2. **組織職責劃分與安全文化**：管理層承諾、產品安全官（PSO）與關鍵的「安全擁護者（Security Champions）」制度。
3. **安全意識與持續培訓**：「解決安全問題的不是工具，也不是金錢，而是人」。
4. **資產盤點與保護目標**：產品關鍵資產分類，以及從傳統 CIA 延伸到真實性（Authenticity）與功能安全（Safety）的交織。
5. **攻擊者模型、威脅與風險分析**：六大攻擊者等級評估、負面衝擊維度與「風險 = 可能性 × 衝擊度」矩陣。
6. **安全需求、架構原則與風險處置**：減緩、轉移、接受與規避四大風險處置策略，以及 Saltzer & Schroeder 經典安全架構原則。
7. **安全實作與測試（Shift Left）**：左移測試哲學、SAST、DAST、SCA（軟體成分分析）與滲透測試（Attackers as a Service）。
8. **漏洞監控與事件處置（PSIRT）**：`security.txt` 漏洞通報管道、CVE 申請、CVSS v3.1 評分、協調披露（CVD）與長期修復信任建立。

---

### 重點 1：安全流程的本質：成熟度（Maturity）而非勾選清單

許多工程團隊將安全視為產品發表前的一個功能測試步驟，但作者指出軟硬體安全具有動態與對抗特性：

> **原文引用（p.34）**
> "Although a product’s technical protection features can be fulﬁlled and marked as done, a secure development process is never done; it must be maintained and improved continuously, which is why the qualitative measure for security engineering processes is called maturity. It can rise and fall, depending on the regularity, quality, and organizational involvement of all corresponding activities."

**白話解析**：
安全不是「0 與 1」的二元狀態。一個組織即使在上一代產品部署了完整的加密開機，若下一代產品換了供應商、開發流程失去紀律、第三方開源套件未追蹤漏洞，其整體安全成熟度就會迅速滑落。

---

### 重點 2：三大主流安全開發指引評比：軟體 vs. 工業元件

面對繁多的安全標準，作者梳理了三套最具影響力的指引：

| 指引名稱 | 提出組織 | 主要目標領域 | 特性與認證價值 |
| :--- | :--- | :--- | :--- |
| **Microsoft SDL** (Security Development Lifecycle) | Microsoft (2004) | 軟體開發、作業系統、大型應用程式 | 著重於開發週期的各階段活動（培訓、需求、設計、實作、驗證、發布、處置），對軟體工程影響深遠。 |
| **OWASP SAMM** (Software Assurance Maturity Model) | OWASP 基金會 | Web 應用、雲端與現代敏捷軟體 | 提供五大業務功能（治理、設計、實作、驗證、維運），具備清晰的成熟度評估層級（Level 1~3）。 |
| **IEC 62443 Part 4-1** | 國際電工委員會 (IEC) | 工業自動化、控制系統、物聯網嵌入式設備 | **工業界的核心標準**，專門規範「產品供應商的安全開發生命週期要求」（8 大實踐、47 條具體要求），可進行第三方正式認證。 |

> **原文引用（p.36）**
> "While the Microsoft and OWASP recommendations are targeted at software products, IEC 62443 Part 4 is clearly meant for industrial components that can be certiﬁed."

**工程實務洞見**：嵌入式與 IoT 設備架構師若面臨客戶對供應鏈安全的合規稽核，IEC 62443-4-1 是目前最具公信力且能取得第三方認證的框架。

---

### 重點 3：關鍵組織機制：Security Champions（安全擁護者）

作者指出，僅在組織最高層設立產品安全官（Product Security Officer, PSO）往往無法落地，因為中央安全專家無法深入每支韌體與硬體團隊的日常站會與程式碼審查。解決之道是培育 **Security Champions（安全擁護者）**：

> **原文引用（p.37）**
> "An effective approach to establish security expertise throughout an organization is training security champions who are directly integrated within the development teams... They don’t have to know everything about security, but they know who to ask, when to raise concerns, and how to keep security on the team’s agenda."

**角色定位**：
- 安全擁護者不是空降的資安稽核員，而是**開發團隊內部的資深工程師**。
- 他們接受額外的安全架構與威脅建模培訓，在產品架構討論時扮演「紅隊思維代言人」，確保安全需求不會在衝刺時程（Sprint）中被犧牲。

---

### 重點 4：資產（Assets）與保護目標（Protection Goals）盤點

安全工程不能空談「讓系統安全」，必須具體定義「我們要保護什麼」：

1. **產品核心資產（Assets）**：
   - **智慧財產（IP）**：專有韌體二進制、機器學習演算法權重、校準參數。
   - **密碼學機密**：開機金鑰、裝置私鑰、對稱主金鑰、對稱資料庫密碼。
   - **使用者與遙測資料**：工業感測讀數、個人身分資訊（PII）、操作日誌。
   - **實體與邏輯完整性**：微控制器開機設定、eFuse 狀態、周邊暫存器保護。
   - **服務連續性**：設備執行核心即時控制功能的能力。

2. **多維保護目標**：
   除了傳統的機密性（Confidentiality）、完整性（Integrity）與可用性（Availability）之外，嵌入式領域必須納入：
   - **真實性（Authenticity）**：確保接收到的指令或韌體確由合法授權者發出。
   - **不可否認性（Non-repudiation）**：防止操作員或對端否認關鍵交易。
   - **功能安全（Safety）**：資安漏洞可能直接轉化為實體人身傷害（例如工業機械臂失控或汽車煞車被鎖死）。資安（Security）是功能安全（Safety）的前提。

---

### 重點 5：威脅建模（Threat Modeling）與六大攻擊者等級

作者強調，防禦設計必須根據「誰是你的潛在敵人」來調配預算與技術深度。原書將攻擊者劃分為六個層級：

```
[國家級 APT (Nation States)]        ▲ 資源無限、國家實驗室、硬體側錄/逆向工程
[競爭對手/工業間諜 (Competitors)]      │ 竊取核心專利、逆向專有通訊協定
[組織犯罪集團 (Cybercriminals)]      │ 勒索病毒、勒索贖金、殭屍網路 (Mirai)
[內部惡意人員 (Insiders)]           │ 擁有物理存取權、合法憑證、供應鏈員工
[黑客行動主義 (Hacktivists)]        │ 政治/意識形態動機、損害公司聲譽
[腳本小子 (Script Kiddies)]         ▼ 隨機掃描、公開漏洞工具利用
```

> **原文引用（p.43–44）**
> "Understanding your adversary is essential for realistic threat modeling. If your product is a smart light bulb, defending against a nation-state attacker using a focused ion beam workstation to extract keys from silicon is financially irrational; however, defending against script kiddies scanning the internet with default credentials is mandatory."

**風險量化公式**：
$$	ext{Risk} = 	ext{Likelihood} 	imes 	ext{Impact}$$
- 可能性（Likelihood）：依據攻擊者動機、所需技能、硬體設備成本與接觸門檻（遠端網路 vs. 必須拆機接觸 PCB）。
- 衝擊度（Impact）：財務損失、法律訴訟、聲譽破壞與功能安全危害。

---

### 重點 6：四大風險處置策略與經典安全架構原則

在完成風險矩陣排序後，組織必須明確採取以下四種策略之一進行處置：
1. **減緩（Mitigate）**：設計技術防護措施（如導入 Secure Boot、AppArmor 或加密通道）。
2. **轉移（Transfer）**：購買網路資安保險，或將敏感認證外包給具備合規認證的雲端提供商。
3. **接受（Accept）**：由管理層正式簽署接受微小且防禦成本不成比例的殘餘風險（必須有記錄與覆審週期）。
4. **規避（Avoid）**：直接**刪除或停用**帶來高風險但非核心的功能。

> **作者特別提醒（IN PRACTICE: DISCARDING FEATURES, p.49）**
> "The most secure feature is the one that was never implemented... If an engineering team realizes that a remote telnet diagnostic interface introduces immense attack surface that cannot be sufficiently hardened with available resources, the right security decision is to remove the feature entirely."

**經典設計原則（Saltzer & Schroeder, 1975）在嵌入式的落實**：
- **最小權限（Least Privilege）**：絕不以 root 身分執行所有物聯網常駐程式。
- **縱深防禦（Defense in Depth）**：即使網路防護失守，系統仍有本地存取控制；即使存取控制被突破，仍有唯讀檔案系統與硬體加密隔離。
- **預設安全（Fail-Safe Defaults）**：認證失敗或網路斷線時，系統應回退至拒絕存取的安全狀態，而非開放後門。
- **機制經濟性（Economy of Mechanism）**：安全架構越簡單越容易驗證；過度複雜的防護層本身就是漏洞的溫床。
- **開放設計（Open Design / Kerckhoffs's Principle）**：不依賴隱匿性（Obscurity），演算法與通訊協定公開仍能維持安全。

---

### 重點 7：Shift Left（測試左移）與滲透測試

在傳統瀑布模型中，安全測試往往發生在產品出貨前幾週，修復成本極高（甚至因硬體已開模而無法修改）。**Shift Left（左移）** 要求將安全驗證前推至設計與程式碼撰寫階段：

1. **靜態應用程式安全測試（SAST）**：在 CI/CD 流程中自動掃描 C/C++ 記憶體安全問題（緩衝區溢位、未初始化變數、整數溢位）。
2. **動態應用程式安全測試（DAST）與 Fuzzing**：對裝置開放的網路通訊協定（MQTT, HTTP, CoAP）灌入大量變異畸形封包，檢驗常駐程式是否崩潰。
3. **軟體成分分析（SCA）與 SBOM**：自動追蹤開源套件版本與依賴鏈，生成軟體物料清單（Software Bill of Materials），即時對照 NVD 資料庫。
4. **滲透測試（Attackers as a Service）**：聘請外部獨立白帽駭客，在黑箱或灰箱條件下模擬真實惡意攻擊者進行紅隊演練。

---

### 重點 8：漏洞監控與事件回應（PSIRT）

沒有任何系統在發表後能永遠無瑕疵。作者闡述了現代硬體廠商必須建立的產品安全事件回應機制（PSIRT）：

```
[漏洞發現 (外部研究員/內部)]
       │
       ▼
[通報管道 (RFC 9116 security.txt)]
       │
       ▼
[PSIRT 接收、初審與驗證]
       │
       ▼
[CVSS v3.1 嚴重度評級 (Base / Temporal / Environmental)]
       │
       ▼
[研發團隊修復並驗證 Patch]
       │
       ▼
[協調披露 (Coordinated Vulnerability Disclosure) & 申請 CVE]
       │
       ▼
[向客戶發布安全通報 (Security Advisory) & OTA 韌體更新]
```

> **原文引用（p.61）**
> "Establishing a formal vulnerability-reporting channel is not a sign of weakness; it’s a sign of organizational maturity. Implementing a simple security.txt file on your web server according to RFC 9116 gives ethical security researchers a clear contact point instead of forcing them to search LinkedIn for engineers or drop 0-days publicly."

---

### 本章 Summary（作者原話）

> **原文引用（p.66–67）**
> "This chapter summarized important activities and processes that organizations should follow when aiming for secure products. In the beginning, I explained that you can choose from multiple guidelines that are similar in content. While the Microsoft and OWASP recommendations are targeted at software products, IEC 62443 Part 4 is clearly meant for industrial components that can be certiﬁed.
> A basic requirement for all secure development processes is that people are aware of risks and receive the security training they need for their daily tasks. The key to creating a secure product is a solid analysis of assets, corresponding protection goals, possible attackers, threats, and involved risks. You can tackle this task with a structured approach that leads to transparency, explicit trust boundaries, and measurable risk.
> During implementation and testing, the shift-left philosophy encourages us to identify and ﬁx issues as early as possible.
> Finally, establishing a vulnerability monitoring and response process closes the loop and supports organizations in building customer trust."

---

### 思考與實務檢驗

1. **為什麼導入 IEC 62443-4-1 或微軟 SDL 對傳統嵌入式開發團隊最困難的往往不是「密碼學演算法實作」，而是「組織文化與角色劃分」？**
   - 傳統嵌入式硬體開發文化強調「交付即完成（Fire and Forget）」與「時程至上（Time-to-Market）」，工程師普遍認為安全是額外的絆腳石。導入 IEC 62443-4-1 要求建立跨部門的職責劃分（PSO、Security Champions）、強制威脅建模審查、持續跟蹤 CVE 與長期維護計畫。若沒有管理層在資源、預算與考核上的實質授權，安全活動往往淪為形式主義的文書作業，一遇到專案交期落後就會被全盤拋棄。

2. **在資源極其有限的小型物聯網新創團隊中，如何落實「資安左移（Shift Left）」而不至於拖垮研發進度？**
   - 務實的策略是「自動化輕量整合」而非全套繁重流程：第一步在 Git 提交管道（GitHub Actions / GitLab CI）中掛入免費開源的靜態檢查工具（如 `cppcheck`、Compiler Warnings `-Wall -Wextra -Werror`）與依賴漏洞檢查（如 `Dependabot` 或 `Trivy`）；第二步將最危險的外部暴露介面（如 Web 管理頁面或 MQTT 監聽器）列為第一級受保護資產，聚焦於最簡單的威脅清單；第三步直接遵從「機制經濟性」，大膽裁撤不必要的除錯功能與網路通訊協定，從根本上減少需要審查的程式碼行數。

---

### 我的整理：本章帶走的五句話

1. **安全開發不是發表前的一次性測試勾選，而是貫穿產品整個生命週期的動態成熟度管理。**
2. **解決資安危機的核心不是昂貴的商用工具或預算，而是具備安全意識與受過架構培訓的工程團隊。**
3. **防禦必須量體裁衣：沒有量化資產價值與攻擊者能力等級的威脅建模，只會導致過度防禦或嚴重漏洞。**
4. **機制經濟性與功能刪減是最高級的防護：不實作非必要的功能，勝過替脆弱的特徵補上一萬道防護鎖。**
5. **公開漏洞回報管道（如 security.txt）與建立 PSIRT 不是承認脆弱，而是建立客戶與市場信任的成熟標誌。**

---

### 延伸資源（p.36, p.50, p.61）

- 國際工業資安標準：IEC 62443-4-1:2018 *Security for industrial automation and control systems - Part 4-1: Secure product development lifecycle requirements*
- 微軟安全開發週期：Microsoft Security Development Lifecycle (SDL) 指引
- 開放網路應用安全專案：OWASP Software Assurance Maturity Model (SAMM) v2.0
- 資安弱點通報文字標準：RFC 9116 *A File Format to Aid in Security Vulnerability Disclosure (`security.txt`)*
- 通用漏洞評分系統：FIRST Common Vulnerability Scoring System (CVSS v3.1) Specification

---
## 第 2 章：Cryptography（嵌入式密碼學基石與演算法選型）

> 對應原書 p.69–101

### 這章在幹嘛

本章為全書所有硬體與韌體防護技術提供精準的數學與演算法地基。作者並非以純理論數學家的視角堆砌公式，而是從「**嵌入式系統工程師如何正確選擇與使用密碼原語**」切入。

作者特別針對嵌入式領域最常見的致命誤解——「只要資料加密了，就不會被篡改」進行了徹底的撥亂反正，深入剖析了為何機密性（Encryption）與真實性／完整性（MAC/AEAD）必須嚴格解耦或以安全模式結合。整章結構清晰分明：
1. **密碼學核心哲學**：Kerckhoffs 原則與「基於窮舉搜尋空間」的安全強度位元定義（112 vs. 128 vs. 256 bits）。
2. **對稱區塊加密**：從 DES 的歷史興衰到 AES 的代換-置換網路（SPN）機制。
3. **區塊加密運作模式（Modes of Operation）**：ECB 企鵝圖之災、CBC 的 IV 需求與填充攻擊、CTR 模式的串流特性與金鑰串流重用死穴。
4. **密碼雜湊函數（Hash Functions）**：原像抗性、第二原像抗性與碰撞抗性（生日悖論），SHA-2 與 SHA-3 的演進。
5. **訊息鑑別碼（MAC）**：為何對稱加密不防篡改？HMAC 與 CMAC 的架構與工作原理。
6. **認證加密（AEAD）**：Encrypt-then-MAC 原則、AES-GCM（Galois Counter Mode）的高效並行架構與 Nonce 絕對不可重用的致命警告。
7. **非對稱密碼體系（Asymmetric Cryptography）**：公鑰／私鑰對偶性。
8. **RSA 密碼系統**：Euler 歐拉函數、質數選取、公鑰指數 $e=65537$、教科書 RSA 弱點與 OAEP / PSS 填充標準。
9. **Diffie-Hellman（DH）金鑰交換**：離散對數問題之美與中間人（MITM）攻擊防禦。
10. **橢圓曲線密碼學（ECC）**：Weierstrass 曲線方程式、純量乘法、金鑰長度壓倒性優勢（256-bit ECC 對決 3072-bit RSA）、曲線選型（NIST P-256 vs. Curve25519 vs. Brainpool）與 ECDSA / Ed25519 / ECDH 實務應用。

---

### 重點 1：Kerckhoffs 原則：金鑰是系統中唯一允許保密的元素

荷蘭密碼學家 Auguste Kerckhoffs 於 1883 年提出了軍事密碼學的六大黃金法則，其中最著名的原則至今仍是現代資安的不可動搖的基石：

> **原文引用（p.69）**
> "Among the six recommendations he published in 1883, one became famously known as Kerckhoffs’s principle: 'The system must not require secrecy and can be stolen by the enemy without causing trouble.'
> For cryptographic algorithms and security protocols, this implies that the security of a system must rely solely on the confidentiality of the cryptographic key, and not on keeping the system itself secret."

**反思「隱匿式安全（Security through Obscurity）」**：
許多硬體廠商喜歡自創「未公開的專有加密演算法」或透過混淆暫存器位址來達成保護。作者強調，逆向工程（JTAG 傾印、反組譯、微探針）遲早會讓演算法攤在陽光下；唯有經受全世界密碼學界公開檢驗數十年的公開演算法（如 AES、ECDSA），搭配高熵值的安全金鑰，才能提供數學上可證明的保護。

---

### 重點 2：安全強度位元（Security Levels）的真實意涵

當我們說一個密碼系統具備「128 位元安全強度」時，究竟代表什麼？

> **原文引用（p.70–71）**
> "The security level describes the effort required for an attacker to break a cryptosystem. A security level of $n$ bits means that an adversary would need an average of $2^{n-1}$ brute-force attempts to guess the secret key... Current international recommendations (such as NIST or BSI) require a minimum security level of 112 bits for legacy systems and at least 128 bits for all newly designed systems."

**主流演算法安全強度對照表**：

| 安全強度（位元） | 對稱加密演算法 | 雜湊函數（碰撞抗性） | RSA 金鑰長度 | 橢圓曲線（ECC）金鑰長度 | 時代適用性建議 |
| :---: | :---: | :---: | :---: | :---: | :--- |
| **80 bits** | 2-Key 3DES, SKIPJACK | SHA-1 | 1024 bits | 160 bits | **完全破解，嚴禁使用** |
| **112 bits** | 3-Key 3DES | SHA-224 | 2048 bits | 224 bits | 舊系統過渡期，新設計不推薦 |
| **128 bits** | **AES-128** | **SHA-256** | **3072 bits** | **256 bits (P-256 / Ed25519)** | **當前工業與 IoT 標準基準** |
| **192 bits** | AES-192 | SHA-384 | 7680 bits | 384 bits | 高機密政府／金融應用 |
| **256 bits** | **AES-256** | **SHA-512** | **15360 bits** | **512 bits (P-521)** | **最高安全級／抗後量子初期威脅** |

---

### 重點 3：AES 對稱區塊加密與運作模式陷阱

AES（FIPS 197）是現代對稱加密的絕對霸主。其區塊長度固定為 **128 位元（16 位元組）**，金鑰長度可選 128、192 或 256 位元，經過 10、12 或 14 輪的 SubBytes、ShiftRows、MixColumns 與 AddRoundKey 代換置換運算。

然而，如何將單一 128-bit 區塊加密擴展至任意長度的資料串流？不同的 **運作模式（Modes of Operation）** 具有生死攸關的差異：

#### 1. ECB 模式（Electronic Codebook）——永遠的禁忌
每個 128-bit 明文區塊各自獨立加密：$C_i = E_K(P_i)$。
- **致命弱點**：相同的明文區塊永遠產生完全相同的密文區塊！
- 經典案例：將點陣圖（Bitmap）的企鵝 Tux 圖像以 ECB 加密後，企鵝的輪廓與色彩分佈在密文中依然清晰可見！
- **結論**：除極特殊單區塊金鑰封裝外，**絕對嚴禁在通訊或儲存中使用 ECB**。

#### 2. CBC 模式（Cipher Block Chaining）
每個明文區塊在加密前，先與前一個密文區塊進行 XOR 運算：$C_i = E_K(P_i \oplus C_{i-1})$。首個區塊需要初始向量（$C_0 = 	ext{IV}$）。
- **關鍵要求**：IV 必須是**不可預測的加密隨機數**，絕不能重用固定 IV 或遞增計數器。
- **隱患**：若未搭配 MAC 驗證，CBC 模式極易遭受「密文填補攻擊（Padding Oracle Attack）」，讓攻擊者在不獲取金鑰的情況下逐字節解密明文。

#### 3. CTR 模式（Counter Mode）
將區塊加密器轉化為**串流加密器（Stream Cipher）**。核心將金鑰與遞增計數器加密產生虛擬金鑰串流（Keystream），再將其與明文進行 XOR：$C_i = P_i \oplus E_K(	ext{Nonce} \mathbin{\Vert} 	ext{Counter}_i)$。
- **優勢**：加解密演算法完全相同、支援任意隨機存取（Seekable）、高度並行化處理、無須任何資料填充（Padding）。
- **致命紅線**：**同一個金鑰絕對不可搭配相同的 Nonce 加密兩次！** 若 Nonce 重用，則兩份密文 XOR 的結果直接等於兩份明文 XOR：$C_1 \oplus C_2 = P_1 \oplus P_2$，金鑰串流徹底瓦解。

---

### 重點 4：加密不等於防篡改：MAC 與認證加密（AEAD）

在硬體與韌體開發中最常見的安全設計錯誤，就是「將密文寫入外部 SPI Flash，並假設黑客因為不知道金鑰而無法竄改它」。

> **原文引用（p.85）**
> "A common misconception about encryption is that it protects against manipulation. It doesn't. Even if you use AES and a state-of-the-art mode of operation, and an attacker can’t read anything from the resulting ciphertext, they might still be able to flip bits within the ciphertext. In CTR mode, flipping a specific bit in the ciphertext flips the exact same bit in the decrypted plaintext with 100% mathematical certainty!"

**解決方案**：
1. **HMAC（Hash-based MAC, RFC 2104）**：利用雜湊函數與對稱金鑰結合：$	ext{HMAC}(K, M) = H((K \oplus opad) \mathbin{\Vert} H((K \oplus ipad) \mathbin{\Vert} M))$。
2. **CMAC（Cipher-based MAC, NIST SP 800-38B）**：利用 AES 區塊加密器構建的對稱認證標籤。
3. **AEAD（Authenticated Encryption with Associated Data）**：
   現代工程標準是一律採用 AEAD 演算法。最廣泛使用的就是 **AES-GCM（Galois Counter Mode, NIST SP 800-38D）**：
   - 採用 CTR 模式進行高吞吐量加密。
   - 採用 Galois Field $GF(2^{128})$ 乘法（GHASH）計算認證標籤（Tag）。
   - 支援附加未加密但受認證的資料（Associated Data, AD，例如封包標頭、IP 位址）。
   - **工程警告**：AES-GCM 若在同一個金鑰下發生 Nonce 重用，攻擊者不僅能還原明文，更能直接推導出 GHASH 認證雜湊金鑰 $H$，使偽造任意封包標籤成為可能！

---

### 重點 5：非對稱密碼系統：RSA 運作機制與填充標準

非對稱加密徹底解決了對稱密鑰預先分發的工程難題。RSA 基於「大整數質因數分解」的計算困難度：
1. 選取兩個超大獨立質數 $p$ 與 $q$。
2. 計算模數 $n = p 	imes q$，歐拉函數 $\phi(n) = (p-1)(q-1)$。
3. 選取公鑰指數 $e$（標準一律固定為 $65537 = 2^{16}+1$，以兼顧效能與安全性）。
4. 計算私鑰指數 $d$，滿足 $e \cdot d \equiv 1 \pmod{\phi(n)}$。
5. **加密運算**：$C = M^e \pmod n$；**解密運算**：$M = C^d \pmod n$。
6. **數位簽章**：$S = H(M)^d \pmod n$；**驗證簽章**：$H(M) \stackrel{?}{=} S^e \pmod n$。

> **原文引用（p.90）**
> "Never implement or use textbook RSA. Without proper padding schemes, RSA is algebraically malleable and vulnerable to chosen-ciphertext attacks."

**必備填充標準**：
- **加密填充**：必須使用 **RSA-OAEP**（Optimal Asymmetric Encryption Padding），徹底消除同態特性。
- **簽章填充**：必須使用 **RSA-PSS**（Probabilistic Signature Scheme），提供嚴格的密碼學安全性證明，淘汰舊式 PKCS#1 v1.5。

---

### 重點 6：橢圓曲線密碼學（ECC）：嵌入式系統的絕對救星

RSA 雖然數學優雅且驗證簽章極快，但隨著安全強度要求提升，RSA 的金鑰尺寸呈現爆炸性成長（128-bit 安全強度需要 3072-bit RSA；256-bit 強度需要 15360-bit RSA！），這在 RAM 僅有幾十 KB、儲存空間斤斤計較的微控制器上是災難性的負擔。

**橢圓曲線密碼學（ECC）** 基於有限體上的 Weierstrass 方程式：
$$y^2 \equiv x^3 + ax + b \pmod p$$
利用點加法（Point Addition）與倍點（Point Doubling）運算定義純量乘法 $Q = d \cdot G$（$d$ 為私鑰整數，$G$ 為基底點，$Q$ 為公鑰座標）。由於「橢圓曲線離散對數問題（ECDLP）」至今沒有類似一般數體篩法的次指數攻擊演算法，ECC 能以極短的金鑰提供同等防護：

> **原文引用（p.95–96）**
> "While an RSA key for a 128-bit security level occupies 3,048 bits (384 bytes), a corresponding ECC key requires only 256 bits (32 bytes). That is an 88% reduction in key storage and transmission overhead, accompanied by vastly superior signing performance!"

**主流曲線族系與實務選型**：
1. **NIST 質數曲線（P-256 / secp256r1）**：國際標準最通用，硬體晶片硬體加速廣泛支援，但部分學界對其種子常數來源存疑。
2. **Curve25519 / Ed25519（Daniel J. Bernstein）**：近代密碼學大作。針對防禦側通道攻擊與常數時間（Constant-time）運算進行了極致優化，在軟體實作上極度安全且迅速，是現代 SSH、TLS 1.3 與 WireGuard 的核心首選。
3. **Brainpool 曲線（RFC 5639）**：德國聯邦資訊安全局（BSI）推薦，種子常數完全透明偽隨機生成，廣泛應用於歐洲汽車與智慧電表。

---

### 本章 Summary（作者原話）

> **原文引用（p.100）**
> "This chapter started off with crypto fundamentals like Kerckhoffs’s principle that a cryptographic key should be the only secret in a cryptosystem, followed by a discussion that security levels are merely a description of the potential search space an attacker has to face. I covered the area of symmetric encryption based on the popular AES algorithm and common operation modes, followed by hash functions and message authentication codes.
> In addition to individual functions, I introduced the concept of authenticated encryption (AEAD) to achieve multiple protection goals within a single algorithm.
> A detailed discussion of asymmetric cryptography, including the mathematical fundamentals of RSA, Diffie-Hellman key exchange, and elliptic-curve cryptography, rounded off the crypto foundations.
> Now you have a sound foundation of cryptographic mechanisms to understand the concepts and implementations presented in the upcoming chapters."

---

### 思考與實務檢驗

1. **為什麼在資源受限的物聯網微控制器上，ECC（如 Ed25519 或 NIST P-256）全面優於 RSA-2048/4096？在哪些例外場景 RSA 仍具備工程優勢？**
   - **優勢對比**：ECC 私鑰僅 32 位元組（RSA-3072 需 384 位元組），RAM 佔用極小；ECC 生成金鑰與產生數位簽章的速度比 RSA 快數十至數百倍，且電池耗電量顯著較低。
   - **RSA 唯一的剩餘優勢**：當公鑰指數固定為 $e=65537$（僅有 17 個位元，且只有兩個位元為 1）時，RSA 的**驗證簽章速度（Verification）**極其迅速（只需幾次平方與乘法運算），甚至可能超越純軟體實作的 ECC 驗證。因此在「設備端只負責驗證開機韌體簽章，而從不在設備端動態簽章」且硬體無 ECC 加速器的情境下，RSA-2048 依然具有一定的開機延遲優勢。

2. **為什麼很多韌體工程師誤以為「資料只要經過 AES 加密就安全了」，而忽略了 MAC / AEAD？若在通訊或儲存中省略 MAC 會引發什麼實體安全後果？**
   - 加密只保證**機密性（Confidentiality）**，絕不保證**完整性（Integrity）**。若通訊協定使用 CTR 模式或串流加密，攻擊者即使無法解密密文，只要知悉明文某個欄位的格式（例如 IP 標頭、工控 PLC 的繼電器啟動命令 `0x00`），就可以在傳輸線路上精確翻轉該位元（XOR 注入），接收端解密後就會執行被惡意修改的致命指令！若省略 MAC，惡意中間人能隨意篡改感測器讀數或硬體暫存器設定，造成設備實體損壞。

---

### 我的整理：本章帶走的五句話

1. **不要發明自己的加密演算法，系統的安全性必須完全仰賴金鑰的保密性，而非隱匿設計。**
2. **現代嵌入式系統的新產品設計，密碼學安全強度底線為 128 位元（如 AES-128、SHA-256、ECC-256、RSA-3072）。**
3. **ECB 模式是災難，CBC 模式需防範 IV 重用與填充攻擊，CTR 與 GCM 模式中 Nonce 重用等同金鑰自毀。**
4. **加密只管保密，防篡改必須仰賴 MAC；現代工程實務應預設全面採用 AEAD（如 AES-GCM 或 ChaCha20-Poly1305）。**
5. **在金鑰儲存空間、運算負擔與傳輸頻寬上，256 位元橢圓曲線（ECC）是嵌入式與物聯網裝置的最佳解。**

---

### 延伸資源（p.73, p.85, p.96）

- 美國國家標準：NIST FIPS 197 *Advanced Encryption Standard (AES)*
- 認證加密模式規範：NIST SP 800-38D *Recommendation for Block Cipher Modes of Operation: Galois/Counter Mode (GCM)*
- 雜湊認證碼規範：RFC 2104 *HMAC: Keyed-Hashing for Message Authentication*
- 橢圓曲線標準：RFC 8032 *Edwards-Curve Digital Signature Algorithm (EdDSA / Ed25519)*
- 密碼演算法強度過渡指南：NIST SP 800-57 Part 1 Rev. 5 *Recommendation for Key Management*

---
## 第 3 章：Random Number Generators（隨機數生成器：從 CMOS 物理噪聲到作業系統熵池）

> 對應原書 p.102–124

### 這章在幹嘛

密碼學界有一句名言：「沒有優質的隨機數，密碼學大廈將在第一天崩塌。」作者在本章帶領讀者深入隨機數生成器（RNG）的底層物理與軟體工程世界。

在嵌入式系統中，隨機數肩負著產生對稱金鑰、RSA/ECC 私鑰、TLS Session ID、AES-GCM Nonce、初始向量（IV）與防重放挑戰碼（Nonce）的重責大任。若隨機數發生偏頗或可被預測，所有加密演算法都將形同虛設。

整章分為六大核心主題與兩大實戰案例研究：
1. **隨機性的本質與安全需求**：不可預測性（Unpredictability）vs. 統計均勻分佈（Statistical Uniformity）。
2. **真隨機數生成器（TRNG / NRBG）**：CMOS 晶片內部的熱噪聲、散粒噪聲、亞穩態（Metastability）與環形振盪器（Ring Oscillators, RO）電路架構。
3. **熵源線上健康監控（Online Health Tests）**：NIST SP 800-90B 與 BSI AIS 31 標準，重複計數測試（RCT）與適應性比例測試（APT）。
4. **偽隨機數生成器（PRNG / DRBG）**：確定性演算法、內部狀態更新、週期性重置種子（Reseeding）。
5. **實務 RNG 混合架構與開機初期熵匱乏困境**：TRNG + CSPRNG 的互補架構，以及無頭嵌入式設備（Headless IoT）在冷啟動時的「熵飢餓（Entropy Starvation）」危機。
6. **實戰案例 1：STM32MP157F 硬體 RNG 直通 Python**：Linux 核心 `hw_random` 框架、`/dev/hwrng`、`rngd` 守護行程、`getrandom()` 系統呼叫與 Python `secrets` vs. `random` 模組的實測對比。
7. **實戰案例 2：隨機數品質快速檢驗工具與兩大經典實作陷阱**：`ent` 工具實務解讀、Modulo 運算產生的空間偏差（Modulo Bias）、自創 PRNG（如線性同餘法 LCG）的毀滅性缺陷。

---

### 重點 1：隨機性的本質：統計隨機 vs. 密碼學不可預測性

許多工程師常犯的錯誤，是認為「通過均勻分佈測試的數列就是隨機數」。作者明確劃分了統計特性與安全特性的界線：

> **原文引用（p.103）**
> "Although randomness is a natural thing, it’s quite difﬁcult to describe. A simple explanation is that randomness is obtained from an experiment for which the outcome can’t be predicted before it’s executed, like a coin toss... For security purposes, statistical uniformity is not enough; the numbers must be cryptographically unpredictable to an adversary who knows the algorithm and has observed past outputs."

**白話解析**：
圓周率 $\pi$ 的小數點後百萬位數在統計學上高度均勻分佈，但如果拿它來當私鑰，攻擊者只要知道你使用了 $\pi$，隨時能精確算出下一個位元組。密碼學需要的不是「貌似混亂」，而是「物理上的不可預測（Unpredictability）與前向／後向保密性（Forward/Backward Secrecy）」。

---

### 重點 2：TRNG 物理機制：環形振盪器（Ring Oscillator）

真隨機數生成器（True Random Number Generator, TRNG）依賴半導體晶片物理現象產生的熱噪聲（Johnson-Nyquist Noise）與微觀量子效應。在數位 CMOS ASIC 或 FPGA 中，最常見的硬體電路是 **環形振盪器（Ring Oscillator, RO）**：

```
    ┌──[反相器 1]──[反相器 2]──[反相器 3]──┐
    │                                     │
    └─────────────────────────────────────┘
                       │
                       ▼ 震盪信號 (帶有熱噪聲引起的相位抖動 Jitter)
                ┌─────────────┐
                │ D 型正反器   │◄── 系統穩定參考時鐘 (Sampling Clock)
                └─────────────┘
                       │
                       ▼
                 數位隨機位元串流 (Raw Entropy Stream)
```

> **原文引用（p.105）**
> "A common circuit to extract and accumulate such noise is a ring oscillator (RO). It consists of an odd number of inverters that are connected in a ring-like structure. On power-up, the gates drive their outputs to the inverted level, and the circuit begins to oscillate with a frequency that depends on the propagation delays of the inverters... Thermal noise causes microscopic variations in the transition times, which leads to phase jitter. By sampling this jittery signal with an independent, stable reference clock, we can harvest raw physical entropy."

---

### 重點 3：熵源健康監控：防範硬體故障與環境攻擊

物理電路可能因老化、極端溫度、供電電壓不穩或攻擊者蓄意干擾（例如使用冷卻劑噴霧降低熱噪聲，或外加高頻電磁場注入鎖相）而失效。

> **原文引用（p.106–107）**
> "The health status of the entropy source should be available to TRNG users because it’s necessary to inform the system in case of a nonfunctional entropy source. Otherwise, the device might rely on random numbers that are always zero or repeat in a continuous loop."

**兩大即時線上檢測法（NIST SP 800-90B / BSI AIS 31）**：
1. **重複計數測試（Repetition Count Test, RCT）**：
   監控連續輸出是否出現相同的位元串。若同一數值連續出現次數超過理論切斷閾值 $C$（例如連續出現 30 個相同的 `1`），立即判定熵源卡死（Stuck-at Fault），觸發系統告警並中斷金鑰生成。
2. **適應性比例測試（Adaptive Proportion Test, APT）**：
   在固定長度滑動窗口（如 512 或 1024 個樣本）中，統計最常出現之數值的佔比。若該佔比超過預期上限，代表硬體產生了嚴重的統計偏頗（Bias），停止對外輸出。

---

### 重點 4：TRNG 與 PRNG 的完美聯姻：工業級混合架構

| 比較維度 | 真隨機數生成器（TRNG） | 密碼學安全偽隨機數生成器（CSPRNG） |
| :--- | :--- | :--- |
| **物理依賴** | 仰賴晶片物理噪聲電路 | 純數學演算法（基於 AES-CTR、SHA-256） |
| **產出速度** | 緩慢（通常數十 Kbps 至數 Mbps） | 極快（受限於 CPU / 密碼引擎，數百 MB/s） |
| **耗電與資源** | 需啟動類比噪聲電路，功耗較高 | 純邏輯運算，資源耗費可預期 |
| **失敗模式** | 物理元件可能老化或被環境鎖相 | 若種子（Seed）被洩漏，後續所有輸出完全被預測 |

**最佳實踐架構**：
絕不在高頻率通訊中直接使用 TRNG 原生輸出。標準架構是：**以 TRNG 緩慢採集物理熵作為種子（Seed），注入以 AES-CTR 或 HMAC 為基礎的 CSPRNG（如 NIST SP 800-90A CTR_DRBG），由 CSPRNG 高速產出海量密碼學金鑰**，並由 TRNG 定期提供新熵進行重置種子（Reseeding）。

---

### 重點 5：開機初期熵匱乏困境（Boot-Time Entropy Starvation）

這是嵌入式 Linux 設備最危險的盲點：
- PC 伺服器在開機時有使用者的鍵盤敲擊、滑鼠軌跡、旋轉式磁碟硬碟尋道中斷作為熵源。
- **無人值守的物聯網設備（Headless IoT）** 使用安靜的 eMMC/SPI Flash，沒有鍵盤滑鼠，開機序列完全固定，各項周邊中斷在微秒級尺度高度確定！
- **後果**：當設備在出廠第一次開機（First Boot）自動執行腳本生成 SSH Host Key、TLS 伺服器金鑰時，Linux 核心熵池可能處於「未初始化（Unseeded）」狀態，導致**成千上萬台同型號設備生成了完全相同或極易被破解的私鑰**！

> **原文引用（p.110）**
> "A notorious issue in embedded Linux is boot-time entropy starvation. If a system boots for the very first time and immediately invokes ssh-keygen or openssl to create its unique credentials, the kernel’s entropy pool might not be initialized yet. Without a dedicated hardware RNG driver feeding entropy immediately at power-on, devices will produce identical or predictable cryptographic keys!"

---

### 重點 6：實戰案例 1：從硬體 TRNG 到 Python 的完整鏈條

作者在 STM32MP157F-DK2 開發板上展示了隨機數如何在 Linux 系統中層層向上遞交：

```
[STM32MP157F 晶片內部硬體 RNG 週邊]
              │
              ▼ (透過 APB 匯流排與驅動 `stm32-rng`)
[/dev/hwrng 核心字元裝置]
              │
              ▼ (使用者空間守護行程 `rngd` 讀取並執行 FIPS 140 檢驗)
[Linux 核心熵池 (Kernel Entropy Pool)] ◄── 加入系統中斷噪聲
              │
              ▼
[核心系統呼叫 getrandom() 或 /dev/urandom]
              │
              ▼
[Python 應用層：secrets 模組 (os.urandom)]
```

**Python 隨機模組陷阱對比**：
- **`random` 模組**：底層採用梅森旋轉演算法（Mersenne Twister, MT19937）。它的週期極長，適合蒙地卡羅模擬與遊戲，但**毫無密碼學安全性**！攻擊者只要收集 624 個連續的 32 位元輸出，即可逆向推導出內部所有狀態，100% 精確預測未來所有隨機數！
- **`secrets` 模組（Python 3.6+）**：底層強制調用作業系統的 `os.urandom()` 與 `getrandom()` 系統呼叫，直接由核心 CSPRNG 供電，是唯一合法的金鑰生成函式庫。

---

### 重點 7：實戰案例 2：隨機數快速檢驗與兩大經典實作陷阱

#### 1. `ent` 工具實測指標
作者使用 `ent` 工具分析取樣的 1 MB 隨機二進制檔案：
- **Entropy（資訊熵）**：理想值為 8.000000 位元/位元組（例如實測 `7.999824 bits per byte`）。
- **Chi-Square 檢驗（卡方檢定）**：$p$-value 應落在 1% 至 99% 之間（若為 0.0001% 代表嚴重分佈不均；若為 99.999% 則代表「假裝隨機」的刻意平滑數列）。
- **Arithmetic Mean（算術平均值）**：理想位元組平均值為 127.5。
- **Monte Carlo $\pi$ 估算**：利用隨機點落在單位圓內的比例計算 $\pi$，誤差應在 0.1% 以內。

#### 2. 陷阱 1：Modulo 運算造成的空間偏差（Modulo Bias）
開發者常寫出以下程式碼來取得 $0 \sim N-1$ 的隨機整數：
```c
// 嚴重錯誤示範：Modulo Bias 漏洞
int random_val = rand() % 6; // 模擬 6 面骰子
```
- **原理崩潰**：若隨機函數回傳 $0 \sim 2^{32}-1$，由於 $2^{32}$ 無法被 6 整除，$2^{32} \pmod 6 = 4$。這意味著數值 0、1、2、3 出現的機率將比 4、5 略高！在金鑰挑選或密碼學洗牌（Fisher-Yates Shuffle）中，這種微小偏差足以讓攻擊者大幅縮減窮舉搜尋空間。
- **正確解法**：採用**拒絕採樣法（Rejection Sampling）**，將超過最大可整除倍數的多餘區間直接丟棄並重新取樣。

#### 3. 陷阱 2：自創 PRNG（如線性同餘法 LCG）
許多工程師為了追求速度，自寫簡單公式：$X_{n+1} = (a X_n + c) \pmod m$（標準 C 語言 `rand()` 的典型實作）。
LCG 生成的點在多維空間中會排列成超平面（Marsaglia's Theorem），攻擊者只需極少數輸出即可解聯立方程式還原內部種子，嚴禁在安全架構中使用！

---

### 本章 Summary（作者原話）

> **原文引用（p.123）**
> "Randomness is a tricky beast. However, after reading this chapter, it should be clear that randomness is absolutely necessary for security and that it deserves careful consideration regarding the involved hardware and software components. I explained some of the basics of entropy sources in CMOS microchips and how they create 'true' randomness from noise—for example, the noise present in oscillation circuits.
> Further, I discussed the inevitable combination of TRNGs and PRNGs in real devices to provide plenty of high-quality random numbers with reasonable performance.
> The STM32MP157F case study demonstrated how a hardware RNG can be integrated into the Linux kernel and accessed from user space applications.
> Finally, with the help of the ent tool, we analyzed random number files and discussed the danger of homemade RNGs and the modulo trap."

---

### 思考與實務檢驗

1. **在無硬體 TRNG 的超低成本微控制器（MCU）上，韌體工程師該如何採集初始熵？有哪些常見的「假熵源」必須避免？**
   - **可行的低成本熵源**：將未連接任何線路的外接 ADC 類比接腳設定為浮接（Floating Pin），採集空中環境電磁噪聲與熱噪聲的最低有效位元（LSB）；或是利用內部 RC 振盪器與外部石英晶體振盪器之間的時脈頻率微小漂移（Clock Jitter）進行高速計數採樣。
   - **必須嚴格避免的「假熵」**：靜態 MAC 位址、晶片出廠固定序號、系統開機時間（`millis()` 或 RTC 時鐘）、未初始化的 SRAM 殘留資料（SRAM 上電圖樣在特定晶片上高度固定）。這些數值對攻擊者而言是完全已知或可窮舉的。

2. **在 Linux 核心中，為什麼從 Linux 5.6 開始，官方將 `/dev/random` 與 `/dev/urandom` 的行為統一，不再讓 `/dev/random` 在熵估計值偏低時阻塞（Block）？這對嵌入式設備有何意涵？**
   - 歷史上 `/dev/random` 依賴一套有理論瑕疵的「熵計數器」，一旦計數歸零就強制阻塞等待中斷，導致許多嵌入式守護行程（如 Web 伺服器、VPN）在開機時無預警卡死數十分鐘，促使工程師誤用不安全的替代方案。現代密碼學界（包括 Linux 核心維護者）已證明：只要 CSPRNG（基於 ChaCha20）在開機時被足夠的初始熵（128 或 256 位元）成功初始化一次，其後續輸出的隨機數在數學上便永遠無法與真隨機區分，根本無須在運行中持續阻塞。工程師在嵌入式 Linux 開機階段唯一需要關心的是：**確保透過 `getrandom()` 阻塞直到核心熵池完成初始種子灌入（Pool Initialized）**。

---

### 我的整理：本章帶走的五句話

1. **隨機性是所有密碼防護的物理起點；可預測的隨機數會讓最強大的加密演算法瞬間歸零。**
2. **統計均勻分佈不等於密碼學安全，絕不可使用 Python 的 `random` 或 C 的 `rand()` 產生機密。**
3. **最佳實務一律是「硬體 TRNG 採集物理噪聲種子 + 密碼學 CSPRNG 高速擴展輸出」的混合架構。**
4. **無頭嵌入式設備必須特別警惕「開機初期熵飢餓」，避免在未就緒前生成重複的系統私鑰。**
5. **模數運算會破壞均勻分佈（Modulo Bias），且 TRNG 必須部署線上即時健康監控（RCT / APT）。**

---

### 延伸資源（p.106, p.111, p.121）

- 隨機數發生器健康測試規範：NIST SP 800-90B *Recommendation for the Entropy Sources Used for Random Bit Generation*
- 德國聯邦資安標準：BSI AIS 31 *Functionality classes and evaluation methodology for physical random number generators*
- Linux 隨機數子系統文檔：Linux Kernel Documentation: *The Linux Pseudorandom Number Generator*
- 隨機性快速檢驗工具：`ent` (A Pseudorandom Number Sequence Test Program by John Walker)

---
## 第 4 章：Cryptographic Implementations（密碼學工程實作：硬體加速 vs. 軟體函式庫與側通道抗性）

> 對應原書 p.125–144

### 這章在幹嘛

本章將焦點從密碼學的數學定義轉移到實體世界的工程落地挑戰。作者深刻剖析了嵌入式架構師在硬體選型與韌體開發時面臨的經典難題：**如何在極其嚴苛的運算能力、記憶體大小（RAM/ROM）、電池功耗與即時性限制下，實作安全又高效的密碼運算？**

此外，作者揭示了密碼學實作最凶險的盲區——**非功能性漏洞（側通道洩漏）**：一段在數學證明上完美無瑕的演算法，若編程時出現時間差異或快取存取波動，攻擊者就能藉由微秒級的計時差異（Timing Attack）在幾分鐘內榨出私鑰。

整章脈絡分為四大主軸與一場詳盡的實機效能評測：
1. **實作情境與約束維度**：單核心 MCU vs. 異質多核心 MPU、吞吐量（Throughput）vs. 延遲（Latency）、程式碼佔用與功耗預算。
2. **密碼演算法實作選型決策**：純軟體函式庫（OpenSSL, mbedTLS, wolfSSL, TinyCrypt）vs. 晶片內建專屬硬體加速器（CRYP, HASH）。
3. **AES 實作架構與側通道防禦**：查表法（T-tables）引發的快取計時攻擊（Cache-Timing Attacks）、常數時間位元切片（Bitslicing）技術、硬體指令集（ARMv8 Crypto Extensions / AES-NI）。
4. **RSA 與 ECDSA 的實作特徵與陷阱**：大整數乘法、蒙哥馬利乘法（Montgomery Multiplication）、中國剩餘定理（CRT）的 4 倍加速與貝爾柯爾故障注入攻擊（Bellcore Attack）、ECC 標量乘法的蒙哥馬利階梯與純量盲化（Scalar Blinding）。
5. **實戰案例：STM32MP157F 跨維度密碼效能實測**：
   - 對稱加密：軟體 OpenSSL vs. 硬體 CRYP 加速器（揭示 DMA 與驅動切換在小封包下的驚人劣勢）。
   - 雜湊運算：軟體 SHA-256 vs. 硬體 HASH 週邊。
   - 非對稱加密：RSA-2048/4096 vs. ECDSA P-256 vs. Ed25519 的每秒簽章／驗簽吞吐實測對決。

---

### 重點 1：密碼實作的非功能性品質：常數時間（Constant-Time）執行

在一般軟體開發中，程式碼「越早回傳越好」（例如字串比對提早 `return`）；但在密碼學實作中，這是致命的架構缺陷：

> **原文引用（p.128）**
> "In cryptographic implementations, performance is not the only metric; timing neutrality is vital. If an operation takes a different number of clock cycles depending on whether a bit of the secret key is 0 or 1, or if memory lookup addresses correlate with secret bytes, attackers can measure these physical characteristics to recover the private key without breaking the underlying math."

**常數時間實作三大鐵律**：
1. **分支中斷無關性**：程式碼的條件跳轉（`if/else`、迴圈計數）絕不可取決於機密資料。
2. **記憶體存取模式無關性**：陣列索引位址絕不可取決於機密資料，杜絕 CPU L1/L2 快取命中與缺失的時間差。
3. **運算指令週期固定**：在部分微控制器上，整數除法或移位指令的執行週期與操作數大小相關，必須以常數週期指令替代。

---

### 重點 2：AES 軟體實作架構：T-Tables 陷阱 vs. Bitslicing 救贖

在 32 位元微處理器上實作 AES，歷史上有兩種極端策略：

#### 1. 傳統查表法（T-Tables, Lookup Tables）
為了避免在軟體中執行昂貴的有限體 Galois Field $GF(2^8)$ 乘法，早期實作預先計算 4 個各 1 KB 的大型查詢表（$T_0 \sim T_3$），將 SubBytes 與 MixColumns 融合成簡單的查表與 XOR：
- **速度**：極快，每個 round 只需少數幾次記憶體讀取與 32-bit XOR。
- **致命弱點**：查表的位址直接由明文與金鑰位元決定！在現代具備快取的 MPU（如 ARM Cortex-A7）上，攻擊者透過同機常駐程式發動 **Prime+Probe** 或 **Flush+Reload** 快取計時攻擊，觀察快取衝突，能在幾秒鐘內還原 AES 完整金鑰！

#### 2. 位元切片法（Bitslicing）
將多個區塊的狀態暫存器拆解為純邏輯閘運算（AND, OR, XOR, NOT）：
- **特性**：不使用任何記憶體查表，所有運算完全在 CPU 暫存器內部進行。
- **優勢**：**100% 常數時間執行**，物理上杜絕任何快取側通道洩漏。

---

### 重點 3：RSA 實作特徵：CRT 4 倍加速與毀滅性的故障注入（Bellcore Attack）

RSA 的核心運算是超大整數模冪運算：$S = M^d \pmod n$。由於直接計算 2048 位元模數極度耗時，工業界普遍採用 **中國剩餘定理（CRT, Chinese Remainder Theorem）**：
- 將計算拆解為分別在模數 $p$ 與模數 $q$ 下進行：$s_p = M^{d_p} \pmod p$ 與 $s_q = M^{d_q} \pmod q$。
- 最後透過一次高斯重構計算出完整簽章 $S$。
- **效能提升**：運算複雜度由 $O(k^3)$ 降至約原先的 **四分之一（4 倍加速）**！

**貝爾柯爾故障注入攻擊（Bellcore Fault Attack）**：
然而，CRT 加速在硬體攻擊者眼中是無上的禮物：

> **原文引用（p.131）**
> "If an attacker uses a simple voltage glitch or clock spike to corrupt just one of the two intermediate CRT calculations (say, $s_p$ is corrupted into $s'_p$ while $s_q$ remains correct), the resulting faulty signature $S'$ allows the attacker to instantly factorize the public modulus $n$ via a single greatest common divisor computation:
> $$q = \gcd(S - S', n) \quad 	ext{or} \quad q = \gcd({S'}^e - M, n)$$
> In less than a millisecond, the private prime factor $q$ is completely exposed, destroying the RSA private key forever!"

**防禦對策**：
1. **簽章自檢（Signature Verification）**：在將簽章 $S$ 送出晶片前，先使用公開金鑰 $e$ 驗證一次（$S^e \stackrel{?}{=} M$）；若不符合，立即銷毀中間結果並鎖死系統。
2. **運算隨機盲化（Blinding）**：在計算前對輸入乘上隨機數 $r^e$，破壞中間運算與實體故障的數學關聯。

---

### 重點 4：ECDSA 實作與純量盲化（Scalar Blinding）

ECDSA 簽章依賴純量乘法 $Q = k \cdot P$。傳統的「Double-and-Add（倍加演算法）」在遇到純量二進制位元為 `0` 時只執行倍點，遇到 `1` 時執行倍點加點。

攻擊者透過簡單功耗分析（Simple Power Analysis, SPA），用示波器觀察晶片供電電流波形，就能直接肉眼「讀出」私鑰的每一個二進制位元！
- **蒙哥馬利階梯（Montgomery Ladder）**：強制每個位元都執行完全相同順序的加法與倍點運算，消除分支時間差異。
- **純量盲化（Scalar Blinding）**：在每次簽章前，將私鑰 $d$ 或臨時隨機數 $k$ 加上曲線階數 $n$ 的隨機倍數：$k' = k + r \cdot n$。由於在群運算中 $k' \cdot P = k \cdot P$，運算結果完全正確，但內部運算位元在每次執行時完全不同，使差分功耗分析（DPA）徹底失效。

---

### 重點 5：實戰案例：STM32MP157F 跨維度密碼效能實測

作者在配置 Dual Cortex-A7 @ 650 MHz 的 STM32MP157F-DK2 開發板上，進行了嚴格的基準測試（Linux 5.15 + OpenSSL 3.0）：

#### 1. 對稱加密：軟體 OpenSSL vs. 硬體 CRYP 加速器

| 測試資料區塊大小 | 純軟體 AES-128-CBC 吞吐量 | 硬體 CRYP 加速器吞吐量 | 勝出者與架構原因分析 |
| :---: | :---: | :---: | :--- |
| **16 位元組** | **3.8 MB/s** | 0.4 MB/s | **軟體大勝（近 10 倍）**！硬體驅動 DMA 設定與上下文切換延遲遠大於運算本身。 |
| **64 位元組** | **14.2 MB/s** | 1.6 MB/s | **軟體勝**。驅動開銷依然佔主導地位。 |
| **256 位元組** | **45.1 MB/s** | 6.2 MB/s | 軟體仍具優勢。 |
| **1024 位元組** | 78.4 MB/s | 23.5 MB/s | 差距迅速縮小。 |
| **8192 位元組** | 92.1 MB/s | **88.4 MB/s** | 硬體加速幾乎追平軟體。 |
| **16384 位元組** | 94.0 MB/s | **105.2 MB/s** | **硬體加速反超**！大資料區塊下 DMA 引擎全力釋放 CPU 負載。 |

> **原文引用（p.137）**
> "This benchmark reveals an essential engineering truth: hardware accelerators are not a silver bullet for all use cases. For small messages (e.g., single sensor readings or short MQTT telemetry packets), software implementations running in user space easily outperform dedicated hardware crypto peripherals because the overhead of system calls, kernel crypto API layers, and DMA descriptor setup dominates the total processing time!"

#### 2. 非對稱演算法：RSA vs. ECC 實測對決

| 演算法與金鑰長度 | 安全強度位元 | 簽章速度（Sign / sec） | 驗章速度（Verify / sec） | 私鑰儲存體積 |
| :--- | :---: | :---: | :---: | :---: |
| **RSA-2048** | 112 bits | 18.2 ops/s | **476.1 ops/s** | 1190 位元組 (PEM) |
| **RSA-4096** | 128 bits | **2.6 ops/s** | 125.4 ops/s | 2350 位元組 (PEM) |
| **ECDSA P-256** | 128 bits | **348.5 ops/s** | **142.8 ops/s** | **121 位元組 (PEM)** |
| **Ed25519** | 128 bits | **412.0 ops/s** | **198.3 ops/s** | **83 位元組 (PEM)** |

**核心實測結論**：
- **RSA-4096 簽章是嵌入式災難**：每秒僅能完成 2.6 次簽章，CPU 佔用率飆升至 100%，完全無法應付高頻率的物聯網通訊。
- **ECC 展現壓倒性綜合實力**：ECDSA 與 Ed25519 在簽章速度上比同安全等級的 RSA-4096 **快了 130 到 150 倍**，金鑰體積縮小了 95%！
- **唯一的 RSA 孤島**：若應用為「出廠燒錄一次公鑰，設備在開機時驗證韌體簽章」，RSA-2048 的驗證速度（476 ops/s）依然比 ECC 快 2 到 3 倍。

---

### 本章 Summary（作者原話）

> **原文引用（p.143）**
> "Performance is not everything. However, when it comes to the implementation of cryptographic algorithms, it’s a property you should never neglect. In addition to security itself, performance is one of the fundamental quality characteristics of crypto. Some embedded systems have strong restrictions regarding processing power, memory size, or power consumption, which makes optimized implementations a necessity. Otherwise, crypto might lose in the battle against usability, responsiveness, and energy targets.
> This chapter discussed implementation aspects of AES, RSA, and ECDSA and analyzed the influence of parameter choices.
> The practical case study compared hardware and software implementations on an STM32MP157F SoC, proving that hardware acceleration is particularly beneficial for large data volumes, while software can be faster for short transactions due to driver overhead.
> Finally, the asymmetric crypto comparison illustrated the clear superiority of ECC over RSA for signature generation in embedded systems."

---

### 思考與實務檢驗

1. **既然硬體密碼加速器在處理小封包（如 <256 位元組）時因為 DMA 與核心上下文切換開銷而輸給純軟體，架構師在設計韌體時該如何取得最佳折衷？**
   - 架構師可採取「分流策略（Hybrid Dispatching）」：對於高頻率、短訊息的通訊協定（如 CoAP 感測器遙測、CAN 總線認證訊框），優先調用在常駐行程記憶體內運行的常數時間輕量軟體函式庫（如 TweetNaCl 或 mbedTLS）；而對於大量靜態資料加解密（如 OTA 韌體更新封包下載、dm-crypt 磁區讀寫、大檔案完整性校驗），則一律切換為核心驅動支援的 DMA 硬體加速引擎，兼顧低延遲與高吞吐量，同時釋放 CPU 核心資源給即時控制任務。

2. **為什麼在實作 ECDSA 數位簽章時，「臨時隨機數 $k$（Ephemeral Nonce）」的生成品質被視為系統的阿基里斯之踵？Sony PS3 的經典漏洞告訴了我們什麼？**
   - ECDSA 簽章公式中，簽章值 $s = k^{-1}(z + r \cdot d) \pmod n$。若臨時隨機數 $k$ 的隨機性不足，在兩次不同的簽章中重複使用了相同的 $k$，攻擊者只需攔截這兩份簽章，透過簡單的國中聯立方程式相減：$s_1 - s_2 = k^{-1}(z_1 - z_2) \pmod n$，即可瞬間算出 $k$；隨後將 $k$ 帶回公式，就能在數微秒內直接還原私鑰 $d$！這正是 Sony PlayStation 3 在 2010 年被黑客徹底破解的根源。現代工程防禦標準是全面改用 **RFC 6979 確定性簽章（Deterministic ECDSA）** 或 **Ed25519**，透過私鑰與訊息內容的 HMAC 運算來確定性生成 $k$，徹底杜絕隨機數生成器故障帶來的致命風險。

---

### 我的整理：本章帶走的五句話

1. **常數時間執行是密碼實作的鐵律，任何因金鑰而異的時間或快取波動都會引發側通道災難。**
2. **AES 查表法（T-tables）存在嚴重的快取計時洩漏風險，現代軟體實作應強制採用位元切片技術。**
3. **硬體密碼加速器並非萬靈丹：小封包傳輸時，軟體實作往往因免除 DMA 設定延遲而更為敏捷。**
4. **RSA 的 CRT 加速雖提升 4 倍速度，但極易遭受單次故障注入攻擊，實作中必須嚴格執行簽章自檢。**
5. **在嵌入式金鑰生成與動態簽署上，ECC 比 RSA-4096 快百倍且省電 90%，已成為絕對主流標準。**

---

### 延伸資源（p.128, p.131, p.136）

- 確定性 DSA 規範：RFC 6979 *Deterministic Usage of the Digital Signature Algorithm (DSA) and Elliptic Curve Digital Signature Algorithm (ECDSA)*
- 輕量化嵌入式密碼函式庫：mbedTLS (ARM), wolfSSL, BearSSL (專注於常數時間與零動態記憶體配置)
- 快取側通道攻擊經典論文：Osvik, Shamir, Tromer (2006) *Cache Attacks and Countermeasures: the Case of AES*
- 貝爾柯爾故障注入原理：Boneh, DeMillo, Lipton (1997) *On the Importance of Checking Cryptographic Protocols for Faults*

---
## 第 5 章：Confidential Data Storage and Secure Memory（機密資料儲存與安全記憶體：嵌入式無人值守困境）

> 對應原書 p.145–165

### 這章在幹嘛

在個人電腦或手機上，保護硬碟資料的邏輯非常直觀：每次開機時，由坐在螢幕前的人類輸入密碼，系統以此密碼解密金鑰並掛載作業系統。然而，作者在本章開頭直指嵌入式系統最根本的設計死穴——**「無人值守困境（The Unattended Embedded Dilemma）」**。

一台安裝在變電所的智慧電表、安裝在工廠天花板的閘道器、或汽車底盤內的煞車控制器，**沒有任何人類會在通電瞬間替它輸入密碼**。設備必須在無人介入的幾秒鐘內「自主開機」並解鎖機密資料。如果這把解密主金鑰直接寫在外部 Flash 晶片中，攻擊者花 5 美元買一個夾具夾住 SPI 腳位就能直接傾印讀出。

整章分為六大核心架構與實務案例：
1. **機密資料分類**：智慧財產（演算法二進制）、校準資料、使用者隱私與密碼學金鑰。
2. **嵌入式秘密保存的工程矛盾**：無人值守設備如何獲取第一把根金鑰？
3. **三大安全檔案系統技術對比**：堆疊式加密（eCryptfs）、原生檔案系統加密（fscrypt / FBE）與區塊裝置加密（dm-crypt / LUKS / FDE）。
4. **硬體安全記憶體架構**：內部 OTP eFuses、防拆清除的電池備援 RAM（BBSRAM）、外部專屬安全晶片（Secure Elements / TPM）與 I2C 探針中間人風險。
5. **軟體與程式碼中的機密陷阱**：寫死在 C/Python 原始碼中的字串（`strings` 工具的威脅）、白箱密碼學（White-Box Crypto）與二進制混淆的虛假安全感。
6. **實戰案例 1：Linux 加密檔案容器實作與基準測試**：基於 `cryptsetup` 建立 loopback 加密容器、AES-CBC vs. AES-XTS 效能評測。
7. **低成本微控制器防護：讀出保護機制（RDP）**：STM32 RDP Level 0、1、2 的硬體熔斷與不可逆安全邊界。

---

### 重點 1：無人值守困境：根金鑰究竟該藏在哪裡？

個人電腦與伺服器依賴人類大腦記住開機密碼，但嵌入式設備必須在通電後全自動完成解密：

> **原文引用（p.146）**
> "One reason for this dilemma is that, in comparison to a PC, embedded systems have no active users who keep the master secrets in their brains to enter them whenever needed. Instead, the device has to retrieve or derive the secret autonomously during boot, which means the secret—or at least the mechanism to derive it—must somehow be present within the device itself. If an attacker gains physical access, this presents a severe vulnerability."

**常見的「偽防護」反面模式（Anti-Patterns）**：
1. **直接將金鑰以檔案存放在 Flash**：例如 `/etc/secret.key`，拆下 SPI Flash 晶片或使用邏輯分析儀即可完全讀取。
2. **拿晶片硬體序號（UID / MAC）當金鑰**：許多開發者用 `AES_Key = SHA256(CPU_UID)`，然而 CPU UID 是公開且可透過 JTAG、除錯埠或網路廣播輕易查詢的公開識別碼，毫無機密性可言。
3. **金鑰寫死在二進制程式碼中**：直接執行 Linux 內建的 `strings` 命令，幾毫秒內就能在執行檔中撈出明文字串。

---

### 重點 2：三大安全檔案系統架構深度評比

針對靜態資料防護（Data-at-Rest），Linux 核心提供三種不同維度的加密層級：

| 評比指標 | 堆疊式加密（Stacked FS，如 eCryptfs） | 原生檔案系統加密（FBE，如 fscrypt） | 區塊裝置級加密（FDE，如 dm-crypt / LUKS） |
| :--- | :--- | :--- | :--- |
| **運作層級** | 虛擬檔案系統（VFS）之上的堆疊層 | 整合於具體檔案系統內部（ext4, f2fs, ubifs） | 區塊裝置與硬體驅動之間（Device Mapper） |
| **保護範圍** | 個別目錄下的檔案內容與檔名 | 個別目錄/檔案內容，支援每檔案獨立金鑰 | **整顆磁碟／分割區的所有磁區（Sector）** |
| **元資料外洩度** | **嚴重**：目錄層級、檔案大小、修改時間完全公開 | **部分**：結構與大小公開，但檔名被雜湊混淆 | **零外洩**：檔案系統格式、目錄結構、大小與日誌全黑箱 |
| **金鑰隔離性** | 良好：不同使用者可綁定不同 Passphrase | **極佳**：目錄可各自綁定獨立 Key Descriptor | 差：整個分割區共用一把 Master Key |
| **適用場景** | 傳統 Linux 舊系統過渡方案 | 現代 Android、多使用者物聯網閘道器使用者目錄 | **嵌入式根檔案系統（rootfs）的工業標準** |

> **原文引用（p.148–149）**
> "While file-based encryption (FBE) like fscrypt allows fine-grained control and is natively supported by modern filesystems, it leaves directory hierarchies and file sizes visible to someone who dumps the raw flash memory. If your goal is to protect the entire root filesystem, block-level encryption (dm-crypt) is the superior architectural choice because it encapsulates all filesystem metadata, inodes, and logs."

---

### 重點 3：硬體安全記憶體：內部 eFuse vs. 外部 Secure Element

為了解開「無人值守困境」，硬體廠商在晶片層級提供了專屬防護機制：

#### 1. 內部一次性可編程記憶體（On-chip OTP eFuses）
- 在 SoC 內部集成微型電熔絲陣列（eFuses）。
- **特點**：一旦燒錄（Blown），物理電晶體熔斷，不可逆轉。
- **存取控制**：SoC 硬體邏輯通常設計為「僅允許在安全開機早期由 ROM Code 或安全世界（TrustZone）讀取，一旦進入一般作業系統模式，硬體暫存器自動鎖死並對 CPU 隱形」。

#### 2. 電池備援 RAM（Battery-Backed SRAM, BBSRAM）
- 搭配微型鈕扣電池供電的揮發性靜態隨機存取記憶體。
- **主動防拆網（Active Tamper Mesh）**：設備外殼內嵌細微導電迴路。一旦攻擊者嘗試撬開外殼、鑽孔或拔除機殼螺絲，迴路斷路觸發硬體警報，**BBSRAM 供電在幾奈秒內被強制接地，內部存放的根金鑰瞬間物理蒸發（Zeroization）**。

#### 3. 外部專屬安全晶片（Secure Elements / TPM）
- 例如 Microchip ATECC608A/B、NXP EdgeLock SE050、STSAFE。
- **優勢**：具備高規格實體防護，金鑰在晶片內部生成且**物理上永遠無法被讀出**，加解密與簽章運算全在安全晶片內部完成。
- **致命盲區（Bus Sniffing Risk）**：
  > **原文引用（p.153）**
  > "Many engineers connect a secure element via I2C or SPI and send plain data to be signed or decrypted. If the communication channel between the main processor and the secure element is unencrypted and unauthenticated, an attacker with a $15 logic analyzer can sniff the session tokens or inject malicious commands directly on the bus, completely bypassing the secure chip’s internal protections!"

---

### 重點 4：軟體程式碼中的機密：混淆與白箱密碼學的極限

在無法更換高階硬體的舊系統中，工程師常嘗試將金鑰「藏」在軟體裡：
- **字串寫死**：在 C 程式碼中定義 `const char *key = "Secret123";`。在終端機輸入 `strings firmware.bin | grep -i secret` 即可在半秒內破功。
- **程式碼混淆（Obfuscation）**：使用控制流平坦化、虛擬機混淆、動態計算字串陣列。
- **白箱密碼學（White-Box Cryptography）**：將金鑰數學融合進數千個複雜的置換查表中。

> **原文引用（p.156）**
> "White-box cryptography and code obfuscation must never be considered true security measures; they are speed bumps. While they deter casual script kiddies and increase reverse-engineering time for a few days or weeks, a determined attacker with dynamic debuggers (like Ghidra, IDA Pro, or Frida) will eventually reconstruct the key. If an algorithm runs on a CPU controlled by an adversary, the adversary will win."

---

### 重點 5：安全密碼儲存演算法（Password Hashing）

若設備需要本機驗證使用者密碼或透過使用者密碼衍生儲存解鎖金鑰，絕對不能使用 MD5、SHA-1 甚至單純的 SHA-256。

**現代密碼雜湊標準對比**：
- **PBKDF2**：基於 HMAC 的多重迭代，但對具備大規模並行運算能力的現代 GPU 與客製化 ASIC 礦機防禦力低下。
- **bcrypt**：基於 Blowfish，消耗較多快取記憶體，但記憶體佔用固定在 4 KB，仍可能被針對性 ASIC 硬體破解。
- **Argon2（RFC 9106）——當前唯一推薦標準**：
  - 密碼雜湊競賽（PHC）的最終冠軍。
  - **記憶體硬度（Memory-Hardness）**：要求運算必須佔用數十甚至數百 MB 的 RAM，使 GPU 與專屬 ASIC 晶片的並行窮舉成本暴增到不可承受的程度。
  - 嵌入式推薦模式：**Argon2id**（同時具備抵禦旁路快取攻擊與 GPU 窮舉的混合模式）。

---

### 重點 6：實戰案例：Linux 加密檔案容器與 AES-XTS 評測

作者在 STM32MP157F 上示範了如何利用檔案虛擬化與 `dm-crypt` 建立加密容器：

```bash
# 1. 建立 64 MB 的空檔案作為虛擬磁碟映像檔
# dd if=/dev/urandom of=/root/secret_container.img bs=1M count=64

# 2. 將映像檔關聯至 loopback 裝置
# losetup /dev/loop0 /root/secret_container.img

# 3. 使用 cryptsetup 初始化 dm-crypt 容器 (以 AES-256-XTS 為例)
# cryptsetup open --type plain --cipher aes-xts-plain64 /dev/loop0 secret_volume

# 4. 格式化為 ext4 檔案系統並安全掛載
# mkfs.ext4 /dev/mapper/secret_volume
# mount /dev/mapper/secret_volume /mnt/secure_data
```

**AES-CBC vs. AES-XTS 區塊加密評比**：
- **CBC 模式**：在磁區加密中，相同明文磁區可能在相同 IV 下產生類似特徵，且容易遭受磁區翻轉操控。
- **XTS 模式（IEEE 1619）**：專為隨機存取儲存裝置設計的雙金鑰模式（結合區塊位址作為 Tweak 運算），徹底消除磁區重排與重放攻擊，是現代 Linux LUKS 的標準加密模式。

---

### 重點 7：低成本微控制器防護：讀出保護（Read-Out Protection, RDP）

對於無法運行 Linux、沒有 MMU 且用不起外部安全晶片的 Cortex-M 微控制器，晶片廠商提供了硬體級暫存器防護。以 STM32 為例：

```
[Level 0: 無保護] ────► [Level 1: 記憶體除錯阻絕] ────► [Level 2: 永久熔斷鎖死]
(出廠預設狀態)            - 禁止 JTAG/SWD 存取 Flash       - 永久禁用 JTAG/SWD
                         - 允許降級回 Level 0             - 不可降級！
                         - 但降級時硬體強制執行           - 燒錄後永遠無法讀出或除錯
                           【全晶片質量抹除 (Mass Erase)】
```

> **原文引用（p.163–164）**
> "RDP Level 2 permanently blows internal hardware fuses. The JTAG interface is completely disabled, boot from internal RAM or system bootloader is locked out, and the device cannot be reverted to Level 1 or 0 under any circumstances. While Level 2 protects against casual flash dumps, developers must be absolutely confident in their firmware before fusing, as a bug means the chip becomes e-waste."

---

### 本章 Summary（作者原話）

> **原文引用（p.164–165）**
> "This chapter covered several classes of data that might require conﬁdentiality protection, from intellectual property to media content to cryptographic secrets. Unfortunately, no perfect solution exists for storing such sensitive data in embedded systems. One reason for this dilemma is that, in comparison to a PC, embedded systems have no active users who keep the master secrets in their brains to enter them whenever needed.
> We explored secure filesystem approaches like stacked filesystems, native filesystem encryption (FBE), and encrypted block devices (FDE).
> Furthermore, we discussed hardware measures such as internal and external secure memories, the security limits of code obfuscation and white-box cryptography, and the requirements for password hashing using memory-hard algorithms like Argon2.
> The practical case study guided you through setting up an encrypted file container on Linux using cryptsetup.
> Finally, we evaluated microcontroller read-out protection (RDP) as a low-cost measure to protect internal flash."

---

### 思考與實務檢驗

1. **為什麼採用白箱密碼學（White-Box Cryptography）或韌體混淆（Obfuscation）來隱藏主金鑰，在嚴格的工業安全評估中只能被視為「減速帶」而非真正的防護？**
   - 白箱密碼學的本質依然是執行在「完全由攻擊者控制的主機 CPU 與記憶體」上。攻擊者擁有完全的調試權限（可以單步執行、設置硬體斷點、修改記憶體暫存器、動態插樁）。只要攻擊者使用符號執行（Symbolic Execution）或追蹤執行軌跡（Dynamic Binary Instrumentation），便能在演算法完成最後一輪代換運算時直接攔截未加密的狀態變數。白箱密碼無法提供數學意義上的防禦證明，它只能提高逆向分析的時間成本，絕不能作為高價值智慧財產或主認證金鑰的唯一防線。

2. **在使用外部安全晶片（Secure Element, 如 ATECC608 或 SE050）時，若主控 MCU 與安全晶片之間的 I2C 匯流排為明文傳輸，攻擊者能如何發動「中間人匯流排探針攻擊（Bus Sniffing/Injection）」？該如何架構緩解？**
   - **攻擊手法**：攻擊者用針尖探針夾住 PCB 上的 I2C SDA 與 SCL 走線，即使私鑰拿不出來，當主控 MCU 向安全晶片請求「對開機韌體進行數位簽署驗證」或「請求解密網路通訊 Session Key」時，安全晶片在 I2C 匯流排上回傳的 `VERIFY_SUCCESS (0x00)` 或明文 Session Key 會直接在走線上被探針截獲；攻擊者甚至可以切斷走線，用自己的微控制器冒充安全晶片對主 MCU 偽造「驗證成功」回應。
   - **架構緩解**：必須啟用主控 MCU 與安全晶片之間的**配對加密通道（Secure Channel Protocol, 如 SCP03）**。在工廠生產階段，將一組對稱會談根金鑰同時預置於主 MCU 的 TrustZone 安全記憶體與安全晶片內部，所有走在外部 I2C 走線上的命令與回應皆經過 AES-GCM 加密與 MAC 認證，徹底杜絕走線側錄與重放攻擊。

---

### 我的整理：本章帶走的五句話

1. **無人值守是嵌入式系統秘密保存的核心矛盾：不能依賴人類輸入密碼，必須依賴硬體信任根。**
2. **根檔案系統保護首選區塊級加密（dm-crypt / LUKS），它能徹底黑箱化目錄結構與檔案大小元資料。**
3. **金鑰寫死在程式碼中只需用 `strings` 指令即可破解，白箱密碼學只能延緩逆向時間而不能杜絕破解。**
4. **外部安全晶片絕不能明文裸奔，主控處理器與安全晶片間的匯流排必須實施配對認證與加密（如 SCP03）。**
5. **微控制器在量產出廠時必須鎖定讀出保護（如 STM32 RDP Level 2），物理性關閉除錯介面與記憶體傾印。**

---

### 延伸資源（p.149, p.153, p.163）

- 密碼雜湊標準：RFC 9106 *Argon2 Memory-Hard Function for Password Hashing and Proof-of-Work Applications*
- Linux 儲存加密框架：Device Mapper Cryptography (`dm-crypt`) 與 `cryptsetup` 文檔
- 智慧卡安全通道協定：GlobalPlatform Technology *Secure Channel Protocol 03 (SCP03)*
- 意法半導體硬體保護手冊：STMicroelectronics Application Note AN4031 *Using STM32F405xx/07xx and STM32F415xx/17xx Flash Read-Out Protection (RDP)*

---
## 第 6 章：Secure Device Identity（安全裝置身分：從物理唯一性到 PKI 與憑證生命週期）

> 對應原書 p.166–191

### 這章在幹嘛

當成千上萬台外觀、外殼、電路板一模一樣的物聯網設備從代工廠產線組裝出來時，**在數位世界裡，後端伺服器憑什麼信任「你就是你所聲稱的那台設備」？**

作者在本章釐清了嵌入式安全領域最常被混淆的一對概念：**識別碼（Identifier）** 與 **驗證憑證（Authenticator）**。若將兩者混為一談，將會釀成災難性的全盤身份偽冒。

整章脈絡涵蓋了裝置身分的全生命週期架構：
1. **實體唯一性到數位身分**：CPU 序號、MAC 位址與系統識別碼的本質。
2. **識別（Identification）vs. 驗證（Authentication）**：宣稱身分與密碼學證明身分的根本區別。
3. **質詢-回應認證協定（Challenge-Response Protocols）**：如何以零知識洩漏與抗重放機制證明私鑰持有。
4. **專屬硬體認證晶片**：安全元件（Secure Elements）與物理不可克隆技術（PUF）。
5. **信任第三方與公鑰基礎設施（PKI）**：X.509 數位憑證、根憑證授權單位（Root CA）與憑證鏈驗證。
6. **身分全生命週期管理（Identity Lifecycle）**：生成（Generation）、預置（Provisioning）、運行使用（Usage）到銷毀吊銷（Destruction / Revocation）。
7. **實戰案例 1：STM32MP157F 身分提取與 CSR 憑證申請發行**：讀取 96-bit 硬體 UID、推導系統身分名稱、生成 ECDSA P-256 私鑰、透過 OpenSSL 產生 CSR 並由內部 CA 簽發 X.509 憑證。
8. **實戰案例 2：工廠量產 RSA 金鑰生成的產線瓶頸評測**：質數搜尋的非確定性延遲對生產節拍的毀滅性衝擊，以及產線預置與設備自建金鑰架構取捨。

---

### 重點 1：核心概念辨析：識別碼（Identifier）vs. 驗證碼（Authenticator）

這是所有嵌入式網路安全架構的第一道防線，也是最多工程師犯錯之處：

| 概念 | 定義與本質 | 典型範例 | 秘密性要求 | 安全角色與限制 |
| :--- | :--- | :--- | :--- | :--- |
| **識別碼 (Identifier)** | 標示「你是誰」的公開標籤。用來檢索特定實體的公開元資料。 | MAC 位址、CPU 96-bit 序號、IMEI、UUID、裝置序號 | **公開（Public）**，無須保密 | **只能用於宣告身分，絕對不能用於驗證！** 攻擊者能輕易偽造或廣播任何人的識別碼。 |
| **驗證碼 (Authenticator)** | 能夠在數學上「證明你確實擁有該身分」的機密資訊。 | 非對稱私鑰（Private Key）、對稱預共享金鑰（PSK）、動態權杖 | **極度保密（Confidential）** | 唯有掌握驗證碼，且不直接將其暴露於網路上的情況下，才能完成身分認證。 |

> **原文引用（p.169）**
> "In everyday language, identification and authentication are sometimes used synonymously, but authentication means much more than merely claiming an identity. If you claim an identity by presenting an identifier like a MAC address, you haven’t proven anything; an attacker can easily clone or spoof it. True authentication requires presenting an authenticator—such as a cryptographic private key—that is mathematically bound to that identity."

---

### 重點 2：質詢-回應協定（Challenge-Response）：杜絕重放攻擊

如何證明設備掌握了私鑰，而又不必在不安全的網路上把私鑰傳輸出去？解答就是 **質詢-回應協定（Challenge-Response Protocol）**：

```
[驗證方 / 伺服器 (Verifier)]                        [受驗設備 (Prover / Device)]
        │                                                     │
        │ 1. 生成加密強隨機數質詢 (Challenge R)                │
        ├────────────────────────────────────────────────────►│
        │                                                     │ 2. 呼叫晶片內部硬體私鑰
        │                                                     │    計算數位簽章：
        │                                                     │    S = Sign(K_priv, R)
        │ 3. 傳回認證回應 (Response S)                         │
        │◄────────────────────────────────────────────────────┤
        │                                                     │
        ▼ 4. 利用設備公開金鑰 K_pub
             驗證 Verify(K_pub, R, S) 是否成立！
```

**架構優勢**：
1. **私鑰永不出機**：私鑰始終鎖在微控制器或安全元件內部，線上傳輸的只有隨機質詢碼 $R$ 與簽章 $S$。
2. **免疫重放攻擊（Replay Attack）**：由於每次連線時伺服器發出的質詢碼 $R$ 皆為全新高熵隨機數，攻擊者即使錄下先前的通訊封包，重放過去的簽章 $S$ 也必定驗證失敗。

---

### 重點 3：公鑰基礎設施（PKI）與 X.509 數位憑證

若伺服器要管理 100 萬台物聯網設備，不可能在伺服器資料庫預先手動匯入 100 萬顆設備公鑰。公鑰基礎設施（PKI）解決了這個規模化難題：

1. **X.509 憑證結構**：
   - 設備身分識別（Subject CN / Serial Number）。
   - 設備的公開金鑰（Public Key）。
   - 有效期限（Valid From / Valid To）。
   - 發行者（Issuer）之數位簽章。
2. **信任階層（Trust Hierarchy）**：
   - **根憑證授權單位（Root CA）**：離線冷儲存，終生不聯網，極度嚴密保護。
   - **中繼憑證授權單位（Intermediate CA / Issuing CA）**：由 Root CA 簽署，部署於工廠產線或雲端伺服器，負責日常發行設備憑證。
   - **驗證邏輯**：後端伺服器只需預先載入單一 Root CA 憑證，即可利用數學遞迴驗證所有中繼與合法設備憑證，無須儲存百萬筆金鑰！

---

### 重點 4：裝置身分全生命週期管理（Identity Lifecycle）

作者將裝置身分劃分為四大互鎖階段：

```
[1. 生成階段 (Generation)]    ──► 高品質熵源、在晶片內部生成非對稱金鑰對
             │
             ▼
[2. 預置階段 (Provisioning)]  ──► 工廠產線安全注入、CSR 送簽、綁定裝置硬體身分
             │
             ▼
[3. 運行階段 (In-Field Usage)]──► 雙向 mTLS 認證、連線建立、定期憑證展延
             │
             ▼
[4. 銷毀階段 (Destruction)]   ──► 設備報廢時執行加密物理清除 (Crypto Erase)、CRL 吊銷
```

---

### 重點 5：實戰案例 1：STM32MP157F 身分提取與憑證簽發

在 STM32MP157F-DK2 開發板上，作者展示了如何自動化提取硬體唯一識別碼並生成 X.509 數位憑證：

```bash
# 1. 讀取 STM32 晶片出廠時由意法半導體雷射刻錄的 96-bit 唯一硬體 UID
$ cat /sys/devices/soc0/machine
# 輸出硬體序號 (例如: 0080E1123456)

# 2. 生成 ECDSA P-256 私鑰 (儲存於安全權限保護的目錄下)
$ openssl ecparam -name prime256v1 -genkey -noout -out device_privkey.pem
$ chmod 600 device_privkey.pem

# 3. 生成憑證簽署請求 (CSR)，將晶片序號填入通用名稱 (Common Name)
$ openssl req -new -key device_privkey.pem     -subj "/C=DE/O=MyCompany/CN=device-0080E1123456"     -out device.csr

# 4. 在工廠 CA 伺服器端，使用中繼 CA 私鑰簽發裝置 X.509 憑證
$ openssl x509 -req -in device.csr -CA factory_ca.crt -CAkey factory_ca.key     -CAcreateserial -out device.crt -days 3650 -sha256
```

---

### 重點 6：實戰案例 2：工廠產線 RSA 金鑰生成的毀滅性瓶頸

在工廠量產環境中，「每台設備組裝完成需要花費幾秒鐘」直接決定了整條產線的產能與硬體成本。

作者以實測數據揭示了在嵌入式設備上生成 RSA 金鑰的致命缺陷：
- **RSA 金鑰生成的數學本質**：必須隨機生成大奇數並反覆執行 Miller-Rabin 質數測試。
- **非確定性時間延遲（Non-deterministic Latency）**：
  > **原文引用（p.188–189）**
  > "Generating an RSA key is a nondeterministic probabilistic search. Sometimes the algorithm finds two suitable primes in 2 seconds; other times it has to test thousands of candidates, taking over 45 seconds on an ARM Cortex-A7! In high-speed manufacturing, having devices pause unpredictably on the assembly line for up to a minute creates severe production bottlenecks."

**實測金鑰生成時間對照**：
- **RSA-2048**：平均耗時 4.2 秒，最差情況高達 **42 秒**！
- **RSA-4096**：平均耗時 35 秒，最差情況高達 **180 秒（3 分鐘）**！
- **ECDSA P-256**：耗時固定在 **0.05 秒（50 毫秒）**以內，具有完全確定性的生產週期！

**架構抉擇**：
1. **全面遷移至 ECDSA**：消除產線延遲，大幅降低功耗。
2. **產線中央預置（Central Provisioning）**：若業務強制要求 RSA，絕不要讓嵌入式設備在產線上自己算，應由工廠端的高效能硬體安全模組（HSM）批次生成好金鑰對，再透過安全探針快速燒入設備 eFuse / Flash 中。

---

### 本章 Summary（作者原話）

> **原文引用（p.190）**
> "There’s no doubt that every single device is a physically unique object. With the help of identiﬁers like CPU serial numbers, MAC addresses, and values chosen by the manufacturer, we’re able to represent this uniqueness in the digital space and provide a base for device identities. However, merely claiming an identity isn’t enough for most applications. Devices have to be able to cryptographically prove their identity with the help of unique and conﬁdential authenticators like cryptographic keys.
> We explored authentication protocols based on challenge-response mechanisms, dedicated authentication chips, and the role of trusted third parties in issuing X.509 certificates.
> Further, we discussed identity life-cycle management from generation to destruction.
> The practical case studies demonstrated extracting STM32MP157F identifiers to request an X.509 certificate and evaluated the immense time penalty of on-device RSA key generation during manufacturing."

---

### 思考與實務檢驗

1. **為什麼絕不能使用 MAC 位址或 CPU Serial Number 直接當作「身分驗證憑證（Authenticator）」？混淆識別碼（Identifier）與驗證碼（Authenticator）會帶來什麼毀滅性漏洞？**
   - 識別碼（如 MAC 位址）是公開明文，且在乙太網路或 Wi-Fi 封包中以廣播形式發送，任何具有封包側錄能力的黑客都能隨意獲取；在軟體層面，Linux 指令 `ifconfig eth0 hw ether <MAC>` 可以在一秒鐘內修改本機 MAC 位址。如果伺服器僅僅檢查「連進來的客戶端 MAC 位址是否合法」，黑客只需偽造該 MAC，就能不費吹灰之力取得整台合法設備的所有特權。識別碼只代表「帳號名稱」，驗證碼（私鑰）才是「密碼」；若把帳號當密碼，系統安全形同門戶大開。

2. **在工廠量產階段，為十萬台物聯網設備注入私鑰與憑證時，應採取「產線伺服器批次注入（Centralized Provisioning）」還是「設備內部自建金鑰後送簽 CSR（On-Device Generation + CSR）」？兩者的安全與效能權衡為何？**
   - **產線批次注入（Centralized Provisioning）**：優勢在於速度極快（由工廠伺服器或 HSM 高速批量生成後燒錄，不拖慢產線節拍），但劣勢在於**私鑰曾在晶片外部存在過**（備份於工廠資料庫中），若工廠內部人員有惡意或產線伺服器被駭，所有設備金鑰皆有被批次洩漏的單點破產風險。
   - **設備自建金鑰（On-Device Generation + CSR）**：最符合最高安全準則（私鑰在設備內部晶片生成後鎖定，世界上沒有任何人能看到私鑰明文），但強烈依賴設備自身的硬體 TRNG 速度與運算能力；因此若採用此方案，**必須強制選用 ECDSA 或 Ed25519**，絕對不可使用 RSA，以確保金鑰生成在毫秒內完成，避免產線停擺。

---

### 我的整理：本章帶走的五句話

1. **識別碼（Identifier）是公開的姓名，驗證碼（Authenticator）是保密的私鑰，混淆兩者將招致毀滅。**
2. **質詢-回應協定（Challenge-Response）讓設備無須交出私鑰即可證明身分，且天然免疫重放攻擊。**
3. **公鑰基礎設施（PKI）與 X.509 憑證階層讓伺服器能以單一 Root CA 管理百萬台設備的信任驗證。**
4. **設備身分具有生命週期，報廢時必須執行加密清除（Crypto Erase）並吊銷憑證。**
5. **在產線上讓嵌入式 CPU 生成 RSA 金鑰是產能災難，應全面採用 ECDSA 或工廠端 HSM 預置。**

---

### 延伸資源（p.175, p.179, p.188）

- 公鑰憑證標準：RFC 5280 *Internet X.509 Public Key Infrastructure Certificate and Certificate Revocation List (CRL) Profile*
- 憑證註冊標準：RFC 7030 *Enrollment over Secure Transport (EST)*
- 安全晶片介面規範：Trusted Computing Group (TCG) *TPM 2.0 Library Specification*
- 工業物聯網身分管理：IEC 62443-4-2 Technical Security Requirements for IACS Components

---
## 第 7 章：Secure Communication（安全通訊：TLS 1.3、mTLS 與輕量化協定架構）

> 對應原書 p.192–214

### 這章在幹嘛

在物聯網與工業 4.0 時代，沒有任何嵌入式設備是一座孤島。然而，將設備連接上網際網路或工廠乙太網路的同時，也等於將設備完全暴露給全球的竊聽者、中間人攻擊者與封包篡改者。

作者在本章系統性建構了嵌入式裝置的傳輸安全架構。本章的核心支柱是 **傳輸層安全協定（Transport Layer Security, TLS）**，但作者強調：在嵌入式系統上實作 TLS 絕不能像寫 Web 網頁一樣想當然爾——**即時時鐘（RTC）遺失、快閃記憶體裝不下龐大的根憑證庫、網路延遲與握手重試**，都是會讓嵌入式連線徹底癱瘓的工程暗礁。

整章內容層層展開：
1. **通訊全維度保護目標**：機密性、完整性、真實性、抗重放（Anti-Replay）與完全前向保密（PFS）。
2. **TLS 的演進與核心剖析**：從 SSL 2.0/3.0 的崩潰到 TLS 1.2，再到現代唯一推薦標準 **TLS 1.3** 的革命性架構精簡。
3. **TLS 1.3 核心優勢與 0-RTT 重放風險**：刪除脆弱加密套件、強制 Ephemeral Diffie-Hellman、1-RTT 極速握手與 0-RTT 早期資料的安全陷阱。
4. **嵌入式設備執行 TLS 的硬體與基礎設施前置需求**：硬體即時時鐘（RTC）對憑證有效期限驗證的決定性影響、快閃記憶體中 Root CA 信任庫的修剪裁切。
5. **嵌入式軟體函式庫選型**：OpenSSL、mbedTLS 與 wolfSSL 的體積、記憶體佔用與授權對比。
6. **實戰案例：STM32MP157F 安全 MQTT 通訊與 SSLyze 審計**：Eclipse Mosquitto MQTT Broker 部署、雙向憑證認證（mTLS / Client Certificate Validation）、Wireshark 封包抓取分析與使用 `SSLyze` 進行加密套件弱點掃描。
7. **非 TLS 輕量化安全通訊方案**：在極度受限環境（CoAP, LPWAN）下的應用層安全（OSCORE / COSE）、UDP 環境下的 DTLS、以及新世代輕量級核心 VPN 協定 WireGuard。
8. **通訊冗餘與高可用性架構**：雙網卡（Ethernet + LTE）安全故障轉移。

---

### 重點 1：傳輸安全的全景目標與前向保密性（PFS）

安全通訊必須同時達成五大維度的保護：
1. **機密性（Confidentiality）**：防止空中封包或路由器上的竊聽。
2. **完整性（Integrity）**：防止傳輸中的感測數值或控制指令遭到惡意修改。
3. **真實性（Authenticity）**：確保通訊對象確實是合法的伺服器或設備。
4. **抗重放性（Anti-Replay）**：防止攻擊者將過去合法的開門或轉帳指令封包重新發送一次。
5. **完全前向保密（Perfect Forward Secrecy, PFS）**：
   > **原文引用（p.194）**
   > "Perfect Forward Secrecy ensures that even if an attacker records encrypted network traffic for years and later manages to compromise the server's long-term private key, the attacker still cannot decrypt any of the past recorded sessions. This is achieved by generating unique ephemeral Diffie-Hellman keys for every single connection."

---

### 重點 2：TLS 1.3 的革命性變革：精簡即安全

TLS 1.3（RFC 8446）徹底擺脫了過去 TLS 1.2 龐大臃腫且充滿歷史包袱的配置噩夢：

| 評比項目 | TLS 1.2（舊標準，極易配置錯誤） | TLS 1.3（現代唯一推薦標準） |
| :--- | :--- | :--- |
| **握手延遲** | 2-RTT（需要客戶端與伺服器來回往返兩次） | **1-RTT（大幅提速 50%）**，支援 0-RTT 恢復 |
| **金鑰交換機制** | 支援靜態 RSA 金鑰交換、靜態 DH、DHE、ECDHE | **強制僅支援暫態 ECDHE 或 DHE（保證 100% PFS）** |
| **被淘汰的弱演算法** | 允許 RC4, 3DES, MD5, SHA-1, CBC 模式 | **全數清除！** 徹底杜絕 POODLE, Lucky 13, Bleichenbacher 攻擊 |
| **允許的密碼套件** | 上百種複雜組合，極易選錯 | **精簡至僅 5 組 AEAD 套件**（如 AES-128-GCM, ChaCha20-Poly1305） |

> **原文引用（p.196）**
> "TLS 1.2 became its own enemy due to immense configuration complexity. To minimize mistakes and maximize security and performance simultaneously, TLS 1.3 was published in 2018. Every newly designed embedded device should use TLS 1.3 by default."

---

### 重點 3：TLS 1.3 的雙面刃：0-RTT 早期資料的重放危機

TLS 1.3 引入了 **0-RTT（Zero Round-Trip Time）** 連線恢復功能：先前連線過的客戶端可在發送第一個封包時直接附帶加密應用資料，無須等待握手完成。
- **物聯網的巨大誘惑**：對於使用電池供電、每小時喚醒一秒上傳溫度的感測器，0-RTT 能大幅縮短無線電晶片發射時間，節省可觀電量。
- **致命安全代價**：
  > **原文引用（p.197）**
  > "0-RTT data does not provide replay protection! Attackers who capture a 0-RTT packet over the air can replay it multiple times to the server. If that packet contains an idempotent read request (like querying temperature), the risk is negligible; however, if it contains an action (like unlocking a door or triggering an actuator), 0-RTT can be catastrophic. Embedded architects must disable 0-RTT for all state-changing endpoints."

---

### 重點 4：嵌入式 TLS 兩大實體阻礙：RTC 時間與憑證庫體積

在 PC 上理所當然的硬體配置，在嵌入式微控制器上卻往往是致命的障礙：

#### 1. 即時時鐘（RTC）遺失困境
X.509 數位憑證有明確的有效期限（`notBefore` 與 `notAfter`）。
- **現象**：許多無電池的物聯網設備在斷電重啟後，硬體時鐘重置為 **1970 年 1 月 1 日（UNIX Epoch 0）**。
- **死鎖**：當設備嘗試連線伺服器時，TLS 函式庫檢驗伺服器憑證（有效起始日通常是 2024 年），發現「當前系統時間（1970）早於憑證生效時間」，判定憑證無效，**直接中斷 TLS 連線**！設備若想透過 NTP 同步時間，而 NTP 伺服器又需要安全認證，系統將陷入雞生蛋、蛋生雞的無限死鎖。
- **工程解法**：在設備非揮發性儲存上記錄每次關機前的「最後已知有效時間戳記」；或在系統初始化時，於 TLS 握手前透過明文 SNTP 向受信任的工廠伺服器進行粗略時間同步。

#### 2. Root CA 信任庫體積爆炸
標準 Linux 發行版的 Mozilla 根憑證包（`ca-certificates`）包含上百個 CA，體積超過 **250 KB**。對於內部 Flash 僅有 512 KB 的微控制器，這幾乎佔用了一半的韌體空間！
- **工程對策**：嵌入式設備絕不應安裝全局 CA 庫；架構師應實施 **憑證釘選（Certificate Pinning）** 或僅在韌體中內嵌設備專屬後端伺服器的單一 Root CA 憑證（通常小於 1 KB）。

---

### 重點 5：實戰案例：STM32MP157F 雙向認證 MQTT 通訊與 SSLyze 審計

作者在開發板上使用熱門的物聯網輕量協定 MQTT，構建了最高安全級別的雙向認證架構（mTLS）：

```
[STM32MP157F-DK2 (MQTT 設備客戶端)]                     [Mosquitto Broker (MQTT 伺服器)]
               │                                                     │
               │ 1. Client Hello (宣告 TLS 1.3)                       │
               ├────────────────────────────────────────────────────►│
               │                                                     │
               │ 2. Server Hello + 傳送 Broker X.509 伺服器憑證       │
               │    + 要求客戶端憑證 (Certificate Request)           │
               │◄────────────────────────────────────────────────────┤
 驗證伺服器憑證  │                                                     │
 是否由 CA 簽發 │ 3. 傳送設備 Client X.509 憑證                        │
 且網域名稱符合 ├────────────────────────────────────────────────────►│ 驗證客戶端憑證
               │                                                     │ 是否由授權 CA 簽署
               │ 4. 傳送 Certificate Verify (以設備私鑰簽章)          │ 且序號未在 CRL 中
               ├────────────────────────────────────────────────────►│
               │                                                     │
               │ 5. 完成握手！建立對稱加密 Application Data 傳輸通道    │
               │◄═══════════════════════════════════════════════════►│
```

**SSLyze 安全稽核實測**：
使用開源 Python 工具 `sslyze` 對 Mosquitto 伺服器進行自動化漏洞掃描：
```bash
$ python3 -m sslyze localhost:8883
```
- **審計重點**：
  1. 檢查是否意外開啟了已被淘汰的協定（SSL 2.0, SSL 3.0, TLS 1.0, TLS 1.1）。
  2. 檢查伺服器是否支援不具備前向保密性（PFS）的弱加密套件（如 `TLS_RSA_WITH_AES_128_CBC_SHA`）。
  3. 驗證伺服器憑證鏈是否完整，是否存在自簽名（Self-Signed）或過期憑證。

---

### 重點 6：非 TLS 輕量化安全通訊替代方案

在某些極端受限的物聯網環境中，TLS 的連線狀態管理與標頭開銷依然太重，作者提出了三種專屬替代方案：

1. **OSCORE（RFC 8613, Object Security for Constrained RESTful Environments）**：
   - 專為 CoAP（受限應用協定）設計的**應用層加密框架**。
   - 基於 CBOR 物件簽章與加密（COSE）。
   - **特性**：加密只發生在應用層酬載（Payload），中間的網路代理伺服器（Proxy）可以正常路由 CoAP 標頭，但完全無法窺探封包內容，且支援跨多種非 IP 傳輸層（如 NB-IoT、LoRaWAN）。
2. **DTLS（Datagram TLS, RFC 9147）**：
   - 將 TLS 機制移植到無連線的 **UDP** 協定之上。
   - 內建封包遺失重傳、重排序與重放視窗檢測機制。
3. **WireGuard VPN**：
   - 現代 Linux 核心原生整合的高效能虛擬私有網路。
   - **程式碼僅約 4,000 行**（相較於 OpenSSL 的數十萬行，攻擊面縮小了 99%！）。
   - 採用固定的現代密碼套件：ChaCha20-Poly1305 與 Curve25519，提供近乎線速的加密吞吐量。

---

### 本章 Summary（作者原話）

> **原文引用（p.212–213）**
> "The IoT and its industrial counterpart are not possible without secure communication. A variety of protection goals from conﬁdentiality to integrity to authenticity, and even availability, are demanded for modern communication channels. Besides the necessary cryptographic capabilities, devices need to support state-of-the-art protocols like TLS 1.3 to achieve a high security level. In some use cases, where the loss of messages is critical and redundant physical communication media are required, devices even have to provide multiple communication interfaces.
> This chapter’s case study showed an example implementation of a MQTT-broker with TLS-based communication and the necessary conﬁguration parameters. It showed that thorough security analysis of the ﬁnal result can help spot misconﬁgurations and weak cipher suites offered by a service.
> Although TLS is one of the most common and most popular security protocols, many others exist that are meant for dedicated applications. For example, Internet Protocol security (IPsec) can be used to establish a virtual private network (VPN) on OSI layer 3, while the SSH protocol enables remote access to devices for administrators. On OSI layer 2, the idea of Time-Sensitive Networking (TSN) takes care of communication channel separation on a shared physical medium, while IEEE 802.1AE (also known as MACsec) aims for protected communication.
> And even if you don’t ﬁnd any protocol that directly suits your needs, the conglomeration of modern security protocols can deﬁnitely serve as inspiration for solving your speciﬁc challenges."

---

### 思考與實務檢驗

1. **許多無電池或低成本物聯網設備在斷電重啟後，硬體 RTC 時鐘會重置回 1970 年 1 月 1 日。這會對 TLS 連線造成什麼致命阻礙？如何以兼顧安全的方式解決「時間雞生蛋、蛋生雞」的難題？**
   - **致命阻礙**：TLS 客戶端在驗證伺服器憑證時，會強制比對本地時間與憑證有效區間（`notBefore` 至 `notAfter`）。若本地時間為 1970 年，憑證會被判定為「尚未生效」而拒絕連線；若設備直接停用憑證有效期限檢查，攻擊者就能永遠使用被吊銷或過期的舊憑證實施中間人劫持。
   - **兼顧安全的解法**：
     1. **非揮發性最後已知時間（Last Known Timestamp）**：在每次連線成功或關機前，將目前時間寫入 Flash/EEPROM。開機時讀取該值作為時間基準底線，至少確保系統時間在憑證生效年（如 2024 年）之後。
     2. **預握手受限 NTP**：在建立完整業務連線前，僅向硬編碼 IP/網域名稱的授權時間伺服器透過 NTP 取得粗略時間，或利用 TLS 握手本身回傳的伺服器時間標頭進行校正。

2. **TLS 1.3 的 0-RTT（Zero Round-Trip Time）特性大幅加速了連線建立，但為何在工業控制與關鍵物聯網場景中被視為高風險功能？架構師該如何規範其使用？**
   - 0-RTT 早期資料缺少防重放保護機制（Anti-Replay）。網路攻擊者只要在空中側錄設備發出的第一個 0-RTT 加密封包，就能隨時原封不動地向後端伺服器反覆重放。若該封包承載的是工業閥門開啟指令、繼電器重置動作或轉帳交易，伺服器每次收到重放封包都會重複執行該項操作，造成嚴重的實體安全事故。
   - **架構規範**：在工業控制與關鍵物聯網設備中，應在 TLS 設定中**預設關閉 0-RTT**；若為極低功耗感測器必須啟用，則嚴格限制 0-RTT **僅允許承載冪等性（Idempotent）的純查詢請求**（如回報溫度讀數），絕對禁止在 0-RTT 傳輸任何控制變數、設定修改或可重複執行的硬體動作。

---

### 我的整理：本章帶走的五句話

1. **傳輸安全不僅要防竊聽（加密），更要防篡改（MAC）、防偽冒（身分認證）與防重放。**
2. **TLS 1.3 是現代物聯網裝置的唯一基準，它簡化了加密套件並強制實施完全前向保密（PFS）。**
3. **0-RTT 早期資料雖然節省電力，但天生不防重放攻擊，嚴禁應用於任何改變設備狀態的指令。**
4. **硬體 RTC 時鐘遺失是嵌入式 TLS 的隱形殺手，系統必須架構可靠的時間初始化與單調保護機制。**
5. **在極度受限環境下，以物件為中心的應用層安全（OSCORE）與 WireGuard 是高效能的非 TLS 替代方案。**

---

### 延伸資源（p.196, p.203, p.211）

- 傳輸層安全標準：RFC 8446 *The Transport Layer Security (TLS) Protocol Version 1.3*
- 受限環境物件安全標準：RFC 8613 *Object Security for Constrained RESTful Environments (OSCORE)*
- TLS 安全配置掃描工具：`SSLyze` (Fast and powerful SSL/TLS scanning library and CLI tool)
- 輕量化 VPN 核心實作：The WireGuard Protocol & Whitepaper by Jason A. Donenfeld

---
## 第 8 章：Secure Boot and System Integrity（安全開機與系統完整性：信任鏈、eFuse 與開源授權衝撞）

> 對應原書 p.215–239

### 這章在幹嘛

在嵌入式裝置的所有防禦體系中，**安全開機（Secure Boot）** 是至高無上的基石。如果系統啟動時載入的開機程式碼已經被黑客置換成惡意二進制（Rootkit），那麼作業系統開機後無論部署多麼嚴密的防火牆、存取控制或加密檔案系統，都將毫無意義——因為底層的惡意核心可以隨意竄改任何檢查結果。

作者在本章揭開了晶片製造商繁複行銷術語背後的真實架構，深入剖析了從晶片內部的不可修改微碼（Mask ROM）開始，如何透過密碼學數位簽章逐級建立不可被破壞的 **信任鏈（Chain-of-Trust）**。

整章分為六大核心架構與深度案例研究：
1. **多階段開機序列的複雜性**：現代異質 SoC 的多級載入流程（ROM -> FSBL -> SSBL -> Kernel -> Rootfs）。
2. **信任鏈本質與硬體信任根（Hardware Root-of-Trust）**：不可變更的 Mask ROM 與一次性可編程電熔絲（OTP eFuses）的不可逆綁定。
3. **實作考量與防變磚風險**：開發金鑰 vs. 量產金鑰、防回滾計數器（Anti-Rollback）以及「熔絲一旦燒斷不可後悔（Bricking）」的工程代價。
4. **開源授權與安全開機的世紀衝撞：GPLv3 提防機器（Tivoization）**：為何 U-Boot 與 Linux 核心堅持停留在 GPLv2？為何設備商對 GPLv3（如 GRUB2、Bash）避之唯恐不及？
5. **實戰案例：STM32MP157F 安全開機端到端實作**：
   - 晶片 ROM Code 透過 SHA-256 與 ECDSA-P256 驗證 ARM TF-A BL2。
   - BSEC OTP 暫存器（Word 24~31）金鑰雜湊燒錄。
   - U-Boot 映像檔驗證機制與 FIT 映像檔（Flattened Image Tree）RSA 簽章驗證實戰。
6. **開機後的系統完整性延伸**：Linux 核心模組簽署驗證（Module Signing）、唯讀雜湊樹檔案系統（dm-verity）、IMA/EVM 與硬體防寫保護接腳（Hardware Write-Protect Pin）。

---

### 重點 1：信任鏈（Chain-of-Trust）的傳遞法則

安全開機的核心邏輯非常嚴密：**任何一個階段在將 CPU 執行權交給下一個階段之前，必須先對下一個階段的映像檔進行密碼學簽章驗證與完整性比對。**

```
[晶片內建不可修改 ROM Code (Root of Trust)]
                 │ 1. 讀取 eFuse 公鑰雜湊，驗證 FSBL 簽章
                 ▼
     [第一階段開機載入器 (FSBL: TF-A BL2)]
                 │ 2. 初始化 DDR RAM，驗證 BL32 (OP-TEE) 與 BL33 (U-Boot)
                 ▼
     [第二階段開機載入器 (SSBL: U-Boot)]
                 │ 3. 讀取 control .dtb 內嵌公鑰，驗證 FIT 映像檔簽章
                 ▼
  [Linux 核心映像檔 (fitImage: Kernel + DTB + Initramfs)]
                 │ 4. 驗證根檔案系統 dm-verity Merkle Tree 根雜湊
                 ▼
         [唯讀受保護根檔案系統 (dm-verity Rootfs)]
```

> **原文引用（p.216）**
> "The basic principle of secure boot is simple: an immutable hardware component verifies the digital signature of the first software component before executing it. If valid, that component verifies the next one, and so on. This sequence of verification steps is called a Chain-of-Trust. If any single signature verification fails along this chain, the boot process must be immediately halted to prevent the execution of untrusted code."

---

### 重點 2：硬體信任根：為什麼純軟體無法達成安全開機？

許多初學者誤以為「在 U-Boot 裡面寫一段程式碼檢查 Linux 核心的 SHA-256」就是安全開機。作者嚴正指出這是致命的邏輯矛盾：
- 如果 U-Boot 存放在外部 SPI Flash，攻擊者只要用燒錄夾將 U-Boot 拔除或重寫，把檢查程式碼整行註解掉，整個防禦瞬間形同虛設！
- **信任根（Root of Trust）必須始於硬體**：
  1. **Mask ROM**：位於 SoC 晶片矽片內部的第一道開機微碼，由半導體晶圓廠在光罩微影階段刻死，物理上任何軟體或外接硬體都無法修改。
  2. **OTP eFuses（電熔絲）**：晶片內部的非揮發性熔絲。在量產時，工程師將廠商公鑰的 SHA-256 雜湊值燒入 eFuse。開機時，Mask ROM 會計算外部 FSBL 映像檔標頭中的公鑰雜湊，並與 eFuse 中的數值進行硬體比對；兩者相符，才使用該公鑰驗證 FSBL 數位簽章。

---

### 重點 3：開發金鑰 vs. 量產金鑰與防變磚（Bricking）警示

在導入安全開機時，最危險的工程事故就是「將設備變磚」：

> **原文引用（p.222）**
> "Fuses are a one-way street. Once you blow the hardware security fuses on a microcontroller, there is no 'undo' button. If you burned a wrong public key hash, made an error in your signature script, or forgot to sign the bootloader image, the processor’s ROM code will permanently reject your boot images, turning an expensive prototype board into unrecoverable electronic waste."

**工程最佳實踐防護規則**：
1. **嚴禁在研發早期燒錄量產金鑰**：許多晶片提供「關閉安全開機檢查」的測試暫存器，研發團隊應使用專門的開發評估板。
2. **分離開發金鑰與量產金鑰**：研發工程師只能接觸開發金鑰（Dev Key）；量產簽署私鑰必須存放在隔離網路的伺服器或硬體安全模組（HSM）中，簽名過程需雙人審批。
3. **防回滾計數器（Anti-Rollback Protection）**：
   僅驗證簽章是不夠的！若黑客發現了一年前的舊版韌體存在緩衝區溢位漏洞，黑客可以將這份「雖然舊、但簽章完全合法」的舊韌體刷入設備。為此，SoC 必須具備硬體單調計數器（Monotonic Counters in eFuses / RPMB），每次韌體大版本更新時強制燒斷相應數量的熔絲位元，使 ROM/U-Boot 永久拒絕版本號低於熔絲計數值的舊映像檔。

---

### 重點 4：開源授權與安全開機的世紀衝撞：GPLv3 提防機器條款（Tivoization）

這是所有嵌入式系統架構師與法務部門必須面對的重大法律戰場：

#### 1. 什麼是 Tivoization？
2000 年代初，數位錄影機公司 TiVo 在其產品中採用了基於 GPLv2 的 Linux 核心。TiVo 遵守規範釋出了核心原始碼，但他們在硬體晶片中加入了數位簽章驗證——如果使用者自行修改編譯了 Linux 核心，硬體會因為沒有 TiVo 的私鑰簽章而拒絕開機！
自由軟體基金會（FSF）認為這嚴重踐踏了使用者的軟體自由，稱此現象為 **Tivoization（提防機器化）**。

#### 2. GPLv3 第 6 條：User Product 條款
為此，FSF 於 2007 年發布的 GPLv3 增加了第 6 條要求：**若將 GPLv3 軟體安裝在「使用者產品（User Product，包含大眾消費電子、家電、物聯網裝置）」中，製造商必須隨產品提供「安裝資訊（Installation Information）」——即製造商必須將燒錄在硬體中的簽署私鑰或提供解鎖通道給終端使用者，讓使用者能夠在該硬體上運行自己修改編譯的二進制程式碼！**

> **原文引用（p.224–225）**
> "This clause creates a direct conflict with the primary goal of secure boot! If a manufacturer provides the private signing key or an unlock mechanism to the end user to comply with GPLv3, any attacker with physical access can use that exact same mechanism to install malicious rootkits, completely neutralizing the secure boot protection."

**業界的應對陣形**：
- **Linux 核心與 U-Boot**：由 Linus Torvalds 與社群主導，**堅決拒絕升級至 GPLv3**，永久維持 GPLv2 授權（GPLv2 不限制硬體鎖定）。
- **ARM TF-A 與 OP-TEE**：採用極其寬鬆的 BSD-3-Clause 授權，完全免除開源授權衝撞。
- **架構師的嚴格禁令**：在具備安全開機鎖定的嵌入式 Linux 設備中，**嚴禁在開機鏈條中引入任何 GPLv3 元件**（例如嚴禁使用 GPLv3 的 GRUB2 開機載入器，使用者空間亦嚴禁使用 GPLv3 的 Bash，全面改用 BusyBox ash 或 BSD 授權 Shell）。

---

### 重點 5：實戰案例：STM32MP157F 安全開機端到端實作

作者在 STM32MP157F 上展示了完整的開機驗證鏈實作：

#### 1. ROM Code 到 TF-A BL2 的硬體驗證
- **公鑰雜湊燒錄**：使用 ECDSA P-256 生成開機金鑰對。計算公開金鑰的 SHA-256 雜湊值（32 位元組），透過 STM32 專屬工具寫入 **BSEC（Boot and Security Controller）** 的 OTP 暫存器（Word 24 到 31）。
- **BL2 簽署**：編譯 ARM Trusted Firmware (TF-A) 時設定 `TRUSTED_BOARD_BOOT=1`，使用私鑰對 BL2 二進制映像檔進行簽章，生成附帶簽章標頭的 `tf-a.stm32` 映像檔。
- **硬體鎖定**：燒錄 BSEC OTP 第 0 個字組的 `SECURED_MCU` 熔絲位元。一旦鎖定，ROM Code 強制開啟驗證，任何無簽章或簽章錯誤的映像檔將直接導致晶片終止開機並進入睡眠。

#### 2. U-Boot FIT 映像檔驗證 Linux 核心
在第二階段，U-Boot 透過 **FIT（Flattened Image Tree）** 容器驗證核心：
```bash
# 使用 mkimage 工具將核心、裝置樹與 initramfs 打包並簽署
$ mkimage -f fit-image.its -k /path/to/keys -K u-boot.dtb -r fitImage
```
- `-K u-boot.dtb` 參數會自動將驗證用的公開金鑰節點寫入 U-Boot 的控制裝置樹二進制檔案中。
- 當 U-Boot 開機執行 `bootm` 時，U-Boot 內建的密碼引擎會自動讀取裝置樹中的公鑰，比對 fitImage 內部各分區的雜湊與 RSA/ECDSA 簽章，驗證通過才跳轉執行 Linux 核心。

---

### 重點 6：開機後的完整性守護：dm-verity 與核心模組驗證

當 Linux 核心載入後，傳統安全開機的職責即告一段落。但黑客仍可能在運行期間修改磁碟上的系統函式庫（如 `/lib/libc.so`）或載入惡意核心模組。

作者提出了三大開機後完整性延伸措施：
1. **核心模組強制簽章（Kernel Module Signing）**：
   在 Linux 核心編譯選項中開啟 `CONFIG_MODULE_SIG_FORCE=y`。核心在載入任何 `.ko` 模組前，強制驗證該模組是否具有編譯時內建私鑰的數位簽章，徹底封死未授權 Rootkit 模組的載入管道。
2. **區塊級唯讀雜湊樹檔案系統（dm-verity）**：
   Android 與 ChromeOS 的核心防護基石。`dm-verity` 在檔案系統所有 4 KB 資料區塊之上構建一座 Merkle Tree（SHA-256 雜湊樹）。
   - 樹的頂端是單一 32 位元組的 **根雜湊（Root Hash）**，由安全開機鏈條在開機命令列中傳給核心。
   - 每當系統從 Flash 讀取任何一個 4 KB 磁區時，核心透明地沿著雜湊樹向上驗證。
   - **防護效果**：攻擊者即使物理拆下 Flash 晶片竄改了 `/bin/login` 裡面的任何一個位元，當系統嘗試執行該檔案時，雜湊驗證立即破功，核心拋出 `EIO`（I/O 錯誤）並拒絕讀取！
3. **硬體防寫保護（Hardware Write Protection）**：
   在電路板設計上，將 SPI Flash 晶片的防寫接腳（$\overline{	ext{WP}}$）直接透過上拉電阻接地或拉至硬體跳線。一旦出廠，任何軟體指令都無法向 Flash 發出抹除或寫入訊號。

---

### 本章 Summary（作者原話）

> **原文引用（p.238）**
> "Processor hardware and its manufacturers play an important role in the implementation of secure boot processes for embedded systems. Proprietary architectures, heterogeneous multicore complexity, and a conglomeration of marketing terms contribute to the high barrier that device architects and developers have to climb when aiming for a protected boot process.
> The basic principle is simple: an immutable hardware component verifies the first software component, which then verifies the next one, forming an unbroken Chain-of-Trust.
> We explored practical implementation aspects, including key management, the irreversible danger of blowing OTP fuses, and the legal minefield of GPLv3 Tivoization.
> The case study walked through the STM32MP157F boot sequence, demonstrating TF-A authentication by ROM code and U-Boot FIT image verification.
> Finally, we covered integrity protection beyond the boot process using dm-verity and kernel module signing, as well as hardware write protection as an effective low-cost measure."

---

### 思考與實務檢驗

1. **為什麼「僅在 U-Boot 階段驗證 Linux 核心簽章」稱不上真正的安全開機？若信任鏈的起點沒有錨定在硬體唯讀 ROM 與 eFuse 中，攻擊者能如何輕易繞過驗證？**
   - 信任鏈的強度永遠取決於其**最脆弱的第一個環節（Root of Trust）**。若硬體沒有啟用 Mask ROM 驗證，U-Boot 本身只是存放在外部 Flash（如 SPI NOR 或 eMMC）中的一般明文程式碼。攻擊者擁有實體接觸權限時，只需用幾美元的燒錄夾讀取 Flash，在反組譯工具中找到 U-Boot 的簽章比對跳轉指令（例如將條件跳轉 `bne signature_failed` 改為無條件略過 `nop`），再重新燒回 Flash；此時 U-Boot 就會毫無保留地載入任何惡意核心。唯有信任鏈始於晶片內部無法竄改的 Mask ROM 與 eFuse 熔絲，任何對外部 Flash 開機碼的修改才會在通電的瞬間被硬體邏輯即時阻絕。

2. **為什麼在商用安全開機架構中，採用 GPLv3 授權的開機元件（如 GRUB2）會帶來巨大的法務合規風險？為何業界主流開機架構偏好 TF-A（BSD-3）與 U-Boot（GPLv2）？**
   - GPLv3 的「提防機器條款（Section 6）」強制要求硬體製造商必須提供終端使用者「安裝資訊與金鑰」，確保使用者能將修改後的開機程式刷入硬體。若廠商採用 GRUB2 作為安全開機鏈的一環，為了遵守 GPLv3，廠商就必須公開自己的簽署私鑰或提供解鎖工具；然而一旦公開金鑰，惡意駭客就能利用這組金鑰合法簽署惡意韌體並在設備上執行，徹底瓦解安全開機防護！相比之下，ARM TF-A 採用寬鬆的 BSD-3-Clause 授權，完全不干涉硬體控制權；而 U-Boot 堅持維持 GPLv2 授權（GPLv2 僅要求公開軟體原始碼，完全不限制硬體金鑰鎖定）。因此，採用「TF-A + U-Boot」能完美兼顧開源合規與硬體安全鎖定。

---

### 我的整理：本章帶走的五句話

1. **安全開機的本質是不可破壞的信任鏈，其起點必須錨定在硬體不可修改的 Mask ROM 與 eFuse 中。**
2. **燒錄硬體熔絲（eFuse）是一條單行道，量產前必須建立極其嚴謹的金鑰隔離與防變磚測試流程。**
3. **安全開機必須搭配硬體防回滾計數器（Anti-Rollback），防止攻擊者將系統降級至具備已知漏洞的舊版韌體。**
4. **受限於 GPLv3 的 Tivoization 條款，安全開機鏈條中嚴禁引入 GPLv3 元件，應一律採用 BSD 或 GPLv2 授權。**
5. **安全開機的終點不應止於核心啟動，必須透過 dm-verity 雜湊樹與模組簽名將完整性保護延伸至整個檔案系統。**

---

### 延伸資源（p.218, p.225, p.236）

- ARM 信任韌體規範：Trusted Firmware-A (TF-A) Documentation & Secure Boot Board Guide
- U-Boot 映像檔簽署架構：Das U-Boot Documentation: *Verified Boot with FIT Images*
- Linux 唯讀檔案系統完整性：Linux Kernel Device Mapper: *dm-verity Documentation*
- 自由軟體基金會授權分析：GNU General Public License v3 (GPLv3) Section 6: Installation Information Requirements

---
## 第 9 章：Secure Firmware Update（安全韌體更新：SWUpdate 實作、A/B 雙分區與防回滾架構）

> 對應原書 p.240–268

### 這章在幹嘛

在過去的單晶片時代，許多工程師認為「把韌體燒進去、機器賣出去，任務就結束了」。然而，作者在開篇便以冰冷的資安現實敲響警鐘：**世界上不存在沒有漏洞的複雜系統**。

隨著物聯網設備的使用年限動輒長達 10 到 15 年，新的密碼學弱點、Linux 核心零日漏洞（0-days）與第三方相依套件 CVE 必然會不斷湧現。**一個無法安全進行遠端韌體更新的設備，在被發現第一個遠端代碼執行漏洞的那一刻起，就註定淪為網路僵屍或電子垃圾。**

然而，韌體更新涉及整個產品生命週期中最錯綜複雜的工程交集。整章分為四大核心主題與一場極具實戰價值的端到端架構實作：
1. **韌體更新的必然性與安全需求矩陣**：真實性（Authenticity）、完整性（Integrity）、機密性（Confidentiality）、傳輸通道安全、防回滾（Anti-Rollback）與全網版本分佈監控。
2. **更新分發與部署策略**：本地更新（USB/SD 卡）vs. 遠端空中更新（OTA）；推播（Push）vs. 輪詢拉取（Pull）架構評比。
3. **更新粒度與套件格式陷阱**：整機映像檔（Monolithic Image）vs. 增量差分（Delta Updates）；**為什麼傳統 Linux 套件管理器（如 `apt`、`opkg`）是嵌入式設備的災難？**
4. **磁碟分割區配置架構深度對決**：
   - 復原分區架構（Update/Recovery Partition）的停機瓶頸。
   - **雙副本 A/B 系統架構（Dual-copy A/B Partitioning）**：無縫背景寫入、原子化切換、開機健康計數（`bootcount`）與自動回滾的黃金標準。
   - 開機載入器（Bootloader）更新的變磚風險與硬體分區防護。
5. **實戰案例：基於 SWUpdate 框架的 STM32MP157F 雙分區安全更新**：
   - SD 卡雙副本 A/B 分割區佈局規劃（`bootA`, `bootB`, `rootfsA`, `rootfsB`, `data`）。
   - `sw-description` 元資料描述檔語法與 SHA-256 校驗。
   - 建立與 RSA/ECDSA 數位簽名 `.swu` 更新歸檔封包。
   - 設備端 SWUpdate 守護行程部署、驗簽與解壓縮寫入。
   - U-Boot 開機計數器環境變數（`bootcount` / `bootlimit`）聯動，實作開機崩潰自動回退。

---

### 重點 1：安全韌體更新的五大核心需求

安全更新機制本身就是設備上權限最高的後門，若防護不當，它會直接成為黑客投放勒索病毒的特快車。

| 核心需求 | 技術實作機制 | 防禦目標與失效後果 |
| :--- | :--- | :--- |
| **真實性與完整性** | 非對稱數位簽章（RSA-PSS / ECDSA） | **絕對不可妥協的核心！** 確保更新包由原廠授權金鑰簽發，內容未遭任何中間人篡改或置換。 |
| **機密性（選用）** | 對稱酬載加密（AES-GCM / AES-CBC） | 防止競爭對手從更新包中逆向工程竊取專利二進制或演算法權重。 |
| **安全傳輸通道** | HTTPS / TLS 1.3 傳輸連線 | 防止傳輸過程中的流量嗅探與惡意代理重導向。 |
| **防回滾保護** | 硬體單調計數器 / eFuse 版本號檢查 | **防止降級攻擊（Downgrade Attack）**：黑客無法強迫設備刷入帶有已知 CVE 的合法舊版本韌體。 |
| **版本分佈監控** | 雲端設備資產管理平台 | 精確追蹤設備網路中各設備的版本分佈，識別更新卡死或失聯的異常節點。 |

---

### 重點 2：為什麼傳統 Linux 套件管理器（`apt`, `opkg`）是嵌入式設備的噩夢？

許多從伺服器或桌面轉行到嵌入式的工程師，習慣在設備上直接跑 `apt update && apt upgrade`。作者嚴肅警告這是災難性的架構決策：

> **原文引用（p.248）**
> "Package managers like apt or opkg are designed for desktop PCs and servers where a human administrator or reliable power supply is assumed. In embedded systems, using a package manager is a recipe for disaster. If power is interrupted while apt is unpacking and configuring a shared C library, the filesystem is left in a half-configured, broken state. Upon reboot, the system will crash with missing symbols or corrupted dependencies, requiring a factory return."

**核心缺陷清單**：
1. **非原子性（Non-atomic）**：套件是一個個解壓縮並設定，更新過程中任何瞬間斷電都會導致系統狀態殘缺。
2. **狀態空間無限發散**：若設備經歷多次個別套件更新，每台設備上的函式庫版本組合將各不相同，研發團隊根本無法在實驗室重現客戶現場的軟體狀態。
3. **回滾極度困難**：一旦更新出錯，套件管理器很難完美乾淨地降級回上一刻的狀態。

**結論**：嵌入式系統必須採用 **基於映像檔的原子化更新（Image-based Atomic Updates）**，例如 SWUpdate、RAUC 或 Mender。

---

### 重點 3：分割區佈局對決：復原分區 vs. 雙副本 A/B 系統

針對映像檔更新，業界存在兩種主流分區哲學：

#### 1. 復原分區架構（Update / Recovery Partition）
- Flash 劃分為一個微型 Recovery OS 與一個 Main OS。
- 更新時，設備必須重開機進入 Recovery OS，由 Recovery OS 下載並覆寫 Main OS 分區，寫完後再重開機回到 Main OS。
- **缺點**：**服務停機時間極長**（更新期間設備完全喪失業務功能），且若 Recovery OS 本身出現缺陷，設備將無法修復。

#### 2. 雙副本 A/B 系統架構（Dual-copy A/B Partitioning）——現代工業標準

```
[開機引導程式 (U-Boot)] ◄─── 讀取環境變數 bootcount, active_slot
         │
         ├───► [Slot A: 目前運行中] ──► 正常執行工廠業務、背景下載更新
         │     - bootA (核心 + DTB)
         │     - rootfsA (系統檔案)
         │
         └───► [Slot B: 閒置備份槽] ◄── SWUpdate 直接在背景將新韌體寫入此處！
               - bootB (待更新核心)
               - rootfsB (待更新系統)
```

> **原文引用（p.250–251）**
> "The dual-copy A/B approach is the state-of-the-art strategy for high-reliability embedded systems. The running system updates the inactive partition in the background while continuing normal operations without downtime. Once the update is verified on disk, the bootloader environment is adjusted to point to the new slot upon the next reboot. If the new system crashes or watchdog triggers, the bootloader automatically reverts to the previously working slot!"

**A/B 系統核心優勢**：
1. **零停機時間更新**：寫入 Flash 的過程完全在背景執行，設備只需在最後進行一次正常的數秒重開機。
2. **絕對原子性切換**：系統切換只是改變 U-Boot 的一個開機指標變數，沒有中間殘缺狀態。
3. **無條件自動回滾（Automatic Rollback）**：若新韌體存在嚴重 Bug 導致 Kernel Panic，硬體看門狗重啟後自動切回舊分區，絕不變磚。

---

### 重點 4：開機載入器（Bootloader）更新的阿基里斯之踵

許多架構師在規劃 A/B 分區時，往往忘記了開機載入器（SPL / U-Boot）本身：
- 根檔案系統可以 A/B 雙備份，但 Bootloader 通常位於 Flash 最前段的單一固定物理區塊中。
- **最致命的時刻**：當更新程式正在執行 `flash_erase` 抹除 Bootloader 區塊的這幾百毫秒內，**若設備突然遭遇外部斷電，設備將立即永久變磚**！因為晶片通電後 Mask ROM 找不到任何開機載入器代碼。
- **解決對策**：
  1. **原則上不更新 Bootloader**：將 Bootloader 視為極簡穩定的硬體驅動層，終生不變，只更新核心與檔案系統。
  2. **硬體開機分區（eMMC Boot Partitions）**：現代 eMMC 晶片內建專屬的 `Boot0` 與 `Boot1` 物理硬體分區，支援硬體層級的原子性開機切換。

---

### 重點 5：實戰案例：基於 SWUpdate 的 STM32MP157F 雙分區實戰

作者在 STM32MP157F 開發板上完整實作了開源界最頂尖的更新框架 **SWUpdate**：

#### 1. 儲存卡 A/B 分割區規劃
```bash
# 劃分 5 個主分區
/dev/mmcblk0p1  bootA       (64 MB, 存放作用中 Kernel fitImage)
/dev/mmcblk0p2  bootB       (64 MB, 存放待更新 Kernel fitImage)
/dev/mmcblk0p3  rootfsA     (768 MB, 作用中根檔案系統)
/dev/mmcblk0p4  rootfsB     (768 MB, 待更新根檔案系統)
/dev/mmcblk0p5  data        (剩餘空間, 使用者與設定持久層，跨版本共享)
```

#### 2. `sw-description` 元資料檔案定義
SWUpdate 的靈魂在於封包內部的描述檔：
```c
software =
{
    version = "2.0.0";
    description = "Firmware update for STM32MP157F";
    
    // 定義更新映像檔
    images: (
        {
            filename = "core-image-minimal.ext4";
            device = "/dev/mmcblk0p4"; // 寫入閒置的 rootfsB
            type = "raw";
            sha256 = "d41d8cd98f00b204e9800998ecf8427e...";
        }
    );
    
    // 更新成功後執行的 U-Boot 環境變數切換腳本
    scripts: (
        {
            filename = "update_bootloader.sh";
            type = "shellscript";
        }
    );
}
```

#### 3. 封包數位簽章與打包（`.swu`）
```bash
# 1. 使用私鑰對 sw-description 進行數位簽署
$ openssl dgst -sha256 -sign swupdate_priv.pem sw-description > sw-description.sig

# 2. 將描述檔、簽章與韌體映像檔依嚴格順序打包為 cpio 格式的 .swu 檔案
$ for f in sw-description sw-description.sig core-image-minimal.ext4 update_bootloader.sh; do
    echo $f
  done | cpio -ov -H crc > update_v2.0.0.swu
```

#### 4. 設備端驗證與自動回滾邏輯
在設備上執行更新：
```bash
$ swupdate -v -k /etc/swupdate_public.pem -i update_v2.0.0.swu
```
- SWUpdate 會在記憶體中率先解出第一個檔案 `sw-description`，並使用 `/etc/swupdate_public.pem` 驗證其數位簽章。**若簽章不符，直接終止更新，絕不讀取或寫入後續任何位元組！**
- 更新完成後，腳本設定 U-Boot 環境變數：
  ```text
  upgrade_available=1
  bootcount=0
  bootlimit=3
  active_slot=B
  ```
- **自動回滾驗證流程**：
  新系統開機時，U-Boot 自動將 `bootcount` 加 1。
  若新系統成功開機進入使用者空間並通過自我檢測常駐程式，應用程式會執行指令將 `upgrade_available` 重置為 `0`（宣告新韌體驗證成功！）。
  若新系統開機途中崩潰或死當，硬體看門狗重開機，`bootcount` 累積達到 `bootlimit=3`，U-Boot 偵測到開機連續失敗，**自動將 `active_slot` 倒退回 `A`**，拯救設備免於變磚！

---

### 本章 Summary（作者原話）

> **原文引用（p.267）**
> "Providing software and ﬁrmware updates for devices doesn’t seem like a very difﬁcult task at ﬁrst glance. However, if all requirements regarding security, scalability, and reliability are considered, it becomes a complex topic that impacts development pipelines, nonvolatile memory layouts, backend services, and customer processes.
> This chapter stressed the inevitable need to provide secure update mechanisms for all kinds of IoT devices because none of them is immune to future vulnerabilities.
> We explored critical security requirements, including authenticity, anti-rollback, and confidentiality.
> We examined why traditional package managers fail on embedded devices and analyzed device partitioning strategies, highlighting the immense resilience of the dual-copy A/B approach.
> Finally, the practical case study implemented a robust, signed A/B firmware update mechanism based on the SWUpdate framework on an STM32MP157F device."

---

### 思考與實務檢驗

1. **在物聯網裝置更新設計中，為什麼「防回滾（Anti-Rollback）」與「簽章驗證」同等重要？若缺乏硬體單調計數器（Monotonic Counter），攻擊者如何利用舊版合法韌體發動降級攻擊？**
   - 數位簽章只能證明「這份韌體是由原廠合法金鑰簽署的」，但它**無法證明這份韌體的發布時間**。假設一年前發布的 v1.0.0 韌體存在一個廣為人知的遠端代碼執行漏洞，原廠在半年前發布了 v2.0.0 進行修補。如果設備只驗證簽章而不檢查防回滾版本號，攻擊者只要攔截並灌入合法的 v1.0.0 舊更新包，設備驗證簽章完全通過並成功刷入；隨後黑客就能利用 v1.0.0 的已知舊漏洞徹底取得設備 root 權限！唯有透過不可逆的硬體單調計數器或 eFuse 強制比對「更新版本必須 $\ge$ 當前最低版本」，才能徹底杜絕降級攻擊。

2. **為什麼在汽車、工業與高可用性設備中，A/B 雙分區架構被視為必備標準？相較於單分區現場寫入（In-Place Update），A/B 架構消除了哪些災難性單點故障？**
   - 單分區現場寫入架構直接將下載的資料覆寫到當前正在運行的同一個檔案系統上。這帶來兩大毀滅性單點故障：
     1. **寫入途中斷電**：只要在 Flash 抹除或寫入的任何一瞬間發生外部停電或電池鬆脫，檔案系統將永遠處於半殘缺狀態，開機時無法載入 init 行程，設備直接永久變磚。
     2. **新軟體邏輯自毀**：即使寫入完整無缺，若新韌體存在嚴重的驅動相容性 Bug、核心崩潰或無法聯網，設備一旦重開機就再也無法連上伺服器，喪失了接收修復更新的能力。
   - A/B 雙分區架構將寫入動作完全隔離在未啟用的被動分區，寫入失敗對當前系統毫髮無傷；且透過 U-Boot 的 `bootcount` 機制，若新分區無法正常啟動，硬體自動無縫退回原分區，徹底消除了以上兩種致命風險。

---

### 我的整理：本章帶走的五句話

1. **世界上不存在沒有漏洞的系統；無法進行安全韌體更新的聯網設備終將淪為資安犧牲品。**
2. **在嵌入式 Linux 設備上使用傳統套件管理器（如 apt）是災難，更新必須基於原子化映像檔。**
3. **雙副本 A/B 分區是高可靠性嵌入式更新的黃金準則，它消除了停機時間並具備自動回滾能力。**
4. **簽署驗證防篡改，防回滾計數器防降級：防回滾機制是抵禦舊漏洞合法重放的必備防線。**
5. **開機載入器（Bootloader）的抹除與更新是全系統最脆弱的變磚時刻，應盡量避免或依賴硬體開機分區。**

---

### 延伸資源（p.248, p.253, p.265）

- 開源嵌入式更新框架：SWUpdate (Software Update for Embedded Linux Devices) 官方手冊
- 替代嵌入式更新專案：RAUC (Robust Auto-Update Controller) 與 Mender.io
- U-Boot 自動回滾機制：Das U-Boot Documentation: *Automatic Boot Count Limit Feature (`bootcount`)*
- 工業物聯網更新規範：IEC 62443-4-2 Component Security Level 3 Requirements on Software Updates

---
## 第 10 章：Robust Device Architecture（強健裝置架構：網路抗壓、即時性衝擊與異質雙核心隔離）

> 對應原書 p.269–292

### 這章在幹嘛

傳統資訊安全往往將焦點放在「防止黑客讀取機密（Confidentiality）」與「防止資料被修改（Integrity）」，然而在工業控制、汽車自動駕駛、機器人與關鍵基礎設施中，**可用性（Availability）與即時性（Real-Time Responsiveness）往往凌駕於一切之上！**

一台控制高速沖壓機的嵌入式設備，如果因為鄰近設備故障發出廣播風暴，或遭遇外部黑客發動 SYN Flood 網路洪水攻擊，導致 CPU 忙於處理網路中斷，使馬達煞車指令延遲了僅僅 50 毫秒，就可能釀成機械粉碎甚至工人傷亡的慘劇。

作者在本章打破了資安工程師與即時嵌入式工程師之間的專業壁壘，深入剖析了網路攻擊如何摧毀微控制器的確定性（Determinism），並提出了從作業系統到硬體層級的強健性架構防線。

整章脈絡分為五大主軸與一場精彩的即時延遲實測：
1. **設備在網路重壓下的威脅場景**：故障鄰居廣播風暴（Broadcast Storms）、通訊協定模糊測試（Fuzzing）、網路漏洞掃描與阻斷服務洪水攻擊（DoS Flooding）。
2. **強健架構與核心功能劃分**：感測器（Sensors）、致動器（Actuators）與控制器（Controllers）的關鍵路徑辨識。
3. **即時系統三分類法與 DoS 實體衝擊**：軟即時（Soft）、確定性即時（Firm）與硬即時（Hard Real-Time）；網路卡硬體中斷風暴（Interrupt Storms）如何導致 CPU 飢餓與記憶體耗盡（OOM）。
4. **硬體與作業系統層級防禦技術**：流量速率限制（Rate Limiting）、硬體看門狗、標準 Linux 的延遲缺陷 vs. **Linux PREEMPT_RT 即時補丁**。
5. **異質非對稱多核心架構（Asymmetric Multiprocessing, AMP）**：應用處理器（Cortex-A）與即時協同處理器（Cortex-M）的物理隔離。
6. **實戰案例：STM32MP157F 在網路洪水攻擊下的即時強健性評測**：
   - 使用 `cyclictest` 基準測試工具。
   - 標準 Linux 核心在網路洪水下的延遲崩潰（延遲飆升至 5 毫秒以上）。
   - Linux PREEMPT_RT 核心的有界延遲表現（維持在 120 微秒以內）。
   - Cortex-M4 協同處理器的示波器實測：在 Linux 完全被洪水打死的極端情況下，Cortex-M4 維持 **零抖動（<1 µs Jitter）** 的絕對物理防禦！

---

### 重點 1：即時系統三等級：延遲的代價決定了防禦等級

並非所有嵌入式系統都具有相同的即時性要求。作者清晰界定了三大即時維度：

```
[軟即時 (Soft Real-Time)]   ──► 偶爾錯過 Deadline 只會降低服務品質 (QoS)
                                例如：智慧電視播放串流影片掉格、UI 介面卡頓。
             │
[確定性即時 (Firm Real-Time)] ──► 錯過 Deadline 的結果毫無價值，但不致毀滅系統
                                例如：天氣監測雷達過期採樣、產線條碼掃描辨識超時。
             │
[硬即時 (Hard Real-Time)]   ──► 錯過任何一個微秒級 Deadline 都將釀成災難性損害！
                                例如：汽車安全氣囊彈開、心臟起搏器、工業機器人急停控制。
```

> **原文引用（p.273）**
> "In a hard real-time system, missing a deadline is considered a total system failure that can lead to catastrophic consequences, including destruction of machinery or loss of human life. Therefore, the security protection goal of availability in such systems is inextricably linked to real-time determinism."

---

### 重點 2：網路洪水攻擊（DoS）如何物理性摧毀 Linux 系統？

在通訊協定棧中，資料封包的接收是由微控制器底層的實體網路卡（MAC/PHY）驅動的：

```
[外部惡意網路洪水 (SYN / Ping Flood)]
                 │ (每秒數萬個中斷封包)
                 ▼
       [網路卡晶片 (Ethernet MAC)]
                 │
                 ▼ 觸發高優先權硬體中斷 (Hardware IRQs)
  [CPU 核心: 強制中斷目前即時控制執行緒]
                 │
                 ▼ 耗盡所有 CPU 週期處理 Bottom Halves (softirqd)
        [核心 Socket 緩衝區耗盡]
                 │
                 ▼
[記憶體耗盡 (OOM Killer 隨機殺死程序) / 即時馬達控制任務徹底餓死！]
```

> **原文引用（p.275）**
> "When a network interface receives a massive flood of packets, the CPU is bombarded with hardware interrupts. The kernel spends almost 100% of its execution cycles in interrupt handlers and softirq routines, attempting to process or drop packets. User-space real-time control applications are completely starved of CPU time, resulting in massive jitter and missed control loops."

---

### 重點 3：軟體層級防禦：標準 Linux vs. PREEMPT_RT 即時補丁

#### 1. 標準 Linux 核心的即時死穴
標準 Linux 追求的是「總吞吐量極大化（Maximum Throughput）」，核心內部充斥著不可被搶佔（Non-preemptible）的自旋鎖（Spinlock）臨界區段與關閉中斷的巨集。當即時任務被喚醒時，若核心剛好在執行一段耗時的檔案系統寫入或記憶體回收，即時任務必須被迫等待數十毫秒，產生無法預測的 **延遲抖動（Latency Jitter）**。

#### 2. Linux PREEMPT_RT 即時補丁的改造
PREEMPT_RT 將標準 Linux 核心徹底改造成確定性即時核心：
- **自旋鎖睡眠化**：將核心內部的 `spinlock_t` 替換為支援優先權繼承（Priority Inheritance）的睡眠互斥鎖（Sleeping Mutexes），幾乎所有核心程式碼區段皆可被高優先權即時任務隨時搶佔。
- **中斷執行緒化（Threaded Interrupts）**：將硬體中斷下半部轉化為可由排程器管理優先權的核心執行緒（`ksoftirqd`），允許工程師將即時控制執行緒的優先權設定得高於網路處理執行緒！

---

### 重點 4：極致的物理強健性：異質多核心架構（AMP）

如果連 PREEMPT_RT 都無法承受極限網路攻擊，嵌入式架構師的王牌是 **異質非對稱多核心（Asymmetric Multiprocessing, AMP）**：

```
 ┌──────────────────────────────────────┐     ┌────────────────────────────────────┐
 │       ARM Cortex-A7 雙核心 (Linux)    │     │      ARM Cortex-M4 協同核心 (RTOS)  │
 │──────────────────────────────────────│     │────────────────────────────────────│
 │ - 執行 Linux 5.15 作業系統             │     │ - 執行極簡 FreeRTOS 或 Bare-metal  │
 │ - 負責 Wi-Fi / Ethernet 網路通訊協定棧│     │ - 完全直連 GPIO / PWM / ADC 接腳   │
 │ - 負責 Web 管理介面、MQTT 雲端遙測    │     │ - 專門執行微秒級馬達閉迴路即時控制 │
 │ - 暴露於潛在網路 DoS 攻擊面           │     │ - 【物理上完全無中斷接觸網路卡！】 │
 └──────────────────┬───────────────────┘     └──────────────────┬─────────────────┘
                    │                                            │
                    └───────────► [晶片內部共享記憶體 (IPC)] ◄─────┘
                                   - OpenAMP / RPMsg 訊息信箱
                                   - 環形緩衝區 (Ring Buffer)
```

> **原文引用（p.278–279）**
> "The asymmetric multiprocessing architecture creates an ultimate physical barrier. The Cortex-A core runs a rich operating system like Linux to handle complex networking, web interfaces, and cloud communication, taking all the hits from network stress. Meanwhile, the critical real-time motor control is offloaded to a dedicated Cortex-M coprocessor that shares no interrupt lines with the network peripherals. Even if the Linux OS is entirely overwhelmed by a DDoS attack, the Cortex-M core continues ticking with microsecond precision."

---

### 重點 5：實戰案例：STM32MP157F 跨核心抗壓延遲對決實測

作者在開發板上使用標準 Linux 工具 `cyclictest`，在遭受激烈網路 Ping 洪水（`ping -f`）與 CPU 滿載壓力下，進行了不同架構的延遲實測：

#### 1. `cyclictest` 實測數據對比（目標排程週期 200 微秒）

| 執行環境與核心架構 | 系統待機時最大延遲 | 網路洪水與 CPU 滿載攻擊時最大延遲 | 即時性判定（以 1 毫秒為限） |
| :--- | :---: | :---: | :--- |
| **標準 Linux 核心 (Standard Kernel)** | 78 µs | **5,412 µs（5.4 毫秒！）** | **徹底崩潰（嚴重錯過 Deadline）** |
| **即時補丁核心 (PREEMPT_RT Kernel)** | 42 µs | **118 µs（0.11 毫秒）** | **完全達標（延遲嚴格收斂有界）** |
| **Cortex-M4 專屬協同處理器 (AMP)** | **< 1 µs** | **< 1 µs（零波動！）** | **完美物理隔離（完全免疫外部攻擊）** |

> **原文引用（p.286）**
> "Listing 10-2 shows the cyclictest results on a standard Linux kernel under attack: maximum latency spiked beyond 5 milliseconds! In an industrial robotics application, that is an eternity.
> In contrast, on the PREEMPT_RT kernel, the maximum latency remained tightly bounded below 120 microseconds.
> Finally, when monitoring the square-wave output of the Cortex-M4 coprocessor on an oscilloscope while the Cortex-A7 cores were subjected to a devastating network flood, the jitter was below 1 microsecond—imperceptible! Hardware-level separation provides the highest resilience against network-induced denial of service."

---

### 本章 Summary（作者原話）

> **原文引用（p.291）**
> "Many industries regard robustness as a fundamental feature for all their devices. However, when it comes to highly interconnected and automated systems, both customers and manufacturers typically have difﬁculty specifying their understanding of digital robustness.
> This chapter bridged the gap between the real-time world of embedded systems and the security protection goal of availability.
> The most important takeaways for device engineers and architects should be the awareness that network stress directly degrades real-time determinism.
> We explored hardware and operating system countermeasures, contrasting the vulnerabilities of standard Linux with the bounded latencies of PREEMPT_RT.
> Finally, the case study on the STM32MP157F proved that heterogeneous asymmetric multicore architectures (AMP), pairing an application processor with an isolated real-time coprocessor, provide the ultimate architectural answer to safeguard critical control functions under malicious network load."

---

### 思考與實務檢驗

1. **許多物聯網架構師嘗試透過優化 Linux 核心排程器（如調整 `nice` 負值或使用 `SCHED_FIFO`）來保障即時馬達控制，但為什麼在嚴酷的網路洪水攻擊（Network Flood）下這依然無法提供硬即時（Hard Real-Time）保證？**
   - 在標準 Linux 核心中，硬體中斷服務常式（Top-half ISR）與部分底層軟體中斷（Softirq）的執行優先級**高於任何使用者空間執行緒**（包括最高即時優先級的 `SCHED_FIFO` 執行緒）。當網路卡遭遇海量封包轟炸時，CPU 會不斷被硬體中斷強行搶佔，陷入切換驅動佇列與處理 socket 緩衝區的泥淖中；此外，標準核心內部的重大全域自旋鎖會關閉搶佔，`SCHED_FIFO` 執行緒即使被喚醒也只能在旁邊空轉等待鎖釋放。因此，不更換為 PREEMPT_RT 核心或硬體隔離，單純調整排程優先權在網路洪水面前只是一廂情願。

2. **異質多核心（Heterogeneous AMP，如 Cortex-A + Cortex-M）架構如何從根本上切斷「網路可用性破壞」與「實體安全控制」之間的傳播鏈？工程師在設計兩者之間的處理器間通訊（IPC）時必須注意什麼安全防線？**
   - **根本切斷機制**：Cortex-M 核心擁有自己獨立的中斷控制器（NVIC）、專屬的內部 SRAM 與獨立的周邊匯流排，它在實體線路上與 Ethernet/Wi-Fi 介面完全斷開。外部網路無論湧入多少千萬個惡意封包，都只能觸發 Cortex-A 的 GIC 中斷，物理上根本無法向 Cortex-M 發送任何電氣中斷訊號，從根本上保證了致動控制的微秒級確定性。
   - **IPC 設計防線**：在設計共享記憶體通訊（如 RPMsg）時，Cortex-M 絕不能盲目信任 Cortex-A 傳來的所有指標與資料結構！因為 Cortex-A 的 Linux 系統一旦被黑客攻陷，黑客可以修改共享記憶體中的緩衝區。Cortex-M 必須實施嚴格的**輸入驗證、邊界檢查（Bounds Checking）與靜態記憶體隔離**，確保惡意畸形訊息不會引發 Cortex-M 內部的記憶體溢位。

---

### 我的整理：本章帶走的五句話

1. **可用性與即時確定性是工控與物聯網裝置功能安全（Safety）的生命線。**
2. **網路洪水（DoS）會引發中斷風暴，能在微秒尺度上徹底癱瘓標準 Linux 核心的排程。**
3. **若必須在單晶片 Linux 上執行即時任務，PREEMPT_RT 即時補丁是收斂最大延遲的最低門檻。**
4. **硬即時控制的最佳架構解法是異質多核心（AMP）：讓 Cortex-M 專注控制，遠離網路中斷。**
5. **在即時核心路徑上，必須杜絕動態記憶體配置（`malloc`）並部署獨立硬體看門狗。**

---

### 延伸資源（p.276, p.281, p.287）

- 即時 Linux 官方組織：Open Source Automation Development Lab (OSADL): *PREEMPT_RT Documentation*
- 即時延遲檢測標準工具：`rt-tests` 專案與 `cyclictest` 基準測試套件
- 異質多核心跨平台通訊框架：The OpenAMP (Open Asymmetric Multi-Processing) Framework
- 工業網路抗壓測試規範：IEC 62443-4-2 Communication Robustness Requirements (CR 7.6)

---
## 第 11 章：Access Control and Management（存取控制與管理：Linux DAC、最小權限與 AppArmor 強制存取控制）

> 對應原書 p.294–322

### 這章在幹嘛

在物聯網與智慧型裝置領域，存在一個長年被開發者視為理所當然的荒謬常態——**「因為這是嵌入式設備，所以所有東西都用 root 執行」**。

作者在本章開宗明義指出：這是造成現代物聯網殭屍網路（如 Mirai）肆虐的最大元兇。當 Web 伺服器、MQTT 客戶端、藍牙常駐行程全部以超級使用者 `root`（UID 0）身分執行時，攻擊者只要在 Web 介面找到一個簡單的目錄遍歷（Directory Traversal）或緩衝區溢位漏洞，就瞬間獲得了整台設備的完全控制權（卸載開機安全模組、竄改韌體、甚至將設備變成攻擊外網的跳板）。

作者帶領讀者從基礎的 Linux 權限重構，一路推進到現代安全作業系統的核心武器——**強制存取控制（Mandatory Access Control, MAC）**。整章架構分為四大模組與兩大實戰案例：
1. **日常威脅與損害圍堵（Damage Containment）**：最小權限原則在全生命週期（研發、量產、運維、除役）的實踐。
2. **自主存取控制（Discretionary Access Control, DAC）深度剖析**：Linux 使用者／群組架構、檔案權限（`rwx`）、危險的 SUID/SGID 特權位元、`umask` 設定與 POSIX ACL 存取控制清單。
3. **實戰案例 1：STM32MP157F OpenSTLinux 韌體權限加固**：
   - 透過 Yocto 專案自動化建立專屬低權限使用者（`useradd`）。
   - 徹底消滅預設無密碼的 root shell。
   - SSH 伺服器安全性配置加固（禁用 root 登入、停用密碼、僅限公鑰）。
4. **強制存取控制（MAC）與 Linux 安全模組（LSM）**：
   - 為何 DAC 必然會失效？資源所有者隨意授權與 root 越權問題。
   - SELinux vs. AppArmor 架構對決：標籤型強制執行（Type Enforcement）vs. 路徑導向設定（Path-based Profiles）。
5. **實戰案例 2：使用 AppArmor 實現應用程式砂箱拘束（Confinement）**：
   - 在 OpenSTLinux 啟用 AppArmor 核心支援。
   - 使用 `aa-genprof` 進行動態行為側寫與規則生成。
   - 使用 `aa-enforce` 實施強制阻絕，實測攔截未授權的敏感檔案讀取（`/etc/shadow`）與網路綁定。

---

### 重點 1：最小權限原則（Least Privilege）與損害圍堵

安全架構的核心不是「假設系統永遠不會被黑」，而是「假設某個服務被攻陷後，黑客能造成的損害被嚴密圍堵在最小沙盒內」：

> **原文引用（p.294）**
> "Access control is fundamentally about damage containment. If an attacker discovers a vulnerability in your web server daemon, the question is not whether they can exploit it, but what they can do next. If the daemon runs as root, game over: the attacker owns the entire system. If the daemon runs as a restricted user confined by AppArmor, the attacker is trapped inside an isolated sandbox, unable to access cryptographic keys, spy on sensors, or pivot to the local network."

---

### 重點 2：Linux DAC 機制的精髓與 SUID 炸彈

Linux 傳統的自主存取控制（DAC）依賴三組經典許可權（Owner, Group, Other）以及讀（r）、寫（w）、執行（x）：

#### 1. 致命的 SUID（Set User ID）特權位元
當一個執行檔被賦予 SUID 位元（`chmod u+s /path/binary`，檔案權限呈現 `-rwsr-xr-x`）時，任何低權限使用者執行該程式時，該程式會**以該檔案擁有者（通常是 root）的完整權限執行**！
- 經典案例：`/bin/su`、`/usr/bin/passwd`。
- **漏洞爆發**：嵌入式系統中常有工程師為了方便，將自己寫的硬體診斷工具或除錯二進制檔設定為 SUID-root。只要該二進制檔存在緩衝區溢位或呼叫了不安全的 `system()` 函數，黑客即可實現秒級本機權限提升（Local Privilege Escalation）！

#### 2. `umask` 遮罩的預防作用
`umask` 決定了應用程式建立新檔案時的預設權限：
- 寬鬆預設值 `0022`：新檔案為 `644`（所有其他使用者皆可讀取！若存放日誌或金鑰將瞬間洩漏）。
- 加固設定 `0027` 或 `0077`：新檔案僅擁有者可讀寫，阻絕任何橫向存取。

---

### 重點 3：實戰案例 1：STM32MP157F Yocto 權限加固實務

作者展示了如何在 Yocto Project 建置系統中，從原始碼層級消除「預設 root 登入」的陋習：

#### 1. 在 Yocto 配方（Recipe）中自動建立非特權使用者
```bitbake
inherit useradd

USERADD_PACKAGES = "${PN}"
USERADD_PARAM_${PN} = "-u 1000 -d /home/appuser -s /bin/sh -g appgroup -G dialout,netdev appuser"
GROUPADD_PARAM_${PN} = "-g 1000 appgroup"
```

#### 2. SSH 伺服器安全性鎖定（`/etc/ssh/sshd_config`）
```text
# 徹底禁止 root 透過 SSH 登入
PermitRootLogin no

# 禁用密碼認證，強制僅允許 SSH 數位金鑰對認證
PasswordAuthentication no
PermitEmptyPasswords no

# 限制僅允許特定使用者群組登入
AllowUsers appuser
```

---

### 重點 4：為什麼 DAC 不夠用？強制存取控制（MAC）的降臨

即使你精心為每個服務建立了獨立的非特權使用者，DAC 仍有致命缺陷：
1. **所有者隨意授權**：低權限使用者可以隨意將自己目錄下的檔案 `chmod 777`，向全世界公開。
2. **Root 是神**：一旦攻擊者透過核心漏洞或 SUID 二進制提升至 UID 0，DAC 的所有檔案權限檢查被作業系統底層無條件直接跳過！

**強制存取控制（MAC）的顛覆性改變**：
- 在 MAC 機制下，**系統上沒有任何人（包括 root！）可以隨心所欲**。
- 由作業系統安全架構師撰寫全域安全策略（Security Policy）。
- 即使行程以 root 身分執行，只要策略規則中沒有明文允許該行程寫入某個目錄，Linux 核心便會**直接拒絕存取並記錄稽核日誌**！

---

### 重點 5：LSM 架構對決：SELinux vs. AppArmor

Linux 核心透過 **LSM（Linux Security Modules）** 框架支援 MAC。在嵌入式領域，兩大霸主的定位鮮明：

| 評比維度 | SELinux（Security-Enhanced Linux） | AppArmor（Application Armor） |
| :--- | :--- | :--- |
| **主導組織** | 美國國家安全局（NSA）與 Red Hat | Canonical（Ubuntu）與 SUSE |
| **存取標的識別** | **基於 Inode 擴充屬性標籤（Security Context Labels）** | **基於檔案系統路徑（Filesystem Paths）** |
| **策略語法複雜度** | 極高，包含 Type Enforcement, Roles, Domains, MLS | **極簡，人類直觀可讀的路徑規則語法** |
| **除錯與學習門檻** | 極其陡峭，小型團隊極易因策略衝突而直接關閉 | **極度平緩，具備直覺的動態學習工具（`aa-genprof`）** |
| **嵌入式團隊首選** | 適合具備專職資安團隊之大型 Android / 國防專案 | **中小型嵌入式 Linux、IoT 與工業設備的最佳實務解** |

> **原文引用（p.312）**
> "While SELinux provides powerful and mathematically rigorous type enforcement, its complexity is legendary. Developing and maintaining custom SELinux policies for an embedded system requires deep expertise; all too often, frustrated developers simply set SELinux to permissive or disabled.
> AppArmor, in contrast, uses pathname-based profiles that are human-readable and straightforward to write. For resource-constrained embedded teams, AppArmor offers 90% of the security benefits with 10% of the administrative pain."

---

### 重點 6：實戰案例 2：使用 AppArmor 拘束自訂網路守護行程

作者在 STM32MP157F-DK2（OpenSTLinux 核心開啟 `CONFIG_SECURITY_APPARMOR=y`）上，完整示範了拘束一個名為 `sample-daemon` 的應用程式：

#### 1. 使用 `aa-genprof` 進行行為側寫
```bash
# 啟動學習監控模式
# aa-genprof /usr/bin/sample-daemon
```
- `aa-genprof` 會將 AppArmor 置於 **投訴模式（Complain Mode）**。
- 工程師在此時讓應用程式執行正常的各項網路與檔案讀寫操作。
- 工具會自動擷取所有系統呼叫，以互動式問答詢問工程師是否授權這些路徑。

#### 2. 產生的 AppArmor 描述檔語法（`/etc/apparmor.d/usr.bin.sample-daemon`）
```text
#include <tunables/global>

/usr/bin/sample-daemon {
  #include <abstractions/base>
  #include <abstractions/nameservice>

  # 僅允許讀取自身的設定檔
  /etc/sample-daemon/config.json r,

  # 僅允許寫入自身的日誌目錄
  /var/log/sample-daemon.log rw,

  # 允許綁定 TCP 8080 連接埠
  network inet tcp,

  # 嚴格禁止存取任何系統機密 (黑名單明確阻絕)
  deny /etc/shadow r,
  deny /etc/ssh/* r,
}
```

#### 3. 實施強制阻絕模式（Enforce Mode）與攻擊測試
```bash
# 切換為強制執行模式
# aa-enforce /etc/apparmor.d/usr.bin.sample-daemon
```
**模擬攻擊實測**：
當黑客透過路徑遍歷漏洞，驅使 `sample-daemon` 嘗試讀取 `/etc/shadow` 或 `/etc/ssh/ssh_host_rsa_key` 時，Linux 核心 LSM 掛鉤立即攔截此係統呼叫，應用程式收到 `EACCES（Permission Denied）`，系統日誌拋出稽核警報：
```text
kernel: audit: type=1400 audit(1620000000.123:45): apparmor="DENIED" 
operation="open" profile="/usr/bin/sample-daemon" name="/etc/shadow" 
pid=1234 comm="sample-daemon" requested_mask="r" denied_mask="r"
```
黑客即使控制了程式邏輯，也被硬性囚禁在 AppArmor 的牢籠內，無法越雷池一步！

---

### 本章 Summary（作者原話）

> **原文引用（p.321）**
> "Access control is an enormously broad topic that could ﬁll a whole book on its own. It encompasses the basic conﬁgurations of users, groups, directory structures, and access permissions (as discussed in this chapter for the Linux DAC system), the deactivation of hardware debugging features and tools not meant for end users, and the complex ﬁeld of OS-enforced MAC policies that have to be ﬁne-tuned to the individual behavior and resource-access demands of a speciﬁc application.
> And while I don’t expect you to invest all your time designing perfect access-control settings, there’s no way around this matter in secure device development.
> We explored hardening techniques for DAC, including eliminating the root-only paradigm, and examined the decisive advantages of Mandatory Access Control.
> The practical case study on the STM32MP157F demonstrated that tools like AppArmor provide a practical, human-manageable path to confine network-facing applications and contain damage when vulnerabilities are inevitably exploited."

---

### 思考與實務檢驗

1. **為什麼在嵌入式 Linux 設備中，單純依賴 DAC（標準 Linux 使用者與檔案權限）來防禦攻擊者是不夠的？為什麼必須引入強制存取控制（MAC，如 AppArmor）？**
   - DAC 存在兩大致命極限：
     1. **全有或全無的特權模型**：在 DAC 下，UID 0（root）擁有超然的神級權限，核心不會對 root 執行任何檔案讀寫阻擋。許多物聯網常駐程式因為需要綁定低於 1024 的網路連接埠或存取硬體 GPIO，被迫以 root 身分啟動。一旦該常駐程式被攻陷，黑客直接以 root 身分接管整台主機。
     2. **無法防禦跨低權限使用者的橫向移動**：若所有服務都跑在未沙盒化的空間中，黑客攻陷一個服務後，可以隨意掃描系統中所有具備「世界可讀（World-readable, 如 `chmod 644`）」屬性的檔案（如其他服務的設定檔、日誌、資料庫）。
   - MAC（如 AppArmor）將權限約束錨定在「程式二進制路徑」而非「使用者 ID」。即使程式是 root，AppArmor 依然依據描述檔強制剝奪其讀取無關檔案、建立未授權 socket 或載入核心模組的特權，達成真正的深層隔離。

2. **在資源受限的嵌入式團隊中，挑選 AppArmor 相較於 SELinux 具有哪些務實的工程優勢？在什麼情況下路徑導向（Path-based）的 AppArmor 可能面臨繞過風險？**
   - **工程優勢**：SELinux 基於檔案系統 Inode 擴充屬性標籤，重新刷寫或掛載不同檔案系統時極易發生標籤遺失；且 SELinux 語法晦澀難懂，開發者經常因為無法解決的權限阻礙而全盤放棄。AppArmor 採用人類可讀的路徑語法，且提供 `aa-genprof` / `aa-logprof` 等強大的半自動化側寫學習工具，大幅降低團隊的維護負擔。
   - **路徑導向的繞過風險**：由於 AppArmor 依據檔案路徑而非 Inode 進行過濾，若系統存在硬連結（Hard Links）、符號連結（Symlinks）、未受保護的 chroot 環境或利用 `mount --bind` 重新掛載目錄，攻擊者可能試圖透過替代路徑名稱繞過路徑字串比對規則。因此，AppArmor 描述檔必須嚴格引入全域抽象規則（如 `<abstractions/base>`），並嚴格限制沙盒程式建立硬連結與呼叫 `mount` 系統呼叫的能力。

---

### 我的整理：本章帶走的五句話

1. **「因為是嵌入式所以全用 root 跑」是物聯網最大的資安毒瘤，必須堅決落實最小權限原則。**
2. **SUID-root 二進制檔是本機提權的定時炸彈，量產韌體中必須清查並剝奪不必要的特權位元。**
3. **DAC 是自主性的，遇到 root 立即失效；MAC 是強制性的，即使 root 也必須服從安全策略。**
4. **AppArmor 是中小型嵌入式團隊的最佳實務 MAC 方案，具備極高的可維護性與動態側寫工具。**
5. **存取控制的核心在於損害圍堵：讓外部暴露的應用程式身處沙箱，即使被攻陷也拿不到核心機密。**

---

### 延伸資源（p.304, p.312, p.318）

- AppArmor 官方技術指南：The AppArmor Project Documentation & Profiling Guide
- Linux 安全模組架構：Linux Kernel Documentation: *Linux Security Modules (LSM)*
- Yocto 專案資安加固層：`meta-security` Layer for OpenEmbedded / Yocto Project
- 最小權限工業標準：IEC 62443-4-2 Technical Security Requirements: Account Management & Least Privilege (CR 1.1 / CR 2.1)

---
## 第 12 章：System Monitoring（系統監控與日誌審計：快閃記憶體壽命平衡、auditd 與防篡改日誌）

> 對應原書 p.323–342

### 這章在幹嘛

在縱深防禦體系中，即使架構師部署了堅不可摧的 Secure Boot、硬體加密記憶體、TLS 1.3 與 AppArmor，仍必須面對一個無可迴避的殘酷工程定律：**這世界上不存在絕對完美的防禦，突破永遠只是時間與成本的問題**。

當前線防護遭到未知零日漏洞（0-day）突破時，**「系統監控與日誌審計（System Monitoring and Logging）」是企業得知自己「被黑了」的最後一道底線**。如果沒有監控，攻擊者可以在設備內潛伏數年，神不知鬼不覺地竊取通訊、竄改感測數據或將設備作為內網滲透跳板。

然而，在嵌入式 Linux 與物聯網設備上實作日誌監控，面臨著一個極為殘酷的硬體物理限制——**快閃記憶體寫入壽命耗盡（Flash Wear-Out）**。若未經深思熟慮就開啟大量 Verbose 日誌，磁碟會在短短幾個月內被日誌活活寫死，導致整台設備硬體永久損壞！

整章分為五大核心維度與一場精彩的嵌入式審計實務：
1. **監控的核心目的與三大價值**：攻擊即時偵測（Detection）、事後數位鑑識（Forensics）與國際法規合規性（IEC 62443 / 歐盟網路韌性法案 CRA）。
2. **監控四大黃金維度**：使用者互動（登入與提權）、網路通訊（未知連線與異常頻寬）、應用程式行為（崩潰與子行程衍生）與核心底層狀態。
3. **嵌入式物理約束與快閃記憶體磨損困境**：eMMC / NAND Flash 的 P/E 循環物理限制；記憶體暫存（`tmpfs` / `RAMlog`）、環形緩衝區與日誌等級過濾策略。
4. **設備端日誌的防篡改架構**：黑客取得 root 後執行 `rm -rf /var/log/*` 的反制之道；前向安全日誌（Forward-Secure Logging / FSS 雜湊鏈）與即時遠端串流傳送。
5. **中央日誌分析與 SIEM 架構**：Syslog 協定（RFC 5424）、中央日誌收集器與自動化關聯告警。
6. **實戰案例：STM32MP157F 全方位事件日誌架構實作**：
   - 使用 `systemd-journald` 進行本機揮發性記憶體日誌管理。
   - 在 Yocto 中整合 Linux 核心審計框架 **`auditd`**。
   - 使用 `auditctl` 佈署關鍵檔案監視（`/etc/shadow` 防篡改）與 `execve` 系統呼叫執行追蹤。
   - 使用 `ausearch` 與 `aureport` 提取資安事件。
   - 透過 `rsyslog` 結合 TLS 加密通道，將關鍵安全日誌即時推播至遠端中央伺服器。

---

### 重點 1：監控的本質：及早發現黑天鵝，避免企業信譽破產

作者在開篇強調了監控在現代縱深防禦中不可取代的地位：

> **原文引用（p.324）**
> "System monitoring is an essential layer within your defense-in-depth strategy because even if all the protection measures you implement fail, monitoring might enable you to detect ongoing attacks and anomalies. Monitoring can make the difference between getting away with a black eye and ruining your product’s and your company’s reputation."

**監控三大核心任務**：
1. **即時威脅偵測（Detection）**：在黑客進入系統的初期（例如反覆猜測密碼、探測內部通訊埠）即時阻斷其橫向擴散。
2. **根因鑑識分析（Root Cause Forensics）**：在資安事故發生後，能夠準確還原「黑客從哪個漏洞進來、碰了哪些檔案、偷了什麼資料」。
3. **法規遵循（Compliance）**：IEC 62443、ISO 21434 與歐盟 Cyber Resilience Act (CRA) 均強制要求聯網設備必須具備安全事件記錄與審計追蹤能力。

---

### 重點 2：監控四大黃金標的

嵌入式設備資源有限，不能盲目記錄所有封包，必須採取**基於風險的監控（Risk-Based Monitoring）**：

| 監控面向 | 核心關注指標 | 異常徵兆範例 |
| :--- | :--- | :--- |
| **使用者互動與身分驗證** | SSH / 終端機登入成功與失敗次數、`sudo` 呼叫、密碼變更 | 短時間內數十次 root 登入失敗（字典暴力破解）、非工作時間的管理員登入。 |
| **網路通訊行為** | 外發連線目標 IP、異常通訊埠開放、資料傳輸流量計數 | 設備突然嘗試向海外未知 IP 建立 IRC 或未列出的 TCP 連線（殭屍網路 C2 簽到）。 |
| **應用程式行為** | 行程異常終止（Segfault）、核心傾印（Core Dump）、常駐程式衍生未知 Shell | Web 伺服器行程突然呼叫了 `/bin/sh`（典型的遠端指令注入攻擊成功信號！）。 |
| **系統與核心層狀態** | 檔案系統掛載變更、核心模組載入（`insmod`）、CPU/記憶體突發暴漲 | 唯讀分區被重新掛載為可讀寫（`remount,rw`）、非預期的核心模組加載。 |

---

### 重點 3：嵌入式特有危機：快閃記憶體壽命耗盡（Flash Wear-Out）

這是在伺服器開發中絕不存在、卻在嵌入式系統中屢見不鮮的致命硬體陷阱：

> **原文引用（p.330）**
> "Flash memory has a strictly limited number of program/erase (P/E) cycles—typically between 3,000 and 10,000 cycles for raw NAND or consumer eMMC. If an application or logging daemon writes verbose debug messages to non-volatile flash continuously, it can completely exhaust the flash memory’s endurance in just a few months, destroying the physical hardware! Embedded monitoring must balance security visibility with flash longevity."

**快閃記憶體壽命保護四大工程防禦策**：
1. **記憶體暫存（RAM-based Volatile Logging）**：將 `/var/log` 掛載為 `tmpfs`（RAM 磁碟）。日常除錯日誌全部寫入記憶體，斷電自動拋棄。
2. **嚴格分級落盤（Severity Filtering）**：僅將 `WARNING`、`ERROR` 與 `CRITICAL` 等級的資安稽核事件寫入非揮發性儲存；`DEBUG` 與 `INFO` 僅保留在 RAM 環形緩衝區中。
3. **日誌旋轉與容量配額限制（Log Rotation & Quotas）**：透過 `logrotate` 或 `journald.conf` 嚴格限制日誌檔案總體積上限（例如不得超過 16 MB），滿額自動循環覆蓋。
4. **寫入節流與聚合（Rate Limiting & Aggregation）**：限制每秒最大寫入日誌條數，防止攻擊者藉由刻意觸發大量報錯來進行「快閃記憶體耗盡型 DoS 攻擊」。

---

### 重點 4：本機日誌的防篡改挑戰：前向安全日誌（Forward-Secure Logging）

當黑客成功攻破設備並取得 root 權限時，他們的第一個動作一定是：
```bash
# 擦除攻擊痕跡
# rm -rf /var/log/*
# history -c
```
如果日誌全在本地，黑客可以隨意偽造或清空日誌，讓鑑識人員無跡可尋。

**兩大防禦對策**：
1. **即時遠端串流（Remote TLS Streaming）**：日誌一旦產生，在數毫秒內透過 TLS 連線同步推送至遠端 SIEM 伺服器；黑客即使隨後取得 root，也只能刪除本機殘留，無法抹除伺服器上已儲存的數位證據。
2. **前向安全密封（Forward Secure Sealing, FSS）**：
   以 `systemd-journald` 內建的 FSS 技術為例：
   - 系統依據密碼學雜湊鏈（Hash Chain）定時對日誌區塊進行不可逆密封簽署。
   - 每次密封後，先前輪次的簽署金鑰立即在記憶體中銷毀。
   - **防禦效果**：黑客即使在第 10 分鐘拿到了當前的系統金鑰，也無法偽造或修改前 9 分鐘已經被密封的歷史日誌條目，任何對歷史日誌的竄改都會導致雜湊驗證失敗！

---

### 重點 5：實戰案例：STM32MP157F 全方位審計日誌實戰

作者在 STM32MP157F 開發板上整合了 Linux 核心最高規格的審計架構：

#### 1. Yocto 中整合 `auditd` 核心審計工具
在 Yocto 建置設定中加入：
```bitbake
IMAGE_INSTALL += "auditd"
```
編譯後，Linux 核心內建的 Audit 子系統會在系統開機時由 `auditd` 常駐行程接管。

#### 2. 配置關鍵安全審計規則（`auditctl`）
```bash
# 1. 監控密碼與權限核心檔案的任何寫入或屬性變更
# auditctl -w /etc/shadow -p wa -k shadow_tamper
# auditctl -w /etc/passwd -p wa -k passwd_tamper

# 2. 監控所有提權執行指令 (execve 系統呼叫)
# auditctl -a always,exit -F arch=b32 -S execve -k command_execution
```

#### 3. 攻擊實測與審計報告分析（`ausearch` / `aureport`）
當非特權使用者或後門腳本嘗試存取 `/etc/shadow` 時，`auditd` 會在 `/var/log/audit/audit.log` 記錄精確的二進制系統呼叫細節：
```bash
# 快速檢索標記為 shadow_tamper 的安全事件
# ausearch -k shadow_tamper --interpret
```
- 輸出中清晰記錄了：攻擊發生時間、發起攻擊的真實 UID/GID、行程名稱（`comm`）、精確的系統呼叫號碼（`syscall=openat`）以及存取結果（`success=no, exit=-13 EACCES`）。
- 使用 `aureport --auth` 可在一秒內產生所有登入成敗的統計摘要報表。

#### 4. 透過 `rsyslog` 實現 TLS 加密遠端轉發
在 `/etc/rsyslog.conf` 中配置安全傳輸通道：
```text
# 載入 TLS 網路串流驅動
$DefaultNetstreamDriver gtls
$DefaultNetstreamDriverCAFile /etc/ssl/certs/central_ca.crt
$ActionSendStreamDriverMode 1
$ActionSendStreamDriverAuthMode x509/name

# 將所有 auth 與 audit 事件透過 TCP+TLS 即時轉發給中央 SIEM 伺服器
auth,authpriv.* @@siem.mycompany.internal:6514
```

---

### 本章 Summary（作者原話）

> **原文引用（p.341–342）**
> "It’s simple to request that a device should aggregate and transmit system data and event information to a central server. However, when you start implementing such measures, you’ll encounter several obstacles to overcome, from legal issues to device constraints to the human resources necessary for continuously operating security-monitoring services.
> In this chapter, I discussed typical information that can be useful for security monitoring of embedded systems, from user sessions and their interactions to network trafﬁc metadata to system crashes and error messages. Clearly, the more you want to monitor, the more on-device and server resources you need. A risk-based approach is absolutely reasonable to ﬁnd a compromise between full device transparency and minimal resource usage for your speciﬁc product.
> System monitoring is an essential layer within your defense-in-depth strategy because even if all the protection measures you implement fail, monitoring might enable you to detect ongoing attacks and anomalies.
> Monitoring can make the difference between getting away with a black eye and ruining your product’s and your company’s reputation."

---

### 思考與實務檢驗

1. **在嵌入式設備設計中，為什麼「毫無節制的本機日誌記錄」本身就是一種嚴重的可用性威脅（Flash Wear-out）？架構師該如何在「資安審計需求」與「快閃記憶體壽命」之間取得平衡？**
   - 嵌入式 Flash（NAND / eMMC）的物理擦寫次數有限（P/E 週期為幾千次），且寫入必須以「區塊（Block）」為單位進行抹除。若程式碼頻繁寫入單行日誌，底層 Flash 控制器會發生嚴重的「寫入放大（Write Amplification）」，在數月內將晶片扇區徹底寫毀，導致檔案系統變為唯讀或硬體報廢，造成嚴重的阻斷服務。
   - **平衡策略**：
     1. **日誌分流與 RAM 緩衝**：99% 的一般資訊日誌全數導向記憶體（`tmpfs`），由環形緩衝區覆蓋，完全不碰 Flash；
     2. **嚴格過濾與批次落盤**：僅有代表入侵跡象的高優先權資安事件（如認證失敗、AppArmor 攔截、檔案篡改）才允許寫入 Flash，且透過批次緩存減少抹除頻率；
     3. **動態節流機制**：當單位時間內的日誌產生量超過安全閾值時，強制啟動節流（Rate Limiting），防止日誌洪水燒毀晶片。

2. **當攻擊者成功取得設備的 root 權限後，本機記錄的所有日誌皆可能遭到偽造或清除。有哪兩種核心技術可以確保日誌的「不可篡改性」與「數位證據效力」？**
   - **第一種：即時遠端異地備份（Real-time Remote Streaming over TLS）**：
     日誌產生的瞬間立即透過加密連線即時推送到外部隔離的中央伺服器（SIEM）。由於資料已經離開設備，攻擊者即使稍後取得本地 root 權限並 `rm -rf /var/log`，也無法追溯刪除遠端已固化的日誌。
   - **第二種：密碼學前向安全日誌（Forward-Secure Logging, 如 FSS）**：
     每隔一個時間區間（例如每 10 分鐘），日誌守護行程使用當前的單向金鑰對日誌區塊進行雜湊簽章，隨後立即以單向演算法將金鑰推進至下一階段並銷毀舊金鑰。攻擊者即使在第 1 個小時攻陷設備並提取出記憶體中的金鑰，也無法偽造或無痕竄改前 50 分鐘的歷史日誌，在數學上保證了鑑識證據的法律效力。

---

### 我的整理：本章帶走的五句話

1. **監控是縱深防禦的最後一道網；沒有監控的系統，連自己何時淪陷都一無所知。**
2. **基於風險的監控是嵌入式系統的核心思維：只聚焦於認證、連線、行程衍生與核心例外。**
3. **快閃記憶體寫入壽命（Flash Wear-Out）是硬體硬約束，日誌必須利用 RAM 緩存與分級落盤。**
4. **本機日誌防不住 root 黑客；具備法律效力的日誌必須依賴即時遠端串流與前向安全密封（FSS）。**
5. **Linux 核心審計（`auditd`）能深入攔截系統呼叫，是偵測權限提升與核心檔案篡改的利器。**

---

### 延伸資源（p.326, p.331, p.337）

- Linux 核心審計架構文檔：Linux Audit Framework Documentation & Rules Guide
- 系統日誌標準規範：RFC 5424 *The Syslog Protocol*
- systemd 前向安全日誌：systemd-journald: *Forward Secure Sealing (FSS) Specification*
- 歐盟聯網設備資安法規：European Union Cyber Resilience Act (CRA) - Post-Market Monitoring Requirements

---
## 結語（Afterword）：超越基礎的進階攻防地平線

> 對應原書 p.343–345

### 這章在幹嘛

當讀者完成了本書前 12 章的旅程，成功在開發板上構建了完整的安全開發流程、真隨機數、硬體加速密碼學、安全儲存、裝置身分、TLS 1.3、Secure Boot、SWUpdate、即時強健架構、AppArmor 與系統監控時，作者在全書結語向所有嵌入式工程師致敬，同時給予了一劑極為清醒的警示：**「請先別急著開香檳慶祝，這只是你安全工程長征的第一步，而不是故事的終點。」**

在現代高度對抗的硬體攻防領域，特別是在付費電視（Pay-TV）、汽車電子、遊戲主機與金融晶片卡等具備巨大經濟利益的戰場上，強大的對手早已超越了傳統的軟體漏洞利用，轉而發動**物理層級與硬體層級的終極攻擊**。

作者在結語中精闢勾勒了嵌入式安全工程師的三大進階修煉地平線：

---

### 地平線 1：側通道分析（Side-Channel Analysis, SCA）

過去二十年，側通道攻擊從學術實驗室全面走向工業實戰。黑客不再需要破解 AES 或 RSA 的深奧數學，而是透過監聽微晶片在運算時向物理世界散發的副產物：
- **微秒級執行時間波動（Timing Analysis）**
- **晶片動態功耗軌跡（Power Analysis: SPA, DPA, CPA）**
- **晶片表面高頻電磁輻射（Electromagnetic Analysis, EMA）**

攻擊者只要在晶片的供電腳位串接一個微小電阻，用示波器抓取幾千條加解密瞬間的電流波形，利用統計學上的皮爾森相關係數（Pearson Correlation），就能在幾分鐘內直接萃取出晶片內部保護得最嚴密的 AES-256 私鑰！

> **延伸典範文獻推薦**：
> - Stefan Mangard, Elisabeth Oswald, Thomas Popp (2007) *Power Analysis Attacks: Revealing the Secrets of Smart Cards* (Springer) —— 側通道分析的傳世奠基之作。
> - Jasper van Woudenberg & Colin O’Flynn (2021) *The Hardware Hacking Handbook* (No Starch Press) —— 本書技術審校 Colin O’Flynn 所著的硬體實戰寶典。

---

### 地平線 2：故障注入攻擊（Fault Injection Attacks, FI）

如果晶片的密碼演算法完全常數時間、電路經過平衡抗功耗分析，黑客該怎麼辦？答案是：**物理干擾晶片讓它「出錯」**！
- **電壓突波／驟降（Voltage Glitching）**：在開機比對簽章的瞬間，拉低供電電壓數十奈秒。
- **時鐘毛刺（Clock Glitching）**：在時脈線路上插入一個微小的超高頻窄脈衝。
- **電磁脈衝（EMFI）與雷射注入（Optical/Laser Fault Injection）**：以雷射精確聚焦照射在矽晶片的快顯記憶體或指令解碼邏輯上。

**破壞後果**：
故障會導致 CPU 的程式計數器（PC）直接跳過關鍵的條件跳轉指令（例如把 `if (verify_signature() != OK) halt();` 直接跳過），使未經簽署的盜版韌體成功開機；或是在 RSA 運算中觸發前述的 Bellcore 故障，直接推導出私鑰。

---

### 地平線 3：深層軟體隔離與硬體信託環境（TEE）

隨著嵌入式系統功能日益龐雜，現代高階產品開始引入更極致的軟體隔離架構：
- **第一型嵌入式 Hypervisor（Type-1 Virtualization）**：如 Xen、Jailhouse、ACRN，將一般 Linux 與關鍵安全作業系統以硬體虛擬化徹底物理隔絕。
- **ARM TrustZone 安全執行環境（TEE / OP-TEE）**：在硬體層面劃分出 **Normal World（一般世界）** 與 **Secure World（安全世界）**。敏感的加解密金鑰、生物辨識特徵與付費邏輯永遠被鎖在安全世界中，一般世界中的 Linux 即使整個被 rootkit 攻陷，也無法存取安全世界的一分一毫。

---

### 作者最後的話：安全工程不是魔法，而是團隊的堅持

> **原文引用（p.345）**
> "Despite all that, always remember one point: engineering secure devices doesn’t require magical powers. All you need are engaged, motivated, and clever people who work together as a team. Go for it!"

**筆記者全書總結**：
《Engineering Secure Devices》是一本極其罕見、完全從「嵌入式工程落地」視角出發的實戰鉅著。它不講空泛的資安口號，不堆砌無用的數學符號，而是以 STM32MP157F 與 Linux 為手術刀，帶領我們一行行解構了從 CMOS 物理噪聲到 Yocto 構建、從 eFuse 熔絲到 TLS 1.3、從 A/B 雙分區到 AppArmor 砂箱的全部防護環節。

**打造安全設備不需要超能力，需要的是對每個位元的敬畏、對物理邊界的洞察，以及在整個產品生命週期中永不懈怠的工程紀律！**
