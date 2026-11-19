# 軟體工程經典英文書目推薦（對標臺大李允中教授《軟體工程》）

本文件整理與國立臺灣大學資工系**李允中教授《軟體工程》**（臺大出版中心，2024 年 10 月，ISBN: `978-986-350-864-9`）主題脈絡直接對應的國際經典英文原典書目，並附上本地 [`books/`](./books/) 目錄下的 PDF 檔案連結。

### 📌 基準參考：李允中《軟體工程》全書十章架構
1. 軟體危機與流程
2. 需求工程
3. 物件導向軟體開發
4. 軟體設計
5. 軟體專案計畫與管理
6. 軟體測試
7. 軟體品質管理與保證
8. 軟體建構管理
9. 軟體正規方法論（Formal Methods）
10. 軟體流程改善（軟體能力成熟度整合模式 CMMI）

---

## 一、 全章節直接對標的「軟體工程教科書聖經」

| 書名 | 作者 | 對標章節 | 本地 PDF 連結 | 核心特色與地位 |
| :--- | :--- | :--- | :--- | :--- |
| ***Software Engineering* (9th / 10th Ed.)** | Ian Sommerville | 全書 1～10 章 | [📖 開啟 PDF](./books/Software%20Engineering,%209th%20Edition%20(Ian%20Sommerville)%20(z-library.sk,%201lib.sk,%20z-lib.sk).pdf) | **全球最權威軟工教科書**。除了流程、需求工程與 OO 設計外，對**高可靠性系統／正規方法（第 9 章）**、**品質與配置管理（第 7、8 章）**有極嚴謹的系統化闡述，脈絡與臺大教材最為吻合。 |
| ***Engineering Software Products: An Introduction to Modern Software Engineering*** | Ian Sommerville | 第 1～8 章<br>*(現代產品開發)* | [📖 開啟 PDF](./books/Engineering%20Software%20Products%20An%20Introduction%20to%20Modern%20Software%20Engineering%20(Ian%20Sommerville)%20(z-library.sk,%201lib.sk,%20z-lib.sk).pdf) | Sommerville 針對**現代敏捷、微服務、雲端架構、DevOps 與產品導向開發**撰寫的輕量現代版軟工教材。 |
| ***Software Engineering: A Practitioner's Approach* (9th Ed.)** | Roger S. Pressman, Bruce R. Maxim | 全書 1～10 章<br>*(尤重 5, 7, 8, 10)* | [📖 開啟 PDF](./books/Software%20Engineering%20(Roger%20S.%20Pressman,%20Bruce%20R.%20Maxim)%20(z-library.sk,%201lib.sk,%20z-lib.sk).pdf) | 與 Sommerville 並列軟工雙璧。在**專案估算、風險管理、SQA（軟體品質保證）、SCM（軟體建構管理）與 CMMI 流程改善**的工程管理實務著墨極深。 |
| ***Object-Oriented Software Engineering: Using UML, Patterns, and Java* (3rd Ed.)** | Bernd Bruegge, Allen H. Dutoit | 第 1～8 章 | [📖 開啟 PDF](./books/Object-oriented%20software%20engineering%20%20using%20UML,%20Patterns,%20and%20Java%20(Bruegge,%20Bernd%20Dutoit,%20Allen%20H.)%20(z-library.sk,%201lib.sk,%20z-lib.sk).pdf) | 卡內基美隆大學（CMU）經典教材，將 **UML 建模、設計模式與建構／專案管理**完整串聯於單一開發生命週期中。 |

---

## 二、 需求工程、OOAD、軟體設計與 Clean Code（對應第 2、3、4 章）

