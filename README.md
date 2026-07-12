# Step 2：TCP Listener

這個分支在 Step 1 的 CRLF byte-stream 處理之上，讓 C、Go、Rust
各自建立真正的本機 TCP listener：監聽動態 port、接受一個 client、讀取
client 關閉前傳來的 bytes，然後釋放 client connection 與 listener。

這一步仍然不是 HTTP parser 或完整 HTTP server。它只證明：程式能從真正的
TCP connection 取得原始 bytes；下一個 milestone 才會在這條連線上建立
incremental request parsing。

## 本步完成的行為

共享 fixture [`testdata/tcp/listen-connect/`](testdata/tcp/listen-connect)
定義 client 傳送的 `ping\n` bytes。三種語言的 acceptance tests 都會：

1. 在 `127.0.0.1:0` 建立 listener，讓作業系統配置可用 port。
2. 用真正的 TCP client 連線並傳送 shared fixture。
3. 確認 server 收到的 bytes 與 `expected.bin` 完全一致。
4. 確認一次性 server 完成後，不再接受新的 client。

## 驗收方式

在 WSL 的 repository 根目錄執行：

```bash
make verify
```

這會累積驗證 Step 1 與本步：

- C：strict warnings、POSIX TCP unit／acceptance tests、AddressSanitizer 與
  UndefinedBehaviorSanitizer。
- Go：`gofmt`、`go vet`、unit tests 與 TCP integration test。
- Rust：`cargo fmt --check`、Clippy、unit tests 與 TCP integration test。

## Function 對照

| 責任 | C | Go | Rust |
| --- | --- | --- | --- |
| 建立 listener | [`tcp_server_open`](c/tcp_server.c) | [`StartOnce`](go/internal/tcpserver/server.go) | [`TcpServer::start_once`](rust/src/lib.rs) |
| 取得動態位址 | [`tcp_server_port`](c/tcp_server.c) | `Server.Addr` | [`TcpServer::address`](rust/src/lib.rs) |
| 接受並讀取一次 | [`tcp_server_accept_once`](c/tcp_server.c) | `serveOnce` | `serve_once` |
| 交回收到的 bytes | `unsigned char **received` | `Server.Received` channel | `TcpServer::receive_timeout` |
| 結束與釋放資源 | [`tcp_server_close`](c/tcp_server.c) | `defer connection.Close()`／`defer listener.Close()` | `Drop` 與 `TcpServer::wait_timeout` |

三種版本的責任相同，但資源模型刻意不同：C 顯式管理 fd 與 heap buffer；Go
以 goroutine 和 channel 回傳結果；Rust 以 thread、channel、`Vec<u8>` 與
Drop 表達資料所有權和資源釋放。

## 重要檔案

| 檔案 | 用途 |
| --- | --- |
| [`testdata/tcp/listen-connect/`](testdata/tcp/listen-connect) | 三種語言共用的 byte-exact TCP fixture |
| [`go/internal/tcpserver/`](go/internal/tcpserver) | Go 的 listener、unit test 與 integration test |
| [`c/tcp_server.c`](c/tcp_server.c) | C 的 POSIX `socket`、`bind`、`listen`、`accept`、`read` 路徑 |
| [`c/tests/tcp_server_acceptance_test.c`](c/tests/tcp_server_acceptance_test.c) | C 的真實 TCP acceptance test；以 `fork()` 建立 client |
| [`rust/src/lib.rs`](rust/src/lib.rs) | Rust 的 `TcpServer`、native unit tests 與 Step 1 `LineReader` |
| [`rust/tests/tcp_server_acceptance.rs`](rust/tests/tcp_server_acceptance.rs) | Rust 的真實 TCP integration test |

## 本步刻意不做

- HTTP request line、headers、body、response 或 routing。
- 多 client、keep-alive、timeout policy、TLS 或 production hardening。
- `net/http`、HTTP parser、web framework、async runtime 或第三方實作依賴。

## 版本導覽

- 前一個 immutable snapshot：[Step 1 · HTTP Streams](https://github.com/zrkluke/raw-http-server/tree/step-1-http-streams-v1)
- 本步 immutable snapshot：[Step 2 · TCP](https://github.com/zrkluke/raw-http-server/tree/step-2-tcp-v1)
- 專案總覽與完整學習地圖：[main README](https://github.com/zrkluke/raw-http-server/tree/main)
- 開發環境與流程：[WSL setup](docs/development-setup.md) · [development workflow](docs/development-workflow.md)
