# 《Dependency Injection Principles, Practices, and Patterns》讀書筆記

- **作者**：Steven van Deursen、Mark Seemann
- **出版**：Manning Publications，2019，ISBN 978-1-61729-473-0
- **前身**：Mark Seemann《Dependency Injection in .NET》（第一版）
- **範例語言**：C#／.NET Core（ASP.NET Core MVC、Entity Framework Core）

**閱讀說明**：本筆記以中文整理重點，引用保留英文原文（附書本頁碼；PDF 頁＝書本頁＋31）。給「C／Linux 熟、C# 不熟」的讀者，所以 C#／.NET 名詞會多解釋一點，必要時對照 C 的寫法。書中的術語（書裡用小型大寫標示的，例如 Composition Root、Control Freak）保留英文，第一次出現附中文。書中未提到的內容一律標「書中未提」，不做補充推測。

**全書結構**：

| 部分 | 章 | 主題 |
|---|---|---|
| Part 1 Putting DI on the map | 1–3 | DI 是什麼；同一個電商程式先寫成緊耦合、再改成鬆耦合 |
| Part 2 Catalog | 4–6 | 型錄：4 個 pattern、4 個 anti-pattern、常見 code smell |
| Part 3 Pure DI | 7–11 | 不用容器的 DI：組裝、物件生命週期、攔截、AOP |
| Part 4 DI Containers | 12–15 | DI 容器概論＋Autofac、Simple Injector、MS.DI 三個容器 |

---

## 第 1 章：The basics of Dependency Injection: What, why, and how（DI 的基本：是什麼、為什麼、怎麼做）

> 對應原書 p.3–33

### 這章在幹嘛

全書的地基章，程式碼最少。先講 DI 的**目的**（可維護性），再拆掉四個常見誤解，然後用「插頭與插座」比喻把 DI 跟幾個設計模式連起來。

骨架是四段：

1. DI 是為了「可維護」，手段是「鬆耦合」；先破除四個迷思
2. Hello DI! 範例：一行的 Hello World 拆成兩個 class＋一個 interface，看它換到什麼好處
3. 什麼該注入、什麼不用：Stable vs. Volatile Dependency
4. DI 的範圍：Object Composition、Object Lifetime、Interception 三個維度

---

### 重點 1：DI 的定義——是一組原則與模式，不是工具

> **原文引用（p.4）**
> "Dependency Injection is a set of software design principles and patterns that enables you to develop loosely coupled code."

作者引用 Stack Overflow 上「怎麼跟五歲小孩解釋 DI」的高票答案：小孩自己去開冰箱會出事（門沒關、拿到不該拿的、東西過期），應該說「我午餐要喝的」，由爸媽準備好。

> **原文引用（p.4）**
> "collaborating classes (the five-year-old) should rely on infrastructure (the parents) to provide necessary services."

**名詞**：書中說 DI 圈子常講的 *service* ＝ 抽象（Abstraction），*component* ＝ 實作那個抽象的 class。因為這兩個字太多義，**全書改用「Abstraction」和「class」**。

**要記住的觀點**：DI 不是目的，是手段。目的是可維護的程式碼。

---

### 重點 2：可維護 ← 鬆耦合 ← DI

> **原文引用（p.5）**
> "Loose coupling makes code extensible, and extensibility makes it maintainable. DI is nothing more than a technique that enables loose coupling."

鬆耦合的老祖宗是 1994 年 Gang of Four《Design Patterns》的那句話：

> **原文引用（p.5）**
> "Program to an interface, not an implementation."

書中強調這句在 GoF 書裡是**前提**，不是結論。

**對照 C**：「對 interface 寫程式」在 C 裡就是 Linux kernel 常見的 ops struct（`struct file_operations` 之類的函式指標表），呼叫端只認表，不認哪個 driver。（這個對照是筆記自己加的，書中未提 C。）

---

### 重點 3：四個要先「忘掉」的迷思

作者說學 DI 最大的障礙是你以為你已經懂了：

> **原文引用（p.5）**
> "Before you can learn, you must unlearn what (you think) you already know."

| 迷思 | 真相 |
|---|---|
| DI 只跟 late binding 有關 | late binding 只是 DI 的好處之一 |
| DI 只是為了 unit test | 單元測試也只是好處之一 |
| DI 是加強版的 Abstract Factory | 那叫 Service Locator，是 DI 的**反面** |
| DI 一定要有 DI Container | 容器是選配；不用容器叫 **Pure DI** |

**late binding（晚期綁定）**：不重新編譯就能換掉程式的某一塊。例如程式同時支援 Oracle 和 SQL Server，用設定檔決定用哪個實作。作者說：DI 能做到 late binding，但「能做到」不代表「只為了它」——這個關係不是對稱的。

**最危險的迷思是第三個**。Abstract Factory 有固定幾個 `CreateXxx()` 方法；Service Locator 只有一個能生出任何型別的方法：

```csharp
public interface IServiceLocator
{
    object GetService(Type serviceType);   // 想要什麼都跟它要
}
```

> **原文引用（p.7）**
> "DI is the opposite of a Service Locator; it's a way to structure code so that you never have to imperatively ask for Dependencies. Rather, you require consumers to supply them."

> **原文引用（p.8）**
> "Pure DI is the practice of applying DI without a DI Container."

（第一版叫 *Poor Man's DI*，第二版改名 Pure DI。）

**要記住的觀點**：DI 是「別人塞給你」，Service Locator 是「你自己去要」。兩者方向相反。

---

### 重點 4：插頭與插座——把設計模式講成電器

作者用電器接線比喻軟體設計，連非技術人員都能聽懂。

**反例：便宜旅館的吹風機直接接死在牆上**（Figure 1.3）。壞了要斷電、找技師、用特殊工具拆。這就是緊耦合。

**正例：插頭＋插座**。插座＝interface，插頭＋電器＝實作。只要插頭合得上、電壓頻率對（遵守 interface 的契約），怎麼組合都行。

| 生活中的東西 | 對應的原則／模式 | 意思 |
|---|---|---|
| 吹風機拔掉、插電腦 | **Liskov Substitution Principle（里氏替換原則）** | 換實作，不用改插座 |
| 兒童安全插座蓋 | **Null Object** | 什麼都不做的實作，插著也不會爆 |
| UPS 不斷電系統 | **Decorator（裝飾者）** | 包在原本實作外面，介面一樣，加功能 |
| 延長線 | **Composite（組合）** | 一個插頭，後面接好幾個實作 |
| 出國用的轉接頭 | **Adapter（轉接器）** | 把兩個不同的介面接起來 |

> **原文引用（p.11）**
> "This principle states that we should be able to replace one implementation of an interface with another without breaking either the client or the implementation."

> **原文引用（p.11）**
> "When it comes to DI, the Liskov Substitution Principle is one of the most important software design principles."

插座發明時沒人想到個人電腦，卻能服務電腦——這就是好設計能滿足「當初沒預料到的需求」。

---

### 重點 5：Open/Closed Principle 與 DI 要回答的核心問題

> **原文引用（p.14）**
> "ideally, a new requirement should only necessitate the addition of a new class, with no changes to other already-existing classes of the system."

這叫 **Open/Closed Principle（開放封閉原則）**：對擴充開放、對修改封閉。作者承認 100% 做不到，但鬆耦合會讓你更接近。

然後拋出整本書的核心問題：interface 沒辦法 `new`，那實例從哪來？

```csharp
IMessageWriter writer = new IMessageWriter();   // 編不過
```

> **原文引用（p.14）**
> "The easy part of loose coupling is programming to an interface instead of an implementation. The question is, 'Where do the instances come from?' In a sense, this is what this entire book is about"

**要記住的觀點**：寫 interface 很簡單；難的是「誰來 new 實作、在哪裡 new」。這就是 DI 要解的題。

---

### 重點 6：Hello DI! —— 把一行拆成兩個 class

```csharp
private static void Main()
{
    IMessageWriter writer = new ConsoleMessageWriter();
    var salutation = new Salutation(writer);
    salutation.Exclaim();
}
```

`Salutation` 是主邏輯，只認 `IMessageWriter`：

```csharp
public class Salutation
{
    private readonly IMessageWriter writer;

    public Salutation(IMessageWriter writer)   // Constructor Injection
    {
        if (writer == null)                    // Guard Clause
            throw new ArgumentNullException("writer");
        this.writer = writer;
    }

    public void Exclaim() { this.writer.Write("Hello DI!"); }
}

public interface IMessageWriter { void Write(string message); }

public class ConsoleMessageWriter : IMessageWriter     // 包住 Console＝Adapter
{
    public void Write(string message) { Console.WriteLine(message); }
}
```

- 在 `Main` 裡自己 `new` 出整串物件、不用容器 → 這就是 **Pure DI** 的最小例子
- **Constructor Injection（建構子注入）**：

> **原文引用（p.16）**
> "Constructor Injection is the act of statically defining the list of required Dependencies by specifying them as parameters to the class's constructor."

- **Guard Clause（防衛子句）**：建構子一開頭檢查參數不是 null，是就丟例外
- C# 小註：`readonly` 欄位只能在建構子裡設定；`: IMessageWriter` 表示實作這個 interface

---

### 重點 7：五個好處（Table 1.1）

作者自己算：一行變 28 行，「overhead 2800%」。但程式從一行長到幾萬行時，這個額外成本就幾乎看不到了。

| Benefit（好處） | Description（說明） | When is it valuable?（何時有價值） |
|---|---|---|
| Late binding | 不重編就能換服務 | 套裝軟體有價值；執行環境固定的企業內部系統較少 |
| Extensibility（可擴充） | 能用沒計畫過的方式擴充、重用 | 永遠有價值 |
| Parallel development（平行開發） | 可以同時開發 | 大型複雜系統有價值；小程式不太需要 |
| Maintainability（可維護） | 職責清楚的 class 好維護 | 永遠有價值 |
| Testability（可測試） | class 可以做單元測試 | 永遠有價值 |

作者觀察：大家想到 DI 只想到第一條 late binding，然後覺得「我們又不會換資料庫」，就判斷 DI 不划算。Mark 說以前大家問「關聯式資料庫還有替代品嗎？」，現在有了 Azure、NoSQL、Event Sourcing，換資料庫的說法變得有說服力了。

**late binding 版本**：從 `appsettings.json` 讀型別名稱，用 reflection（反射，執行期依名字建立物件）`Activator.CreateInstance()` 生出 `IMessageWriter`。書中自己註明這段其實犯了 Constrained Construction anti-pattern（第 5 章）。

---

### 重點 8：擴充＝加一個 Decorator，不改舊 code

要「只有登入的使用者才能印訊息」，不改 `Salutation`、也不改 `ConsoleMessageWriter`，而是加一個新的 `IMessageWriter`：

```csharp
public class SecureMessageWriter : IMessageWriter   // 實作它，也消費它
{
    private readonly IMessageWriter writer;
    private readonly IIdentity identity;

    public SecureMessageWriter(IMessageWriter writer, IIdentity identity) { ... }

    public void Write(string message)
    {
        if (this.identity.IsAuthenticated)
            this.writer.Write(message);
    }
}
```

唯一要改的是 `Main` 的組裝：

```csharp
IMessageWriter writer =
    new SecureMessageWriter(
        new ConsoleMessageWriter(),
        WindowsIdentity.GetCurrent());
```

> **原文引用（p.20）**
> "The only place where you need to modify the code is at the application entry point. SecureMessageWriter implements the security features of the application, whereas ConsoleMessageWriter addresses the user interface."

每個 class 只做一件事 → **Single Responsibility Principle（單一職責原則）**，第 2 章細講。

---

### 重點 9：平行開發、可維護、可測試

- **平行開發**：`SecureMessageWriter` 和 `ConsoleMessageWriter` 互不依賴，兩隊人只要先講好 `IMessageWriter` 就能各做各的

> **原文引用（p.21）**
> "In object-oriented software design, a module is a group of logically related classes (or components), where a module is independent of and interchangeable with other modules."

- **可維護**：職責清楚 → 知道改哪裡；除錯時嫌疑犯範圍變小
- **可測試**：作者把 Testability 標成「永遠有價值」。Michael Feathers 甚至把「沒有單元測試的程式」定義成 legacy application

**Test Double（測試替身）**：只在測試時用、代替真正依賴的實作。真的依賴太慢、太貴、有破壞性、或跟這次測試無關時就用它。底下還分 Stub、Mock、Fake、Spy 等。

```csharp
[Fact]   // xUnit 的測試標記（C# attribute：掛在方法上的中繼資料）
public void ExclaimWillWriteCorrectMessageToMessageWriter()
{
    var writer = new SpyMessageWriter();     // Test Spy：記下被呼叫的內容
    var sut = new Salutation(writer);        // SUT = System Under Test
    sut.Exclaim();
    Assert.Equal(expected: "Hello DI!", actual: writer.WrittenMessage);
}
```

作者的實戰例子（Steven）：系統會透過第三方發 SMS，每封要錢。手動測試時套一個 Decorator，把收件號碼改成測試人員自己的手機。

> **原文引用（p.23）**
> "Regardless, DI provides options in the future with minimal additional overhead today."

---

### 重點 10：Seam（接縫）——在哪裡切開程式

> **原文引用（p.25）**
> "Everywhere you decide to program against an Abstraction instead of a concrete type, you introduce a Seam into the application. A Seam is a place where an application is assembled from its constituent parts, similar to the way a piece of clothing is sewn together at its seams."

Hello DI! 裡 `Salutation` 和 `ConsoleMessageWriter` 之間就是一個 Seam。（這個詞出自 Feathers《Working Effectively with Legacy Code》。）

但不是每個依賴都要做成 Seam——加 Seam 是額外工作。要分清楚兩種依賴。

---

### 重點 11：Stable vs. Volatile Dependency（本章最實用的判斷工具）

**Stable Dependency（穩定依賴）**——直接用，不用抽象：

> **原文引用（p.26）**
> "The class or module already exists. You expect that new versions won't contain breaking changes. The types in question contain deterministic algorithms. You never expect to have to replace, wrap, decorate, or Intercept the class or module with another."

例：.NET 的 BCL（Base Class Library，標準函式庫）大部分型別、化學計算之類的專門演算法庫。判斷法是排除法：**不是 volatile 的就是 stable**。

**Volatile Dependency（易變依賴）**——符合任一條就是：

1. **需要設定執行環境**：資料庫（關聯式資料庫是典型）、message queue、web service、**連檔案系統都算**。不藏在 Seam 後面就換不掉、也很難跑自動化測試
2. **還不存在或還在開發中**
3. **不是每台開發機都裝得起來**：昂貴的第三方庫、只能裝在某些 OS 上的
4. **行為不確定（nondeterministic）**：亂數、跟現在日期時間有關的演算法（`System.Random`、`DateTime.Now`）

> **原文引用（p.27）**
> "Volatile Dependencies are the focal point of DI. It's for Volatile Dependencies rather than Stable Dependencies that you introduce Seams into your application."

**DI Container 本身**呢？看你想不想換它。作者建議只有程式進入點引用容器，免得一輩子被綁住。

**要記住的觀點**：只替 Volatile Dependency 開 Seam。時間、亂數、檔案、資料庫、網路都算 volatile。

---

### 重點 12：DI 的三個維度

class 交出「建立依賴」的控制權，並不是失去控制，而是**把控制搬到別的地方**：

> **原文引用（p.28）**
> "As developers, we gain control by removing a class's control over its Dependencies. This is an application of the Single Responsibility Principle. Classes shouldn't have to deal with the creation of their Dependencies."

| 維度 | 意思 | 書中哪裡講 |
|---|---|---|
| **Object Composition（物件組裝）** | 把 class 組成應用程式 | 第 7 章 |
| **Object Lifetime（物件生命週期）** | 依賴何時建立、何時釋放、能不能共用 | 第 8 章 |
| **Interception（攔截）** | 交給消費者之前先包一層、改它的行為 | 第 9–11 章 |

- **Composition**：一開始 DI 就等於物件組裝，Martin Fowler 的原始文章也只講這個
- **Lifetime**：依賴可以共用（同一個 `writer` 塞給兩個消費者），所以單一消費者不可能管它的壽命。只靠 GC 還好，碰到 `IDisposable`（C# 中需要手動釋放資源的物件，類似 C 要記得 `close()`）就複雜了
- **Interception**：就是 Decorator 的應用，再往前走就是 AOP（Aspect-Oriented Programming，切面導向程式設計），用來處理 logging、auditing、存取控制、驗證這類 **Cross-Cutting Concern（橫切關注點：影響整個程式很多地方的非功能需求）**

> **原文引用（p.31）**
> "Object Composition provides the foundation, and Lifetime Management addresses some important side effects. But it's mainly when it comes to Interception that you start to reap the benefits."

---

### 重點 13：DI 和 IoC 不是同一件事

> **原文引用（p.29）**
> "These two terms are sometimes used interchangeably, but DI is a subset of IoC."

- **IoC（Inversion of Control，控制反轉）** 原意是「框架控制程式流程」。寫 ASP.NET Core MVC 時，是框架在呼叫你的 controller 方法——你沒有控制權
- 命令列程式則相反：`Main` 一被呼叫，你就完全掌控
- 後來大家把管理依賴的函式庫叫「IoC Container」，IoC 的意思就漂移成「依賴的控制反轉」。Martin Fowler 才另外取名 **Dependency Injection**，專指這個意思

**要記住的觀點**：IoC 比較廣；DI 是 IoC 用在「依賴管理」上的那一塊。本書一律用比較精確的 DI。

---

### 重點 14：DI 一定要全面，不能事後補

> **原文引用（p.32）**
> "DI must be pervasive. You can't easily retrofit loose coupling onto an existing code base."

> **原文引用（p.32）**
> "Small code bases, like a classic Hello World example, are inherently maintainable because of their size. This is why DI tends to look like overengineering in simple examples."

所以作者接下來用兩章做一個比較大的例子，讓好處看得見。

---

### 本章 Summary（作者原話）

> **原文引用（p.32–33）**
> - "Dependency Injection is a set of software design principles and patterns that enables you to develop loosely coupled code. Loose coupling makes code more maintainable."
> - "When you have a loosely coupled infrastructure in place, it can be used by anyone and adapted to changing needs and unanticipated requirements without having to make large changes to the application's code base and its infrastructure."
> - "Troubleshooting tends to become less taxing because the scope of likely culprits narrows."
> - "DI enables late binding, which is the ability to replace classes or modules with different ones without the need for the original code to be recompiled."
> - "DI makes it easier for code to be extended and reused in ways not explicitly planned for, similar to the way you have flexibility when working with electrical plugs and sockets."
> - "DI simplifies parallel development on the same code base because the Separation of Concerns allows each team member or even entire teams to work more easily on isolated parts."
> - "DI makes software more Testable because you can replace Dependencies with test implementations when writing unit tests."
> - "When you practice DI, collaborating classes should rely on infrastructure to provide the necessary services. You do this by letting your classes depend on interfaces, instead of concrete implementations."
> - "Classes shouldn't ask a third party for their Dependencies. This is an anti-pattern called Service Locator. Instead, classes should specify their required Dependencies statically using constructor parameters, a practice called Constructor Injection."
> - "Many developers think that DI requires specialized tooling, a so-called DI Container. This is a myth. A DI Container is a useful, but optional, tool."
> - "One of the most important software design principles that enables DI is the Liskov Substitution Principle. It allows replacing one implementation of an interface with another without breaking either the client or the implementation."
> - "Dependencies are considered Stable in the case that they're already available, have deterministic behavior, don't require a setup runtime environment (such as a relational database), and don't need to be replaced, wrapped, or intercepted."
> - "Dependencies are considered Volatile when they are under development, aren't always available on all development machines, contain nondeterministic behavior, or need to be replaced, wrapped, or intercepted."
> - "Volatile Dependencies are the focal point of DI. We inject Volatile Dependencies into a class's constructor."
> - "By removing control over Dependencies from their consumers, and moving that control into the application entry point, you gain the ability to apply Cross-Cutting Concerns more easily and can manage the lifetime of Dependencies more effectively."
> - "To succeed, you need to apply DI pervasively. All classes should get their required Volatile Dependencies using Constructor Injection. It's hard to retrofit loose coupling and DI onto an existing code base."

### 我的整理：本章帶走的五句話

1. DI 是「讓別人把依賴塞進來」；Service Locator 是「自己去要」，兩者相反。
2. DI 不需要容器。自己在 `Main` 裡 `new` 完整串，叫 Pure DI。
3. 只替 Volatile Dependency（資料庫、檔案、網路、時間、亂數、還沒寫好的東西）開 Seam；Stable 的直接用。
4. 插座比喻：換電器＝LSP、空插頭＝Null Object、UPS＝Decorator、延長線＝Composite、轉接頭＝Adapter。
5. DI 有三個維度：組裝、生命週期、攔截。真正的好處多半來自攔截。

---

## 第 2 章：Writing tightly coupled code（寫出緊耦合的程式）

> 對應原書 p.34–51

### 這章在幹嘛

這章是**反面教材**。主角 Mary 想做一個「標準三層式」電商網站，結果每一層都緊緊黏在一起。作者帶你看她怎麼一步步寫壞，再分析為什麼壞。

骨架是三段：

1. Mary 依序寫資料層 → 領域層 → UI 層（每一步都看起來很合理）
2. 評估：畫依賴圖、問「每個模組能不能單獨拿出來換」
3. 分析：問題根源在哪裡，加上四個其他小問題

第 3 章會用 DI 把同一個程式整個重寫。作者說急著看好程式的人可以跳過這章。

---

### 重點 1：鬆耦合是用來對付複雜度的

> **原文引用（p.34–35）**
> "DI is a way to enable loose coupling, and loose coupling is first and foremost an efficient way to deal with complexity."

軟體除了業務邏輯，還要處理安全、診斷、維運、效能、擴充。鬆耦合讓你把每個關注點**分開處理**，不要變成一團「big ball of mud」——但最後還是得把它們**組起來**。

---

### 重點 2：畫三層圖很容易，做對很難

> **原文引用（p.35）**
> "Drawing a three-layer diagram is deceptively simple, but the act of drawing the diagram is akin to stating that you'll have sauce béarnaise with your steak: it's a declaration of intent that carries no guarantee with regard to the final result."

三層：UI layer → Domain layer → Data access layer。這是 n-layer 架構最常見的版本。

**主角設定**：Mary Rowan，34 歲，11 年資歷的 .NET 資深工程師，常加班趕 deadline，買了很多書沒時間讀。

**需求**：用 ASP.NET Core MVC＋Entity Framework Core＋SQL Server 做電商網站。第一個功能：列出「精選商品」，**如果使用者是 preferred customer（優惠客戶），所有價格打 95 折**。

---

### 重點 3：資料層——`Product` 和 `CommerceContext`

```csharp
public class Product                      // 一列資料
{
    public Guid Id { get; set; }          // C# property：自帶 getter/setter 的欄位
    public string Name { get; set; }
    public string Description { get; set; }
    public decimal UnitPrice { get; set; }
    public bool IsFeatured { get; set; }
}

public class CommerceContext : DbContext  // EF Core 的入口
{
    public DbSet<Product> Products { get; set; }

    protected override void OnConfiguring(DbContextOptionsBuilder builder)
    {
        var config = new ConfigurationBuilder() ... .AddJsonFile("appsettings.json").Build();
        builder.UseSqlServer(config.GetConnectionString("CommerceConnectionString"));
    }
}
```

**名詞**：
- **Entity Framework Core（EF Core）**：微軟的 ORM（Object/Relational Mapper），幫你把 C# 物件轉成 SQL，不用自己寫 SQL
- **`DbContext`**：EF 的核心 class，是一個 **Unit of Work**（一次業務交易所需物件的本地快取）
- **NuGet**：.NET 的套件管理（像 pip、apt）

書中特別警告：**在 `CommerceContext` 裡自己讀設定檔是個陷阱**。

> **原文引用（p.38）**
> "It causes every new CommerceContext to read the configuration file, even though the configuration file typically doesn't change while an application is running. A CommerceContext shouldn't contain a hard-coded connection string, but neither should it load a configuration value from the configuration system."

---

### 重點 4：領域層——同事 Jens 的爛建議

> **原文引用（p.39）**
> "Implementing domain logic in either the UI or data access layers will lead to pain and suffering. Do yourself a favor and create a domain layer from the beginning."

Mary 問同事 Jens：「怎麼判斷是不是優惠客戶？」Jens 說：在 UI 用 ASP.NET 的 `HttpContext.User` 查角色，再把結果用 `bool` 傳給領域層，「這樣才符合 Separation of Concerns、SRP，這是 Agile 的做法！」

作者直接點名 Jens 不懂裝懂，丟了三個 buzzword：

> **原文引用（p.40）**
> "As the discussion takes him away from his comfort zone, he steamrolls Mary with a triple combo of buzzwords. Be aware that Jens doesn't know what he's talking about"

- 他誤用 Separation of Concerns：把安全邏輯搬到 UI 層，並沒有分離關注點
- 提 Agile 只是因為最近聽人講過
- SRP 是設計原則，跟用什麼開發方法論無關

**要記住的觀點**：業務規則（誰算優惠客戶）就該放在領域層。拿 SRP 當理由把 code 放錯地方是藉口。

---

### 重點 5：SRP 與 Cohesion（內聚力）

> **原文引用（p.40）**
> "the Single Responsibility Principle (SRP) states that each class should only have a single responsibility, or, better put, a class should have only one reason to change."

例子：把 SQL 寫進含 HTML 的 view 裡。SQL 會因資料模型、效能調整而改；HTML 會因外觀改版而改。**改的時間、頻率、理由都不同**，所以違反 SRP。

難判斷時，從**內聚力**看：

> **原文引用（p.40）**
> "Cohesion is defined as the functional relatedness of the elements of a class or module. The lower the relatedness, the lower the cohesion, and the higher the risk a class violates the SRP."

但也不要看到就拆：

> **原文引用（p.41）**
> "It isn't wise to apply the SRP if there are no symptoms. Needlessly splitting up classes that cause no maintainability problems can add extra complexity. The trick in software design is to manage complexity."

---

### 重點 6：`ProductService` —— 在建構子裡 `new` 資料庫

```csharp
public class ProductService
{
    private readonly CommerceContext dbContext;

    public ProductService()
    {
        this.dbContext = new CommerceContext();     // ← 問題的根源
    }

    public IEnumerable<Product> GetFeaturedProducts(bool isCustomerPreferred)
    {
        decimal discount = isCustomerPreferred ? .95m : 1;
        var featured = from p in this.dbContext.Products where p.IsFeatured select p;
        return from p in featured.AsEnumerable()
               select new Product { ..., UnitPrice = p.UnitPrice * discount };
    }
}
```

（`from … where … select` 是 C# 的 LINQ 查詢語法；`IEnumerable<T>` 是「可以逐一走訪的 T 序列」。）

Mary 覺得很滿意：資料技術、設定、領域邏輯都包在 `ProductService` 裡了。但她沒發現：

> **原文引用（p.42）**
> "What Mary doesn't realize is that by letting the ProductService depend on the data access layer's CommerceContext class, she tightly coupled her domain layer to the data access layer."

---

### 重點 7：UI 層——controller 裡又 `new` 一次

**ASP.NET Core MVC 速成**：MVC＝Model View Controller。請求進來 → **controller** 處理（可能用到領域 model）→ 交給 **view** 輸出 HTML。controller 繼承 `Controller`，裡面的 **action method**（例如 `HomeController.Index`）處理每個請求。

```csharp
public ViewResult Index()
{
    bool isPreferredCustomer = this.User.IsInRole("PreferredCustomer");
    var service = new ProductService();                 // 又是 new
    var products = service.GetFeaturedProducts(isPreferredCustomer);
    this.ViewData["Products"] = products;
    return this.View();
}
```

> **原文引用（p.43）**
> "In this case, HomeController is tightly coupled to ProductService. This wouldn't be that bad if ProductService was a Stable Dependency but, as you learned in chapter 1, ProductService is a Volatile Dependency. It's Volatile because it introduces a requirement to set up and configure a relational database."

view（Razor 語法，HTML 裡混 C#）裡還要把 `ViewData["Products"]` 轉型回 `IEnumerable<Product>`，再格式化價格。

按 F5，網頁出來了。Mary 覺得三層都做完了。

---

### 重點 8：每加一個 reference，就多一個依賴

> **原文引用（p.44）**
> "Visual Studio makes it easy and natural to work with solutions and projects in this way. If you need functionality from a different library, you can easily add a reference to it and write code that creates new instances of the types defined in the other libraries. Every time you add a reference, though, you take on a Dependency."

畫出依賴圖（Figure 2.6）：**UI 同時依賴領域層和資料層**；領域層依賴資料層。看起來 UI 某些時候可以繞過領域層。

---

### 重點 9：評估法——「每個模組能不能單獨換掉？」

> **原文引用（p.45）**
> "To evaluate Mary's implementation, we can ask a simple question: Is it possible to use each module in isolation?"

作者強調：問「能不能換」只是**評估耦合的技巧**，就算你永遠不換，這種分析也會挖出耦合問題。

| 情境 | 結果 | 原因 |
|---|---|---|
| 換 UI（web → WPF 桌面程式） | ✅ 可以 | 沒有人依賴 web UI |
| 換資料層（SQL Server → Azure Table Storage） | ❌ 不行 | UI 和領域層都依賴資料層；拿掉它，**整個程式什麼都不剩** |
| 換領域層 | 不用問 | 領域層是程式的心臟，換掉它就沒有存在理由 |

（Azure Table Storage 是 key-value 型資料庫，沒有 schema、沒有參照完整性，走 HTTP，跟 EF Core 存取的關聯式資料庫完全不同。）

> **原文引用（p.46）**
> "Enabling the profit-maximizing cloud abilities requires a major rewrite of the application because none of the existing modules can be reused."

---

### 重點 10：根源分析——不是 UI 的錯，是 `new CommerceContext()`

第一個嫌疑犯：`GetFeaturedProducts` 回傳的 `Product` 定義在資料層，所以 UI 也得引用資料層。

但就算把 UI → 資料層這條線剪掉（Figure 2.9），**還是換不掉資料層**，因為領域層仍然依賴它。

真正的元兇：

```csharp
this.dbContext = new CommerceContext();
```

> **原文引用（p.48）**
> "This tightly couples the ProductService class to the data access layer. There's no reasonable way you can Intercept this piece of code and replace it with something else. The reference to the data access layer is hard-coded into the ProductService class!"

**要記住的觀點**：在 class 裡 `new` 一個 Volatile Dependency，就把它焊死了。

---

### 重點 11：第 1 章的五個好處全部失去

| 好處 | Mary 的程式為什麼沒有 |
|---|---|
| Late binding | 不能同時部署「本機 SQL Server 版」和「Azure 版」 |
| Extensibility | 加 Cross-Cutting Concern（例如第 1 章的安全功能）要改很多 class |
| Maintainability | 每加一個橫切關注點，被改到的 class 就更複雜、更難讀 |
| Parallel development | 改動遍布全程式 → merge conflict 變多 |
| Testability | 換不掉資料層 → 沒有資料庫就不能測 → 不是 Testable |

---

### 重點 12：理想的依賴圖——把領域層和資料層的箭頭反過來

> **原文引用（p.49）**
> "For the highest degree of reuse, the lowest amount of dependencies is desirable. On the other hand, the application would become rather useless if there were no dependencies at all."

既然不打算換領域層，其他層依賴它就是安全的。目標（Figure 2.10）：

```text
UI layer ──────► Domain layer ◄────── Data access layer
```

**資料層改成依賴領域層**，而不是反過來。這就是下一章的 Dependency Inversion。

---

### 重點 13：其他四個問題

> **原文引用（p.50）**
> "Most of the domain model seems to be implemented in the data access layer."

1. **`Product` 放錯層**：技術上的問題是領域層引用資料層；概念上的問題是 `Product` 這種 class 該屬於領域模型
2. **「誰是優惠客戶」寫在 UI**：這是業務邏輯，該放領域層。在同一個 library 裡也完全可以遵守 SRP
3. **`CommerceContext` 自己讀設定檔**：對使用者來說，這個依賴完全看不見

> **原文引用（p.50）**
> "Although the ability to configure a compiled application is important, only the finished application should rely on configuration files. It's more flexible for reusable libraries to be imperatively configurable by their callers, instead of reading configuration files themselves."

4. **view 做太多事**：轉型、字串格式化，應該移到 model

---

### 重點 14：為什麼這麼容易寫出緊耦合

> **原文引用（p.50）**
> "Even when Mary set out with the express intent of writing a three-layer application, it turned into a largely monolithic piece of Spaghetti Code. (When we're talking about layering, we call this Lasagna.)"

> **原文引用（p.50）**
> "If you need a new instance of an object, you can use the new keyword. If you don't have a reference to the required assembly, Visual Studio makes it easy to add. But every time you use the new keyword, you introduce a tight coupling."

**要記住的觀點**：語言和工具都在把你往緊耦合推。不是所有緊耦合都不好，但要避免緊耦合到 Volatile Dependency。

---

### 本章 Summary（作者原話）

> **原文引用（p.51）**
> - "Complex software must address lots of different concerns, such as security, diagnostics, operations, performance, and extensibility."
> - "Loose coupling encourages you to address all application concerns in isolation, but ultimately you must still compose this complex set of concerns."
> - "It's easy to create tightly coupled code. Although not all tight coupling is bad, tight coupling to Volatile Dependencies is and should be avoided."
> - "In Mary's application, because the domain layer depended on the data access layer, there was no way to replace the data access layer with a different one. The tight coupling in her application caused Mary to lose the benefits that loose coupling provides: late binding, extensibility, maintainability, Testability, and parallel development."
> - "Only the finished application should rely on configuration files. Other parts of the application shouldn't request values from a configuration file, but should instead be configurable by their callers."
> - "The Single Responsibility Principle states that each class should only have one reason to change."
> - "The Single Responsibility Principle can be viewed from the perspective of cohesion. Cohesion is defined as the functional relatedness of the elements of a class or module. The lower the amount of relatedness, the lower the cohesion; and the lower the cohesion, the greater the chance a class violates the Single Responsibility Principle."

### 我的整理：本章帶走的四句話

1. 三層圖畫出來不等於三層架構；看依賴箭頭才算數。
2. 評估耦合的方法：問「拿掉這個模組，還剩什麼？」
3. 元兇常常只是一行 `new`——在 class 裡 `new` 一個 Volatile Dependency。
4. 只有最終的應用程式才讀設定檔；library 應該由呼叫者把設定值塞進來。

---

## 第 3 章：Writing loosely coupled code（寫出鬆耦合的程式）

> 對應原書 p.52–80

### 這章在幹嘛

把第 2 章 Mary 的電商程式**從頭重寫**成鬆耦合版本，同樣的功能（精選商品＋優惠客戶 95 折）。第一次正式用上 Constructor Injection、Method Injection、Composition Root 三個 pattern，並介紹 **Dependency Inversion Principle**。

骨架：

1. 目標：把「領域層 → 資料層」的箭頭反過來
2. 由外往內寫：UI → 領域層 → 資料層 → `IUserContext` 的 Adapter → 在 Composition Root 組起來
3. 分析新的依賴圖，再回答第 2 章「能不能換」的問題

---

### 重點 1：不建議大改寫，但這裡為了教學從頭來

> **原文引用（p.53）**
> "You shouldn't infer from this decision that it's impossible to refactor an existing application towards DI or that we encourage you to rewrite existing applications completely from scratch. Big rewrites are costly and high risk. Preferred are slow, step-by-step refactorings."

目標依賴圖：

```text
UI layer ──► Domain layer ◄── Data access layer
```

Mary 用了 4 個 class；新版會有 **9 個 class＋3 個 interface**（Figure 3.3）：

| 層 | class／interface |
|---|---|
| UI | `HomeController`、`FeaturedProductsViewModel`、`ProductViewModel`、`AspNetUserContextAdapter` |
| Domain | `IProductService`、`ProductService`、`DiscountedProduct`、`Product`、`IProductRepository`、`IUserContext` |
| Data access | `SqlProductRepository`、`CommerceContext` |

---

### 重點 2：由外往內（outside-in），不是 Mary 的由內往外

> **原文引用（p.54）**
> "When we write software, we prefer to start in the most significant place — the part that has most visibility to our stakeholders. As in Mary's e-commerce application, this is often the UI. From there, we work our way in, adding more functionality until one feature is done; then we move on to the next."

- Mary 從資料層開始（inside-out），要到很後面才能跑起來
- 作者從 UI 開始，很快就有可以給利害關係人看的版本 → 回饋比較快
- 跟 **YAGNI（You Aren't Gonna Need It）** 有關：只做需要的功能，越簡單越好
- 作者實際是用 TDD 寫的，但測試不放在書裡（在附的原始碼裡）

---

### 重點 3：UI——用 view model 讓 view 變笨

新 view 只剩迴圈：

```html
@model FeaturedProductsViewModel
<h2>Featured Products</h2>
<div>
@foreach (ProductViewModel product in this.Model.Products)
{
    <div>@product.SummaryText</div>
}
</div>
```

格式化邏輯搬進 `ProductViewModel`：

```csharp
public class ProductViewModel
{
    private static CultureInfo PriceCulture = new CultureInfo("en-US");
    public ProductViewModel(string name, decimal unitPrice)
    {
        this.SummaryText = string.Format(PriceCulture, "{0} ({1:C})", name, unitPrice);
    }
    public string SummaryText { get; }
}
```

**作者抓到 Mary 的 bug**：`ToString("C")` 會照瀏覽器的語系格式化貨幣。美國人看到 `$`，丹麥人看到丹麥克朗符號——但數字一樣，丹麥人就用很便宜的價格買到東西。所以要明確指定 `en-US`。

**名詞**：**POCO（Plain Old CLR Object）**：不依賴任何外部框架的單純 class。**DTO（Data Transfer Object）**：在行程之間搬資料的物件。

> **原文引用（p.58）**
> "From a DI point of view, POCOs, DTOs, and view models like FeaturedProductsViewModel and ProductViewModel aren't really interesting. They don't contain any behavior you might want to Intercept, replace, or mock. They are mere data objects. This makes them safe to create in your code"

**要記住的觀點**：資料物件（POCO、DTO、view model、Entity）直接 `new` 沒關係；有行為的服務才需要注入。

---

### 重點 4：`HomeController` 改用 Constructor Injection

先寫死假資料做出可以跑的 stub，再改成：

```csharp
public class HomeController : Controller
{
    private readonly IProductService productService;

    public HomeController(IProductService productService)
    {
        if (productService == null) throw new ArgumentNullException("productService");
        this.productService = productService;
    }

    public ViewResult Index()
    {
        IEnumerable<DiscountedProduct> products = this.productService.GetFeaturedProducts();
        var vm = new FeaturedProductsViewModel(
            from product in products select new ProductViewModel(product));
        return this.View(vm);
    }
}
```

