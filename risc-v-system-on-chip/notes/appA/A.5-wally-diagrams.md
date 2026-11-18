# A.5 Wally Diagrams

> 原書 p.784.e109、784.e128–（Appendix A: Wally Synopsis）

## 這節在講什麼

Table A.11 把全書 31 張主要方塊圖集中列出，每張標明：附錄裡的圖號、原書章節的圖號、在畫什麼、對應哪個 Verilog 模組。圖本身則重印在表格後面（Fig. A.2–A.32）。

**這張表是「從模組名反查設計圖」的索引**——讀 RTL 讀到一半想不起某個模組的整體架構時，翻這裡最快。

## 關鍵點

### 這張表提供什麼

> "Table A.11 lists the high-level diagrams for Wally's main units, which are found throughout the book. It also gives their module names, brief descriptions, and their original figure number in the textbook. Those figures are then shown after Table A.11, for convenience."
> — p.784.e109

中文解釋：**「模組名」這一欄是這張表最有價值的部分**——前面 23 章的圖都是用功能命名的（「取指單元」「快取」），這裡把它們對到 `src/` 底下的實際路徑。

### 31 張圖按階層整理

**頂層與核心**
| 圖 | 內容 | 模組 |
| --- | --- | --- |
| A.2（原 20.3） | SoC 頂層 | `wallypipelinedsoc` |
| A.3（原 7.13） | 簡化的 Wally 核心 | `wallypipelinedcore` |

Fig. A.2 值得單獨看——**它是整本書的一張總圖**。上半部是 uncore（ROM、RAM、GPIO、UART、PLIC、CLINT、SPI 透過 `ahbapbbridge` 掛上匯流排，加上 off-chip AHB 接 DDR 和 SDC），下半部是五級管線裡的六個主要單元（IFU、IEU、LSU、MDU、FPU、privileged），最底下橫跨全部的是 `hazard`。五條虛線標出 Fetch/Decode/Execute/Memory/Writeback 的分界。

**取指與分支預測（第 7、13、14 章）**
| 圖 | 內容 | 模組 |
| --- | --- | --- |
| A.4（原 13.12） | 取指單元 | `ifu` |
| A.5（原 14.3） | 貫穿 IEU 的 PC 邏輯 | `ifu`、`bpred`、`privileged` |
| A.6（原 13.15） | 分支預測 | `ifu/bpred` |

注意 A.5 的模組欄寫了**三個**模組——PC 邏輯不屬於任何單一單元，它散在取指、分支預測、特權三處（因為 trap 也會改 PC）。

**管線控制（第 7 章）**
| A.7（原 16.26） | 冒險單元 | `hazard` |

**記憶體系統（第 10、11、12 章）**
| 圖 | 內容 | 模組 |
| --- | --- | --- |
| A.8（原 12.13） | 載入/儲存單元 | `lsu` |
| A.9（原 10.11） | 快取 | `cache` |
| A.10（原 11.15） | 記憶體管理單元 | `mmu` |
| A.11（原 11.19） | TLB | `mmu/tlb` |
| A.12（原 11.16） | 實體記憶體屬性 | `mmu/pmachecker` |
| A.13（原 11.18） | 實體記憶體保護 | `mmu/pmpchecker` |
| A.14（原 17.5） | 原子記憶體操作 | `lsu/atomic` |

**這七張圖的階層關係值得注意**：`lsu` 底下有 `atomic`，`mmu` 底下有 `tlb`、`pmachecker`、`pmpchecker`——**目錄結構反映了包含關係**。

**匯流排與周邊（第 9、20 章）**
| 圖 | 內容 | 模組 |
| --- | --- | --- |
| A.15（原 9.25） | 外部匯流排單元 | `ebu` |
| A.16（原 9.20） | AHB 與 APB 介面 | `wallypipelinedsoc` |
| A.19（原 9.28） | Uncore：記憶體與周邊 | `uncore` |

**特權架構（第 8 章）**
| A.17（原 8.13） | 特權單元 | `privileged` |
| A.18（原 8.14） | Trap 邏輯 | `privileged/trap` |

**整數運算（第 15、18 章）**
| 圖 | 內容 | 模組 |
| --- | --- | --- |
| A.20（原 18.29） | 含位元操作與密碼學的 ALU | `ieu/alu`、`ieu/bitmanipalu` |
| A.21（原 18.22） | Zbb 基本位元操作 | `ieu/bmu/zbb` |
| A.22（原 18.28） | Zbc carry-less 乘法 | `ieu/bmu/zbb` |
| A.23（原 15.22） | 乘除單元 | `mdu` |
| A.24（原 15.19） | 整數乘法器 | `mdu/mul` |
| A.25（原 15.20） | 整數除法器 | `mdu/div` |

**浮點（第 16 章、附錄 F）**
| 圖 | 內容 | 模組 |
| --- | --- | --- |
| A.26（原 16.21） | FPU | `fpu` |
| A.27（原 16.6） | 解包 | `fpu/unpack` |
| A.28（原 16.11） | 融合乘加 | `fpu/fma` |
| A.29（**原 F.35**） | 浮點/整數除法與開根號 | `fdivsqrt` |
| A.30（原 16.14） | 浮點/整數轉換 | `fcvt` |
| A.31（原 16.15） | 比較與 min/max | `fcmp` |
| A.32（原 16.16） | 後處理 | `postprocess` |

**注意 A.29 的來源是附錄 F 而不是第 16 章**——第 16.2.3 章只講了 `fdivsqrt` 的演算法概念，完整的方塊圖留到附錄 F。這暗示了附錄 F 的深度。

### 怎麼用這張表

三種實用情境：

1. **從模組名找圖**：在 `src/lsu/atomic.sv` 裡迷路了 → 查表發現是 Fig. A.14（原 17.5）→ 翻到第 17 章複習原子操作的資料流。

2. **從圖找模組**：想動手改分支預測器 → Fig. A.6 → `src/ifu/bpred/`。

3. **看階層**：`fpu` 底下有六個子模組（`unpack`、`fma`、`fdivsqrt`、`fcvt`、`fcmp`、`postprocess`）——正好對應第 16.2 章講的「四個執行單元 + 前後共用的解包與後處理」。**表格的模組名欄位直接反映了第 16 章的架構決策。**

## 名詞表

| 名詞 | 意思 |
| --- | --- |
| `wallypipelinedsoc` / `wallypipelinedcore` | SoC 頂層（含 uncore）／處理器核心 |
| uncore | 核心之外、晶片之內：匯流排、周邊、記憶體介面 |
| `ahbapbbridge` | AHB 轉 APB 的橋接器 |
| `bmu` | bit manipulation unit（`ieu` 底下） |
| `bitmanipalu` | 包住所有 Zb*/Zk* 模組的 ALU 外層 |

## 所以呢

Table A.11 是「設計圖 ↔ 程式碼」的對照索引。三個觀察：

1. **模組階層就是設計決策的紀錄**——`mmu` 底下有 `tlb`/`pmachecker`/`pmpchecker`、`fpu` 底下有六個子模組，這些都不是隨意分的。
2. **有些功能跨模組**（PC 邏輯散在 `ifu`+`bpred`+`privileged`），表格誠實地標出來了。
3. **`fdivsqrt` 的圖來自附錄 F**——提示了除法與開根號的實作細節比第 16 章講的深得多。

附錄 A 到此結束。它把全書的設定、參數、測試、指令、方塊圖濃縮成十一張表，是讀完書之後最常回頭翻的部分。
