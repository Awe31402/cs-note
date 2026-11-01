# 《Secure Boot Encryption with Linux: Implementation for Embedded Developers》讀書筆記

- **作者**：Rodolfo Giometti
- **出版**：Apress (Apress Pocket Guides)，2026 年，ISBN-13 (pbk): 979-8-8688-2817-1 / ISBN-13 (electronic): 979-8-8688-2818-8 / DOI: 10.1007/979-8-8688-2818-8
- **範例程式碼**：https://github.com/Apress/Secure-Boot-Encryption-with-Linux

**閱讀說明**：本筆記以中文（繁體）整理重點，引用保留英文原文（附原書印刷頁碼）。本書專為「嵌入式 Linux 開發者與安全架構師」編寫，重點聚焦於硬體信任根（Hardware Root-of-Trust）、Linux 核心金鑰管理系統（Kernel Keyring）、Crypto API、Transparent Encryption（fscrypt / dm-crypt）、Chain-of-Trust 安全開機流程、Rescue 與 A/B 分區實作，以及防篡改（Tamper Detection）與跨開機階段機密防護。書中未提到的內容一律標註「書中未提」，不做補充推測。

---
## 第 1 章：Linux Cryptography（Linux 密碼學基礎與核心設施）

> 對應原書 p.1–76

### 這章在幹嘛

這是全書的技術地基章。作者開宗明義指出，許多嵌入式系統的安全模型之所以失敗，是因為攻擊者一旦透過漏洞拿到 root 權限，整台機器的機密、金鑰與程式碼完整性就瞬間全裸暴露。

為了達成「即使 root 被攻陷，機密依然拿不走（root-proof）」的目標，本章全面剖析 Linux 核心內建的密碼子系統與防護設施。整章骨架分為四大部分：
1. **密碼學核心觀念釐清**：數位簽章（真實性／完整性）與加密（機密性）的角色劃分。
2. **Linux Crypto API 與 AF_ALG**：核心如何排定硬體加速優先權，以及使用者空間如何透過 socket 呼叫核心密碼引擎。
3. **Linux Key-Management Facility（Keyring）**：不可置換（non-swappable）核心記憶體、`logon` 金鑰、TPM/TEE 封裝的 `trusted`/`encrypted` 金鑰，以及 NXP CAAM 的黑金鑰（Black Key）。
4. **Linux 透明加密（Transparent Encryption）**：檔案系統級（fscrypt）與區塊裝置級（dm-crypt）的機制對比與實作。

---

### 重點 1：簽章金鑰 vs. 加密金鑰（本質與目的之別）

作者在開場先釐清數位簽章（Signature）與資料加密（Encryption）的根本差異。在安全開機架構中，這兩者肩負完全不同維度的防護責任：
- **數位簽章（Signature Keys）**：確保**真實性（Authenticity）**與**完整性（Integrity）**。目標是驗證「資料是誰發出的」以及「資料在簽署後是否遭到任何篡改」。
- **加密金鑰（Encryption Keys）**：確保**機密性（Confidentiality）**。目標是使未授權的任何人皆無法讀取原始資料內容。

> **原文引用（p.1–2）**
> "In cryptography, the use of signature keys or encryption keys is intended for two different purposes. The former are used to ensure authenticity and integrity; that is, the goal is to verify who sent the data and that the data has not been altered since it was signed. While the latter are used to ensure confidentiality, so the goal is to make data unreadable to anyone except the intended recipient."

**要記住的觀點**：開機映像檔（Bootloader/Kernel）通常需要「驗證簽章」以防程式碼被掉包；而根檔案系統或敏感資料則需要「加密」以防機密被讀取。

---

### 重點 2：對稱加密與非對稱演算法的取捨（速度 vs. 架構）

在演算法選型上，理論上非對稱加密（Asymmetric Encryption）也能做機密保護，但在嵌入式 CPU 開機情境下，執行效能具有決定性影響：

> **原文引用（p.2）**
> "To do a digital signature, we must use an asymmetric key pair, while for encryption, we can use asymmetric or symmetric encryption... However, due to speed issues, all CPUs use symmetric encryption for securing their code, so from now until the end of this book, we are going to use symmetric encryption only."

書中選定的標準基準演算法如下：
1. **加密（Encryption）**：採用 **AES-256-CBC**（Advanced Encryption Standard in Cipher Block Chaining mode）。CBC 模式透過將每個明文區塊與前一個密文區塊進行 XOR 運算，徹底消除相同明文產生相同密文的弱點，並搭配初始向量（IV）。
2. **數位簽章（Signature）**：採用 **ECDSA**（Elliptic Curve Digital Signature Algorithm，搭配 SHA-256 雜湊）。相較於依賴大數分解的傳統 RSA，基於橢圓曲線密碼學（ECC）的 ECDSA 能在顯著較小的金鑰長度下提供同等安全強度，大幅減少開機階段的運算延遲與儲存空間。

---

### 重點 3：Linux Crypto API 核心子系統架構與優先權機制

Linux 核心直接在核心空間內建完整的密碼演算法與硬體驅動架構，不僅提供給核心自身（如網路 IPsec、dm-crypt、fscrypt），亦可調用專屬硬體加速引擎（如 NXP CAAM、STM32 密碼模組）。

透過檢視 `/proc/crypto`，開發者可以觀察系統上已註冊的各項密碼演算法屬性：
- `name`：演算法標準名稱（例如 `cbc(aes)`、`sha256`）。
- `driver`：負責具體實作的驅動名稱（例如軟體實作 `cbc-aes-generic`，硬體加速實作 `cbc-aes-caam`）。
- `priority`：核心排程與選取演算法時的優先權數值。

> **原文引用（p.8–9）**
> "The priority-based selection is a way to select the best available implementation for a specific cipher according to each driver's priority... In this scenario, if we ask for the cbc(aes) algorithm, the kernel uses the one with the higher priority, in our example the one implemented by the driver cbc-aes-caam (if multiple implementations have the same highest priority, the kernel's behavior is undefined)."

**優先權選取原則**：
當應用程式或核心元件請求通用演算法名稱（如 `cbc(aes)`）時，核心自動綁定優先權最高（數值最大）的驅動程式。若欲強制指定特定軟體或硬體實作，必須在請求時直接宣告具體的驅動名稱（driver name）而非通用名稱。

---

### 重點 4：AF_ALG Socket 介面：使用者空間如何直通核心密碼層

傳統使用者空間應用若要呼叫硬體密碼加速器，往往需要專屬的專有 API。Linux 核心透過 `AF_ALG`（Application Layer Cryptography）通訊端介面，將核心密碼子系統抽象成標準 BSD Socket：

> **原文引用（p.9）**
> "The kernel's crypto layer can also be accessed by applications in user space by using the AF_ALG socket interface. This interface allows user-space programs to access the kernel's cryptographic algorithms (software or hardware, according to each priority)."

**通訊協定工作流程**：
1. **建立 Socket**：調用 `socket(AF_ALG, SOCK_SEQPACKET, 0)`。
2. **綁定演算法**：透過 `bind()` 傳入 `struct sockaddr_alg`，填入 `salg_family = AF_ALG`、`salg_type = "skcipher"`（或 `"hash"`）及 `salg_name = "cbc(aes)"`。
3. **設定金鑰**：對 bound socket 執行 `setsockopt(..., SOL_ALG, ALG_SET_KEY, key, keylen)`。
4. **建立運算描述元**：執行 `accept(sock, NULL, 0)` 取得操作連線。
5. **資料傳輸與運算**：使用 `sendmsg()` 透過輔助控制訊息（control messages, `cmsg`）傳入 IV（`cmsg_type = ALG_SET_IV`）與加解密方向（`ALG_SET_OP`），隨後使用 `read()` 讀取密文或明文。

OpenSSL 亦支援原生掛載此介面：
```bash
$ openssl enc -engine afalg -aes-256-cbc -e -K <key_hex> -iv <iv_hex>
```

---

### 重點 5：crypto-afalg 工具：避免密鑰洩漏於命令列的實作示範

在 OpenSSL 的命令列示範中，金鑰與 IV 往往必須透過參數 `-K` 或環境變數傳入，這在多工 Linux 系統中存在嚴重漏洞：任何本地未特權使用者皆能透過 `ps aux`、`/proc/<pid>/cmdline` 或 shell history 竊取十六進制金鑰。

作者自行撰寫並開源了 `crypto-afalg` 工具（https://github.com/giometti/crypto-afalg），專門解決金鑰在使用者空間的暴露問題：

> **原文引用（p.16）**
> "In the above output, we see that by using the -S option argument, we can specify a key by using its serial number within the Linux keyring. And this is a crucial feature for security because it allows us to specify a key that is held in the kernel!"

**使用範例對比**：
```bash
# 傳統不安全方式：金鑰在命令列完全暴露
$ openssl enc -engine afalg -aes-256-cbc -K 0123... -iv ...

# 安全方式：透過核心金鑰序號（Key Serial Number）調用
$ cat /tmp/secret.enc | crypto-afalg decrypt -S 779318636 cbc(aes) <IV>
```
金鑰實質本體僅存在於核心空間，使用者空間命令列與程序環境中只出現無機密價值的數值代號。

---

### 重點 6：Linux Keyring 金鑰管理系統：不可被 Swap 的安全記憶體

要達成上述「不將金鑰傳入使用者空間」，核心必須有獨立存放機密的設施——這就是 **Linux Keyring（金鑰環）**。

> **原文引用（p.18）**
> "The kernel keyring is a non-swappable key-management facility held in the kernel space designed to store secrets."

**核心特點與安全邊界**：
1. **非置換記憶體（Non-swappable）**：Keyring 中的金鑰存放於鎖定的核心實體記憶體中，保證永不會被作業系統 page out 到 Swap 分區，徹底杜絕從磁碟殘留還原金鑰的可能。
2. **階層式架構**：
   - `@t`（Thread Keyring）：特定執行緒專屬。
   - `@p`（Process Keyring）：特定程序內部共用。
   - `@s`（Session Keyring）：同一個登入 session 共享。
   - `@u`（User Keyring）：特定 UID 的使用者專屬金鑰庫。
   - `@us`（User Session Keyring）：使用者跨 session 共享。
3. **細緻權限模型（Permissions）**：
   - 區分 Possessor（持有人）、User、Group、Other。
   - 六種權限遮罩：`v`（view 元資料）、`r`（read 讀取內容）、`w`（write 修改載荷）、`s`（search 搜尋）、`l`（link 連結至其他 keyring）、`a`（setattr 設定屬性與逾時）。

---

### 重點 7：Plain Text Keys 的兩大類型：user key 與 logon key

在標準 Linux 金鑰類型中，最常見的明文金鑰（Plain Text Keys）分為兩類：

#### 1. User Key（`user` 類型）
由使用者空間程序注入，並**允許使用者空間讀回**載荷資料：
```bash
$ keyctl add user mykey "secret_payload" @u
$ keyctl read <key_id>
secret_payload
```
缺點：只要使用者具有讀取權限，金鑰載荷便能在使用者空間被 dump 出來。

#### 2. Logon Key（`logon` 類型）
專為核心內部服務（如檔案系統加密、網路驗證）設計的金鑰類型：

> **原文引用（p.26）**
> "Another difference between a logon key and a user key is that the former payload cannot be read back by user space (even by the root user)."

**安全性實測**：
```bash
# 必須包含冒號前綴 description，例如 cypher: 或 fscrypt:
$ echo -n "supersecret" | keyctl padd logon cypher: @s
779318636

# 嘗試使用 root 讀取 logon key 的載荷
$ keyctl pipe 779318636
keyctl_read_alloc: Operation not supported
```