對 `HomeController` 來說，`ProductService` 是 Volatile Dependency（還在開發中、還不存在），所以要開 Seam。

作者坦承第一次聽到 Constructor Injection 也不懂好處在哪：

> **原文引用（p.60）**
> "Doesn't it push the burden of controlling the Dependency onto some other class? Yes, it does — and that's the whole point. In an n-layer application, you can push that burden all the way to the top of the application into a Composition Root."

---

### 重點 5：Composition Root（組裝根）第一次登場

> **原文引用（p.60）**
> "This level of modularity can be achieved by centralizing the creation of our classes into a single place. We call this location the Composition Root."

> **原文引用（p.60）**
> "The Composition Root is located as close as possible to the application's entry point. In most .NET Core application types, the entry point is the Main method. Inside the Composition Root, you can decide to compose your application manually — that's using Pure DI — or to delegate it to a DI Container."

加了帶參數的建構子之後，MVC 不知道怎麼建 `HomeController`，首頁會壞。但**建 controller 不是 UI 層的事**，是 Composition Root 的事（書中註明：Composition Root 不屬於 UI 層，即使放在同一個 assembly）。

（**assembly**：.NET 編譯出來的 `.dll`／`.exe`，大致對應一個專案。）

---

### 重點 6：領域層——`IProductService` 和 `DiscountedProduct`

```csharp
public interface IProductService
{
    IEnumerable<DiscountedProduct> GetFeaturedProducts();
}

public class DiscountedProduct          // POCO，沒有 interface
{
    public DiscountedProduct(string name, decimal unitPrice) { ... }
    public string Name { get; }
    public decimal UnitPrice { get; }
}
```

> **原文引用（p.62）**
> "Programming to interfaces doesn't mean that all classes should implement an interface. It typically makes little sense to hide POCOs, DTOs, and view models behind an interface, because they contain no behavior that requires mocking, Interception, or replacement."

---

### 重點 7：Repository 與 Entity——資料存取的抽象放在領域層

```csharp
public interface IProductRepository      // 定義在「領域層」
{
    IEnumerable<Product> GetFeaturedProducts();
}
```

- **Repository pattern**：資料存取的常見抽象。書中註明這裡用的是業界「常見用法」，跟 Fowler 原書描述不太一樣
- `IProductRepository` 回傳「原始」Entity；`IProductService` 加上業務邏輯（折扣），轉成比較窄的物件
- 只定義現在需要的方法：「加功能比刪功能容易」

**Entity（實體）**：來自 Domain-Driven Design。

> **原文引用（p.63）**
> "Any .NET object instance has an in-memory address (identity), but an Entity has an identity that lives across process lifetimes."

例：商品用資料庫的 primary key 識別，電腦重開還在 → `Product` 是 Entity。

> **原文引用（p.63）**
> "By letting ProductService depend on IProductRepository, we allow behavior to be replaced or Intercepted. By placing that behavior in a different library, we allow a whole library to be replaced."

折扣邏輯搬進 `Product` 本身：

```csharp
public class Product
{
    public string Name { get; set; }
    public decimal UnitPrice { get; set; }
    public bool IsFeatured { get; set; }

    public DiscountedProduct ApplyDiscountFor(IUserContext user)
    {
        bool preferred = user.IsInRole(Role.PreferredCustomer);
        decimal discount = preferred ? .95m : 1.00m;
        return new DiscountedProduct(name: this.Name, unitPrice: this.UnitPrice * discount);
    }
}
```

---

### 重點 8：`ProductService`——兩個依賴，一個 Constructor、一個 Method Injection

```csharp
public class ProductService : IProductService
{
    private readonly IProductRepository repository;
    private readonly IUserContext userContext;

    public ProductService(IProductRepository repository, IUserContext userContext) { ... }

    public IEnumerable<DiscountedProduct> GetFeaturedProducts()
    {
        return from product in this.repository.GetFeaturedProducts()
               select product.ApplyDiscountFor(this.userContext);   // Method Injection
    }
}

public interface IUserContext { bool IsInRole(Role role); }
public enum Role { PreferredCustomer }
```

**Method Injection（方法注入）**：把依賴當方法參數傳進去。適合 Entity 這種短命物件需要依賴的情況（第 4 章細講）。

---

### 重點 9：使用者身分是「情境資訊」，要藏在抽象後面

Mary 是在 UI 判斷完傳一個 `bool`。新版改成 `ProductService` 自己透過 `IUserContext` 去問。理由：

- 判斷誰是優惠客戶屬於領域邏輯
- 不想每個 controller 都去收集使用者資訊——重複、容易錯、可能造成安全 bug
- `HomeController` 不會不小心把 `false` 傳成 `true`

> **原文引用（p.65）**
> "To reduce the overall complexity of a system, runtime data that describes contextual information is best hidden behind an Abstraction and injected into a consumer that requires it to function. Contextual information is metadata about the current request. This is typically information that the user shouldn't be allowed to influence directly. Examples are the user's identity (which was established on login) and the system's current time."

BCL 雖然有 `IPrincipal`，但太通用，不合這個程式的需要 → 自己定義 `IUserContext`。

---

### 重點 10：Dependency Inversion Principle（依賴反轉原則）——還有「抽象歸誰所有」

> **原文引用（p.67）**
> "This principle states that higher-level modules in our applications shouldn't depend on lower-level modules; instead, modules of both levels should depend on Abstractions."

`ProductService`（高層）和 `SqlProductRepository`（低層）都依賴 `IProductRepository`。

> **原文引用（p.67）**
> "the Dependency Inversion Principle prescribes what we would like to accomplish, and DI states how we would like to accomplish it."

**很多人不知道的另一半**：

> **原文引用（p.67）**
> "Not only does the principle prescribe loose coupling, it states that Abstractions should be owned by the module using the Abstraction. In this context, 'owned' means that the consuming module has control over the shape of the Abstraction, and it's distributed with that module, rather than with the module that implements it."

所以 `IUserContext`、`IProductRepository` 都放在**使用它們的**領域層，實作分別在 UI 層和資料層（Figure 3.11）。好處：高層模組不用依賴低層，抽象也剛好符合自己的需要、比較簡單。

**作者自己承認的違規**：`IProductService` 是 UI 在用、領域層在實作，照理該歸 UI 所有。但搬到 UI 會讓領域層依賴 UI；要正確修好得再多兩個專案。**為了務實，作者選擇不修**。

**對照 C**：Linux kernel 裡 `struct file_operations` 定義在 VFS（使用者）那邊、driver（實作者）去填，正是「抽象歸使用者所有」。（筆記自己的對照，書中未提。）

---

### 重點 11：interface 還是 abstract class？

> **原文引用（p.68）**
> "With relation to DI, the reassuring answer is that it doesn't matter. The important part is that you program against some sort of abstraction."

但寫應用程式時，作者偏好 interface：

1. abstract class 容易被濫用成 base class，長成 **God Object**（什麼都知道、什麼都做的物件，anti-pattern）；interface 逼你走「**Composition over Inheritance**（組合優於繼承）」
2. 一個 class 可以實作多個 interface，但 .NET 只能繼承一個 base class
3. interface 寫起來比較簡潔，不用寫 `abstract`、`public`

**例外：寫 reusable library 時**。interface 加成員是 breaking change；abstract class 可以之後加非抽象成員。所以 .NET Framework Design Guidelines 偏好 abstract class。

> **原文引用（p.69）**
> "Reusable libraries within the .NET ecosystem are typically distributed through NuGet. An important characteristic is that their clients aren't known at compile time."

同一個 solution 裡被多個專案用的專案（例如領域層），**不算** reusable library。

---

### 重點 12：資料層——`CommerceContext` 變成實作細節，連線字串明確注入

```csharp
public class SqlProductRepository : IProductRepository
{
    private readonly CommerceContext context;
    public SqlProductRepository(CommerceContext context) { ... }

    public IEnumerable<Product> GetFeaturedProducts()
    {
        return from product in this.context.Products where product.IsFeatured select product;
    }
}

public class CommerceContext : DbContext
{
    private readonly string connectionString;
    public CommerceContext(string connectionString)   // 不再自己讀設定檔
    {
        if (string.IsNullOrWhiteSpace(connectionString)) throw new ArgumentException(...);
        this.connectionString = connectionString;
    }
    public DbSet<Product> Products { get; set; }
    protected override void OnConfiguring(DbContextOptionsBuilder builder)
    {
        builder.UseSqlServer(this.connectionString);
    }
}
```

- 資料層以外沒有人知道 EF 的存在，可以整個換掉
- 資料層直接重用領域層的 `Product`（EF Core 允許 **persistence ignorant** 的 Entity：不依賴任何持久化框架的 POCO）。以後不合用，可以在資料層內部加自己的持久化物件再轉換，不影響其他層

---

### 重點 13：`AspNetUserContextAdapter`——框架相關的實作放 UI 層

```csharp
public class AspNetUserContextAdapter : IUserContext
{
    private static HttpContextAccessor Accessor = new HttpContextAccessor();

    public bool IsInRole(Role role)
    {
        return Accessor.HttpContext.User.IsInRole(role.ToString());
    }
}
```

怎麼取得目前使用者，跟框架高度相關（ASP.NET Core 跟 Windows service 完全不同）。領域層、資料層都不該知道框架 → 放 UI 層最合適。

這是 **Adapter pattern**（結構型模式，同類還有 Composite、Decorator、Facade、Proxy）：把應用程式自訂的 `IUserContext` 轉成 ASP.NET Core 的 API。

> **原文引用（p.73）**
> "Implementations of the Adapter pattern are typically quite straightforward, but don't be surprised if the Adapter contains complex conversions. The idea is that this complexity is hidden from the client."

---

### 重點 14：在 Composition Root 組出 object graph

```csharp
new HomeController(
    new ProductService(
        new SqlProductRepository(
            new CommerceContext(connectionString)),
        new AspNetUserContextAdapter()));
```

> **原文引用（p.74）**
> "In an object-oriented application, groups of objects form a network through their relationships with each other, either through a direct reference to another object or through a chain of intermediate references. These groups of objects are referred to as object graphs."

**執行流程**（Figure 3.14）：
1. 程式啟動：`Startup` 從設定檔讀連線字串，建一個自訂的 **controller activator**（MVC 用來建 controller 的工廠，第 7 章細講）
2. 每個請求進來：activator 的 `Create()` 由內而外 `new` 出整串物件，回傳 `HomeController`
3. MVC 呼叫 `Index()` → `ProductService` → `SqlProductRepository` → 回傳 `ViewResult`

---

### 重點 15：新依賴圖——領域層不依賴任何人

> **原文引用（p.75）**
> "The domain model no longer has any dependencies and can act as a standalone module. On the other hand, the data access layer now has a dependency; in Mary's application, it had none."

回答第 2 章的兩個問題：

| 問題 | 第 2 章 | 第 3 章 |
|---|---|---|
| web UI 換成 WPF？ | ✅ | ✅ 一樣可以 |
| SQL Server 換成 Azure Table？ | ❌ | ✅ 只要新的資料層也實作 `IProductRepository`（之後講怎麼用 late binding 載入） |

---

### 重點 16：關於 DI Container，以及「會不會看不到全貌」

> **原文引用（p.77）**
> "A DI Container is a software library that provides DI functionality and automates many of the tasks involved in Object Composition, Interception, and Lifetime Management. DI Containers are also known as Inversion of Control (IoC) containers."

- 第一版時作者**所有程式都用容器**，覺得 Pure DI 不實際；現在想法變了，所以第二版更強調 pattern
- 組裝 object graph 跟 logging 一樣，是不直接產生業務價值的基礎建設，用通用函式庫有時合理

> **原文引用（p.77）**
> "Don't expect a DI Container to magically change tightly coupled code into loosely coupled code. A DI Container can make your Composition Root more maintainable, but for an application to become maintainable, it must first be designed with DI patterns and techniques in mind."

**常見抱怨：用 DI 之後看不出誰呼叫誰**。作者的回答：

> **原文引用（p.78）**
> "When practicing Pure DI, the Composition Root typically contains this information in a coherent way. Even better, it gives you a view of the complete object graph, not just the direct Dependencies of a class, which is what you get with tightly coupled code."

但換成 DI Container 就可能失去這個全貌，因為容器是在執行期用 reflection 建 object graph。作者說程式設計得好時，這個損失比較不明顯。第 12.3 節會比較兩者。

---

### 本章 Summary（作者原話）

> **原文引用（p.78–79）**
> - "Refactoring existing applications towards a more maintainable, loosely coupled design is hard. Big rewrites, on the other hand, are often riskier and expensive."
> - "The use of view models can simplify the view, because the incoming data is shaped specifically for the view."
> - "Because views are harder to test, the dumber the view, the better. It also simplifies the work of a UI designer who might work on the view."
> - "When you limit the amount of Volatile Dependencies within the domain layer, you get a higher degree of decoupling, reuse, and Testability."
> - "When building applications, the outside-in approach facilitates more rapid prototyping, which can shorten the feedback cycle."
> - "When you want a high degree of modularity in your application, you need to apply the Constructor Injection pattern and build object graphs in the Composition Root, which is located close to the application's entry point."
> - "Programming to interfaces is a cornerstone of DI. It allows you to replace, mock, and Intercept a Dependency, without having to make changes to its consumers. When implementation and Abstraction are placed in different assemblies, it enables whole libraries to be replaced."
> - "Programming to interfaces doesn't mean that all classes should implement an interface. Short-lived objects, such as Entities, view models, and DTOs, typically contain no behavior that requires mocking, Interception, decoration, or replacement."
> - "With respect to DI, it doesn't matter whether you use interfaces or purely abstract classes. From a general development perspective, as authors, we typically prefer interfaces over abstract classes."
> - "A reusable library is a library that has clients that aren't known at compile time. Reusable libraries are typically shipped via NuGet. Libraries that only have callers within the same (Visual Studio) solution aren't considered to be reusable libraries."
> - "DI is closely related to the Dependency Inversion Principle. This principle implies that you should program against interfaces, and that a layer must be in control over the interfaces it uses."
> - "The use of a DI Container can help in making the application's Composition Root more maintainable, but it won't magically make tightly coupled code loosely coupled. For an application to become maintainable, it must be designed with DI patterns and techniques in mind."

### 我的整理：本章帶走的五句話

1. 把箭頭反過來：介面放在**使用它的那一層**（DIP 的後半句），實作放在外圍層。
2. 每個有行為的 class 都用建構子宣告它要什麼；誰來 `new`，推到最頂端的 Composition Root。
3. 資料物件（Entity、DTO、view model）不用 interface，直接 `new`。
4. 使用者身分、現在時間這種「情境資訊」，藏在抽象後面注入，不要一路當參數傳。
5. Pure DI 的 Composition Root 就是整個程式的接線圖；換成容器可能看不到。

---

## 第 4 章：DI patterns（DI 的四個模式）

> 對應原書 p.83–123

### 這章在幹嘛

Part 2「型錄」的第一章。把前面已經用過的手法正式命名，每個 pattern 都照「問題 → 解法 → 怎麼運作 → 何時用 → 已知用例 → 範例」的格式寫。四個 pattern：

| Pattern | 一句話 |
|---|---|
| **Composition Root** | 在哪裡、怎麼組 object graph |
| **Constructor Injection** | class 用建構子靜態宣告它需要的依賴 |
| **Method Injection** | 每次操作依賴或消費者都可能不同時，用方法參數傳 |
| **Property Injection** | 有 Local Default 時，讓呼叫者用可寫屬性覆蓋它 |

> **原文引用（p.84）**
> "The most important patterns are Composition Root and Constructor Injection, which you should use in most situations — the other patterns become more specialized as the chapter progresses."

---

### 重點 1：Composition Root——盡量靠近程式進入點

> **原文引用（p.85）**
> "Where should we compose object graphs? As close as possible to the application's entry point."

> **原文引用（p.85）**
> "A Composition Root is a single, logical location in an application where modules are composed together."

每個 class 都用 Constructor Injection 把「建立依賴」往上推，但不能無限往上推，總要有個地方真的 `new`。那個地方就是 Composition Root。

> **原文引用（p.85）**
> "If you use a DI Container, the Composition Root should be the only place where you use the DI Container. Using a DI Container outside the Composition Root leads to the Service Locator anti-pattern"

console 版的例子：`Main` 讀命令列參數拿連線字串，呼叫 `CreateController(connectionString)` 組出整串物件。

> **原文引用（p.87）**
> "the Composition Root isn't a method or a class, it's a concept. It can be part of the Main method, or it can span multiple classes, as long as they all reside in a single module."

---

### 重點 2：Composition Root 不屬於 UI 層

> **原文引用（p.87）**
> "Assemblies are a deployment artifact: you split code into multiple assemblies to allow code to be deployed separately. An architectural layer, on the other hand, is a logical artifact: you can group multiple logical artifacts in a single deployment artifact."

- 放在同一個 assembly 沒關係：「放 Composition Root 的 assembly」依賴全部，但「UI 層」本身不依賴資料層
- 也可以把 UI 搬到另一個專案，這樣 UI class 就不可能誤用資料層；但 ASP.NET Core 的 view、前端資源不好搬
- **Composition Root 不會被重用**，它專屬於這個應用程式

**Composition Root 是唯一知道 object graph 結構的地方**，所以有一個推論：

> **原文引用（p.88）**
> "This also means that application code can't pass on Dependencies to other threads that run parallel to the current operation, because a consumer has no way of knowing whether it's safe to do so. Instead, when spinning off concurrent operations, it's the job of the Composition Root to create a new object graph for each concurrent operation."

**要記住的觀點**：應用程式碼交出的不只是依賴的控制權，還有「對依賴的認識」。要開新 thread，就請 Composition Root 另外組一份。

---

### 重點 3：在 Composition Root 用容器——一次 `Resolve` 組完整串

- 容器只能當「組 object graph 的引擎」，不能當 Service Locator
- 只有 Composition Root 引用容器；其他模組完全不知道容器存在
- 很多人怕「每次請求組一整串物件」很慢。作者說：**幾乎從來不是問題**（少數情況第 8.4.2 節有解法）
- 請求型程式（網站、服務）：容器設定一次，每個請求 resolve 一次

---

### 重點 4：Pure DI 的 Composition Root 實作——`Startup`＋`CommerceControllerActivator`

ASP.NET Core 的 `Main` 把大部分工作交給 `Startup` class，夠靠近進入點，就拿它當 Composition Root：

```csharp
public class Startup
{
    public void ConfigureServices(IServiceCollection services)   // 只跑一次
    {
        services.AddMvc();
        services.AddHttpContextAccessor();
        var connectionString = this.Configuration.GetConnectionString("CommerceConnection");
        services.AddSingleton<IControllerActivator>(
            new CommerceControllerActivator(connectionString));   // 換掉預設的 activator
    }
}

public class CommerceControllerActivator : IControllerActivator
{
    private readonly string connectionString;
    public CommerceControllerActivator(string connectionString) { ... }

    public object Create(ControllerContext ctx)        // 每個請求呼叫一次
    {
        Type type = ctx.ActionDescriptor.ControllerTypeInfo.AsType();
        if (type == typeof(HomeController))
            return new HomeController(
                new ProductService(
                    new SqlProductRepository(new CommerceContext(this.connectionString)),
                    new AspNetUserContextAdapter()));
        else
            throw new Exception("Unknown controller.");
    }
}
```

> **原文引用（p.90）**
> "Any well-designed framework provides the appropriate Seams to intercept the creation of framework types. These Seams are usually shaped as factory abstractions, as is MVC's IControllerActivator."

**讀設定 vs. 組物件也要分開**：`Startup` 讀設定檔，`CommerceControllerActivator` 只拿到設定**值**、不碰設定**系統** → 沒有設定檔也能測。Composition Root 分散在這兩個 class 裡（Figure 4.2），但都在同一個模組，這是正常的。

---

### 重點 5：「依賴爆炸」是錯覺

常見抱怨：用了 Composition Root，進入點就要引用全部 assembly；以前只要引用下一層。

作者的反駁：**assembly 依賴有遞移性（transitive）**。Mary 的 UI 依賴領域、領域依賴資料，所以 UI 實際上也依賴資料（部署時就感受得到）。

| | 看起來的依賴數 | 實際依賴數 |
|---|---|---|
| Mary 的緊耦合程式 | 3 | **6**（進入點其實也依賴所有 library） |
| 鬆耦合程式 | 5 | 5 |

> **原文引用（p.93）**
> "Remember, though, that dependencies aren't defined by the number of modules, but the number of times each module depends on another module."

專案越多，緊耦合的依賴數爆炸得越厲害。

---

### 重點 6：Constructor Injection——怎麼寫

> **原文引用（p.95）**
> "How do we guarantee that a necessary Volatile Dependency is always available to the class we're currently developing? By requiring all callers to supply the Volatile Dependency as a parameter to the class's constructor."

標準寫法四件事：

```csharp
public class HomeController
{
    private readonly IProductService service;      // 1. readonly 欄位

    public HomeController(IProductService service) // 2. 唯一的 public 建構子
    {
        if (service == null)                       // 3. Guard Clause
            throw new ArgumentNullException("service");
        this.service = service;                    // 4. 存起來，別的什麼都不做
    }
}
```

> **原文引用（p.96）**
> "Constrain the design to a single (public) constructor. Because the constructor is the definition of a class's Dependencies, it makes little sense to have multiple definitions. Overloaded constructors lead to ambiguity: which constructor should the caller (or a DI Container) use?"

> **原文引用（p.96）**
> "Keep the constructor free of any other logic to prevent it from performing any work on Dependencies. ... This makes the construction of your classes fast and reliable."

為什麼要 Guard Clause：interface 是 reference type，呼叫者可以傳 `null` 照樣編得過。編譯器＋Guard Clause 一起保證：建構子沒丟例外，依賴就一定在。之後其他方法**不用再檢查 null**。

---

### 重點 7：Local Default vs. Foreign Default

> **原文引用（p.97）**
> "A Local Default is a default implementation of a Dependency that originates in the same module or layer."

- 寫領域服務時，你心裡可能已經想好要用 SQL 版 Repository。但它在**別的 assembly**，拿它當預設值＝硬引用別的模組 → **Foreign Default**，這就是 Control Freak anti-pattern（第 5 章）
- Local Default 通常出現在 **Strategy pattern** 的實作上（同一個 library 裡的預設策略）

> **原文引用（p.97）**
> "A Local Default with Dependencies becomes a Foreign Default when one of its Dependencies is a Foreign Default. Transitivity strikes again."

---

### 重點 8：Constructor Injection 是預設選擇（Table 4.1）

| Advantages（優點） | Disadvantages（缺點） |
|---|---|
| Injection guaranteed（保證注入） | 採用 Constrained Construction anti-pattern 的框架會讓它難用（要求無參數建構子的舊框架） |
| Easy to implement（容易實作） | |
| Statically declares a class's Dependencies（靜態宣告依賴） | |

**「一次建整串物件不會很慢嗎？」** 作者說通常只有幾十個物件，.NET 建物件極快，瓶頸會在別處。

Steven 的真實故事：有人換容器後每個請求快了 300～400 ms。一查，某些 object graph **超過 19,000 個物件**，很多 class 有 20 個以上的依賴。

> **原文引用（p.99）**
> "In a well-designed system, classes only have a few Dependencies (up to four or five), and this makes object graphs quite narrow. Object graphs tend to get deeper in well-designed systems because of the ease with which you can apply multiple layers of Decorators. But, in the end, the number of objects in the graphs of well-designed systems will stay within a few hundred at most."

**BCL 裡的例子**：`StreamWriter(Stream stream)`、`StreamReader`——`Stream` 是 abstract class，傳 null 會丟 `ArgumentNullException`。作者補充：reusable library 有多個建構子常常合理，應用程式的 class 不合理。

---

### 重點 9：範例——加上幣別轉換 `ICurrencyConverter`

需求：客戶想用不同幣別付款。

```csharp
public interface ICurrencyConverter
{
    Money Exchange(Money money, Currency targetCurrency);
}
public class Currency { public readonly string Code; ... }
public class Money { public readonly decimal Amount; public readonly Currency Currency; ... }
```

匯率來源多半是 web service 或資料庫（out-of-process）→ 實作放資料層 → **沒有合理的 Local Default** → Constructor Injection：`ProductService` 加第三個建構子參數。用的時候直接用，不用檢查是否存在。

---

### 重點 10：依賴幾乎不該是 optional——用 Null Object

> **原文引用（p.102）**
> "Dependencies should hardly ever be optional. Optional Dependencies complicate the consuming component with null checks. Instead, make Dependencies required, and create and inject Null Object implementations in cases where there's no reasonable implementation available for the required Dependency."

典型例子：logging。某個 class 支援 log，但這次部署不需要它 log。不要在 class 裡到處 `if (logger != null)`，而是注入一個 `NullLogger`：

```csharp
public class NullLogger : ILogger
{
    public void Log(LogEntry e) { /* Do nothing. */ }
}
```

Null Object 通常是空的；如果必須回傳值，就回傳最簡單的正確值。

---

### 重點 11：Method Injection——依賴在呼叫時才給

> **原文引用（p.104）**
> "Method Injection supplies a consumer with a Dependency by passing it as method argument on a method called outside the Composition Root."

常見形式：一個「正常的值」＋一個「情境」參數：

```csharp
public decimal CalculateDiscountPrice(decimal price, IUserContext context)
{
    if (context == null) throw new ArgumentNullException("context");
    decimal discount = context.IsInRole(Role.PreferredCustomer) ? .95m : 1;
    return price * discount;
}
```

| Advantages | Disadvantages |
|---|---|
| 呼叫者可以提供這次操作專屬的情境 | 適用範圍有限 |
| 能把依賴注入到**不是在 Composition Root 建立的**資料型物件 | 依賴會變成 class 或其抽象的公開 API 的一部分 |

兩種典型用法：
1. **每次呼叫的消費者不同**（最常見）
2. **每次呼叫的依賴不同**

---

### 重點 12：Method Injection 用法 1——Entity 需要依賴

DDD 風格的 Entity 會把資料和行為放在一起，但 Entity 不是在 Composition Root 建的（是 Repository 從資料庫讀出來的）。如果用 Constructor Injection，每個 Entity 都要帶著所有方法的依賴才能建立，測試也麻煩。

```csharp
public class Customer
{
    public Guid Id { get; private set; }
    public string Name { get; private set; }

    public void RedeemVoucher(Voucher voucher, IVoucherRedemptionService service)
    {
        ...
        service.ApplyRedemptionForCustomer(voucher, this.Id);
    }

    public void MakePreferred(IEventHandler handler)
    {
        ...
        handler.Publish(new CustomerMadePreferred(this.Id));
    }
}

public class CustomerServices : ICustomerServices
{
    // 服務本身用 Constructor Injection 拿到 repository 和 service
    public void RedeemVoucher(Guid customerId, Voucher voucher)
    {
        var customer = this.repository.GetById(customerId);
        customer.RedeemVoucher(voucher, this.service);     // 再用 Method Injection 傳給 Entity
        this.repository.Save(customer);
    }
}
```

同一個 `IVoucherRedemptionService` 被傳給很多不同的 `Customer` → **依賴不變，消費者一直換**。

---

### 重點 13：Method Injection 用法 2——外掛（add-in）需要情境

繪圖軟體讓大家寫影像特效外掛：

```csharp
public interface IImageEffectAddIn
{
    Bitmap Apply(Bitmap source, IApplicationContext context);
}

// 呼叫端
foreach (IImageEffectAddIn effect in this.effects)
    result = effect.Apply(result, this.context);
```

**什麼時候不該用**：如果依賴對呼叫者來說只是實作細節，就別讓它污染抽象，改用 Constructor Injection。不然會從 object graph 頂端一路往下傳，到處都要改。

> **原文引用（p.111）**
> "When we recommend that Constructor Injection should be your preferred DI pattern, we're assuming that you generally build applications. On the other hand, if you're building a framework, Method Injection can be useful because it lets the framework pass information about the context to add-ins."

BCL 裡的例子：`IDesignerHost.GetDesigner(IComponent)`、`TypeConverter.ConvertTo(ITypeDescriptorContext context, …)`、ASP.NET Core 的 `IValidationAttributeAdapterProvider.GetAttributeAdapter(attribute, IStringLocalizer)`。

---

### 重點 14：Temporal Coupling（時序耦合）——不要在 Composition Root 用 Method Injection

> **原文引用（p.109）**
> "Temporal Coupling is a common problem in API design. It occurs when there's an implicit relationship between two or more members of a class, requiring clients to invoke one member before the other."

最典型的就是 `Initialize()` 方法：

```csharp
public class Component
{
    private ISomeInterface dependency;
    public void Initialize(ISomeInterface dependency) { this.dependency = dependency; }

    public void DoSomething()
    {
        if (this.dependency == null)                       // 每個 public 方法都要加
            throw new InvalidOperationException("Call Initialize first.");
        this.dependency.DoStuff();
    }
}

var c = new Component();
c.DoSomething();      // 編得過，執行時爆
```

BCL 也有：`EndpointAddressBuilder` 不先設 `Uri` 就呼叫 `ToEndpointAddress()`，編得過、執行時失敗，API 完全沒提示。

解法：改成 Constructor Injection。

> **原文引用（p.110）**
> "Don't store injected method Dependencies. This leads to Temporal Coupling, Captive Dependencies, or hidden side effects. A method should use the Dependency or pass it on, and should refrain from storing such a Dependency."

**對照 C**：Linux driver 裡「先 `foo_init()` 才能 `foo_xxx()`」、每個函式開頭 `if (!priv->initialized) return -EINVAL;` 就是同一種病。（筆記自己的對照，書中未提。）

---

### 重點 15：範例——把幣別轉換搬進 `Product` Entity

轉換商品幣別在很多地方都會用到，Mary 把它集中到 `Product`：

```csharp
public class Product
{
    public string Name { get; set; }
    public Money UnitPrice { get; set; }
    public bool IsFeatured { get; set; }

    public Product ConvertTo(Currency currency, ICurrencyConverter converter)   // Method Injection
    {
        ... // Guard Clauses
        var newUnitPrice = converter.Exchange(this.UnitPrice, currency);
        return this.WithUnitPrice(newUnitPrice);
    }

    public Product WithUnitPrice(Money unitPrice) => new Product { Name = this.Name, UnitPrice = unitPrice, ... };
}

// ProductService
return from product in this.repository.GetFeaturedProducts()
       select product.ConvertTo(currency, this.converter).ApplyDiscountFor(this.userContext);
```

> **原文引用（p.113）**
> "Unlike the other DI patterns in this chapter, you mainly use Method Injection when you want to supply Dependencies to an already existing consumer. With Constructor Injection and Property Injection, on the other hand, you supply Dependencies to a consumer while it's being created."

---

### 重點 16：Property Injection——有好的預設值時才用

> **原文引用（p.114）**
> "Property Injection allows a Local Default to be replaced via a public settable property. Property Injection is also known as Setter Injection."

最陽春的寫法很脆弱：

```csharp
public class Consumer
{
    public IDependency Dependency { get; set; }   // 不能 readonly
}

var instance = new Consumer();
instance.DoSomething();          // 忘了設 → NullReferenceException
```

要寫穩，得處理兩件事：
1. 建構子先設好預設值，setter 加 Guard Clause
2. 用內部旗標，只允許初始化期間設定——Eric Lippert 叫這 **Popsicle immutability**（冰棒式不可變：先能改，凍住後就不能改）

| Advantages | Disadvantages |
|---|---|
| Easy to understand | Not entirely simple to implement robustly |
| | Limited applicability |
| | Only applicable to reusable libraries |
| | Causes Temporal Coupling |

> **原文引用（p.117）**
> "When building applications, on the other hand, we never use Property Injection, and you should do so sparingly. Even though you might have a Local Default for a Dependency, Constructor Injection still provides you with a better alternative."

> **原文引用（p.118）**
> "Prevent the use of Property Injection as a solution to Constructor Over-injection. Classes with many Dependencies are a code smell and Property Injection won't lower the class's complexity."

想用 Property Injection 解循環依賴？那也是 code smell（第 6 章）。

---

### 重點 17：Open/Closed Principle（插入說明）

> **原文引用（p.116）**
> "They're open for extension. ... They're closed for modification. This means that when you extend the system, you must be able to do so without touching any of the existing source code."

DI 讓你替換或攔截 class 來加功能，而消費者和依賴都不知道。但作者坦承「完全不改舊碼」是達不到的理想：

> **原文引用（p.116）**
> "As a developer, it's your job to find out what kind of changes are the most likely to occur in your application. Based on the understanding of how you expect a particular application or system to evolve, you should model it in such a way that you maximize maintainability."

OCP 跟 **DRY（Don't Repeat Yourself）** 密切相關。只想留一個擴充點、預設什麼都不做時，Local Default 可以是 Null Object。

---

### 重點 18：Property Injection 範例——Simple Injector 的 `ConstructorResolutionBehavior`

BCL 裡 Property Injection 反而比 Constructor Injection 常見（例如 `IComponent.Site`），因為 BCL 很多地方有好的 Local Default。

作者用自己的容器 Simple Injector 當例子（reusable library）：預設只接受「只有一個 public 建構子」的 class，但可以換掉這個行為：

```csharp
var container = new Container();
container.Options.ConstructorResolutionBehavior = new CustomConstructorResolutionBehavior();
```

```csharp
public class ContainerOptions
{
    IConstructorResolutionBehavior resolutionBehavior =
        new DefaultConstructorResolutionBehavior();          // Local Default

    public IConstructorResolutionBehavior ConstructorResolutionBehavior
    {
        get { return this.resolutionBehavior; }
        set
        {
            if (value == null) throw new ArgumentNullException("value");
            if (this.Container.HasRegistrations)              // Popsicle immutability
                throw new InvalidOperationException("... cannot be changed after the first registration ...");
            this.resolutionBehavior = value;
        }
    }
}
```

為什麼註冊後不准改：註冊時容器已經用這個行為驗證過建構子；之後換掉可能讓前面的驗證失效。重新驗證成本高又難做，所以乾脆禁止。

---

### 重點 19：怎麼選 pattern（Figure 4.9 決策流程）

```text
需要這個依賴，還是要把手上的依賴傳出去？
├─ 要傳出去（例如給外掛情境）────────────► Method Injection
└─ 自己需要
   └─ 誰需要？
      ├─ 資料型物件（Entity）──────────────► Method Injection
      └─ 元件（component）
         └─ 什麼程式碼？
            ├─ 應用程式 ─────────────────► Constructor Injection
            └─ reusable library
               └─ 有 Local Default 嗎？
                  ├─ 有 ─────────────────► Property Injection
                  └─ 沒有 ───────────────► Constructor Injection
```

> **原文引用（p.120）**
> "These patterns aren't interchangeable. In most cases, your default choice should be Constructor Injection, but there are situations where one of the other patterns affords a better alternative. ... if in doubt, choose Constructor Injection. You can't go terribly wrong with that choice."

> **原文引用（p.121）**
> "When it comes to writing application code, even the use of Local Defaults should be prevented in favor of having these defaults set in one central place in the application — the Composition Root."

**要記住的觀點**：寫應用程式時，幾乎只用 Constructor Injection；Method Injection 留給 Entity 和外掛；Property Injection 留給 reusable library。

---

### 本章 Summary（作者原話）

> **原文引用（p.122–123）**
> - "The Composition Root is a single, logical location in an application where modules are composed together. The construction of your application's components should be concentrated into this single area of your application."
> - "Only startup projects will have a Composition Root."
> - "Although a Composition Root can be spread out across multiple classes, they should be in a single module."
> - "The Composition Root takes a direct dependency on all other modules in the system. Loosely coupled code that applies the Composition Root pattern lowers the overall number of dependencies between modules, subsystems, and layers, compared to tightly coupled code."
> - "Even though you might place the Composition Root in the same assembly as your UI or presentation layer, the Composition Root isn't part of those layers. Assemblies are deployment artifacts, whereas layers are logical artifacts."
> - "Where a DI Container is used, it should only be referenced from the Composition Root. All other modules should be oblivious to the existence of the DI Container."
> - "Use of a DI Container outside the Composition Root leads to the Service Locator anti-pattern."
> - "The performance overhead of using a DI Container to compose large object graphs is usually not an issue in a well-designed system."
> - "The Composition Root should be the sole place in the entire application that knows about the structure of the constructed object graphs. This means that application code can't pass on Dependencies to other threads that run parallel to the current operation, because a consumer has no way of knowing whether it's safe to do so. Instead, when spinning off concurrent operations, it's the Composition Root's job to create a new object graph for each concurrent operation."
> - "Constructor Injection is the act of statically defining the list of required Dependencies by specifying them as parameters to the class's constructor."
> - "A constructor that's used for Constructor Injection should do no more than apply Guard Clauses and store the receiving Dependencies. Other logic should be kept out of the constructor. This makes building object graphs fast and reliable."
> - "Constructor Injection should be your default choice for DI, because it's the most reliable and the easiest to apply correctly."
> - "Constructor Injection is well suited when a Dependency is required. It's important to note, however, that Dependencies should hardly ever be optional. Optional Dependencies complicate the component with null checks. Inside the Composition Root, a Null Object implementation should instead be injected when there's no reasonable implementation available."
> - "Application components should only have a single constructor. Overloaded constructors lead to ambiguity. For reusable class libraries, like the BCL, having multiple constructors often makes sense; for application components, it doesn't."
> - "Method Injection is the act of passing Dependencies on method invocations."
> - "Where either a Dependency or a Dependency's consumer can differ for each operation, you can apply Method Injection. This can be useful for add-in scenarios where some runtime context needs to be passed along to the add-in's public API, or when a data-centric object requires a Dependency for a certain operation, as will often be the case with domain Entities."
> - "Method Injection is unsuited for use inside the Composition Root because it leads to Temporal Coupling."
> - "A method that accepts a Dependency through Method Injection shouldn't store that Dependency. This leads to Temporal Coupling, Captive Dependencies, or hidden side effects. Dependencies should only be stored with Constructor Injection and Property Injection."
> - "A Local Default is a default implementation of a Dependency that originates in the same module or layer."
> - "Property Injection allows class libraries to be open for extension, because it lets callers change the library's default behavior."
> - "Property Injection may look simple, but when properly implemented, it tends to be more complex compared to Constructor Injection."
> - "Beyond optional Dependencies within reusable libraries, the applicability of Property Injection is limited, and Constructor Injection is usually a better fit. Constructor Injection simplifies the class, allows the Composition Root to be in control over the value that all consumers get, and prevents Temporal Coupling."

