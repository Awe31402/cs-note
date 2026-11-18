# B.3 More Handy Commands

> 原書 p.784.e154–784.e161（Appendix B: Hitchhiker's Guide to Linux）
> 涵蓋 B.3.1 Cast of Characters、B.3.2 Standard Input and Output、B.3.3 Common Commands、B.3.4 Working With Processes、B.3.5 Compressing, Bundling, and Transferring Files

## 這節在講什麼

四張表（B.2–B.6）的常用指令。對已經在用命令列的人來說，值得挑出來看的只有幾個地方：**串流與管線的組合方式**、**`grep`/`find`/`diff` 這幾個在大型 RTL 專案裡天天用的工具**、**行程管理**、以及 **`rsync`**（在伺服器與本機之間搬檔案的正確方式）。

## 關鍵點

### 三個特殊字元

> "* is a wildcard that matches any set of 0 or more characters."
> — p.784.e157

中文解釋：Table B.2 的三個字元：
- **`*`** 萬用字元：`ls *.sv` 列出所有 SystemVerilog 檔。`rm -rf *` 清空當前目錄——書上標了「Use with caution!」
- **`.`** 當前目錄：`mv tutorial/README .` 把檔案搬到當前目錄
- **`&`** 背景執行：`vsim &` 開模擬器但不佔住終端機（第 23 章的 `vivado &` 就是這樣用）

### 幾個省時間的鍵盤操作

> "Pressing the TAB key will autocomplete what you are typing, which is helpful if you are typing long filenames. Pressing the up arrow followed by return will replay a previous command. ... Pressing Ctrl-a or Ctrl-e will bring you to the beginning or end of the current command."
> — p.784.e157

中文解釋：Tab 自動補完、上下鍵翻歷史、左右鍵編輯、Ctrl-a/Ctrl-e 跳到行首/行尾。

> "Filenames starting with a . (e.g., .bashrc) are called dotfiles. They are usually used for application configuration and are not displayed by ls. Use ls -a (list all) to show dotfiles. Combining the command options after the dash does them all. For example, ls -al shows the long listing for all files."
> — p.784.e157

中文解釋：**dotfile 是設定檔的慣例**——`.bashrc`、`.profile`、`.gitignore`。`ls` 預設看不到，要 `ls -a`。選項可以合併寫（`-al` = `-a -l`）。

### 三個串流與重導向

> "Linux commands communicate with three streams called standard input (stdin), standard output (stdout), and standard error (stderr)."
> — p.784.e157

中文解釋：Table B.3 的三種重導向：

| 符號 | 作用 | 例子 |
| --- | --- | --- |
| `>` | stdout 導到檔案 | `ls *.sv > svfilenames` |
| `\|` | 一個指令的 stdout 接到下一個的 stdin | `ls -l \| less` |
| `tee` | **同時**送到螢幕和檔案 | `ls *.sv \| tee svfilenames` |

**`tee` 特別實用**：

> "tee pipes its stdin to both stdout and one or more files. This is helpful to see the output of a tool while also generating a log file to review later."
> — p.784.e158

中文解釋：跑合成或長時間模擬時，想即時看進度**又**想留下 log——這就是 `tee` 的用途。

**一個容易踩的坑**：`>` 只導 stdout，**stderr 還是會印到螢幕**。這其實常常是好事（錯誤訊息不會被藏進 log 檔），但要做完整記錄時要用 `2>&1`。

### 大型專案裡真正天天用的四個指令

Table B.4 裡最值得記的是這幾個：

> "grep (global regular expression print) searches files for a string and prints where it is found in the specified files. The string may be simple text, or may be a regular expression"
> — p.784.e159

中文解釋：在 `cvw` 這種幾萬行的 RTL 專案裡，`grep` 是找東西的主力：
```bash
grep XLEN *.sv *.vh     # 在指定副檔名裡找
grep -r XLEN            # 遞迴搜尋所有子目錄
```
**`grep -r` 是讀懂陌生程式碼的第一個工具**——想知道某個訊號在哪裡被驅動、某個參數在哪裡被用到，直接搜。

> "find finds all the files in the specified directory (including its recursive subdirectories) matching a particular criterion."
> — p.784.e159

中文解釋：`find . -name *.sv` 列出所有 SystemVerilog 檔的路徑。**`grep` 找內容，`find` 找檔名**。

> "diff compares two files and prints the differences."
> — p.784.e159

中文解釋：驗證時的基本工具——把模擬輸出和參考輸出 `diff` 一下就知道對不對。第 13 章的 RISCOF 流程本質上就是自動化的 `diff`。

> "ln -s creates a symbolic link to a file. A symbolic link is like a shortcut in Windows or an alias on MacOS."
> — p.784.e159

中文解釋：**`ln -s` 就是第 22 章 BusyBox 的原理**——`/bin/ls`、`/bin/cp` 全都是指向 `/bin/busybox` 的符號連結。`ls -l` 會顯示成 `README4 -> tutorial/README`。