**白話解析**：核心系統呼叫直接在驅動層拒絕讀取載荷請求。即使取得系統最高 root 權限，也無法直接透過標準 Keyring API 取回 `logon` 金鑰明文。然而作者亦特別警告：`logon` 金鑰在核心 RAM 中本質上仍是明文存放，若遭遇 DMA 攻擊或低層記憶體 dump，仍有外洩風險。

---

### 重點 8：Sealed Keys 密封金鑰：硬體信任根綁定（trusted 與 encrypted keys）

為了解決 RAM 中明文金鑰遭遇硬體側錄的問題，Linux 核心引進了 **Sealed Keys（密封金鑰）** 機制：

> **原文引用（p.31）**
> "By using the term sealed, we mean such keys that are created in the kernel, and their content is stored in an encrypted form using a trusted source. Furthermore, these keys are persistent across reboots, unlike the logon keys, because their encrypted form can be stored in the filesystem."

書中深入剖析兩種互補的 Sealed Key 類型：
1. **Trusted Key（受信任金鑰）**：
   - 仰賴硬體信任來源（Hardware Trusted Source），如 **TPM**（Trusted Platform Module）或 **TEE**（Trusted Execution Environment / OP-TEE）。
   - 核心直接命令硬體晶片在內部生成隨機數，並使用硬體根密鑰（Root of Trust）封裝為加密 Blob。
   - 使用者空間可透過 `keyctl pipe` 將 Blob 儲存到磁碟檔案；重新開機後，將 Blob 重新塞入核心，只有原裝的同一顆硬體晶片才能解密還原。
2. **Encrypted Key（已加密金鑰）**：
   - 金鑰本體在核心內生成對稱金鑰（如 AES-CBC），但使用「主金鑰」（Master Key，通常是一個 Trusted Key 或特定 User Key）進行加密保護。
   - 若編譯設定開啟 `CONFIG_USER_DECRYPTED_DATA=y`，允許開發者自訂初始明文資料並透過主金鑰密封。

---

### 重點 9：Vendor-Specific Keys：以 NXP CAAM Black Key 為例

在高效能嵌入式系統中，許多晶片廠商提供專屬的硬體安全密碼模組。作者以常見的 NXP i.MX 系列處理器與其 **CAAM（Cryptographic Acceleration and Assurance Module）** 為範例：

> **原文引用（p.41）**
> "Vendor-specific keys are keys that are generated and sealed/unsealed directly by a dedicated cryptographic hardware engine... in this case the key resides within the CAAM while its sealed form is available in the kernel or user space!"

**三種金鑰色彩與安全層次**：
- **Red Key（紅金鑰）**：在核心記憶體或暫存器中以明文存在的原始金鑰，風險最高。
- **Black Key（黑金鑰）**：原始紅金鑰在晶片內部直接被 CAAM 的專屬主金鑰（如 OTPMK，One-Time Programmable Master Key）加密包裹後的金鑰 Blob。
  - **ECB Black Key**：僅提供機密性保護。
  - **CCM Black Key**：提供機密性與完整性認證（附帶 MAC 標籤）。
- **Secure Memory（安全記憶體）**：CAAM 內部專屬的隔離 RAM 區段，一般 CPU 核心與作業系統皆無法透過記憶體定址存取。

> **原文引用（p.46–47）**
> "While a trusted key is stored in its unsealed form in the kernel, a CAAM key is stored in its unsealed form in the CAAM. When it is moved to the kernel, it is sealed... here it holds a sealed key, while in the case of trusted keys, key holds a key in plain text (even if user space cannot read it)."

**黑金鑰生成與工具鏈**：
使用 `caam-keygen` 工具生成黑金鑰：
```bash
# 從特定明文生成 ECB 黑金鑰 Blob
$ caam-keygen create caamkey ecb -t 0123456789abcdef... -h
```
CAAM 黑金鑰可直接被 Linux 的 `dm-crypt` 驅動調用。解密運算完全發生於硬體加速器內部，金鑰明文從頭到尾不曾出現在 CPU 快取或系統 DDR RAM 中！

---

### 重點 10：Linux 透明加密架構：Filesystem-Level vs. Block-Level

針對靜態資料防護（Data-at-Rest），Linux 提供了兩種截然不同的透明加密途徑：

| 評比維度 | 檔案系統級加密（Filesystem-Level，如 fscrypt） | 區塊級加密（Block-Level，如 dm-crypt） |
| :--- | :--- | :--- |
| **運作層級** | 虛擬檔案系統（VFS）與具體檔案系統內部（ext4, f2fs） | 區塊裝置與驅動層之間（Device Mapper） |
| **加密標的** | 個別檔案內容與檔名 | 整顆磁碟、分割區或虛擬 loop 裝置上的所有磁區 |
| **元資料保護** | 結構部分外洩（目錄結構、檔案大小、權限公開） | 完全保護（檔案系統結構、inode、大小、日誌全數加密） |
| **多使用者金鑰隔離**| 極佳（不同目錄可綁定不同使用者密鑰） | 困難（單一分割區通常共用單一主密鑰） |
| **系統開機根目錄適用性**| 不適合加密核心開機必需之 `/sbin/init`、系統庫 | 最適合（根分割區 full rootfs 保護的工業標準） |

---

### 重點 11：檔案系統級加密（fscrypt）的原生實作與金鑰隔離

`fscrypt` 是現代 Linux（ext4 / f2fs / ubifs）原生支援的目錄級加密框架。

> **原文引用（p.48）**
> "The filesystem-level encryption works at a higher level than block-level encryption; it encrypts individual files and directories rather than the entire block device."

**fscrypt 原理與工具操作**：
1. **初始化支援**：
   ```bash
   # 初始化系統全局設定與指定掛載點
   # fscrypt setup
   # tune2fs -O encrypt /dev/mmcblk0pX
   # fscrypt setup /data
   ```
2. **目錄加密與保護器（Protector）**：
   ```bash
   # 對特定目錄啟用加密，可綁定 Passphrase 或 PAM 登入認證
   # fscrypt encrypt /data/secret/
   ```
3. **上鎖與解鎖機制**：
   - 當未解鎖時，目錄內檔案名稱會被替換成無法辨識的隨機 base64 編碼，且嘗試讀取檔案會回傳 `Required key not available`。
   - 解鎖時：`# fscrypt unlock /data/secret/` 載入金鑰至 process/session keyring，應用程式即可透明讀寫。
4. **v1 vs v2 加密策略（Policies）**：
   - **v1 Policy**：直接使用金鑰描述符，目前已廢棄（deprecated）。
   - **v2 Policy**：強制使用 HKDF（HMAC-based Extract-and-Expand Key Derivation Function）從主金鑰推導出每檔案獨立的加密金鑰，安全性更高。

---

### 重點 12：區塊級加密（dm-crypt）與安全金鑰槽整合

區塊級加密是嵌入式安全開機用來保護根檔案系統（Root Filesystem）的骨幹技術。它將底層實體儲存（如 `/dev/mmcblk2p3`）包裝成一個虛擬的對應裝置（如 `/dev/mapper/root`）：

> **原文引用（p.63）**
> "The block-level encryption, instead, works at a lower level, that is, at the block device level, and it is used to get a transparent disk encryption on whole disks or single partitions."

**操作架構實作示範**：
1. **透過 Keyring 傳遞金鑰（避免密鑰落地）**：
   ```bash
   # 將 32 位元組隨機金鑰放入 session keyring 中的 logon 類型
   $ keyctl padd logon cypher: @s < /tmp/rootfs.key
   
   # 使用 dmsetup 或 cryptsetup 建立映射裝置
   # cryptsetup open --type plain --cipher aes-cbc-essiv:sha256        --key-file <(keyctl pipe 779318636) /dev/mmcblk2p3 rootfs_mapped
   ```
2. **格式化與透明掛載**：
   ```bash
   # 在解密裝置上建立一般 ext4 檔案系統
   $ mkfs.ext4 /dev/mapper/rootfs_mapped
   $ mount /dev/mapper/rootfs_mapped /mnt
   ```
所有寫入 `/mnt` 的資料在通過 Device Mapper 時皆會由核心密碼引擎進行即時透明加密，再寫入底層 eMMC 磁區；讀取時則在記憶體中即時解密。

---

### 本章 Summary（作者原話）

> **原文引用（p.75）**
> "In this chapter, we have introduced the foundational elements of Linux cryptography, beginning with the Linux Crypto API and the kernel keyring for secure key management.
> We have detailed the AF_ALG socket interface, which allows user-space applications to directly access the kernel's crypto layer, providing examples of its use with OpenSSL. The crypto-afalg tool was then introduced as a more secure solution that integrates the crypto API with Linux keyring management.
> Finally, we have presented the Linux Transparent Encryption (used to encrypt data for user-space processes transparently).
> Now that these concepts are clear, we can move to the next chapter to discover what Secure Boot is (and what it is NOT) in detail."

---

### 思考與實務檢驗

1. **為什麼即使系統有 root 權限，`logon` 金鑰依然無法被讀取？這真能完全抵禦攻擊嗎？**
   - 核心透過系統呼叫層級阻絕了 `keyctl_read` 對 `logon` 類型的請求；但在 RAM 中金鑰本體仍是明文。若攻擊者具備載入惡意核心模組（rootkit）的能力，或透過硬體 JTAG、Cold Boot Attack、DMA 介面（如 PCIe / Thunderbolt），依然能從記憶體傾印中撈出密鑰。因此，高安全場景必須採用硬體隔離的 Sealed Key 或 CAAM Black Key。
2. **在根檔案系統加密情境下，為何選擇 dm-crypt 而非 fscrypt？**
   - `fscrypt` 無法加密檔案系統本身的 metadata、目錄層級大小與系統二進制可執行檔結構；且開機初期 `/sbin/init` 與動態連結庫尚未載入時，無法直接依賴使用者空間工具來解鎖。`dm-crypt` 則能在 initramfs 階段統一將整個分區透明對應，對整個作業系統提供全盤保護。

---

### 我的整理：本章帶走的五句話

1. **數位簽章管真偽與防篡改，資料加密管保密與防窺探，兩者在安全架構中各司其職。**
2. **所有主流嵌入式處理器基於開機速度考量，在執行碼與檔案解密上一律採用對稱式加密（如 AES-256-CBC）。**
3. **千萬別在命令列或腳本參數中傳遞明文金鑰，應一律採用 Linux Keyring 與非置換記憶體進行安全委託。**
4. **Logon Key 讓 root 亦無法讀回明文載荷，而硬體綁定的 Trusted Key 與 CAAM Black Key 能進一步抵禦實體 RAM 傾印攻擊。**
5. **保護整個系統環境的根基在於區塊級加密（dm-crypt），它能將包括目錄、檔案大小與日誌在內的所有磁區全數黑箱化。**

---

### 延伸資源（p.14, p.56, p.61）

- crypto-afalg 開源工具：https://github.com/giometti/crypto-afalg
- Google fscrypt 專案與設定指南：https://github.com/google/fscrypt#setting-up-fscrypt-on-a-filesystem
- fscrypt_v1 示範原始碼：https://github.com/giometti/fscrypt_v1

---
## 第 2 章：What Secure Boot Is (and What It Is NOT)（什麼是安全開機，以及它「不是」什麼）

> 對應原書 p.77–108

### 這章在幹嘛