### 我的整理：本章帶走的五句話

1. Composition Root 是概念不是 class：靠近進入點、在同一個模組、唯一知道整張 object graph 的地方，容器只准出現在這裡。
2. Constructor Injection 的建構子只做兩件事：Guard Clause、存欄位。只能有一個 public 建構子。
3. 不要有 optional 依賴；沒有實作就注入 Null Object。
4. Method Injection 給 Entity 和外掛用；拿到的依賴不准存起來，否則就是 Temporal Coupling。
5. Property Injection 只在 reusable library＋有 Local Default 時用；寫應用程式作者「從來不用」。

---

## 第 5 章：DI anti-patterns（DI 的四個反模式）

> 對應原書 p.124–162

### 這章在幹嘛

第 4 章講「該怎麼做」，這章講「常見的錯法」。四個 anti-pattern 依重要性排列，每個都給例子、分析、怎麼重構回正確的 pattern。作者說四個他們自己都犯過。

| Anti-pattern | Description（說明） |
|---|---|
| **Control Freak** | 跟 IoC 相反：自己直接控制依賴 |
| **Service Locator** | 一個隱性的服務可以供應依賴，但不保證一定有 |
| **Ambient Context** | 用 static 存取點提供某一個依賴 |
| **Constrained Construction** | 假設所有建構子都長同一個樣子 |

> **原文引用（p.126）**
> "If you decide to read only part of this chapter, we recommend that you read Control Freak and Service Locator."

Control Freak 最常見；Service Locator 最危險，因為它**看起來像在解決問題**。

---

### 重點 1：anti-pattern 的定義

> **原文引用（p.125）**
> "An anti-pattern is a commonly occurring solution to a problem, which generates decidedly negative consequences, although other documented solutions that prove to be more effective are available."

開場比喻：油熱到冒煙不代表鍋子好了，代表油已經過了 smoke point 開始變質——看似合理、很常見、結果有害。

作者也承認：重構 legacy code 時，anti-pattern 有時是合適的**暫時**解法；比原本好，但它仍然是 anti-pattern。

---

### 重點 2：Control Freak——在 Composition Root 以外依賴 Volatile Dependency

> **原文引用（p.127）**
> "The Control Freak anti-pattern occurs every time you depend on a Volatile Dependency in any place other than a Composition Root. It's a violation of the Dependency Inversion Principle"

```csharp
public ViewResult Index()
{
    var service = new ProductService();     // Control Freak
    ...
}
```

> **原文引用（p.127）**
> "In general, the new keyword isn't suddenly 'illegal,' but you should refrain from using it to get instances of Volatile Dependencies outside the Composition Root. Also, be aware of static classes. Static classes can also be Volatile Dependencies."

- 對 Stable Dependency `new` 沒問題
- **static class 也可能是 Volatile Dependency**：你不會 `new` 它，但依賴它問題一樣

---

### 重點 3：Control Freak 變形 1——宣告成 interface，卻自己 `new`

```csharp
private readonly IProductRepository repository;

public ProductService()
{
    this.repository = new SqlProductRepository();   // 型別是 interface，但永遠是 Sql 版
}
```

聽說過「對 interface 寫程式」但不懂背後原因的人很常這樣寫。變數型別是抽象，執行期卻永遠是同一個具體型別，不改 code 重編就換不掉。

---

### 重點 4：Control Freak 變形 2——三種 factory 都救不了

Mary 和 Jens 又登場了，依序試三種 factory：

| Factory | 寫法 | 為什麼失敗 |
|---|---|---|
| **Concrete Factory** | `new ProductRepositoryFactory().Create()` | 只是把 `new SqlProductRepository()` 搬到另一個 class，`ProductService` 還是控制 factory 的壽命 |
| **Abstract Factory** | `IProductRepositoryFactory` | 原本要一個 `IProductRepository`，現在變成要一個 `IProductRepositoryFactory`——同一個問題 |
| **Static Factory** | `static ProductRepositoryFactory.Create()` 讀設定檔 `switch` | 看起來成功了，其實最糟 |

書中特別說明：不是反對 Concrete Factory 本身，它能封裝複雜的建立邏輯、避免重複，只是對 DI 沒有幫助。Abstract Factory 在 DI 裡常被濫用，第 6 章再談。

**Static Factory 為什麼最糟**（Figure 5.1、5.2）：

```csharp
public static IProductRepository Create()
{
    ... // 讀 appsettings.json
    switch (repositoryType)
    {
        case "sql":   return new SqlProductRepository();
        case "azure": return new AzureProductRepository();
        default: throw new InvalidOperationException("...");
    }
}
```

1. factory 依賴**所有**具體實作 → `ProductService` 也遞移地依賴它們
2. factory 放領域層 → 領域依賴資料層；放資料層 → `ProductService` 還是要依賴資料層。**不管放哪都是循環依賴**，唯一解是全部塞一個專案
3. 部署在 Azure 也得帶著 SQL 版的 dll
4. 要加第三種實作就得改 factory 重編：**可設定（configurable）但不可擴充（extensible）**，別的團隊沒有原始碼就加不了
5. 不能換成測試用的實作（測試需要在執行期決定實例）

> **原文引用（p.132）**
> "If we were the consultants in this example, we wouldn't be at all happy. In fact, such a solution was suggested on a project Mark was involved with"

Mark 參與的是一個 Fortune 500 公司核心業務的大案子，他太晚加入，建議被否決。後來案子勉強交差，但被視為失敗、有人因此丟了工作。作者說不能全怪沒用 DI，但這種做法反映了缺乏正確設計。

---

### 重點 5：Control Freak 變形 3——多載建構子＋Foreign Default

```csharp
public ProductService()
    : this(new SqlProductRepository())       // 正式環境用這個（Foreign Default）
{
}

public ProductService(IProductRepository repository)   // 測試用這個
{
    ...
}
```

（`: this(...)` 是 C# 的建構子串接，Constructor Chaining。）

> **原文引用（p.134）**
> "A Foreign Default is the opposite of a Local Default. It's an implementation of a Volatile Dependency that's used as a default, even though it's defined in a different module than its consumer."

看起來兩全其美：測試能塞假的，正式環境方便建立。但領域層還是被綁在 SQL 資料層上，而且正式環境裡就沒辦法攔截那個 repository 了。

---

### 重點 6：Control Freak 的傷害與重構步驟

> **原文引用（p.135–136）**
> "Control Freak is the most common DI anti-pattern. It represents the default way of creating instances in most programming languages, so it can be observed even in applications where developers have never considered DI. It's such a natural and deeply rooted way to create new objects that many developers find it difficult to discard."

傷害：
- 只能在幾個預先寫好的依賴裡選，不能隨意替換，也不能用編譯後才寫的實作
- 消費模組難以重用：會拖著不想要的依賴（例如透過 Foreign Default 依賴 ASP.NET Core，就不能拿去做 Windows service 或手機 app）
- 平行開發變難
- 不能用 Test Double

**重構三步**：
1. 確定是對抽象寫程式（必要時先抽出 interface）
2. 把散在各處的建立動作集中到**一個**建立方法，回傳型別寫抽象
3. 把這個建立動作移出消費者，改用 DI pattern（通常是 Constructor Injection）

---

### 重點 7：Service Locator——給 Composition Root 以外的程式「無上限」的依賴

> **原文引用（p.138）**
> "A Service Locator supplies application components outside the Composition Root with access to an unbounded set of Volatile Dependencies."

```csharp
public class HomeController : Controller
{
    public HomeController() { }                        // 無參數建構子
    public ViewResult Index()
    {
        IProductService service = Locator.GetService<IProductService>();
        ...
    }
}
```

> **原文引用（p.138）**
> "Years ago, it was quite controversial to call Service Locator an anti-pattern. The controversy is over: Service Locator is an anti-pattern."

**作者的親身經歷**：
- Mark 讀了 Fowler 的文章，覺得 Service Locator 解決了他的問題，還寫了一個叫「Service Locator」的函式庫，2007 年重寫後沒多久就放棄，因為發現它是 anti-pattern
- Steven 受 Common Service Locator（CSL）啟發，寫了「Simple Service Locator」，也很快發現是 anti-pattern，拿掉 CSL 依賴、改名 **Simple Injector**（就是第 14 章那個容器）

---

### 重點 8：Service Locator 的最小實作——就是一個 static 字典

```csharp
public static class Locator
{
    private static Dictionary<Type, object> services = new Dictionary<Type, object>();

    public static void Register<T>(T service) { services[typeof(T)] = service; }
    public static T GetService<T>() { return (T)services[typeof(T)]; }
    public static void Reset() { services.Clear(); }
}
```

（`<T>` 是 C# generic，泛型型別參數；`typeof(T)` 取得型別物件。）

測試要這樣寫：

```csharp
Locator.Reset();                                       // 不然上一個測試會影響這個
Locator.Register<IProductRepository>(stub);
var sut = new ProductService();                        // Register 和使用之間有 Temporal Coupling
```

---

### 重點 9：為什麼 Service Locator 危險——它「幾乎」能用

> **原文引用（p.142）**
> "Service Locator is a dangerous pattern because it almost works."

拿第 1 章的五個好處檢查，它幾乎都過關：late binding ✅、平行開發 ✅、可維護（較難）、可測試 ✅。只有一個地方不行，而且很嚴重：**重用性**。

1. **多拖一個多餘的依賴**：要重用 `ProductService`，還得帶上 `Locator`（只是為了機制而存在）
2. **看不出它需要什麼**：IntelliSense（Visual Studio 的自動完成）只告訴你它有一個無參數建構子（Figure 5.5）；忘了註冊就執行期爆

> **原文引用（p.143）**
> "The problem with Service Locator is that any component using it is being dishonest about its level of complexity. It looks simple as seen through the public API, but it turns out to be complex — and you won't find out until you try to run it."

其他問題：
- 新版本偷偷加依賴，舊程式照樣編得過、執行才失敗 → 很容易不小心造成 breaking change
- 用 generic 看似強型別，其實**是弱型別**：什麼型別都能要，編得過不代表執行不會爆
- static locator 造成 **Interdependent Tests**（測試互相影響），每個測試後都要記得 `Reset()`

---

### 重點 10：不是看 API 長相，而是看「誰在用、在哪用」

`public T Resolve<T>()` 這個簽名，大部分 DI Container 都有。那容器就是 Service Locator 嗎？

> **原文引用（p.144）**
> "It's not the static structure of an API that determines it as a Service Locator, but rather the role the API plays in the application."

- Composition Root 本來就依賴所有東西，不可能「多拖一個依賴」；它的工作就是組 object graph，也沒有對誰「隱藏」依賴
- **應用程式碼**主動去查依賴，才變成 Service Locator

> **原文引用（p.144）**
> "A DI Container encapsulated in a Composition Root isn't a Service Locator — it's an infrastructure component."

**要記住的觀點**：同一個 `Resolve<T>()`，在 Composition Root 裡呼叫是正確用法，在業務 class 裡呼叫就是 Service Locator。

---

### 重點 11：重構 Service Locator——還能換到「驗證」能力

> **原文引用（p.145）**
> "Because Constructor Injection statically declares a class's Dependencies, it enables the code to fail at compile time, assuming you practice Pure DI. When you use a DI Container, on the other hand, you lose the ability to verify correctness at compile time."

- 用容器時，靠「靜態宣告的建構子」仍然可以在**啟動時或測試裡**請容器把所有 object graph 建一遍來驗證；有些容器還能做更複雜的設定分析
- 用 Service Locator 的話，容器根本看不到這些依賴，無法幫你驗證

**重構步驟**：把散落各處的 `GetService` 集中到建構子，存進 `readonly` 欄位 → 把建構子裡的 `GetService` 改成建構子參數 → 拿掉 locator。加參數會打壞呼叫者，所以**從 object graph 最上層的 class 往下改**。

---

### 重點 12：Ambient Context——用 static 屬性提供「一個」依賴

> **原文引用（p.146）**
> "An Ambient Context supplies application code outside the Composition Root with global access to a Volatile Dependency or its behavior by the use of static class members."

Service Locator 給「無上限的一堆」依賴；Ambient Context 只給「一個強型別的」依賴。

```csharp
public string GetWelcomeMessage()
{
    ITimeProvider provider = TimeProvider.Current;     // static 存取點
    DateTime now = provider.Now;
    string partOfDay = now.Hour < 6 ? "night" : "day";
    return string.Format("Good {0}.", partOfDay);
}

public static class TimeProvider
{
    private static ITimeProvider current = new DefaultTimeProvider();   // Local Default
    public static ITimeProvider Current { get { return current; } set { current = value; } }
    private class DefaultTimeProvider : ITimeProvider
    {
        public DateTime Now { get { return DateTime.Now; } }
    }
}
```

**跟 Singleton 的差別**：結構很像，都是 static 成員；但 Ambient Context 允許換掉依賴，Singleton 保證唯一實例永不改變。

> **原文引用（p.146）**
> "The Singleton pattern should only be used either from within the Composition Root or when the Dependency is Stable. On the other hand, when the Singleton pattern is abused to provide the application with global access to a Volatile Dependency, its effects are identical to those of the Ambient Context"

把 `DateTime.Now` 藏在 `ITimeProvider` 後面是**對的**；錯在用 static 屬性讓大家取得它。其他變形：static `GetCurrentTime()` 方法、把 static 存取點和 interface 合成一個 base class、用 `Func<DateTime>` delegate（C# 的函式型別，類似 C 的函式指標）代替 interface。

**對照 C**：全域的函式指標變數（`extern int (*get_time)(void);`）讓測試時換掉，就是 C 版的 Ambient Context。（筆記自己的對照，書中未提。）

---

### 重點 13：Ambient Context 最常見的地方——logging

```csharp
public class MessageGenerator
{
    private static readonly ILog Logger = LogManager.GetLogger(typeof(MessageGenerator));
    public string GetWelcomeMessage()
    {
        Logger.Info("GetWelcomeMessage called.");
        ...
    }
}
```

為什麼這麼普遍：
1. logging 函式庫的文件**第一個範例就這樣寫**——文件為了簡單，不是為了最佳實踐
2. 幾乎每個 class 都要 log，注入到建構子會讓依賴變很多（這是 Constructor Over-injection，第 6 章）

作者引用 Jeff Atwood 2008 年的文章《The Problem With Logging》：
- log 越多 code 越亂
- log 不是免費的，一直寫磁碟
- log 越多越找不到東西
- 值得寫進 log 的，就值得顯示在 UI 上

Jeff 做 Stack Overflow 時拿掉大部分 log，只記沒處理的例外：**是錯誤就該丟例外**。

> **原文引用（p.150）**
> "By no means are we stating that you shouldn't log. Logging is a crucial part of any application, as it is in the applications we build. What we're saying, however, is that you should design your application in such way that there's only a handful of classes in your system affected by logging."

第 10 章會講怎麼設計，讓 logging 只碰到少數 class。

---

### 重點 14：Ambient Context 的四個傷害

> **原文引用（p.150）**
> "The Dependency is hidden. Testing becomes more difficult. It becomes hard to change the Dependency based on its context. There's Temporal Coupling between the initialization of the Dependency and its usage."

1. **藏依賴**：更容易藏住「這個 class 依賴太多」的事實 → 掩蓋 SRP 違規
2. **難測**：它是全域狀態，測試改了會影響其他測試（平行跑更糟），要寫特殊的 teardown
3. **難依情境換實作**：例如某些地方要「請求開始時凍結的時間」，某些長時間操作要即時時間（作者說他們真的遇過跨午夜、日光節約時間造成的 bug）。`GetLogger(typeof(...))` 就是為了這個，要消費者自己傳型別，徒增複雜
4. **時序耦合**：沒在 Composition Root 初始化，就會在第一次使用時才失敗。作者希望程式 **fail fast**

**Steven 的慘痛經驗**：客戶用 Common.Logging 包住 log4net，到處都是 `LogManager.GetLogger`。Common.Logging 有個 bug，在某些開發機的 IIS 裡第一次呼叫 `GetLogger` 就失敗，程式根本起不來——Steven 剛好是中招的兩個人之一。全公司花了無數小時找不到解法，最後他把自己要跑的路徑上所有呼叫都註解掉。

> **原文引用（p.152）**
> "The moral of the story is that if the developers had used proper DI patterns instead of an Ambient Context, it'd have been simple for me to locally swap out the configured logger for a fake one in the Composition Root that didn't require Common.Logging to be loaded. A few minutes of work would have saved the organization countless wasted hours."

---

### 重點 15：重構 Ambient Context——很簡單，難的是 logging 習慣

如果程式已經有 DI，重構很直接：在建構子集中呼叫 → 存進 `readonly` 欄位 → 換成建構子參數＋Guard Clause。結果就是普通的 Constructor Injection：

```csharp
public class WelcomeMessageGenerator
{
    private readonly ITimeProvider timeProvider;
    public WelcomeMessageGenerator(ITimeProvider timeProvider) { ... }
    public string GetWelcomeMessage() { DateTime now = this.timeProvider.Now; ... }
}
```

真正難改的是背後的問題：大量原因不明的 log，原作者早已離職，要判斷哪些可以刪、哪些該改成例外，很慢。

> **原文引用（p.153）**
> "Although less problematic than Control Freak and Service Locator, Ambient Context is typically a cover-up for larger design problems in the application."

沒用 DI 的程式，先修 Control Freak 和 Service Locator，再處理 Ambient Context。

---

### 重點 16：Constrained Construction——為了 late binding 硬性規定建構子長相

> **原文引用（p.154）**
> "Constrained Construction forces all implementations of a certain Abstraction to require their constructors to have an identical signature with the goal of enabling late binding."

```csharp
// Startup 裡
string typeName = settings.GetValue<string>("ProductRepositoryType");
var type = Type.GetType(typeName, throwOnError: true);
var repository = (IProductRepository)Activator.CreateInstance(
    type, new object[] { connectionString });      // 隱含規定：建構子要吃一個 string
```

> **原文引用（p.155）**
> "The ubiquity of databases makes it all too easy to forget that a connection string implicitly represents an implementation choice."

如果實作是 REST 服務或 XML 檔，「連線字串」根本沒意義。除了「要實作 `IProductRepository`」這個明確限制，又多了「建構子要吃一個 string」這個**隱含限制**。作者說 string 還算寬鬆（可以塞各種編碼的資訊）；想像規定要吃 `TimeSpan` 加一個數字有多綁手。

> **原文引用（p.156）**
> "Modeling Dependency construction exclusively on explicit constraints (interface or base class) is a better and more flexible option."

（作者附帶一提：假想的 `XmlProductRepository` 應該吃 `XmlReader`，不是檔名。）

這個 anti-pattern **只在 late binding 時才會出現**；early binding（直接引用）時編譯器會確保不會有隱含限制。

---

### 重點 17：最常見的版本——要求無參數建構子

規定「所有實作都要有無參數建構子」，讓最簡單的 `Activator.CreateInstance(type)` 能用。看似最低公約數，但彈性損失很大：

- 實作只好在內部自己初始化（例如自己讀設定檔）
- **無法共用實例**：例如想讓 `ProductRepository` 和 `ConsumerRepository` 共用同一個 `CommerceContext`（Figure 5.6），只有從外面注入才做得到

> **原文引用（p.157）**
> "The fact that DI allows you to share a single instance among many consumers doesn't mean you should always do so. Sharing an instance saves memory but can introduce interaction-related problems, such as threading issues."

---

### 重點 18：重構 Constrained Construction——Abstract Factory 也不夠，要嘛容器、要嘛別 late bind

想法：改成 late bind 一個 `IProductRepositoryFactory`，只規定 factory 的建構子長相。

```csharp
public class SqlProductRepositoryFactory : IProductRepositoryFactory
{
    public SqlProductRepositoryFactory(IConfigurationRoot configuration) { ... }  // 規定的簽名
    public IProductRepository Create()
    {
        return new SqlProductRepository(
            new AspNetUserContextAdapter(),                  // UI 層的東西
            new CommerceContext(this.connectionString));
    }
}
```

問題：
- factory 要引用所有依賴，所以必須放在**另一個** assembly（不然 SQL 資料層要引用 UI 層，Figure 5.8）
- 每種新組合都要寫新 factory → 它變成 **Abstract Composition Root**
- 核心程式想換或攔截 `IUserContext`，要同時改核心程式和 factory 專案
- 這種 factory 很難管理 Object Lifetime

> **原文引用（p.160）**
> "To combat this inflexibility, the only feasible solution is to use a general-purpose DI Container. Because DI Containers analyze constructor signatures using reflection, the Abstract Composition Root doesn't need to know the Dependencies used to construct its components."

但大部分程式不需要這麼彈性：

> **原文引用（p.160）**
> "If you can get away with recompiling the startup project, you should keep your Composition Root centralized in the startup project. Late binding introduces extra complexity, and complexity increases maintenance costs."

**要記住的觀點**：換資料層時「不用重編領域層、但要改啟動專案」通常就夠了。不要為了不重編啟動專案而引入 late binding。

---

### 本章 Summary（作者原話）

> **原文引用（p.161–162）**
> - "An anti-pattern is a description of a commonly occurring solution to a problem that generates decidedly negative consequences."
> - "Control Freak is the most dominating of the anti-patterns presented in this chapter. It effectively prevents you from applying any kind of proper DI. It occurs every time you depend on a Volatile Dependency in any place other than a Composition Root."
> - "Although the new keyword is a code smell when it comes to Volatile Dependencies, you don't need to worry about using it for Stable Dependencies. In general, the new keyword isn't suddenly illegal, but you should refrain from using it to get instances of Volatile Dependencies."
> - "Control Freak is a violation of the Dependency Inversion Principle."
> - "Control Freak represents the default way of creating instances in most programming languages, so it can be observed even in applications where developers have never considered DI. It's such a natural and deeply rooted way to create new objects that many developers find it difficult to discard."
> - "A Foreign Default is the opposite of a Local Default. It's an implementation of a Dependency that's used as a default even though it's defined in a different module than its consumer. Dragging along unwanted modules robs you of many of the benefits of loose coupling."
> - "Service Locator is the most dangerous anti-pattern presented in this chapter because it looks like it's solving a problem. It supplies application components outside the Composition Root with access to an unbounded set of Volatile Dependencies."
> - "Service Locator impacts the reusability of the components consuming it. It makes it non-obvious to a component's consumers what its Dependencies are, makes such a component dishonest about its level of complexity, and causes its consuming components to drag along the Service Locator as a redundant Dependency."
> - "Service Locator prevents verification of the configuration of relationships between classes. Constructor Injection in combination with Pure DI allows verification at compile time; Constructor Injection in combination with a DI Container allows verification at application startup or as part of a simple automated test."
> - "A static Service Locator causes Interdependent Tests, because it remains in memory when the next test case is executed."
> - "It's not the mechanical structure of an API that determines it as a Service Locator, but rather the role the API plays in the application. Therefore, a DI Container encapsulated in a Composition Root isn't a Service Locator — it's an infrastructure component."
> - "Ambient Context supplies application code outside the Composition Root with global access to a Volatile Dependency or its behavior by using static class members."
> - "Ambient Context is similar in structure to the Singleton pattern with the exception that Ambient Context allows its Dependency to be changed. The Singleton pattern ensures that the single created instance will never change."
> - "Ambient Context is usually encountered when developers have a Cross-Cutting Concern as a Dependency that's used ubiquitously, making them think it justifies moving away from Constructor Injection."
> - "Ambient Context causes the Volatile Dependency to become hidden, complicates testing, and makes it difficult to change the Dependency based on its context."
> - "Constrained Construction forces all implementations of a certain Abstraction to have a particular constructor signature with the goal of enabling late binding. It limits flexibility and might force implementations to do their initialization internally."
> - "Constrained Construction can be prevented by utilizing a general-purpose DI Container because DI Containers analyze constructor signatures using reflection."
> - "If you can get away with recompiling the startup project, you should keep your Composition Root centralized in the startup project and refrain from using late binding. Late binding introduces extra complexity, and complexity increases maintenance costs."

### 我的整理：本章帶走的五句話

1. Control Freak＝在 Composition Root 以外 `new`（或呼叫 static）一個 Volatile Dependency；三種 factory 都只是把問題搬家。
2. Service Locator「幾乎能用」所以最危險：它讓 class 對自己的依賴說謊，錯誤延到執行期。
3. 判斷是不是 Service Locator 看位置，不看 API：`Resolve<T>()` 在 Composition Root 裡是對的。
4. Ambient Context（static 的 `TimeProvider.Current`、`LogManager.GetLogger`）是單一依賴版的 Service Locator；logging 到處都是，代表設計有問題。
5. 能重編啟動專案就別 late binding；硬要的話，用容器避免 Constrained Construction。

---

## 第 6 章：Code smells（程式碼異味）

> 對應原書 p.163–208

### 這章在幹嘛

Part 2 的最後一章，型錄式的「問題 → 解法（重構）」。三個 DI 新手最常遇到的 code smell：

1. **Constructor Over-injection**（建構子參數太多）→ 重構成 Facade Service 或 domain event
2. **濫用 Abstract Factory**（為了管生命週期、為了依執行期資料選依賴）→ 改成 Proxy 或 Adapter
3. **循環依賴** → 拆 class；最後手段才用 Property Injection

> **原文引用（p.164）**
> "A code smell is a hint that something might be wrong, not a certainty. A perfectly good idiom may be considered a code smell because it's often misused, or because there's a simpler alternative that works better in most cases. Calling something a code smell is not an attack; it's a sign that a closer look is warranted."

anti-pattern 是「確定有害的常見解法」；code smell 只是「可能有問題，值得再看一下」。

---

### 重點 1：Constructor Over-injection——不要怪 Constructor Injection

```csharp
public OrderService(
    IOrderRepository orderRepository,
    IMessageService messageService,
    IBillingSystem billingSystem,
    ILocationService locationService,
    IInventoryManagement inventoryManagement)
```

> **原文引用（p.165）**
> "Constructor Injection makes it easy to spot SRP violations. Instead of feeling uneasy about Constructor Over-injection, you should embrace it as a fortunate side effect of Constructor Injection. It's a signal that alerts you when a class takes on too much responsibility."

> **原文引用（p.165）**
> "Our personal threshold lies at four constructor arguments. When we add a third argument, we already begin considering whether we could design things differently, but we can live with four arguments for a few classes."

**錯誤的解法**：

> **原文引用（p.166）**
> "A tempting, but erroneous, attempt to resolve Constructor Over-injection is through the introduction of Property Injection, perhaps even by moving those properties into a base class. Although the number of constructor Dependencies can be reduced by replacing them with properties, such a change doesn't lower the class's complexity, which should be your primary focus."

正確方向：拆成符合設計模式的小 class；Cross-Cutting Concern 用 Decorator 疊上去（第 9 章）而不是注入；真的需要協調很多依賴的入口（粗粒度的 web service 操作、排程批次工作），用下面兩招。

（書中從這裡開始為了篇幅省略大部分 Guard Clause。）

---

### 重點 2：例子——核准訂單要做五件事（Table 6.1）

下單時價格、匯率、折扣都要凍結，所以訂單常交給獨立子系統。

| Action（動作） | Required Dependencies（需要的依賴） |
|---|---|
| 更新訂單 | `IOrderRepository` |
| 寄收據 email 給客戶 | `IMessageService` |
| 通知會計系統發票金額 | `IBillingSystem` |
| 依商品和距離選最適合的倉庫 | `ILocationService`、`IInventoryManagement` |
| 請倉庫揀貨出貨 | `IInventoryManagement` |

```csharp
public void ApproveOrder(Order order)
{
    this.UpdateOrder(order);      // order.Approve(); repository.Save(order);
    this.Notify(order);           // 寄收據、通知會計、Fulfill（找倉庫＋通知倉庫）
}
```

光是核准訂單就五個依賴，其他訂單操作呢？

---

### 重點 3：重構 1——Facade Service：找出「自然的互動群組」

`ILocationService` 和 `IInventoryManagement` 總是一起用，是在「找倉庫並通知它們」。而且 `ILocationService` 只是通知倉庫的實作細節：

```csharp
public interface IOrderFulfillment { void Fulfill(Order order); }

public class OrderFulfillment : IOrderFulfillment
{
    public OrderFulfillment(ILocationService locationService, IInventoryManagement inventoryManagement) { ... }
    public void Fulfill(Order order)
    {
        this.locationService.FindWarehouses(...);
        this.inventoryManagement.NotifyWarehouses(...);
    }
}
```

> **原文引用（p.169）**
> "A Facade Service hides a natural cluster of interacting Dependencies, along with their behavior, behind a single Abstraction."

「訂單履行（order fulfillment）」聽起來本身就是一個領域概念——你把**隱性的概念變成顯性的**。

（Facade Service 跟 **Parameter Object**（把多個參數包成一個物件）有關，差別是 Facade Service 只露出包起來的行為，不露出裡面的成員。）

---

### 重點 4：再進一步——共同介面＋Composite

五個依賴變四個後，發現剩下的「寄收據、通知會計、履行」都是「訂單核准後通知其他系統」：

```csharp
public interface INotificationService { void OrderApproved(Order order); }

public class CompositeNotificationService : INotificationService
{
    IEnumerable<INotificationService> services;
    public CompositeNotificationService(IEnumerable<INotificationService> services) { ... }
    public void OrderApproved(Order order)
    {
        foreach (var service in this.services) service.OrderApproved(order);
    }
}
```

每個都包成新 interface，數量沒變，有什麼用？**因為它們實作同一個介面，就能用 Composite 包成一個**。`OrderService` 剩兩個依賴：

```csharp
public void ApproveOrder(Order order)
{
    this.UpdateOrder(order);
    this.notificationService.OrderApproved(order);
}
```

Composition Root：

```csharp
var notificationService = new CompositeNotificationService(new INotificationService[]
{
    new OrderApprovedReceiptSender(messageService),
    new AccountingNotifier(billingSystem),
    new OrderFulfillment(locationService, inventoryManagement)
});
var orderService = new OrderService(repository, notificationService);
```

> **原文引用（p.173）**
> "Refactoring to Facade Services is more than just a party trick to get rid of too many Dependencies. The key is to identify natural clusters of interaction."

功能完全沒變（真正的重構），但 `OrderService` 的職責在概念上變了：收訂單、存檔、通知其他系統；通知誰、怎麼通知，推到下一層。Facade Service 的 Facade Service 也完全合理。

---

### 重點 5：重構 2——domain event（領域事件）

`INotificationService` 套用到其他 class 後會一直長方法：`OrderCancelled`、`OrderShipped`、`CustomerCreated`……（這也違反 ISP）。每改一次介面，所有實作都要改。

> **原文引用（p.173）**
> "The essence of a domain event is that you use it to capture actions that can trigger a change to the state of the application you're developing"

把每個事件變成**一個型別**（純資料，沒有行為）：

```csharp
public class OrderApproved  { public readonly Guid OrderId; ... }
public class OrderCancelled { public readonly Guid OrderId; ... }
```

結構一樣也要分成不同 class，才能讓處理者強型別地對應特定事件。然後介面泛型化，只剩一個方法：

```csharp
public interface IEventHandler<TEvent>
{
    void Handle(TEvent e);
}

public class OrderFulfillment : IEventHandler<OrderApproved>
{
    public void Handle(OrderApproved e) { ... }
}

public class CompositeEventHandler<TEvent> : IEventHandler<TEvent>
{
    private readonly IEnumerable<IEventHandler<TEvent>> handlers;
    public void Handle(TEvent e) { foreach (var h in this.handlers) h.Handle(e); }
}

// OrderService
this.handler.Handle(new OrderApproved(order.Id));
```

**C# generic 小註**：`IEventHandler<TEvent>` 像一個模板，實作時指定 `TEvent`（例如 `OrderCancelled`），`Handle` 的參數型別就跟著定下來。前面的 `IEnumerable<T>`、`DbSet<T>`、`Dictionary<TKey, TValue>` 都是 generic。

> **原文引用（p.178）**
> "The beauty of a generic interface like IEventHandler<TEvent> is that the addition of new features won't cause any changes to either the interface nor any of the already existing implementations. In case you need to generate an invoice for your approved order, you only have to add a new implementation that implements IEventHandler<OrderApproved>."

DI Container 的 Auto-Registration 可以簡化這種 Composition Root（第 13～15 章）。但 `OrderService` 本身還是會一直長大——第 10 章處理。

**對照 C**：這很像 Linux kernel 的 notifier chain（`blocking_notifier_call_chain`）——發事件的人不知道誰在聽。（筆記自己的對照，書中未提。）

---

### 重點 6：domain event 的額外好處——可靠訊息（reliable messaging）

情境：尖峰時倉庫的 web service timeout，但收據已寄、會計已通知。資料庫可以 rollback，**email 收不回來**。或是 web server 在寄完確認信後當機，會計和倉庫都沒收到，客戶永遠收不到貨。

domain event 可以序列化後放進持久化佇列（MSMQ、Azure Queue、資料庫表）。`OrderService` 只做：

1. 開始交易
2. 在交易內更新訂單
3. 在交易內把事件發佈到持久化佇列
4. commit

commit 後事件才被處理，每個 handler 在自己的交易裡跑，失敗可以單獨重試、也能平行跑。書中註明這會導致 eventual consistency（最終一致性），可以用 Outbox pattern 避免分散式交易；細節超出本書範圍。

> **原文引用（p.180）**
> "no matter which refactoring approach you choose, be it Decorators, Facade Services, domain events, or perhaps another, the important takeaway here is that Constructor Over-injection is a clear sign that code smells. Don't ignore such a sign, but act accordingly."

---

### 重點 7：濫用 Abstract Factory 1——用無參數 `Create()` 自己管生命週期

```csharp
public interface IProductRepositoryFactory
{
    IProductRepository Create();      // 無參數的 factory method
}

public ViewResult Index()
{
    using (IProductRepository repository = this.factory.Create())   // 自己建、自己 Dispose
    {
        var products = repository.GetFeaturedProducts();
        return this.View(products);
    }
}
```

（C# 的 `using (...) { }` 區塊結束時自動呼叫 `Dispose()`，類似 C 的「用完記得 `close()`」，但由語言保證。）

> **原文引用（p.182）**
> "Although an implementation might require deterministic cleanup, that doesn't imply that it should be the responsibility of the consumer to ensure proper cleanup."

---

### 重點 8：Leaky Abstraction（洩漏的抽象）

抽出 interface 時，參數或回傳型別還是原本函式庫的具體型別：

```csharp
public interface IRequestContext
{
    HttpContext Context { get; }     // 還是 ASP.NET 的型別 → 洩漏
}
```

一層層把型別都換成 interface（**Deep Extraction**）也不一定對：

```csharp
public interface IHttpContext
{
    IHttpRequest Request { get; }
    IHttpResponse Response { get; }
    IHttpSession Session { get; }
    IPrincipal User { get; }
}
```

全部都是 interface，但 HTTP 模型還是明顯洩漏出來。正確做法：看**消費者想做什麼**。如果只是要知道使用者角色，就是第 3 章的 `IUserContext { bool IsInRole(Role role); }`——完全看不出是 web 程式，Windows service 也能用。

> **原文引用（p.183）**
> "Always consider whether a given Abstraction makes sense for implementations other than the one you have in mind. If it doesn't, you should reconsider your design."

介面露出無行為的資料物件（Parameter Object、view model、DTO）沒關係——它們定義在介面同一個 library，是抽象的一部分。

---

### 重點 9：無參數 factory method 和 `IDisposable` 抽象都是 Leaky Abstraction

> **原文引用（p.183）**
> "Conceptually, there's only one instance of a service Abstraction. During the lifetime of a consumer, it shouldn't be concerned with the possibility that multiple instances of a Dependency can exist."

無參數 `Create()` 等於告訴消費者「有很多個實例，你自己處理」。但別的實作可能根本不需要多個實例、也不需要 dispose → 洩漏了實作細節。

> **原文引用（p.184）**
> "This doesn't mean that classes shouldn't implement IDisposable. What this does mean, however, is that Abstractions shouldn't implement IDisposable. Because the client only knows about the Abstraction, it can't be responsible for managing the lifetime of that instance. We move this responsibility back to the Composition Root."

> **原文引用（p.183）**
> "The Dependencies created by an Abstract Factory should conceptually require a runtime value, and the translation from a runtime value into an Abstraction should make sense. If you feel the urge to introduce an Abstract Factory because you have a specific implementation in mind, you may have a Leaky Abstraction at hand."

**對照 C**：介面裡有 `close()` 函式指標，代表每個實作都得持有要關的資源——對純計算的實作來說是多餘的。（筆記自己的對照，書中未提。）

---

### 重點 10：解法——直接注入 `IProductRepository`，生命週期交給 Proxy

`HomeController` 回到最單純的 Constructor Injection，只「用」不「管」。誰管 `SqlProductRepository` 的 `Dispose`？**Proxy**：

```csharp
public class SqlProductRepositoryProxy : IProductRepository
{
    private readonly string connectionString;

    public IEnumerable<Product> GetFeaturedProducts()
    {
        using (var repository = this.Create())       // 呼叫時才建，用完就 Dispose
            return repository.GetFeaturedProducts();
    }

    private SqlProductRepository Create() => new SqlProductRepository(this.connectionString);
}

// Composition Root
new HomeController(new SqlProductRepositoryProxy(connectionString));
```

**Proxy pattern**：替另一個物件當代理人，控制對它的存取，可以延後建立成本；實作同一個介面，消費者以為在跟真的講話。

> **原文引用（p.186）**
> "Having factory-like behavior (like the Create method of listing 6.17) is typically unavoidable. Application-wide Factory Abstractions, however, should be viewed with suspicion."

這個 Proxy 跟 `SqlProductRepository` 緊耦合，放在領域層就是 Control Freak → 放資料層，更適合放 **Composition Root**（它在組 object graph 的一部分）。

---

### 重點 11：Interface Segregation Principle（介面隔離原則）

介面方法很多時，寫 Proxy 很煩——但那通常代表違反 ISP。

> **原文引用（p.187）**
> "The Interface Segregation Principle (ISP) states that 'No client should be forced to depend on methods it doesn't use.'"

> **原文引用（p.187）**
> "The ISP can be considered to be the conceptual underpinning of the Single Responsibility Principle (SRP). The ISP states that interfaces should model only a single concept, whereas the SRP states that implementations should have only one responsibility."

- 介面塞太多 → 把你拉向某個特定實作 → 常是 Leaky Abstraction 的徵兆，也更難替換
- 介面變窄 → Proxy、Decorator、Test Double 都好寫（第 10 章會說 ISP 對 AOP 很關鍵）
- 不一定要一個實作對一個介面；實作可以比介面大，實作好幾個小介面

