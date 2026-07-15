# Step 5：HTTP Headers

這個分支建立在 Step 1 的 CRLF line reader、Step 2 的 TCP listener、Step 3 的
request-head state machine 與 Step 4 的 request-line parser 之上。C、Go、Rust
現在都能從第一個 header field 開始，逐步讀取 HTTP header section，直到空白的
CRLF 行結束。

本步不處理 `Content-Length` 或 body；它只將每一個完整 header line 拆成名稱與值，
並保留 TCP bytes 可能分段到達的事實。

## 本步完成的行為

共享 fixtures [`testdata/headers/`](testdata/headers) 以原始 bytes 與不同的
`chunks.txt` 切分，驗證三種語言在下列情況得到相同結果：

| 類型 | 代表案例 | 預期行為 |
| --- | --- | --- |
| 正常欄位 | `Host: example.com` | 名稱正規化為 `host`，值為 `example.com` |
| 不分大小寫 | `CONTENT-TYPE: text/plain` | 名稱轉為 ASCII lowercase |
| optional whitespace | `X-Note: \t hello \t` | 只去除值兩端的 SP／HTAB |
| 重複名稱 | 兩個 `Set-Cookie` | 依輸入順序保留兩個欄位 |
| 空白 section | 直接收到 `\r\n` | 完成、沒有 header field |
| TCP 分段 | 名稱、值、CRLF 分屬不同 chunks | 與完整輸入相同 |
| 格式錯誤 | 缺少 colon、leading whitespace、bare LF／CR | `invalid` |

一個真正的空白行是剛好 `\r\n`：前面的 CRLF 已完成上一個 header field，下一個
CRLF 則告訴 HTTP parser「header section 到這裡為止」。只有 `\r` 仍是 incomplete；
收到非 `\n` 的下一個 byte 後才可確認為 invalid。

## 驗收方式

在 WSL 的 repository 根目錄執行：

```bash
make verify
```

本次驗收累積涵蓋 Step 1 到 Step 5：

- C：strict warnings、native unit／shared acceptance tests、AddressSanitizer 與
  UndefinedBehaviorSanitizer。
- Go：`gofmt`、`go vet`、native unit tests 與 shared acceptance test。
- Rust：`cargo fmt --check`、Clippy、native unit tests 與 shared acceptance test。

## Function 對照

| 責任 | C | Go | Rust |
| --- | --- | --- | --- |
| 建立 parser | [`header_parser_init`](c/header_parser.c) | [`NewHeaderParser`](go/internal/httprequest/header.go) | [`HeaderParser::new`](rust/src/lib.rs) |
| 餵入 TCP bytes | [`header_parser_feed`](c/header_parser.c) | [`HeaderParser.Feed`](go/internal/httprequest/header.go) | [`HeaderParser::feed`](rust/src/lib.rs) |
| 取得 parser 狀態 | [`header_parser_state`](c/header_parser.c) | `HeaderParser.State` | [`HeaderParser::state`](rust/src/lib.rs) |
| 保存已完成欄位 | `struct header_field` | `Header` | `Header` |
| 釋放／結束生命週期 | [`header_parser_free`](c/header_parser.c) | Go GC | Rust RAII |

C 的 parser 自己配置可成長的 line buffer 與 field array，並要求呼叫端最後呼叫
`header_parser_free`。Go 使用 slice 與 `Header` struct；失敗以 `ParserInvalid` 表達。
Rust 讓 `Vec<u8>` 與 `Vec<Header>` 擁有資料，離開 scope 時由 RAII 釋放，並以
`HeaderState` enum 表達 incomplete、complete、invalid。三者都保留重複欄位順序，
不把 header 視為 map。

## 重要檔案

| 檔案 | 用途 |
| --- | --- |
| [`testdata/headers/`](testdata/headers) | 共用原始 bytes、預期輸出與 chunk 分段 |
| [`go/internal/httprequest/header.go`](go/internal/httprequest/header.go) | Go 的 incremental header parser |
| [`go/internal/httprequest/header_acceptance_test.go`](go/internal/httprequest/header_acceptance_test.go) | Go 讀取 shared fixtures |
| [`c/header_parser.h`](c/header_parser.h) | C 的 parser、state 與 field 契約 |
| [`c/header_parser.c`](c/header_parser.c) | C 的 buffer、欄位配置與 cleanup 實作 |
| [`c/tests/header_parser_acceptance_test.c`](c/tests/header_parser_acceptance_test.c) | C 讀取 shared fixtures |
| [`rust/src/lib.rs`](rust/src/lib.rs) | Rust 的 `HeaderParser`、`HeaderState` 與單元測試 |
| [`rust/tests/header_parser_acceptance.rs`](rust/tests/header_parser_acceptance.rs) | Rust 讀取 shared fixtures |

## 本步刻意不做

- `Content-Length`、request body、body framing 或 header-specific semantics。
- 完整 RFC field-name grammar、header size limits、trailers 或 obs-fold 相容模式。
- 多 client、keep-alive、timeout policy、TLS 或 production hardening。
- `net/http`、HTTP parser、web framework、async runtime 或第三方實作依賴。

## 版本導覽

- 前一個 immutable snapshot：[Step 4 · Request Lines](https://github.com/zrkluke/raw-http-server/tree/step-4-request-lines-v1)
- 這是開發分支 `step-5-headers`；通過文件 review、commit 與 tag 後，會建立
  Step 5 的 immutable snapshot。
- 專案總覽與完整學習地圖：[main README](https://github.com/zrkluke/raw-http-server/tree/main)
- 開發環境與流程：[WSL setup](docs/development-setup.md) · [development workflow](docs/development-workflow.md)