其他：`man`（手冊）、`which`（找指令的路徑）、`sort`、`wc`（算行數/字數/位元組數）、`du -h`（磁碟用量，`-h` 用人看得懂的 KiB/MiB/GiB）。

### 行程管理：跑模擬跑到失控時

> "Typing Ctrl-c will usually terminate a process that is running. If that doesn't work, open another terminal, use ps to find the PID of the process, and then use kill <PID> to kill it."
> — p.784.e159

中文解釋：這是一個**完整的急救流程**，值得背下來：
1. `Ctrl-c` — 大部分情況夠用
2. 沒用 → 開另一個終端機，`ps -u` 找 PID
3. `kill <PID>` → 還不死就 `kill -9 <PID>`（強制）
4. 要殺掉所有同名的 → `killall vsim`

跑第 21–23 章那些幾小時的模擬時，這些遲早會用到。

> "top displays the processes with the greatest load on the computer. htop gives more information on multiprocessor systems."
> — p.784.e160

中文解釋：`top`/`htop` 看誰在吃 CPU。在共用伺服器上很重要——要確認是自己的工作卡住，還是別人佔滿了機器。

**背景/前景切換**：

> "Typing Ctrl-z puts a process to sleep. Then type bg to restart the process in the background (running concurrently with the terminal) or fg to restart the process in the foreground ... The jobs command will list an integer associated with each process. bg and fg followed by % and the integer restarts the specific process; for example, bg %2 restarts process 2 in the background."
> — p.784.e160

中文解釋：**忘記加 `&` 的補救方法**——`Ctrl-z` 暫停，`bg` 丟到背景。`jobs` 列出所有背景工作。

**歷史指令的快捷**（Table B.5）：
- `history` — 列出用過的指令（帶編號）
- `!!` — 重複上一個指令
- `!c` — 重複最後一個 c 開頭的指令
- `!103` — 重複第 103 號指令

### 壓縮、打包、傳輸

> "tar ('tape archive') creates a tarball, a single file containing all the files and subdirectories in a directory, making it easier to move directories between systems."
> — p.784.e161

中文解釋：**`tar` 的選項字母有記憶法**：
- `-c` **C**reate（建立）
- `-x` e**X**tract（解開）
- `-v` **V**erbose（顯示過程）
- `-f` **F**ilename（指定檔名）
- `-z` g**Z**ip（順便壓縮）

所以 `tar -czvf archive.tar.gz tutorial` 是「建立+壓縮+顯示+指定檔名」，解開就把 `c` 換成 `x`。

> "gzip compresses files. It works best for text files with repetitive or structured content, achieving compression of around 50% for a plain text novel to 90+% for more repetitive files."
> — p.784.e161

中文解釋：**RTL 原始碼和 log 檔是高度重複的文字**，壓縮率接近 90%。第 22 章那個 51 GiB 的 `boottrace.log` 就是典型例子。

**`rsync` 是伺服器工作最重要的一個**：

> "rsync is a flexible tool to copy/synchronize files or directories. Directories can be remote or local, and rsync can efficiently sync only modified files or files that do not exist at the destination."
> — p.784.e161

中文解釋：**`rsync` 比 `scp` 好在「只傳改過的部分」**。用法：
```bash
rsync lynn@vlsi.hmc.edu:archive.tar.gz .          # 從遠端抓
rsync -auv --info=progress2 src/mmu lynn@vlsi.hmc.edu:src/new   # 推上去
```
選項：`-a`（保留權限等屬性）、`-u`（只傳比較新的）、`-v`(verbose)、`--info=progress2`（進度條）。

**實務上的用途**：在筆電上用 VSCode 編輯（B.4 會講），改完 `rsync` 到伺服器跑合成。或者第 23 章講的「用本機瀏覽器下載檔案，再 `rsync` 到伺服器」。

## 名詞表

| 名詞 | 意思 |
| --- | --- |
| wildcard | 萬用字元 `*` |
| dotfile | 以 `.` 開頭的設定檔，`ls` 預設不顯示 |
| stdin / stdout / stderr | 標準輸入／輸出／錯誤三個串流 |
| redirect (`>`) / pipe (`\|`) | 導向檔案／接到下一個指令 |
| `tee` | 同時輸出到螢幕與檔案 |
| regular expression | 正規表示式 |
| symbolic link | 符號連結（BusyBox 的原理） |
| PID | process ID，行程編號 |
| `kill -9` | 強制終止（SIGKILL） |
| foreground / background | 前景（佔住終端機）／背景 |
| tarball | `tar` 打包出來的單一檔案 |
| `rsync` | 只同步差異的檔案傳輸工具 |
| `du -h` | 磁碟用量，人類可讀單位 |

## 所以呢

真正值得記的只有五個：**`grep -r`**（讀懂陌生程式碼的第一工具）、**`\| tee`**（邊看邊留 log）、**`ps` + `kill -9`**（模擬跑失控時的急救）、**`Ctrl-z` + `bg`**（忘記加 `&` 的補救）、**`rsync -auv`**（本機與伺服器之間搬檔案）。

下一節 B.4 講生產力工具：`tmux`、編輯器、檔案系統結構、環境變數。
