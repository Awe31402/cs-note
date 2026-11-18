# D.1 Tcl Usage

> 原書 p.784.e185–784.e187（Appendix D: Tcl Book of Armaments）
> 涵蓋 D.1.1 Arguments and Variables in Tcl、D.1.2 Nested Commands、D.1.3 Summary of the Basic Tcl Command Syntax

## 這節在講什麼

Tcl（Tool Command Language，唸作 "tickle"）是 EDA 工具的通用腳本語言。第 6 章的合成流程、第 23 章的 Vivado bitstream 產生——**全都是 Tcl 腳本驅動的**。

這節講 Tcl 的基本語法。最重要的一件事是：**Tcl 的一切都是字串**。這個設計決定造成了它所有的怪異之處，包括書上舉的那個把 500 MHz 除以 3 得到 0 的實際事故。

## 關鍵點

### 為什麼 EDA 界用 Tcl

> "Tcl (Tool Command Language) is an embeddable scripting language, i.e., an interpreted language implemented as a library package that can be easily incorporated into a variety of applications. Tcl is extensible, so additional functions (e.g., those provided by the enclosing application) can easily be added to those already provided by the Tcl interpreter."
> — p.784.e185

中文解釋：**關鍵字是 "embeddable"（可嵌入）和 "extensible"（可擴充）**。

EDA 工具需要一個腳本語言，但不想自己發明一個。Tcl 是一個**函式庫**，直接連進去就有了完整的腳本語言；然後工具再把自己的功能（`report_timing`、`compile_ultra`、`create_clock`…）註冊成新的 Tcl 命令。

> "System-on-chip (SoC) designs heavily use Tcl scripts to run synthesis and place & route. So, it is advantageous to learn these scripts for SoC runs and maximizing EDA optimization."
> — p.784.e185

中文解釋：這正是這個附錄存在的理由。書上也坦承：「Many books on Tcl/Tk exist... but we have yet to find a document that summarizes the main elements for EDA.」

（Tcl 由 John Ousterhout 在 1980 年代末於 UC Berkeley 開發。Tk 是配套的 GUI 工具包，合稱 Tcl/Tk。它們解決了兩個問題：**做 GUI 比 Motif 容易得多**、**任何應用程式都能輕鬆內嵌一個腳本語言**。而且免費。）

### 最基本的語法：命令 + 參數

> "Tcl commands are written in words separated by a white space. The first word is the command, and all others are the arguments. The result is a string."
> — p.784.e186

中文解釋：

```tcl
% set a 22
% set b 33
```

**注意最後那句話——「結果是一個字串」**。這是理解 Tcl 的鑰匙。

註解用 `#`，但有一個陷阱：

> "Comments begin with the # character. However, if you want a comment at the end of a Tcl command, you must also use a semicolon to indicate the end of the arguments."
> — p.784.e186

中文解釋：

```tcl
% set a 42 ;# a = 42, the meaning of the universe
```

**沒有那個分號的話，`#` 和後面的字會被當成 `set` 的參數**（因為 Tcl 不知道命令結束了）。這是一個典型的「因為一切都是字串」造成的怪異。

### 一切都是字串造成的問題

> "One of the challenging elements in Tcl is that the result is returned as a string. If you want to use that result in some way (e.g., an expression), you must combine it with other commands. That is, different commands assign different meaning to their arguments. Specific 'type checking' is done by the commands themselves."
> — p.784.e186

中文解釋：**Tcl 不做型別檢查，是每個命令自己決定怎麼解讀參數**。所以 `expr` 才存在——它是「請把這個字串當算式來算」的命令：

```tcl
% set a 122
122
% expr 24/3.2
7.5
% string length Hello
5
```

**沒有 `expr` 的話，`set a 1+1` 只會讓 a 變成字串 `"1+1"`**，不是 2。

### 整數除法：一個真實的事故

> "% expr 10 + 5          → 15
> % expr 10.0 + 5        → 15.0
> % expr 12 / 7          → 1
> % expr 12 / 7 + 3.0    → 4.0
> % expr 12.0 / 7 + 3    → 4.71428571429"
> — p.784.e186

中文解釋：**`12 / 7 = 1`**——因為兩個都是整數，所以做整數除法。

書上的邊欄講了一個真實事故：

> "The number representation in the argument indicates whether it is an integer or floating-point number. Therefore, care must be made to make sure these numbers are interpreted correctly. For example, one incident by the authors involved a frequency divider (dividing 500 MHz by 3) and using integers caused the result to be 0, which is incorrect. If a user wants to use a floating-point value, one of the numbers should be expressed as a floating-point value, i.e., 500.0/3"
> — p.784.e186（邊欄）

中文解釋：**這是一個值得記住的真實教訓**。他們要算「500 MHz 除以 3」的週期，用整數算出 0——**然後把 0 當成時脈週期送進合成工具**。

