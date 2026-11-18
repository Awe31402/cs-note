# B.5 Scripting and Programming

> 原書 p.784.e165–784.e169（Appendix B: Hitchhiker's Guide to Linux）
> 涵蓋 B.5.1 Shell Scripts、B.5.2 Python、B.5.3 C、B.5.4 Makefiles

## 這節在講什麼

四種「把重複工作自動化」的方式。前三個（shell、Python、C）對寫過程式的人沒什麼新東西，**但 B.5.4 的 Makefile 值得認真讀**——這本書從第 3 章到第 23 章，幾乎每個流程都是 `make` 驅動的（`make` 建測試、`make synth` 合成、`make run` 跑 benchmark、`make Image` 建 Linux、`make` 產生 FPGA bitstream）。

## 關鍵點

### shell script：兩種執行方式的差別

> "A shell script is a text file with a series of commands that get executed as if you typed them at the command line. It conventionally may have a .sh suffix."
> — p.784.e165

中文解釋：兩種跑法：

```bash
$ source hello.sh      # 在當前 shell 執行
```
或者

> "If you don't want to type source to invoke a script, make the script executable using chmod and run it directly: $ chmod 755 hello.sh / $ ./hello.sh"
> — p.784.e166

中文解釋：**`source` 和 `./` 有一個重要差別**（書上沒明說但值得知道）：`source` 在**當前 shell** 執行，所以腳本裡設的環境變數會留下來；`./` 開一個**子 shell**，變數設完就沒了。這就是為什麼 B.4 講的設定檔要用 `source`（`source /cad/scripts/setups/...`）而不是直接執行。

### 為什麼要打 `./`

> "For security reasons, your current directory (.) shouldn't be in your $PATH. Therefore, to run executable programs in the local directory, prefix them with ./ This prevents a malicious user from creating a file in their home directory with the same name as a standard system command (e.g., /home/lynn/ls) and then tricking the system administrator into running ls in the user's directory and inadvertently running the malicious program."
> — p.784.e166

中文解釋：**這是一個具體的安全機制，值得理解**。如果 `.` 在 `$PATH` 裡，攻擊者只要在自己目錄放一個叫 `ls` 的惡意程式，再騙管理員 `cd` 進來打 `ls`，就中招了。

所以 Linux 的設計是：**當前目錄永遠不在 `$PATH` 裡，要執行就得明確寫 `./`**。這個小麻煩換來的是「你永遠知道自己在執行哪一個程式」。

### 把腳本放哪裡

> "Another good place to keep personal scripts is in ~/bin, which you can add to your path, or ~/.local/bin, which may already be in your path."
> — p.784.e166

中文解釋：**放進 `~/bin` 並加到 `$PATH` 之後就能像系統指令一樣直接用**。這正是 Wally 的做法——附錄 A 的 `wsim`、`regression-wally`、`lint-wally` 都是 `cvw/bin/` 底下的腳本，安裝時把那個目錄加進 `$PATH`。

### 一個實用的腳本範例

> "Scripts are handy to automate tedious tasks. For example, suppose you frequently rsync'd files between two computers in directories that were long and tedious to type."
> — p.784.e166

中文解釋：書上的例子把 B.3 那個長長的 `rsync` 指令包成一行：

```bash
$ cat > ~/bin/mysync
#!/usr/bin/bash
rsync -auv --info=progress2 src/${1} lynn@vlsi.hmc.edu/src
<Ctrl-d>
$ chmod 755 ~/bin/mysync
$ mysync foo.sv
```

**兩個重點**：
1. **`${1}` 是第一個命令列參數**（`${2}` 是第二個，以此類推）
2. **`#!/usr/bin/bash` 是 shebang**——告訴系統用哪個直譯器跑這個檔案

`chmod 755` 的意思（回顧 B.2）：擁有者可讀寫執行、其他人可讀可執行。書上邊欄補充：`chmod +x` 只加執行權限，`+r`/`+w` 同理。

### Python 與 shebang 的可攜性技巧

> "Alternatively, you can make a script in any language executable by adding a special comment starting with #! on the first line telling Linux which program to use to run the script."
> — p.784.e167

中文解釋：

```python
#! /usr/bin/env python
print("Hello World!")
```

**`/usr/bin/env python` 比 `/usr/bin/python` 好**，書上邊欄解釋了原因：

> "/usr/bin/env finds the first executable that matches the specified name (e.g., python) in your $PATH. This makes scripts more portable because they will work even if the Python executable is in a different place on different servers."
> — p.784.e167（邊欄）

中文解釋：**寫死路徑的腳本換一台機器就壞了**，`env` 會去 `$PATH` 裡找。這是寫可攜腳本的標準做法。

Wally 的 `coremark_sweep.py`、`embench_arch_sweep.py`、`CacheSim.py`、`parseHPMC.py`、`proberange` 都是 Python 腳本——**這本書裡「分析資料、產生表格、畫圖」的工作全是 Python 做的**。

### C：為什麼硬體工作要用 C

> "C is a good programming language for system programming and working with hardware because pointers allow you to access memory explicitly."
> — p.784.e167

中文解釋：**「指標讓你能明確存取記憶體」**——這正是第 20 章寫周邊驅動程式的需求（`volatile uint8_t *UART_THR = (uint8_t*)0x10000000;`）。

```bash
$ gcc -o hello hello.c
$ ./hello
```

（推薦書籍：[C17] *C Programming* 免費線上書、Kernighan & Ritchie 的 *The C Programming Language*。）

