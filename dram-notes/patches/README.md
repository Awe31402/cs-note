# 實驗 patch

都只加 `NOTICE()` log，只讀暫存器，不改任何設定。做完都已還原、燒回原版（md5 跟實驗前一樣）。

| 檔案 | 改哪裡 | 原始碼樹 | 燒哪個分區 | 實驗 |
|---|---|---|---|---|
| `d4-bl2-dramexp.diff` | `plat/mediatek/mt8391/bl2_plat_setup.c`：`bl2_platform_setup()` 各段印 `cntpct_el0`；`mt_mem_init()` 之後讀 DDRPHY `DVFS_STATUS`、DRAMC `SA_RESERVE`、DVFSRC 幾個字；印 `BOOT_ARGUMENT` 和 mblock | `build/tmp/work/genio_720_evk_ufs-poky-linux/bl2/2.6/git` | `mmc0boot0`（`bl2.img`） | D4 |
| `d5-bl31-dramexp.diff` | `plat/mediatek/mt8189/drivers/dpmfw/mt_dpm.c`：`dpm_init()` 前後讀 `DVFS_STATUS`；`plat/mediatek/drivers/smpu/smpu.c`：`smpu_init()` 最後讀回 region 1～4 的 SA／EA 和 enable | `…/trusted-firmware-a/2.6/git` | `bootloaders`（`fip.bin`） | D5 |

## 流程（2026-10-17）

1. `git diff > patches/…`
2. 實驗前的 md5：`bl2.img` = `4c434e208d70c0a1f7e2794339101619`，`fip.bin` = `d506658b72f6f6fc76710cf2158c442d`（`logs/d5-fip-md5-before.log`）
3. `bitbake -C compile bl2`、`bitbake -C compile trusted-firmware-a`、`bitbake rity-demo-image`
4. `strings -a` 確認 `DRAMEXP` 有編進去（`bl2.img` 7 個、`fip.bin` 4 個）
5. `genio-flash -P build/tmp/deploy/images/genio-720-evk-ufs mmc0boot0 bootloaders`
6. 實驗（`logs/d45-flash-uart.log`、`logs/d6-uboot.log`、`logs/d8-*`）
7. 兩棵樹 `git checkout -- .`（`git status` 乾淨）→ 重 build → md5 回到上面兩個值、`DRAMEXP` 0 個
8. 燒回 `mmc0boot0 bootloaders`，開機 UART 沒有 `DRAMEXP`（`logs/restore-uart.log`），`logs/restore-check.log` 確認 kernel、USB gadget、Weston、`dramk` md5

## 注意

- d5 第一次 build 失敗：`smpu.c` 沒有 include `<common/debug.h>`，`NOTICE` 沒定義。diff 裡已經補上。
- BL2 的 `cntfrq_el0` 是 0，所以 patch 印的「us」都是 0，要自己用 tick ÷ 13 MHz。
- 沒在 `mt_mem_init()` **之前**讀 DRAMC／DVFSRC：那時候可能還沒開時脈，讀了可能卡住。
