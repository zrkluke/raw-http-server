## Why

目前課程的 Steps 1–9 與收尾驗收已完成，但後續可延伸的學習方向只存在於討論中。需要一份可從 README 找到的 roadmap，讓讀者理解哪些主題延續既有的 TCP／HTTP 學習目標，以及何時應將構想提升為獨立的實作變更。

## What Changes

- 新增 `docs/roadmap.md`，記錄完成課程後的候選延伸方向與建議順序。
- 說明 persistent connections 是最優先的實作延伸，並列出其驗收方向。
- 將其他方向限制為規劃說明，不在本 change 新增 HTTP 功能、路由、代理或並發實作。
- 從 README 提供簡短入口，並說明開始實作前須建立獨立 OpenSpec change。

## Capabilities

### New Capabilities

- `future-extension-roadmap`: 定義課程完成後 roadmap 的內容、範圍與升級為正式 change 的規則。

### Modified Capabilities

<!-- No existing capability requirements are modified. -->

## Impact

- 影響 `README.md` 與新增的 `docs/roadmap.md`。
- 新增文件用 OpenSpec change 與其主規格；不影響 C、Go、Rust 實作、測試、tags 或既有 milestone branches。
