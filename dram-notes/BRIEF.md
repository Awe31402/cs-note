# DRAM 分析筆記 — Brief

2026-10-17 跟使用者一起定的（define-task 流程）。方向：**LK 和 BL2 都分析；範圍含電源／頻率＋配置／保護；LK 不改 code，其他棒可以加 log 做實驗。**

## Purpose

給使用者自己學習。讀完要能對每一棒（ROM → LK（燒錄時）→ BL2 → BL31 → OP-TEE → U-Boot → Linux）講出：

1. **初始化**：誰把 DRAM 叫醒、用哪一份 libdram、大概做哪幾步（時脈、calibration、決定大小）、花多久；LK 和 BL2 有什麼不同。
2. **執行時改狀態**：誰會改 DRAM 頻率／電壓（DVFS）、讓它進 self-refresh、在什麼時候、是硬體自己還是軟體要求。
3. **配置和保護**：DRAM 大小怎麼一棒一棒往下傳、誰切出 reserved memory、誰設 EMI MPU 保護區、保護什麼。

每一點都要有證據（原始碼行號、binary、UART log、暫存器讀值）。黑盒子的部分寫清楚「推測」。中文、PlantUML 圖。

## Situation

- 板子：Genio 720 EVK UFS 版，machine `genio-720-evk-ufs`，`rity-demo-image`，kernel 6.6.137。
- **LK** 只在燒錄時跑（ROM 載進 SRAM 的下載工具），所以要自己把 DRAM 叫醒。
  - 原始碼 `lk/lk`、`lk/libdram-lk`（HEAD `1913f22`）。燒錄時印 `libdram version: ca5e1f7`，recipe `libdram.inc` 的 `SRCREV` 也是 `ca5e1f7` → 讀 code 時對 `ca5e1f7` 那一版。
- **BL2** 連的是 `libdram-prebuilt`（沒有原始碼）。可能跟 LK 同一套 code，還沒證實。
- **BL31** 會載 SPM、DPM 兩個韌體，都只有 binary。
- 已經做過、要引用不重做：`notes/power-notes/suspend-resume.md`（suspend flag、SPM 流程）、`notes/ufs-emmc-notes/lk-flash-*.md`（LK 流程、`mt_mem_init()` 位置、版本對不上）、CLAUDE.md（BL2 在 `mt_mem_init()` 前沒有 DRAM）。
- **沒有 DRAM／EMI 的 TRM**。暫存器意義大多要從原始碼的名字推。
- 冷開機和熱開機可能不同（fast calibration：校正結果存起來下次用）。要查證存哪、熱開機會不會跳過。
- 筆記放 `notes/dram-notes/`，自己的 git repo。

## Inputs

主要（可以直接當證據）：
1. 實機輸出：UART log（燒錄時 LK、開機時 BL2／BL31／U-Boot）、Linux `/dev/mem` 唯讀讀值、sysfs、debugfs、`/proc/iomem`、`/sys/firmware/devicetree`、改 code 加的 log。**先存 `logs/` 再引用。**
2. 這次 build 實際用的原始碼和 binary：
   - LK：`lk/lk`、`lk/libdram-lk`（看 `ca5e1f7`）
   - BL2／BL31：`build/tmp/work/genio_720_evk_ufs-poky-linux/{bl2,trusted-firmware-a}/2.6/git`＋`libdram-prebuilt` 的 `.a`
   - OP-TEE（先 `bitbake -c unpack optee-os`）、U-Boot、kernel
   - deploy 出來的 binary（`nm`、`strings`、反組譯）

次要（解釋用）：
3. 已有筆記（power、lk-flash、ufs-emmc）
4. JEDEC LPDDR4X／LPDDR5 的公開概念（training、self-refresh、MR）、上游 kernel mtk-dvfsrc 文件

不用：
- 憑感覺或用其他 MTK 晶片的經驗猜暫存器值

找不到就標「**未知（無 TRM）**」或「**推測：…（理由）**」，不編數字。DRAM 型號、容量要從 log 或 code 查證。

## Limits

不能：
- **LK 不改**：不 build 自己的 `lk.bin` 拿來燒，只錄它的 log。
- **不寫** DRAMC／EMI／DVFSRC／EMI MPU 暫存器（`/dev/mem` 只用唯讀 mmap）；kernel 有「強制調頻」的 sysfs／debugfs 也不用。
- 加 log 的 patch **只讀、只印**：不改 libdram 參數、頻率表、時序，不跳過校正。
- 校正資料（如果找到存的地方）只讀，不清掉逼它重校。

照舊：
- 改 code 實驗：`git diff` 存 `patches/` → 實驗 → 還原 → 重 build → 燒回原版；改 kernel 要整顆燒。
- 不寫 UFS descriptor、Linux 跑著不拔電；冷開機用 `systemctl poweroff` → `genio-board power`。
- suspend 可以做，只為觀察，用 `systemd-run` 背景跑。

範圍外：
- 調效能／省電建議、memtest、壓力測試
- 其他板子（norboot、eMMC 只對照 code）
- GPU／顯示的記憶體管理（IOMMU、SMI 只在跟 EMI 保護有關時提）

安全：Wi-Fi AP 密碼不寫進筆記、log、git。

風險：`/dev/mem` 讀到沒時脈的區塊可能 bus hang → 先從 dts／clock 確認有電有時脈；卡死就重開機。

## Done-when

- 位置：`notes/dram-notes/`（自己的 git repo）。
- 先 `BRIEF.md` → `outline.md`（章節、實驗清單 D1…、先做的查證）。使用者 2026-10-17 用 `/goal` 指示：**大綱寫完直接做完整個任務**。
- 正文：
  1. `dram-intro.md`：DRAM、EMI、DRAMC、DVFSRC、SPM、DPM 是什麼；本板 DRAM 型號／容量
  2. `dram-init.md`：LK（燒錄）vs BL2（開機）的步驟、時間、冷熱開機差別、校正結果存哪
  3. `dram-runtime.md`：每一棒（BL31、OP-TEE、U-Boot、kernel）有沒有改頻率／電壓、self-refresh；suspend 時
  4. `dram-layout-protect.md`：大小怎麼傳、reserved memory、EMI MPU
  5. `dram-brief.md`：一頁總結（總表、發現、實驗、坑、待深挖、還原）
- 每份有 PlantUML 圖（`.puml`／`.svg`／`.png`，筆記連 `.png`）；`logs/`、`patches/`、`scripts/`。
- 收尾：git commit；板子還原並確認（沒實驗字串、沒 `-dirty`、adb／Weston 正常）；CLAUDE.md 加一段、memory 加一條。
