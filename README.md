# 第 9 步：二進位資料

這個里程碑把前八步的元件接成一條只處理單一連線的 HTTP/1.1 伺服器流程。
它不是路由框架，也不是可供正式環境使用的伺服器；目標是理解同一條 TCP 位元組
串流中，文字格式的訊息頭與不透明的二進位資料本體如何正確分界。

## 已完成的行為

伺服器接受一個 `POST /echo HTTP/1.1` 請求，依據標頭選擇唯一合法的資料本體定界
方式：

```text
TCP 讀取資料
  -> 使用嚴格 CRLF 解析請求訊息頭
  -> 解析請求列與標頭
  -> Content-Length 資料本體，或 Transfer-Encoding: chunked 資料本體
  -> 回傳帶有完全相同解碼位元組的 200 image/bmp 回應
```

C、Go、Rust 都會執行三個 TCP 黑箱驗收案例：

1. `Content-Length` 二進位資料本體：回應本體與請求本體完全相同。
2. `Transfer-Encoding: chunked` 二進位資料本體：解碼後回應完全相同。
3. 同時提供 `Content-Length` 和 `Transfer-Encoding: chunked`：回傳
   `400 Bad Request`，且不回傳請求資料本體。

每個請求都拆成多次 TCP 寫入；用戶端在收到回應前不關閉寫入端。因此通過測試代表
伺服器是在資料本體定界完成時回應，而不是依賴 TCP EOF 或剛好一次讀完整個訊息。

## 二進位測試資料

[`testdata/binary/echo-bmp/image.bmp`](testdata/binary/echo-bmp/image.bmp) 是一個
58-byte 的真實 1×1 BMP 圖片，其中包含 NUL、LF 和非 ASCII 位元組。驗收請求會再
加上原始 `\r\n` 作為資料本體後綴。這刻意讓二進位資料本體含有 HTTP 文字訊息頭
也使用的分隔位元組，證明資料本體不能交給逐行或標頭解析器處理。

## 為什麼把二進位資料放在最後

TCP 從第 1 步開始就只傳遞位元組；第 9 步不是才開始「支援位元組」。差別在
HTTP/1.1 的混合定界方式：請求列與標頭是依 CRLF 和分隔符解析的文字中繼資料，
資料本體則必須依 `Content-Length` 或 chunked 結構，以長度和狀態機解析，並保留
原始位元組。

前面的步驟分別把串流、請求列、標頭、固定長度資料本體、回應與 chunked 解碼做成
可單獨驗證的元件。這一步才將它們放進同一個 socket 讀取迴圈，使訊息頭／資料本體
邊界與模糊的定界方式成為可觀察的整合行為。

HTTP/2 與 HTTP/3 都以二進位框架表示 HTTP 訊息；這減少 HTTP/1.1「文字訊息頭加上
二進位資料本體」交界的解析負擔，並帶來多路複用、標頭壓縮等能力。它們不在本專案
範圍內；第 9 步的目的只是建立理解這個設計演進所需的位元組定界基礎。

## 驗證

請在 Ubuntu on WSL 的 repository root 執行：

```bash
make verify
```

它會執行 C 的嚴格警告檢查、原生測試、ASan/UBSan，Go 的 `gofmt`、`go vet` 與測試，
以及 Rust 的格式檢查、`clippy` 與測試。

## 函式對照

| 責任 | C | Go | Rust |
| --- | --- | --- | --- |
| 單一請求的連線處理迴圈 | `http_echo_server_serve_once` | `serveEchoOnce` | `serve_echo_once` |
| 資料本體定界方式選擇 | `select_framing` | `selectRequestFraming` | `select_echo_framing` |
| 固定長度資料本體 | `body_parser` | `BodyParser` | `BodyParser` |
| chunked 資料本體 | `chunked_parser` | `ChunkedParser` | `ChunkedParser` |
| 回應位元組序列化 | `http_response_build` | `httpresponse.New` | `Response::bytes` |

C 以檔案描述符、`unsigned char *` 和明確的 `free` 表達資源責任；Go 以 slice、`error`
和 goroutine 管理 listener；Rust 則由 `Vec<u8>` 的所有權、`Result` 和 `drop` 負責
資源釋放。三者的可觀察行為相同，但不追求逐行翻譯。

## 刻意未做

- 多請求 keep-alive、路由、檔案 I/O、MIME 判斷或 range request
- chunk extension、transfer-coding 組合、完整 RFC 標頭與定界規則
- TLS、HTTP/2、HTTP/3 或正式環境的防護措施

## 導覽

- 前一里程碑：[第 8 步：Chunked Encoding](https://github.com/zrkluke/raw-http-server/tree/step-8-chunked)
- 完整專案入口：[main](https://github.com/zrkluke/raw-http-server/tree/main)
- 環境與流程：[development setup](docs/development-setup.md) · [development workflow](docs/development-workflow.md)