本章破除許多開發者對「安全開機」的過度神化與誤解，建立嚴格的威脅模型與架構觀念。作者首先定義安全開機的職責範疇——它不是防毒軟體，也不保證開機後的應用程式漏洞不會被利用，它的核心任務是「**將軟體與硬體鎖定在一起，建立不可破壞的信任鏈（Chain-of-Trust）**」。

整章骨架由淺入深展開：
1. **安全開機的定義與防禦邊界**：明確區分「開機時保護」與「開機後狀態」。
2. **信任鏈架構與 ARM 啟動序列**：從晶片 ROM Code、SPL、TF-A、OP-TEE、U-Boot 到 fitImage 的逐級驗證。
3. **進入 Rootfs 的最後一哩路**：為何 Rootfs 無法被傳統簽署？initramfs 與 `switch_root` 的運作細節。
4. **工廠量產安全架構**：自有工廠（In-House）與第三方代工廠（Third-Party）的密鑰防護，以及 Diffie-Hellman 金鑰交換。
5. **安全系統更新（System-Update）流程**：更新包加密、數位簽章與金鑰隔離。

---

### 重點 1：安全開機的定義與邊界（What It Is NOT）

作者在開頭強烈提醒開發者必須建立正確的安全認知：

> **原文引用（p.77）**
> "The Secure Boot is a set of rules and mechanisms designed to ensure that only trusted and authenticated software can run during the boot process of a computer system... The Secure Boot aims to solve this by creating a Chain-of-Trust from the moment the computer powers on by locking the software with the hardware in such a way that only legitimate software can be executed on legitimate hardware and vice versa."

作者列舉 Secure Boot **不是**什麼（What It Is NOT）：
- **它不是防毒軟體（NOT an anti-virus）**：它不掃描記憶體中的病毒或惡意程式碼。
- **它不修復應用程式漏洞**：如果使用者空間有可被遠端注入的漏洞（例如未授權遠端 code execution），Secure Boot 無法阻止攻擊者利用。
- **它不防範系統運行後的 root 特權濫用**：

> **原文引用（p.78）**
> "Moreover, we should consider that once the encrypted rootfs is mounted, from the process perspective it is a normal rootfs (with no encryption), so the rootfs encryption is an effective protection when the machine is turned off."

**要記住的觀點**：安全開機的職責是在系統啟動時驗證程式碼從未遭到置換或篡改，並在機器斷電時確保資料被嚴密加密。一旦系統成功開機且掛載了 rootfs，內部程序看到的就是一般明文，開機後的保護必須依賴 Linux 權限控制、沙盒與存取控制機制。

---

### 重點 2：信任鏈（Chain-of-Trust）的建立原則與逐級驗證

信任鏈的核心邏輯非常簡單而嚴格：**每個開機階段在執行下一個階段之前，必須先對其進行密碼學完整性驗證（簽章比對）與解密。**

```
[Hardware Root-of-Trust (eFuses / ROM Code)]
                  │ 驗證簽章
                  ▼
         [Pre-Bootloader (SPL)]
                  │ 驗證簽章
                  ▼
    [Secure Monitor / TEE (TF-A / OP-TEE)]
                  │ 驗證簽章
                  ▼
         [Bootloader (U-Boot)]
                  │ 驗證 FIT 簽章
                  ▼
     [Linux Kernel (fitImage: Kernel + DTB + Initramfs)]
                  │ 解密封金鑰 (CAAM / TPM / Keyring)
                  ▼
      [Encrypted Root Filesystem (dm-crypt / rootfs)]
```

> **原文引用（p.78–79）**
> "All components of the boot sequence (the bootloaders and their companion software), the kernel, and the rootfs must be digitally protected. Usually, this is the ROM code fused into the chip... [it] begins by validating the first stage bootloader against a public key burned into one-time programmable (OTP) memory (like FUSEs—or non-volatile memory in general—in the chip)."

只要整個鏈條中的任何一個環節簽章驗證失敗，開機程序立即中斷，硬體進入當機（hang）或安全鎖死狀態，絕不允許載入不受信任的程式碼。

---

### 重點 3：根檔案系統不可簽章悖論與加密替代方案

在傳統開機鏈條中，SPL、U-Boot、Linux 核心都是靜態且唯讀的二進制映像檔，因此可以使用非對稱私鑰簽名，並在啟動時以公鑰驗證。然而，根檔案系統（Rootfs）面臨一個經典工程難題：

> **原文引用（p.80）**
> "As the last step, the kernel, by using an encryption key, mounts the real rootfs. Readers should notice that in this last step the security is given by the encryption key only, since it’s not possible to have a signed rootfs!"

**白話解析**：
一般 Linux 運作時，根檔案系統若為可讀寫分區（Read-Write Filesystem），檔案在運行期間會不斷產生、修改（日誌、組態檔、暫存資料）。如果對整個分區進行單一數位簽章，任何檔案改動都會導致簽章破壞；因此：
1. **不能對讀寫 Rootfs 直接進行整體數位簽章驗證**。
2. 唯有**透明加密（Encryption Key）**能解決這個問題：只有持有正確硬體解密金鑰的合法核心，才能打開並掛載該根分區。
3. 如果根檔案系統是唯讀的（Read-Only），書中提到可透過 dm-verity 或將其壓縮為靜態包來驗證雜湊樹，但在通用讀寫架構中，根目錄的安全基石是**加密保護**。

---

### 重點 4：ARM 系統開機階段全景（ROM Code -> SPL -> TF-A -> OP-TEE -> U-Boot）

在現代 ARM Cortex-A 嵌入式架構中，信任鏈被細分為多個安全層級（Exception Levels）：

1. **ROM Code（BL1）**：燒錄於 SoC 內部唯讀記憶體，開機通電執行的第一個程式碼。負責讀取晶片內部的 eFuse，比對第一階段開機程式（SPL）的數位簽章。
2. **SPL（Secondary Program Loader / BL2）**：負責初始化外部 DDR RAM，隨後將 TF-A、OP-TEE 與 U-Boot 載入記憶體。
3. **TF-A（ARM Trusted Firmware / BL31）**：執行於最高特權等級 **EL3（Secure Monitor）**，負責處理安全世界（Secure World）與非安全世界（Non-secure World）之間的上下文切換。
4. **OP-TEE（BL32）**：執行於 **S-EL1（Secure World OS）** 的開源安全執行環境：
   > **原文引用（p.83）**
   > "The OP-TEE (for ARM Cortex-A) is primarily designed to run as a companion to a non-secure Linux kernel utilizing ARM TrustZone technology. OP-TEE’s main purpose is to enable secure processing for critical applications and data on devices. It allows developers to build Trusted Applications (TAs) that run exclusively within the Secure World, protecting them from malware or vulnerabilities in the Normal World OS."
5. **U-Boot（BL33）**：執行於非安全世界的 **EL2/EL1**，負責提供開機選單、網路支援與載入 Linux 核心。

---

### 重點 5：核心與 fitImage 整合簽署機制

在傳統嵌入式開機中，U-Boot 分別載入 `zImage`、`dtb` 與 `initrd`，這會產生漏洞：若攻擊者只掉包 Device Tree（DTB），便能惡意竄改周邊定址或將硬體設為不安全狀態。

為了解決這個問題，業界與本書採用 **FIT Image（Flattened Image Tree）** 格式：
- 將 Kernel Image、Device Tree Blob（DTB）以及 Initramfs 全部包裝進單一 `fitimage` 檔案中。
- FIT 支援密碼學簽章節點：在打包時使用 OEM 私鑰對各節點進行雜湊運算與簽署。
- 驗證用公鑰直接嵌入在 U-Boot 本身的 Control DTB 內部。
- 開機時，U-Boot 逐項驗證 fitImage 內的核心與裝置樹簽章，確認完全無誤後才允許跳轉執行。

---

### 重點 6：進入根檔案系統的樞紐：initramfs 與 switch_root 的真實角色

當核心開機並印出 `Starting kernel ...` 之後，真正的挑戰才剛開始：核心如何安全地打開加密的 rootfs？

> **原文引用（p.90–91）**
> "The rootfs is normally located on a mass storage, and the kernel can mount it directly, but in most cases, embedded devices prefer to use an intermediate step before effectively mounting this final rootfs... Once the real rootfs has been retrieved and then mounted into the real_rootfs directory, we can use the switch_root command to start the final operating system:
> `exec switch_root /real_rootfs /sbin/init`"

**initramfs 在安全架構中的三大任務**：
1. **載入驅動與模組**：掛載密碼模組、硬體加密晶片驅動（如 CAAM 驅動、TPM 驅動）。
2. **安全取回金鑰並打開磁碟**：從硬體 Keyring 或加密分區解封 DEK（資料加密金鑰），並調用 `cryptsetup` 或 `dmsetup` 建立 `/dev/mapper/root`。
3. **安全接管控制權**：執行檔案系統檢查後，將 PID 1 移交給真正的系統初始化程序。

---

### 重點 7：chroot 與 switch_root 的本質差異與記憶體回收

作者特別以方塊提示提醒開發者，千萬不能把 `switch_root` 當成單純的 `chroot`：

> **原文引用（p.92）**
> "Readers should not confuse the switch_root command with chroot(8) command; in fact, the latter just changes the root directory for the current running process and its children, while switch_root removes recursively all files and directories on the current rootfs... freeing all initramfs memory!"

**差異對照表**：
| 功能特性 | `chroot` | `switch_root` |
| :--- | :--- | :--- |
| **運作原理** | 僅改變呼叫程序及其子程序的根目錄路徑 | 移動掛載點並將目標目錄設定為全系統新根目錄 |
| **PID 1 行為** | 無法替換核心 PID 1，原 init 程序仍在背景執行 | 透過 `exec` 讓新系統的 `/sbin/init` 成為真正的 PID 1 |
| **記憶體回收** | 原 initramfs 的 RAM disk 檔案完全滯留在記憶體中 | **遞迴刪除 initramfs 內所有檔案，徹底釋放 RAM 空間** |
| **安全意涵** | initramfs 內部的金鑰解密腳本或暫存密碼仍可能殘留在 RAM | 確保用於解鎖開機的暫存輔助檔案完全被銷毀 |

---

### 重點 8：廠端生產模式 1：自有信任廠（In-House Environment）的 eFuse 燒錄

如何將第一把安全金鑰安裝到全新的空白硬體上？如果是在設備商**自有的工廠內部生產**：

> **原文引用（p.97）**
> "If the factory production stage is done in-house, we can use a simplified protocol since we can trust the factory environment... the CPU fuses are programmed directly via a programming tool."

**流程要點**：
1. 產線網路與燒錄機台被視為安全邊界內（Trusted Network）。
2. 工程人員使用原廠燒錄工具（如 JTAG 或 USB 製造工具），直接將公鑰雜湊（SRK Hash）與對稱加密主密鑰寫入晶片 eFuse。
3. 燒錄完成後，立即將晶片的「安全開機啟用 FUSE」（如 Security Configuration Fuse）燒斷（Blow Fuse），將晶片永久鎖定為 Closed/Secure 狀態。

---

### 重點 9：廠端生產模式 2：第三方代工廠（Third-Party Environment）與 Diffie-Hellman 金鑰協議

若生產製造委託給**外部合約代工廠（Third-Party EMS）**，代工廠的產線環境必須被視為「不可信任（Untrusted）」。若直接寄送明文金鑰供代工廠燒錄，將面臨金鑰外洩、未授權超額生產（Overproduction/Cloning）或仿冒硬體風險。

