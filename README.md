# Step 3：Incremental Request States

這個分支在 Step 1 的 CRLF line reader 與 Step 2 的 TCP listener 之上，讓
C、Go、Rust 都能從分段到達的 TCP bytes 判斷 HTTP request head 目前是
`incomplete`、`complete` 或 `invalid`。

本步只處理 request head 的框架邊界：第一行不可為空，且後續必須以一個空白
CRLF 行結束。它尚未解讀 method、request target、HTTP version、header 或
body；這些責任會分別留給後續 steps。

## 本步完成的行為

共享 fixtures [`testdata/requests/`](testdata/requests) 定義三種結果，並以多種
chunk 切分方式輸入，確保 TCP read 邊界不會改變解析結果：

| Fixture | 輸入結果 |
| --- | --- |
| `incomplete-head` | 尾端只有 `\r`，仍是 `incomplete` |
| `complete-head` | 非空第一行後收到空白 `\r\n` 行，成為 `complete` |
| `bare-lf` | 收到不符合 CRLF 的單獨 `\n`，成為 `invalid` |

同樣地，若目前可見的最後一個 byte 是 `\r`，parser 會等待下一個 byte；只有
下一個 byte 不是 `\n` 時，才判定 `invalid`。一旦進入 `complete` 或
`invalid`，後續輸入不會改變結果。

## 驗收方式

在 WSL 的 repository 根目錄執行：

```bash
make verify
```

這會累積驗證 Step 1 到 Step 3：

- C：strict warnings、request parser unit／shared acceptance tests，以及
  AddressSanitizer 與 UndefinedBehaviorSanitizer。
- Go：`gofmt`、`go vet`、native unit tests 與 shared acceptance test。
- Rust：`cargo fmt --check`、Clippy、native unit tests 與 shared acceptance test。

## Function 對照

| 責任 | C | Go | Rust |
| --- | --- | --- | --- |
| 建立初始 parser | [`request_parser_init`](c/request_parser.c) | [`NewParser`](go/internal/httprequest/request.go) | [`RequestParser::new`](rust/src/lib.rs) |
| 送入一段 TCP bytes | [`request_parser_feed`](c/request_parser.c) | [`Parser.Feed`](go/internal/httprequest/request.go) | [`RequestParser::feed`](rust/src/lib.rs) |
| 表達解析狀態 | `enum request_state` | `State` | `RequestState` |
| 轉為 fixture 可比較文字 | [`request_state_name`](c/request_parser.c) | [`State.String`](go/internal/httprequest/request.go) | `Display for RequestState` |

三種實作都保留 `saw_first_line`、`line_has_bytes` 與 `pending_cr` 這三個
必要狀態。C 以 enum 與可變 struct 讓呼叫端持有記憶體；Go 回傳輕量 `State`，
並以 pointer receiver 更新 parser；Rust 以 enum 與 `&mut self` 表達唯一可變
借用，且不需要手動釋放 parser。

## 重要檔案

| 檔案 | 用途 |
| --- | --- |
| [`testdata/requests/`](testdata/requests) | 共用的 bytes、chunk 切分與預期狀態 |
| [`go/internal/httprequest/`](go/internal/httprequest) | Go parser、native tests 與 acceptance test |
| [`c/request_parser.c`](c/request_parser.c) | C 的狀態機實作 |
| [`c/tests/request_parser_acceptance_test.c`](c/tests/request_parser_acceptance_test.c) | C 載入 shared fixtures 的 acceptance test |
| [`rust/src/lib.rs`](rust/src/lib.rs) | Rust `RequestParser`、`RequestState` 與 native tests |
| [`rust/tests/request_parser_acceptance.rs`](rust/tests/request_parser_acceptance.rs) | Rust 載入 shared fixtures 的 acceptance test |

## 本步刻意不做

- request line 的 method、target、HTTP version 驗證。
- header 欄位語法、大小寫、空白規則與 body framing。
- 多 client、keep-alive、timeout policy、TLS 或 production hardening。
- `net/http`、HTTP parser、web framework、async runtime 或第三方實作依賴。

## 版本導覽

- 前一個 immutable snapshot：[Step 2 · TCP](https://github.com/zrkluke/raw-http-server/tree/step-2-tcp-v1)
- 本分支會在所有驗收、文件 review 與 commit 完成後，建立 Step 3 的 immutable tag。
- 專案總覽與完整學習地圖：[main README](https://github.com/zrkluke/raw-http-server/tree/main)
- 開發環境與流程：[WSL setup](docs/development-setup.md) · [development workflow](docs/development-workflow.md)