---

### 重點 12：濫用 Abstract Factory 2——依執行期資料選依賴

地圖網站讓使用者選「最短／最快／風景最好」路線：

```csharp
public enum RouteType { Shortest, Fastest, Scenic }
public interface IRouteAlgorithm { RouteResult CalculateRoute(RouteSpecification spec); }

// 直覺解法
public interface IRouteAlgorithmFactory { IRouteAlgorithm CreateAlgorithm(RouteType routeType); }

public ViewResult GetRoute(RouteSpecification spec, RouteType routeType)
{
    IRouteAlgorithm algorithm = this.factory.CreateAlgorithm(routeType);
    var route = algorithm.CalculateRoute(spec);
    ...
}
```

問題在哪？回到 DIP：**抽象該照消費者的需要設計**。這個設計對 `RouteController` 不好：

> **原文引用（p.190）**
> "When you introduce an Abstract Factory, you always increase the number of Dependencies a consumer has. If you only look at the constructor of RouteController, you may be led to believe that the controller only has one Dependency. But IRouteAlgorithm is also a Dependency of RouteController, even if it isn't injected into its constructor."

寫單元測試時馬上感覺到：要測跟 factory 的互動，**也**要測跟 algorithm 的互動。

---

### 重點 13：解法——Adapter 把 factory 和產品合成一個

能用 Proxy 嗎？要 `IRouteAlgorithm` 拿得到 `RouteType` 才行，但演算法本身根本不需要 `RouteType`，概念上很怪。所以改用 Adapter：

```csharp
public interface IRouteCalculator
{
    RouteResult Calculate(RouteSpecification spec, RouteType routeType);
}

public class RouteCalculator : IRouteCalculator
{
    private readonly IDictionary<RouteType, IRouteAlgorithm> algorithms;
    public RouteCalculator(IDictionary<RouteType, IRouteAlgorithm> algorithms) { ... }
    public RouteResult Calculate(RouteSpecification spec, RouteType type)
        => this.algorithms[type].CalculateRoute(spec);
}

// Composition Root
var algorithms = new Dictionary<RouteType, IRouteAlgorithm>
{
    { RouteType.Shortest, new ShortestRouteAlgorithm() },
    { RouteType.Fastest,  new FastestRouteAlgorithm() },
    { RouteType.Scenic,   new ScenicRouteAlgorithm() }
};
new RouteController(new RouteCalculator(algorithms));
```

作者不在 `RouteCalculator` 裡注入 factory（多餘的一層），直接注入演算法字典。

**對照 C**：就是用 enum 當索引的函式指標陣列 `route_algos[ROUTE_FASTEST](spec)`，包在一個函式後面。（筆記自己的對照，書中未提。）

> **原文引用（p.193）**
> "Typically, service Abstractions shouldn't expose other service Abstractions in their definition. This means that a service Abstraction shouldn't accept another service Abstraction as input, nor should it have service Abstractions as output parameters or as a return type. Application services that depend on other application services force their clients to know about both Dependencies."

作者說這是準則不是鐵則（跟 principle of least knowledge 有關），所以算 code smell 不算 anti-pattern。

---

### 重點 14：循環依賴——雞生蛋

```csharp
public class Chicken : IChicken { public Chicken(IEgg egg) { ... } }
public class Egg : IEgg         { public Egg(IChicken chicken) { ... } }

new Chicken(new Egg(???));     // 組不出來
```

> **原文引用（p.195）**
> "The main takeaway from this section, therefore, is this: Dependency cycles are typically caused by an SRP violation."

---

### 重點 15：例子——Mary 加稽核紀錄（audit trail）後出現循環

老闆要求記錄每個使用者改了什麼。Mary 做了：

- `IAuditTrailAppender.Append(Entity)` → `SqlAuditTrailAppender` 需要 `IUserContext`（誰改的）、`CommerceContext`、`ITimeProvider`
- `AspNetUserContextAdapter` 加了 `CurrentUser` 屬性，需要 `IUserRepository.GetByName()`
- `SqlUserRepository.Update()` 要寫稽核 → 需要 `IAuditTrailAppender`

Composition Root 編不過：

```text
Cannot use local variable 'userContext' before it's declared.
```

把宣告往上搬，換別的變數報錯。循環是：

```text
AspNetUserContextAdapter → IUserRepository(SqlUserRepository)
  → IAuditTrailAppender(SqlAuditTrailAppender) → IUserContext(AspNetUserContextAdapter)
```

呼叫圖：`UserService.UpdateMailAddress` → `SqlUserRepository.Update` → `SqlAuditTrailAppender.Append` → `AspNetUserContextAdapter.CurrentUser` → `SqlUserRepository.GetByName`。

**物件圖有循環，但呼叫圖不是遞迴**（`GetByName` 不會再呼叫 `Append`）——如果是遞迴就會 `StackOverflowException`，得改方法。這裡的原因在別處：SRP。

---

### 重點 16：診斷——`IUserRepository` 太寬

| Abstraction | Role（角色） | Methods |
|---|---|---|
| `IAuditTrailAppender` | 記錄使用者做的重要變更 | 1 |
| `IUserContext` | 提供目前請求的使用者資訊 | 2 |
| `IUserRepository` | 用某種持久化技術存取、查詢、儲存使用者 | 3 |

三個看起來都很專注。但 `IUserRepository` 是圍繞「使用者」這個**很廣**的概念分組，會一直被改。

> **原文引用（p.200）**
> "Ever-changing Abstractions are a strong indication of SRP violations."

> **原文引用（p.201）**
> "The more methods a class has, the higher the chance it violates the Single Responsibility Principle. This is also related to the Interface Segregation Principle, which prefers narrow interfaces."

**解法**：把 `GetByName` 抽成 `IUserByNameRetriever`／`SqlUserByNameRetriever`（不依賴 appender）。`AspNetUserContextAdapter` 改依賴它 → 循環斷了。

```csharp
var userContext = new AspNetUserContextAdapter(new SqlUserByNameRetriever(context));
var appender = new SqlAuditTrailAppender(userContext, context, timeProvider);
var repository = new SqlUserRepository(context, appender);
```

**作者插話：稽核其實不該這樣做**。稽核是 Cross-Cutting Concern，Mary 的做法要改一堆 class，違反 OCP 和 SRP。比較簡單：覆寫 `CommerceContext.SaveChanges`，用 EF 的 `ChangeTracker` 查變更。作者更偏好 domain event：記錄的是「使用者改了 email」這種**功能層級**的事件，可以畫出訂單時間軸（建立 → 核准 → 付款 → 出貨 → 送達）。第 10 章還有第三種做法。

---

### 重點 17：打破循環的策略（Table 6.4，依偏好排序）

| Strategy | Description |
|---|---|
| **Split classes（拆 class）** | 大部分情況：把方法太多的 class 拆小 |
| **.NET events** | 把其中一方改成發事件，而不是直接呼叫依賴通知它；特別適合一方只呼叫 `void` 方法時 |
| **Property Injection** | 都不行時，把一個 class 從 Constructor Injection 改成 Property Injection。只治標 |

> **原文引用（p.204）**
> "When we encounter a Dependency cycle, our first question is, 'Where did I fail?' A Dependency cycle should immediately trigger a thorough evaluation of the root cause."

---

### 重點 18：最後手段——Property Injection 或 Virtual Proxy

```csharp
var appender = new SqlAuditTrailAppender(context, timeProvider);   // 先建一個「半初始化」的
var repository = new SqlUserRepository(context, appender);
var userContext = new AspNetUserContextAdapter(new SqlUserByNameRetriever(context));
appender.UserContext = userContext;                                // 事後補上
```

代價：`SqlAuditTrailAppender` 要能處理「依賴還沒到」→ Temporal Coupling。

> **原文引用（p.205）**
> "As we stated previously in section 4.2.1, classes should never perform work involving Dependencies in their constructors. Besides making object construction slow and unreliable, using an injected Dependency might fail, because it may not yet be fully initialized."

不想放寬原本 class 的話，用 **Virtual Proxy**，原本的 class 保持不變：

```csharp
public class LazyAuditTrailAppender : IAuditTrailAppender
{
    public IAuditTrailAppender Appender { get; set; }
    public void Append(Entity changedEntity)
    {
        if (this.Appender == null) throw new InvalidOperationException("Appender was not set.");
        this.Appender.Append(changedEntity);
    }
}

var lazyAppender = new LazyAuditTrailAppender();
var repository = new SqlUserRepository(context, lazyAppender);
...
lazyAppender.Appender = new SqlAuditTrailAppender(userContext, context, timeProvider);
```

> **原文引用（p.204）**
> "Only resort to solving cycles by using Property Injection as a last-ditch effort. It only treats the symptoms instead of curing the illness."

---

### 本章 Summary（作者原話）

> **原文引用（p.206–208）**
> - "Ever-changing Abstractions are a clear sign of Single Responsibility Principle (SRP) violations. This also relates to the Open/Closed Principle that states that you should be able to add features without having to change existing classes."
> - "The more methods a class has, the higher the chance it violates the SRP. This is also related to the Interface Segregation Principle, which states that no client should be forced to depend on methods it doesn't use."
> - "Making Abstractions thinner solves many problems, such as the complexity of creating Proxies, Decorators, and Test Doubles."
> - "A benefit of Constructor Injection is that it becomes more obvious when you violate the SRP. When a single class has too many Dependencies, it's a signal that you should redesign it."
> - "When a constructor's parameter list grows too large, we call the phenomenon Constructor Over-injection and consider it a code smell. It's a general code smell unrelated to, but magnified by, DI."
> - "You can redesign from Constructor Over-injection in many ways, but splitting up a large class into smaller, more focused classes according to well-known design patterns is always a good move."
> - "You can refactor away from Constructor Over-injection by applying Facade Services refactoring. A Facade Service hides a natural cluster of interacting Dependencies with their behavior behind a single Abstraction."
> - "Facade Service refactoring allows discovering these natural clusters and draws previously undiscovered relationships and domain concepts out in the open. Facade Service is related to Parameter Objects but, instead of combining and exposing components, it exposes only the encapsulated behavior while hiding the constituents."
> - "You can refactor away from Constructor Over-injection by introducing domain events into your application. With domain events, you capture actions that can trigger a change to the state of the application you're developing."
> - "A Leaky Abstraction is an Abstraction, such as an interface, that leaks implementation details, such as layer-specific types or implementation-specific behavior."
> - "Abstractions that implement IDisposable are Leaky Abstractions. IDisposable should be put into effect within the implementation instead."
> - "Conceptually, there's only one instance of a service Abstraction. Abstractions that leak this knowledge to their consumers aren't designed with those consumers in mind."
> - "Service Abstractions should typically not expose other service Abstractions in their definition. Abstractions that depend on other Abstractions force their clients to know about both Abstractions."
> - "When it comes to applying DI, Abstract Factories are often overused. In many cases, better alternatives exist."
> - "The Dependencies created by an Abstract Factory should conceptually require a runtime value. The translation from a runtime value into an Abstraction should make sense on the conceptual level. If you feel the urge to introduce an Abstract Factory to be able to create instances of a concrete implementation, you may have a Leaky Abstraction on hand. Instead, the Proxy pattern provides you with a better solution."
> - "Having factory-like behavior inside some classes is typically unavoidable. Application-wide Factory Abstractions, however, should be reviewed with suspicion."
> - "An Abstract Factory always increases the number of Dependencies a consumer has, along with its complexity."
> - "When you use Abstract Factories to select Dependencies based on supplied runtime data, more often than not, you can reduce complexity by refactoring towards Facades that don't expose the underlying Dependency."
> - "Dependency cycles are typically caused by SRP violations."
> - "Improving the design of the part of the application that contains the Dependency cycle should be your preferred option. In the majority of cases, this means splitting up classes into smaller, more focused classes."
> - "Dependency cycles can be broken using Property Injection. You should only resort to solving cycles by using Property Injection as a last-ditch effort. It only treats the symptoms instead of curing the illness."
> - "Classes should never perform work involving Dependencies in their constructors because the injected Dependency may not yet be fully initialized."

### 我的整理：本章帶走的五句話

1. 建構子參數超過 3～4 個是 SRP 警訊；不要用 Property Injection 掩蓋，要用 Facade Service、Composite、domain event 拆開。
2. 抽象不該有 `Dispose()`，也不該有無參數 `Create()`；生命週期交給 Composition Root（或 Proxy）。
3. 服務抽象不要回傳或接收別的服務抽象；「factory 回傳演算法」改成「一個 Adapter 直接幫你算」。
4. 看抽象好不好：換一個完全不同的實作，這個介面還說得通嗎？
5. 循環依賴先問「哪裡設計錯了」，通常是某個介面太寬；Property Injection／Virtual Proxy 只是最後手段。

---

## Part 3 導讀：Pure DI（p.209–210）

Part 3 把第 1 章提到的 DI 三個維度各寫一章，而且**全部不用容器**（Pure DI）：

| 章 | 主題 | 作者的說法 |
|---|---|---|
| 7 | Object Composition | 各種框架的 Seam 型錄 |
| 8 | Object Lifetime | 讀完會開始覺得容器有用：生命週期管理要寫很多不產生業務價值的程式；也附 lifestyle 型錄 |
| 9 | Interception | 用 Decorator 實作 Cross-Cutting Concern |
| 10 | AOP by design | 用 SOLID 設計做 AOP，不用特殊工具。**作者說這章是全書高潮** |
| 11 | Tool-based AOP | compile-time weaving、dynamic Interception |

> **原文引用（p.209）**
> "Because a DI Container is an optional tool, we feel it's more important to explain the underlying principles and techniques that containers typically use to implement these features."

---

## 第 7 章：Application composition（組裝應用程式）

> 對應原書 p.211–235

### 這章在幹嘛

前面講過 Composition Root「是什麼」，這章講「在各種框架裡放在哪」。難的不是組物件本身，而是**每個框架的 IoC 程度不同**，得找到框架給你的 Seam。三個例子由簡到難：

1. console 程式（`Program.Main`）
2. UWP 桌面程式（`App.OnLaunched`，MVVM）
3. ASP.NET Core MVC（`IControllerActivator`＋middleware）

作者說舊框架（PowerShell、舊版 Web Forms）根本是「敵視 DI」的環境；這版只講 .NET Core。ASP.NET MVC、Web Forms、WCF、WPF、PowerShell 的做法在第一版第 7 章（買第二版附送第一版電子書）。

---

### 重點 1：mise en place——事前備好，最後才組裝

開場比喻：法式廚房的 *mise en place*（一切就位）：能先做的都先做好（切菜、熬高湯、冰淇淋前一天做），客人要吃時只花 5～10 分鐘組合。

軟體也一樣：元件先寫好，組裝最後才做。差別在於廚房是用**時間**分開準備和組裝，軟體是用**模組和層**分開。

> **原文引用（p.212）**
> "Object Composition is the act of building up hierarchies of related components. This composition takes place inside the Composition Root."

> **原文引用（p.213）**
> "Understanding each framework's Seams is key to applying DI for that particular framework."

---

### 重點 2：console 程式——最簡單，幾乎沒有 IoC

> **原文引用（p.213）**
> "When execution hits the application's entry point (usually the Main method in the Program class), you're on your own. There are no special events to subscribe to, no interfaces to implement, and precious few services you can use."

例子：**UpdateCurrency** 工具，讓管理員更新匯率（主貨幣 USD）：

```text
d:\> dotnet commerce\UpdateCurrency.dll EUR "0.88"
Updated: 0.88 EUR = 1 USD.
```

（.NET Core 的 console 程式是 `.dll`，用 `dotnet` 指令執行。）

```csharp
class Program
{
    static void Main(string[] args)
    {
        string connectionString = LoadConnectionString();               // 1. 讀設定
        CurrencyParser parser = CreateCurrencyParser(connectionString); // 2. 組 object graph
        ICommand command = parser.Parse(args);                          // 3. 執行
        command.Execute();
    }

    static CurrencyParser CreateCurrencyParser(string connectionString)
    {
        IExchangeRateProvider provider =
            new SqlExchangeRateProvider(new CommerceContext(connectionString));
        return new CurrencyParser(provider);
    }
}
```

---

### 重點 3：Composition Root 只做四件事

> **原文引用（p.215）**
> "Any Composition Root should only do four things: load configuration values, build the object graph, invoke the desired functionality, and, as we'll discuss in the next chapter, release the object graph. As soon as it has done that, it should get out of the way and leave the rest to the invoked instance."

1. 讀設定值
2. 建 object graph
3. 呼叫要的功能
4. 釋放 object graph（第 8 章）

再次強調：**讀設定**和**組物件**分成不同方法，沒有設定檔也能測組裝。

UpdateCurrency 的分層（Figure 7.3）：`Program`（可執行檔）→ `CurrencyParser`、`UpdateCurrencyCommand`、`HelpCommand`（應用服務層）→ `IExchangeRateProvider`、`Currency`（領域層）→ `SqlExchangeRateProvider`（資料層）。

這裡 `IExchangeRateProvider` 被當 Abstract Factory 用來建 `Currency`。第 6 章說要懷疑 Abstract Factory，但作者說**只有 Composition Root 用它，所以沒問題**。

---

### 重點 4：UWP 與 MVVM

**UWP（Universal Windows Platform）**：Windows 10 的應用平台，一套 API 跑在 PC、平板、手機、Xbox、HoloLens 等。入口在 `App.xaml.cs` 的 `OnLaunched`，地位跟 `Main` 一樣 → `App` class 就是 Composition Root。

**MVVM（Model-View-ViewModel）**：

| 角色 | 職責 |
|---|---|
| Model | 底層模型，常是領域模型（POCO），跟 UI 無關 |
| View | 畫面，用 XAML（XML 格式的 UI 描述語言）宣告，最好沒有 code-behind |
| ViewModel | View 和 Model 的橋，用 UWP 的方式露出資料（`ObservableCollection`、`ICommand`） |

> **原文引用（p.221）**
> "The role of the ViewModel in MVVM is different from the View Model in an MVC application. With MVC, the View Model is a behaviorless data object and is newed up in your application code. MVVM ViewModels, on the other hand, are components with Dependencies. In your UWP application, ViewModels will be composed using DI."

**要記住的觀點**：MVC 的 view model 是資料物件（直接 `new`）；MVVM 的 ViewModel 是有依賴的元件（要注入）。名字很像，DI 上完全不同。

例子：商品管理桌面程式，四個 assembly：UWP client（Composition Root）→ Presentation logic（ViewModel）→ Domain → Data access。

---

### 重點 5：把 `DataContext` 的 Property Injection 改成 Constructor Injection

UWP 正常做法是設定 page 的 `DataContext` 屬性——這是一種 Property Injection。作者改成建構子：

```csharp
public sealed partial class MainPage : Page
{
    public MainPage(MainViewModel vm)
    {
        this.InitializeComponent();
        this.DataContext = vm;
    }
}
```

`MainViewModel` 需要兩個依賴：

```csharp
public interface INavigationService
{
    void NavigateTo<TViewModel>(Action whenDone = null, object model = null)
        where TViewModel : IViewModel;     // generic 型別限制，編譯器檢查
}

public interface IViewModel
{
    void Initialize(Action whenDone, object model);
}
```

（`= null` 是 C# 的選用參數；`Action` 是無參數無回傳值的 delegate。）

`MainViewModel(INavigationService navigator, IProductRepository repository)`：
- `Initialize()` 時用 repository 載入所有商品，觸發 `PropertyChanged` 讓畫面更新
- 「新增」按鈕 → `navigator.NavigateTo<NewProductViewModel>(whenDone: this.GoBack)`
- 點一列 → `NavigateTo<EditProductViewModel>(whenDone: this.GoBack, model: product)`

XAML 用 `{Binding Model}`、`{Binding EditProductCommand}` 綁定。因為 ViewModel 放在無型別的 `DataContext`，只能用舊的 `Binding`，不能用有編譯期檢查的 `x:Bind`。

（書中註明 `IViewModel.Initialize` 帶的是這次導覽的執行期資料，不是依賴。）

---

### 重點 6：UWP 的 Composition Root——`App` 同時是 navigation service 和 Composer

```csharp
public sealed partial class App : Application, INavigationService
{
    protected override void OnLaunched(LaunchActivatedEventArgs e)
    {
        if (Window.Current.Content == null)
        {
            Window.Current.Content = new Frame();
            Window.Current.Activate();
            this.NavigateTo<MainViewModel>(null, null);
        }
    }

    public void NavigateTo<TViewModel>(Action whenDone, object model) where TViewModel : IViewModel
    {
        var page = this.CreatePage(typeof(TViewModel));
        var viewModel = (IViewModel)page.DataContext;
        viewModel.Initialize(whenDone, model);
        var frame = (Frame)Window.Current.Content;
        frame.Content = page;               // 不用 Frame.Navigate(Type)，直接塞自己建的 page
    }

    private Page CreatePage(Type vmType)   // Composer
    {
        var repository = new WcfProductRepository();
        if (vmType == typeof(MainViewModel))
            return new MainPage(new MainViewModel(this, repository));   // this = INavigationService
        else if (vmType == typeof(EditProductViewModel))
            return new EditProductPage(new EditProductViewModel(repository));
        ...
        else throw new Exception("Unknown view model.");
    }
}
```

**Composer**：第一次出現的術語，指「任何組裝依賴的物件或方法」，第 8 章細講。

預設範本的 `Frame.Navigate(Type)` 會自己建 page，沒辦法塞你建好的 → 改成直接設 `frame.Content`。

---

### 重點 7：ASP.NET Core MVC——找到 `IControllerActivator`

> **原文引用（p.228）**
> "ASP.NET Core MVC was built and designed to support DI. It comes with its own internal composition engine that you can use to build up its own components; although, as you'll see, it doesn't enforce the use of a DI Container for your application components."

**作者的強烈意見**（腳註）：ASP.NET Core 設計了一個「DI 容器的抽象」，讓第三方容器能完全取代內建的。作者認為這是**嚴重的錯誤**，讓容器維護者痛苦。建議：**保留內建容器給框架用**，就算你用第三方容器建自己的元件。

ASP.NET Core 把 MVC 和 Web API 合成一個框架，DI 的做法一樣。

請求流程（Figure 7.9）：請求進來 → `IControllerActivator.Create()` → 呼叫 action method → `Release(context, controller)` → 回應。

```csharp
public interface IControllerActivator
{
    object Create(ControllerContext context);
    void Release(ControllerContext context, object controller);
}
```

> **原文引用（p.230）**
> "A more practical way to ensure that Dependencies are disposed of is to add them to the list of disposable request objects using the HttpContext.Response.RegisterForDispose method."

---

### 重點 8：完整的 `CommerceControllerActivator`

在 `Startup.ConfigureServices` 註冊：

```csharp
services.AddMvc();
var controllerActivator = new CommerceControllerActivator(
    Configuration.GetConnectionString("CommerceConnectionString"));
services.AddSingleton<IControllerActivator>(controllerActivator);
```

兩個 controller 的直接依賴合起來只有兩個：`HomeController` → `IProductService`、`RouteController` → `IRouteCalculator`。

```csharp
public object Create(ControllerContext context)
{
    Type type = context.ActionDescriptor.ControllerTypeInfo.AsType();
    if (type == typeof(HomeController))       return this.CreateHomeController();
    else if (type == typeof(RouteController)) return this.CreateRouteController();
    else throw new Exception("Unknown controller " + type.Name);
}

private HomeController CreateHomeController() =>
    new HomeController(
        new ProductService(
            new SqlProductRepository(new CommerceContext(this.connectionString)),
            new AspNetUserContextAdapter()));

private RouteController CreateRouteController() =>
    new RouteController(new RouteCalculator(routeAlgorithms));

public void Release(ControllerContext context, object controller) { }   // 第 8.2 節再補
```

ASP.NET Core 有內建容器（第 15 章），第 12 章討論 Pure DI vs. 容器怎麼選；Part 3 都用 Pure DI。

---

### 重點 9：middleware 也要注入

**middleware**：插在 ASP.NET Core 請求管線中，處理請求前後的行為。

```csharp
app.Use(async (context, next) =>
{
    logger.LogInformation("Request started");
    await next();                         // 跑管線剩下的部分
    logger.LogInformation("Request ended");
});
```

（`async`／`await` 是 C# 的非同步語法。）

> **原文引用（p.233）**
> "Fortunately, when it comes to applying DI, asynchronous programming isn't an issue, because construction of object graphs should always be fast, should never depend on any I/O, and, therefore, should always be synchronous."

把邏輯抽成 class，好測試、`Startup` 不雜亂：

```csharp
public class LoggingMiddleware
{
    private readonly ILogger logger;
    public LoggingMiddleware(ILogger logger) { this.logger = logger; }
    public async Task Invoke(HttpContext context, Func<Task> next) { ... }
}

// Startup：一小段 Composition Root
app.Use(async (context, next) =>
{
    var middleware = new LoggingMiddleware(logger);
    await middleware.Invoke(context, next);
});
```

middleware 的 object graph 變複雜時，可以搬到 `CommerceControllerActivator` 那邊一起組。

---

### 重點 10：本章收尾——下一個問題是生命週期

> **原文引用（p.234）**
> "You may find it self evident that the external caller (often a DI Container) creates new instances of Dependencies — but when are injected instances deallocated? And what if the external caller doesn't create new instances each time, but instead hands you an existing instance?"

| 框架 | Composition Root 放哪 | 難度 |
|---|---|---|
| Console | `Program.Main` | 最簡單，完全掌控 |
| UWP | `App.OnLaunched`（＋自己的 `CreatePage`） | 簡單，要繞過 `Frame.Navigate(Type)` |
| ASP.NET Core MVC | 自訂 `IControllerActivator`＋middleware 的 `app.Use` | 為 DI 設計，一個擴充點就夠 |

---

### 本章 Summary（作者原話）

> **原文引用（p.235）**
> - "Object Composition is the act of building up hierarchies of related components, which takes place inside the Composition Root."
> - "A Composition Root should only do four things: load configuration values, build object graphs, invoke the desired functionality, and release the object graphs."
> - "Only the Composition Root should rely on configuration files because it's more flexible for libraries to be imperatively configurable by their callers."
> - "Separate the loading of configuration values from the methods that do Object Composition. This makes it possible to test Object Composition without the existence of a configuration file."
> - "Model View ViewModel (MVVM) is a design in which the ViewModel is the bridge between the view and the model. Each ViewModel is a class that translates and exposes the model in a technology-specific way. In MVVM, ViewModels are the application components that will be composed using DI."
> - "In a console application, the Program class is a suitable Composition Root."
> - "In a UWP application, the App class is a suitable Composition Root, and its OnLaunched method is the main entry point."
> - "In an ASP.NET Core MVC application, the IControllerActivator is the correct extensibility point to plug in Object Composition."
> - "A practical way to ensure that Dependencies are disposed of in ASP.NET Core is to use the HttpContext.Response.RegisterForDispose method to add them to the list of disposable request objects."
> - "Middleware can be added to ASP.NET Core by registering a function to the pipeline that implements a small part of the Composition Root. This composes the middleware component and invokes it."

### 我的整理：本章帶走的四句話

1. Composition Root 只做四件事：讀設定、組物件、呼叫、釋放。
2. 每個框架都要先找到它「建立你的物件」的那個 Seam（`Main`、`OnLaunched`、`IControllerActivator`）。
3. MVVM 的 ViewModel 是元件（要注入）；MVC 的 view model 是資料（直接 `new`）。
4. 組 object graph 要快、不碰 I/O、同步完成。

---

## 第 8 章：Object lifetime（物件生命週期）

> 對應原書 p.236–280

### 這章在幹嘛

DI 的第二個維度。交出「建立依賴」的控制權，也就交出了「依賴活多久」的控制權。這章講誰來管、怎麼管，尤其是需要手動釋放（`IDisposable`）的依賴。

骨架：

1. 生命週期管理的基本觀念：Composer 比任何消費者更能決定壽命
2. disposable 依賴：誰該 dispose、什麼時候、什麼順序
3. Lifestyle 型錄：Singleton、Transient、Scoped
4. 三個壞選擇：Captive Dependency、把 Lifestyle 洩漏給消費者、per-thread Lifestyle

開場比喻：葡萄酒什麼時候最好喝，交給侍酒師決定——就像依賴的壽命交給 Composer。

---

### 重點 1：Composer 與 Lifestyle 兩個新術語

> **原文引用（p.237）**
> "Composer is a unifying term to refer to any object or method that composes Dependencies. It's an important part of the Composition Root. The Composer is often a DI Container, but it can also be any method that constructs object graphs manually (using Pure DI)."

> **原文引用（p.238）**
> "A Lifestyle is a formalized way of describing the intended lifetime of a Dependency."

> **原文引用（p.237）**
> "When you allow a Composer to supply a Dependency, you must also accept that you can't control its lifetime."

---

### 重點 2：消費者不控制建立，也不控制銷毀

- **建立**：Composition Root 裡 `new SqlProductRepository(...)` 和 `new ProductService(...)` 之間就算插一個 `Thread.Sleep`，`ProductService` 也管不著
- **銷毀**：.NET 由 GC（garbage collector，垃圾回收器）回收，沒人引用就可回收；消費者無法主動銷毀，只能**用欄位抓著讓它活著**

**共用 vs. 不共用**（Listing 8.1 vs. 8.2）：

```csharp
// 各自一份
var repository1 = new SqlProductRepository(connString);
var repository2 = new SqlProductRepository(connString);
var productService = new ProductService(repository1);
var calculator = new DiscountCalculator(repository2);

// 共用一份
var repository = new SqlProductRepository(connString);
var productService = new ProductService(repository);
var calculator = new DiscountCalculator(repository);
```

兩種寫法沒有絕對好壞。消費者完全不知道有沒有共用——這是 **LSP** 的結果：

> **原文引用（p.241）**
> "Methods that consume Abstractions must be able to use any class derived from that Abstraction without noticing the difference."

共用時，所有消費者都離開 scope 後依賴才能被回收；消費者夠多，就幾乎永遠有人抓著——但這通常不是問題，反而省記憶體 → 正式化成 **Singleton Lifestyle**。

> **原文引用（p.242）**
> "The key point to appreciate is that the Composer has a greater degree of influence over the lifetime of Dependencies than any single consumer."

---

### 重點 3：Releasing（釋放）的定義

> **原文引用（p.242）**
> "Releasing is the process of determining which Dependencies can be dereferenced and possibly disposed of. The Composition Root requests an object graph from the Composer. After the Composition Root has finished working with that resolved graph, it informs the Composer that it has finished with the graph. The Composer can then decide which of the Dependencies of that particular graph can be released."

注意：**release ≠ dispose**。釋放是「判斷哪些可以丟、哪些要留著重用」，由 Composer 決定。

---

### 重點 4：用 Pure DI 管理 Singleton 和 Transient

第 7 章的 `CommerceControllerActivator` 每次都建整串新的（全 Transient）。分析一下（Figure 8.4）：

| 物件 | 狀態 | 適合 |
|---|---|---|
| `AspNetUserContextAdapter` | 無狀態 | Singleton |
| `RouteCalculator` | 無狀態 | Singleton |
| 連線字串 | 不會變 | 重用 |
| `CommerceContext`（EF `DbContext`） | **不是 thread-safe，不能跨請求共用** | 每個請求新建 |
| `ProductService`、`SqlProductRepository` | 依賴 `DbContext` | Transient |

```csharp
public class CommerceControllerActivator : IControllerActivator
{
    private readonly string connectionString;
    private readonly IUserContext userContext;      // Singleton
    private readonly RouteCalculator calculator;    // Singleton

    public CommerceControllerActivator(string connectionString)
    {
        this.connectionString = connectionString;
        this.userContext = new AspNetUserContextAdapter();
        this.calculator = new RouteCalculator(this.CreateRouteAlgorithms());
    }

    private HomeController CreateHomeController() =>
        new HomeController(
            new ProductService(                                    // Transient
                new SqlProductRepository(new CommerceContext(this.connectionString)),
                this.userContext));                                // 注入 Singleton

    private RouteController CreateRouteController() => new RouteController(this.calculator);
}
```

功能跟第 7 章一樣，只是更有效率。也可以改成 lazy 初始化。

> **原文引用（p.245）**
> "The ability to fine-tune each Dependency's Lifestyle can be important for performance reasons, but can also be important for correct behavior. For instance, the Mediator design pattern relies on a shared director through which several components communicate. This only works when the Mediator is shared among the involved collaborators."

---

### 重點 5：disposable 依賴——抽象不該是 disposable

.NET 有 GC，但碰到 unmanaged 資源（例如 ADO.NET 資料庫連線）就需要確定性釋放，這就是 `IDisposable` 的用途。

```csharp
public interface IMyDependency : IDisposable     // 技術上可以，但是 Leaky Abstraction
```

如果消費者照 Visual Studio Code Analysis 的建議「持有 disposable 成員就自己也實作 `IDisposable` 並 dispose 它」：

```csharp
var repository = new SqlOrderRepository(connectionString);
var validator = new OrderValidator(repository);
var orderService = new OrderService(repository);

orderService.AcceptOrder(order);
orderService.Dispose();          // 把共用的 repository 也 dispose 了
validator.Validate(order);       // 爆
```

不 dispose 又等於忽略「抽象是 disposable」這件事；而且 `Dispose()` 是消費者用不到的成員 → 違反 ISP。

> **原文引用（p.247）**
> "Declaring an Abstraction as deriving from IDisposable provides no benefit."

**對照 C**：被注入的指標，消費者不該 `free()`——因為它不知道還有沒有別人在用。只有配置它的人知道。（筆記自己的對照，書中未提。）

---

### 重點 6：ephemeral disposable（短命的 disposable）——用完就丟，藏在實作裡

> **原文引用（p.247）**
> "An ephemeral disposable is an object with a clear and short lifetime that typically doesn't exceed a single method call."

例子：WCF（Windows Communication Foundation）proxy。每個 proxy 都會在服務端開 session，不盡快 dispose 就會撞到同一來源的連線上限。

```csharp
public class WcfProductRepository : IProductRepository      // 介面不是 disposable
{
    private readonly ChannelFactory<IProductManagementService> factory;

    public void Delete(Guid productId)
    {
        using (var channel = this.factory.CreateChannel())  // 方法內建立、方法內釋放
        {
            channel.DeleteProduct(productId);
        }
    }
}
```

- 消費者 `EditProductViewModel` 只看到 `IProductRepository.Delete(id)`，可以安心持有
- `channel` 是 disposable 抽象（洩漏），但洩漏被**限制在同一個 WCF 專用 library 裡**——那裡的程式本來就該懂這些
- ephemeral disposable 從不注入給消費者，而是注入 factory（`ChannelFactory` 是 thread-safe 的，可當 Singleton）
- 為什麼不在內部用 static 欄位建 factory？那會讓 repository 隱性依賴設定檔

**WCF 例外警告**：`using` 一般是最佳做法，但 WCF proxy 的 `Dispose` 會丟例外、蓋掉原本的例外。實務上要寫 `finally` 並忽略 `Dispose` 的例外。

> **原文引用（p.249）**
> "In summary, disposable Abstractions are Leaky Abstractions. Sometimes we must accept such a leak to avoid bugs (such as refused WCF connections); but when we do that, we should do our best to contain that leak so it doesn't propagate throughout an entire application."

> **原文引用（p.250）**
> "Strive to implement services so they don't hold references to disposables, but rather create and dispose of them on demand as illustrated in listing 8.6. This makes memory management simpler because the service can be garbage collected like other objects."

---

### 重點 7：Composer 負責 dispose，而且順序要反過來

Composer 最清楚自己什麼時候建了 disposable，所以由它負責。難的是「什麼時候所有消費者都不用了」——通常靠框架給的 scope 事件，例如 ASP.NET Core 請求結束時呼叫 `IControllerActivator.Release`。

Figure 8.5 的流程：resolve 時，共用的 disposable 重用、私有的 disposable 和非 disposable 當場建；release 時只 dispose **私有的** disposable，共用的不動。

> **原文引用（p.250）**
> "An object might require its Dependencies to be called during disposal, which causes problems when these Dependencies are already disposed of. Disposal should therefore happen in the opposite order of creation — this means from the outside in."

**要不要 dispose `DbContext`？** 以前網路上常爭論（它的連線本身是 ephemeral disposable）。EF Core 2.0 加了 **DbContext pooling**，`Dispose` 時把實例還回池子——不 dispose 會把池子餓死。

> **原文引用（p.251）**
> "The moral of this story is that you should always make sure disposable objects are correctly disposed of. Even if you determined that you could omit a call to Dispose in your specific case, an external component, such as Entity Framework Core, is free to change that behavior any time in the future."

---

### 重點 8：實作——把 disposable 掛在請求上，`Release` 時反向 dispose

作者坦承 Listing 8.3 有 bug：`CommerceContext` 是 disposable，卻從沒被 dispose。

```csharp
private HomeController CreateHomeController(ControllerContext context)
{
    var dbContext = new CommerceContext(this.connectionString);
    TrackDisposable(context, dbContext);                // 記在這個請求上
    return new HomeController(new ProductService(new SqlProductRepository(dbContext), this.userContext));
}

private static void TrackDisposable(ControllerContext context, IDisposable disposable)
{
    IDictionary<object, object> items = context.HttpContext.Items;
    object list;
    if (!items.TryGetValue("Disposables", out list))
    {
        list = new List<IDisposable>();
        items["Disposables"] = list;
    }
    ((List<IDisposable>)list).Add(disposable);
}

public void Release(ControllerContext context, object controller)
{
    var disposables = (List<IDisposable>)context.HttpContext.Items["Disposables"];
    if (disposables != null)
    {
        disposables.Reverse();                          // 建立的反向
        foreach (IDisposable disposable in disposables) disposable.Dispose();
    }
}
```

- 用請求（`HttpContext.Items`）追蹤，而不是 static 字典——static 可變狀態要處理 thread-safety，更難寫對
- 這個 `Release` 有捷徑：某個 `Dispose` 丟例外，後面的就不會被 dispose；嚴謹的話要用 `try/finally`
- 在 ASP.NET Core 裡其實一行 `HttpContext.Response.RegisterForDispose` 就做到同樣的事（含反向、含失敗後繼續）

> **原文引用（p.254）**
> "DI Containers are particularly good at Lifetime Management. ... When you find yourself in the situation where maintaining your Composition Root using Pure DI becomes difficult, consider switching to a DI Container instead."

**誰負責發起 release？** 要求 object graph 的那段程式（通常是 Composition Root）也負責要求釋放。console 程式自己呼叫 `Release(parser)`；ASP.NET Core 則是框架呼叫 `Create` 和 `Release`。

---

### 重點 9：Lifestyle 型錄（Table 8.1）