> **原文引用（p.98–99）**
> "A possible protocol can be the Diffie–Hellman exchange protocol, which allows two parties, who have no prior knowledge of each other, to establish a shared secret key over an otherwise insecure communication channel... to ensure the initial software is properly verified, we must provide a key-agreement protocol within the bootloader so it can exchange data in a secure form with a remote server that manages the factory production."

**安全協議架構流程（ECDH / Diffie-Hellman）**：
1. **板端生成暫時金鑰對**：開機時，晶片 ROM Code 或生產用 bootloader 在硬體內部生成一組臨時非對稱金鑰對（Ephemeral Key Pair）。
2. **公鑰交換**：代工產線的板子將自己的暫時公鑰傳送給委託方遠端受控的 HSM（硬體安全模組）伺服器；伺服器亦回傳其公鑰。
3. **推導共享密鑰**：雙方在互聯網上不傳遞任何私鑰的情況下，各自算出相同的共享對稱金鑰（Shared Secret）。
4. **加密燒錄 Payload**：遠端伺服器將要寫入 eFuse 的設備專屬認證密鑰加密後下發；板端硬體晶片解密後直接燒入 eFuse。代工廠作業員在整個過程中完全接觸不到明文密鑰！

---

### 重點 10：初始根檔案系統的交付打包（rootfs.tar.zip.enc + signature）

全新板子首次通電時，eMMC 是全空的。工廠階段如何安全寫入作業系統？

> **原文引用（p.104）**
> "In the factory production stage, the mass storage is initialized with... A rootfs image as a TAR archive, both signed and encrypted, which holds the rootfs files to be installed into the future encrypted rootfs partition. In the figure, this compressed and signed image archive has been named rootfs.tar.zip.enc, while with the rootfs.signature we have addressed the relative digital signature."

**初始 Rootfs 佈署四部曲**：
1. **打包與雙重防護**：將根目錄檔案打包壓縮為 `tar.gz`，先以 OEM 私鑰簽名產生 `rootfs.signature`，再使用工廠資料加密金鑰（DEK）加密為 `rootfs.tar.zip.enc`。
2. **驗證簽章**：由核心內部的 factory initramfs 使用內嵌公鑰檢查 `rootfs.signature`。
3. **解密與格式化**：驗證通過後，核心生成一組全新的隨機金鑰（Device Encryption Key），以此金鑰建立全新的 `dm-crypt` 分區並格式化為 ext4。
4. **解壓縮落盤**：將解密後的 rootfs 檔案就地解壓縮到新建的加密分區中，完成首開機系統初始化。

---

### 重點 11：系統更新（System-Update）的信任延續與金鑰隔離

當設備部署在現場後，如何安全進行空中升級（OTA / System Update）？

> **原文引用（p.106）**
> "To update the rootfs, we must get access to the encryption key used to encrypt the rootfs TAR archive and (in some circumstances) also the encryption key used to encrypt the running rootfs where these files must be installed. Getting access to the latter key, that is, the key used to encrypt the running rootfs, is not an issue since it is in a sealed form within the /boot directory; a little more delicate is the issue about how to access the rootfs TAR archive encryption key in a way that the key is not exposed in the filesystem."

**更新包核心防護原則**：
1. 更新套件本體必須在發布伺服器端完成數位簽署與加密。
2. 正在運行的系統從網路下載更新包後，不能直接覆蓋當前分區。
3. 更新解密金鑰必須妥善保護於密封儲存或透過安全通道取得，絕不可在一般檔案系統中以未保護明文常駐。
4. 升級驗證與寫入必須在獨立的輔助環境（Rescue 模式或 A/B 備用分區）完成。

---

### 本章 Summary（作者原話）

> **原文引用（p.107）**
> "In this chapter, we have seen what Secure Boot is by introducing the concept of Chain-of-Trust and explaining the boot sequence in detail.
> We detailed the Linux boot sequence, focusing on ARM-based systems and the roles of components like the SPL, U-Boot, ARM Trusted Firmware (TF-A) acting as a secure monitor, and the Open Portable Trusted Execution Environment (OP-TEE) for Secure World isolation. We have also introduced what we should keep in mind when we wish to produce a secured system (in-house or in a third-party factory) or when we distribute a system update in order to prevent possible corruption of the Chain-of-Trust or the system’s secrets exploitation. Now we are ready to move to the next chapter, where we are going to present how a real implementation works to better understand how we can implement all Secure Boot concepts within our systems."

---

### 思考與實務檢驗

1. **為什麼即使安全開機全鏈路驗證成功，系統依然可能被植入後門？**
   - 因為 Secure Boot 只驗證開機時所載入二進制檔案的數位簽名是否來自合法原廠。一旦進入 Linux 使用者空間，若應用程式（如 Web 伺服器、SSH）包含已知漏洞或弱密碼，攻擊者依然可透過網路入侵並獲取 root 權限。Secure Boot 的防線在於：攻擊者即使拿到 root，也無法把惡意程式寫進 Bootloader/Kernel 並在下次重開機時自動執行（因為簽章不符無法開機）。
2. **在代工廠環境中，為什麼不能直接用隨身碟把已加密的整顆磁碟 Raw Image 複製過去？**
   - 如果每台設備使用相同的靜態加密金鑰，一旦其中一台機器的金鑰被逆向破解，全球所有相同型號的產品安全防護將集體潰堤（Class Break）。因此真正的安全架構要求每台硬體具備「獨立且隨機的裝置加密金鑰（Unique Per-Device Key）」，這必須在工廠通電時透過晶片硬體即時產生。

---

### 我的整理：本章帶走的五句話

1. **安全開機的本質是軟硬體信任鏈，它杜絕非法程式碼在開機時被載入，但它不是執行期的防毒軟體。**
2. **讀寫根檔案系統無法以整體數位簽章保護，區塊級透明加密是維護 rootfs 靜態機密與完整性的唯一通用解。**
3. **ARM 信任鏈是由 ROM Code、SPL、TF-A、OP-TEE、U-Boot 到 fitImage 構成的嚴格逐級簽章防線。**
4. **進入真實 rootfs 必須使用 `switch_root` 而非 `chroot`，才能遞迴銷毀 initramfs 記憶體、避免敏感開機殘留洩漏。**
5. **在第三方代工廠生產設備時，應利用 Diffie-Hellman 等金鑰協議在晶片與遠端伺服器間安全推導金鑰，絕不可明文下發密鑰。**

---
## 第 3 章：Real Implementations（實務開機架構：Rescue 與 A/B 分區實作）

> 對應原書 p.109–187

### 這章在幹嘛

這是全書最關鍵、篇幅最長也最具實戰價值的工程章。在工業控制、醫療設備、車載系統與關鍵基礎設施中，設備往往部署於偏遠或無人值守環境，系統必須具備「**在無人介入下保證可靠還原與安全升級**」的能力。

作者在此章將理論化為具體的磁碟分割區設計、U-Boot 腳本、initramfs 流程與系統更新腳本，並詳細實作兩大主流架構：
1. **救援模式架構（Rescue Schema）**：以單一加密根分區搭配獨立 Factory 救援分區，著重於磁碟空間受限下的災難復原與重建。
2. **A/B 備份分區架構（A/B Schema）**：以雙加密根分區（`root_a` / `root_b`）實現不停機背景更新、原子級切換（Atomic Switch）與開機失敗自動回滾（Automatic Rollback）。

---

### 重點 1：嵌入式大容量儲存（eMMC）結構與 RPMB 安全區

在深入分區架構前，作者先介紹了現代嵌入式 Linux 最常用的儲存介質——**eMMC（embedded MultiMediaCard）**：

> **原文引用（p.109）**
> "The term eMMC stands for embedded MultiMediaCard. Essentially, it is a type of NAND flash memory used for storage in portable devices and embedded systems. The difference between raw NAND flash is that it also integrates a controller and an interface into a single package, making it a compact and efficient storage solution that, from the point-of-view of the final user, it is just like a hard disk... As a last note, the device mmcblk2rpmb is the Replay-Protected Memory Block (RPMB), which is another secure area for storing sensitive data, such as encryption keys, securely via encryption."

**eMMC 的三大關鍵區域**：
- **User Data Area（`/dev/mmcblkX`）**：存放作業系統、Boot、Factory、Rootfs 與使用者資料的一般區塊儲存區。
- **Boot Partitions（`mmcblkXboot0` / `mmcblkXboot1`）**：晶片上電時硬體直接讀取的唯讀啟動分區，通常存放極早期的 ROM Code / SPL 與 TF-A。
- **RPMB（Replay-Protected Memory Block，`/dev/mmcblkXrpmb`）**：具備「防重播攻擊（Anti-Replay）」特性的硬體加密儲存區。只有持有晶片製造時燒入之共享對稱密鑰的程式（如 OP-TEE）才能讀寫，常作為最高安全等級金鑰的存放庫。

---

### 重點 2：救援模式（Rescue Schema）的磁碟分區拓撲

Rescue 架構採用經濟實惠且經典的四分區配置：

```
+-----------------------------------------------------------------------------------+
|  /dev/mmcblk2p1  |  /dev/mmcblk2p2  |      /dev/mmcblk2p3      |  /dev/mmcblk2p4  |
|      /boot       |     /factory     |      /root (加密)        |      /data       |
|      128 MB      |      512 MB      |          動態            |       剩餘       |
|    (未加密ext4)  |    (未加密ext4)  |      (dm-crypt)          |   (未加密ext4)   |
|   fitimage + key |   復原映像+DEK   |       真實系統           |     使用者資料   |
+-----------------------------------------------------------------------------------+
```

> **原文引用（p.110–111）**
> "In the above list, the only encrypted partition is /root while other partitions are in plain text format. This is not a problem at all because in the boot and factory partitions all kernel and rootfs images are encrypted, so we can consider them securely protected too."

**各分區關鍵職責**：
1. **`/boot`（p1，~128MB，ext4）**：存放開機核心 `fitimage`（含 Kernel、DTB、Normal initramfs）以及密封的金鑰檔案 `rootfs.key.bb`（CAAM Black Key 或 TPM Blob）。
2. **`/factory`（p2，~512MB，ext4）**：工廠復原分區。存放工廠初始救援用的 `fitimage`、工廠資料加密金鑰 `dek` 以及備份的根目錄壓縮包。
3. **`/root`（p3，dm-crypt 加密區塊）**：唯有此分區被 `dm-crypt` 全盤透明加密，掛載為真實作業系統根目錄。
4. **`/data`（p4，ext4）**：存放持久化使用者資料、暫存的 OTA 升級包等。

---

### 重點 3：Rescue Schema 的正常開機流程（Normal Boot）

在正常開機下，U-Boot 讀取環境變數 `bootmode=normal` 與 `schema=rescue`：

> **原文引用（p.116–117）**
> "Schema: Rescue... Then it loads the fitimage file from it and then executes the kernel with:
> `device=/dev/mmcblk2 root=/dev/mmcblk2p3 rootwait rw initramfs_normal boot_schema=rescue root_name=root`"

**開機各階段具體動作**：
1. **U-Boot 載入**：從 `/boot` 分區讀取 `fitimage`，驗證 FIT 簽章無誤後解開各節點並跳轉。
2. **Normal initramfs 接手**：
   - 讀取 bootargs 中的分區參數 `device=/dev/mmcblk2` 與 `root_name=root`。
   - 掛載 `/boot` 分區，讀取 `rootfs.key.bb` 密封金鑰。
   - 將金鑰送入 CAAM 或硬體 Keyring 解密出執行期金鑰。
   - 執行 `cryptsetup` 將底層分區 `/dev/mmcblk2p3` 映射為 `/dev/mapper/root`。
   - 執行 `e2fsck` 檢查檔案系統健康度。
   - 掛載 `/dev/mapper/root` 至 `/sysroot`。
   - 執行 `exec switch_root -c /dev/console /sysroot /sbin/init` 轉交系統控制權。

