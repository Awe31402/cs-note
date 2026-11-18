# C.9 Submodules

> 原書 p.784.e180–784.e181（Appendix C: Version Control Using Git）

## 這節在講什麼

submodule 是「把另一個 Git 倉庫嵌進自己的倉庫」。這對 `cvw` 特別重要——附錄 A.1 列過，`addins/` 底下有十幾個 submodule（SoftFloat、CoreMark、Embench、riscv-arch-test…）。

這節的核心是解釋**為什麼不直接複製程式碼進來**，以及 submodule 帶來的幾個額外指令。

## 關鍵點

### 為什麼不直接複製

> "Large projects often need to incorporate other projects. For example, the cvw project includes test cases from the riscv-arch-test repository. If one simply copied the code from riscv-arch-test into cvw, it would be difficult to apply any improvements that were later made to riscv-arch-test. Instead, Git supports submodules to incorporate one repository within another."
> — p.784.e180

中文解釋：**關鍵理由是「上游會繼續更新」**。

`riscv-arch-test` 是 RISC-V 官方維護的測試套件，一直在增加新測試（第 13 章、附錄 A.3 都提過「steadily growing」）。如果複製一份進 `cvw`，就再也拿不到新版了——除非手動重新複製，然後處理一堆差異。

submodule 的做法是：**`cvw` 只記錄「我用的是 `riscv-arch-test` 的哪一個 commit」**，實際的程式碼還是屬於那個倉庫。

### 四個指令

```bash
$ git submodule add <URL>                    # 加一個新的 submodule
$ git submodule update --remote              # 更新到 submodule 的最新版
$ git pull --recurse-submodules              # 拉主倉庫並更新 submodule
$ git clone --recurse-submodules <URL>       # clone 時一併取得
```

> "To clone a repository with submodules, be sure to include the --recurse-submodules flag so you don't have to initialize and update each submodule manually."
> — p.784.e180

中文解釋：**最後一個最常用也最常忘**——C.2 已經強調過一次。漏掉的話 `addins/` 底下全是空目錄。

### 兩個陷阱

**(1) `git pull` 不會抓新加的 submodule**

> "When submodules are added to a repository, git pull does not fetch the new submodules. Force the fetch by adding --recurse-submodules."
> — p.784.e181

中文解釋：**這是很容易中的坑**。別人加了一個新 submodule，你 `git pull` 之後會出現一個空目錄，然後建置失敗，而錯誤訊息完全看不出原因。

**養成習慣用 `git pull --recurse-submodules`** 就不會遇到。

**(2) 更新到主專案指定的版本**

> "To update all submodules to the versions checked into the main project, use: $ git submodule update --init --recurse-submodules"
> — p.784.e181

中文解釋：**注意這個和 `--remote` 的差別**，這是 submodule 最容易混淆的地方：

| 指令 | 更新到 |
| --- | --- |
| `git submodule update --init --recursive` | **主專案記錄的那個 commit**（你想要的通常是這個） |
| `git submodule update --remote` | **submodule 上游的最新版**（會改變主專案的記錄） |

**日常工作用第一個。** 因為主專案記錄的那個版本是「已知能通過測試」的版本；跳到上游最新版可能引入未經驗證的改動。

`--remote` 是刻意要升級 submodule 時才用，而且升級後要跑完整回歸測試，確認沒壞掉，然後把新的 commit 記錄提交進主專案。

### 不要直接改 submodule

> "Often a submodule is owned by somebody else, and you should not modify it or attempt to push commits back into it. If you want to propose changes to the submodule, fork it and make a pull request."
> — p.784.e181

中文解釋：**submodule 是別人的倉庫**。在 `cvw/addins/riscv-arch-test/` 裡改東西，你的改動：
- 不會被 `cvw` 的 commit 記錄下來（`cvw` 只記 commit hash）
- 推不上去（你沒有那個倉庫的寫入權限）
- 下次 `git submodule update` 就消失了

想改就走 C.1 的標準流程：**fork 那個 submodule 的倉庫 → 改 → 發 PR**。

（一個例外情況：第 21.3.2 節提過 Wally 的 `sim_bp` 是 fork 自 `synxlin/branch-predictor-simulator` 並修改過的——因為上游「似乎不接受 pull request」。這種情況只能自己維護一份 fork。）

### submodule 在 `cvw` 裡的實際樣貌

附錄 A.1 的目錄樹裡，`addins/` 底下這些全是 submodule：

| submodule | 用在哪一章 |
| --- | --- |
| `berkeley-softfloat-3` / `berkeley-testfloat-3` | 第 16 章的浮點參考模型與測試向量 |
| `riscv-arch-test` / `cvw-riscv-arch-test` / `cvw-arch-verif` | 第 13 章的架構測試 |
| `riscv-dv` | 隨機指令產生器 |
| `coremark` / `embench-iot` | 第 21 章的兩個 benchmark |
| `branch-predictor-simulator` | 第 21.3 章的 `sim_bp` |
| `verilog-ethernet` / `vivado-boards` | 第 23 章的 FPGA 支援 |

**這張表說明了 submodule 的價值**——`cvw` 本身只維護處理器的 RTL，這十幾個外部專案各有各的社群在維護。用 submodule 就能「借用而不接管」。

## 名詞表

| 名詞 | 意思 |
| --- | --- |
| submodule | 嵌在一個倉庫裡的另一個 Git 倉庫 |
| `git submodule add <URL>` | 加入新的 submodule |
| `--recurse-submodules` | clone 或 pull 時一併處理 submodule |
| `git submodule update --init --recursive` | 更新到**主專案記錄的**版本 |
| `git submodule update --remote` | 更新到 submodule **上游最新**版本 |

## 所以呢

submodule 的核心價值是**「借用別人的倉庫而不接管它」**——主專案只記錄「用哪個 commit」，上游更新時可以選擇性升級。

三個實務要點：**clone 和 pull 都要加 `--recurse-submodules`**、**`--init --recursive` 和 `--remote` 是完全不同的意思**（日常用前者）、**不要直接改 submodule 的內容**（要改就 fork 它並發 PR）。

下一節 C.10 是幾個零散但好用的功能。
