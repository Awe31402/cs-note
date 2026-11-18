# A.2 Wally Configurations

> 原書 p.784.e106–784.e124（Appendix A: Wally Synopsis）

## 這節在講什麼

六張表，是全書最密集的參考資料：
- **Table A.1**：六個基礎組態與它們的合成結果
- **Table A.2**：預設記憶體映射
- **Table A.3**：會隨組態變化的關鍵參數
- **Table A.4**：其他參數（各組態相同但可改）
- **Table A.5**：衍生組態（為驗證、合成、FPGA 而變化出來的）
- **Table A.6**：所有已批准的 RISC-V 擴充與 profile 的對照

這節不需要「讀懂」，而是知道**要查什麼的時候翻哪一張**。

## 關鍵點

### 六個基礎組態（Table A.1）

> "Table A.1 lists the main Wally configurations introduced in Section 2.8.2, with their main features and figures of merit from TSMC 28nm synthesis."
> — p.784.e106

中文解釋：

| 組態 | XLEN | Profile | DTIM/IROM | 匯流排 | 周邊 | 快取 | 特權模式 | 虛擬記憶體 | 最高頻率 | 邏輯面積 | 記憶體面積 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Embedded `rv32e` | 32 | — | — | 有 | 無 | 無 | 無 | 無 | **2.99 GHz** | **0.007 mm²** | — |
| Simple CPU `rv32i` | 32 | RVI20U32 | 2/2 KiB | 無 | 無 | 無 | 無 | 無 | 2.58 | 0.023 | — |
| Microcontroller `rv32imc` | 32 | RVI20U32 | 4/16 KiB | 有 | 有 | 無 | M,U | 無 | 2.18 | 0.045 | — |
| Apps `rv32gc` | 32 | RVI20U32 | — | 有 | 有 | 16K | M,S,U | 有 | **0.89** | 0.119 | 0.314 |
| Simple CPU `rv64i` | 64 | — | 2/2 KiB | 無 | 無 | 無 | 無 | 無 | 2.52 | 0.032 | — |
| Apps `rv64gc` | 64 | RVA22S64 | — | 有 | 有 | 16K | M,S,U | 有 | **0.80** | 0.176 | 0.314 |

**這張表最有資訊量的是「頻率 vs 面積」的兩極**：
- `rv32e` 跑 2.99 GHz、只佔 0.007 mm²
- `rv64gc` 只跑 0.80 GHz、邏輯面積 0.176 mm²（25 倍）加記憶體 0.314 mm²

**頻率差了 3.7 倍**——這是「加功能就變慢」的直接量化（第 6 章的 PPA 分析）。主要拖累來自快取存取路徑和虛擬記憶體的位址轉換（呼應第 16 章「FPU 的關鍵路徑和快取存取路徑相當就夠了」）。

### 記憶體映射（Table A.2）

> "Table A.2 provides the default memory map for configurations that have peripherals."
> — p.784.e106

中文解釋：

| 裝置 | 起始位址 | 大小 |
| --- | --- | --- |
| Boot ROM | 0x00001000 | 4 KiB |
| SDC | 0x00013000 | 4 KiB |
| CLINT | 0x02000000 | 64 KiB |
| PLIC | 0x0C000000 | 64 MiB |
| UART | 0x10000000 | **8 B** |
| SPI | 0x10040000 | 4 KiB |
| GPIO | 0x10060000 | 256 B |
| RAM | 0x80000000 | 128 MiB（第 22 章擴到 256 MiB） |

**幾個值得注意的設計**：
- **Boot ROM 在 0x1000**——就是第 22 章的 reset vector
- **UART 只有 8 位元組**——第 20.4 章講的八個 8 位元組暫存器，一個不多
- **PLIC 佔 64 MiB**——遠超過實際需要，是為了讓每個 context 的暫存器能落在不同的頁（第 20.3 章講的權限隔離）
- **RAM 從 0x80000000 開始**——所有 RISC-V 系統的慣例

