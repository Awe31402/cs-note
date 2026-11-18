# B.4 Linux Productivity

> 原書 p.784.e161–784.e165（Appendix B: Hitchhiker's Guide to Linux）
> 涵蓋 B.4.1 Multiple Terminal Windows、B.4.2 Text Editors、B.4.3 The Linux File System、B.4.4 Environment Variables、B.4.5 Web Browsing、B.4.6 Document Viewing and Printing、B.4.7 Installing Packages

## 這節在講什麼

七個小節的工作環境建議。其中三個對做 RTL 專案特別有用：**`tmux`**（ssh 斷線也不會殺掉工作）、**VSCode + Remote-SSH**（書上明說是他們偏好的方式）、**環境變數**（`$PATH`、`$LM_LICENSE_FILE`——這正是第 23 章 Vivado 授權設定的機制）。

## 關鍵點

### tmux：ssh 斷線的保險

> "When working over ssh without a graphical user interface (GUI) or over a slow connection, tmux provides a multi-window terminal environment in a single session. tmux also provides persistent terminal sessions which can continue executing commands after an ssh session has disconnected."
> — p.784.e162

中文解釋：**這解決了 B.1 提到的問題**——遠端桌面（VNC/X2Go）可以暫停後接回，但純 `ssh` 一斷線，跑到一半的程式就被殺掉。`tmux` 讓 `ssh` 也有同樣的能力。

對這本書的工作流程來說這很關鍵：第 21 章的分支預測器掃描要跑半天、第 22 章的 Linux 開機模擬要 20 小時——**中途網路抖一下就重來是不可接受的**。

常用操作：
```bash
tmux new -s <sessionname>       # 開新工作階段
tmux -2 attach -t <sessionname> # 接回既有的
tmux kill-session -t <name>     # 從外面殺掉
```
在工作階段裡：`Ctrl-b d` 離開（工作繼續跑）、`Ctrl-b c` 開新視窗、`Ctrl-b n`/`p` 切換視窗、`exit` 結束。

### 多開終端機的實務建議

> "It's usually helpful to have multiple terminal windows open when you are working on a complex project. Keep one terminal in each directory where you are working so you aren't constantly changing directories back and forth."
> — p.784.e161

中文解釋：**「每個工作目錄開一個終端機」**——聽起來瑣碎，但在 `cvw` 這種有 `src/`、`sim/`、`tests/`、`synthDC/`、`fpga/` 的專案裡，省下的 `cd` 次數很可觀。

### 編輯器：書上的實際偏好

> "Linux has many text editors, with vim and emacs being the most widely used console-based editors. Each has a set of basic functionality, and each is rather cryptic for new users."
> — p.784.e162

中文解釋：`vim` 的最小生存指令（值得記，因為在任何伺服器上都有）：
| 按鍵 | 作用 |
| --- | --- |
| `i` | 進入 insert mode（可以打字） |
| `Esc` | 回到 command mode |
| `dd` | 刪掉整行 |
| `p` | 貼上剛剛刪的 |
| `u` | 復原 |
| `/text` | 搜尋 |
| `:w` | 存檔 |
| `:q!` | 不存檔離開 |
| `ZZ` | 存檔並離開 |

書上很誠實：「Expect to use vim for several days before you become comfortable with it.」

**但書上自己不用 vim**：

> "Our preferred editing method for large projects, as long as a good network connection is available, is to run Microsoft's Visual Studio Code (also called VSCode, pronounced 'V S code') natively on your laptop with an SSH connection to the server. VSCode is free and runs on Windows, Mac, and Linux. It is presently used by about 70% of developers."
> — p.784.e162

中文解釋：**這是最實用的建議**。運作方式是：編輯器跑在筆電上（所以反應快），但透過 SSH 直接編輯伺服器上的檔案（所以不用同步）。

推薦的擴充：
- **Remote - SSH**（必裝，就是上面那個機制）
- **Verilog-HDL/SystemVerilog/Bluespec**（語法highlight）
- **GitLens**（追蹤每一行是誰什麼時候改的——對附錄 C 的 Git 工作流程很有幫助）

設定方式：左下角藍色的 `><` 圖示 → Connect to Host → 輸入主機名 → File → Open Folder → 輸入倉庫路徑（例如 `/home/lynn/cvw`）。

書上也說了為什麼不推薦在伺服器上跑 GUI 編輯器：「They are not particularly responsive over an x2go connection.」

### Linux 檔案系統結構

> "Most Linux distributions are organized according to the Filesystem Hierarchy Standard summarized in Table B.7."
> — p.784.e163

中文解釋：Table B.7 裡和硬體開發相關的幾個：

| 目錄 | 內容 | 為什麼相關 |
| --- | --- | --- |
| `/bin`、`/sbin`、`/lib` | 開機必需的執行檔與函式庫 | **第 22 章的 BusyBox rootfs 就是這個結構** |
| `/boot` | GRUB、`vmlinuz`、`initramfs` | **第 22 章講的 kernel 映像格式** |
| `/etc` | 設定檔（如 `/etc/passwd`） | **第 22 章改的 `/etc/inittab`** |
| `/usr/bin`、`/usr/local/bin`、`/opt` | 大部分指令／手動安裝的工具 | **`$RISCV` 預設是 `/opt/riscv`** |
| `/dev` | 裝置檔 | **第 23 章的 `/dev/ttyUSB0`、`/dev/sd<letter>`** |
| `/tmp` | 暫存檔，重開機會清掉 | |

