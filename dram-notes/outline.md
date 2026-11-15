# DRAM 筆記大綱

2026-10-17。範圍、規則看 [BRIEF.md](BRIEF.md)。使用者指示：大綱寫完直接做完。

## 寫大綱前先查到的事（之後正文要引用）

| 事情 | 結果 | 證據 |
|---|---|---|
| DRAM 型號 | LPDDR5，2 rank，每 rank 4 GiB，共 8 GiB（`0x200000000`） | `logs/d0-warm-boot-uart.log` L174、L352-361、L439 |
| 最高速度 | 6400 Mbps（log 寫 `DDR frequency: 6400MHz`，其實是 data rate） | 同上 L438 |
| LK 的 libdram | `libdram-lk.a`，`-Dlk=true` → `-D_FLASH_MODE_DA_`；`lk.bin` 是 prebuilt（`lk-prebuilt`），印 `ca5e1f7` | `libdram-lk.bb`、`src/mt8391/meson.build`、eMMC 筆記 `em1-flash-uart.log` L22 |
| BL2 的 libdram | `libdram-prebuilt`（SRCREV `2db459fe`）裡的 `genio-720-evk/libdram.a`，印 `315bfaa6eef, build time: 20251210-021218` | `strings libdram.a`、`d0` L339 |
| 兩份是不是同一套 code | **是**。`315bfaa`、`ca5e1f7`、HEAD `1913f22` 都在 `lk/libdram-lk`；`src/mt8391` 在三版之間只差一行（`dramc_top.c` 的 `return;` → `return -1;`）。prebuilt 的 77 個 `.o` 名字跟 `src/mt8391/meson.build` 的檔案清單一模一樣 | `git diff 315bfaa ca5e1f7 -- src/mt8391`、`ar t` |
| LK 和 BL2 的差別 | 編譯旗標：LK `-D_FLASH_MODE_DA_`；BL2 `-DCFG_DRAM_CALIB_OPTIMIZATION=1`（存校正結果） | `src/mt8391/meson.build:88-92` |
| 校正結果存哪 | UFS 上的 GPT 分區 `dramk`（`/dev/sdc5`），BL2 log `init partition address is 0x4880000` | `d0` L338、`ls -l /dev/disk/by-partlabel` |
| 熱開機有沒有重校 | 看起來**沒有**：BL2 log 沒有 `Final Impedance Cal Result`、`RXGating` 這類校正輸出（LK 有） | `d0` vs `em1-flash-uart.log` |
| DRAM 大小往下傳 | BL2 `BOOT_ARGUMENT->dram_size`（`0x40000100`）→ U-Boot `dram_init()` → U-Boot 改 dts → Linux | `bl2_plat_setup.c:195`、U-Boot `mach-mediatek/mt8391/init.c:20-30` |
| BL2 保留給 DRAMC 的記憶體 | `dramc-rk0` `0x13fffe000`、`dramc-rk1` `0x23fffe000`，各 8 KiB → U-Boot `mtk_reserved_memory_init()` 加進 dts → Linux `reserved mem` | `d0` L440-441、`cpu.c:104`、Linux dmesg |
| BL31 跟 DRAM 有關的 | `dpm_init`（每個 channel 載一份 DPM 韌體，release note：「DRAM suspend/DVFS support」）、SPM、SSPM、`smpu_init`（region 1 = BL31、region 2 = OP-TEE） | `d0` L447-455、`mt_dpm.c`、`smpu/mt8189/mt_smpu.c` |
| kernel | 沒有 DVFSRC driver、沒有 DRAM devfreq；`dvfsrc_top` 只被時脈 driver 當成一個 gate；MSDC 找不到 `dvfsrc-vcore` 用 dummy | `.config`、`clk-mt8189.c:3041`、dmesg |

## 正文 5 份

### 1. `dram-intro.md` 入門

1. DRAM 為什麼要「初始化」：SRAM 一上電就能用，DRAM 不行（要電壓、時脈、模式暫存器、校正）
2. 名詞：LPDDR5、channel／rank／die、data rate vs 時脈、MR（mode register）、shuffle（一組頻率設定）、self-refresh
3. SoC 這邊的方塊：EMI（記憶體匯流排的入口）、DRAMC（控制器）、DDRPHY、DVFSRC（電壓／頻率投票）、SPM、DPM、SSPM、SMPU（保護）
4. 本板實際的值：型號、容量、channel、rank、5 組 shuffle（1600／1066／3200／2750／2133 MHz 那張表）
5. 每一棒在哪裡跑（SRAM／L3C／DRAM），什麼時候才能用 DRAM
- 圖：intro-01 方塊圖（CPU → EMI → DRAMC ×2 channel → LPDDR5；DVFSRC／SPM／DPM 在旁邊）

### 2. `dram-init.md` 初始化：LK vs BL2

1. 開機順序：誰在 `mt_mem_init()` 之前做了什麼（PLL、PMIC、SPMI、儲存）
   - LK：`platform_early_init()`，沒有儲存
   - BL2：`bl2_platform_setup()`，**先開 UFS、讀 GPT，再初始化 DRAM**（因為要讀 `dramk`）
2. libdram 內部：`mt_mem_init()` → `mt_base_init()` → `normal_mem_init()` → `mt_set_emi()` → …（照原始碼走，標檔名行號）
   - 電壓（Vcore、VMDDR、VDDQ、VDD2H）
   - 偵測型號、rank、大小（MR5～MR8）
   - 校正項目：impedance、CBT、write leveling、RX gating、RX／TX、DATLAT…
   - 每個 shuffle 都校一次
   - 記憶體測試（TA2、`complex_mem_test`）
   - EMI 設定、`emi_init3()`
