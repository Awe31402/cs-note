# 軟體工程經典英文書目推薦（對標臺大李允中教授《軟體工程》）

本文件整理與國立臺灣大學資工系**李允中教授《軟體工程》**（臺大出版中心，2024 年 10 月，ISBN: `978-986-350-864-9`）主題脈絡直接對應的國際經典英文原典書目。

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

| 書名 | 作者 | 對標章節 | 核心特色與地位 |
| :--- | :--- | :--- | :--- |
| ***Software Engineering* (10th Edition)** | Ian Sommerville | 全書 1～10 章 | **全球最權威軟工教科書**。除了流程、需求工程與 OO 設計外，對**高可靠性系統／正規方法（第 9 章）**、**品質與配置管理（第 7、8 章）**有極嚴謹的系統化闡述，脈絡與臺大教材最為吻合。 |
| ***Software Engineering: A Practitioner's Approach* (9th Edition)** | Roger S. Pressman, Bruce R. Maxim | 全書 1～10 章<br>*(尤重 5, 7, 8, 10)* | 與 Sommerville 並列軟工雙璧。在**專案估算、風險管理、SQA（軟體品質保證）、SCM（軟體建構管理）與 CMMI 流程改善**的工程管理實務著墨極深。 |
| ***Object-Oriented Software Engineering: Using UML, Patterns, and Java* (3rd Edition)** | Bernd Bruegge, Allen H. Dutoit | 第 1～8 章 | 卡內基美隆大學（CMU）經典教材，將 **UML 建模、設計模式與建構／專案管理**完整串聯於單一開發生命週期中。 |

---

## 二、 需求工程、OOAD 與軟體設計（對應第 2、3、4 章）

1. ***Applying UML and Patterns: An Introduction to Object-Oriented Analysis and Design and Iterative Development* (3rd Edition) — Craig Larman**
   * **地位**：**OOAD（物件導向分析與設計）領域第一神書**（亦為水球潘 OOA/OOD 推導與 GRASP 九大職責分配原則的核心理論來源）。
   * **重點**：完整示範如何從 Use Case（需求工程）$\rightarrow$ Domain Model（OOA 領域模型）$\rightarrow$ Interaction & Class Diagrams（OOD 軟體設計），將 UML 當作思考與推導工具。
2. ***Object-Oriented Software Construction* (2nd Edition) — Bertrand Meyer**
   * **地位**：物件導向理論巨著，提出 **Design by Contract（契約式設計，DbC）** 與開關原則（OCP）。
   * **重點**：將**物件導向設計（第 3、4 章）**與**軟體正規方法／事前事後條件驗證（第 9 章）**完美結合。
3. ***Software Requirements* (3rd Edition) — Karl Wiegers & Joy Beatty**
   * **地位**：需求工程（第 2 章）業界實務最高標準專書，詳述需求萃取、分析、規格撰寫（SRS）與驗證。

---

## 三、 專案管理、品質保證、建構管理與 CMMI（對應第 5、7、8、10 章）

4. ***CMMI for Development: Guidelines for Process Integration and Product Improvement* (3rd Edition) — Mary Beth Chrissis, Mike Konrad, Sandy Shrum**
   * **重點**：CMU SEI（卡內基美隆大學軟體工程研究所）官方 CMMI 權威指南，直接對應第 10 章「軟體流程改善（CMMI）」與第 5、7、8 章的管理體系。
5. ***Modern Software Engineering: Doing What Works to Build Better Software Faster* — David Farley**
   * **重點**：由《Continuous Delivery》作者撰寫，從現代 CI/CD 與經驗主義視角重新詮釋「管理複雜度」、「持續驗證」與「建構管理」，適合與傳統教科書對照閱讀。
6. ***Software Engineering at Google: Lessons Learned from Programming Over Time* — Titus Winters, Tom Manshreck, Hyrum Wright**
   * **重點**：將教科書中的測試（第 6 章）、品質與 Code Review（第 7 章）、大規模建構與依賴管理（第 8 章）置於超大型程式碼庫與時間演進維度下檢視。

---

## 四、 軟體測試與正規方法論（對應第 6、9 章）

7. ***Introduction to Software Testing* (2nd Edition) — Paul Ammann & Jeff Offutt**
   * **重點**：將「第 6 章：軟體測試」與「第 9 章：正規邏輯與數學模型」結合的權威教科書，以圖論覆蓋（Graph Coverage）、邏輯覆蓋（Logic Coverage）、語法覆蓋（Syntax Coverage）系統化定義測試設計。
8. ***Software Abstractions: Logic, Language, and Analysis* — Daniel Jackson**
   * **重點**：MIT 教授撰寫的**輕量級正規方法（Lightweight Formal Methods）**經典（基於 Alloy 建模語言），能對軟體設計模型進行自動化邏輯驗證，比傳統 Z Notation 或 Petri Net 更易於工程師上手。

---

## 五、 閱讀路線建議

* **路線 A（課堂 1:1 原文對照與考研究所／扎實打底）**：
  * 李允中《軟體工程》 + Ian Sommerville ***Software Engineering (10th Ed.)***
* **路線 B（精通 UML、OOAD 與軟體架構設計）**：
  * 李允中《軟體工程》第 2～4 章 + Craig Larman ***Applying UML and Patterns (3rd Ed.)***
* **路線 C（連結現代敏捷、CI/CD 與 AI 規格驅動開發）**：
  * Craig Larman ***Applying UML and Patterns*** + David Farley ***Modern Software Engineering***