---

### 重點 4：Rescue Schema 的原廠重置流程（Factory-Reset）

當系統發生嚴重損壞、或使用者按下硬體復原按鈕時，U-Boot 被設定為 `bootmode=factory-reset`：

> **原文引用（p.128, p.133）**
> "In order to do a factory-reset, we must instruct U-Boot to boot the system using the factory partition instead of the boot partition...
> 7) Create the encrypted root filesystem and then repopulate it"

**Factory-Reset 實作八步驟**：
1. **引導至救援核心**：U-Boot 從 `/factory` 分區載入專屬的 factory `fitimage`，核心開機參數傳入 `initramfs_factory`。
2. **掛載工廠分區**：掛載 `/dev/mmcblk2p2` 讀取 `dek`（資料加密金鑰）與分區表設定檔 `storage_info`。
3. **動態磁碟重構**：調用 `sgdisk` 重新劃分 `/root` 與 `/data` 分區大小，抹除原有受損的根目錄。
4. **生成全新裝置密鑰**：透過硬體隨機數生成器（RNG）生成全新的隨機 256 位元根目錄金鑰，並由 CAAM/TPM 封裝為全新的 `rootfs.key.bb`。
5. **建立全新加密分區**：使用該隨機金鑰執行 `cryptsetup -q luksFormat` 或 `plain` 映射 `/dev/mapper/root`。
6. **還原初始系統檔案**：使用 `/factory/dek` 解密 `/factory/rootfs.tar.zip.enc`，解壓縮寫入 `/dev/mapper/root`。
7. **更新 `/boot` 分區**：將工廠核心映像檔複製回 `/boot/fitimage`，並寫入全新的 `rootfs.key.bb`。
8. **重啟**：將 `bootmode` 重置為 `normal`，下次開機即可進入宛如出廠時的全新系統。

---

### 重點 5：Rescue Schema 的系統更新流程（System-Update）

在 Rescue 架構下，由於只有一個 root 分區，系統無法在開機中直接覆蓋正在運行的根檔案系統，必須重開機進入更新模式：

> **原文引用（p.141–142）**
> "When we ask to do a system-update by setting bootmode=update, U-Boot switch to partitions #0, OK... loads the fitimage file from it, and then executes the kernel in it:
> `initramfs_update device=/dev/mmcblk2 boot_schema=rescue root_name=root`... Now bootname points to the data partition, and the image file is set to update/fitimage; this means that U-Boot now expects to find a valid fitimage file under the directory /data/update"

**更新套件（Bundle）的構成**：
- `fitimage`：即將更新的新版核心（通常尾端附加了 `dek` 金鑰）。
- `rootfs.tar.zip.enc`：經加密與數位簽署的全新根目錄壓縮檔。
- `rootfs.signature`：對更新包的數位簽名。
- `bundle-update.sh`：驅動更新流程的控制腳本。

**更新流程關鍵安全查驗**：
1. 進入 `initramfs_update` 後，首先以公鑰檢查 `rootfs.signature`。
2. 簽名驗證通過後，重新格式化 `/boot` 並寫入新版 `fitimage`。
3. 重新建立 `/dev/mapper/root`，解密並釋放新版根系統檔案。
4. 若中途發生斷電或校驗失敗，系統可退回 Factory-Reset 分區進行復原，保證不變磚（Anti-Bricking）。

---

### 重點 6：A/B 備份分區架構（A/B Schema）與不停機切換優勢

雖然 Rescue 架構能可靠復原，但它的致命痛點在於：**更新過程系統必須離線停機（Downtime），且一旦升級失敗只能回退到最原始的出廠狀態（失去所有現有軟體配置）**。

為克服此限制，工業與現代嵌入式廣泛採用 **A/B 分區架構（A/B Schema）**：

> **原文引用（p.159）**
> "The A/B schema is a way to boot the system from two partitions alternating between them at each update... In fact, as already seen, in the rescue schema, to update the system, the init program will find the following partition scenario within the mass storage:
> • One boot partition
> • One factory partition
> • root_a and root_b partitions"

**A/B 分區拓撲**：
```
+-----------------------------------------------------------------------------------------------------+
|  /dev/mmcblk2p1  |  /dev/mmcblk2p2  |   /dev/mmcblk2p3   |   /dev/mmcblk2p4   |  /dev/mmcblk2p5 |
|      /boot       |     /factory     |    /root_a (加密)  |    /root_b (加密)  |      /data      |
|      128 MB      |      512 MB      |      動態分配      |      動態分配      |       剩餘      |
|    fitimage + keys                  |     Slot A 系統    |     Slot B 系統    |    持久化使用者 |
+-----------------------------------------------------------------------------------------------------+
```

---

### 重點 7：A/B 選擇器（Selector）與 U-Boot 狀態機控制

A/B 系統的核心控制變數是 U-Boot 環境變數中的 `ab_selector`（數值為 `a` 或 `b`）：

> **原文引用（p.164–165）**
> "Schema: A/B [selector=b]... However, now we are using the A/B schema, so variables image and rootname are redefined as shown below...
> `if test ${bootmode} = normal ; then`"

**U-Boot 狀態控制邏輯**：
```bash
# 根據 selector 決定要掛載哪一個分區與核心
if test ${ab_selector} = a ; then
    setenv rootname root_a
    setenv image fitimage_a
else
    setenv rootname root_b
    setenv image fitimage_b
fi
```
開機參數自動注入 `root_name=${rootname}`。initramfs 根據此名稱自動解封對應的 `rootfs_a.key.bb` 或 `rootfs_b.key.bb`，映射 `/dev/mapper/root_a` 或 `/dev/mapper/root_b`。

---

### 重點 8：A/B 架構下的背景原子級線上更新（Online In-Place Update）

A/B 架構最優雅之處在於**更新作業完全在背景運行（In-Place Background Updating）**，系統完全不需要重新開機進入特殊模式：

> **原文引用（p.180, p.184–185）**
> "This is logical since the A/B schema is adopted when our system must be able to recover autonomously from a failed update...
> `bundle-update.sh: A/B selector is now set to "a". So, just reboot to enter in the new rootfs!`"

**升級全過程時序圖**：
1. **假設當前在 Slot B 運行**：
   - 系統正掛載 `/dev/mapper/root_b` 為 `/`。
   - 備用的 Slot A（`/dev/mmcblk2p3`）處於完全閒置或卸載狀態。
2. **下載更新套件**：守護程序自網路下載新版系統套件至 `/data` 或暫存目錄。
3. **驗證與就地寫入**：
   - 驗證更新套件的數位簽名。
   - 使用隨機金鑰將閒置的 Slot A 格式化為全新的 `dm-crypt` 裝置。
   - 直接將新系統解壓縮寫入 Slot A，並把新版核心寫入 `/boot/fitimage_a`。
4. **設定檔同步（Sync Configurations）**：
   - 調用備份同步腳本（如 `bundle-backup.sh copy`），將目前 Slot B 中的網路設定、SSH 金鑰、機器憑證選擇性複製到 Slot A。
5. **原子級翻轉選擇器（Atomic Flip）**：
   - 一切就緒後，呼叫 `fw_setenv ab_selector a`。
   - 此操作僅改變微小的 bootloader 變數，瞬間完成升級切換。
6. **重開機生效**：設備執行重新開機，無縫跳入 Slot A。

---

### 重點 9：開機看門狗與自動回滾機制（Watchdog & Auto-Rollback）

如果在切換到 Slot A 後，新軟體存在嚴重 Bug 導致核心恐慌（Kernel Panic）或無法連網，系統如何自救？

**書中設計的防護邏輯**：
1. **開機計數器（Boot Counter / Try Counter）**：
   - 在切換 selector 時，同時設定 `bootcount=0` 與 `bootlimit=3`。
   - 每次 U-Boot 啟動時自動累加計數器。
2. **成功標記（Confirmation）**：
   - 只有當新系統順利進入使用者空間、各關鍵守護服務（systemd services）皆健康運行時，使用者空間守護程式才會呼叫指令將開機計數器清零，確認升級成功。
3. **自動復原回滾（Automatic Rollback）**：
   - 若重啟超過 3 次皆未獲得使用者空間的健康確認，硬體看門狗觸發重置。
   - U-Boot 偵測到 `bootcount > bootlimit`，自動將 `ab_selector` 翻轉回原先正常運作的 Slot B！
   - 設備自動從已知的完好系統啟動，徹底避免因更新失敗而派工維修。

---

### 重點 10：Rescue Schema vs. A/B Schema 架構對照表

| 評估面向 | 救援模式架構（Rescue Schema） | 備份分區架構（A/B Schema） |
| :--- | :--- | :--- |
| **磁碟空間佔用** | 較小（僅需單一 rootfs 加上壓縮的 factory 映像） | 較大（需兩份完整容量的獨立 rootfs 分區） |
| **升級體驗** | 必須重開機進入專門的 update 模式，業務中斷時間長 | 背景在線更新，僅需一次常規重開機時間（數秒內） |
| **升級失敗處置** | 只能重置回初始出廠狀態（Factory Reset），遺失所有更新記錄 | 自動回滾至更新前的運作版本，業務無縫接軌 |
| **金鑰管理複雜度** | 較低（單一 `rootfs.key.bb`） | 較高（需分別維護 `rootfs_a.key.bb` 與 `rootfs_b.key.bb`） |
| **適用設備定位** | 儲存容量嚴格受限、低成本、容許停機的 IoT 終端 | 關鍵任務、工業自動化、車載、電網高可用性節點 |

---

### 本章 Summary（作者原話）

> **原文引用（p.187）**
> "This chapter explored the practical implementation of reliable and secure Linux-based operating systems for diverse embedded and industrial devices. A central focus was placed on ensuring reliable system recovery and updates without specialized user intervention, which requires careful organization of system components on the mass storage device. We provided a deep dive into the mass storage partitioning and the boot process logic (specifically the rescue schema), which are foundational for building robust, secure, and easily maintainable embedded Linux systems."

---

### 思考與實務檢驗

1. **在 A/B 架構中，為什麼每當更新 Slot B 時，要重新產生新的隨機金鑰，而不是沿用原先的金鑰？**
   - 密碼學中的「金鑰退役（Key Retirement）」原則：若每次升級皆重新生成密鑰，即便舊分區的某份金鑰在日後遭到物理側信道分析破解，攻擊者也僅能解密過去已被覆蓋的歷史殘存資料，無法讀取新版本的任何內容，達成前向安全性（Forward Secrecy）。
2. **為什麼存放核心與密封金鑰的 `/boot` 分區本身不需要加密？**
   - `/boot` 內存放的 `fitimage` 已經過 OEM 私鑰的數位簽署（任何修改皆會被 U-Boot 拒絕），且其中的金鑰檔案（`rootfs.key.bb`）本體已是經由晶片 CAAM 或 TPM 密封的高強度密文（Black Key Blob）。攻擊者在 `/boot` 既改不了核心代碼，也看不懂密封金鑰明文，因此該分區保持 ext4 明文能大幅簡化 Bootloader 驅動複雜度並提高開機可靠性。

---

### 我的整理：本章帶走的五句話