**這張表和第 22 章可以互相對照**——Fig. 22.1 那個 BusyBox rootfs 的目錄樹，就是這個標準的最小實作。

書上也提到：「Linux systems may have other directories such as `/cad`, `/courses`, or `/proj` for sharing software and files across users.」——**共用 EDA 工具通常裝在 `/cad`**。

### 環境變數：`$PATH` 與授權伺服器

> "Linux uses environment variables to configure how the system operates. Environment variable names start with $ and are usually in upper case. One of the most important variables is $PATH, which defines an ordered list of directories in which Linux searches for commands."
> — p.784.e164

中文解釋：`echo $PATH` 看目前的搜尋路徑，`export PATH=/cad/verilator/bin:$PATH` 把新工具加到**最前面**（所以會優先被找到）。

**注意 `:$PATH` 這個慣用寫法**——不寫的話會把原本的路徑全部蓋掉，然後連 `ls` 都找不到。

> "Nobody wants to have to modify the path every time you log in, so paths are generally set in a configuration file. Depending on your local configuration, the .profile or .bash_profile file will be sourced (executed) when you open a terminal. ... Better yet, if the configuration is needed by many users, add a line to .profile or .bash_profile to source a shared configuration file: source /cad/scripts/setups/S21/riscv-setup"
> — p.784.e164

中文解釋：**「共用一個設定檔」是團隊環境的標準做法**——工具升級時只要改一個檔案，所有人的環境一起更新。

其他兩個重要變數：
- **`LD_LIBRARY_PATH`**——像 `PATH` 但用於共享函式庫（`.so` 檔）。工具跑不起來說找不到某個 `.so`，通常就是這個沒設對。
- **`LM_LICENSE_FILE`**——指向授權管理器。**第 23 章的 `XILINXD_LICENSE_FILE=<port>@<server>` 就是同一個機制**，只是 Xilinx 用自己的變數名。

### 其他三個小節

> "Web browsing is generally less responsive from a Linux server than from your local computer. If you need to open a browser to download a file, firefox is commonly available. Or download the file with a local browser to your local machine and rsync it to the server."
> — p.784.e164–e165

中文解釋：**用本機瀏覽器下載 + `rsync` 上傳**比在伺服器上開瀏覽器快得多。

> "Printing from Linux can be finicky. If your server has printers set up, follow your site's documentation. Otherwise, your best bet may be to generate plain text or print to PDF and then rsync the text/PDF file to your local computer for printing."
> — p.784.e165

中文解釋：看 PDF 用 `evince` 或 `okular`。列印的建議一樣是「產生檔案再傳回本機」。

> "The primary package management tool for RedHat is dnf (formerly yum) and for Ubuntu is apt. One software package often depends on others, so the package manger handles dependencies, but this can be imperfect in Linux. Not all packages have an installer. Installing new software under Linux is frequently an exercise in frustration."
> — p.784.e165

中文解釋：書上對安裝套件的評價很直白——「經常是一場挫折的練習」。而且：

> "Usually you need superuser (root) permission to install new packages (with the sudo command). Such permission also gives you the power to accidentally break your server. ... Don't be surprised if a sysadmin is reluctant to install new software on a production server, however."
> — p.784.e165

中文解釋：**`sudo` 的權力同時是「安裝軟體」和「弄壞伺服器」的權力**。在共用伺服器上通常拿不到，要請系統管理員。這也是為什麼 Wally 的安裝腳本（`wally-tool-chain-install.sh`）會在需要 sudo 時提示輸入密碼（第 22.2.1 節）。

## 名詞表

| 名詞 | 意思 |
| --- | --- |
| `tmux` | 多視窗、可斷線重連的終端機工作階段管理器 |
| persistent session | 持久工作階段，ssh 斷線後程式繼續跑 |
| `vim` / `emacs` | 兩大主控台文字編輯器 |
| command mode / insert mode | vim 的兩種模式 |
| VSCode / Remote-SSH | 本機編輯器透過 SSH 直接編伺服器檔案 |
| GitLens | 顯示每行程式碼修改歷史的 VSCode 擴充 |
| Filesystem Hierarchy Standard | Linux 目錄結構標準 |
| environment variable | 環境變數 |
| `$PATH` / `$LD_LIBRARY_PATH` | 指令／共享函式庫的搜尋路徑 |
| `$LM_LICENSE_FILE` | 授權管理器位置 |
| `source` | 在當前 shell 執行一個腳本（使變數設定生效） |
| `.profile` / `.bash_profile` | 開終端機時自動執行的設定檔 |
| `dnf` / `apt` | RedHat／Ubuntu 的套件管理器 |
| superuser / root / `sudo` | 超級使用者權限 |

## 所以呢

四個實際會改變工作方式的建議：**用 `tmux` 保護長時間的模擬**、**用 VSCode + Remote-SSH 編輯（本機反應速度 + 伺服器檔案）**、**環境變數集中在共用設定檔**、**下載與列印都在本機做，用 `rsync` 搬**。

下一節 B.5 講 shell 腳本、Python、C、以及 Makefile——其中 Makefile 是這本書從頭到尾都在用的東西。
