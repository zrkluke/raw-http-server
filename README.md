# Step 0：Repository and Validation Foundation

這個分支建立 C、Go、Rust 三種實作共用的開發與驗證基礎，尚未開始
HTTP Server 功能。本階段的重點是先讓後續九個 step 都能使用相同的測試
資料、品質門檻與完成標準。

完整的專案目標、實作界線與九個里程碑請回到
[main 分支](https://github.com/zrkluke/raw-http-server/tree/main) 閱讀。

## 本階段完成內容

- 建立 C、Go、Rust 的可編譯專案骨架。
- 提供根目錄 `make verify`，統一執行三種語言的驗證。
- 建立三種語言共用的 byte-exact fixture 格式與載入器。
- 為 C 啟用嚴格 compiler warnings、AddressSanitizer 與
  UndefinedBehaviorSanitizer。
- 為 Go 啟用 `gofmt`、`go vet` 與 `go test`。
- 為 Rust 啟用 `cargo fmt`、Clippy 與 `cargo test`。
- 記錄 Red-Green-Refactor、累積回歸、Definition of Done、branch 與
  immutable tag 規則。

## 為什麼先做 Foundation

後續每個 step 都會對 C、Go、Rust 實作相同責任。如果沒有共同的 fixture
與驗證入口，三份實作很容易在不知不覺間採用不同的輸入、預期結果或品質
標準。Step 0 先固定比較基準，讓之後發現錯誤時能用累積回歸測試定位。

## 驗收方式

請在 WSL 的 repository 根目錄執行：

```bash
make verify
```

驗收成功時，C、Go、Rust 的 build、native tests、格式、lint 與適用的
sanitizer 都會通過，指令最後以成功狀態回到 prompt。

## 重要檔案

| 檔案 | 本階段責任 |
| --- | --- |
| [`Makefile`](Makefile) | 統一呼叫 C、Go、Rust 的驗證命令 |
| [`c/Makefile`](c/Makefile) | C 的嚴格編譯、測試與 sanitizer targets |
| [`c/tests/fixture.c`](c/tests/fixture.c) | 以 C 載入共用 binary fixtures |
| [`go/internal/testfixture/fixture.go`](go/internal/testfixture/fixture.go) | 以 Go 載入共用 binary fixtures |
| [`rust/tests/support/mod.rs`](rust/tests/support/mod.rs) | 以 Rust 載入共用 binary fixtures |
| [`testdata/README.md`](testdata/README.md) | 定義跨語言 fixture 結構與 byte-exact 規則 |
| [`docs/development-setup.md`](docs/development-setup.md) | WSL 與三種語言工具鏈安裝方式 |
| [`docs/development-workflow.md`](docs/development-workflow.md) | TDD、DoD、branch 與 immutable tag 工作流程 |
| [`openspec/changes/build-three-language-http-course`](openspec/changes/build-three-language-http-course) | 專案需求、設計、驗收規格與任務 |

## 本階段刻意不做

- 不監聽 TCP port，也不接受連線。
- 不解析 CRLF、request line、headers 或 body。
- 不產生 HTTP response。
- 不使用高階 HTTP 函式庫、框架或第三方 runtime；這項界線適用於所有
  後續 step。

TCP 與 HTTP 功能會從 `step-1-http-streams` 開始依序以 TDD 實作。

## 版本導覽

- 前一狀態：`main` 的三語言空專案骨架。
- 目前分支：`step-0-foundation`。
- 第一版固定快照：
  [`step-0-foundation-v1`](https://github.com/zrkluke/raw-http-server/tree/step-0-foundation-v1)。
- 本次 README 修訂驗收後發布：`step-0-foundation-v2`。
- 下一步：`step-1-http-streams`。