1. **高可用性嵌入式安全設備的終極考驗，在於無人介入下的災難復原與空中升級能力。**
2. **eMMC 不只是一般快閃記憶體，它的 RPMB 區域提供硬體級防重播與安全金鑰存儲。**
3. **Rescue 模式透過 Factory 救援分區達成災難重置，適合極度節省空間的低階場景。**
4. **A/B 模式以雙加密分區實現不停機背景更新，搭配開機計數器與看門狗能做到完全自動回滾。**
5. **在 A/B 升級中，更新套件就地解密寫入閒置分區、設定檔選擇性同步、最後原子級翻轉 selector，是工業級最佳實務。**

---
## 第 4 章：Final Considerations（最終考量：金鑰燒錄生成與 GPLv3 合規）

> 對應原書 p.188–201

### 這章在幹嘛

在完成開機與升級架構的設計後，本章處理工程落地時不可迴避的兩大實體問題：
1. **硬體廠商的真實金鑰生成與燒錄差異**：以意法半導體（ST）的 STM32MP1 系列與恩智浦（NXP）的 i.MX 系列為例，對比「全熔絲依賴模式」與「混合黑金鑰模式」。
2. **開源授權與法律合規（Licensing Issues）**：如何在採用 GPLv3 軟體（如 U-Boot、Linux 核心組件）建立安全開機裝置的同時，徹底避免「Tivoization（TiVo 化）」的法律訴訟，兼顧合規與商業智財保護。

---

### 重點 1：全熔絲依賴模式（The Fuse-Centric Approach）：以 STM32MP1x 為例

在 STMicroelectronics 的 STM32MP1 系列微處理器架構中，所有核心密碼學機密皆直接依賴晶片內部的硬體電氣熔絲（eFuses）：

> **原文引用（p.189）**
> "This approach is based on the fact that all cryptographic secrets are held in protected FUSEs; that is, both the encryption key and the signing public key (or its hash, to save space) are securely fused into the chip... Protected FUSEs are designed to enhance security by restricting direct access from the normal world (for instance, the kernel and its applications). Only the secured world (like ATF and OPTEE) can read these FUSEs."

**運作與開機引導機制**：
1. **公鑰雜湊比對**：STM32MP1 將公鑰打包在啟動映像檔中，ROM Code 讀取映像後計算公鑰雜湊，並與 eFuse 中的 `PKHTH`（Public Key Hash Table Hash）比對。
2. **映像解密**：若雜湊驗證吻合且簽章無誤，ROM Code 讀取燒錄於晶片內的 `EDMK`（Encryption Device Master Key），將加密的二進制程式碼即時解密至內部記憶體執行。

---

### 重點 2：STM32MP1x 工具鏈與 U-Boot 燒錄指令

ST 提供了專屬的命令列工具 `STM32_KeyGen_CLI` 用於離線金鑰生成：

```bash
# 生成簽章金鑰對（支援多達 8 組金鑰對以利輪換）
$ STM32_KeyGen_CLI -abs stm32mp13-key/ -n 8 -pwd azerty

# 生成 16 位元組（128-bit）對稱加密主密鑰
$ STM32_KeyGen_CLI -rand 16 stm32mp13-key/stm32mp13_encryption_key.bin
```

在生產線或工程測試環境中，透過 U-Boot 專屬指令執行硬體熔絲燒錄：
```bash
# 1. 燒錄公鑰雜湊（PKHTH）
STM32MP> stm32key select PKHTH
STM32MP> load mmc 0:1 ${loadaddr} publicKeysHashHashes.bin
STM32MP> stm32key fuse ${loadaddr}

# 2. 燒錄對稱加密裝置主密鑰（EDMK）
STM32MP> stm32key select EDMK
STM32MP> load mmc 0:1 ${loadaddr} stm32mp13_encryption_key.bin
STM32MP> stm32key fuse ${loadaddr}

# 3. 讀取確認狀態
STM32MP> stm32key read
```
**不可逆警告**：eFuse 為一次性可程式化（One-Time Programmable, OTP）實體元件，一旦燒斷物理熔絲線即永久定型，無法抹除或重寫。

---

### 重點 3：混合架構模式（The Hybrid Approach）：以 NXP i.MX 為例

與 STM32MP1 將加密金鑰直接燒入熔絲不同，NXP i.MX 系列（如 i.MX8、i.MX9）採取了兼具彈性與安全性的混合途徑：

> **原文引用（p.193）**
> "This mechanism is used by the i.MX CPU family by NXP, where only the public key's hash (known as Super Root Key or SRK) is burned into FUSEs, while the encryption key is not stored in FUSEs but is encapsulated into a dedicated blob (known as black key) sealed by the CAAM OTPMK."

**混合架構的核心邏輯**：
1. **熔絲僅存放 SRK 雜湊**：硬體 eFuse 僅燒錄四組公鑰的雜湊表（SRK Hash, Super Root Key Table Hash），大幅節省熔絲位元數。
2. **加密金鑰黑盒化（Black Key Blob）**：對稱加密金鑰（DEK）並不常駐於熔絲中，而是使用晶片在出廠時由硬體亂數產生的唯一主密鑰（OTPMK，One-Time Programmable Master Key）進行封裝，產生 Black Key Blob。
3. **優劣得失對比**：
   - **優點**：加密金鑰可以在不破壞或重燒硬體熔絲的情況下進行版本輪換與更新。
   - **缺點**：開發者必須確保產生 DEK Blob 的執行環境（通常在受信任的製造階段或安全作業系統中）絕對安全，防範封裝前明文洩漏。
4. **工具鏈**：NXP 官方提供之 Code Signing Tool（CST）與 `srktool`。

---

### 重點 4：開源授權衝擊：GPLv3 與 Tivoization（TiVo 化）爭議

當嵌入式安全產品採用 GNU 通用公眾授權條款第三版（GPLv3）軟體時，安全開機的硬體鎖定機制會直接與授權條款產生劇烈法律衝突。作者在此進行了深入的法遵探討：

> **原文引用（p.198–199）**
> "However, if the system uses GPLv3 software components (such as U-Boot or the Linux kernel), the manufacturer must provide the user with the Installation Information to comply with Section 6 of the GPLv3... The practice of using hardware locks to prevent users from running modified versions of GPLv3 software is known as Tivoization. The name is due to the digital video recorder company TiVo, which used GNU GPL software on its devices but designed the hardware to only run software cryptographically signed by TiVo... Stallman believes this practice denies users some of the fundamental freedoms granted by free software."

**GPLv3 第 6 條要求**：
凡散布載有 GPLv3 軟體的消費者硬體（User Product），製造商必須一併提供「**安裝資訊（Installation Information）**」，確保終端使用者能依其意願修改該開源程式碼，並將修改後的程式碼重新安裝於該實體裝置上正常執行！

---

### 重點 5：兼顧 GPLv3 合規與商業機密的「雙軌開機架構」

許多企業擔心：如果完全向使用者開放自訂簽署金鑰，那公司耗費鉅資開發的商業智慧財產（Proprietary Code）、商業演算法與專用證書豈不任人宰割？

作者在書中提出了典範級的**雙軌架構解決方案（Split Chain-of-Trust Architecture，Figure 4-3）**：

```
                              [開機通電]
                                  │
                                  ▼
                     [硬體載入開機載入器 (Bootloader)]
                                  │
                   ┌──────────────┴──────────────┐
                   │                             │
        【官方合法簽名路徑】             【使用者自訂修改路徑】
                   │                             │
    [原廠私鑰驗證通過]                   [合法允許自訂簽章/未簽章執行]
                   │                             │
    [硬體安全模組釋放解密金鑰]            [硬體安全模組拒絕解密封閉機密]
                   │                             │
         [解密並執行完整商業系統]        [僅能以無特權方式啟動開放系統]
    (所有商業智財與安全功能全開)         (無法存取商業演算法與受保護金鑰)
```

> **原文引用（p.200）**
> "users will be able to modify and re-run the GPLv3 software provided in the secured system, fulfilling the requirements of Section 6 of the GPLv3 without exposing the proprietary code. In Figure 4-3, both the secured and modified bootloaders are loaded under the Chain-of-Trust, but just in the left path the Chain-of-Trust continues. On the right one, the chain stops, allowing the end user to load whatever is within the root filesystem. On the other hand, in the left path, the proprietary code is protected by the Chain-of-trust, while in the right one, there is nothing to protect."

**雙軌合規核心機制**：
1. **滿足 GPLv3**：提供使用者替換與編譯 U-Boot / 核心的工具與說明，裝置允許載入使用者自簽程式碼，完全符合 Section 6 要求。
2. **保護商業機密**：硬體安全單元（CAAM / OP-TEE / TPM）在偵測到開機鏈條非官方合法簽章時，立即**拒絕釋放**專屬商業金鑰與根分區解密密鑰；自訂系統只能在非受保護的空白或一般使用者分區運行，商業機密絲毫不受侵害。

---

### 本章 Summary（作者原話）

> **原文引用（p.201）**
> "In this chapter, we’ve seen some simple examples of how to generate keys to encrypt and sign bootloaders (and fitimages) for two specific types of CPUs (although these examples aren't generic, they provide an idea of what to do to protect our systems).
> We then introduced some aspects of how to keep our system compliant with GPLv3 requirements to avoid tedious (and expensive) legal battles.
> At this point, it should be clearer what Secure Boot is and the steps to follow to get a working system.
> Enjoy!"

---

### 思考與實務檢驗

1. **為什麼很多專案的 U-Boot 仍停留在 GPLv2 版本？這與安全開機有何關係？**
   - GPLv2 沒有第 6 條關於「安裝資訊」與反 TiVo 化的強制規範。因此，若裝置採用 GPLv2 授權的開機引導程式（且完全不包含 GPLv3 程式庫），製造商在法律上不被強制要求提供私鑰或硬體簽名解鎖手段。然而一旦導入新版包含 GPLv3 的組件，就必須嚴格實施雙軌開機或提供金鑰更換通道。
2. **STM32MP1 的全熔絲模式 vs. i.MX 的黑金鑰模式，在製造良率上有何影響？**
   - 全熔絲模式中，若燒錄過程中發生斷電或通信錯誤導致熔絲位元寫入殘缺，整顆晶片可能直接永久報廢（無法補救）。而混合模式僅需燒錄少量的 SRK Hash 熔絲，大量加密金鑰皆以軟體檔案形式的 Black Key Blob 存在，製造失敗風險與晶片報廢率顯著降低。

---

### 我的整理：本章帶走的五句話

1. **硬體熔絲（eFuse）是物理世界不可逆的單向開關，一旦燒斷便永久確立設備的信任基石。**
2. **STM32MP1 採取全熔絲策略，將公鑰雜湊與主加密密鑰全數硬化在晶片微線路中。**
3. **i.MX 採取混合策略，以熔絲保護根公鑰雜湊，並透過硬體 CAAM OTPMK 封裝可置換的黑金鑰。**
4. **GPLv3 第 6 條禁止以密碼鎖死硬體（Anti-Tivoization），廠商有義務讓用戶能替換與執行修改版軟體。**
5. **化解開源法遵與商業智財衝突的解法是「雙軌開機」：允許執行修改版核心，但硬體模組切斷對商業機密的解鎖通道。**

---
## 附錄 A：Notes on Tamper Detection（防篡改偵測筆記）

> 對應原書 p.202–216

### 這章在幹嘛

在高度關鍵與極端嚴苛的安全領域（如發電廠與輸配電網的 IED 智慧電子裝置、遠端終端單元 RTU、分散式控制系統 DCS、醫療生命維持儀器、金融 POS 終端與計費系統），系統威脅不僅來自網路封包，更常來自**實體物理接觸攻擊（Physical Intrusion）**。