| Name | Description |
|---|---|
| **Singleton** | 一個實例永遠重用 |
| **Transient** | 每次都給新的 |
| **Scoped** | 在一個（隱式或明確定義的）scope 內，每種型別最多一個實例 |

> **原文引用（p.255）**
> "The use of a Scoped Lifestyle is widespread; most exotic Lifestyles are variations of it."

---

### 重點 10：Singleton Lifestyle——能用就用

> **原文引用（p.256）**
> "Within the scope of a single Composer, there'll only be one instance of a component with the Singleton Lifestyle. Each and every time a consumer requests the component, the same instance is served."

**跟 Singleton design pattern 不一樣**：design pattern 提供全域存取點（跟 Ambient Context 很像）；Singleton Lifestyle 不能透過 static 成員取得，兩個 Composer 會給你兩個不同實例。

> **原文引用（p.256）**
> "Use the Singleton Lifestyle whenever possible."

不能用的兩種情況：
1. 元件不是 thread-safe
2. 元件的某個依賴壽命應該比較短 → 會變成 Captive Dependency

無狀態服務、不可變型別、專門設計成 thread-safe 的 class，都可以放心當 Singleton。很少用卻很吃記憶體的，可以包一個 Virtual Proxy。

**某些東西必須共用才正確**：Circuit Breaker（第 9 章）、in-memory cache、in-memory repository——而且一定要 thread-safe（Figure 8.7）。

```csharp
public CommerceControllerActivator()
{
    this.userContext = new FakeUserContext();                 // Singleton
    this.repository = new InMemoryProductRepository();        // Singleton，必須 thread-safe
}
private HomeController CreateHomeController() =>
    new HomeController(new ProductService(this.repository, this.userContext));
```

Singleton 在 Composer 離開 scope 時才結束；那時若是 disposable，Composer 要 dispose 它。

---

### 重點 11：Transient Lifestyle——最安全，最沒效率

- 每次給新的；不是 disposable 就什麼都不用追蹤，是的話 Composer 要記住並在 release 時 dispose
- **警告**：不同 DI Container 對 Transient 的處理不同，有的會追蹤並 dispose，有的不會
- 桌面程式通常只在啟動時 resolve 一次整棵樹，所以 Transient 也只建幾個、活很久；如果每個依賴只有一個消費者，全 Transient 跟全 Singleton 結果一樣

> **原文引用（p.259）**
> "The Transient Lifestyle is the safest choice of Lifestyles, but also one of the least efficient."

不確定 thread-safety 時用 Transient 最安全；很多情況可以換成 Scoped（存取也保證是循序的）。

> **原文引用（p.259）**
> "Although you can mix Dependencies with different Lifestyles, you should make sure that a consumer only has Dependencies with a lifetime that's equal to or exceeds its own, because a consumer will keep its Dependencies alive by storing them in its private fields."

---

### 重點 12：Scoped Lifestyle——一個請求內是 Singleton

web 程式要同時處理很多請求。不 thread-safe 的依賴不能當 Singleton；當 Transient 又沒效率，或同一請求內需要共用時有問題。

ASP.NET Core 一個請求可能跨好幾個 thread（非同步），但只要正確 `await`，**執行是循序的**（thread 一個接一個用，不是平行）→ 請求內共用不用擔心 thread-safety。

> **原文引用（p.261）**
> "Scoped Dependencies behave like Singleton Dependencies within a single, well-defined scope or request but aren't shared across scopes. Each scope has its own cache of associated Dependencies."

「請求」不限於 web：從佇列一筆筆處理的服務，每筆就是一個請求；桌面或手機程式的一次按鈕點擊也可以當一個 scope。

> **原文引用（p.262）**
> "If you ever need to compose an Entity Framework Core DbContext in a web request, a Scoped Lifestyle is an excellent choice. DbContext instances aren't thread-safe, but you typically only want to have one DbContext instance per web request."

scope 結束時，裡面的 disposable 都要 dispose。

---

### 重點 13：Scoped 範例——CurrencyMonitoring，每分鐘一個 scope

console 程式，每分鐘印一次某金額換算成各幣別的結果，直到使用者停止：

```text
Exchange rates for EUR 1.00000 at 12/10/2018 22:55:00.
CAD 1.48864
USD 1.13636
...
```

Lifestyle 配置（Figure 8.10）：`CurrencyRateDisplayer`、`SqlCurrencyRepository`、`CurrencyConverter`、`SqlExchangeRateProvider` 都是 Transient；**`CommerceContext` 是 Scoped**；連線字串是 Singleton。

```csharp
public class Composer
{
    private readonly string connectionString;     // Singleton

    public CurrencyRateDisplayer CreateRateDisplayer()
    {
        var context = new CommerceContext(this.connectionString);     // 這次 scope 的唯一一份
        return new CurrencyRateDisplayer(
            new SqlCurrencyRepository(context),
            new CurrencyConverter(new SqlExchangeRateProvider(context)));
    }
}

public static void Main(string[] args)
{
    var money = new Money(currency: new Currency(code: args[0]), amount: decimal.Parse(args[1]));
    composer = new Composer(LoadConnectionString());
    var timer = new Timer(interval: 60000);
    timer.Elapsed += (s, e) => DisplayRates(money);      // 每次觸發＝一個新 scope
    timer.Start();
    Console.ReadLine();
}
```

就算平行呼叫 `DisplayRates`，也沒問題——每次都有自己的一組 scoped 實例。（書中為了簡化省略了 release。）

---

### 重點 14：三個壞的 Lifestyle 選擇（Table 8.2）

| Subject | Type | Description |
|---|---|---|
| **Captive Dependencies** | Bug | 依賴被抓得比預期活得久 |
| **Leaky Abstractions** | 設計問題 | 把 Lifestyle 選擇洩漏給消費者 |
| **Per-thread Lifestyle** | Bug | 把實例綁在 thread 壽命上，造成並行 bug |

> **原文引用（p.266）**
> "More often than not, these bugs only appear after deploying the application to production, because they are concurrency related. When we start the application, as developers, we typically run it for a short period of time, one request at a time."

---

### 重點 15：Captive Dependency（被俘虜的依賴）

> **原文引用（p.266）**
> "A Captive Dependency is a Dependency that's inadvertently kept alive for too long because its consumer was given a lifetime that exceeds the Dependency's expected lifetime."

最常見：把短命依賴注入 Singleton。

```csharp
public Composer(string connectionString)
{
    this.repository = new SqlProductRepository(            // Singleton
        new CommerceContext(connectionString));            // 跟著變成全程式只有一個 → 被俘虜
}
```

> **原文引用（p.267）**
> "A component should only reference Dependencies that have an expected lifetime that's equal to or longer than that of the component itself."

用容器時特別常見（容器動態建圖，容易看不清形狀），但 Pure DI 也會發生。**Pure DI 的防範：把建立分成三個階段**：

```csharp
public CommerceControllerActivator(string connectionString)
{
    // 階段 1：啟動時建 Singleton
    this.connStr = connectionString;
    this.userContext = new AspNetUserContextAdapter();
}

public object Create(ControllerContext ctx)
{
    // 階段 2：請求開始時建 Scoped
    var context = new CommerceContext(this.connStr);
    var provider = new SqlExchangeRateProvider(context);

    // 階段 3：依請求組出 Transient＋Scoped＋Singleton 的 graph
    if (type == typeof(HomeController)) return this.CreateHomeController(context);
    ...
}
```

每個請求都建全部 Scoped 依賴（就算沒用到）看似浪費，但建構子只有 Guard Clause，很快，沒關係。

> **原文引用（p.269）**
> "More often than we'd like to admit, we've wasted many hours trying to find bugs caused by Captive Dependencies. That's why we consider tool support for spotting Captive Dependencies invaluable when you're using a DI Container."

（書中介紹的所有容器都有偵測 Captive Dependency 的功能。）

---

### 重點 16：`Lazy<T>` 當依賴是 Leaky Abstraction

想延後建立昂貴依賴，用 .NET 的 `Lazy<T>`（第一次讀 `.Value` 才建立，之後快取）：

```csharp
public ProductService(IProductRepository repository, Lazy<IUserContext> userContext) { ... }
// 用時：this.userContext.Value
```

延後建立本身合理，**錯在把這個考量推給消費者**——洩漏了 Composition Root 的實作選擇，違反 DIP。問題：

1. 消費者和測試變複雜：還要多寫測試驗證 `.Value` 沒在錯的時間被呼叫，每個消費者都要
2. 後來才把依賴改成 lazy → 所有消費者和測試都要改
3. 為了避免 2 就全部預設 lazy？瘋了——每個依賴都可能變成多個實作，那就要 `IEnumerable<Lazy<T>>`
4. 新元件不小心依賴 `IUserContext` 而非 `Lazy<IUserContext>`，lazy 就整個失效

> **原文引用（p.271）**
> "you should keep the constructors of your components free of any logic other than Guard Clauses and the storing of incoming Dependencies. This makes the construction of your classes fast and reliable, and will prevent such components from ever becoming expensive to instantiate."

真的沒辦法時（例如第三方元件），把 `Lazy<T>` 藏在 **Virtual Proxy** 裡、放在 Composition Root：

```csharp
public class LazyUserContextProxy : IUserContext
{
    private readonly Lazy<IUserContext> userContext;
    public LazyUserContextProxy(Lazy<IUserContext> userContext) { ... }
    public bool IsInRole(Role role) => this.userContext.Value.IsInRole(role);
}

IUserContext lazyProxy = new LazyUserContextProxy(
    new Lazy<IUserContext>(() => new AspNetUserContextAdapter()));
new HomeController(new ProductService(new SqlProductRepository(...), lazyProxy));
```

`ProductService` 照樣依賴 `IUserContext`。依賴 `Func<T>` 效果幾乎一樣，解法也一樣。

---

### 重點 17：`IEnumerable<T>` 當依賴也是 Leaky Abstraction

```csharp
public Component(IEnumerable<ILogger> loggers) { ... }
public void DoSomething()
{
    foreach (var logger in this.loggers) logger.Log("DoSomething called");
}
```

「可能有多個實作」是實作細節。每個消費者都要寫迴圈；需求一變（某個 logger 失敗要繼續、失敗要記到下一個 logger 當備援），每個地方都要改。

解法：**Composite**，把策略集中在一處：

```csharp
public class CompositeLogger : ILogger
{
    private readonly IList<ILogger> loggers;
    public void Log(LogEntry entry)
    {
        for (int index = 0; index < this.loggers.Count; index++)
        {
            try { this.loggers[index].Log(entry); }
            catch (Exception ex)
            {
                if (loggers.Count > index + 1)
                    loggers[index + 1].Log(new LogEntry(ex));   // 下一個當備援
            }
        }
    }
}

ILogger composite = new CompositeLogger(new ILogger[]
{
    new SqlLogger(connectionString),
    new WindowsEventLogLogger(source: "MyApp"),
    new FileLogger(directory: "c:\\logs")
});
new Component(composite);
```

（作者說這裡暫時忽略第 5 章「不要讓 logging 污染程式」的建議，第 10 章再處理。）

---

### 重點 18：per-thread Lifestyle——不要把依賴綁在 thread 上

```csharp
[ThreadStatic]                                   // CLR 讓每個 thread 各有一份這個 static 欄位
private static CommerceContext context;

static CurrencyParser CreateCurrencyParser(string connectionString)
{
    if (context == null) context = new CommerceContext(connectionString);
    return new CurrencyParser(new SqlExchangeRateProvider(context), context);
}
```

（`[ThreadStatic]` 類似 C 的 `__thread`／`_Thread_local`。）

> **原文引用（p.276）**
> "Some DI Containers refer to this method as the per-thread Lifestyle and have built-in support for it — avoid this!"

兩個問題：

1. **thread 壽命不明確**：`new Thread().Start()` 拿到新的；thread pool 可能給舊的也可能給新的；框架（如 ASP.NET Core）自己也會建 thread。有的很短，有的活整個程式
2. **非同步框架會換 thread**（Figure 8.12）：請求 1 在 thread A 開始，拿到 A 專屬的依賴；做 I/O 時暫停，thread A 去處理請求 2（也拿到同一個 A 專屬依賴）；I/O 完成後請求 1 在 thread B 繼續，**還用著 A 的依賴** → 兩個請求平行使用同一個不 thread-safe 的物件

> **原文引用（p.277）**
> "The object graph of Request 1 in figure 8.12 moves from one thread to another, although the Dependency is thread specific. The Dependency effectively becomes a Captive Dependency once the object graph is moved to another thread."

解法：改成 Scoped（用區域變數）：

```csharp
static CurrencyParser CreateCurrencyParser(string connectionString)
{
    var context = new CommerceContext(connectionString);
    return new CurrencyParser(new SqlExchangeRateProvider(context), context);
}
```

壽命變短若真的有問題，可以用 pooling，或把 thread-static 存取藏在 proxy 的方法內（不讓依賴跨 thread 移動）。

---

### 重點 19：需要自訂 Lifestyle 時，先懷疑自己的設計

> **原文引用（p.278）**
> "The point is that if you feel the need to implement a custom Lifestyle or create a Leaky Abstraction, you should first seriously reconsider your design. For this reason, we decided to leave specialized Lifestyles out of this book. We can often handle such situations better with a redesign or Interception, as you'll see in the next chapter."

---

### 本章 Summary（作者原話）

> **原文引用（p.279–280）**
> - "Composer is a unifying term, referring to any object or method that composes Dependencies. It's an important part of the Composition Root."
> - "The Composer can be a DI Container, but it can also be any method that constructs object graphs manually using Pure DI."
> - "The Composer has a greater degree of influence over the lifetime of Dependencies than any single consumer can have. The Composer decides when instances are created, and, by its choice of whether to share instances, it determines whether a Dependency goes out of scope with a single consumer or whether all consumers must go out of scope before the Dependency can be released."
> - "A Lifestyle is a formalized way of describing the intended lifetime of a Dependency."
> - "The ability to fine tune each Dependency's Lifestyle is important for performance reasons but can also be important for correct behavior. Some Dependencies must be shared between several consumers for the system to work correctly."
> - "The Liskov Substitution Principle states that you must be able to substitute the Abstraction for an arbitrary implementation without changing the correctness of the system."
> - "Failing to adhere to the Liskov Substitution Principle makes applications fragile, because it disallows replacing Dependencies that might cause a consumer to break."
> - "An ephemeral disposable is an object with a clear and short lifetime that typically doesn't exceed a single method call."
> - "Diligently work to implement services so they don't hold references to disposables, but rather create and dispose of them on demand. This makes memory management simpler, because the service can be garbage collected like other objects."
> - "The responsibility of disposing of Dependencies falls to the Composer. It, better than anything else, knows when it creates a disposable instance, so it also knows that the instance needs to be disposed of."
> - "Releasing is the process of determining which Dependencies can be dereferenced and (possibly) disposed of. The Composition Root signals the Composer to release a resolved Dependency."
> - "A Composer must take care of the correct order of disposal for objects. An object might require its Dependencies to be called during disposal, which causes problems if these Dependencies are already disposed of. Disposal should, therefore, happen in the opposite order of object creation."
> - "The Transient Lifestyle involves returning a new instance every time it's requested. Each consumer gets its own instance of the Dependency."
> - "Within the scope of a single Composer, there'll only be one instance of a component with the Singleton Lifestyle. Each time a consumer requests the component, the same instance is served."
> - "Scoped Dependencies behave like singletons within a single, well-defined scope or request, but aren't shared across scopes. Each scope has its own set of associated Dependencies."
> - "The Scoped Lifestyle makes sense for long-running applications that are tasked with processing operations that need to run in some degree of isolation. Isolation is required when these operations are processed in parallel, or when each operation contains its own state."
> - "If you ever need to compose an Entity Framework Core DbContext in a web request, a Scoped Lifestyle is an excellent choice. DbContext instances aren't thread-safe, but you typically only want one DbContext instance per web request."
> - "Object graphs can consist of Dependencies of different Lifestyles, but you should make sure that a consumer only has Dependencies with a lifetime that's equal to or exceeds its own, because a consumer will keep its Dependencies alive. Failing to do so leads to Captive Dependencies."
> - "A Captive Dependency is a Dependency that's inadvertently kept alive for too long, because its consumer was given a lifetime that exceeds the Dependency's expected lifetime."
> - "Captive Dependencies are a common source of bugs when working with a DI Container, although the problem can also arise when working with Pure DI."
> - "When applying Pure DI, a careful structure of the Composition Root can reduce the chance of running into problems."
> - "When working with a DI Container, Captive Dependencies are such a widespread problem that some DI Containers perform analysis on constructed object graphs to detect them."
> - "Sometimes you need to postpone the creation of a Dependency. Injecting the Dependency as a Lazy<T>, Func<T>, or IEnumerable<T>, however, is a bad idea because it causes the Dependency to become a Leaky Abstraction. Instead, you should hide this knowledge behind a Proxy or Composite."
> - "Don't bind the lifetime of a Dependency to the lifetime of a thread. The lifetime of a thread is often unclear, and using it in an asynchronous framework can cause multi-threading issues. Instead, use a proper Scoped Lifestyle or hide access to the thread-static value behind a Proxy."

### 我的整理：本章帶走的五句話

1. 誰建立誰負責：Composer 決定共不共用、何時 dispose；消費者只用不管。抽象不要繼承 `IDisposable`。
2. 三種 Lifestyle：Singleton（能用就用，要 thread-safe）、Transient（最安全最浪費）、Scoped（一個請求一份，`DbContext` 的標準選擇）。
3. 鐵則：依賴的壽命 ≥ 消費者的壽命，否則就是 Captive Dependency——最常見、最難抓的生命週期 bug。
4. Dispose 順序跟建立相反；Pure DI 時把建立分成「啟動／請求開始／組圖」三階段。
5. `Lazy<T>`、`Func<T>`、`IEnumerable<T>` 不要注入給業務 class，包成 Proxy 或 Composite 放在 Composition Root；不要用 per-thread Lifestyle。

---

## 第 9 章：Interception（攔截）

> 對應原書 p.281–300

### 這章在幹嘛

DI 的第三個維度。前面八章花力氣把程式弄成鬆耦合，這章開始**收成**：在不改消費者、也不改服務的情況下，於兩者之間插入程式碼。核心手法就是 **Decorator**。

作者說這章理論少、例子多；熟的人可以直接跳第 10 章。骨架：

1. Decorator pattern 複習（`IGreeter` 小例子）＋稽核（auditing）Decorator
2. 三個 Cross-Cutting Concern 例子：Circuit Breaker、錯誤處理、權限檢查
3. 結尾點出問題：Decorator 寫起來很重複 → 引出第 10 章 AOP

開場比喻：小牛排生吃不好吃，加奶油、鹽、胡椒、裹粉、塞火腿起司就變成 cordon bleu——主食材沒變，靠外面加東西。

---

### 重點 1：Interception 與 Cross-Cutting Concern 的定義

> **原文引用（p.283）**
> "Interception is the ability to intercept calls between two collaborating components in such a way that you can enrich or change the behavior of the Dependency without the need to change the two collaborators themselves."

> **原文引用（p.282）**
> "Cross-Cutting Concerns are aspects of a program that affect a larger part of the application. They're often non-functional requirements. They don't directly relate to any particular feature, but, rather, are applied to existing functionality."

> **原文引用（p.284）**
> "The set of software design principles and patterns around DI (such as, but not limited to, loose coupling and the Liskov Substitution Principle) are the enablers of Interception. Without these principles and patterns, it's impossible to apply Interception."

第 8 章的 `LazyUserContextProxy` 其實已經是一種攔截了。

---

### 重點 2：Decorator pattern——同一個介面包同一個介面

GoF 對 Decorator 的描述：

> **原文引用（p.284）**
> "attach additional responsibilities to an object dynamically. Decorators provide a flexible alternative to subclassing for extending functionality."

```csharp
interface IComponent { void Operation(); }

class Decorator : IComponent
{
    private readonly IComponent decoratee;           // 包住同一個介面的另一個實作
    public Decorator(IComponent decoratee) { this.decoratee = decoratee; }
    public void Operation() { /* 前 */ this.decoratee.Operation(); /* 後 */ }
}
```

- 「dynamically」：在執行期決定要不要套，而不是像繼承在編譯期寫死
- 可以一層包一層，像俄羅斯娃娃（Figure 9.3），最裡面一定是真正做事的實作

**對照 C**：就是包一層同簽名的函式指標表，每個函式先做點事再呼叫原本那張表的同名函式。（筆記自己的對照，書中未提。）

---

### 重點 3：`IGreeter` 例子——改輸入、改輸出、或乾脆不呼叫

```csharp
public interface IGreeter { string Greet(string name); }

public class FormalGreeter : IGreeter
{
    public string Greet(string name) => "Hello, " + name + ".";
}

// TitledGreeterDecorator：改輸入
public string Greet(string name) => this.decoratee.Greet("Mr. " + name);

// NiceToMeetYouGreeterDecorator：改輸出
public string Greet(string name) => this.decoratee.Greet(name) + " Nice to meet you.";

IGreeter greeter =
    new NiceToMeetYouGreeterDecorator(
        new TitledGreeterDecorator(
            new FormalGreeter()));

greeter.Greet("Samuel L. Jackson");
// Hello, Mr. Samuel L. Jackson. Nice to meet you.
```

Decorator 也可以**不呼叫**被包的物件（例如 `name == null` 就回 `"Hello world!"`）。這時它是「取代」而不是「增強」原行為——嚴格說比較像 Chain of Responsibility，但兩者很接近，書中統稱 Decorator。更常見的是丟例外中止（見重點 9）。

> **原文引用（p.287）**
> "What differentiates a Decorator from any class containing Dependencies is that the wrapped object implements the same Abstraction as the Decorator. This enables a Composer to replace the original component with a Decorator without changing the consumer."

被包的物件通常以抽象型別注入，所以 Decorator 也要遵守 LSP，一視同仁。第 1 章的 `SecureMessageWriter` 就是 Decorator。

---

### 重點 4：稽核 Decorator——修正第 6 章的 SRP 違規

第 6 章把 `IAuditTrailAppender` 注入 `SqlUserRepository`，逼 repository 自己懂稽核 → 違反 SRP。改用 Decorator：

```csharp
public class AuditingUserRepositoryDecorator : IUserRepository
{
    private readonly IAuditTrailAppender appender;
    private readonly IUserRepository decoratee;

    public AuditingUserRepositoryDecorator(IAuditTrailAppender appender, IUserRepository decoratee) { ... }

    public User GetById(Guid id) => this.decoratee.GetById(id);    // 讀取不稽核

    public void Update(User user)
    {
        this.decoratee.Update(user);
        this.appender.Append(user);                                 // 寫入成功後才稽核
    }
}
```

常見需求：稽核所有新增／修改／刪除，忽略讀取。Decorator 只負責**協調**被包的 repository 和 appender；`IAuditTrailAppender` 本身也是抽象，可以獨立換實作。

組裝：

```csharp
IUserRepository userRepository =
    new AuditingUserRepositoryDecorator(
        new SqlAuditTrailAppender(this.userContext, context),
        new SqlUserRepository(context));

IProductService productService =
    new ProductService(new SqlProductRepository(context), this.userContext, userRepository);
```

`ProductService` 只看到 `IUserRepository`，不知道有 Decorator。沒改任何現有 class 就加了稽核 → **OCP**。（書中警告這個例子忽略了 `CommerceContext` 的 dispose；正確版本要結合第 8.3.3 節的做法，會變得相當複雜。）

---

### 重點 5：常見的 Cross-Cutting Concern（Table 9.1）

| Aspect | Description（說明） |
|---|---|
| **Auditing（稽核）** | 每個改資料的操作都留下時間、使用者、改了什麼 |
| **Logging（日誌）** | 記錄反映程式狀態的事件，給維運或業務看 |
| **Performance monitoring（效能監控）** | 記錄效能而非事件；SLA 無法用標準基礎設施監控時要自己做 |
| **Validation（驗證）** | 驗證本身依情境而定，但「呼叫驗證、處理結果」常是橫切的 |
| **Security（安全）** | 某些操作只允許特定角色或群組 |
| **Caching（快取）** | 資料存取元件不該自己處理；可以依實作開關 |
| **Error handling（錯誤處理）** | 處理、記錄、轉換例外，或顯示訊息給使用者 |
| **Fault tolerance（容錯）** | 外部資源一定會偶爾掛掉；資料庫交易可能 deadlock；用 Circuit Breaker 等模式處理 |

架構圖裡，Cross-Cutting Concern 常畫成**跨越所有層的直條**（Figure 9.6）。

---

### 重點 6：Circuit Breaker（斷路器）——快速失敗

情境：網路斷了、資料庫離線、web service 被 DDoS。多數 .NET API 有預設 timeout，但收到 timeout 後，下一次呼叫要怎麼辦？再等一次 timeout 嗎？

> **原文引用（p.292）**
> "Circuit Breaker is a stability pattern that adds robustness to an application by failing fast instead of hanging and consuming resources as it hangs. This is a good example of a non-functional requirement and a true Cross-Cutting Concern, because it has little to do with the feature implemented in the out-of-process call."

名稱來自電路的斷路器。狀態機（Figure 9.7）：

```text
         Error                     Timeout
Closed ─────────► Open ─────────────────────► Half-open
  ▲                ▲                              │
  │                └──────────── Error ───────────┤
  └───────────────────── Call succeeds ───────────┘
```

- **Closed**：正常放行
- **Open**：不放任何呼叫，直接丟例外
- **Half-open**：timeout 後放一個呼叫試試；成功 → Closed，失敗 → 回 Open 重新計時

實務上可以更複雜：不要每次偶發錯誤就跳，用門檻；只對特定錯誤跳（timeout、通訊例外可以；`NullReferenceException` 多半是 bug，不該跳）。

---

### 重點 7：Circuit Breaker Decorator

```csharp
public class CircuitBreakerProductRepositoryDecorator : IProductRepository
{
    private readonly ICircuitBreaker breaker;
    private readonly IProductRepository decoratee;

    public void Insert(Product product)
    {
        this.breaker.Guard();                  // Open 狀態就直接丟例外
        try
        {
            this.decoratee.Insert(product);
            this.breaker.Succeed();
        }
        catch (Exception ex)
        {
            this.breaker.Trip(ex);             // 實務上只對特定例外型別 Trip
            throw;
        }
    }

    public IEnumerable<Product> GetAll()
    {
        this.breaker.Guard();
        try
        {
            var products = this.decoratee.GetAll();   // 有回傳值的要先存起來
            this.breaker.Succeed();
            return products;
        }
        catch (Exception ex) { this.breaker.Trip(ex); throw; }
    }
}
```

`CircuitBreaker` 本身用 **State pattern** 實作，可以完全重用（程式碼在書的附件裡）。

> **原文引用（p.295）**
> "Although we aren't going to dive deeper into the implementation of CircuitBreaker here, the important message is that you can Intercept with arbitrarily complex code."

作者也提到更精簡的介面 `T Execute<T>(Func<T> action)`（他們自己偏好這種 continuation passing），但為了聚焦在攔截，書中用三方法版本。

---

### 重點 8：Circuit Breaker 應該是 Singleton

```csharp
var channelFactory = new ChannelFactory<IProductManagementService>("*");
ICircuitBreaker breaker = new CircuitBreaker(TimeSpan.FromMinutes(1));
IProductRepository repository =
    new CircuitBreakerProductRepositoryDecorator(breaker, new WcfProductRepository(channelFactory));
```

這裡每次 resolve 都建新的 breaker（Transient）。UWP 只 resolve 一次所以沒差，但一般來說：另一端只有一個 web service，它掛了，**所有**用到它的 Decorator 都應該斷開 → breaker 應該是 Singleton。代價：breaker 有狀態，必須自己實作 thread-safety，更複雜。

> **原文引用（p.297）**
> "The Cross-Cutting Concern can easily be more complex than the original implementation."

---

### 重點 9：錯誤處理 Decorator

Circuit Breaker 只是讓失敗更快，**還是會丟例外**，沒處理就當掉。錯誤處理不該塞進依賴本身：依賴是可重用元件，不可能有一種處理策略適合所有情境，而且違反 SRP。

```csharp
public void Insert(Product product)
{
    try
    {
        this.decoratee.Insert(product);
    }
    catch (CommunicationException ex)    { this.AlertUser(ex.Message); }
    catch (InvalidOperationException ex) { this.AlertUser(ex.Message); }
}
```

- 只處理**已知的**例外型別——吞掉所有例外很危險
- `AlertUser` 用 `MessageBox.Show` 顯示（Figure 9.8 顯示的錯誤訊息其實來自 Circuit Breaker）
- 又一次：只加新型別、不改舊程式 → SRP＋OCP

---

### 重點 10：權限檢查 Decorator

（作者說安全是大題目，這裡只碰**授權**：確保只有被授權的人能做某些事。）

```csharp
public class SecureProductRepositoryDecorator : IProductRepository
{
    private readonly IUserContext userContext;
    private readonly IProductRepository decoratee;

    public void Delete(Guid id)
    {
        this.CheckAuthorization();
        this.decoratee.Delete(id);
    }

    public IEnumerable<Product> GetAll() => this.decoratee.GetAll();   // 查詢人人可做

    private void CheckAuthorization()
    {
        if (!this.userContext.IsInRole(Role.Administrator))
            throw new SecurityException("Access denied.");
    }
}
```

只有 CUD（create／update／delete）需要檢查，角色寫死成 Administrator。

---

### 重點 11：問題浮現——Decorator 很重複

> **原文引用（p.300）**
> "In our current design, for a given Cross-Cutting Concern, the implementation based on a Decorator tends to be repetitive. Implementing a Circuit Breaker involves applying the same code template to all methods of the IProductRepository interface. Had you wanted to add a Circuit Breaker to another Abstraction, you would've had to apply the same code to more methods."

- Circuit Breaker：每個方法都套同一個樣板
- 安全：有的方法要檢查、有的直接轉呼叫，更亂
- 換一個抽象要加同樣功能，又要重寫一遍 → 系統變大後維護成本很高

這就是第 10 章 **Aspect-Oriented Programming** 要解決的問題。

**要記住的觀點**：Decorator 讓你不改舊碼就加 Cross-Cutting Concern；但「每個介面 × 每個方法 × 每種關注點」都要寫一次，數量會爆炸。

---

### 本章 Summary（作者原話）

> **原文引用（p.300）**
> - "Interception is the ability to intercept calls between two collaborating components in such a way that you can enrich or change the behavior of the Dependency without the need to change the two collaborators themselves."
> - "Loose coupling is the enabler of Interception. When you program to an interface, you can transform or enhance a core implementation by wrapping it in other implementations of that interface."
> - "At its core, Interception is an application of the Decorator design pattern."
> - "The Decorator design pattern provides a flexible alternative to subclassing by attaching additional responsibilities to an object dynamically. It works by wrapping one implementation of an Abstraction in another implementation of the same Abstraction. This allows Decorators to be nested like Russian nesting dolls."
> - "Cross-Cutting Concerns are non-functional aspects of code that typically cut across a wide area of the code base. Common examples of Cross-Cutting Concerns are auditing, logging, validation, security, and caching."
> - "Circuit Breaker is a stability design pattern that adds robustness to a system by cutting connections when a fault occurs in order to prevent the fault from propagating."

### 我的整理：本章帶走的四句話

1. Interception＝在消費者和服務之間插一層，兩邊都不用改；手段是 Decorator（同一個介面包同一個介面）。
2. 稽核、日誌、快取、驗證、權限、錯誤處理、容錯，都是可以用 Decorator 疊上去的 Cross-Cutting Concern。
3. Decorator 可以改輸入、改輸出、在前後做事、或直接丟例外不往下呼叫；裡面的邏輯可以比原本的實作還複雜（Circuit Breaker）。
4. 共用狀態的 Decorator（Circuit Breaker）要當 Singleton，而且要 thread-safe。

---

## 第 10 章：Aspect-Oriented Programming by design（用設計做 AOP）

> 對應原書 p.301–340

### 這章在幹嘛

**作者自稱全書高潮。** 第 9 章結尾的問題：Decorator 每個介面、每個方法都要寫一次。很多人以為解法是 AOP 工具，作者主張：**先用 SOLID 設計解決，不用任何工具**。

骨架：

1. AOP 是什麼、三種做法比較
2. SOLID 五原則複習
3. 主菜：把 Mary 長大的 `IProductService` 一步步重構成 `ICommandService<TCommand>`
4. 在這個抽象上一次加好交易、稽核、權限三個 aspect

作者事先警告：「準備好心理不適，甚至認知失調。」

開場比喻：專業廚房講究動線和工作站配置，好的配置讓大家專注、少重複——程式庫就是我們的廚房。

---

### 重點 1：AOP 的定義——Decorator 的重複違反 DRY

AOP 在 1997 年由 Xerox PARC 發明（AspectJ，Java 的擴充）。

> **原文引用（p.302）**
> "Aspect-Oriented Programming aims to reduce boilerplate code required for implementing Cross-Cutting Concerns and other coding patterns. It does this by implementing such patterns in a single place and applying them to a code base either declaratively or based on convention, without modifying the code itself."

第 9 章的 Circuit Breaker Decorator，`Delete` 和 `Insert` 只差一行（呼叫 `decoratee.Delete` 還是 `decoratee.Insert`），其餘一模一樣。

> **原文引用（p.303）**
> "Even though we've successfully delegated the Circuit Breaker implementation to a separate class via the ICircuitBreaker interface, this plumbing code violates the DRY principle. It tends to be reasonably unchanging, but it's still a liability."

稽核也一樣：放進 `SqlProductRepository` 違反 SRP；每個 repository 寫一個稽核 Decorator 又大量重複、改一次要改一堆（違反 OCP）。理想是：**宣告式地把稽核套到所有 repository 的某些方法上，稽核只寫一次**。

---

### 重點 2：三種 AOP 做法（Table 10.1）

| Method | Description | Advantages | Disadvantages |
|---|---|---|---|
| **SOLID** | 用 Decorator 包在可重用抽象外面；依行為把 class 分組來套 aspect | 不需工具；aspect 好寫；聚焦設計；系統更好維護 | 舊系統不一定好套 |
| **Dynamic Interception** | 執行期依抽象自動產生 Decorator，注入工具專用的 aspect（Interceptor） | 舊程式只要已經對介面寫程式，改動很少；編譯後的程式不依賴攔截函式庫；有免費好工具 | aspect 跟 AOP 工具強耦合；失去編譯期檢查；慣例（convention）脆弱易錯 |
| **Compile-time weaving** | 編譯後處理，最常見是 IL weaving：工具讀編譯好的 assembly，織入 aspect 再替換原檔 | 舊程式就算沒對介面寫程式也能加 | 把 Volatile Dependency 注入 aspect 會造成 Temporal Coupling 或 Interdependent Tests；aspect 在編譯期織入，無法不帶 aspect 呼叫程式 → 難測、彈性低；**是 DI 的反面** |

（IL＝.NET 的中間語言，類似 bytecode。）後兩種在第 11 章細講。

---

### 重點 3：SOLID 五原則複習

> **原文引用（p.306）**
> "None of the principles encapsulated by SOLID represent absolutes. They're guidelines that can help you write clean code. To us, they represent goals that help us decide which direction we should take our applications."

| 原則 | 一句話 | 書中哪裡講過 |
|---|---|---|
| **S**RP | 每個 class 只有一個改變的理由；從內聚力看 | 2.1.3 |
| **O**CP | 擴充系統不用改現有 class；跟 DRY 目標相同，但 OCP 談程式碼、DRY 談知識 | 4.4.2 |
| **L**SP | 每個實作都照抽象定義的行為；Decorator 也必須遵守契約才能替換 | 8.1.1 |
| **I**SP | 用細的抽象不用寬的；消費者沒用到某些成員就是違反 | 6.2.1 |
| **D**IP | 對抽象寫程式；**消費端**決定抽象的形狀 | 3.1.2 |

> **原文引用（p.307）**
> "Every piece of knowledge must have a single, unambiguous, authoritative representation within a system."（DRY，出自《The Pragmatic Programmer》）

> **原文引用（p.307）**
> "The LSP is a foundation of DI. When consumers don't observe it, there's little advantage in injecting Dependencies, because you can't replace them at will, and you'll lose many (if not all) benefits of DI."

> **原文引用（p.308）**
> "If you find yourself adding members to an interface to satisfy the needs of other, specific implementations — including potential future implementations — then you're almost certainly violating the DIP."

作者說 ISP 在全書前面被忽略了，這章會發現它對 AOP 至關重要。

---

### 重點 4：AOP 不一定要工具

> **原文引用（p.309）**
> "A common misconception, however, is that AOP requires tooling. AOP tool vendors are all to eager to keep this fallacy alive. Our preferred approach is to practice AOP by design, which means you apply patterns and principles first, before reverting to specialized AOP tooling like dynamic Interception libraries."

判斷 OCP 不能只看某個時間點，要看**程式怎麼演變過來、預測接下來會怎麼變**。

> **原文引用（p.309）**
> "Being 100% closed for modification is neither possible nor desirable. Besides, conforming to the OCP is expensive. It takes considerable effort to find and design the appropriate Abstractions, although too many Abstractions can have a negative impact on the complexity of the application. Your job is to balance the risks and the costs and come up with a global optimum."

---

### 重點 5：Mary 的 `IProductService` 長大了

第 3 章只有一個方法，現在：

```csharp
public interface IProductService
{
    IEnumerable<DiscountedProduct> GetFeaturedProducts();
    void DeleteProduct(Guid productId);
    Product GetProductById(Guid productId);
    void InsertProduct(Product product);
    void UpdateProduct(Product product);
    Paged<Product> SearchProducts(int pageIndex, int pageSize, Guid? manufacturerId, string searchText);
    void UpdateProductReviewTotals(Guid productId, ProductReview[] reviews);
    void AdjustInventory(Guid productId, bool decrease, int quantity);
    void UpdateHasTierPricesProperty(Product product);
    void UpdateHasDiscountsApplied(Guid productId, string discountDescription);
}
```

每加一個商品相關功能就要改這個介面，而且 backlog 裡還有交叉銷售、商品圖片、商品評論等功能——**一定還會再改**。這是核心又常變的地方，值得改善。

---

### 重點 6：分析——同時違反 ISP、SRP、OCP

**違反 ISP**：不會有消費者用到全部方法。單元測試最痛：`HomeController` 只用 `GetFeaturedProducts`，但 Test Double 要實作全部，還要斷言其他方法沒被呼叫：

```csharp
public abstract class MockProductService : IProductService
{
    public virtual void DeleteProduct(Guid productId) { Assert.True(false, "Should not be called."); }
    public virtual Product GetProductById(Guid id)    { Assert.True(false, "Should not be called."); return null; }
    ...   // 10 個方法全部要寫
}
```

