# 從 TCP 位元組串流手寫 HTTP/1.1：C、Go、Rust 對照學習

這是一個學習型專案：不使用現成 HTTP server、HTTP parser 或 web framework，
而是從 TCP socket 讀到的 bytes 開始，逐步手寫 HTTP/1.1 的核心機制。

同一組行為會分別以 C、Go、Rust 實作。重點不是把程式逐行翻譯，而是比較同一責任
在不同語言中如何表達：C 的 buffer、pointer 與 cleanup，Go 的 slice、`error` 與
goroutine，以及 Rust 的 ownership、`Result` 與 RAII。

## 你會學到什麼

- TCP 是沒有訊息邊界的 byte stream；一次 `read` 不等於一次 HTTP request。
- HTTP/1.1 如何用 CRLF、request line、headers 與 body framing 建立訊息邊界。
- 為什麼 parser 必須處理逐 byte、CRLF 跨 read、欄位跨 read 與多種切分方式。
- `Content-Length`、chunked transfer encoding 與 binary body 如何影響 socket 讀取。
- 三種語言如何以不同的型別、錯誤與資源管理模型完成相同行為。

## 學習路徑

| Step | 主題 | 建立的責任 |
| --- | --- | --- |
| 1 | HTTP Streams | 從 fragmented TCP bytes 讀出 CRLF line |
| 2 | TCP | listen、connect、accept、read、cleanup |
| 3 | Requests | 區分 incomplete、complete、invalid request head |
| 4 | Request Lines | 驗證 method、target、`HTTP/1.1` |
| 5 | Headers | 增量解析、名稱正規化與 CRLF termination |
| 6 | Body | 依 `Content-Length` 精確讀取 bytes |
| 7 | Responses | 序列化與傳送 HTTP response |
| 8 | Chunked Encoding | 解碼 chunk size、data、terminator 與 trailers |
| 9 | Binary Data | 將前八步組成一條 byte-safe HTTP connection 流程 |

## 三語言怎麼對照

| 概念 | C | Go | Rust |
| --- | --- | --- | --- |
| CRLF stream line | `c/line_reader.*` | `go/internal/httpstream/line_reader.go` | `rust/src/line_reader.rs` |
| TCP lifecycle / binary echo | `c/tcp_server.*`、`c/http_echo_server.*` | `go/internal/tcpserver/server.go` | `rust/src/tcp_server.rs` |
| Request parsing | `c/request_parser.*`、`c/request_line.*` | `go/internal/httprequest/request*.go` | `rust/src/request.rs`、`rust/src/request_line.rs` |
| Headers / body / chunked | `c/*_parser.*` | `go/internal/httprequest/{header,body,chunked}.go` | `rust/src/{header,body,chunked}.rs` |
| Response bytes | `c/http_response.*` | `go/internal/httpresponse/response.go` | `rust/src/response.rs` |

[Rust Module Map](docs/rust-module-map.md) 提供更完整的 Rust 檔案與 C／Go 對照。

## Step 9：把前八步接成一條 HTTP connection

Step 9 的 one-request `POST /echo HTTP/1.1` server 會選擇唯一合法的
`Content-Length` 或 course-defined `Transfer-Encoding: chunked` framing，並以
`image/bmp` 回傳完全相同的 bytes。它拒絕兩種 framing 同時出現的請求，也不需要
客戶端先送 EOF 才回應。

TCP 從 Step 1 開始傳遞的就是 bytes；差別在 HTTP/1.1 的 request line 與 headers
以 delimiter 解析，body 則必須依長度或 chunked 結構定界。binary body 可以包含
`\r\n`，所以不能交給逐行 parser。把這個整合放在最後，能清楚看見 HTTP/1.1
「文字訊息頭 + 不透明 body」交界的複雜性，也為理解 HTTP/2、HTTP/3 的 binary
frames、多路複用與 header compression 鋪路；後兩者不在本專案範圍。

## 驗證

在 Ubuntu on WSL、repository root 執行：

```bash
make verify
```

它會執行 C strict warnings 與 sanitizers、Go vet/tests、Rust format/clippy/tests，
以及 shared fixtures 與 TCP integrations。

## 範圍、文件與版本

- 僅使用語言標準函式庫與作業系統 socket API；C 以 WSL Ubuntu 的 POSIX sockets
  為準。
- 不實作 keep-alive、routing、TLS、HTTP/2、HTTP/3、完整 RFC conformance 或任何
  production hardening。
- [開發環境](docs/development-setup.md) · [開發流程](docs/development-workflow.md) · [最終驗收矩陣](docs/course-completion.md)
- 每個 `step-*-v*` tag 是已驗證的 milestone 快照；最終累積版本會在 `main`。