一旦攻擊者撬開機殼、接觸印刷電路板（PCB），便能透過硬體探針側錄總線、替換關鍵晶片或發動故障注入（Fault Injection）。此時所有純軟體防禦皆形同虛設。

本附錄專門討論嵌入式硬體防篡改系統的設計原則：
1. **外部防篡改（External Tamper）與內部防篡改（Internal Tamper）機制**。
2. **哨兵暫存器（Sentinel Register）與零化（Zeroization）破壞性反應**。
3. **開機階段的電池監控與防護初始化代碼實作**。
4. **Linux 執行期的篡改偵測與 sysfs 狀態回報**。

---

### 重點 1：物理實體入侵的致命威脅與防禦維度

作者指出，在實體可被接觸的環境下，防線必須延伸到物理世界：

> **原文引用（p.202）**
> "In high-security environments, the integrity of a system extends beyond the digital realm. In fact, a successful physical intrusion, such as opening a chassis and inserting a malicious hardware component or removing a benevolent one, allows an attacker to bypass many traditional software-based controls. Let’s think about Intelligent Electronic Devices (IED), Remote Terminal Units (RTUs), or Distributed Control Systems (DCS) for critical infrastructure... life-support and healthcare medical devices, and fiscal measurement systems"

實體防篡改偵測主要分為兩大維度：
1. **外部篡改偵測（External Tamper）**：監控機殼與 PCB 實體結構。
2. **內部篡改偵測（Internal Tamper）**：監控晶片自身的微觀運作物理環境。

---

### 重點 2：外部篡改偵測（External Tamper）：機殼感測與 PCB 導電網

處理器通常配備專屬的**防篡改實體針腳（Tamper Pins）**：

> **原文引用（p.203）**
> "Physical detection can be divided into two categories: external tamper (on the chassis/board) and internal tamper (on the chip itself ). The former mechanism is specifically designed to detect the opening of the chassis or the tampering of board components, and usually the processor has dedicated input pins (often called tamper pins) to detect these attempts. These tamper pins are connected to a physical sensor installed on the system’s enclosure, such as a simple switch that opens or closes when the chassis is opened (the most common method). Or conductive thin traces printed on the PCB or inside the chassis where their breakage or interruption, caused by an attempt to drill or force open the system, triggers an alarm."

**常見硬體感測手法**：
- **機殼常閉微動開關（Micro-switch）**：當外殼被拆卸或掀開時，開關彈開中斷迴路，立即向處理器發送硬體中斷。
- **PCB 連續導電蛇形走線（Conductive Traces Mesh）**：在電路板表面層鋪設微細導線網格。若攻擊者嘗試用電鑽鑽孔接觸底層總線，走線斷裂或短路即刻觸發警報。
- **光敏／壓力感測器**：外殼密封內部為完全黑暗，若有外部光線射入即判定外殼已遭破壞。

---

### 重點 3：內部篡改偵測（Internal Tamper）：電壓、溫度與時脈監控

即使攻擊者不破壞機殼，也可能透過環境干擾引發處理器異常跳轉（Fault Attack / Glitch Attack）：

> **原文引用（p.204）**
> "On the other hand, the internal tampering detection is a set of mechanisms that are usually not linked to the chassis but monitor the CPU's immediate operating environment. For example, constant monitoring of the power supply voltage levels (a sudden drop or spike in voltage can indicate a physical attack attempt or an attempt to force a chip reset) or temperature monitoring (too high or too low temperature can indicate an attack, e.g., freezing the chip to alter memory or overheating) or by monitoring the system clock frequency (clock tampering is an attack technique to disrupt or slow down the processor)."

**三大晶片監控機制**：
1. **電壓毛刺偵測（Voltage Glitch Monitor）**：突發的瞬間降壓或突波通常代表攻擊者試圖繞過密碼指令的跳轉判斷。
2. **極端溫度感測（Temperature Sensors）**：低溫冷凍可延長 RAM 殘留時間（Cold Boot Attack），高溫可引發硬體錯誤。超出正常工作區間即判斷受攻。
3. **時脈毛刺偵測（Clock Glitch Monitor）**：監控主頻率震盪器，防止時脈被瞬間超頻或跳拍引發指令執行錯誤。

---

### 重點 4：零化反應（Zeroization）：破壞性銷毀密鑰

當防篡改電路觸發時，系統唯一的正確防禦反應就是**同歸於盡式的機密銷毀**：

> **原文引用（p.204）**
> "Once a tampering access has been detected, the most critical feature of a good tampering system is its response, which must be immediate and destructive to sensitive data. Firstly, an alarm flag in a dedicated register (sentinel) is set; then the most common and crucial reaction is the zeroization of sensitive secrets (such as encryption keys) stored in battery-backed secure RAM."

**零化反應（Zeroization）特點**：
- **硬體自發動作**：不依賴軟體或 CPU 執行程式碼（因為軟體可能已被凍結或攻擊者切斷時脈），純硬體邏輯在數個時脈週期內完成。
- **清空 Secure RAM**：由硬體開關直接將存放主密鑰的低功耗電池供電 RAM（SNVS Secure Non-Volatile Storage）強制拉低接地，所有金鑰位元全數歸零（0x00）。
- **永久失效**：失去主金鑰後，磁碟中的所有資料與後續開機引導程式皆無法解密，設備自動變磚，但機密絕不落入敵手。

---

### 重點 5：哨兵暫存器（Sentinel Register）與備用電池 domain

防護電路必須在主電源切斷（甚至整機拔掉插頭）時依然 24 小時保持警戒。這依賴專屬的**電池供電安全域（Battery-Backed Security Domain）**：

> **原文引用（p.204–205）**
> "to detect tampering accesses under all circumstances; in fact, by properly setting up the sentinel register, we can detect whether this battery has been removed or not... The battery-backed region is also not directly accessible by the normal world (which is unprivileged), but it must be accessed through a privileged supervisor. For example, on ARM-based systems, the supervisor can be either the ATF or OP-TEE according to the specific boot stage."

**哨兵暫存器工作機制**：
- 在安全初始化階段，特權軟體向電池供電暫存器寫入一個預設的魔術數值（如 `SENTINEL_MAGIC = 0x5a5a`）。
- 若主板鈕扣電池被拔除、電量耗盡或發生篡改，該暫存器數值因斷電而遺失並重置為預設的 `0x0000`。
- 開機時，SPL / TF-A 優先讀取哨兵暫存器；若數值非魔術字元，即判定發生過「電池斷電篡改」。

---

### 重點 6：開機階段的防篡改狀態機與復原程式碼

書中給出具體的 Bootloader 檢查邏輯與狀態機：

```c
// 檢查防篡改電池與哨兵暫存器狀態
static bool is_tampering_battery_ok(void)
{
    // 讀取電池備援暫存器
    return (read_sentinel_register() == SENTINEL_MAGIC);
}

// 開機引導核心入口
void check_tamper_state(void)
{
#if defined(CONFIG_TAMPERING_INIT_FORCED)
    // 工廠初始化或合法授權復原
    setup_tampering_battery_status();
    hang("tampering detection initialized. Reset the system...");
#endif

    // 開機時使能硬體篡改偵測電路
    enable_tamper_detection();

    // 檢查電池與暫存器
    if (!is_tampering_battery_ok()) {
        hang("Tampering battery fault! Restart the system...");
    }
}
```

> **原文引用（p.206–207）**
> "The first section of the above code (i.e., the tampering recovery section) is used to initialize or recover the system. The second one (the tampering enable section) is used to actually enable the tampering detection within the CPU (by using the enable_tamper_detection() function) and then to check the tampering battery status by checking the sentinel register"

若檢查失敗，系統直接呼叫 `hang()` 永久暫停執行，阻絕進入任何作業系統。

---

### 重點 7：Linux 執行期的狀態回報與 sysfs 介面

在系統成功開機進入 Linux 使用者空間後，核心驅動程式持續透過中斷與暫存器監控硬體狀態，並透過 sysfs 節點向使用者空間揭露：

> **原文引用（p.211–212）**
> "If a tampering event is detected, the supervisor may force a reboot (in order to stop the normal boot sequence) or we can use a system daemon that monitors the tampering events and then properly react... And, by using the same interface, a user space application can detect whether the machine has been tampered with or not:
> `$ cat /sys/devices/soc0/tampered`
> `no`"

使用者空間的防護守護程序（Daemon）可監聽此節點。一旦狀態轉變為 `yes`，程序可立即執行緊急處置（如主動銷毀應用層機密日誌、發送遠端警報後強制關機）。

---

### 本章 Summary（核心精要）

在具備高物理安全要求的場景中，防禦邊界不能止步於韌體簽章。外部機殼微動開關與 PCB 導線網結合晶片內部的電壓、溫度、時脈毛刺監控，構成了全方位的實體感測陣列；而電池供電安全域中的哨兵暫存器與硬體零化機制，確保了一旦外力破壞發生，關鍵密鑰將在數個週期內自我毀滅，徹底阻斷實體探針竊密的可能。

---

### 思考與實務檢驗

1. **若工廠在組裝板子更換鈕扣電池時，如何避免誤觸發防篡改警報？**
   - 必須透過受控的專用工程韌體（定義了 `CONFIG_TAMPERING_INIT_FORCED`），在授權環境下重新對哨兵暫存器寫入魔術字元以初始化狀態；該工程韌體本身必須具有原廠最高簽章權限，一般未簽名韌體無法執行此復原動作。
2. **硬體零化（Zeroization）是否會造成不可逆的設備報廢？**
   - 取決於硬體架構設計。若零化的是動態電池備援 RAM（SNVS RAM），更換電池並重新在安全產線完成原廠重置即可復活；但若晶片設計為一旦篡改即物理燒斷專用防篡改熔絲，則整顆 SoC 將永久報廢。

---

### 我的整理：本章帶走的五句話

1. **面對手持電鑽與邏輯探針的物理攻擊者，純軟體安全機制完全無能為力。**
2. **外部防篡改看機殼微動開關與 PCB 導線網，內部防篡改看電壓、溫度與時脈毛刺。**
3. **哨兵暫存器依靠鈕扣電池在斷電時維持記憶，電池拔除等同於遭外力篡改。**
4. **硬體零化（Zeroization）能在數個微秒內強制放電拉低金鑰暫存器，達成密鑰同歸於盡。**
5. **Linux 核心透過 sysfs 節點提供執行期篡改狀態回報，供守護程序發動緊急銷毀應變。**

---
## 附錄 B：Securing Secrets Across Booting Stages（跨開機階段保護機密）

> 對應原書 p.217–236

### 這章在幹嘛

在完整的信任鏈中，不同軟體階段（Stage）之間不可避免地需要交換資料。最典型的範例就是：**U-Boot 如何將儲存裝置名稱、加密分區參數甚至是解密密鑰傳遞給 Linux 核心？**

傳統上，這一切都是透過 U-Boot 的環境變數（Environment Variables）與核心命令列（`bootargs`）完成。然而作者在本書最後投下一枚震撼彈：**未受保護的 `bootargs` 是一片完全未加密的明文記憶體區域，它極易成為徹底瓦解整條信任鏈的重大破口！**

本附錄深入剖析：
1. **命令列注入攻擊（Command Line Exploit）**：如何透過竄改 `init=` 或 `rdinit=` 直落 root shell。
2. **核心硬化防禦（Hardened Kernel Config）**：啟用 `CONFIG_CMDLINE_FORCE=y` 阻絕外部參數竄改。
3. **核心內部金鑰注入器（Key Injector Kernel Module）**：使用 `keyring_alloc()` 與 `key_create_or_update()` 在核心層級直接註冊 `logon` 金鑰。
4. **逆向工程攻防分析**：使用 `objdump` 檢視 `.rodata` 剖析硬編碼金鑰的危險性，並指明硬體安全根（CAAM / OP-TEE）的終極結合之道。