**違反 SRP**：「處理商品相關用例」這個職責太模糊。改變的理由很多：折扣規則變、庫存調整流程變、加搜尋條件、加新功能……

> **原文引用（p.312）**
> "A simple way to spot low cohesion is to check how easy it is to move some of the class's functionality to a new class. The easier this is, the lower the relatedness of the two parts, and the more likely SRP is violated."

**違反 OCP**：可預期的兩種變化都會造成大範圍修改：
1. 加新功能 → `ProductService`、所有 Decorator、所有 Test Double 都要改
2. 加新 Cross-Cutting Concern → 要替 `IProductService`、`ICustomerService`、`IOrderService`…每個都寫 Decorator，每個方法都重複

> **原文引用（p.313）**
> "The amount of changes you need to make to the existing Decorators grows proportionally with the amount of features in the system. This makes adding new aspects and features more expensive over time, up to the point that adding features becomes too costly."

用 AOP 工具只解決了「怎麼套 aspect」，沒解決底層設計問題：

> **原文引用（p.313）**
> "Although you shouldn't reject tool-based methods of AOP immediately, your first instinct should be to improve the application's design. Only when that doesn't solve the maintainability issues should you resort to the use of tooling."

---

### 重點 7：步驟 1——讀寫分離（CQS）

很多 aspect 只需要套在一部分方法上：稽核、驗證、容錯多半只套在**改狀態**的操作；快取只對**讀取**有意義。所以先把介面拆成 `IProductQueryServices`（只讀）和 `IProductCommandServices`（只寫）。

> **原文引用（p.315）**
> "Command-Query Separation (CQS) was coined by Bertrand Meyer ... each method should either ... Return a result, but not change the observable state of the system ... Change the state, but not produce any value"

Meyer 把產生值的方法叫 **query**，改狀態的叫 **command**。Mary 本來在方法層級就遵守 CQS，現在推到介面層級。

好處：交易 Decorator 只需要包 command 介面。但還是不夠：加新功能時被改到的 class 數量幾乎一樣。

（以下只處理 command；query 那邊作者說篇幅不夠，另見 Steven 的部落格。）

---

### 重點 8：步驟 2——拆成一個方法一個介面（ISP＋SRP）

「讓我們來點激進的。」把 7 個 command 方法各拆成一個單方法介面＋一個實作：

```csharp
public interface IAdjustInventoryService
{
    void AdjustInventory(Guid productId, bool decrease, int quantity);
}
public interface IUpdateProductReviewTotalsService
{
    void UpdateProductReviewTotals(Guid productId, ProductReview[] reviews);
}
...
```

好處：
- 每個介面都隔離了，沒有消費者被迫依賴用不到的方法
- 一個用例一個 class → 小、專注、單一職責
- 加新功能＝加一對新的介面＋實作，不用改現有 class

但 Decorator 更糟了：7 個介面 × 10 個 aspect ＝ **70 個 Decorator**；改一個 aspect 要改一堆 class。這些介面沒有共同點可以套 aspect——都回傳 `void`，但方法名、參數都不同。真的沒有共同點嗎？

---

### 重點 9：步驟 3——Parameter Object＋共同介面（OCP）

> **原文引用（p.317）**
> "A Parameter Object is a group of parameters that naturally go together."

把每個方法的參數包成一個物件，方法統一改名 `Execute`：

```csharp
public interface IAdjustInventoryService { void Execute(AdjustInventory command); }

public class AdjustInventory          // Parameter Object，純資料
{
    public Guid ProductId { get; set; }
    public bool Decrease { get; set; }
    public int Quantity { get; set; }
}
```

- Parameter Object 雖然是具體 class，但**屬於抽象的一部分**（純資料，藏在介面後面沒意義；實作搬到別的 assembly 時它留在抽象那邊）
- 這個物件就是一個 command 操作的**定義**，所以直接叫它 **command**
- command 只有一個參數、甚至零個參數都沒問題；`InsertProduct` 和 `UpdateHasTierPricesProperty` 參數都是一個 `Product`，但是兩種完全不同的 command

> **原文引用（p.319）**
> "At this point, you might think we're certifiably nuts and perhaps you're ready to toss this book out the window. This might be the mental discomfort we warned about at the beginning of this section."

> **原文引用（p.319）**
> "Although this refactoring increases the number of files in the project, assuming that each class and interface gets its own file in the project, you didn't change the executable code. Every method still contains the same amount of code as before."

現在浮現一個模式：每個抽象一個方法、都叫 `Execute`、都回傳 `void`、都只有一個參數 → 抽出共同介面：

```csharp
public interface ICommandService       // "One interface to rule them all!"
{
    void Execute(object command);
}

public class AdjustInventoryService : ICommandService
{
    public void Execute(object cmd)
    {
        var command = (AdjustInventory)cmd;     // 要轉型
        ...
    }
}
```

消費者：

```csharp
public class InventoryController : Controller
{
    private readonly ICommandService service;

    [HttpPost]
    public ActionResult AdjustInventory(AdjustInventoryViewModel viewModel)
    {
        if (!this.ModelState.IsValid) return this.View(viewModel);
        AdjustInventory command = viewModel.Command;     // view model 直接包著 command
        this.service.Execute(command);
        return this.RedirectToAction("Index");
    }
}
```

**對照 C**：這很像把各種 ioctl 統一成 `ioctl(fd, cmd, void *arg)`——一個入口、參數是一個結構。（筆記自己的對照，書中未提。）

---

### 重點 10：一個 aspect 只要一個 Decorator

所有改狀態的用例都實作同一個介面 → 每個 aspect 只寫一個 Decorator：

```csharp
public class TransactionCommandServiceDecorator : ICommandService
{
    private readonly ICommandService decoratee;
    public void Execute(object command)
    {
        using (var scope = new TransactionScope())
        {
            this.decoratee.Execute(command);
            scope.Complete();
        }
    }
}
```

（`TransactionScope`：包住任意程式碼成一個交易，期間建立的資料庫交易都會自動加入，不用沿著呼叫堆疊傳交易物件。.NET Core 不支援分散式交易——作者認為這反而是好事，本來就該避免。）

現在真的對修改封閉了：
- 加新 command 功能＝加一個 Parameter Object＋一個 `ICommandService` 實作，不改任何現有 class
- 加新功能不用新建或修改 Decorator
- 加新 Cross-Cutting Concern＝加**一個** Decorator
- 改一個 Cross-Cutting Concern＝改**一個** class

> **原文引用（p.323）**
> "the maintainability of the application improved dramatically because sweeping changes will be rare. This leads to the important realization that the number of classes in itself is a bad metric for measuring maintainability."

從 2 個型別變成 15 個（1 介面＋7 Parameter Object＋7 實作）。嫌 class 多不好找？用 namespace 分組（例如 `MyApp.Services.Products`），分組從 class 層級移到專案結構層級，專案結構直接反映程式的行為。

---

### 重點 11：意外造成的 LSP 違反

```csharp
ICommandService service =
    new TransactionCommandServiceDecorator(
        new UpdateProductReviewTotalsService(repository));   // 注入錯的實作
new InventoryController(service);                            // 編得過，執行時轉型失敗
```

`InventoryController` 傳 `AdjustInventory` 進去，`UpdateProductReviewTotalsService` 轉型成 `UpdateProductReviewTotals` 失敗 → 換了實作就壞掉 → 違反 LSP。

> **原文引用（p.324）**
> "Because their primary method for Object Composition is based on this, DI Containers are bad for handling ambiguous Abstractions. LSP violations, therefore, tend to complicate your Composition Root when using a DI Container. Or, put differently, the use of a DI Container makes LSP violations more obvious, just as Constructor Injection makes SRP violations more obvious."

而且每個實作都要轉型，本身就是 code smell。

---

### 重點 12：步驟 4——泛型抽象 `ICommandService<TCommand>`（LSP）

```csharp
public interface ICommandService<TCommand>
{
    void Execute(TCommand command);
}

public class AdjustInventoryService : ICommandService<AdjustInventory>
{
    public void Execute(AdjustInventory command)    // 不用轉型
    {
        var productId = command.ProductId;
        ...
    }
}

public class InventoryController : Controller
{
    readonly ICommandService<AdjustInventory> service;   // 只能注入處理 AdjustInventory 的
    ...
}
```

注入錯的實作就**編不過**。主要目的不是省掉轉型，而是防止消費者違反 LSP。

> **原文引用（p.325）**
> "The signature of this new ICommandService<TCommand> is identical to IEventHandler<TEvent>'s, and that's no coincidence. This is the kind of structure that'll frequently emerge when you apply SOLID principles to your code base — that is, one-membered generic interfaces that accept and/or return messages based on their generic types."

各種框架對這種介面有不同名字：`IHandler<T>`、`ICommandHandler<T>`、`IMessageHandler<T>`、`IHandleMessages<T>`；有的回傳 `Task`、加 `CancellationToken`、方法叫 `Handle`。概念一樣。

**沒有泛型的語言**：用非泛型 `ICommandService`＋**Mediator** pattern（一個抽象接收任意 command，負責分派給正確的實作）。

泛型版交易 Decorator：

```csharp
public class TransactionCommandServiceDecorator<TCommand> : ICommandService<TCommand>
{
    private readonly ICommandService<TCommand> decoratee;
    public void Execute(TCommand command)
    {
        using (var scope = new TransactionScope())
        {
            this.decoratee.Execute(command);
            scope.Complete();
        }
    }
}

new InventoryController(
    new TransactionCommandServiceDecorator<AdjustInventory>(
        new AdjustInventoryService(repository)));
```

---

### 重點 13：aspect 要套在正確的粒度——業務操作，不是 repository

第 9 章的 aspect 都套在 repository 上。作者故意往上移一層到領域層：

> **原文引用（p.327）**
> "A single business action defined in the domain layer would potentially call multiple Repositories, or call the same Repository multiple times. If you were to apply, for instance, a transaction at the level of the repository, it'd still mean that the business operation could potentially run in dozens of transactions, which would endanger the correctness of the system."

> **原文引用（p.327）**
> "A single business operation should typically run in a single transaction."

套在 `ICommandService<TCommand>` 上的好處：

| Aspect | 為什麼這一層比較好 |
|---|---|
| Auditing | repository 層只記到個別 Entity 的變更，失去「為什麼改」；command 層記下整個操作 |
| Logging | 記下每個業務操作和它的資料，通常就不用在每個方法開頭 log |
| Performance monitoring | 99% 的請求時間花在業務操作本身，這裡是理想的量測邊界 |
| Security | repository 層太細；權限通常是以業務操作為單位。在 command 上標角色或權限，一個 Decorator 搞定 |
| Fault tolerance | 交易套在業務操作上，其他容錯也該在同一層；例如**資料庫 deadlock 重試**一定要包在交易 aspect 外面 |
| Validation | command 就是網頁送上來的資料；加上 Data Annotations attribute，MVC 會驗證；再加一個 Decorator 用 `Validator` 當保險 |

---

### 重點 14：稽核 aspect——記下 command 本身

```csharp
public class AuditingCommandServiceDecorator<TCommand> : ICommandService<TCommand>
{
    private readonly IUserContext userContext;
    private readonly ITimeProvider timeProvider;
    private readonly CommerceContext context;
    private readonly ICommandService<TCommand> decoratee;

    public void Execute(TCommand command)
    {
        this.decoratee.Execute(command);
        this.AppendToAuditTrail(command);
    }

    private void AppendToAuditTrail(TCommand command)
    {
        var entry = new AuditEntry
        {
            UserId = this.userContext.CurrentUser.Id,
            TimeOfExecution = this.timeProvider.Now,
            Operation = command.GetType().Name,                          // 用 reflection 取名字
            Data = Newtonsoft.Json.JsonConvert.SerializeObject(command)  // 序列化成 JSON
        };
        this.context.AuditEntries.Add(entry);
        this.context.SaveChanges();
    }
}
```

結果（Table 10.2，節錄）：

| User | Time | Operation | Data |
|---|---|---|---|
| Mary | 2018-12-24 11:20 | AdjustInventory | `{ ProductId: "ae361...00bc", Decrease: false, Quantity: 2 }` |
| Mary | 2018-12-24 11:25 | UpdateHasDiscountsApplied | `{ ProductId: "ae361...00bc", DiscountDescription: "Test" }` |
| Mary | 2018-12-24 15:12 | UpdateProductReviewTotals | `{ ProductId: "5435...a845", Reviews: [{ Rating: 5, Text: "nice!" }] }` |

- 這個 Decorator 把稽核邏輯和裝飾邏輯合在一起——因為它已經套在所有業務操作上，沒必要再分享給別的 class。但它依賴 `CommerceContext`，所以要放資料層或 Composition Root
- 可以把資料反序列化回 command **重播**失敗的操作，或拿來做真實的壓力測試
- 只記成功的；同樣機制加上 stack trace 就能記失敗的（開發者很需要）

> **原文引用（p.330）**
> "The application design went from an RPC-like method-calling model to a message-passing model. These messages can be serialized, queued, logged, and replayed — all abilities that are harder to achieve with a method-calling model, such as the initial IProductService implementation from listing 10.4."

---

### 重點 15：好設計的程式只有少數幾行 log

作者的經驗：SOLID 設計下的幾個 Decorator 就能滿足 **98%** 的 logging 需求。其他配合做法：

1. 不要「記一筆然後繼續執行」，**丟例外**
2. 不要「抓意外例外、記錄、然後在操作中途繼續」——讓例外往上冒，在呼叫堆疊頂端統一記錄；fail fast 也避免讓使用者以為請求成功了
3. 方法寫小——例外的 stack trace 更能看出走了哪條路

效能監控也一樣：在操作細節旁邊多存一個執行時間，就能看出哪些操作越來越慢。

---

### 重點 16：權限 aspect——用 passive attribute 標在 command 上

第 9 章的權限 Decorator 只管 `IProductRepository`，角色寫死 Administrator。現在一個 Decorator 包所有業務操作，有些操作要給其他角色 → 用 **passive attribute**。

> **原文引用（p.332）**
> "A passive attribute provides metadata rather than behavior. Passive attributes prevent the Control Freak anti-pattern, because aspect attributes that include behavior are often Volatile Dependencies."

（C# attribute：掛在 class、方法上的中繼資料，執行期可用 reflection 讀出來。）

```csharp
public class PermittedRoleAttribute : Attribute
{
    public readonly Role Role;
    public PermittedRoleAttribute(Role role) { this.Role = role; }
}
public enum Role { PreferredCustomer, Administrator, InventoryManager }

[PermittedRole(Role.InventoryManager)]
public class AdjustInventory { ... }

[PermittedRole(Role.Administrator)]
public class UpdateProductReviewTotals { ... }
```

允許的角色**變成 command 定義的一部分**。

```csharp
public class SecureCommandServiceDecorator<TCommand> : ICommandService<TCommand>
{
    private static readonly Role PermittedRole = GetPermittedRole();   // 每種 TCommand 算一次
    private readonly IUserContext userContext;
    private readonly ICommandService<TCommand> decoratee;

    public void Execute(TCommand command)
    {
        if (!this.userContext.IsInRole(PermittedRole)) throw new SecurityException();
        this.decoratee.Execute(command);
    }

    private static Role GetPermittedRole()
    {
        var attribute = typeof(TCommand).GetCustomAttribute<PermittedRoleAttribute>();
        if (attribute == null)
            throw new InvalidOperationException("[PermittedRole] missing.");   // 沒標就拒絕，不預設放行
        return attribute.Role;
    }
}
```

- passive attribute 跟使用它的 aspect 沒有直接關係，同一份中繼資料可以被多個 aspect 用（對比第 11 章的 aspect attribute）
- 用領域自己定義的 attribute，不要用框架的（例如 ASP.NET Core 的 `[Authorize]` 會把 `Microsoft.AspNetCore.Authorization.dll` 拖進領域層）

**授權的其他做法**：用 namespace 代表角色（`MyApp.Domain.Commands.Administrator`）；用**權限**而非角色（更細，由管理員設定使用者／角色／權限的對應）；**row-based security**（同角色的人也只能管某些區域的商品）。

---

### 重點 17：在 Composition Root 疊三層 Decorator

```csharp
ICommandService<AdjustInventory> service =
    new SecureCommandServiceDecorator<AdjustInventory>(          // 最外：權限
        this.userContext,
        new TransactionCommandServiceDecorator<AdjustInventory>( // 中：交易
            new AuditingCommandServiceDecorator<AdjustInventory>(// 內：稽核
                this.userContext, this.timeProvider, context,
                new AdjustInventoryService(repository))));        // 真正的實作
return new InventoryController(service);
```

會有很多 command service，每個都要同樣的 Decorator → 抽成方法：

```csharp
private ICommandService<TCommand> Decorate<TCommand>(
    ICommandService<TCommand> decoratee, CommerceContext context)
{
    return new SecureCommandServiceDecorator<TCommand>(this.userContext,
        new TransactionCommandServiceDecorator<TCommand>(
            new AuditingCommandServiceDecorator<TCommand>(
                this.userContext, this.timeProvider, context, decoratee)));
}

var service = Decorate(new AdjustInventoryService(repository), context);
return new InventoryController(service);
```

Composition Root 也完全 DRY 了。第 12 章會用容器 Auto-Register 所有實作並自動套 Decorator。

---

### 重點 18：結論——重點是 SOLID，不是 `ICommandService`

- 重構沒改任何業務邏輯，只是把 command 這個概念**變顯性**
- 跟 ASP.NET Core MVC 的做法類似：controller 都繼承 `Controller`，框架才能用 reflection 找到它們、有共同 API 可操作
- 除了 command，**query** 也值得自己的抽象（篇幅關係沒寫）

> **原文引用（p.337）**
> "But the goal of this chapter wasn't to state that the ICommandService<TCommand> Abstraction is the way to design your applications. The important takeaway from this chapter should be that designing applications according to SOLID is the way to keep applications maintainable."

> **原文引用（p.337）**
> "But let's not beat around the bush. Writing maintainable software is hard, even when you try to apply the SOLID principles. Besides, you often work in projects that aren't designed to stand the test of time. It might be unfeasible or dangerous to make big architectural changes. At those times, using AOP tooling might be your only viable option, even if it presents you with a temporary solution."

**要記住的觀點**：當一群元件有共同的形狀（單方法泛型介面），Cross-Cutting Concern 就能用一個 Decorator 套到全部。

---

### 本章 Summary（作者原話）

> **原文引用（p.337–339）**
> - "The Single Responsibility Principle (SRP) states that each class should have only one reason to change. This can be viewed from the perspective of cohesion. Cohesion is defined as the functional relatedness of the elements of a class or module. The lower the amount of relatedness, the lower the cohesion; and the lower the cohesion, the greater the chance a class violates the SRP."
> - "The Open/Closed Principle (OCP) prescribes an application design that prevents you from having to make sweeping changes throughout the code base. A strong relationship between the OCP and the DRY principle is that they both strive for the same objective."
> - "The Don't Repeat Yourself (DRY) principle states that every piece of knowledge must have a single, unambiguous, authoritative representation within a system."
> - "The Liskov Substitution Principle (LSP) states that every implementation should behave as defined by its Abstraction. This lets you replace the originally intended implementation with another implementation of the same Abstraction without worrying about breaking a consumer. It's a foundation of DI. When consumers don't observe it, there's little advantage in injecting Dependencies, because you can't replace Dependencies at will, and you lose many (if not all) benefits of DI."
> - "The Interface Segregation Principle (ISP) promotes the use of fine-grained Abstractions rather than wide Abstractions. Any time a consumer depends on an Abstraction where some of the members stay unused, this principle is violated. This principle is crucial when it comes to effectively applying Aspect-Oriented Programming."
> - "The Dependency Inversion Principle (DIP) states that you should program against Abstractions and that the consuming layer should be in control of the shape of a consumed Abstraction. The consumer should be able to define the Abstraction in a way that benefits itself the most."
> - "These five principles together form the SOLID acronym. None of the SOLID principles represents absolutes. They're guidelines that can help you write clean code."
> - "Aspect-Oriented Programming (AOP) is a paradigm that focuses on the notion of applying Cross-Cutting Concerns effectively and maintainably."
> - "The most compelling AOP technique is SOLID. A SOLID application prevents code duplication during normal application code and implementation of Cross-Cutting Concerns. Using SOLID techniques can also help developers avoid the use of specific AOP tooling."
> - "Even with a SOLID design, there likely will come a time where a change becomes sweeping. Being 100% closed for modification is neither possible nor desirable. Conforming to the OCP takes considerable effort when finding and designing the appropriate Abstractions, although too many Abstractions can have a negative impact on the complexity of the application."
> - "Command-Query Separation (CQS) is an influential object-oriented principle that states that each method should either return a result but not change the observable state of the system, or change the state but not produce any value."
> - "Placing command methods and query methods in different Abstractions simplifies applying Cross-Cutting Concerns, because the majority of aspects need to be applied to either commands or queries, but not both."
> - "A Parameter Object is a group of parameters that naturally go together. The extraction of Parameter Objects allows the definition of a reusable Abstraction that can be implemented by a large group of components. This allows these components to be handled similarly and Cross-Cutting Concerns to be applied effectively."
> - "Rather than a component's Abstraction, these extracted Parameter Objects become the definition of a distinct operation or use case in the system."
> - "Although splitting larger classes into many smaller classes with Parameter Objects can drastically increase the number of classes in a system, it can also dramatically improve the maintainability of a system. The number of classes in a system is a bad metric for measuring maintainability."
> - "Cross-Cutting Concerns should be applied at the right granular level in the application. For all but the simplest CRUD applications, Repositories aren't the right granular level for most Cross-Cutting Concerns. With the application of SOLID principles, reusable one-membered Abstractions typically emerge as the levels where Cross-Cutting Concerns need to be applied."

### 我的整理：本章帶走的五句話

1. AOP 首選是設計，不是工具：讓一群元件有共同的形狀，一個 Decorator 就能套到全部。
2. 四步重構：讀寫分離（CQS）→ 一方法一介面（ISP/SRP）→ 參數包成 command 物件、統一 `Execute`（OCP）→ 泛型化 `ICommandService<TCommand>`（LSP）。
3. class 數量不是可維護性的指標；「加功能要改幾個地方」才是。
4. aspect 套在業務操作那一層（一個業務操作一個交易），不是 repository 那一層。
5. 從「呼叫方法」變成「傳訊息」：command 可以序列化、排隊、記錄、重播；權限用 passive attribute 寫在 command 定義上。

---

## 第 11 章：Tool-based Aspect-Oriented Programming（用工具做 AOP）

> 對應原書 p.341–356

### 這章在幹嘛

第 10 章的續篇。如果第 10 章的設計改動太激進，還有兩種靠工具的 AOP：

1. **Dynamic Interception（動態攔截）**：執行期自動產生 Decorator。作者說是「次佳選擇」，可以當過渡方案
2. **Compile-time weaving（編譯期織入）**：改寫編譯好的程式。作者說**這是 DI 的反面、是 anti-pattern**，之所以介紹是要讓你知道它不是 DI 的替代品

（想深入工具式 AOP，作者推薦 Matthew D. Groves《AOP in .NET》。）

---

### 重點 1：Decorator 是機械化的工作，可以交給工具

第 10 章 Circuit Breaker Decorator 的 `Delete`、`Insert` 幾乎一樣。作者說寫 aspect 最難的是設計「樣板」，之後就是機械化的三步：

1. 建一個新的 Decorator class
2. 實作目標介面
3. 每個成員套用樣板

.NET 能在執行期**動態產生型別**（emit types）——沒有原始碼檔，直接從抽象模型編譯出 class。所以可以在執行期自動產生 Decorator。

流程（Figure 11.1、11.2）：

```text
Client ─► RuntimeGeneratedDecorator ─► LoggingAspect.Intercept(invocation)
                                            │  ……前置
                                            ├─► invocation.Proceed() ─► SomeRepository.Insert()
                                            │  ……後置
                                       ◄────┘
```

你還是要自己寫 aspect（Interceptor），再告訴攔截函式庫要套到哪些抽象上。

**對照 C**：有點像 Linux 的 ftrace／kprobe 在函式邊界插入 handler，原本的函式不用改。（筆記自己的對照，書中未提。）

---

### 重點 2：用 Castle Dynamic Proxy 寫 Circuit Breaker Interceptor

> **原文引用（p.344）**
> "Castle Dynamic Proxy is the de facto standard tool for dynamic Interception in .NET. It's free and open source. In fact, most DI Containers use it under the hood for their dynamic Interception functionality."

```csharp
public class CircuitBreakerInterceptor : Castle.DynamicProxy.IInterceptor
{
    private readonly ICircuitBreaker breaker;

    public CircuitBreakerInterceptor(ICircuitBreaker breaker)    // 可以用 Constructor Injection
    {
        this.breaker = breaker;
    }

    public void Intercept(IInvocation invocation)                // 唯一要實作的方法
    {
        this.breaker.Guard();
        try
        {
            invocation.Proceed();                                 // 讓呼叫繼續到被包的實例
            this.breaker.Succeed();
        }
        catch (Exception ex)
        {
            this.breaker.Trip(ex);
            throw;
        }
    }
}
```

- 跟第 10 章的差別：不呼叫特定方法，而是用 `IInvocation` 代表「任何一次方法呼叫」
- `IInvocation` 可以取得方法名、參數值，也能在 `Proceed` 前設回傳值

Composition Root（Pure DI 照用）：

```csharp
var generator = new Castle.DynamicProxy.ProxyGenerator();     // 應該建一次、整個程式重用
var breaker = new CircuitBreaker(TimeSpan.FromMinutes(1));
var interceptor = new CircuitBreakerInterceptor(breaker);
var wcfRepository = new WcfProductRepository();

IProductRepository repository = generator
    .CreateInterfaceProxyWithTarget<IProductRepository>(wcfRepository, interceptor);
```

（Castle 把 Decorator 叫 Proxy。）

---

### 重點 3：Dynamic Interception 和 AOP by design 的共同點

- 都是對抽象寫程式時處理 Cross-Cutting Concern
- Interceptor 跟 Decorator 一樣可以用 Constructor Injection → DI 友善，跟被裝飾的程式解耦，業務碼和 aspect 都好測
- aspect 可以集中在 Composition Root，不重複；同一個 solution 有多個程式時，可以只在某個 Composition Root 套用

---

### 重點 4：Dynamic Interception 的四個缺點（Table 11.1）

| Disadvantage | Summary |
|---|---|
| **失去編譯期支援** | 攔截程式比 Decorator 複雜，難讀難維護 |
| **aspect 跟工具緊耦合** | 更難測；Interceptor 必須放在 Composition Root，才不會讓其他 assembly 依賴攔截函式庫 |
| **不是哪裡都能套** | 只能套在 virtual 或 abstract 方法的邊界（例如介面方法） |
| **沒修好底層設計問題** | 系統只比原本好一點點，比 SOLID 設計差很多 |

細節：

1. **編譯期支援**：Castle 的 `IInvocation.Arguments` 回傳 `object[]`，讀寫參數要轉型，值型別（int、bool）還有 boxing。效能影響通常可忽略（I/O 貴好幾個數量級），但寫起來要處理轉型、型別檢查和轉型錯誤
2. **工具耦合**：每個 aspect 都實作 `Castle.DynamicProxy.IInterceptor`、用 `IInvocation` → 多一個外部函式庫要學，多了成本和風險（第 12.3.1 節細講）
3. **適用範圍**：private 方法攔截不到（不在介面上）。AOP by design 也有同樣限制，但那時你會刻意把抽象設計成只需要在邊界套 aspect；用動態攔截時，你通常是接受現狀（作者認為「需要攔截 private 方法」本身就是 code smell）
4. **設計問題**：第 10 章那個大 `IProductService` 的問題不只影響 Cross-Cutting Concern；動態攔截只處理了 aspect，其他部分沒變

> **原文引用（p.348）**
> "You'll still end up with a system that's only marginally more maintainable than the existing design and considerably less maintainable than a more SOLID-based design This is because dynamic Interception only considers the application of Cross-Cutting Concerns — not other parts of your code."

**要記住的觀點**：動態攔截是不想（或還不能）大改設計時的過渡方案。

---

### 重點 5：Compile-time weaving 是什麼

C# 編譯器把程式編成 IL（Intermediate Language），CLR 的 JIT（Just-In-Time）編譯器在執行時翻成機器碼。

**Compile-time weaving**：用特殊工具（post-compiler）讀編譯好的 assembly，把 aspect 的 IL 織進原本的 IL，寫回磁碟取代原檔（Figure 11.3、11.4）。

```text
C# 原始碼 ─► C# compiler ─► IL assembly ─► Post-compiler ─► 改過的 IL assembly
```

> **原文引用（p.349）**
> "Compile-time weaving isn't a desirable method of applying AOP for Volatile Dependencies. Favor applying the SOLID principles, or fall back to dynamic Interception if that isn't possible."

**對照 C**：概念上像用工具改寫編好的 object file／binary 插入程式碼（binary instrumentation），原始碼看不出來。（筆記自己的對照，書中未提。）

---

### 重點 6：用 PostSharp 加 `[Transaction]` 屬性

看起來很誘人：在方法上掛一個 attribute 就套用 aspect。

```csharp
public class SqlProductRepository : IProductRepository
{
    [Transaction] public void Insert(Product product) ...
    [Transaction] public void Update(Product product) ...
    [Transaction] public void Delete(Guid id) ...
    public IEnumerable<Product> GetAll() ...        // 不需要交易
}
```

（**aspect attribute**：書中用來指「實作或代表一個 aspect 的自訂 attribute」。）

PostSharp（商業工具）的實作：

```csharp
[AttributeUsage(...)]
[PostSharp.Serialization.PSerializable]
[PostSharp.Extensibility.MulticastAttributeUsage(...)]
public class TransactionAttribute : PostSharp.Aspects.OnMethodBoundaryAspect
{
    public override void OnEntry(MethodExecutionArgs args)
    {
        args.MethodExecutionTag = new TransactionScope();
    }
    public override void OnSuccess(MethodExecutionArgs args)
    {
        ((TransactionScope)args.MethodExecutionTag).Complete();
    }
    public override void OnExit(MethodExecutionArgs args)        // PostSharp 保證在 finally 呼叫
    {
        ((TransactionScope)args.MethodExecutionTag).Dispose();
    }
}
```

跟第 10 章的泛型交易 Decorator 比：要覆寫好幾個方法、掛一堆 attribute、用 `MethodExecutionTag` 在方法間傳狀態——樣板很多。

> **原文引用（p.351）**
> "Especially when it comes to writing code that contains catch, finally, or using blocks, a SOLID Decorator — and even the dynamic Interception approach — is superior in simplicity and maintainability to compile-time weaving as offered by tools such as PostSharp."

---

### 重點 7：缺點 1——aspect 無法注入依賴（DI-unfriendly）

| Disadvantage | Summary |
|---|---|
| **DI-unfriendly** | 沒有好方法把 Volatile Dependency 注入 aspect；替代方案會造成 Temporal Coupling、Captive Dependency、Interdependent Tests |
| **Compile-time coupling** | aspect 在編譯期織入，不可能不帶 aspect 呼叫程式 → 難測、彈性低 |

Cross-Cutting Concern 常常需要 Volatile Dependency，預設要用 Constructor Injection。但：

```csharp
public class CircuitBreakerAttribute : OnMethodBoundaryAspect
{
    private readonly ICircuitBreaker breaker;
    public CircuitBreakerAttribute(ICircuitBreaker breaker) { ... }   // 行不通
}

[CircuitBreakerAttribute(???)]     // 要塞什麼？怎麼塞？
```

1. .NET attribute 的建構子參數**只能是基本型別**（字串、整數等）
2. 就算可以，attribute 實例是 .NET runtime 建的，建立的時間和地點跟 Composition Root 完全不同，你無法介入

而且不只 attribute：任何 post-compiler 都是在**編譯期**把 aspect 織進程式；object graph 卻是在**執行期**由 Composition Root 組。兩個模型合不起來。

**兩個變通都是 hack**：

**Ambient Context**：

```csharp
public class CircuitBreakerAttribute : OnMethodBoundaryAspect
{
    public static ICircuitBreaker Breaker { get; set; }      // 啟動時在 Composition Root 設定
    public override void OnEntry(MethodExecutionArgs args) { Breaker.Guard(); }
}
```

- **Temporal Coupling**：忘了設就 `NullReferenceException`
- 只能在啟動時設一次 → 必須是 static → 可能變成 **Captive Dependency**
- static 值留在記憶體 → **Interdependent Tests**，每個測試後都要 teardown

**Service Locator**：更糟——同樣的問題，再加上看不出依賴是什麼、還多拖一個多餘依賴。作者連例子都省了。

---

### 重點 8：缺點 2——編譯期耦合，測試很難

> **原文引用（p.354）**
> "Although compile-time weaving decouples your source code from your aspects, it still causes your compiled code to be tightly coupled with the woven aspects. This is a problem, because Cross-Cutting Concerns often depend on an external system."

- **單元測試**要隔離，不能跨行程、網路（資料庫、檔案系統）；織入的 aspect 讓你做不到
- **整合測試**也有問題：測 `SqlProductRepository` 跟資料庫的互動時，通常不想套所有 aspect。例如 `[CheckAuthorization]` 會逼每個整合測試先跑登入流程 → 難維護、變慢
- 更好笑的：有快取 aspect 時，你想測資料庫查詢，結果測試一直打到快取，根本沒碰資料庫

你需要完全控制「哪個測試、什麼時候套哪些 aspect」；編譯期織入讓這件事極難。

---

### 重點 9：結論——對 Volatile Dependency 用編譯期織入是 anti-pattern

> **原文引用（p.355）**
> "The aim of DI is to manage Volatile Dependencies by introducing Seams into your application. This enables you to centralize the composition of your object graphs inside the Composition Root. This is the complete opposite of what you achieve when applying compile-time weaving: it causes Volatile Dependencies to be coupled to your code at compile time."

> **原文引用（p.355）**
> "We're explicitly talking about the use of compile-time weaving in combination with Volatile Dependencies. From a DI point of view, Stable Dependencies aren't that interesting. While you might find value in applying Stable Dependencies using compile-time weaving, there's no value in applying Volatile Dependencies using compile-time weaving."

**AOP 的優先順序**：

1. **SOLID 設計**（第 10 章）
2. 做不到 → **Dynamic Interception**
3. **Compile-time weaving**：不要用在 Volatile Dependency 上

**對比第 10 章的 passive attribute**：passive attribute 只帶中繼資料（例如允許的角色），不含行為，由 Decorator 讀取——所以沒有上面這些問題。aspect attribute 本身就含行為，才會卡在「沒辦法注入依賴」。

---

### 本章 Summary（作者原話）

> **原文引用（p.355–356）**
> - "Dynamic Interception is an Aspect-Oriented Programming (AOP) technique that automates the generation of Decorators to be emitted at runtime. Aspects are written as Interceptors, which are injected into a runtime-generated Decorator."
> - "Dynamic Interception exhibits the following disadvantages: – Loss of compile-time support. – Aspects are strongly coupled to the tooling. – Not universally applicable. – Doesn't fix underlying design problems."
> - "To prevent or delay making design changes like the ones we suggested in chapter 10, dynamic Interception might be a good temporary solution until its time to start making these kinds of improvements."
> - "Compile-time weaving is an AOP technique that alters the compilation process. It uses special tools to alter a compiled assembly using IL manipulation. It isn't a desirable method of applying AOP to Volatile Dependencies."
> - "In relation to DI, compile-time weaving exhibits the following problems: – Compile-time weaving aspects are DI-unfriendly. – Compile-time weaving causes tight coupling at compile time."
> - "Favor applying SOLID principles, falling back to dynamic Interception if that isn't possible."

### 我的整理：本章帶走的四句話

1. Dynamic Interception（Castle Dynamic Proxy）＝執行期自動產生 Decorator；Interceptor 還能用 Constructor Injection，但要轉型、綁工具、只能攔 virtual／介面方法。
2. 它只解決「怎麼套 aspect」，不修設計本身，適合當過渡方案。
3. Compile-time weaving（PostSharp）把 aspect 織進編譯結果：aspect 無法注入依賴（只能退回 Ambient Context／Service Locator），測試也無法拿掉 aspect——是 DI 的反面。
4. 優先順序：SOLID 設計 → 動態攔截 → 不要對 Volatile Dependency 用編譯期織入。

---

## Part 4 導讀：DI Containers（p.357–358）

前三部都只教 Pure DI——作者強調**不是推薦 Pure DI**，而是想讓你看到不被任何容器 API 污染的 DI 原貌。

| 章 | 容器 | 作者的態度 |
|---|---|---|
| 12 | （概論） | 什麼時候用容器、什麼時候用 Pure DI |
| 13 | **Autofac** | .NET 最流行的容器，市佔太大不能不講 |
| 14 | **Simple Injector** | （作者之一 Steven 是它的作者） |
| 15 | **Microsoft.Extensions.DependencyInjection（MS.DI）** | 作者持懷疑態度，功能有限；但很多人會先用內建工具，所以還是要講 |

第一版講過的 Castle Windsor、StructureMap、Spring.NET、Unity、MEF 都拿掉了（看第一版）；Ninject 很流行，但寫書時沒有 .NET Core 版所以不收。三章用同一個模板寫，方便對照。

> **原文引用（p.358）**
> "The information presented in part 4 was accurate at the time of writing, but always be sure to consult more up-to-date sources as well."

---

## 第 12 章：DI Container introduction（DI 容器概論）

> 對應原書 p.359–392

### 這章在幹嘛

DI 是技術，容器是輔助這個技術的工具——就像做冰淇淋的手法不變，但有了電動冰淇淋機就輕鬆很多。

骨架：

1. 容器是什麼：Resolve API、**Auto-Wiring**、自己寫一個 30 行的玩具容器看原理
2. 三種設定方式：設定檔、**Configuration as Code**、**Auto-Registration**
3. **什麼時候該用容器**（作者說每個人都該讀這節）

> **原文引用（p.360）**
> "A DI Container is a software library that provides DI functionality and automates many of the tasks involved in Object Composition, Interception, and Lifetime Management. It's an engine that resolves and manages object graphs."

---

### 重點 1：組物件是「Generic Subdomain」，可以交給通用函式庫

> **原文引用（p.361）**
> "Although it's possible to write all the required infrastructure code with Pure DI, it doesn't add much value to an application. On the other hand, the task of composing objects is of a general nature and can be resolved once and for all; this is what's known as a Generic Subdomain."