3. **LK vs BL2 對照表**：旗標、步驟、log、時間、有沒有存結果、有沒有 mem test
4. 校正結果：`dramk` 分區格式（header、`pl_build_time`、每個 shuffle 的資料、checksum），什麼時候會被清掉重校
5. 冷開機 vs 熱開機 vs 燒錄後第一次開機
6. 量時間：BL2 `mt_mem_init()` 花多久
- 圖：init-01 兩條時序（LK、BL2）並排；init-02 fast calibration 判斷流程

### 3. `dram-runtime.md` 執行時

1. BL31：`dpm_init`、SPM、SSPM、MCUPM 載入；SiP 呼叫裡有沒有 DRAM 相關的
2. OP-TEE：有沒有碰 DRAMC／EMI／DVFSRC
3. U-Boot：只用不改（確認沒有 DRAM driver）
4. Linux：
   - 沒有 DVFSRC／devfreq driver → kernel 不主動調 DRAM 頻率
   - 但硬體（DVFSRC＋SPM＋DPM）可能自己調 → 用 `/dev/mem` 唯讀讀 DRAMC／DVFSRC 狀態，看閒置和忙碌時目前是哪個 shuffle
5. suspend／resume：引用 power 筆記；這次補 DRAM 的部分（DPM 存在的意義、resume 後 shuffle）
- 圖：runtime-01 誰能改 DRAM 狀態（投票 → DVFSRC → SPM／DPM → DRAMC）

### 4. `dram-layout-protect.md` 配置和保護

1. 大小怎麼傳：libdram 量出 → BL2 `BOOT_ARGUMENT` → U-Boot `gd->ram_size` → dts `/memory` → Linux `memblock`
2. 位址圖：`0x40000000` 起 8 GiB；BL2 用的 buffer、BL31（`0x54600000`）、OP-TEE（`0x43200000`）、U-Boot、kernel、dtb 的位置
3. reserved memory 三個來源：kernel dts 寫死的、BL2 mblock 經 U-Boot 加的、kernel 自己的（CMA）
4. 保護：
   - SMPU（BL31 `smpu_init`）：region 1、2 的範圍和權限
   - libdram 裡的 `emi_mpu_v3.c`（BL2 階段有沒有設）
   - OP-TEE 自己有沒有設
   - 疑點：SMPU 給 OP-TEE 10 MiB（`BL32_LIMIT 0xa00000`），dts 保留 12 MiB
- 圖：layout-01 位址圖；layout-02 大小／保留區的傳遞鏈

### 5. `dram-brief.md` 一頁總結

## 實驗清單

| 編號 | 做什麼 | 改 code？ | 燒什麼 | 還原 |
|---|---|---|---|---|
| D0 | 熱開機 UART 全程；Linux 基本資料（dmesg、`/proc/iomem`、dts `/memory`、`/reserved-memory`、meminfo） | 否 | — | — |
| D1 | **LK 的 DRAM log**：`genio-board download` → `aiot-bootrom` 載 prebuilt `lk.bin`（不燒任何分區）→ 錄 UART → `fastboot reboot` | 否 | 不燒 | — |
| D2 | 冷開機 UART（`systemctl poweroff` → `genio-board power`），看 BL2 有沒有重校 | 否 | — | — |
| D3 | 唯讀讀 `dramk` 分區（`dd if=/dev/sdc5`，只讀 header 和摘要），對照 libdram 的資料結構 | 否 | — | — |
| D4 | **BL2 加 log**：`mt_mem_init()` 前後印 timer（量時間）；初始化後只讀 dump 幾個 DRAMC／EMI／DVFSRC 暫存器；印 `BOOT_ARGUMENT` 的 mblock | 是（BL2） | `mmc0boot0` | `git checkout`、重 build、md5 對原版、燒回 |
| D5 | **BL31 加 log**：`dpm_init`／`smpu_init` 前後只讀 dump；印 SMPU region 設定；（如果找得到）目前的 shuffle | 是（BL31） | `bootloaders` | 同上，燒回 `bootloaders` |
| D6 | U-Boot：`bdinfo`、`fdt` 看 `/memory`、`/reserved-memory`（停在 U-Boot 讀，不 `saveenv`） | 否 | — | — |
| D7 | Linux `/dev/mem` 唯讀讀 DRAMC／DVFSRC 狀態：閒置、忙碌（`dd`／壓 CPU）、多次取樣 | 否 | — | — |
| D8 | suspend／resume 前後讀 D7 那些暫存器 | 否 | — | — |
| D9 | OP-TEE：`bitbake -c unpack optee-os` 讀 code；需要的話看開機 log | 否 | — | — |

- 讀暫存器前先從 libdram 原始碼找位址和欄位名稱，**只讀 DRAM 一定開著的那些（DRAMC、EMI、DVFSRC）**。SMPU 是 secure 才能讀的，**不從 Linux 讀**（可能 SError），改在 D5 從 BL31 印。
- 只要改到 BL2／BL31 就照 CLAUDE.md 流程還原。kernel 不打算改。

## 交付順序

`dram-intro.md` → `dram-init.md` → `dram-runtime.md` → `dram-layout-protect.md` → `dram-brief.md`，每份寫完回報。最後 commit、還原確認、CLAUDE.md、memory。