注意 `12 / 7 + 3.0 = 4.0`——**`12/7` 先算成整數 1，之後才和 3.0 相加**。加一個小數點在後面救不回來，**必須讓除法的其中一邊是浮點數**。

在寫時脈約束、面積計算、延遲換算時，這個坑一定要避開。

### 變數與替換

> "Variables are not declared separately, and their names can include letters, digits, or underscores. Variables are referred to by preceding the variable name with a $, and substitution may occur anywhere within a word"
> — p.784.e186

中文解釋：

```tcl
% set b 77
% set a b        → b        ← 沒有 $，所以 a 是字串 "b"
% set a $b       → 66       ← 有 $，所以取 b 的值
% set a $b+$b+$b → 77+77+77 ← 只做替換，不做運算！
% set a $b.3     → 77.3
% set a $b4      → no such variable   ← Tcl 以為變數叫 b4
```

**這五行把 Tcl 的替換機制講完了**：
1. **`$` 是「取值」**，沒有 `$` 就是字面字串
2. **替換之後不會再求值**——`$b+$b+$b` 變成 `"77+77+77"` 這個字串，要算就得包 `expr`
3. **`$b.3` 能正確解析**（`.` 不是變數名的合法字元），但 **`$b4` 會被當成變數 `b4`**

第三點是常見的 bug 來源。要明確界定變數名要用大括號：`${b}4`。

### 特殊字元的四種行為

邊欄整理了四個「阻止斷詞或替換」的方式：

> "Double quotes prevent breaks: % set y '$x; y is $y' → x is 5; y is 5
> Curly braces prevent breaks and substitutions: % set a {[expr $y*$y]} → [expr $y*$y]
> Backslashes escape special characters: % set a You're using\ coconuts! → You're using coconuts!
> Backslashes can also escape newlines (i.e., line continuation): % report_constraints \ -all_violators"
> — p.784.e187（邊欄）

中文解釋：**雙引號和大括號的差別是 Tcl 最重要的語法點**：

| | 阻止斷詞 | 阻止替換 |
| --- | --- | --- |
| `" "` 雙引號 | ✔ | ✘（`$` 和 `[]` 還會生效） |
| `{ }` 大括號 | ✔ | **✔（完全照字面）** |

**反斜線換行**（最後一個）在 EDA 腳本裡到處都是——合成命令的選項常常長到一行寫不下。

### 巢狀命令：`[ ]`

> "One advantage of Tcl is that commands can be nested or embedded within other commands. This can occur anywhere within a word. The square brackets nest the command properly (i.e., [ ])."
> — p.784.e187

中文解釋：

```tcl
% set b 8
% set a [expr $b + 9]        → 17
% set a "b-5 is [expr $b-5]" → b-5 is 3
% set my_clk_freq_MHz 500
% set my_period [expr 1000.0 / $my_clk_freq]  → 2.0
```

**`[ ]` 的意思是「先執行裡面的，把結果字串代回來」**——和 shell 的 `$(...)` 是同一個概念。

**注意最後那個例子用了 `1000.0` 而不是 `1000`**——正是為了避開整數除法的坑。這是寫時脈週期約束的標準寫法。

### 五個特殊字元的總表

Table D.1：

| 字元 | 作用 |
| --- | --- |
| `$` | 變數替換 |
| `[ ]` | 命令替換 |
| `" "` | 阻止斷詞（**替換仍生效**） |
| `{ }` | 阻止所有替換與斷詞 |
| `\` | 跳脫特殊字元（含換行） |

**這五個就是 Tcl 語法的全部。** 剩下的都是命令。

## 名詞表

| 名詞 | 意思 |
| --- | --- |
| Tcl | Tool Command Language，唸作 "tickle" |
| Tk | Tcl 的 GUI 工具包 |
| embeddable | 可嵌入，以函式庫形式併進應用程式 |
| extensible | 可擴充，應用程式能註冊自己的命令 |
| `tclsh` | Tcl 的獨立直譯器 |
| `%` | Tcl 的提示符號 |
| `set` | 設定變數 |
| `expr` | 把參數當算式求值 |
| `puts` | 輸出字串 |
| variable substitution | `$` 的變數替換 |
| command substitution | `[ ]` 的命令替換 |
| line continuation | 反斜線換行 |

## 所以呢

Tcl 的一切怪異都來自一個設計：**所有東西都是字串，型別由命令自己解讀**。所以才需要 `expr`、才需要分號才能寫行尾註解、才會有整數除法的陷阱。

三個一定要記住的：
1. **算數要包 `expr`**，而且**除法至少一邊要寫成浮點數**（書上那個 500/3 = 0 的真實事故）
2. **`" "` 會做替換，`{ }` 不會**
3. **`${b}4` 和 `$b4` 是不同的東西**

下一節 D.2 是清單、控制流程、程序、檔案操作這些「真正寫腳本會用到」的命令。