跟 logging、資料存取一樣，是可以用通用函式庫解決的一般性問題。（Generic Subdomain 出自 DDD。）

---

### 重點 2：Resolve API——長得就像 Service Locator

```csharp
var controller = (HomeController)container.Resolve(typeof(HomeController));       // Autofac
var controller = (HomeController)container.GetInstance(typeof(HomeController));   // Simple Injector
```

一般形狀：

```csharp
object Resolve(Type serviceType);    // 弱型別，要轉型
T Resolve<T>();                      // 泛型版
```

> **原文引用（p.362）**
> "The signature of the Resolve method is extremely powerful and versatile. You can request an instance of any type and your code still compiles. In fact, the Resolve method fits the signature of a Service Locator. As discussed in 5.2, you'll need to exercise care not to use your DI Container as a Service Locator by calling Resolve outside the Composition Root."

設定不完整時，好的容器會給清楚的錯誤訊息。Simple Injector 的範例：

```text
The constructor of type HomeController contains the parameter with name 'productService'
and type IProductService, which isn't registered. Please ensure IProductService is registered
or change the constructor of HomeController.
```

---

### 重點 3：Auto-Wiring（自動接線）

> **原文引用（p.363）**
> "Auto-Wiring is the ability to automatically compose an object graph from maps between Abstractions and concrete types by making use of type information supplied by the compiler and the Common Language Runtime (CLR)."

容器靠 reflection 讀 class 的建構子，就知道它要什麼依賴——所以**所有容器天生懂 Constructor Injection**。多數也懂 Property Injection，有些要明確開啟（作者說這是好事）。

演算法（Figure 12.2）：

```text
要求某個抽象的實例
  → 查設定找到對應的具體型別
  → 用 reflection 看它的建構子
  → 每個建構子參數型別：遞迴回到第一步
  → 用解出來的參數建構具體型別
  → 回傳
```

多數容器有快取最佳化讓後續請求更快；作者再次強調 I/O 才是一般程式的瓶頸，不是物件組裝。

---

### 重點 4：30 行玩具容器（只為教學，**不要用在正式環境**）

```csharp
public class AutoWireContainer
{
    Dictionary<Type, Func<object>> registrations = new Dictionary<Type, Func<object>>();

    public void Register(Type serviceType, Type componentType)
    {
        this.registrations[serviceType] = () => this.CreateNew(componentType);   // 只說建什麼，不說怎麼建
    }

    public void Register(Type serviceType, Func<object> factory)
    {
        this.registrations[serviceType] = factory;                               // 手動接線，繞過 Auto-Wiring
    }

    public object Resolve(Type type)
    {
        if (this.registrations.ContainsKey(type)) return this.registrations[type]();
        throw new InvalidOperationException("No registration for " + type);
    }

    private object CreateNew(Type componentType)                                 // Auto-Wiring 的核心
    {
        var ctor = componentType.GetConstructors()[0];
        var dependencies = from p in ctor.GetParameters() select this.Resolve(p.ParameterType);
        return Activator.CreateInstance(componentType, dependencies.ToArray());
    }
}
```

使用（Table 12.1 的對應）：

```csharp
var container = new AutoWireContainer();
container.Register(typeof(IUserContext), typeof(AspNetUserContextAdapter));
container.Register(typeof(IProductRepository), typeof(SqlProductRepository));
container.Register(typeof(IProductService), typeof(ProductService));
container.Register(typeof(HomeController), typeof(HomeController));      // 對應到自己
container.Register(typeof(CommerceContext), () => new CommerceContext(connectionString));

object controller = container.Resolve(typeof(HomeController));
```

- 註冊順序無關（內部是字典）
- `HomeController` 對應到自己：object graph 頂端的型別（例如 MVC controller）常這樣
- `CommerceContext` 要手動接線，因為建構子吃 `string`：程式裡有很多 `string` 型別的設定值，容器只看型別分不出要哪個

> **原文引用（p.370）**
> "When you start using a DI Container, it's not required that you abandon hand wiring object graphs altogether."

**對照 C**：就是一張「型別 → 建構函式」的查找表，加上遞迴解析參數。（筆記自己的對照，書中未提。）

**這個玩具的致命缺陷**：遇到循環依賴會 `StackOverflowException`，整個程式直接死掉、很難查原因。成熟的容器都能偵測循環而不讓程式崩潰——這是不要自己寫容器的理由之一。

---

### 重點 5：Auto-Wiring 的好處——改建構子不用改 Composition Root

如果 `AspNetUserContextAdapter` 新增一個 `CommerceContext` 依賴：

- **Pure DI**：Composition Root 要跟著改
- **Auto-Wiring**：什麼都不用改，`CommerceContext` 已經註冊過，容器自己會塞

> **原文引用（p.371）**
> "Although Auto-Wiring can decrease the required maintenance to the Composition Root, this still doesn't mean you should always prefer a DI Container over Pure DI."

---

### 重點 6：三種設定方式（Table 12.2）

|  | 明確（explicit） | 隱含（implicit） |
|---|---|---|
| **early binding** | Configuration as Code | ← Auto-Registration（介於中間）→ |
| **late binding** | 設定檔 | |

| Style | Description | Advantages | Disadvantages |
|---|---|---|---|
| **Configuration files** | 對應寫在設定檔（XML、JSON） | 不重編就能換 | 沒有編譯期檢查；冗長、脆弱 |
| **Configuration as Code** | 程式碼明確寫出對應 | 編譯期檢查；控制度高 | 不重編就不能換 |
| **Auto-Registration** | 用規則＋reflection 找元件並建立對應 | 不重編就能換；省力；幫助強制慣例、讓程式更一致 | 沒有編譯期檢查；控制度低；一開始看起來比較抽象 |

容器的歷史是從設定檔開始的，所以舊容器還支援；新的 Simple Injector、MS.DI 都**沒有內建**設定檔支援。

> **原文引用（p.373）**
> "You shouldn't go back and reconfigure the container once you've started resolving object graphs — that will only give you grief. This is related to the Register Resolve Release pattern"

---

### 重點 7：設定檔——只用在真正需要 late binding 的地方

2000 年代初的容器全都用 XML。問題：冗長、脆弱、拼錯字編譯器不會發現、就算名字對 assembly 也不一定在搜尋路徑上；表達力也比程式碼差（例如 lambda 就寫不進 XML 或 JSON）。JSON 比較好讀，但一樣脆弱冗長。

Autofac 的 JSON 範例：

```json
{
  "defaultAssembly": "Commerce.Web",
  "components": [
    { "services": [{ "type": "Commerce.Domain.IUserContext, Commerce.Domain" }],
      "type": "Commerce.Web.AspNetUserContextAdapter" },
    ...
    { "type": "Commerce.SqlDataAccess.CommerceContext, Commerce.SqlDataAccess",
      "parameters": { "connectionString": "Server=.;Database=MaryCommerce;Trusted_Connection=True;" } }
  ]
}
```

> **原文引用（p.374）**
> "Configuration files are as much a part of your Composition Root as is Configuration as Code and Auto-Registration. Using configuration files, therefore, doesn't make your Composition Root smaller, it just moves it."

**Steven 的故事——設定檔不會 scale**：一個累計上百人年的產品，用 Spring.NET（當時只支援 XML）而且那個版本沒有 Auto-Wiring——每個對應、每個建構子參數都要寫在 XML 裡。十幾個團隊同時改，冗長、脆弱、常常 merge conflict，每天浪費大量工時。作者說他們一開始就用 Pure DI 會好很多（至少編譯器能抓錯）。

容器不支援設定檔不該影響你的選擇：真正 late-bound 的元件頂多幾個，用第 1 章那幾行 reflection 就能從設定檔載入。

---

### 重點 8：Configuration as Code

> **原文引用（p.377）**
> "I often think that people are over-eager to define configuration files. Often a programming language makes a straightforward and powerful configuration mechanism."（Martin Fowler）

> **原文引用（p.377）**
> "In the context of DI Containers, Configuration as Code allows the container's configuration to be stored as source code. Each mapping between an Abstraction and a particular implementation is expressed explicitly and directly in code."

MS.DI 範例，跟上面的 JSON 做一樣的事：

```csharp
var services = new ServiceCollection();
services.AddSingleton<IUserContext, AspNetUserContextAdapter>();
services.AddTransient<IProductRepository, SqlProductRepository>();
services.AddTransient<IProductService, ProductService>();
services.AddTransient<HomeController>();                                  // 對應到自己
services.AddScoped<CommerceContext>(p => new CommerceContext(connectionString));
```

精簡得多，而且有編譯器檢查；泛型限制還能檢查具體型別是否符合抽象。缺點：每加一個新型別都要記得註冊，很多註冊長得很像。

---

### 重點 9：Auto-Registration＋Convention over Configuration

註冊會越來越重複：

```csharp
services.AddTransient<IProductRepository, SqlProductRepository>();
services.AddTransient<ICustomerRepository, SqlCustomerRepository>();
services.AddTransient<IOrderRepository, SqlOrderRepository>();
...
services.AddTransient<IProductService, ProductService>();
services.AddTransient<ICustomerService, CustomerService>();
...
```

> **原文引用（p.379）**
> "Auto-Registration is the ability to automatically register components in a container by scanning one or more assemblies for implementations of desired Abstractions, based on a certain convention. Auto-Registration is sometimes referred to as Batch Registration or Assembly Scanning."

**Convention over Configuration（慣例優於設定）**：大家講好慣例，不用寫一堆設定。例：ASP.NET Core MVC 收到 `Home` 的請求，就去找叫 `HomeController` 的 class。

> **原文引用（p.380）**
> "Convention over Configuration has more advantages than just supporting DI configuration. It makes your code more consistent, because it automatically works, as long as you follow your conventions."

用 Simple Injector＋LINQ 寫慣例：「同一個 assembly、名字結尾是 Repository、只實作一個介面」：

```csharp
var assembly = typeof(SqlProductRepository).Assembly;
var repositoryTypes =
    from type in assembly.GetTypes()
    where !type.Abstract
    where type.Name.EndsWith("Repository")
    select type;

foreach (Type type in repositoryTypes)
    container.Register(type.GetInterfaces().Single(), type);
```

（MS.DI 本身**沒有** Auto-Registration 功能，但這段 LINQ 幾乎一樣，只有迴圈內呼叫容器 API 那行不同。）

只有 4 個 repository 時程式碼反而變多；但寫好之後，**幾百個元件也不用多寫一行**。

---

### 重點 10：泛型抽象的 Auto-Registration——一行註冊全部、一行套 Decorator

用 reflection 查泛型型別很痛苦（要找泛型介面的實作、處理多介面、還要套 Decorator），所以 Simple Injector 的 Auto-Registration API 是圍繞泛型抽象設計的：

```csharp
Assembly assembly = typeof(AdjustInventoryService).Assembly;
container.Register(typeof(ICommandService<>), assembly);     // 註冊所有 ICommandService<T> 實作
```

**open generic vs. closed generic**：`ICommandService<TCommand>`（沒填型別參數，C# 寫成 `ICommandService<>`）是 open generic；`ICommandService<AdjustInventory>`（填好了）是 closed generic。

套第 10 章那三層 Decorator：

```csharp
container.RegisterDecorator(typeof(ICommandService<>), typeof(AuditingCommandServiceDecorator<>));
container.RegisterDecorator(typeof(ICommandService<>), typeof(TransactionCommandServiceDecorator<>));
container.RegisterDecorator(typeof(ICommandService<>), typeof(SecureCommandServiceDecorator<>));
```

依註冊順序由內往外包：稽核 → 交易 → 權限，結果跟第 10 章手寫的 `Decorate` 方法一樣。Decorator 有泛型限制時，Simple Injector 還會自動判斷適不適用。

不用 Auto-Registration 的話，要替**每個 command × 每個 Decorator** 寫一行（Listing 12.14），Composition Root 爆炸式成長。

> **原文引用（p.384）**
> "The most prominent downside of Auto-Registration is that you lose some control. It must be possible to Auto-Wire every component that's picked up by the Auto-Registration facility. When there's a particular component that requires hand wiring, it should be excluded from Auto-Registration to prevent errors."

> **原文引用（p.385）**
> "Auto-Registration prevents the Composition Root from constantly being updated. It's a powerful technique that has the potential to make the DI Container invisible."

---

### 重點 11：混合使用

三種方式不互斥。經驗法則：

> **原文引用（p.385）**
> "As a rule of thumb, you should prefer Auto-Registration as a starting point, complemented by Configuration as Code to handle more special cases. You should reserve configuration files for cases where you need to be able to vary an implementation without recompiling the application — which is rarer than you may think."

---

### 重點 12：第三方函式庫有成本和風險

容器多半開源免費，但工時才是最貴的：

- **學習曲線**：API、行為、怪癖、限制；整個團隊都要懂。只有一個人懂就是 **bus factor** 風險（多少人被公車撞到專案就停擺）
- **理念不合**：函式庫的設計觀點可能跟你的不同，要寫一堆 workaround（yak shaving，為了做 A 先得做 B、C、D 的瑣事）
- **被放棄**：維護者不再維護 → 要換函式庫，重新付學習和遷移成本。連微軟也放棄過 Workflow Foundation、Silverlight、LightSwitch、Windows Phone、Windows RT

一般來說不用外部函式庫就得自己造輪子，通常更慘。但**容器不一樣——替代方案不是自己寫，而是 Pure DI**。

---

### 重點 13：不要自己寫 DI 容器

> **原文引用（p.387）**
> "A fully functional DI Container should support Lifetime Management, Interception, Auto-Registration, and Dependency cycle detection; communicate configuration mistakes effectively; have properly designed extensibility points; rest on great documentation; and much, much more. This isn't something you'll be able to do in a couple of weeks."

Steven（Simple Injector 作者）說這要好幾年才會穩定成熟。不是說永遠不該寫新的開源容器——創新很重要——但花雇主的錢做這件事，成本會遠超過你的預期。

容器只出現在 Composition Root，降低了換容器的風險，但換容器、學新 API 和新理念仍然費時。

---

### 重點 14：Pure DI 的優點——好學、強型別、回饋最快

> **原文引用（p.388）**
> "Pure DI has a big advantage that's often overlooked: it's strongly typed. This allows the compiler to provide feedback about correctness, which is the fastest feedback that you can get."

- 好學：不用學任何容器 API；找到 Composition Root 就看得懂 object graph 怎麼建的
- Composition Root 結構良好（Singleton 和 Scoped 分開建）時，**編譯器甚至能抓到 Captive Dependency**
- 容器用 reflection，設定錯誤要到執行期 resolve 時才發現——容器搶了編譯器的工作

**Pure DI 的缺點**：
- 強型別是兩面刃：每次重構建構子都會打壞 Composition Root；多個程式共用一個 library 就有多個 Composition Root 要改；作者看過一天改好幾次的專案，多人開發還會 merge conflict
- 編譯器能檢查的有限，抓不到：
  1. 建構子內丟例外（例如 Guard Clause 失敗）
  2. disposable 元件離開 scope 時有沒有被 dispose
  3. 該是 Singleton／Scoped 的 class 在 Composition Root 別處又被（意外地）建了一次、可能用不同 lifestyle（Simple Injector 叫這 Torn Lifestyles、Ambiguous Lifestyles）
- Composition Root 大小**跟程式大小線性成長**；小的時候這些錯誤很好找，大了就容易漏

容器能幫忙：自動處理 disposable、偵測 Captive Dependency 等常見陷阱（本書三個容器都能某種程度偵測 Captive Dependency）。

---

### 重點 15：結論——Pure DI 或 Auto-Registration，不要只用 Configuration as Code

如果用容器但只用 Configuration as Code（每個元件逐一註冊）：失去強型別的回饋；維護負擔因 Auto-Wiring 降低一些，但每加一個 class 還是要註冊（線性成長）；還要學容器 API、承擔換容器風險——**可能得不償失**。

> **原文引用（p.389）**
> "The use of Convention over Configuration using Auto-Registration can minimize the amount of maintenance on the Composition Root to almost zero."

Figure 12.6 的價值圖：

```text
          有價值
   Pure DI  │  Auto-Registration
  (簡單)    │    (精巧)
  ──────────┼──────────
            │  Configuration as Code
          沒意義
```

> **原文引用（p.389）**
> "a Composition Root should either be focused around Pure DI with, perhaps, a few late-bound types, or around Auto-Registration with, optionally, a limited amount of Configuration as Code, Pure DI, and configuration files. A Composition Root that focuses around Configuration as Code is pointless and should therefore be avoided."

> **原文引用（p.390）**
> "In general, though, you should use Pure DI for Composition Roots that are small and switch to Auto-Registration when maintaining such a Composition Root becomes a problem."

作者也推崇讓程式保持小（Bounded Context，DDD 概念）。

**Mark 的故事——太自動魔法（automagical）**：他在一個客戶那裡導入以慣例為基礎的 Auto-Registration。團隊擁抱 DI 和 TDD，但不想學容器 API。慣例大多運作良好；偶爾有人用了慣例沒預料到的做法，就得改慣例——只有 Mark 會改，他變成瓶頸。他離開時預期團隊會把容器拆掉換回 Pure DI。一年後回去，果然如此。「我不怪他們。」

**作者不告訴你選哪個容器**：除了技術評估，還要看授權模式、信不信任維護者、合不合公司 IT 策略；也不要只看本書介紹的三個。

> **原文引用（p.391）**
> "The most important thing to understand is that the use of DI in no way depends on the use of a DI Container."

---

### 本章 Summary（作者原話）

> **原文引用（p.391–392）**
> - "A DI Container is a library that provides DI functionality. It's an engine that resolves and manages object graphs."
> - "DI in no way hinges on the use of a DI Container. A DI Container is a useful, but optional, tool."
> - "Auto-Wiring is the ability to automatically compose an object graph from maps between Abstractions and concrete types by making use of the type information as supplied by the compiler and the Common Language Runtime (CLR)."
> - "Constructor Injection statically advertises the Dependency requirements of a class, and DI Containers use that information to Auto-Wire complex object graphs."
> - "Auto-Wiring makes a Composition Root more resilient to change."
> - "When you start using a DI Container, you're not required to abandon hand wiring object graphs altogether. You can use hand wiring in parts of your configuration when this is more convenient."
> - "When using a DI Container, the three configuration styles are configuration files, Configuration as Code, and Auto-Registration."
> - "Configuration files are as much a part of your Composition Root as Configuration as Code and Auto-Registration. Using configuration files, therefore, doesn't make your Composition Root smaller, it just moves it."
> - "As your application grows in size and complexity, so will your configuration file. Configuration files tend to become brittle and opaque to errors, so only use this approach when you need late binding."
> - "Don't let the absence of support for handling configuration files influence your choice for picking a DI Container. Types can be loaded from configuration files in a few simple statements."
> - "Configuration as Code allows the container's configuration to be stored as source code. Each mapping between an Abstraction and a particular implementation is expressed explicitly and directly in code. This method is preferred over configuration files unless you need late binding."
> - "Convention over Configuration is the application of conventions to your code to facilitate easier registration."
> - "Auto-Registration is the ability to automatically register components in a container by scanning one or more assemblies for implementations of desired Abstractions, which is a form of Convention over Configuration."
> - "Auto-Registration helps avoid constantly updating the Composition Root and is, therefore, preferred over Configuration as Code."
> - "Using external libraries such as DI Containers incurs costs and risks; for example, the cost of learning a new API and the risk of the library being abandoned."
> - "Avoid building your own DI Container. Either use one of the existing, well-tested, and freely available DI Containers, or practice Pure DI. Creating and maintaining such a library takes a lot of effort, which is effort not spent producing business value."
> - "The big advantage of Pure DI is that it's strongly typed. This allows the compiler to provide feedback about correctness, which is the fastest feedback that you can get."
> - "You should use Pure DI for Composition Roots that are small and switch to Auto-Registration whenever maintaining such Composition Roots becomes a problem. Bigger applications with many classes that can be captured by several conventions can greatly benefit from using Auto-Registration."

### 我的整理：本章帶走的五句話

1. 容器＝「型別對應表＋用 reflection 遞迴解建構子參數」（Auto-Wiring）；`Resolve` 只能在 Composition Root 呼叫，否則就是 Service Locator。
2. 三種設定：設定檔（只給真正要 late binding 的少數幾個）、Configuration as Code、Auto-Registration（用慣例一次註冊一整群）。
3. Composition Root 要嘛 Pure DI，要嘛 Auto-Registration 為主；只用 Configuration as Code 是兩頭落空。
4. Pure DI 強型別、好學、編譯器抓錯最快；容器擅長 dispose、偵測 Captive Dependency、泛型 Decorator。小專案用 Pure DI，維護痛了再換 Auto-Registration。
5. 不要自己寫容器；用容器要算進學習成本、bus factor 和被放棄的風險。

---

## 第 13 章：The Autofac DI Container（Autofac 容器）

> 對應原書 p.393–426

### 這章在幹嘛

把前三部的原則對應到 **Autofac** 的 API。第 13～15 章用同一個模板：

1. 介紹：怎麼 resolve、三種設定方式
2. 生命週期管理
3. 難搞的 API（基本型別參數、static factory）
4. 多個元件：在多個候選中挑選、序列、Decorator、Composite

範例改用餐點領域（`IIngredient`、`ICourse`、`IMeal`、`SauceBéarnaise`、`Steak`…）。下面只記 Autofac 跟別人不一樣的地方。

| 問題 | 答案（Table 13.1） |
|---|---|
| 去哪拿 | NuGet 套件 `Autofac` |
| 平台 | .NET 4.5、.NET Standard 1.1（.NET Core、Mono、Xamarin、UWP…） |
| 費用／授權 | 免費、開源、MIT |
| 求助 | 有商業支援；Stack Overflow、官方論壇 |
| 本章版本 | 4.9.0-beta1 |

---

### 重點 1：先 `ContainerBuilder` 再 `Container`——設定和使用嚴格分開

```csharp
var builder = new ContainerBuilder();
builder.RegisterType<SauceBéarnaise>();
IContainer container = builder.Build();

ILifetimeScope scope = container.BeginLifetimeScope();
SauceBéarnaise sauce = scope.Resolve<SauceBéarnaise>();
```

節奏是「註冊一次、resolve 多次」（Figure 13.1）。`ContainerBuilder` 不能 resolve；`Build()` 之後才能用。

> **原文引用（p.396）**
> "With Autofac, resolving from the root container directly is a bad practice. This can easily lead to memory leaks or concurrency bugs. Instead, you should always resolve from a lifetime scope."

沒註冊就 resolve 會丟 `ComponentNotRegisteredException`，訊息會建議用 `IsRegistered()` 或 `ResolveOptional()`。

---

### 重點 2：從具體型別開始，再 `.As<抽象>()`——而且沒有型別檢查

```csharp
builder.RegisterType<SauceBéarnaise>().As<IIngredient>();
IIngredient sauce = scope.Resolve<IIngredient>();
```

- **方向跟大部分容器相反**：Autofac 從具體型別出發對應到抽象；其他容器從抽象出發對應到具體型別
- **警告**：`RegisterType` 和 `As` 之間**沒有泛型型別限制**（Simple Injector、MS.DI 有），對應到不相容的型別也編得過，要到 `Build()` 時才丟例外
- 弱型別版本 `scope.Resolve(controllerType)` 給 `IControllerActivator` 這種只拿得到 `Type` 的場合用

---

### 重點 3：Configuration as Code，以及 Torn Lifestyle 陷阱

```csharp
builder.RegisterType<SauceBéarnaise>().As<SauceBéarnaise, IIngredient>();   // 同時當兩種服務
// 或
builder.RegisterType<SauceBéarnaise>().As<SauceBéarnaise>().As<IIngredient>();
```

**Torn Lifestyle（撕裂的生命週期）**：以下兩種寫法**不一樣**：

```csharp
// A：一次註冊、兩種服務 → 共用一個 Singleton
builder.RegisterType<SauceBéarnaise>().As<SauceBéarnaise>().As<IIngredient>().SingleInstance();

// B：兩次註冊 → 各自一個快取 → 兩個「Singleton」
builder.RegisterType<SauceBéarnaise>().SingleInstance();
builder.RegisterType<SauceBéarnaise>().As<IIngredient>().SingleInstance();
```

> **原文引用（p.400）**
> "Supplying multiple service types to the As method isn't the same as making multiple RegisterType calls. Each call will get its own cache, which can cause the lifetime to be torn when the chosen Lifestyle is other than Transient."

同一個抽象註冊多次時，**最後一個贏**，但前面的不會被忘記（重點 8）。

---

### 重點 4：Auto-Registration——`RegisterAssemblyTypes` 是 `RegisterType` 的複數版

```csharp
Assembly ingredientsAssembly = typeof(Steak).Assembly;
builder.RegisterAssemblyTypes(ingredientsAssembly).As<IIngredient>();

// 加過濾
builder.RegisterAssemblyTypes(ingredientsAssembly)
    .Where(type => type.Name.StartsWith("Sauce"))
    .As<IIngredient>();

// 對應到 base class
builder.RegisterAssemblyTypes(policiesAssembly)
    .Where(type => type.Name.EndsWith("Policy"))
    .As(type => type.BaseType);
```

回傳的介面跟 `RegisterType` 一樣，**不用學新 API**。也有 `Except` 做反向過濾。

**泛型抽象**：

```csharp
builder.RegisterAssemblyTypes(assembly).AsClosedTypesOf(typeof(ICommandService<>));
```

`RegisterAssemblyTypes` 吃一個 assembly 陣列，所以可以掃描一個資料夾的所有 assembly，實作不重編就能加的外掛（另一種 late binding）。

---

### 重點 5：設定檔——Autofac 是本書唯一內建支援的

需要另外裝 `Autofac.Configuration`：

```csharp
var configuration = new ConfigurationBuilder().AddJsonFile("autofac.json").Build();
builder.RegisterModule(new ConfigurationModule(configuration));
```

```json
{
  "defaultAssembly": "Ploeh.Samples.MenuModel",
  "components": [{
    "services": [{ "type": "Ploeh.Samples.MenuModel.IIngredient" }],
    "type": "Ploeh.Samples.MenuModel.Steak",
    "instance-scope": "single-instance"
  }]
}
```

技巧：因為**最後的設定贏**，可以最後才讀設定檔，用它覆蓋寫死在程式裡的設定。

---

### 重點 6：生命週期——Autofac 叫 instance scope（Table 13.2）

| Autofac 名稱 | Pattern 名稱 | 說明 |
|---|---|---|
| Per-dependency | Transient | **預設**。實例會被容器追蹤 |
| Single instance | Singleton | 容器 dispose 時才 dispose |
| Per-lifetime scope | Scoped | 跟 lifetime scope 綁在一起 |

```csharp
builder.RegisterType<SauceBéarnaise>().As<IIngredient>().SingleInstance();
builder.RegisterType<SauceBéarnaise>().InstancePerDependency();      // 明確寫 Transient
builder.RegisterType<SauceBéarnaise>().As<IIngredient>().InstancePerLifetimeScope();
builder.RegisterAssemblyTypes(asm).As<IIngredient>().SingleInstance();   // 慣例也一樣
```

作者偏好把 `As` 緊接在 `RegisterType` 後面（它們一起構成對應），lifestyle 寫在後面當修飾。

---

### 重點 7：lifetime scope——Autofac 沒有 `Release`，用丟棄式的子容器

> **原文引用（p.406）**
> "Autofac has no explicit Release method but instead uses a concept called lifetime scopes. A lifetime scope can be regarded as a throw-away copy of the container."

```csharp
using (var scope = container.BeginLifetimeScope())
{
    IMeal meal = scope.Resolve<IMeal>();
    meal.Consume();
}   // scope dispose → 釋放這個 scope 建的所有元件
```

- scope 繼承父容器的 Singleton，自己也當「區域 Singleton」的容器；不同 scope 拿到不同的 scoped 實例（Figure 13.3）
- 依賴總是在元件自己的 scope 或更上層解析：Singleton 需要的 Transient 會從**根容器**來，避免 scope 結束時被 dispose 而讓 Singleton 壞掉

> **原文引用（p.408）**
> "Autofac tracks most components — even disposable Transients — so it's important to resolve all components from a lifetime scope and dispose of the scope after use. Resolving scoped instances from the root container causes the same instance to always be returned. This'll cause concurrency bugs when such a scoped component isn't thread-safe. When the root container is used to resolve a disposable Transient, even though new instances are created on each call to Resolve, instances are kept alive as well, in order to allow them to be disposed of when the container is disposed of. Because the root container won't be disposed of until the application stops, this causes memory leaks."

Singleton 要到 `container.Dispose()` 才釋放；通常程式結束時才做，反正 OS 會回收記憶體。

---

### 重點 8：難搞的 API——基本型別參數

```csharp
public ChiliConCarne(Spiciness spiciness)          // Spiciness 是 enum
public enum Spiciness { Mild, Medium, Hot }
```

（作者附註：經驗法則上 enum 是 code smell，該重構成多型 class，但這裡拿來當例子剛好。）

| 做法 | 程式碼 | 評價 |
|---|---|---|
| 註冊值本身 | `builder.Register<Spiciness>(c => Spiciness.Medium);` | 所有消費者共用同一個值 |
| 指定參數名 | `.WithParameter("spiciness", Spiciness.Hot)` | 次佳選擇 |
| **Parameter Object** | 包成 `Flavoring` 再 `RegisterInstance` | **首選** |

```csharp
public class Flavoring
{
    public readonly Spiciness Spiciness;
    public readonly bool ExtraSalty;
    ...
}
builder.RegisterInstance<Flavoring>(new Flavoring(Spiciness.Medium, extraSalty: true));
builder.RegisterType<ChiliConCarne>().As<ICourse>();     // Auto-Wiring 照用
```

（Mark 附註：從廚藝角度他對「特別鹹」這個概念不寒而慄。）

> **原文引用（p.411）**
> "Avoid injecting Parameter Objects that function as application-wide configuration objects. Instead, prefer narrow, focused, Parameter Objects that only contain the values a particular consumer requires. ... In general terms, injecting application-wide configuration objects is an Interface Segregation Principle violation."

Parameter Object 在功能和技術兩個層面都消除歧義，名字還能更好地描述意圖；但要改建構子，不一定可行。

---

### 重點 9：用程式碼區塊註冊——處理 static factory

```csharp
builder.Register<ICourse>(c => new ChiliConCarne(Spiciness.Hot));    // 型別安全，但關掉 Auto-Wiring

// 建構子是 internal，只能透過 static factory 建立
builder.Register<IMeal>(c => JunkFoodFactory.Create("chicken meal"));
```

自己寫建立程式碼了，還有什麼好處？
- 仍然把 `IMeal` 對應到 `JunkFood`，消費者保持鬆耦合
- 仍然可以設定 lifestyle（改成 Singleton 就只呼叫一次）

---

### 重點 10：多個實作——名稱、key、`IEnumerable<T>`

```csharp
builder.RegisterType<Steak>().As<IIngredient>();
builder.RegisterType<SauceBéarnaise>().As<IIngredient>();   // 最後註冊的是預設
// 不想蓋掉前面的預設：.PreserveExistingDefaults()

IEnumerable<IIngredient> all = scope.Resolve<IEnumerable<IIngredient>>();   // 要全部
```

Autofac 沒有專門的「resolve all」方法，而是用 **relationship type**：容器看得懂的型別（`IEnumerable<T>`、陣列）。

```csharp
builder.RegisterType<Steak>().Named<IIngredient>("meat");
IIngredient meat = scope.ResolveNamed<IIngredient>("meat");

object meatKey = new object();
builder.RegisterType<Steak>().Keyed<IIngredient>(meatKey);          // key 可以是任何物件
IIngredient meat2 = scope.ResolveKeyed<IIngredient>(meatKey);
```

具名元件**不算**預設元件。作者說：如果你在 Composition Root 裡用名稱或 key 呼叫 `Resolve`，想想能不能改得比較不歧義。

（附註：很多抽象都只有一個實作是 design smell，見 Reused Abstraction Principle。）

---

### 重點 11：三個同型別參數——`WithParameter` 很囉嗦，該改設計

```csharp
public ThreeCourseMeal(ICourse entrée, ICourse mainCourse, ICourse dessert)

builder.RegisterType<Rillettes>().Named<ICourse>("entrée");
builder.RegisterType<CordonBleu>().Named<ICourse>("mainCourse");
builder.RegisterType<MousseAuChocolat>().Named<ICourse>("dessert");

builder.RegisterType<ThreeCourseMeal>().As<IMeal>()
    .WithParameter((p, c) => p.Name == "entrée",     (p, c) => c.ResolveNamed<ICourse>("entrée"))
    .WithParameter((p, c) => p.Name == "mainCourse", (p, c) => c.ResolveNamed<ICourse>("mainCourse"))
    .WithParameter((p, c) => p.Name == "dessert",    (p, c) => c.ResolveNamed<ICourse>("dessert"));
```

可以寫 `Named(...)`、`InjectWith<T>(...)` 兩個 helper 讓它讀起來像詩；或用慣例一行解決（「元件名＝參數名」）：

```csharp
.WithParameter((p, c) => true, (p, c) => c.ResolveNamed(p.Name, p.ParameterType));
```

**警告**：用參數名稱對應不是重構安全的——改參數名可能打壞設定。

> **原文引用（p.417）**
> "If configuring a certain part of your API is difficult with Autofac, take a step back and reevaluate your design against the patterns and principles presented in this book. More often than not, configuration difficulties are caused by an application design that doesn't follow these patterns or violates these principles."

**更好的設計**：`public Meal(IEnumerable<ICourse> courses)`——只有一個依賴，沒有歧義；還能表示一道菜到 12 道菜的各種餐。Autofac 原生懂序列，註冊所有 `ICourse` 再註冊 `Meal` 就自動注入全部。只想挑一部分時，才用 `WithParameter` 回傳 `new[] { c.ResolveNamed<ICourse>("entrée"), ... }`。

---

### 重點 12：Decorator——`RegisterDecorator`／`RegisterGenericDecorator`

```csharp
builder.RegisterType<VealCutlet>().As<IIngredient>();
builder.RegisterDecorator<HamCheeseGarlic, IIngredient>();
builder.RegisterDecorator<Breading, IIngredient>();
// 等同 new Breading(new HamCheeseGarlic(new VealCutlet()))  ← 依註冊順序由內往外
```

第 10 章的三層泛型 Decorator：

```csharp
builder.RegisterAssemblyTypes(assembly).AsClosedTypesOf(typeof(ICommandService<>));
builder.RegisterGenericDecorator(typeof(AuditingCommandServiceDecorator<>),    typeof(ICommandService<>));
builder.RegisterGenericDecorator(typeof(TransactionCommandServiceDecorator<>), typeof(ICommandService<>));
builder.RegisterGenericDecorator(typeof(SecureCommandServiceDecorator<>),      typeof(ICommandService<>));
```

---

### 重點 13：Composite——Autofac 的弱項

非泛型 Composite：把真正的實作註冊成具名序列，Composite 用程式碼區塊註冊成預設：

```csharp
builder.RegisterAssemblyTypes(assembly).Named<INotificationService>("service");
builder.Register(c => new CompositeNotificationService(
        c.ResolveNamed<IEnumerable<INotificationService>>("service")))
    .As<INotificationService>();
```

**陷阱**：如果 Composite 跟實作在同一個 assembly，`RegisterAssemblyTypes` 會把 Composite 自己也掃進序列：

```text
Circular component dependency detected: CompositeNotificationService ->
INotificationService[] -> CompositeNotificationService -> ...
```

（Autofac 偵測到循環並給清楚的訊息。）解法：把 Composite 搬到別的 assembly（例如 Composition Root 那個），或用 `.Except<CompositeNotificationService>()` 排除；Decorator 也要排除，可寫個 `IsDecoratorFor<T>` helper（「實作 T 而且建構子也要一個 T」）。

泛型 Composite（`CompositeEventHandler<TEvent>`）要用 `RegisterGeneric`＋`WithParameter`＋`KeyedService`，相當複雜；好處是 `RegisterAssemblyTypes` 只挑非泛型型別，泛型的 Composite／Decorator 不用排除。

> **原文引用（p.425）**
> "Although the registration of Decorators is simple, this unfortunately doesn't hold for Composites. Autofac hasn't been designed — yet — with the Composite design pattern in mind."

---

### 本章 Summary（作者原話）

> **原文引用（p.426）**
> - "The Autofac DI Container offers a fairly comprehensive API and addresses many of the trickier situations you typically encounter when you use DI Containers."
> - "An important overall theme for Autofac seems to be one of explicitness. It doesn't attempt to guess what you mean, but rather offers an easy-to-use API that provides you with options to explicitly enable features."
> - "Autofac enforces stricter separation of concerns between configuring and consuming a container. You configure components using a ContainerBuilder instance, but a ContainerBuilder can't resolve components. When you're done configuring a ContainerBuilder, you use it to build an IContainer that you can use to resolve components."
> - "With Autofac, resolving from the root container directly is a bad practice. This can easily lead to memory leaks or concurrency bugs. Instead, you should always resolve from a lifetime scope."
> - "Autofac supports the standard Lifestyles: Transient, Singleton, and Scoped."
> - "Autofac allows working with ambiguous constructors and types by providing an API that allows supplying code blocks. This allows any code that creates a service to be executed."

### 我的整理：本章帶走的四句話

1. Autofac：`ContainerBuilder` 註冊 → `Build()` → **永遠從 `BeginLifetimeScope()` 解析**，從根容器解析會記憶體洩漏或並行 bug。
2. 從具體型別出發 `.As<抽象>()`，沒有編譯期型別檢查；一個元件要當多種服務時用一次 `As<A, B>()`，分兩次註冊會撕裂 lifestyle。
3. 基本型別參數首選包成窄的 Parameter Object；用參數名的 `WithParameter` 不是重構安全的。
4. Decorator 很好註冊（依註冊順序由內往外）；Composite 很麻煩，小心 `RegisterAssemblyTypes` 把 Composite 自己掃進去造成循環。

---

## 第 14 章：The Simple Injector DI Container（Simple Injector 容器）

> 對應原書 p.427–465

### 這章在幹嘛

Simple Injector 是 Steven 在 2010 年寫的（Autofac 是 Nicholas Blumhardt 2007 年寫的）。章節結構跟第 13 章一樣。它的 API 跟多數容器差很多，設計上刻意消除常見的 bug 來源。作者列出四個主要差異：

1. **scope 是 ambient（環境式的）**：永遠可以直接從容器 resolve，不會記憶體洩漏或並行 bug
2. **序列用另一套 API 註冊**：避免重複註冊互相覆蓋
3. **不能直接註冊基本型別**：避免歧義
4. **可以驗證 object graph**：抓出 Captive Dependency 等常見設定錯誤

