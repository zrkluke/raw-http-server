# Step 7：HTTP Responses

這個分支讓 C、Go、Rust 的 TCP server 在收到一個 request 後，回傳同一份
有限範圍的 HTTP/1.1 response。重點不是 routing 或 framework，而是親手組裝並
傳送 HTTP bytes：status line、headers、空白行與 body。

## 這一步完成了什麼

三個實作都回傳下列固定 response，並由 shared fixture 驗證每一個 byte：

```http
HTTP/1.1 200 OK
Content-Type: text/plain
Content-Length: 13
Connection: close

Hello, World!
```

`Content-Length` 不是手動寫死成協定的一部分；它由 body 的 byte 長度產生。
TCP integration test 會真的連到本機 listener、送出 request、讀到 server
關閉連線為止，再將 response 與 fixture 逐 byte 比較。

## 驗收

請在 WSL 的 repository 根目錄執行：

```bash
make verify
```

它會執行 C 的 strict warnings、sanitizers 與 tests，Go 的 `gofmt`、`go vet`
與 tests，以及 Rust 的 `cargo fmt --check`、Clippy 與 tests；同時保留所有前面
Step 1–6 的回歸測試。

## Function 對照

| 責任 | C | Go | Rust |
| --- | --- | --- | --- |
| 建立固定 200 response | [`http_response_basic_ok`](c/http_response.c) | [`httpresponse.BasicOK`](go/internal/httpresponse/response.go) | [`Response::basic_ok`](rust/src/lib.rs) |
| 組裝 response bytes | [`http_response_build`](c/http_response.c) | [`Response.Bytes`](go/internal/httpresponse/response.go) | [`Response::bytes`](rust/src/lib.rs) |
| 接收一次並回寫 response | [`tcp_server_respond_once`](c/tcp_server.c) | [`StartResponseOnce`](go/internal/tcpserver/server.go) | [`TcpServer::start_response_once`](rust/src/lib.rs) |
| TCP byte-exact 驗收 | [`response_acceptance_test.c`](c/tests/response_acceptance_test.c) | [`response_acceptance_test.go`](go/internal/tcpserver/response_acceptance_test.go) | [`response_acceptance.rs`](rust/tests/response_acceptance.rs) |

C 明確配置與釋放 response buffer；Go 以 slice、`error`、`defer` 和 `io.Writer`
處理寫入；Rust 以 `Vec<u8>`、`Result`、`write_all` 與 thread/RAII 管理資源。
三者的 observable behavior 相同，但不做逐行翻譯。

## 重要檔案

- [`testdata/responses/basic/`](testdata/responses/basic)：request 與預期 response
  的原始 bytes。
- [`go/internal/httpresponse/response.go`](go/internal/httpresponse/response.go)：Go
  的 response serializer。
- [`c/http_response.c`](c/http_response.c)：C 的 response builder。
- [`rust/src/lib.rs`](rust/src/lib.rs)：Rust 的 `Response` 與 one-shot response
  server。

## 這一步刻意不做

- 依據 method、path 或 application handler 選擇不同 response。
- 完整解析並驗證 request 後才回應；此處只需要讀到非空 request bytes。
- keep-alive、多 request pipeline、chunked response、TLS、HTTP/2 或 framework。

## 導覽

- 前一階段：[Step 6：HTTP Body](https://github.com/zrkluke/raw-http-server/tree/step-6-body-v1)
- 目前分支：`step-7-responses`
- 完整學習地圖：[main README](https://github.com/zrkluke/raw-http-server/tree/main)
- 環境與流程：[development setup](docs/development-setup.md)、
  [development workflow](docs/development-workflow.md)
