# Raw HTTP Server in C, Go, and Rust

這是一個以學習為目的的 Spec Driven Development 專案：分別使用 C、Go、
Rust，從 TCP byte stream 開始實作 HTTP/1.1，藉此比較三種語言如何處理
相同的網路、解析、錯誤與資源管理問題。

專案靈感來自 ThePrimeagen 與 Boot.dev 的
[From TCP to HTTP](https://www.youtube.com/watch?v=FknTw9bJsXM) 課程。

## 開發環境

本專案統一使用 Ubuntu on WSL 2 進行開發與驗證。完整的 WSL、C、Go、
Rust、Git、專案複製及骨架驗證流程，請參考
[WSL 開發環境設定](docs/development-setup.md)。

TDD、累積回歸、Definition of Done 與 Git milestone 規則，請參考
[開發與里程碑工作流程](docs/development-workflow.md)。

## 統一驗證

在 WSL 的 repository 根目錄執行：

```bash
make verify
```

此命令會依序執行三種語言的測試與品質檢查：

- C：嚴格 compiler warnings、原生測試、AddressSanitizer 與
  UndefinedBehaviorSanitizer。
- Go：`gofmt`、`go vet` 與 `go test`。
- Rust：`cargo fmt`、`cargo clippy` 與 `cargo test`。

任一檢查失敗時，整體驗證也會失敗。

## 學習目標

- 理解 TCP stream、buffer、partial read 與 HTTP/1.1 的核心機制。
- 以 function 對 function 的方式比較 C、Go、Rust。
- 保留各語言慣用的型別、錯誤處理、記憶體與資源管理方式。
- 透過規格驗收測試、TDD 與累積回歸測試降低早期設計錯誤的風險。

## 實作界線

實作與測試預設只使用標準工具鏈、語言標準函式庫與作業系統 Socket
API，不使用現成 HTTP Server、HTTP Parser、Web framework、async
runtime 或第三方實作依賴。C 版以 WSL 中的 POSIX sockets 為基準。

## 九個里程碑

1. HTTP Streams
2. TCP
3. Requests
4. Request Lines
5. HTTP Headers
6. HTTP Body
7. HTTP Responses
8. Chunked Encoding
9. Binary Data

每個里程碑都會先定義驗收案例，再分別以三種語言實作。里程碑完成前，
必須通過該階段與所有先前階段的測試。

## 專案狀態

目前已完成 OpenSpec 規格提案、三語言專案骨架與統一驗證基礎，尚未開始
HTTP Server 實作。詳細規格位於
[`openspec/changes/build-three-language-http-course`](openspec/changes/build-three-language-http-course)。

## 預計結構

```text
raw-http-server/
├── Makefile
├── README.md
├── c/
│   ├── Makefile
│   └── main.c
├── docs/
│   ├── development-setup.md
│   └── development-workflow.md
├── go/
│   ├── go.mod
│   └── main.go
├── rust/
│   ├── Cargo.lock
│   ├── Cargo.toml
│   └── src/
│       └── main.rs
└── testdata/
```

`testdata/` 保存三種語言共用且 byte-exact 的驗收資料；格式與命名慣例請
參考 [`testdata/README.md`](testdata/README.md)。

本專案依據適用的 HTTP/1.1 RFC 實作與驗證課程範圍內的協定行為，
但不以涵蓋完整 RFC、production-ready 或高效能 HTTP Server 為目標；
所有刻意簡化的行為都會在對應里程碑中明確記錄。