### 兩張參數表的分工（Table A.3 / A.4）

> "Table A.3 lists key Wally configuration parameters that vary from one configuration to another, and Table A.4 lists other microarchitectural parameters that have common values in the Wally configurations that need them but could be changed (including the base and range of the memory map)."
> — p.784.e106

中文解釋：**分工的原則是「會不會因組態而異」**。

Table A.3（約 50 個參數）的價值在於**它是整本書的功能清單**，而且標了章節。橫著讀某一行能看到「這個功能哪些組態有」，直著讀某一欄能看到「這個組態支援什麼」。幾個觀察：

- `rv32e` 幾乎全部是 0，只有 `E_SUPPORTED`、`BUS_SUPPORTED`、`BOOTROM_SUPPORTED`、`UNCORE_RAM_SUPPORTED` 是 1
- `rv32gc` 和 `rv64gc` 幾乎一樣，差別在：`ZCF_SUPPORTED`（RV32 才有壓縮單精度浮點）、`ZICCLSM_SUPPORTED`（只有 rv64gc 開，見第 19.2 章的驗證考量）、`IDIV_ON_FPU`（只有 rv64gc，見第 16.6.4 章）、`DIVCOPIES`（2 vs 4）
- `rv32imc` 是「有周邊但沒快取、沒虛擬記憶體」的微控制器定位

Table A.4 的參數多半是**尺寸與位址**：快取幾路、每路多大、快取線多長、分支預測器幾個項目、TLB 幾個項目、各周邊的 base/range。典型值：
- D$/I$：4 路 × 4,096 位元組 × 512 位元的線
- 分支預測器：Gshare，2¹⁰ 項；BTB 2¹⁰ 項；RAS 16 項
- TLB：ITLB 與 DTLB 各 32 項
- `RESET_VECTOR = 0x80000000`（但 buildroot 衍生組態改成 0x1000）

### 衍生組態：Table A.5 才是驗證策略的全貌

> "In addition to the base configurations, Wally generates derivative configurations for verification and synthesis, as described in Section 4.4, and for various platforms, such as an FPGA, as described in Chapter 23. The derivatives are variations on base configurations created by changing certain parameter values."
> — p.784.e108–e109

中文解釋：**這張表比前面幾張更有啟發性**——它顯示了「怎麼用參數化設計做系統性驗證」。衍生組態分成幾類：

**平台類**：`buildroot`（模擬跑 Linux）、`fpga`（FPGA 上跑 Linux）

**合成類**：`syn_*` 系列把記憶體縮小（`DTIM_RANGE = 0x1FF`、`WAYSIZEINBYTES = 512`、`NUMWAYS = 1`、`BPRED_SIZE = 5`）——因為第 6 章的合成實驗只關心邏輯面積，不想被大記憶體淹沒。`syn_sram_*` 則用真的 SRAM 巨集。

**逐層剝功能**：`syn_rv64gc_pmp0` → `noPriv` → `noFPU` → `noMulDiv` → `noAtomic`，**每一層在前一層的基礎上再拿掉一個功能**。這是第 6 章量化「每個功能值多少面積」的方法。

**參數掃描**（為了效能研究與驗證覆蓋）：
- `div_{2/4}_{1/1i/2/2i/4/4i}` — 除法器的 radix、每週期位元數、整數除法是否用 FPU
- `ram_{0/1/2}_{0/1}` — **RAM 延遲 0/1/2 週期 × burst 開關**，「verify the bus is robust」。這正是第 23.4.1 章那個 burst bug 修好後加進來的測試
- `bpred_*` — 分支預測器的型別、大小、RAS 大小、BTB 大小、IC 預測開關（第 21.3 章的掃描）
- `way_*` — 快取的路數、每路大小、線長（第 10 章）
- `tlb2` / `tlb16` — TLB 大小
- `noicache` / `nodcache` / `nocache` — 無快取操作