| 問題 | 答案（Table 14.1） |
|---|---|
| 去哪拿 | NuGet 套件 `SimpleInjector` |
| 平台 | .NET 4.0、.NET Standard 1.0 |
| 費用／授權 | 免費、開源、MIT |
| 求助 | 沒有保證的支援；官方論壇、Stack Overflow |
| 本章版本 | 4.4.3 |

---

### 重點 1：註冊和 resolve 用同一個 `Container`，但第一次 resolve 後就鎖住

```csharp
var container = new Container();
SauceBéarnaise sauce = container.GetInstance<SauceBéarnaise>();   // 不用註冊就能 resolve 具體型別
```

- Autofac 用 `ContainerBuilder` → `IContainer` 分兩步；Simple Injector 同一個 `Container`，但**第一次 resolve 之後就不准再註冊**
- 具體型別只要整棵樹的葉子都有無參數建構子，不註冊也能 Auto-Wire（例：`Mayonnaise(EggYolk, SunflowerOil)`）
- `GetInstance<T>` 等於 Autofac 的 `Resolve<T>`

```csharp
container.Register<IIngredient, SauceBéarnaise>();     // 有泛型型別限制，不相容會編譯失敗
IIngredient sauce = container.GetInstance<IIngredient>();
```

> **原文引用（p.433）**
> "Even though Simple Injector allows resolving concrete unregistered types, make sure you register your topmost types explicitly. This allows Simple Injector to verify your complete object graph, including these root types."

---

### 重點 2：同一個抽象註冊兩次會直接丟例外

```csharp
container.Register<IIngredient, SauceBéarnaise>();
container.Register<IIngredient, Steak>();              // 例外！
```

```text
Type IIngredient has already been registered. If your intention is to resolve a collection of
IIngredient implementations, use the Collection.Register overloads.
```

> **原文引用（p.434）**
> "In contrast to most other DI Containers, Simple Injector doesn't allow stacking up registrations to build up a sequence of types, as the previous code snippet shows. Its API explicitly separates the registration of sequences from single Abstraction mappings."

序列要用專門的 API（Simple Injector 叫 **collection**）：

```csharp
container.Collection.Register<IIngredient>(typeof(SauceBéarnaise), typeof(Steak));
// 或
container.Collection.Append<IIngredient, SauceBéarnaise>();
container.Collection.Append<IIngredient, Steak>();
```

**對比 Autofac 的 Torn Lifestyle**：在 Simple Injector 裡同時註冊 `Register<IIngredient, SauceBéarnaise>()` 和 `Register<SauceBéarnaise>()` 沒問題，它內部會自動去重，lifestyle 不會被撕裂。

---

### 重點 3：Auto-Registration——API 只專注在序列和泛型

```csharp
container.Collection.Register<IIngredient>(typeof(Steak).Assembly);    // 一個 assembly 的所有實作

var types = container.GetTypesToRegister<IIngredient>(assembly)        // 只找不註冊，可以再過濾
    .Where(type => type.Name.StartsWith("Sauce"));
container.Collection.Register<IIngredient>(types);
```

其他慣例（例如「名字結尾是 Policy、註冊到 base class」）直接用 .NET 的 reflection＋LINQ 寫迴圈，幾乎不用 Simple Injector 的 API。

**泛型抽象**：

```csharp
container.Register(typeof(ICommandService<>), assembly);
```

用 `Register` 而不是 `Collection.Register`，因為每個 command **恰好只有一個**實作。Simple Injector 會自動跳過 open generic 實作、Decorator、Composite（它們通常需要特殊註冊）。

---

### 重點 4：沒有內建設定檔支援，但自己加很簡單

```json
{ "registrations": [{
    "service": "Ploeh.Samples.MenuModel.IIngredient, Ploeh.Samples.MenuModel",
    "implementation": "Ploeh.Samples.MenuModel.Steak, Ploeh.Samples.MenuModel" }] }
```

```csharp
var config = new ConfigurationBuilder().AddJsonFile("simpleinjector.json").Build();
foreach (var reg in config.GetSection("registrations").GetChildren())
    container.Register(Type.GetType(reg["service"]), Type.GetType(reg["implementation"]));
```

---

### 重點 5：Lifestyle（Table 14.2）

| Simple Injector 名稱 | Pattern 名稱 | 說明 |
|---|---|---|
| Transient | Transient | **預設**。**不被容器追蹤，所以永遠不會被 dispose**；把 disposable 元件註冊成 Transient，診斷功能會警告 |
| Singleton | Singleton | 容器 dispose 時 dispose |
| Scoped | Scoped | 由 `ScopedLifestyle` 基底類別定義的樣板，有多種實作；.NET Core 最常用 `AsyncScopedLifestyle`。scope 結束時 dispose |

```csharp
container.Register<IIngredient, SauceBéarnaise>(Lifestyle.Singleton);

// 慣例註冊時統一設 lifestyle：建 Registration 物件
container.Collection.Register<IIngredient>(
    from type in container.GetTypesToRegister<IIngredient>(assembly)
    select Lifestyle.Singleton.CreateRegistration(type, container));
```

（預設 lifestyle 可以用 `Container.Options.DefaultLifestyle` 改。）

**對比 Autofac**：Autofac 會追蹤 disposable Transient；Simple Injector 完全不追蹤 Transient。

---

### 重點 6：Scoped 要先設定預設的 ScopedLifestyle

直接用 `Lifestyle.Scoped` 會丟例外，要求你先設定：

```csharp
var container = new Container();
container.Options.DefaultScopedLifestyle = new AsyncScopedLifestyle();   // ASP.NET Core 用這個
container.Register<IIngredient, SauceBéarnaise>(Lifestyle.Scoped);
```

釋放：

```csharp
using (AsyncScopedLifestyle.BeginScope(container))
{
    IMeal meal = container.GetInstance<IMeal>();    // 從容器 resolve，不是從 scope！
    meal.Consume();
}
```

> **原文引用（p.441）**
> "This example shows how IMeal is resolved from the Container instance, instead of being resolved from a Scope instance. This isn't a typo — the container automatically 'knows' in which active scope it's operating."

Simple Injector 有很多整合套件，會自動替每個 web 請求包一個 scope。

---

### 重點 7：ambient scope——容器自己知道現在在哪個 scope

```csharp
Scope scope1 = Lifestyle.Scoped.GetCurrentScope(container);          // null
using (Scope scope2 = AsyncScopedLifestyle.BeginScope(container))
{
    Scope scope3 = Lifestyle.Scoped.GetCurrentScope(container);      // == scope2
}
Scope scope4 = Lifestyle.Scoped.GetCurrentScope(container);          // null
```

行為像 .NET 的 `TransactionScope`。`AsyncScopedLifestyle` 把 scope 存在 `AsyncLocal<T>` 裡，所以 `await` 之後換了 thread，scope 也會跟著流過去。

> **原文引用（p.444）**
> "Ambient scopes shouldn't be confused with Ambient Context. Ambient Context supplies application code outside the Composition Root with global access to a Volatile Dependency or its behavior. Because a DI Container's scopes will only be used inside the Composition Root, ambient scopes don't expose the same problems as Ambient Context does."

> **原文引用（p.442）**
> "Because of Simple Injector's ambient scopes, you can always resolve directly from the Container without being afraid of the accidental memory leaks that would occur were you to do this with a DI Container, such as Autofac. In case you resolve a Scoped Dependency from the container while there's no active scope, Simple Injector throws a descriptive exception."

scope 實例永遠不會被快取在根容器裡——這是用其他容器要小心的地方。

---

### 重點 8：診斷——`Verify()` 抓 Captive Dependency

能建出 object graph 不代表設定正確（Captive Dependency 就是例子）；大部分容器錯誤都跟生命週期設定有關。所以 Simple Injector 的「設定」階段分兩步：**註冊 → 驗證**，然後才 resolve（Figure 14.4）。

```csharp
var container = new Container();
container.Register<EggYolk>(Lifestyle.Transient);       // 蛋黃很快壞
container.Register<Mayonnaise>(Lifestyle.Singleton);    // 不小心設成 Singleton
container.Register<SunflowerOil>(Lifestyle.Singleton);
container.Verify();                                     // 丟例外
```

```text
The configuration is invalid. The following diagnostic warnings were reported:
-[Lifestyle Mismatch] Mayonnaise (Singleton) depends on EggYolk (Transient).
```

- `Register` 時只做基本檢查（不是抽象、有 public 建構子…），因為註冊順序任意（`SunflowerOil` 可以比 `Mayonnaise` 晚註冊）；全部註冊完才能驗證
- Simple Injector 把 Captive Dependency 叫 **Lifestyle Mismatch**

**對比 Autofac**：Simple Injector **不准**把 Transient 注入 Singleton。Autofac 認為 Transient 本來就該跟消費者活一樣久（所以叫 `InstancePerDependency`），只把「Scoped 注入 Singleton」當 Captive Dependency。Simple Injector 的座右銘是「better safe than sorry」：Transient 通常預期短命，注入 Singleton 就變成跟程式一樣長命。

**真的要的話可以逐一壓掉警告**：

```csharp
Registration reg = Lifestyle.Transient.CreateRegistration<EggYolk>(container);
reg.SuppressDiagnosticWarning(DiagnosticType.LifestyleMismatch,
    justification: "I like to eat rotten eggs.");      // 必填的理由，程式不用它，只是逼你寫文件
container.AddRegistration(typeof(EggYolk), reg);
```

---

### 重點 9：不准註冊基本型別

> **原文引用（p.448）**
> "Where one component might require a database connection string, another might require a file path. The two are conceptually different, but because Auto-Wiring works by selecting Dependencies based on their type, they become ambiguous. For that reason, Simple Injector blocks the registration of primitive Dependencies."

```csharp
container.Register<ICourse, ChiliConCarne>();     // 例外：Spiciness 是 value type，不能用於建構子注入
container.Register<ICourse>(() => new ChiliConCarne(Spiciness.Medium));   // 用 delegate
```

delegate 的缺點：
- 建構子一改就要改註冊（例如加了 `IIngredient` 要寫 `container.GetInstance<IIngredient>()` 手動塞）
- **delegate 讓容器看不見**這個依賴，無法驗證 `ChiliConCarne` 和 `IIngredient` 的關係

所以首選一樣是 **Parameter Object**（`Flavoring`＋`RegisterInstance`），delegate 次之。（也可以換掉預設的 `IDependencyInjectionBehavior` 來允許基本型別，書中不談。）

static factory 也用 delegate：`container.Register<IMeal>(() => JunkFoodFactory.Create("chicken meal"));`。Simple Injector 預設也能建立 internal class（只要建構子是 public）。

**多個建構子**：多數容器會用啟發式規則挑一個；Simple Injector 預設直接丟例外說型別定義有歧義（可改，但作者建議不要讓元件有多個建構子）。

---

### 重點 10：多個實作——條件式註冊取代具名註冊

取得全部：`container.GetAllInstances<IIngredient>()` 或 `GetInstance<IEnumerable<IIngredient>>()`（`IList<T>`、`ICollection<T>`、`IReadOnlyList<T>`、`IReadOnlyCollection<T>` 也行）。

三個同型別參數的 `ThreeCourseMeal`：其他容器用具名／keyed 註冊、改**消費者**的註冊；Simple Injector 改**依賴**的註冊：

```csharp
container.Register<IMeal, ThreeCourseMeal>();       // 消費者照常 Auto-Wiring
container.RegisterConditional<ICourse, Rillettes>(c => c.Consumer.Target.Name == "entrée");
container.RegisterConditional<ICourse, CordonBleu>(c => c.Consumer.Target.Name == "mainCourse");
container.RegisterConditional<ICourse, MousseAuChocolat>(c => c.Consumer.Target.Name == "dessert");
```

- 找不到適用的註冊會丟例外；**有多個適用的註冊也會丟例外**——它從不猜你的意圖
- 所以它會呼叫所有適用條件註冊的 predicate 來找重疊；看似沒效率，但只在第一次 resolve 時做
- 同樣警告：用參數名稱不是重構安全的
- 好處：不用退回 delegate，容器能看見整個 object graph、能診斷

> **原文引用（p.454）**
> "The use of code blocks blinds a container, which might cause configuration mistakes to stay undetected for too long."

更好的設計同樣是 `Meal(IEnumerable<ICourse> courses)`：`Register<IMeal, Meal>()`＋`Collection.Register<ICourse>(assembly)` 就自動注入全部。

只要一部分：

```csharp
IEnumerable<ICourse> subset = container.Collection.Create<ICourse>(
    typeof(Rillettes), typeof(CordonBleu), typeof(MousseAuChocolat));
container.RegisterInstance<IMeal>(new Meal(subset));
```

`Collection.Create` 當下**不會**建立任何 course（它是 stream，見重點 13），所以可以安全地注入 Singleton 的 `Meal`。

---

### 重點 11：Decorator——`RegisterDecorator`

```csharp
c.Register<IIngredient, VealCutlet>();
c.RegisterDecorator<IIngredient, HamCheeseGarlic>();
c.RegisterDecorator<IIngredient, Breading>();
// = new Breading(new HamCheeseGarlic(new VealCutlet()))  依註冊順序由內往外
```

泛型（第 12 章已出現過）：

```csharp
container.Register(typeof(ICommandService<>), assembly);
container.RegisterDecorator(typeof(ICommandService<>), typeof(AuditingCommandServiceDecorator<>));
container.RegisterDecorator(typeof(ICommandService<>), typeof(TransactionCommandServiceDecorator<>));
container.RegisterDecorator(typeof(ICommandService<>), typeof(SecureCommandServiceDecorator<>));
```

還有條件式套用 Decorator 等進階功能（書中不談）。注意：泛型的參數順序是 `<抽象, Decorator>`，跟 Autofac 的 `<Decorator, 抽象>` 相反。

---

### 重點 12：Composite——因為序列和單一註冊分開，所以超簡單

```csharp
container.Collection.Register<INotificationService>(assembly);                  // 實作們當序列
container.Register<INotificationService, CompositeNotificationService>();       // Composite 當單一註冊
```

**對比 Autofac**：Autofac 會把 Composite 自己掃進序列造成循環；Simple Injector 的 `Collection.Register` **自動過濾掉 Composite 和 Decorator**。

泛型 Composite：

```csharp
container.Collection.Register(typeof(IEventHandler<>), assembly);   // 一個事件可能有多個 handler
container.Register(typeof(IEventHandler<>), typeof(CompositeEventHandler<>));
```

如果不小心用 `Register` 而不是 `Collection.Register`，錯誤訊息會直接提示：

```text
In the supplied list of types or assemblies, there are 3 types that represent the same
closed-generic type IEventHandler<OrderApproved>. Did you mean to register the types as a
collection using the Collection.Register method instead? ...
```

---

### 重點 13：序列是 stream（串流）

> **原文引用（p.462）**
> "When resolving or injecting an IEnumerable<T>, Simple Injector doesn't prepopulate the sequence with all ingredients right away. Instead, IEnumerable<T> behaves like a stream."

作者說就他們所知，**Simple Injector 是唯一用串流處理抽象序列的容器**。每次走訪都回頭請容器依 lifestyle 解析：

```csharp
var stream = container.GetAllInstances<IIngredient>();
object.ReferenceEquals(stream.First(), stream.First());   // Transient → false

c.Collection.Append<IIngredient, Steak>(Lifestyle.Singleton);
object.ReferenceEquals(s.Last(), s.Last());               // Singleton → true
```

（小細節：回傳的物件實作 `IList<T>`——設定完成後元素個數就固定了——所以 `Last()` 會走 `IList<T>` 的最佳化，只建最後一個。）

**好處 1：注入幾乎免費**。stream 本身是 Singleton，注入時不建任何元素：

```csharp
public class CompositeLogger : ILogger
{
    private readonly IEnumerable<ILogger> loggers;
    public void Log(LogEntry entry)
    {
        foreach (ILogger logger in this.loggers)
        {
            try { logger.Log(entry); break; }      // 第一個成功就停
            catch { }
        }
    }
}
container.Collection.Register<ILogger>(assembly);
container.Register<ILogger, CompositeLogger>(Lifestyle.Singleton);
```

通常只會建出第一個 logger。

**好處 2：元素永遠不會變成 Captive Dependency**。只要只存 `IEnumerable<T>` 的參考，Singleton 的 Composite 依賴它也安全（stream 本身也是 Singleton），即使它產生的元素不是。

---

### 本章 Summary（作者原話）

> **原文引用（p.464–465）**
> - "Simple Injector is a modern DI Container that offers a fairly comprehensive feature set, but its API is quite different from most DI Containers. The following are a few of its characteristic attributes: – Scopes are ambient. – Sequences are registered using Collection.Register instead of appending new registrations of the same Abstraction. – Sequences behave as streams. – The container can be diagnosed to find common configuration pitfalls."
> - "An important overall theme for Simple Injector is one of strictness. It doesn't attempt to guess what you mean and tries to prevent and detect configuration errors through its API and diagnostic facility."
> - "Simple Injector enforces a strict separation of registration and resolution. Although you use the same Container instance for both register and resolve, the Container is locked after first use."
> - "Because of Simple Injector's ambient scopes, resolving from the root container directly is good practice and encouraged: it doesn't lead to memory leaks or concurrency bugs."
> - "Simple Injector supports the standard Lifestyles: Transient, Singleton, and Scoped."
> - "Simple Injector has excellent support for registration of sequences, Decorators, Composites, and generics."

### 我的整理：本章帶走的四句話

1. Simple Injector 的主題是「嚴格」：重複註冊、多個建構子、基本型別參數、多個條件註冊都適用時，一律丟例外，不猜。
2. scope 是 ambient 的（存在 `AsyncLocal`），永遠直接從 `Container` resolve；Transient 不被追蹤也不會被 dispose。
3. 註冊完呼叫 `Verify()`：會抓 Lifestyle Mismatch（Captive Dependency），連 Transient 注入 Singleton 都算。
4. 序列用 `Collection.Register`，自動排除 Composite／Decorator，所以 Composite 很好註冊；序列是 stream，注入免費且元素不會被俘虜。

---

## 第 15 章：The Microsoft.Extensions.DependencyInjection DI Container（MS.DI 容器）

> 對應原書 p.466–498

### 這章在幹嘛

ASP.NET Core 內建的容器，書中簡稱 **MS.DI**。微軟做它是為了讓框架和第三方元件開發者好管理依賴，定位是「**所有容器都能符合的最低公約數**」功能集。

作者開宗明義：

> **原文引用（p.467）**
> "During the course of this chapter, however, you'll find that MS.DI is so limited in functionality that we deem it unsuited for development of any reasonably sized application that practices loose coupling and follows the principles and patterns described in this book."

那為什麼還寫一章？因為它乍看跟其他容器很像，又內建在 .NET Core 裡，不花時間用一下看不出差別。這章的目的就是**揭露它的限制**，讓你做有根據的決定。已經決定用別的容器的人可以跳過。

| 問題 | 答案（Table 15.1） |
|---|---|
| 去哪拿 | 新建 ASP.NET Core 專案自動包含；或 NuGet 套件 `Microsoft.Extensions.DependencyInjection` |
| 平台 | .NET Standard 2.0 |
| 費用／授權 | 免費、開源、Apache 2.0 |
| 求助 | 微軟官方產品，有保證的商業支援；也可問 Stack Overflow |
| 本章版本 | 2.1.0 |

---

### 重點 1：`ServiceCollection` → `ServiceProvider`，永遠從 scope resolve

```csharp
var services = new ServiceCollection();
services.AddTransient<SauceBéarnaise>();
ServiceProvider container = services.BuildServiceProvider(validateScopes: true);

IServiceScope scope = container.CreateScope();
SauceBéarnaise sauce = scope.ServiceProvider.GetRequiredService<SauceBéarnaise>();
```

- `ServiceCollection` 等於 Autofac 的 `ContainerBuilder`；兩步驟是明確的（跟 Autofac 一樣，跟 Simple Injector 不同）
- 必須先註冊才能 resolve；沒註冊會丟 `InvalidOperationException: No service for type '...' has been registered.`
- ASP.NET Core 的 hosting 環境會幫你建好 `ServiceCollection`

> **原文引用（p.469）**
> "With MS.DI, avoid resolving from the root container. This can easily lead to memory leaks or concurrency bugs. Instead, you should always resolve from a scope"

> **原文引用（p.469）**
> "As a safety measure, always build the ServiceProvider using the BuildServiceProvider overload with the validateScopes argument set to true ... This prevents the accidental resolution of Scoped instances from the root container."

ASP.NET Core 2.0 在開發環境會自動開啟 `validateScopes`；作者建議**所有環境都開**。

`GetRequiredService` 找不到會丟例外；`GetService` 找不到回 `null`。幾乎總是該用前者。

---

### 重點 2：只支援 Configuration as Code

```csharp
services.AddTransient<IIngredient, SauceBéarnaise>();   // 有泛型型別限制
```

> **原文引用（p.471）**
> "MS.DI is designed around Configuration as Code and contains no API that supports either configuration files or Auto-Registration."

**Torn Lifestyle 陷阱**（跟 Autofac 一樣）：

```csharp
// 錯：兩次 AddSingleton → 兩個各自的快取 → 兩個實例
services.AddSingleton<SauceBéarnaise>();
services.AddSingleton<IIngredient, SauceBéarnaise>();

// 對：第二個註冊用 delegate 轉給第一個
services.AddSingleton<SauceBéarnaise>();
services.AddSingleton<IIngredient>(c => c.GetRequiredService<SauceBéarnaise>());
```

> **原文引用（p.473）**
> "Each call to one of the AddScoped and AddSingleton methods results in its own unique cache. Having multiple Add... calls can, therefore, result in multiple instances per scope or per container. To prevent this, register a delegate that resolves the concrete instance."

同一個抽象註冊多次：最後一個贏，前面的不會被忘記（跟 Autofac 一樣）。

---

### 重點 3：沒有 Auto-Registration，要自己用 LINQ＋reflection 寫

```csharp
var ingredientTypes =
    from type in typeof(Steak).Assembly.GetTypes()
    where !type.IsAbstract
    where typeof(IIngredient).IsAssignableFrom(type)
    select type;
foreach (var type in ingredientTypes)
    services.AddTransient(typeof(IIngredient), type);
```

泛型抽象就麻煩了：

```csharp
var mappings =
    from type in assembly.GetTypes()
    where !type.IsAbstract
    where !type.IsGenericType
    from i in type.GetInterfaces()
    where i.IsGenericType
    where i.GetGenericTypeDefinition() == typeof(ICommandService<>)
    select new { service = i, type };
foreach (var mapping in mappings)
    services.AddTransient(mapping.service, mapping.type);
```

**這段有個嚴重缺陷**：如果不小心有兩個 class 實作同一個 closed generic 介面，它會**靜默地**都註冊，resolve 時拿到「最後一個」——但 `Assembly.GetTypes()` 的順序是未定義的，**重新編譯甚至重啟程式都可能變**。（對比 Simple Injector 會直接丟例外。）

---

### 重點 4：Lifestyle（Table 15.2）——Transient 可以注入 Singleton

| Microsoft 名稱 | Pattern 名稱 | 說明 |
|---|---|---|
| Transient | Transient | 會被容器追蹤並 dispose |
| Singleton | Singleton | 容器 dispose 時 dispose |
| Scoped | Scoped | 同一個 `IServiceScope` 內重用；scope dispose 時 dispose |

```csharp
services.AddSingleton<IIngredient, SauceBéarnaise>();
services.AddScoped<IIngredient, SauceBéarnaise>();

using (IServiceScope scope = container.CreateScope())
{
    IMeal meal = scope.ServiceProvider.GetRequiredService<IMeal>();
    meal.Consume();
}
```

跟其他容器相比，設定 lifestyle 的選項不多。依賴總是在元件自己的 scope 或更上層解析（Singleton 需要的 Transient 來自根容器）。

> **原文引用（p.479）**
> "With MS.DI, a Transient component is a component that's expected to live as long as the consumer it's injected into. That's why MS.DI allows injecting Transients into Singletons, although injection of Scoped instances into Singletons is blocked. Although injecting a Transient into a Singleton might be exactly the desired behavior, more often it's not. You need to take extra care to check that Transients don't become accidental Captive Dependencies."

（「阻擋 Scoped 注入 Singleton」要開 scope 驗證才有。）三個容器的對比：

| 容器 | Transient 注入 Singleton | Scoped 注入 Singleton |
|---|---|---|
| Autofac | 允許 | 偵測為 Captive Dependency |
| Simple Injector | **Verify 報 Lifestyle Mismatch** | 報錯 |
| MS.DI | 允許 | 開 `validateScopes` 時阻擋 |

> **原文引用（p.479）**
> "MS.DI tracks most components — even disposable Transients. This will cause problems when you resolve from the root container instead of a scope. ... Because the root container won't be disposed of until the application stops, this causes memory leaks"

---

### 重點 5：難搞的 API

- **基本型別**：泛型 API 用型別限制擋掉 value type（數字、enum），但非泛型 API 還是可以：`services.AddSingleton(typeof(Spiciness), Spiciness.Medium);`
- **首選**：Parameter Object（`services.AddSingleton<Flavoring>(flavoring);`）
- **次選**：delegate：

```csharp
services.AddTransient<ICourse>(c => new ChiliConCarne(Spiciness.Hot));
services.AddTransient<IMeal>(c => JunkFoodFactory.Create("chicken meal"));
```

delegate 的型別是 `Func<IServiceProvider, TService>`，拿到的 `IServiceProvider` 屬於目前的 scope，所以可以在裡面 resolve 同一個 scope 的其他實例。

---

### 重點 6：三個同型別參數——只能退回 delegate 或 `ActivatorUtilities`

Autofac 有 keyed 註冊、Simple Injector 有條件註冊；**MS.DI 沒有任何內建功能**處理這種歧義：

```csharp
services.AddTransient<IMeal>(c => new ThreeCourseMeal(
    entrée:     c.GetRequiredService<Rillettes>(),
    mainCourse: c.GetRequiredService<CordonBleu>(),
    dessert:    c.GetRequiredService<CrèmeBrûlée>()));
```

如果 `ThreeCourseMeal` 還有其他依賴，想讓那些照常 Auto-Wire，就用 **`ActivatorUtilities`**：

```csharp
services.AddTransient<IMeal>(c =>
    ActivatorUtilities.CreateInstance<ThreeCourseMeal>(c, new object[]
    {
        c.GetRequiredService<Rillettes>(),
        c.GetRequiredService<CordonBleu>(),
        c.GetRequiredService<MousseAuChocolat>()
    }));
```

`CreateInstance<T>` 把傳入的參數由左到右對應到相容的建構子參數，剩下的從 `IServiceProvider` 解析。多給一個對不上的參數會丟例外。

**作者警告——這比前一個寫法更危險**：
- 前一個寫法有編譯器檢查
- `ActivatorUtilities` 版本：如果建構子的三個 `ICourse` 參數**換了順序**，照樣編得過、照樣建得出來，但菜就上錯順序了，**沒有任何工具會提醒你**
- 連 Autofac、Simple Injector 用參數名稱的版本都比較好：改了參數名至少 resolve 時會丟例外——**總比靜默失敗好**

更好的解法同樣是改設計成 `Meal(IEnumerable<ICourse>)`。MS.DI 懂序列，`AddTransient<ICourse, ...>` 幾次再 `AddTransient<IMeal, Meal>()` 就自動注入全部；取全部用 `GetServices<T>()`。只要一部分時，要把每個 course 用具體型別註冊、再用 delegate 對應到 `ICourse`（避免撕裂 lifestyle），最後在 `Meal` 的 delegate 裡手動挑——相當囉嗦（Listing 15.6）。

---

### 重點 7：Decorator——沒有內建支援，只能用 `ActivatorUtilities` 硬湊

> **原文引用（p.489）**
> "MS.DI has no built-in support for Decorators, and this is one of the areas where the limitations of MS.DI can hinder productivity."

```csharp
services.AddTransient<IIngredient>(c =>
    ActivatorUtilities.CreateInstance<Breading>(c,
        ActivatorUtilities.CreateInstance<HamCheeseGarlic>(c,
            ActivatorUtilities.CreateInstance<VealCutlet>(c))));
```

泛型 Decorator（第 10 章那三層）要自己掃描、用 `MakeGenericType` 組出 closed 型別、再一層層 `ActivatorUtilities.CreateInstance`（Listing 15.7，約 30 行）。而且這還只是開始，缺點有：

1. Decorator 的泛型參數跟抽象對不上時（例如 `CachingDecorator<TRequest, TResponse>` 實作 `IHandler<TRequest, ReadOnlyCollection<TResponse>>`），很難組出 closed 型別
2. 不可能讓 open generic 實作自動套 Decorator，除非逐一註冊每個 closed 抽象
3. 條件式套用 Decorator（例如依泛型參數）很複雜
4. 換別的 lifestyle 時，實作多個介面的 class 很難避免撕裂
5. 整條 Decorator 鏈只能用同一個 lifestyle

> **原文引用（p.492）**
> "You could try working through these limitations one-by-one and suggest improvements to listing 15.7, but you'd effectively be developing a new DI Container on top of MS.DI, which is something we discourage."

對比：Autofac 和 Simple Injector 各只要幾行。

---

### 重點 8：Composite——自己掃描、小心 StackOverflow

```csharp
Type[] types = (
    from type in assembly.GetTypes()
    where !type.IsAbstract
    where typeof(INotificationService).IsAssignableFrom(type)
    where type != typeof(CompositeNotificationService)           // 一定要排除自己
    where !IsDecoratorFor<INotificationService>(type)            // Decorator 也要排除
    select type).ToArray();                                      // 物化成陣列，避免每次 resolve 都重掃 assembly

foreach (Type type in types) services.AddTransient(type);

services.AddTransient<INotificationService>(c =>
    new CompositeNotificationService(
        types.Select(t => (INotificationService)c.GetRequiredService(t)).ToArray()));
```

如果忘了排除 Composite 自己：

```text
Exception of type 'System.StackOverflowException' was thrown.
```

> **原文引用（p.495）**
> "While writing this chapter, we found that exception messages thrown by MS.DI are often generic or confusing, which makes troubleshooting problems harder than with most of the other popular DI Containers. Most mature DI Containers have pretty clear exception messages."

StackOverflow 會讓整個行程中止、很難除錯，也沒告訴你原因；Autofac 和 Simple Injector 都會丟清楚描述循環的例外。

---

### 重點 9：泛型 Composite——只能讓 Composite 自己建 handler

作者找到最簡單的方法是：**乾脆不註冊 handler**，改由 Composite 自己建：

```csharp
public class CompositeSettings { public Type[] AllHandlerTypes { get; set; } }

public class CompositeEventHandler<TEvent> : IEventHandler<TEvent>
{
    private readonly IServiceProvider provider;        // 依賴 MS.DI 本身
    private readonly CompositeSettings settings;

    public void Handle(TEvent e)
    {
        foreach (var handler in this.GetHandlers()) handler.Handle(e);
    }

    IEnumerable<IEventHandler<TEvent>> GetHandlers() =>
        from type in this.settings.AllHandlerTypes
        where typeof(IEventHandler<TEvent>).IsAssignableFrom(type)       // 做了本該是容器做的事
        select (IEventHandler<TEvent>)ActivatorUtilities.CreateInstance(this.provider, type);
}

services.AddSingleton(new CompositeSettings { AllHandlerTypes = handlerTypes.ToArray() });
services.AddTransient(typeof(IEventHandler<>), typeof(CompositeEventHandler<>));
```

- 這個 Composite 依賴 `IServiceProvider` 和 `ActivatorUtilities`，**必須放在 Composition Root**（其他程式不該知道容器存在）
- 要對個別 handler 套 Decorator，就把 Listing 15.7 的程式碼混進 `GetHandlers`——Composite 變成負責建整個 object graph
- 其他情況可能就沒這麼幸運：序列混合泛型和非泛型實作、泛型實作有型別限制、Decorator 需要條件式套用……

> **原文引用（p.498）**
> "We do admit that this is an unpleasant solution. ... That's why, in our day-to-day development jobs, we prefer Pure DI or one of the mature DI Containers, such as Autofac and Simple Injector."

---

### 重點 10：全書最後一段話

> **原文引用（p.498）**
> "No matter which DI Container you select, or even if you prefer Pure DI, we hope that this book has conveyed one important point — DI doesn't rely on a particular technology, such as a particular DI Container. An application can, and should, be designed using the DI-friendly patterns and practices presented in this book. When you succeed in doing that, selection of a DI Container becomes of less importance. A DI Container is a tool that composes your application, but ideally, you should be able to replace one container with another without rewriting any part of your application other than the Composition Root."

---

### 本章 Summary（作者原話）

> **原文引用（p.498）**
> - "The Microsoft.Extensions.DependencyInjection (MS.DI) DI Container has a limited set of features. A comprehensive API that addresses Auto-Registration, Decorators, and Composites is missing. This makes it less suited for development of applications that are designed around the principles and patterns presented in this book."
> - "MS.DI enforces a strict separation of concerns between configuring and consuming a container. You configure components using a ServiceCollection instance, but a ServiceCollection can't resolve components. When you're done configuring a ServiceCollection, you use it to build a ServiceProvider that you can use to resolve components."
> - "With MS.DI, resolving from the root container directly is a bad practice. This will easily lead to memory leaks or concurrency bugs. Instead, you should always resolve from an IServiceScope."
> - "MS.DI supports the three standard Lifestyles: Transient, Singleton, and Scoped."

### 我的整理：本章帶走的四句話

1. MS.DI 是「最低公約數」容器：只有 Configuration as Code，沒有 Auto-Registration、沒有 Decorator 支援、沒有處理歧義的具名／條件註冊。
2. `BuildServiceProvider(validateScopes: true)` 在所有環境都要開；永遠從 `CreateScope()` 解析。
3. 自己硬湊的結果很脆弱：重複實作會靜默挑「隨機」一個、`ActivatorUtilities` 參數換順序不會報錯、循環依賴直接 StackOverflow。
4. 作者的建議：框架內部讓它用，自己的元件用 Pure DI 或成熟容器（Autofac、Simple Injector）。

---

## 全書總結

### 全書一句話

> DI 是一組讓程式鬆耦合的原則和模式；目的是可維護，手段是把「建立依賴」和「管理壽命」的控制權集中到 Composition Root，然後用 Decorator 把橫切關注點疊上去。容器只是可選的工具。

### 15 章地圖

| 部分 | 章 | 一句話 |
|---|---|---|
| Part 1 | 1 | DI 是「別人塞給你」，Service Locator 是「自己去要」；只替 Volatile Dependency 開 Seam |
| | 2 | Mary 的三層式程式：一行 `new CommerceContext()` 就把領域層焊死在資料層 |
| | 3 | 把箭頭反過來：介面歸使用者所有（DIP），由外往內寫，在 Composition Root 組裝 |
| Part 2 | 4 | 四個 pattern：Composition Root、Constructor（預設）、Method（Entity／外掛）、Property（library） |
| | 5 | 四個 anti-pattern：Control Freak、Service Locator（最危險）、Ambient Context、Constrained Construction |
| | 6 | 三個 code smell：建構子參數太多、濫用 Abstract Factory、循環依賴——根源多半是 SRP |
| Part 3 | 7 | 各框架的 Seam：`Main`、`OnLaunched`、`IControllerActivator` |
| | 8 | Singleton／Transient／Scoped；依賴壽命 ≥ 消費者壽命，否則 Captive Dependency |
| | 9 | Interception＝Decorator；Circuit Breaker、錯誤處理、權限都能疊上去 |
| | 10 | **高潮**：CQS → ISP → Parameter Object → `ICommandService<TCommand>`，一個 aspect 一個 Decorator |
| | 11 | 動態攔截當過渡；編譯期織入是 DI 的反面 |
| Part 4 | 12 | Composition Root 要嘛 Pure DI、要嘛 Auto-Registration；不要自己寫容器 |
| | 13 | Autofac：明確、從 lifetime scope 解析；Composite 是弱項 |
| | 14 | Simple Injector：嚴格、ambient scope、`Verify()`、序列是 stream |
| | 15 | MS.DI：功能太少，自己硬湊很脆弱 |

### 名詞速查

| 術語 | 一句話 | 章 |
|---|---|---|
| Stable / Volatile Dependency | 能直接用的 vs. 要藏在 Seam 後面的（資料庫、檔案、時間、亂數…） | 1 |
| Seam | 對抽象寫程式的地方，程式可以在這裡拆開 | 1 |
| Pure DI | 不用容器、在 Composition Root 自己 `new` | 1 |
| Composition Root | 唯一組裝 object graph 的地方，靠近進入點 | 4 |
| Local / Foreign Default | 同模組 vs. 別模組的預設實作；後者＝Control Freak | 4、5 |
| Temporal Coupling | 必須先呼叫 A 才能呼叫 B（`Initialize()`） | 4 |
| Leaky Abstraction | 抽象洩漏實作細節（`IDisposable`、無參數 `Create()`、框架型別） | 6 |
| Facade Service | 把一群互動的依賴藏在一個抽象後面 | 6 |
| Composer | 任何組裝依賴的物件或方法（容器或 Pure DI 的方法） | 8 |
| Lifestyle | 依賴預期活多久的正式描述 | 8 |
| Captive Dependency | 短命依賴被長命消費者抓住 | 8 |
| Interception | 在消費者和服務之間插一層，兩邊都不改 | 9 |
| Cross-Cutting Concern | 稽核、日誌、快取、驗證、權限、容錯… | 9 |
| CQS | 方法要嘛回傳值不改狀態，要嘛改狀態不回傳值 | 10 |
| Passive attribute | 只帶中繼資料、不含行為的 attribute | 10 |
| Auto-Wiring | 容器用 reflection 讀建構子自動接線 | 12 |
| Auto-Registration | 用慣例掃描 assembly 一次註冊一整群 | 12 |

### 我的整理：全書帶走的七句話

1. **不要在業務 class 裡 `new` 有副作用的東西**（資料庫、檔案、網路、時間、亂數），用建構子要進來。
2. **介面由使用者定義**，放在使用它的那一層；實作在外圍。
3. **只有一個地方知道整張接線圖**：Composition Root。容器的 `Resolve` 只准出現在這裡。
4. **建構子只做 Guard Clause 和存欄位**；參數超過 3～4 個就是 SRP 警訊。
5. **誰建立誰負責銷毀**；依賴的壽命不能短於消費者。
6. **橫切關注點用 Decorator 疊**；想一個 Decorator 套全部，就讓元件有共同的形狀（單方法泛型介面）。
7. **容器是可選的**：小專案 Pure DI，大了再用 Auto-Registration；不要自己寫，也不要只用 Configuration as Code。
