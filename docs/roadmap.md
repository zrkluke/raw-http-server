# 後續探索路線圖

Steps 1–9 已完成，`main` 是目前完整的課程成品：從 TCP byte stream、
HTTP/1.1 request line、headers、body framing、chunked encoding，一直到可
回傳 binary body 的單一 request server。

本文件列出可延伸的學習方向；它們**不是目前已實作的功能，也不是已承諾
的開發排程**。選定其中一項時，先建立獨立的 OpenSpec change、branch 與
acceptance cases，再開始實作。

## 建議順序

| 順序 | 主題 | 為什麼值得做 |
| --- | --- | --- |
| 1 | HTTP/1.1 persistent connections | 將目前「一條 connection、一個 request」延伸為真正的 connection lifecycle。 |
| 2 | Handler boundary 與小型 routing | 理解 HTTP server 如何把已解析的 request 交給 application；這是 WSGI／ASGI 所在的層次。 |
| 3 | Static file serving | 處理路徑、binary response、Content-Type 與安全邊界。 |
| 4 | Reverse proxy | 理解 Nginx 類 component 如何把 downstream request 轉送給 upstream。 |
| 5 | Concurrency-model comparison | 比較 C、Go、Rust 如何處理多條連線與資源生命週期。 |
| 6 | HTTP/2 frame experiment | 以獨立小實驗理解 text-based HTTP/1.1 與 binary frames 的差異。 |

## 1. HTTP/1.1 persistent connections

這是最適合的第一個 extension，因為它不增加新的 application feature，而是
直接驗證目前課程最核心的問題：**message boundary 和 connection lifetime
不是同一件事。**

目前 Step 9 的路徑是：

```text
一條 TCP connection → 一個 POST /echo → response → close
```

延伸後會變成：

```text
一條 TCP connection
  → request 1 → response 1
  → request 2 → response 2
  → ...直到 Connection: close 或協議錯誤
```

### 這一步會迫使你理解什麼

- `Content-Length` body 必須剛好讀完，不能吃到下一個 request 的開頭。
- 一次 socket read 可能同時帶回第一個 request 的尾端與下一個 request 的
  開頭；parser 必須保留尚未消費的 bytes。
- chunked terminator 後面可能立刻就是下一個 request，而不是 TCP EOF。
- 遇到 malformed request 時，要決定回傳錯誤後是否關閉 connection。
- `Connection: close` 是 connection-level 的決定，不是 message framing 的
  替代品。

### 建議的驗收順序

1. 同一連線先完成一個 `Content-Length` request 與 response，再送第二個
   request。
2. 在同一次 TCP write 送出兩個完整 requests，確認 server 不會丟失第二個
   request 的前綴 bytes。
3. 讓第一個 request 使用 chunked body，第二個使用 `Content-Length`。
4. 確認 server 在收到完整 message 後回應，不等待 client 關閉寫入端。
5. 測試 `Connection: close` 與 malformed request 的關閉規則。
6. 在 sequential requests 穩定後，才另行評估 HTTP/1.1 pipelining。

先做 sequential requests，不直接做 pipelining。前者已足以暴露 buffer
ownership、leftover bytes 與 framing 問題；後者還會加入 response ordering
與背壓等額外複雜度。

## 2. Handler boundary 與小型 routing

把固定的 `/echo` handler 拆成明確邊界：

```text
HTTP parser → Request → handler / router → Response → HTTP writer
```

這會讓你看見 WSGI／ASGI 位於哪裡：它們不是 HTTP parser，而是 server 與
application 之間的交接契約。

三語言可保留慣用設計，而不逐行翻譯：C 使用 function pointer 與 context
pointer，Go 使用 function type 或 interface，Rust 使用 trait 或 closure。

## 3. Static file serving

Static file server 是很好的 binary data 延伸：response body 不再只來自 echo，
而是從檔案讀取。重點不只是讀檔，還包括 URL path 與 filesystem path 的邊界、
path traversal 防護、Content-Type 與錯誤 response。

## 4. Reverse proxy

Reverse proxy 讓 server 成為兩條 connection 之間的中介：接收 downstream
request、連到 upstream、轉送 response。這會讓你更具體理解 Nginx 類元件，
但也會引入 upstream failure、timeout、header policy 與 backpressure，因此應在
connection lifecycle 穩定後另開 scope。

## 5. Concurrency-model comparison

這一項適合在單 connection 行為已穩定後進行。重點是比較資源管理與併發模型，
不是追求吞吐量競賽：C 的 `poll`／`epoll`，Go 的 goroutine，以及 Rust 的
標準執行緒或另行明確選定的 runtime 策略，都需要分開設計與驗收。

## 6. HTTP/2 frame experiment

不要把 HTTP/2 直接塞進目前的 HTTP/1.1 server。較好的做法是另建小型 frame
parser：讀取固定長度 frame header 與 payload，對照目前 HTTP/1.1 以 CRLF 與
body framing 找邊界的作法。這能把 Step 9 的 binary-data 脈絡自然延伸到
binary-based protocol。

## 從構想走到實作

選定一個題目後，建立新的 OpenSpec change，明確寫下：

1. 仍沿用哪些既有 HTTP 行為與 fixtures。
2. 新的 acceptance cases 與 failure modes。
3. C、Go、Rust 各自負責的對應行為與刻意保留的語言差異。
4. WSL 的完整驗收命令，以及是否需要新的 immutable extension tag。

這樣 roadmap 保持為學習地圖，而每個真正的 extension 都維持可驗收、可 review、
可回溯的獨立變更。