**單一擴充開關**：`zba_`、`zbb_`、`zbc_`、`zbs_`、`zbkb_`… **「每次只開右欄的一個參數，其他都是 0」**——這樣才能確認每個擴充都能獨立運作，而不是只在全開時才對。

**權限層級剝除**：`noS_` → `noU_`（第 8 章）

**這張表的教訓**：參數化設計的價值不只是「一份程式碼支援多種產品」，更是**讓驗證能系統性地掃描設計空間**。

### 擴充與 profile 對照（Table A.6）

> "Table A.6 summarizes the RISC-V extensions and profiles that were ratified at the time of this writing. If an extension is supported in at least one of the Wally configurations, the table indicates which section describes the support. If the extension is required (R) or optional (O) for a particular profile, it is marked in the corresponding column. In particular, the rv64gc profile supports all the required extensions for RVA22S64."
> — p.784.e109

中文解釋：這張表跨了七頁，列出約 100 個擴充 × 九個 profile。實用的讀法有三種：

**(1) 看 Wally 支援什麼**——有「Section」欄位的就是書裡有講、Wally 有實作的。沒有的標「Not supported」，包括：V（向量）、H（hypervisor）、Sdtrig/Sdext（除錯）、Zcmp/Zcmt（壓縮 push/pop 與表跳躍）、Zacas（原子比較交換）、Zksed/Zksh（ShangMi 國密演算法）、Z*inx（浮點放整數暫存器）、以及所有 Zv*（向量擴充）。

**(2) 看 profile 的演進**——RVI20 → RVA20 → RVA22 → RVA23/RVB23。幾個明顯的趨勢：
- **越來越多東西從 O（選用）變成 R（必要）**：例如 B（位元操作）在 RVA22 還沒有，RVA23 變成 R
- **RVA23 開始要求 V（向量）**——這是 Wally 沒實作的最大缺口
- **Zicbom/Zicbop/Zicboz（第 19.1 章）在 RVA22 才成為 R**——這解釋了第 19 章為什麼要專門處理它們

**(3) 看批准日期**——從 12/2019（基礎 ISA）到 2/2025（Sdtrig/Sdext），**六年間批准了近百個擴充**。這具體說明了第 19 章開頭那句「RISC-V is evolving rapidly」。

（小細節：有些擴充標註「\*Ratified with the RVA23 profile」——它們是連同 profile 一起批准的，而不是單獨批准。）

## 名詞表

| 名詞 | 意思 |
| --- | --- |
| 基礎組態 / 衍生組態 | 六個主要設定／由它們改參數產生的變體 |
| figures of merit | 評比指標（此處為 TSMC 28nm 的最高頻率與面積） |
| DTIM / IROM | data/instruction tightly integrated memory（第 7.2.1 章） |
| `*_SUPPORTED` | 開關某個擴充或功能的參數 |
| `RESET_VECTOR` | 重設後 PC 的值 |
| `DIVCOPIES` | 除法遞迴硬體的複製份數（第 16.6.4 章的 k） |
| `USE_SRAM` | 合成時用真的 SRAM 巨集而非標準單元 |
| `RAM_LATENCY` / `BURST_EN` | RAM 額外延遲／AHB burst 開關（第 23.4.1 章） |
| profile | RISC-V 規定的擴充組合（RVI20、RVA22S64、RVA23U64…） |
| R / O | required（必要）／optional（選用） |

## 所以呢

這節是查表用的，但有三個值得記住的結論：

1. **`rv32e`（2.99 GHz、0.007 mm²）到 `rv64gc`（0.80 GHz、0.49 mm²）差了 3.7 倍頻率、70 倍面積**——這是「完整功能」的價格標籤。
2. **Table A.5 的衍生組態是驗證策略的具體形式**——參數化不只為了產品線，更是為了能系統性掃描設計空間（每次只開一個擴充、掃描所有除法器設定、RAM 延遲 × burst 的六種組合）。
3. **Table A.6 顯示 Wally 的最大缺口是向量擴充**——而 RVA23 已經把 V 列為必要。

下一節 A.3 整理回歸測試套件。
