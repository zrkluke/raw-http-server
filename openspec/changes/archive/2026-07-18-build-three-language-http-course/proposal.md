## Why

以 C、Go、Rust 從 TCP byte stream 開始實作 HTTP/1.1，藉由解決相同問題來理解三種語言在型別、錯誤處理、記憶體與資源管理上的特性。專案需要在實作前固定共同範圍與驗收方式，避免後期才發現早期 parser 或 buffer 設計錯誤。

## What Changes

- 建立同一 repository 內的 C、Go、Rust 獨立專案。
- 依原課程九個章節建立累積式里程碑：HTTP Streams、TCP、Requests、Request Lines、HTTP Headers、HTTP Body、HTTP Responses、Chunked Encoding、Binary Data。
- 三種實作採用相同的功能責任與行為契約，但保留各語言的慣用寫法，提供 function 對 function 的比較基礎。
- 實作與測試預設僅使用標準工具鏈、語言標準函式庫與 POSIX Socket API，不使用現成 HTTP 函式庫、HTTP parser、Web framework 或第三方實作依賴；若測試基礎設施確實需要第三方開發工具，須先取得明確同意，且不得成為 HTTP 實作依賴。
- 建立規格驗收測試、各語言 unit tests 與跨語言 TCP integration tests，並要求每個里程碑重跑全部既有 regression tests。
- 以 WSL 作為 C／POSIX 實作與整體驗收的共同執行環境。
- 使用累積式 Git 里程碑保存各章狀態，供日後文章引用；production-ready HTTP server 不在專案目標內。

## Capabilities

### New Capabilities

- `three-language-http-server`: 規範 C、Go、Rust 對九個 HTTP 學習里程碑的共同功能、範圍與跨語言行為一致性。
- `milestone-validation`: 規範每個里程碑的 test-first 驗收、TDD、回歸測試及完成條件。

### Modified Capabilities

<!-- No existing capabilities are modified. -->

## Impact

- 新增 `c/`、`go/`、`rust/` 三個實作區域與共用測試資料。
- 新增 WSL 下的統一建置與測試入口。
- 新增九個累積式開發里程碑及對應 Git refs。
- 不引入第三方實作 dependencies；C 依賴 POSIX API，Go 與 Rust 依賴各自標準函式庫。
