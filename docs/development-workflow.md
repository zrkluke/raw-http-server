# 開發與里程碑工作流程

本專案以 OpenSpec 定義需求與驗收條件，並以小步、可重現的
Red-Green-Refactor 循環實作。所有 milestone 都是累積式：新功能不得破壞
先前已通過的行為。

## 每個 milestone 的固定順序

1. 依 OpenSpec 建立 shared acceptance fixtures 或 black-box acceptance test。
2. 執行測試，確認它因目標行為尚未實作而失敗（Red）。
3. 以 Go native test 完成最小實作，再在綠燈下重構。
4. 依相同行為契約，以 C native test 完成等價實作與重構。
5. 依相同行為契約，以 Rust native test 完成等價實作與重構。
6. 在 WSL 執行 `make verify`，重跑三語言與所有既有回歸測試。
7. 記錄 function-to-function 對照、語言慣用差異及刻意簡化。
8. 通過 Definition of Done 後，才建立該 milestone 的 immutable tag。

Go 是第一份參考實作，但 C 與 Rust 必須保留各自的型別、錯誤處理、
ownership 與 resource-management 慣例，不進行逐行翻譯。

## Red-Green-Refactor

### Red

- milestone 實作前，先加入能描述 observable behavior 的失敗驗收案例。
- function 新增行為前，先以該語言的 native test 重現尚未支援的案例。
- 修正 defect 前，先加入能在錯誤版本上失敗的 regression test。
- 失敗必須來自缺少或錯誤的目標行為，不可由測試語法錯誤、環境錯誤或
  無關的既有失敗冒充。

### Green

- 只加入讓目前失敗案例通過的最小實作。
- 不在同一步驟偷偷加入下一個 milestone 的 abstraction 或功能。
- 每種語言通過自己的 unit tests 後，仍須執行 shared fixtures。

### Refactor

- 只在相關測試維持綠燈時整理命名、結構與重複程式碼。
- 重構不可改變已定義的 observable behavior。
- 三種語言比較的是 function responsibility，不要求 signature 或內部表示
  完全相同。

## 累積回歸規則

每次修改實作、測試或建置規則後，在 WSL repository 根目錄執行：

```bash
make verify
```

目前統一 gate 包含：

- C：嚴格 compiler warnings、native tests、AddressSanitizer 與
  UndefinedBehaviorSanitizer。
- Go：`gofmt`、`go vet` 與 `go test`。
- Rust：`cargo fmt`、`cargo clippy` 與 `cargo test`。

任一當前或先前 milestone 的測試失敗時，當前 milestone 不得宣告完成。
如果後續發現早期設計錯誤，先新增 regression test，再修正實作；不得只
修改程式碼而沒有可重現的保護案例。

## Definition of Done

milestone 只有在下列條件全部成立時才算完成：

- OpenSpec 對應 requirements 與 scenarios 已由自動測試或明確的人工檢查
  覆蓋。
- C、Go、Rust 都實作相同責任與 observable behavior。
- 三語言 native unit tests 全部通過。
- shared byte fixtures 與適用的 TCP integration tests 全部通過。
- incremental parsing 已驗證完整輸入、逐 byte、CRLF 邊界、欄位邊界及
  固定大小 chunks。
- `make verify` 在 WSL 成功，且所有先前 milestone 的測試仍然通過。
- C sanitizer、Go static checks、Rust format／Clippy 等品質 gate 通過。
- function-to-function 比較、刻意差異與 protocol 簡化已有文件記錄。
- OpenSpec task checkbox 已反映實際狀態。
- 以上條件確認後，才可建立 immutable milestone tag。

## Branch 規則

每個 step 使用累積式工作分支：

```text
step-0-foundation
step-1-http-streams
step-2-tcp
step-3-requests
step-4-request-lines
step-5-http-headers
step-6-http-body
step-7-http-responses
step-8-chunked-encoding
step-9-binary-data
```

- 新 step 從前一個已驗證的 milestone commit 或 tag 建立。
- branch 是開發與 GitHub 瀏覽入口，可以增加 commit，因此不是文章的
  canonical reference。
- 未通過 Definition of Done 前，不把 branch 宣告為完成狀態。
- `main` 最終保存完整的累積實作；何時整合 branch 仍需使用者明確同意。
- 不 force-push 或改寫已分享的 milestone 歷史。

## Immutable tag 規則

完成後建立 annotated tag，格式為：

```text
step-<編號>-<主題>-v<修訂編號>
```

例如：

```text
step-0-foundation-v1
step-1-http-streams-v1
step-2-tcp-v1
```

- tag 必須指向通過 Definition of Done 的確切 commit。
- tag 是教學文章與完成狀態的 canonical reference。
- 已發布的 tag 不得移動、覆寫、刪除或重複使用名稱。
- 若完成後發現 defect，保留原 tag；加入 regression test、修正並完整驗收
  後，以遞增修訂建立新 tag，例如 `step-1-http-streams-v2`。
- 建立或 push branch／tag 前，仍須取得使用者明確同意。

範例命令僅供說明，不會由 agent 自動執行：

```bash
git tag -a step-1-http-streams-v1 -m "Milestone 1: HTTP Streams"
git push origin step-1-http-streams-v1
```