1. ***Applying UML and Patterns: An Introduction to Object-Oriented Analysis and Design and Iterative Development* (3rd Edition) — Craig Larman**
   * **本地檔案**：[📖 Applying UML and Patterns (Craig Larman).pdf](./books/Applying%20UML%20and%20Patterns%20an%20Introduction%20to%20the%20Object-Oriented%20Analysis%20and%20Design%20and%20Iterative%20Development%20(Craig%20Larman)%20(z-library.sk,%201lib.sk,%20z-lib.sk).pdf)
   * **地位**：**OOAD（物件導向分析與設計）領域第一神書**（亦為水球潘 OOA/OOD 推導與 GRASP 九大職責分配原則的核心理論來源）。
   * **重點**：完整示範如何從 Use Case（需求工程）$\rightarrow$ Domain Model（OOA 領域模型）$\rightarrow$ Interaction & Class Diagrams（OOD 軟體設計），將 UML 當作思考與推導工具。
2. ***Object-Oriented Software Construction* (2nd Edition) — Bertrand Meyer**
   * **本地檔案**：[📖 Object-oriented software construction (Bertrand Meyer).pdf](./books/Object-oriented%20software%20construction%20(Bertrand%20Meyer)%20(z-library.sk,%201lib.sk,%20z-lib.sk).pdf)
   * **地位**：物件導向理論巨著，提出 **Design by Contract（契約式設計，DbC）** 與開關原則（OCP）。
   * **重點**：將**物件導向設計（第 3、4 章）**與**軟體正規方法／事前事後條件驗證（第 9 章）**完美結合。
3. ***Software Requirements* (3rd Edition) — Karl Wiegers & Joy Beatty**
   * **本地檔案**：[📖 Software_Requirements_3rd_Edition.pdf](./books/Software_Requirements_3rd_Edition.pdf)
   * **地位**：需求工程（第 2 章）業界實務最高標準專書，詳述需求萃取、分析、規格撰寫（SRS）與驗證。
4. ***Clean Code Principles and Patterns* (2nd Edition) — Petri Silen**
   * **本地檔案**：[📖 Clean Code Principles and Patterns, 2nd Edition (Petri Silen).pdf](./books/Clean%20Code%20Principles%20and%20Patterns,%202nd%20Edition%20(Petri%20Silen)%20(z-library.sk,%201lib.sk,%20z-lib.sk).pdf)
   * **重點**：結合現代物件導向設計原則（SOLID）、整潔程式碼規範與常見設計模式（Design Patterns）的實務指南。
5. ***Software Architecture with C++* (2nd Edition) — Piotr Gaczkowski**
   * **本地檔案**：[📖 Software Architecture with C++ (2nd Edition).pdf](./books/dokumen.pub_software-architecture-with-c-designing-robust-c-systems-with-modern-architectural-practices-2.pdf)
   * **重點**：以現代 C++ 實踐軟體架構設計、模組化、設計模式、測試與建構部署。

---

## 三、 BDD（行為驅動開發）與 DDD（領域驅動設計）（對應第 2、3、4、6 章）

6. ***BDD in Action* (2nd Edition) — John Ferguson Smart & Jan Molak**
   * **本地檔案**：[📖 BDD in Action, 2nd Edition (John Ferguson Smart, Jan Molak).pdf](./books/BDD%20in%20Action,%202nd%20Edition%20(Final%20Release)%20(John%20Ferguson%20Smart,%20Jan%20Molak)%20(z-library.sk,%201lib.sk,%20z-lib.sk).pdf)
   * **重點**：**行為驅動開發（Behavior-Driven Development）權威指南**。詳述如何從商業目標推導出可執行的規格（Executable Specifications / Given-When-Then 活文件），是實踐 **AI × BDD 規格驅動開發** 的必讀核心。
7. ***Learning Domain-Driven Design: Aligning Software Architecture and Business Strategy* — Vladik Khononov**
   * **本地檔案**：[📖 Learning Domain-Driven Design (Vladik Khononov).pdf](./books/Learning%20Domain-Driven%20Design%20Aligning%20Software%20Architecture%20and%20Business%20Strategy%20(Vladik%20Khononov)%20(z-library.sk,%201lib.sk,%20z-lib.sk).pdf)
   * **重點**：O'Reilly 出版的現代 DDD 最佳導讀，清晰拆解戰略設計（Bounded Context、Ubiquitous Language）與戰術設計（Aggregate、Domain Event），並與微服務及現代軟體架構對齊。