### Makefile：這本書的核心工具

> "Large programs are composed of many files and often require specifying command line options for the compiler. If you need to recompile a program, it's a hassle to try to remember exactly what command is needed to compile. ... Moreover, when only one of the files has been changed, it would be faster to just recompile and relink that one file rather than rebuilding the entire program."
> — p.784.e167

中文解釋：**Makefile 解決兩個問題——「記住怎麼編譯」和「只重建需要重建的部分」**。第二個對大型專案是關鍵：`cvw` 的完整建置要好幾分鐘，改一個檔案不該重來一次。

**規則的格式**：

> "The Makefile consists of a series of rules in the form: target : dependencies / [tab]command"
> — p.784.e168

中文解釋：

> "To build target, make checks if any of the dependencies have changed since target has last changed. If they have, it issues the command to rebuild target."
> — p.784.e168

中文解釋：**核心機制是比較檔案的修改時間**。`target` 比它的 `dependencies` 舊 → 重建。

**一個一定會踩的坑**（書上用邊欄特別警告）：

> "Be sure to indent the command by pressing tab. Make will fail with a confusing 'missing separator' message if you indent with spaces."
> — p.784.e168（邊欄）

中文解釋：**必須用 Tab，不能用空格**。錯誤訊息 `missing separator` 完全看不出是這個原因。這是 Makefile 最惡名昭彰的設計缺陷。

**完整範例**：

```makefile
objects = a.o b.o c.o

example : $(objects)
	gcc -o example $(objects)

a.o: a.c x.h y.h
	gcc -c a.c
b.o: b.c x.h
	gcc -c b.c
c.o: c.c y.h
	gcc -c c.c

clean:
	rm example $(objects)
```

**四個要點**：

1. **`objects = a.o b.o c.o` 是變數**，用 `$(objects)` 引用——避免在多處重複列出檔名。

2. **第一條規則是預設目標**：
> "The first rule is the default goal. Hence, typing make has a goal of building example."
> — p.784.e168

3. **相依關係會遞迴檢查**：`example` 依賴 `a.o`/`b.o`/`c.o`，所以先檢查它們；`a.o` 依賴 `a.c`/`x.h`/`y.h`，任何一個比 `a.o` 新就重編。

4. **精確的相依性帶來精確的重建**：
> "If we changed y.h or run touch y.h (which changes the modification date to now) and then make again, a.o, c.o, and example are remade, but b.o is already up to date because it doesn't depend on y.h."
> — p.784.e168

中文解釋：**這就是 Makefile 的價值**——`b.o` 不依賴 `y.h`，所以不重編。這個判斷完全來自你寫的相依性清單，寫錯了就會漏編（更糟）或多編（只是慢）。

**`clean` 是一個慣例目標**：
> "Running make clean executes the clean goal instead of the default goal. In this case, it removes all the compiled output files, leaving only the source files."
> — p.784.e169

中文解釋：`make clean` 是業界慣例。第 21 章的 `make clean && make spike`、第 22 章的 `make clean` 都是這個。

**注意 `clean` 沒有相依性**——它不是真的要「建立一個叫 clean 的檔案」，只是借用 Makefile 的語法來執行一組指令。這種叫 phony target。

### 回頭看這本書用到的 make

現在回頭看前面章節的指令就很清楚了：
| 指令 | 章節 | 在做什麼 |
| --- | --- | --- |
| `make --jobs` | 附錄 A | 建所有測試（`--jobs` 平行編譯） |
| `make run` | 第 21 章 | 編譯並跑 benchmark |
| `make synth DESIGN=... TECH=... FREQ=...` | 第 6 章 | 用變數控制合成參數 |
| `make Image BUILDROOT=...` | 第 22 章 | 建 Linux kernel |
| `make ArtyA7` | 第 23 章 | 建特定板子的 bitstream |

**`make <變數>=<值>` 是從命令列覆寫 Makefile 變數的標準寫法**——第 6 章的 PPA 掃描和第 23 章的多板子支援都靠這個。

## 名詞表

| 名詞 | 意思 |
| --- | --- |
| shell script | 一連串指令的文字檔，慣例副檔名 `.sh` |
| `source` vs `./` | 在當前 shell 執行（變數留下）／開子 shell 執行 |
| shebang (`#!`) | 第一行指定直譯器 |
| `/usr/bin/env` | 從 `$PATH` 找直譯器，提高可攜性 |
| `${1}` | 第一個命令列參數 |
| `~/bin` | 放個人腳本的慣例位置 |
| Makefile | 描述建置規則的檔案 |
| target / dependencies / command | 目標／相依項／建置指令 |
| default goal | 第一條規則的目標，`make` 不帶參數時建它 |
| phony target | 不對應實際檔案的目標（如 `clean`） |
| `touch` | 把檔案的修改時間改成現在 |
| `--jobs` | make 的平行建置選項 |

## 所以呢

四種自動化方式各有定位：**shell script** 包裝重複的指令、**Python** 做資料分析與繪圖（這本書的 benchmark 分析全靠它）、**C** 做需要直接碰記憶體的工作（周邊驅動）、**Makefile** 管理建置相依性。

Makefile 的三件事值得記住：**指令前面必須是 Tab**、**第一條規則是預設目標**、**相依性寫得多精確，重建就多精確**。這本書從頭到尾的 `make` 指令，現在都看得懂在做什麼了。

附錄 B 到此結束。下一個附錄 C 講 Git——B.2 提過「Linux 沒有還原指令，除非你用版本控制」，那個保險就在下一章。
