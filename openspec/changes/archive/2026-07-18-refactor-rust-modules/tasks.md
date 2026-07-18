## 1. Refactor Baseline

- [x] 1.1 Record the existing Rust public exports and run the native Rust test suite as the behavior-preservation baseline
- [x] 1.2 Add the responsibility-focused Rust module declarations and re-export boundary in `rust/src/lib.rs`

## 2. Responsibility Modules

- [x] 2.1 Move line reader, request-line, header, body, chunked, and request parser responsibilities with their unit tests into focused modules
- [x] 2.2 Move response and TCP server responsibilities, including the binary echo composition, with their unit tests into focused modules
- [x] 2.3 Keep black-box acceptance tests under `rust/tests/` and make only the import changes required by the preserved public API

## 3. Verification and Documentation

- [x] 3.1 Run Rust formatting, tests, and lints; fix only structural issues introduced by the move
- [x] 3.2 Run the complete WSL unified verification suite and confirm existing milestone tags remain unchanged
- [x] 3.3 Document the final Rust module-to-responsibility mapping for the later course-completion learning map