8. ***Domain-Driven Design: A Pragmatic Approach* — Eduard Ghergu**
   * **本地檔案**：[📖 Domain-Driven Design A pragmatic approach (Eduard Ghergu).pdf](./books/Domain-Driven%20Design%20A%20pragmatic%20approach%20(Eduard%20Ghergu)%20(z-library.sk,%201lib.sk,%20z-lib.sk).pdf)
   * **重點**：以務實角度切入領域驅動設計的建模與落地實作。

---

## 四、 專案管理、品質保證、建構管理與現代軟工（對應第 5、7、8、10 章）

9. ***Modern Software Engineering: Doing What Works to Build Better Software Faster* — David Farley**
   * **本地檔案**：[📖 Modern Software Engineering (David Farley).pdf](./books/dokumen.pub_modern-software-engineering-doing-what-works-to-build-better-software-faster-1nbsped-0137314914-9780137314911.pdf)
   * **重點**：由《Continuous Delivery》作者撰寫，從現代 CI/CD 與工程經驗主義視角重新詮釋「管理複雜度（模組化、內聚、耦合）」與「持續驗證（迭代、回饋、實驗）」，極適合與傳統教科書對照閱讀。
10. ***Software Engineering at Google: Lessons Learned from Programming Over Time* — Titus Winters, Tom Manshreck, Hyrum Wright**
    * **本地檔案**：[📖 Software Engineering at Google.pdf](./books/software-engineering-at-google-lessons-learned-from-programming-over-time-1nbsped-1492082791-9781492082798_compress.pdf)
    * **重點**：將教科書中的測試（第 6 章）、品質與 Code Review（第 7 章）、大規模建構與依賴管理（第 8 章）置於超大型程式碼庫與時間演進維度下檢視。
11. ***CMMI for Development: Guidelines for Process Integration and Product Improvement* (3rd Edition) — Mary Beth Chrissis, Mike Konrad, Sandy Shrum**
    * **本地檔案**：*(未收錄 PDF，可參考 Sommerville 與 Pressman 書中對應章節)*
    * **重點**：CMU SEI（卡內基美隆大學軟體工程研究所）官方 CMMI 權威指南，直接對應第 10 章「軟體流程改善（CMMI）」與第 5、7、8 章的管理體系。

---

## 五、 軟體測試與正規方法論（對應第 6、9 章）

12. ***Introduction to Software Testing* (2nd Edition) — Paul Ammann & Jeff Offutt**
    * **本地檔案**：[📖 Introduction to Software Testing.pdf](./books/Introduction%20to%20Software%20Testing.pdf)
    * **重點**：將「第 6 章：軟體測試」與「第 9 章：正規邏輯與數學模型」結合的權威教科書，以圖論覆蓋（Graph Coverage）、邏輯覆蓋（Logic Coverage）、語法覆蓋（Syntax Coverage）系統化定義測試設計。
13. ***Software Abstractions: Logic, Language, and Analysis* — Daniel Jackson**
    * **本地檔案**：[📖 AlloyBook (Software Abstractions - Daniel Jackson).pdf](./books/AlloyBook.pdf)
    * **重點**：MIT 教授撰寫的**輕量級正規方法（Lightweight Formal Methods）**經典（基於 Alloy 建模語言），能對軟體設計模型進行自動化邏輯驗證，比傳統 Z Notation 或 Petri Net 更易於工程師上手。

---

## 六、 閱讀路線建議

* **路線 A（課堂 1:1 原文對照與扎實打底）**：
  * 李允中《軟體工程》 + Ian Sommerville ***Software Engineering*** + Roger Pressman ***Software Engineering: A Practitioner's Approach***
* **路線 B（精通 UML、OOAD 與領域建模）**：
  * 李允中《軟體工程》第 2～4 章 + Craig Larman ***Applying UML and Patterns*** + Vladik Khononov ***Learning Domain-Driven Design***
* **路線 C（連結現代敏捷、CI/CD 與 AI × BDD 規格驅動開發）**：
  * John Ferguson Smart ***BDD in Action*** + Craig Larman ***Applying UML and Patterns*** + David Farley ***Modern Software Engineering***
