# Step 1：HTTP Streams

本分支完成第一個可觀察的 HTTP 前置行為：從任意切分的 TCP
byte chunks 累積資料，直到讀到 `\r\n`，才產生一個完整 line。
輸出的 line 不包含 CRLF；尚未完整的資料會保留到下一次 feed。

這一步尚未建立 TCP listener，也尚未解析 HTTP request line、headers
或 body。它只處理後續 HTTP parser 共同依賴的串流分行責任。

## 已完成的行為

同一份 `testdata/http-streams/` fixture 會以三種切法送入 C、Go、Rust：

- `complete-line`：整行一次送入。
- `crlf-split`：CR 在前一個 chunk、LF 在下一個 chunk。
- `one-byte`：每次只送入一個 byte。

三種切法都必須得到相同的 `hello\n` 驗收輸出。空行 `\r\n`
也會產生空 line，為後續 HTTP headers 的終止行保留正確行為。

## 驗收與品質檢查

在 WSL 的 repository 根目錄執行：

```bash
make verify
```

它會依序執行：

- C：嚴格編譯、shared fixture 驗收、unit tests、AddressSanitizer 與
  UndefinedBehaviorSanitizer。
- Go：`gofmt`、`go vet`、shared fixture 驗收與 unit tests。
- Rust：`cargo fmt --check`、`cargo clippy -D warnings`、shared fixture
  驗收與 unit tests。

本分支的最後一次 WSL `make verify` 已通過。

## 三語言 function 對照

| 責任 | C | Go | Rust |
| --- | --- | --- | --- |
| 初始化 reader | [`line_reader_init`](c/line_reader.c) | `LineReader` 零值 | [`LineReader::new`](rust/src/lib.rs) |
| 餵入 TCP bytes | [`line_reader_feed`](c/line_reader.c) | [`LineReader.Feed`](go/internal/httpstream/line_reader.go) | [`LineReader::feed`](rust/src/lib.rs) |
| 尚未完整的資料 | `pending` 指標、長度與容量 | 私有 `pending []byte` | 私有 `pending: Vec<u8>` |
| 完整 line 的交付 | callback；指標只在 callback 期間有效 | 回傳 `[][]byte` | 回傳 `Vec<Vec<u8>>` |
| 資源釋放 | [`line_reader_free`](c/line_reader.c) | GC 管理 slice | `Vec` 在離開 scope 時自動釋放 |

三個版本都先附加新 chunk、搜尋 CRLF、產生完整 line，再保留最後一段
不完整 bytes。差異在於 C 明確管理 buffer 與 callback 的生命週期，Go
以 slice 與 GC 管理資料，Rust 則透過所有權讓回傳的 line 脫離內部 buffer。

## 重要檔案

| 檔案 | 用途 |
| --- | --- |
| [`testdata/http-streams/`](testdata/http-streams) | 三語言共用、byte-exact 的輸入、期望輸出與 chunk 切法 |
| [`c/line_reader.c`](c/line_reader.c) | C 的動態 buffer、CRLF 搜尋與 callback 輸出 |
| [`c/tests/line_reader_acceptance_test.c`](c/tests/line_reader_acceptance_test.c) | C 的 shared fixture 驗收 |
| [`go/internal/httpstream/line_reader.go`](go/internal/httpstream/line_reader.go) | Go 的 LineReader 實作 |
| [`go/internal/httpstream/line_reader_acceptance_test.go`](go/internal/httpstream/line_reader_acceptance_test.go) | Go 的 shared fixture 驗收 |
| [`rust/src/lib.rs`](rust/src/lib.rs) | Rust 的 LineReader 與 native unit tests |
| [`rust/tests/line_reader_acceptance.rs`](rust/tests/line_reader_acceptance.rs) | Rust 的 shared fixture 驗收 |

## 刻意不包含

- TCP `listen`、`accept`、`read`、`write` 或 connection cleanup。
- HTTP request line、headers、body、response 或 routing。
- 高階 HTTP 函式庫、HTTP parser、Web framework、async runtime 或第三方
  implementation dependency。

完整專案界線、環境安裝與 Red-Green-Refactor 流程請看：

- [`main` README](https://github.com/zrkluke/raw-http-server/tree/main)
- [`docs/development-setup.md`](docs/development-setup.md)
- [`docs/development-workflow.md`](docs/development-workflow.md)

## 版本導覽

- 前一步的驗證基線：[`step-0-foundation-v2`](https://github.com/zrkluke/raw-http-server/tree/step-0-foundation-v2)。
- 本步驟的 immutable snapshot：[`step-1-http-streams-v1`](https://github.com/zrkluke/raw-http-server/tree/step-1-http-streams-v1)。
- 下一步將在 `step-2-tcp` 分支加入 TCP listener，讓 byte stream 由真實連線提供。
