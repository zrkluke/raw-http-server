# AGENTS.md

本文件定義 AI agent 在此 repository 內長期協作時必須遵守的規則。
需求、設計、驗收條件與任務進度以 OpenSpec 為準；本文件不取代規格。

## 專案目的

- 這是學習型專案，不以 production-ready、完整 RFC conformance 或效能競賽為目標。
- 主要目標是理解 TCP byte stream 與 HTTP/1.1 的核心機制。
- 使用 C、Go、Rust 解決相同問題，觀察型別、錯誤處理、記憶體與資源管理差異。
- 比較粒度是 function-to-function 的責任與行為，不做 line-by-line 翻譯。
- 未來教學文章與面試展示是衍生價值，不得凌駕於實際理解與可驗證行為。

## 規格與任務來源

- 開始規格相關實作前，先閱讀適用的 OpenSpec proposal、design、specs 與 tasks。
- OpenSpec 是需求、設計、驗收條件與任務狀態的 source of truth。
- `AGENTS.md` 管理長期協作行為；`docs/` 管理環境與教學文件；README 提供入口與學習地圖。
- 若實作與 OpenSpec 衝突，先指出衝突並修正或確認規格，不可靜默偏離。
- 不因方便而擴大 scope；不確定的 protocol 行為要先查明適用規格並記錄簡化項目。
- 完成 OpenSpec task 後立即更新 checkbox；未達 Definition of Done 不得標記完成。

## 技術邊界

- 僅允許語言標準函式庫與作業系統 Socket API。
- C 以 WSL Ubuntu 的 POSIX sockets 為基準，不實作 Windows Winsock 版本。
- Go 可使用 `net` 與標準 I/O 套件，但不可使用 `net/http` 完成 server 或 parsing。
- Rust 使用 `std::net` 與標準 I/O traits。
- 實作與測試預設只使用標準工具鏈、標準函式庫與作業系統 API。
- 禁止現成 HTTP server、HTTP parser、Web framework、async runtime 與第三方實作依賴。
- 測試基礎設施若確實需要第三方開發工具，必須先取得使用者明確同意，且不得成為 HTTP 實作依賴。
- 不實作 HTTP/2、HTTP/3、TLS 或 routing framework，除非 OpenSpec 明確新增範圍。

## 里程碑工作順序

每個 milestone 固定依序進行：

1. 先建立可執行且因功能尚未實作而失敗的 acceptance case。
2. 以 native test 進行 Go 的 Red-Green-Refactor。
3. 以 native test 進行等價 C 實作的 Red-Green-Refactor。
4. 以 native test 進行等價 Rust 實作的 Red-Green-Refactor。
5. 在 WSL 執行三語言統一回歸與品質驗證。
6. 記錄 function-to-function 比較與刻意差異。
7. Definition of Done 通過後，才建立 milestone ref。

Go 是每個 milestone 的第一份參考實作，但 C 與 Rust 必須維持各自慣用設計，
不可直接逐行翻譯 Go。

## 測試與驗收

- 新增可觀察行為前，先展示會失敗的 acceptance test 或 unit test。
- 遵循 Red-Green-Refactor：先失敗、最小實作通過、保持綠燈後重構。
- incremental parser 必須涵蓋完整輸入、逐 byte、CRLF 邊界、欄位邊界及固定大小 chunks。
- 修正 defect 前，先新增能重現錯誤的 regression test。
- 每個 milestone 都必須通過當前及所有先前 milestone 的測試。
- networking milestone 應使用相同 bytes 對三種實作執行 black-box TCP integration tests。
- 不得把「可以編譯」當成 milestone 完成；必須符合 OpenSpec Definition of Done。

## 三語言對照原則

- 三種語言對應 function 應有相同責任與 observable behavior。
- 不要求相同的 signature、型別、資料結構或內部演算法。
- C 應清楚呈現 buffer 邊界、pointer、ownership、錯誤碼與 resource cleanup。
- Go 應清楚呈現 slices、`error`、`defer` 與標準 I/O abstraction。
- Rust 應清楚呈現 ownership、borrowing、enums、`Result` 與 RAII。
- 不為追求表面一致而犧牲語言慣用性或隱藏重要的資源管理差異。

## WSL 與品質驗證

- Ubuntu on WSL 2 是 C／POSIX 實作與跨語言驗收的共同環境。
- 修改實作或測試後，執行 repository 提供的統一 WSL verification command。
- C 使用 strict warnings；在對應 task 完成後加入 sanitizer checks。
- Go 使用 native test tooling；Rust 使用 tests 與 lints。
- build artifacts 不得提交。Rust application 的 `Cargo.lock` 應提交。
- Windows 可用於編輯、文件與 Git 操作，但不得取代必要的 WSL 驗收。

## Git 規則

- 未經使用者明確同意，不得 commit、push、force-push、建立或刪除 branch/tag。
- 未經使用者明確要求，不直接在 `main` 開發 milestone 實作。
- milestone 為累積式；immutable tag 是文章與完成狀態的 canonical reference。
- 不覆蓋、刪除或回復使用者既有且與目前任務無關的變更。
- 禁止使用 `git reset --hard`、`git checkout --` 等破壞性操作，除非使用者明確要求且已確認範圍。
- Commit message 預設使用繁體中文 Conventional Commits。
- Header 格式為 `<type>(<scope>): <subject>`；subject 不超過 50 字且不加句號。
- Body 說明變動內容、原因與先前行為差異，每行不超過 72 字元。

## 文件規則

- README 保持精簡，放置專案目的、學習地圖、快速開始與詳細文件連結。
- 安裝、工作流程、測試方式與深入比較放在 `docs/`。
- 每個 milestone 記錄對應 function、行為契約、語言差異與驗證方式。
- 文件應解釋「為什麼」，不能只重述程式碼。

## AI 協作與教學方式

- Agent 不只交付程式碼；實作前要說明本步驟的責任、設計與驗收方式。
- 一次推進可驗收的小步驟，避免同時引入多個尚未理解的 abstraction。
- 請使用者執行命令時，要交代執行環境、預期輸出與常見失敗原因。
- 每個核心 function 完成後，提供 C、Go、Rust 的責任對照與重要語言差異。
- 對話中發現先前說法或實作錯誤時，明確承認、更正並加入必要的防回歸措施。
- 編譯與測試由 Agent 協助驗證，但也要讓使用者能解釋、修改與除錯核心行為。
- 不把大量輸入工作等同於學習成果；優先建立可觀察、可測試、可解釋的理解。
