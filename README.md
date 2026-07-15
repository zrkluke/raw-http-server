# Step 4：HTTP Request Lines

這個分支建立在 Step 1 的 CRLF line reader、Step 2 的 TCP listener 與
Step 3 的 request-head state machine 之上。C、Go、Rust 現在都能將一條完整的
HTTP request line 拆成 `method`、`target` 與 `HTTP version`，或明確判定它無效。

本步處理的是 HTTP/1.1 request line 的最小學習契約：恰好三個非空 byte 欄位，
欄位之間只能有一個 SP（空白），且 version 必須完全等於 `HTTP/1.1`。這讓後續
header parser 可以接收已分好的第一行，而不用重新處理 TCP chunk 邊界。

## 本步完成的行為

共享 fixtures [`testdata/request-lines/`](testdata/request-lines) 以原始 bytes 與
不同的 `chunks.txt` 切分，驗證三種語言在下列情況得到相同結果：

| 類型 | 代表案例 | 預期結果 |
| --- | --- | --- |
| 有效 request line | `GET /example HTTP/1.1` | `GET|/example|HTTP/1.1` |
| target 帶 query | `GET /search?q=rust HTTP/1.1` | 保留完整 target |
| TCP 分段 | 最終 CRLF、欄位邊界、逐 byte、固定大小切分 | 與完整輸入相同 |
| 格式錯誤 | 缺欄位、前後／重複空白、tab | `invalid` |
| 不支援 version | `HTTP/1.0` | `invalid` |

`LineReader` 先依 CRLF 從 TCP bytes 取出不含 terminator 的一行，request-line
parser 才檢查欄位。這兩層責任分開，才能正確處理 `\r` 與 `\n` 分屬不同 read 的
情況。

## 驗收方式

在 WSL 的 repository 根目錄執行：

```bash
make verify
```

本次驗收已涵蓋 Step 1 到 Step 4 的累積回歸：

- C：strict warnings、native unit／shared acceptance tests、AddressSanitizer 與
  UndefinedBehaviorSanitizer。
- Go：`gofmt`、`go vet`、native unit tests 與 shared acceptance test。
- Rust：`cargo fmt --check`、Clippy、native unit tests 與 shared acceptance test。

## Function 對照

| 責任 | C | Go | Rust |
| --- | --- | --- | --- |
| 解析一條 request line | [`request_line_parse`](c/request_line.c) | [`ParseRequestLine`](go/internal/httprequest/request_line.go) | [`RequestLine::parse`](rust/src/lib.rs) |
| 表達有效結果 | `struct request_line` 的 pointer + length slices | `RequestLine` 的 string fields 與 `Valid` | `RequestLine` 的私有 `Vec<u8>` 欄位 |
| 表達無效結果 | `REQUEST_LINE_INVALID` | 零值 `RequestLine{}`，`Valid == false` | `Err(RequestLineError::Invalid)` |
| 轉為 fixture 輸出 | acceptance test 組合 slices | `RequestLine.String()` | `Display for RequestLine` |

C 的結果直接借用輸入 buffer，因此呼叫端必須確保原始 bytes 還有效；它不額外配置
記憶體。Go 將三個 byte slice 轉成 `string`，以 `Valid` 表示是否可用。Rust 則把
欄位複製到受 ownership 管理的 `Vec<u8>`，以 `Result` 強迫呼叫端處理失敗分支。
三者的可觀察輸出相同，但記憶體與錯誤處理方式不同。

## 重要檔案

| 檔案 | 用途 |
| --- | --- |
| [`testdata/request-lines/`](testdata/request-lines) | 共用原始 bytes、預期輸出與 chunk 分段 |
| [`go/internal/httprequest/request_line.go`](go/internal/httprequest/request_line.go) | Go 的 request-line parser |
| [`go/internal/httprequest/request_line_acceptance_test.go`](go/internal/httprequest/request_line_acceptance_test.go) | Go 讀取 shared fixtures |
| [`c/request_line.h`](c/request_line.h) | C 的結果 struct 與 parser 契約 |
| [`c/request_line.c`](c/request_line.c) | C 的 pointer/length slice 實作 |
| [`c/tests/request_line_acceptance_test.c`](c/tests/request_line_acceptance_test.c) | C 讀取 shared fixtures |
| [`rust/src/lib.rs`](rust/src/lib.rs) | Rust 的 `RequestLine`、`RequestLineError` 與單元測試 |
| [`rust/tests/request_line_acceptance.rs`](rust/tests/request_line_acceptance.rs) | Rust 讀取 shared fixtures |

## 本步刻意不做

- 完整 HTTP method token grammar，或各種 request-target form 的語意驗證。
- header 名稱／值解析、case-insensitive 規則、body framing 與 `Content-Length`。
- 多 client、keep-alive、timeout policy、TLS 或 production hardening。
- `net/http`、HTTP parser、web framework、async runtime 或第三方實作依賴。

## 版本導覽

- 前一個 immutable snapshot：[Step 3 · Requests](https://github.com/zrkluke/raw-http-server/tree/step-3-requests-v1)
- 這是開發分支 `step-4-request-lines`；通過文件 review、commit 與 tag 後，會建立
  Step 4 的 immutable snapshot。
- 專案總覽與完整學習地圖：[main README](https://github.com/zrkluke/raw-http-server/tree/main)
- 開發環境與流程：[WSL setup](docs/development-setup.md) · [development workflow](docs/development-workflow.md)
