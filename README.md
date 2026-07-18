# Step 8：Chunked Encoding

這個分支讓 C、Go、Rust 以增量方式解碼本課程範圍內的 HTTP/1.1
`Transfer-Encoding: chunked` body。它不是另一種文字編碼，而是一種 body framing：
每段資料先以十六進位大小宣告，再接資料與 `\r\n` delimiter。

## 這一步完成了什麼

三個實作都能處理：

- 十六進位 chunk size，例如 `4\r\nWiki\r\n`；
- 任意 TCP chunk 邊界，包括 size line、資料與 CRLF 被拆開；
- `0\r\n` terminal chunk；
- terminal chunk 後沿用既有 header grammar 解析的 trailers；
- 完成後保留同一批輸入中多出的 bytes，供下一個協定階段使用。

範例輸入：

```text
4\r\nWiki\r\n5\r\npedia\r\n0\r\n\r\n
```

解碼 body 是 `Wikipedia`。fixture 也驗證 trailer 被拆在不同輸入 chunk 時，仍能得到
相同結果。

## 刻意不做的範圍

- chunk extensions，例如 `4;name=value`；
- 禁止的 trailer field names；
- 根據 `Transfer-Encoding` 自動選擇 parser；
- chunked response、keep-alive、TLS 或 HTTP/2。

這些限制是為了讓注意力放在 byte framing 與 state machine，而不是完整 RFC 相容性。

## 驗收

請在 WSL 的 repository 根目錄執行：

```bash
make verify
```

它會執行 C 的 strict warnings、sanitizers 與 tests，Go 的 `gofmt`、`go vet`
與 tests，以及 Rust 的 `cargo fmt --check`、Clippy 與 tests；並保留 Step 1–7
的回歸驗證。

## Function 對照

| 責任 | C | Go | Rust |
| --- | --- | --- |
| 建立 parser | [`chunked_parser_new`](c/chunked_parser.c) | [`NewChunkedParser`](go/internal/httprequest/chunked.go) | [`ChunkedParser::new`](rust/src/lib.rs) |
| 增量接收 bytes | [`chunked_parser_feed`](c/chunked_parser.c) | [`ChunkedParser.Feed`](go/internal/httprequest/chunked.go) | [`ChunkedParser::feed`](rust/src/lib.rs) |
| 讀取 body、trailers、remaining | [`chunked_parser_body`](c/chunked_parser.c) 等 accessors | [`Body`、`Trailers`、`Remaining`](go/internal/httprequest/chunked.go) | [`body`、`trailers`、`remaining`](rust/src/lib.rs) |
| Shared fixture 驗收 | [`chunked_acceptance_test.c`](c/tests/chunked_acceptance_test.c) | [`chunked_acceptance_test.go`](go/internal/httprequest/chunked_acceptance_test.go) | [`chunked_acceptance.rs`](rust/tests/chunked_acceptance.rs) |

C 以 opaque parser、手動 buffer 成長與 `free` 表達 ownership；Go 以 slice、
`error` 與 callback 處理 state transition；Rust 以 `Vec<u8>`、enum state 與借用的
accessor 保持記憶體安全。三者保證相同 observable behavior，但不逐行翻譯。

## 重要檔案

- [`testdata/chunked/`](testdata/chunked/)：完整、fragmented trailer 與 malformed
  framing 的共享 bytes fixtures。
- [`c/chunked_parser.c`](c/chunked_parser.c)：C 的 chunked state machine。
- [`go/internal/httprequest/chunked.go`](go/internal/httprequest/chunked.go)：Go 實作。
- [`rust/src/lib.rs`](rust/src/lib.rs)：Rust 的 `ChunkedParser` 與 `ChunkedState`。

## 導覽

- 前一階段：[Step 7：HTTP Responses](https://github.com/zrkluke/raw-http-server/tree/step-7-responses-v2)
- 目前分支：`step-8-chunked-encoding`
- 完整學習地圖：[main README](https://github.com/zrkluke/raw-http-server/tree/main)
- 環境與流程：[development setup](docs/development-setup.md)、
  [development workflow](docs/development-workflow.md)
