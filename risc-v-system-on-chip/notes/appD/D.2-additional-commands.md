# D.2 Additional Commands

> 原書 p.784.e188–784.e192（Appendix D: Tcl Book of Armaments）
> 涵蓋 D.2.1 Lists、D.2.2 Outputting Data to the Screen、D.2.3 File, Glob, and Other File-Related Commands、D.2.4 Control Flow、D.2.5 Procedures、D.2.6 Running Tcl Scripts、D.2.7 Aliases、D.2.8 Exiting Tcl
> 註：章首目錄作「D.2 Advanced Commands」，正文標題為「D.2 ADDITIONAL COMMANDS」

## 這節在講什麼

寫出一個能用的 EDA 腳本所需的其餘語法：清單、輸出、檔案、控制流程、程序、別名。

對寫過程式的人來說概念都不新，**值得注意的只有 Tcl 與其他語言語法不同的地方**——特別是 `for` 迴圈的四段大括號、以及「控制流程的條件必須用大括號包住」這個規則。

## 關鍵點

### 清單：EDA 腳本的主要資料型別

> "Tcl includes lists, which are an ordered group of elements where each element can be a string or another list. Lists group items such as a set of cells or report file names."
> — p.784.e188

中文解釋：**「a set of cells or report file names」道出了 EDA 裡的實際用途**——合成工具回傳的東西幾乎都是清單（所有暫存器、所有違反時序的路徑、所有符合某個 pattern 的訊號）。

```tcl
% set a 5
% set D_pins "I1/FF3/D I1/FF4/D I1/FF5/D"
% set b [list a 1 $a [list $a z]]
a 1 5 {5 z}
```

**注意兩種建立清單的方式**：
- 直接用引號寫成空白分隔的字串（`D_pins`）——因為清單本質上就是字串
- 用 `list` 命令（會正確處理巢狀，內層清單顯示成 `{5 z}`）

取元素用 `lindex`，**從 0 開始編號**：

```tcl
% lindex $D_pins 1
I1/FF4/D
```

Table D.2 的其他清單命令：`concat`（串接）、`join`（合成字串）、`lappend`（附加）、`linsert`（插入）、`lsort`（排序）、`split`（字串切成清單）。

**`split` 在解析報告檔時很常用**——把一行文字切成欄位。

### 輸出：`echo` 與 `puts` 的差別

> "The echo and puts commands output data to the screen. echo prints its argument to the console window. puts prints its argument to the standard output, which may or may not be the same as the screen."
> — p.784.e189

中文解釋：**這個區別在 EDA 工具裡有實際影響**——`puts` 寫到標準輸出（可能被重導向到 log 檔），`echo` 直接寫到工具的 console 視窗。想確保訊息出現在工具介面上用 `echo`，想讓它進 log 檔用 `puts`。

（`echo` 不是標準 Tcl 命令，是 Synopsys 等工具自己加的——這正是 D.1 講的「extensible」。）

### 檔案操作

> "To retrieve information about a file, use the file command, which has the following syntax: file option argument argument ... For example, file exists filename returns 1 when that file exists; otherwise, it returns 0. The glob command generates a list of file names that match one or more patterns."
> — p.784.e189

中文解釋：

```tcl
% set mySVfiles [glob hdl/*.sv]
```

**`glob` 是合成腳本的標準開頭**——把所有 RTL 檔抓成一個清單再餵給 `analyze`/`read_verilog`。這樣加新檔案時不用改腳本。

`file exists` 常用來做條件判斷（例如「如果已經有合成結果就跳過」）。

`open`/`close`/`flush` 做檔案讀寫，模式預設唯讀，`w+` 是讀寫：

```tcl
% set f [open flopenr.sv w+]
% close $f
```

### 控制流程：條件一定要包大括號

> "All control flow statements evaluate a conditional expression, which is enclosed in curly braces {} and is implicitly evaluated using the expr command."
> — p.784.e189

中文解釋：**這是 Tcl 控制流程最重要的規則**。條件包在 `{}` 裡，Tcl 會**隱含地**用 `expr` 求值。

為什麼要用 `{}` 而不是 `""`？回顧 D.1 的表：**大括號阻止替換**。如果用引號，`$p` 會在迴圈開始前就被替換成當時的值，之後每次迭代都用同一個舊值——**無窮迴圈**。

```tcl
% set p 0
% while {$p < 5} {
%   puts "$p squared is : [expr $p * $p]"
%   incr p
% }
0 squared is 0
1 squared is 1
...
```

`incr p` 是 `set p [expr $p + 1]` 的簡寫。

### `for` 迴圈：四段大括號

> "The Tcl for loop has slightly different syntax than other programming languages: each part of the for loop is separated by curly braces"
> — p.784.e190

中文解釋：

```tcl
% for {init} {test} {reinit} {
%   body
% }
```

**和 C/Java 的差別是「用空白分隔的四個大括號區塊」而不是「分號分隔的三段加一個 body」**：

```tcl
% for {set p 0} {$p < 5} {incr p} {
%   puts "$p squared is: [expr $p * $p]"
% }
```

> "A for loop runs init as a Tcl script, then evaluates test as an expression. If test evaluates to TRUE (a nonzero value), body runs followed by reinit; the for loop then repeats, starting by evaluating test."
> — p.784.e190