---

### 重點 1：跨階段資料交換的致命破口 —— U-Boot 明文環境變數

在多數開發者的認知中，U-Boot 透過命令列向 Linux 核心傳遞參數是理所當然的操作：

```bash
optargs=setenv bootargs ${bootargs} ${kernelargs};
```

然而作者從專業安全架構師的角度提出嚴厲警告：

> **原文引用（p.217–218）**
> "For a security expert developer, this data exchange can be considered a security risk that can compromise the Chain-of-Trust. In fact, U-Boot holds its environment within a plain text memory area that can interact with the following stages (such as the kernel or the user space applications). Due to its unsecure nature, it can be used to exploit the Chain-of-Trust if not correctly managed."

**破口核心原因**：
1. U-Boot 環境變數通常儲存於 SPI Flash 或 eMMC 某個非加密磁區。若未啟用環境變數簽署或保護，攻擊者透過硬體燒錄器或暫存區注入即可修改。
2. 開發階段留下的序列埠終端機（Serial Console）若未鎖死，攻擊者只要在倒數計時中按下空白鍵進入 U-Boot prompt，便能任意改寫 `bootargs`。

---

### 重點 2：命令列劫持實測：透過 `rdinit=/bin/sh` 直落 Root Shell

攻擊者能透過竄改 `bootargs` 造成何種毀滅性後果？作者在書中進行了無情實測：

若攻擊者將開機參數附加：
```bash
rdinit=/bin/sh
```
核心在開機末期執行 `init/main.c` 中的初始化流程：

> **原文引用（p.220–221）**
> "We can see this behavior in the file init/main.c of the Linux sources:
> `ret = run_init_process(CONFIG_DEFAULT_INIT);`
> ... Now, on the next boot, we get on the system’s serial console...
> /bin/sh: can't open 'initramfs_normal': No such file or directory"

**攻擊後果剖析**：
核心原本應當執行具備密鑰驗證與簽章檢查的 `initramfs_normal` 流程，卻因為被 `rdinit=/bin/sh` 劫持，核心直接繞過了所有預設驗證腳本，在串口上直接彈出**毫無任何密碼保護的 root 互動式 shell**！
更糟的是：若開發者偷懶在 `bootargs` 中傳遞密碼（例如 `rootfs.key=0123456789abcdef`），任何本機使用者只要執行 `cat /proc/cmdline` 或 `dmesg`，金鑰明文就完全洩漏！

---

### 重點 3：第一道防線：核心強制鎖死命令列（CONFIG_CMDLINE_FORCE）

針對命令列參數被置換的風險，第一層快速有效的防護是在編譯核心時將開機參數「硬化」：

> **原文引用（p.220）**
> "To prevent this kind of attack, we can instruct the kernel to ignore the U-Boot bootargs completely by enabling CONFIG_CMDLINE_FORCE=y in the kernel configuration."

**核心組態關鍵項目**：
- `CONFIG_CMDLINE_BOOL=y`：開啟靜態命令列支援。
- `CONFIG_CMDLINE="console=ttymxc0,115200 root=/dev/mmcblk2p3 ..."`：將合法的開機參數寫死在核心二進制映像中。
- `CONFIG_CMDLINE_FORCE=y`：**強制核心完全忽略來自 Bootloader（U-Boot）傳入的任何 bootargs**！

此舉能徹底粉碎串口終端或未授權 U-Boot 腳本注入 `init=/bin/sh` 的企圖。

---

### 重點 4：第二道防線：核心金鑰注入模組（key_injector.ko）架構

如果命令列被鎖死，且金鑰不能出現在 `bootargs` 或磁碟檔案中，initramfs 又該如何取得金鑰以解開根檔案系統？

作者提出了一套高階純核心方案：**撰寫專屬的核心模組（`key_injector.ko`），在開機時直接調用核心內部 C 語言 API，將金鑰直接注入 Linux Keyring！**

核心模組的核心邏輯包含兩個關鍵函數：
1. **建立專屬 Keyring**：使用 `keyring_alloc()`。
2. **註冊安全金鑰**：使用 `key_create_or_update()` 註冊為 `logon` 類型。

---

### 重點 5：key_injector.ko 核心原始碼深度拆解

```c
#include <linux/module.h>
#include <linux/key.h>
#include <linux/key-type.h>

static char *keyring_description = "secure_boot_ring";
static char *key_description = "rootfs_key";
static unsigned char key_payload[32] = {
    0x24, 0xcc, 0xd6, 0x90, 0x79, 0x64, 0x3e, 0x83,
    0x35, 0xe2, 0xd8, 0xdb, 0xbf, 0x81, 0xcb, 0x54,
    0xad, 0x36, 0x5b, 0x79, 0x8c, 0x53, 0x99, 0x3b,
    0x3f, 0xc7, 0xfb, 0x73, 0xd8, 0x57, 0x42, 0x09
};

static int __init key_injector_init(void)
{
    struct key *keyring;
    key_ref_t key_ref;

    // 1. 建立屬於 root 的核心 Keyring
    keyring = keyring_alloc(keyring_description,
                            GLOBAL_ROOT_UID, GLOBAL_ROOT_GID,
                            current_cred(),
                            KEY_POS_ALL | KEY_USR_VIEW | KEY_USR_READ | KEY_USR_SEARCH,
                            KEY_ALLOC_NOT_IN_QUOTA,
                            NULL, NULL);
    if (IS_ERR(keyring))
        return PTR_ERR(keyring);

    // 2. 在該 Keyring 中建立 logon 類型的金鑰
    key_ref = key_create_or_update(make_key_ref(keyring, true),
                                   "logon",
                                   key_description,
                                   key_payload,
                                   sizeof(key_payload),
                                   KEY_POS_ALL | KEY_USR_VIEW | KEY_USR_SEARCH,
                                   KEY_ALLOC_NOT_IN_QUOTA);
    if (IS_ERR(key_ref)) {
        key_put(keyring);
        return PTR_ERR(key_ref);
    }

    pr_info("key_injector: Key successfully injected into keyring!
");
    return 0;
}
```

> **原文引用（p.226, p.230）**
> "First, the function allocates the keyring where to place the key by using the function keyring_alloc()... If everything works well (a.k.a., no errors arise), the function uses key_create_or_update() to create the key and insert it into the just created keyring as a logon key."

**運作優勢**：
- 模組在 initramfs 階段由 `insmod key_injector.ko` 載入。
- 注入的金鑰為 `logon` 類型，使用者空間的 `keyctl read/pipe` 永遠無法回讀明文載荷。
- 後續的 `dm-crypt` 透過核心內部 API 或序號直接使用該金鑰解開 `/dev/mapper/root`，金鑰明文完全不落地。

---

### 重點 6：逆向工程分析：使用 objdump 暴露硬編碼金鑰缺陷

作者身為資深工程師，在展示完 `key_injector.ko` 後，立刻指出了它的**致命盲點**：

> **原文引用（p.236）**
> "So, to read its content, we must use objdump specifying the variable's offset 0x18 reported above:
> `$ aarch64-linux-gnu-objdump -s -j .rodata key_injector.ko`
> `Contents of section .rodata:`
> `0010 ... 24ccd690 79643e83`
> `0020 35e2d8db bf81cb54 ad365b79 8c53993b`
> `0030 3fc7fb73 d8574209 ...`
> And we can see that the desired data are the 32 bytes at offset 0x18, that is, 24ccd690 79643e83 35e2d8db bf81cb54 ad365b79 8c53993b 3fc7fb73 d8574209."

**逆向漏洞展示**：
若開發者直接將金鑰常數以靜態陣列寫在 C 語言程式碼中，編譯出來的 `.ko` 模組內部，該 32 位元組密鑰會原封不動、毫無遮掩地躺在 `.rodata` 唯讀資料段中！任何具備二進制反組譯工具（如 `objdump`、`strings`、IDA Pro 或 Ghidra）的攻擊者，只需幾秒鐘即可將其精準挖出。

---

### 重點 7：工業級最終架構：結合硬體 Root-of-Trust

作者在全書末尾總結：靜態程式碼注入只是「教育性範例（Educational Example）」。真正的工業高安全架構，必須將此流程與前述的硬體設施完全閉環：

> **原文引用（p.227）**
> "Please also note that the solution presented here is intended solely as an educational example. In industrial-grade and security-critical systems, the cryptographic keys must never be hardcoded into the kernel module source code."

**工業級閉環實踐**：
1. **結合 Secure World**：金鑰不能靜態寫死在模組中，而應由 `key_injector.ko` 透過 SMC（Secure Monitor Call）呼叫 OP-TEE 或向 CAAM 請求。
2. **硬體解開封裝**：CAAM 從 OTPMK 熔絲或安全暫存器解封 Black Key Blob，並直接載入硬體密碼引擎暫存器。
3. **無明文記憶體暴露**：整個系統在開機、載入、解密根分區的所有環節中，記憶體與磁碟皆不曾出現過 1 個位元的紅金鑰（Red Key）明文，達成真正牢不可破的硬體級 Linux Secure Boot！

---

### 本章 Summary（核心精要）

U-Boot 命令列與環境變數是脆弱的明文傳遞通道，若未加以防範，`init=/bin/sh` 注入即可秒殺整條信任鏈。透過核心 `CONFIG_CMDLINE_FORCE=y` 鎖死命令列、並搭配核心級 `keyring_alloc` 與 `key_create_or_update` 註冊 `logon` 金鑰，能在核心內部達成安全的機密傳遞；而徹底防禦反組譯逆向的終極解答，則是將金鑰生成與解封完全委託給硬體 CAAM 與 OP-TEE 信任根。

---

### 思考與實務檢驗

1. **若開啟了 `CONFIG_CMDLINE_FORCE=y`，工程師在開發板除錯時如何自訂串口 baudrate 或切換開機分區？**
   - 在開發除錯階段（Development Build），通常關閉此選項以保持除錯彈性；只有在發行生產韌體（Production Release）且燒斷 eFuse 鎖死晶片之前，才強制開啟 `CONFIG_CMDLINE_FORCE=y`。
2. **為什麼即使 `key_injector.ko` 經過數位簽名，將明文金鑰寫在程式碼中依然極度危險？**
   - 數位簽名（Signature）只能保證模組未遭竄改（Integrity），但**不能提供機密性（Confidentiality）**。任何人只要把 `.ko` 檔案從根分區或 initramfs 中複製出來，即可使用 `objdump` 讀取 `.rodata` 段。因此機密必須永遠處於「硬體封裝（Sealed Blob）」狀態。

---

### 我的整理：本章帶走的五句話

1. **U-Boot 明文環境變數是開機鏈中最常被忽視的後門，參數竄改能直接癱瘓信任鏈。**
2. **開機參數注入 `rdinit=/bin/sh` 能直接跳過所有認證、彈出無密碼 root shell。**
3. **生產環境核心應強制啟用 `CONFIG_CMDLINE_FORCE=y`，徹底忽視未受信任的 bootargs。**
4. **利用核心模組動態呼叫 `key_create_or_update` 註冊 `logon` 金鑰，能讓密鑰在核心內安全遞送。**
5. **永遠不要在 C 語言程式碼中硬編碼靜態金鑰，唯有硬體 Root-of-Trust（CAAM / OP-TEE）才能抵禦二進制逆向工程。**

---
