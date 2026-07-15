# Step 6：HTTP Body

這一步讓 C、Go、Rust 在已知 `Content-Length` 後，從 TCP byte stream
逐步累積 HTTP request body。parser 只取走宣告長度內的 bytes，並保留多出
的 bytes，讓後續功能可以繼續處理它們。

這不是把 body 當成文字，也不會假設一次 TCP read 就剛好拿到完整 body。

## 本步完成的行為

共享 fixture [`testdata/bodies/`](testdata/bodies) 驗證五種情況：

| 情況 | `Content-Length` | 輸入 bytes | 預期結果 |
| --- | --- | --- | --- |
| 缺少欄位 | absent | `after\n` | complete；body 為空，全部保留為 remaining |
| 完整 body | `6` | `hello\n` | complete；累積六個 bytes |
| 尚未收齊 | `6` | `hel\n` | incomplete；保留已收到的四個 bytes |
| 非法欄位 | `not-a-number` | `hello\n` | invalid；不消耗任何 bytes |
| 收到過多 | `4` | `cat\nnext\n` | complete；body 是 `cat\n`，`next\n` 保留 |

`chunks.txt` 仍描述 TCP read 的分段方式。相同的 raw bytes 即使在不同
chunk 邊界到達，最終 body、state 與 remaining bytes 都必須一致。

## 驗收

在 WSL 的 repository 根目錄執行：

```bash
make verify
```

這會累積驗證 Step 1 到 Step 6：

- C：strict warnings、native unit/shared acceptance tests、ASan 與 UBSan
- Go：`gofmt`、`go vet`、native unit/shared acceptance tests
- Rust：`cargo fmt --check`、Clippy、native unit/shared acceptance tests

## Function 對照

| 責任 | C | Go | Rust |
| --- | --- | --- | --- |
| 建立 body parser | [`body_parser_init`](c/body_parser.c) | [`NewBodyParser`](go/internal/httprequest/body.go) | [`BodyParser::new`](rust/src/lib.rs) |
| 餵入 TCP bytes | [`body_parser_feed`](c/body_parser.c) | [`BodyParser.Feed`](go/internal/httprequest/body.go) | [`BodyParser::feed`](rust/src/lib.rs) |
| 取得 parser state | [`body_parser_state`](c/body_parser.c) | `BodyParser.State` | [`BodyParser::state`](rust/src/lib.rs) |
| 取得已累積 body | [`body_parser_body`](c/body_parser.c) | `BodyParser.Body` | [`BodyParser::body`](rust/src/lib.rs) |
| 取得未消耗 bytes | [`body_parser_remaining`](c/body_parser.c) | `BodyParser.Remaining` | [`BodyParser::remaining`](rust/src/lib.rs) |
| 結束生命週期 | [`body_parser_free`](c/body_parser.c) | Go GC | Rust RAII |

C 的 `struct body_parser` 明確管理 body 與 remaining 的配置容量，並由呼叫端
執行 `body_parser_free`。Go 使用兩個可成長的 `[]byte`，以 `State` 與
copy-on-read slice 表示結果。Rust 則以 `Vec<u8>` 擁有兩段資料，`BodyState`
表達狀態，離開 scope 時由 RAII 自動釋放記憶體。

## 重要檔案

| 檔案 | 用途 |
| --- | --- |
| [`testdata/bodies/`](testdata/bodies) | 共用 body bytes、state、remaining 與 chunk fixture |
| [`testdata/README.md`](testdata/README.md) | body fixture metadata 的精確定義 |
| [`go/internal/httprequest/body.go`](go/internal/httprequest/body.go) | Go 的 `Content-Length` body parser |
| [`go/internal/httprequest/body_acceptance_test.go`](go/internal/httprequest/body_acceptance_test.go) | Go 讀取 shared fixtures |
| [`c/body_parser.h`](c/body_parser.h) | C 的 state、buffer 與 ownership 契約 |
| [`c/body_parser.c`](c/body_parser.c) | C 的 bytes 累積與 remaining 管理 |
| [`c/tests/body_parser_acceptance_test.c`](c/tests/body_parser_acceptance_test.c) | C 讀取 shared fixtures |
| [`rust/src/lib.rs`](rust/src/lib.rs) | Rust 的 `BodyParser`、`BodyState` 與單元測試 |
| [`rust/tests/body_parser_acceptance.rs`](rust/tests/body_parser_acceptance.rs) | Rust 讀取 shared fixtures |

## 本步刻意不做

- 從 parsed headers 自動找出或驗證 `Content-Length`；本步以 fixture metadata
  模擬已取得的欄位值。
- `Transfer-Encoding: chunked`、trailers 或其他 body framing。
- 把 body 解碼成 UTF-8、JSON、form data，或交給 application framework。
- keep-alive、多個 request 的 pipeline、response 寫回與 production hardening。

## 版本導覽

- 前一步固定快照：[Step 5：HTTP Headers](https://github.com/zrkluke/raw-http-server/tree/step-5-headers-v1)
- 目前開發分支：`step-6-body`
- 本步通過文件 review、commit 與 tag 後，會建立 immutable snapshot。
- 完整學習地圖請看 [main README](https://github.com/zrkluke/raw-http-server/tree/main)。
- WSL 與工作流程請看 [development setup](docs/development-setup.md) 與
  [development workflow](docs/development-workflow.md)。