中文解釋：注意 **`init` 是當成腳本執行，`test` 是當成算式求值**——又一次印證「不同命令對參數賦予不同意義」。

`continue`（跳過本次）和 `break`（跳出）的語意和其他語言一樣。

### 程序：`proc`

> "A procedure (proc) is a named block of commands. The syntax of the proc command is proc name args body, where name is the procedure's name, args are the arguments, and the procedure's commands are in body. Procedures don't require arguments, but any argument to a procedure must be a scalar value. So, arrays cannot be arguments to a procedure."
> — p.784.e191

中文解釋：**注意那個限制——「參數必須是純量，陣列不能當參數」**。要傳陣列得用 `upvar` 傳名字（本書沒講）。

```tcl
% proc max {a b} {
%   if {$a > $b} {
%     return $a
%   }
%   return $b
% }
% max 42 0
42
% set meaning [max 42 0]
```

**三個大括號區塊**：名稱、參數清單、函式本體。

### 執行腳本

> "A group of Tcl commands may be saved to a file, called a script. It is best practice to make Tcl scripts executable in Unix/Linux by invoking the Tcl/Tk interpreter (tclsh) in the first line of the script"
> — p.784.e191

中文解釋：

```tcl
#!/usr/bin/tclsh
set p 0
while {$p < 5} {
    puts "$p squared is : [expr $p * $p]"
    set p [expr $p + 1]
}
```

**這就是附錄 B.5 講的 shebang**。或者在工具的命令列用 `source test.tcl`——這是 EDA 工具裡最常見的用法（第 6 章的 `wally.tcl`、第 23 章的 `xlnx_mmcm.tcl`、`_ddr3-ArtyA7.tcl` 都是這樣被叫起來的）。

### 呼叫系統命令與環境變數

> "Some Linux commands also work within Tcl. The cd and pwd commands are equivalent to the Linux commands with the same name. ... Other system commands can be executed using the exec command."
> — p.784.e192

中文解釋：

```tcl
% exec date
Fri 18 Feb 2022 00:01:10 AM CST
```

> "Tcl can also access or create environment variables. For example, the following command makes environment variable TECH = tech. % set tech $::env(TECH)"
> — p.784.e192

中文解釋：**`$::env(...)` 是 EDA 腳本最實用的一招之一**——它讓腳本能讀取 shell 的環境變數。

這正是附錄 A.4 那個合成指令的機制：
```bash
make synth DESIGN=wallypipelinedcore TECH=sky130 CONFIG=synth_rv32e FREQ=330
```
Makefile 把這些變成環境變數，Tcl 腳本用 `$::env(TECH)`、`$::env(FREQ)` 讀進來——**同一份腳本就能跑不同的製程與頻率**。這是第 6 章 PPA 掃描的實作基礎。

### 別名

> "The alias command creates short forms (aliases) of common commands."
> — p.784.e192

中文解釋：四條規則：名稱可含字母/數字/底線/標點但**不能以數字開頭**、**大小寫敏感**、**不能和既有命令同名**、**只在當前工作階段有效**。

> "An alias definition takes effect immediately but lasts only until you exit the Tcl session. To save commonly used alias definitions, store them in a setup file as shown in the synthDC/.synopsys_dc.setup file used for Wally synthesis."
> — p.784.e192

中文解釋：**`.synopsys_dc.setup` 是 Synopsys Design Compiler 每次啟動時自動讀的設定檔**——把常用別名和設定放這裡。這和附錄 B.4 的 `.bashrc`/`.profile` 是同一個概念。

```tcl
% alias h history
```

多字命令的別名要包成清單（大括號或引號）。

### 離開

> "To exit the tool, use the exit command or type Ctrl-c."
> — p.784.e192

## 名詞表

| 名詞 | 意思 |
| --- | --- |
| list | Tcl 的有序元素群，元素可以是字串或子清單 |
| `lindex` | 取清單第 n 個元素（從 0 起算） |
| `glob` | 依 pattern 產生檔名清單 |
| `echo` / `puts` | 印到工具 console ／ 印到標準輸出 |
| `file exists` | 檔案是否存在（回傳 1 或 0） |
| conditional expression | 控制流程的條件，用 `{}` 包住並隱含以 `expr` 求值 |
| `incr` | 變數加一 |
| `proc name args body` | 定義程序 |
| scalar | 純量，程序參數只能是純量 |
| `tclsh` | Tcl 直譯器 |
| `source` | 在當前工作階段執行腳本 |
| `exec` | 執行系統命令 |
| `$::env(VAR)` | 讀取環境變數 |
| `alias` | 命令別名（只在當前工作階段有效） |
| `.synopsys_dc.setup` | Design Compiler 的啟動設定檔 |

## 所以呢

Tcl 語法上和其他語言最不同的三處：**`for` 是四個大括號區塊**、**條件必須用 `{}` 包住**（用 `""` 會因為提前替換而無窮迴圈）、**程序參數只能是純量**。

真正對 EDA 工作最有用的兩個：**`glob`**（自動抓所有 RTL 檔，不用手動維護清單）和 **`$::env(VAR)`**（讓同一份腳本能透過環境變數跑不同的製程、頻率、組態——這是第 6 章 PPA 掃描和第 23 章多板子支援的實作基礎）。

附錄 D 到此結束。
