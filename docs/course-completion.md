# Course Completion Validation

This document is the final traceability record for the three-language HTTP
learning course. Run `make verify` from Ubuntu on WSL for the automated rows.

## Requirement and scenario matrix

| Requirement / scenario | Evidence | Check type |
| --- | --- | --- |
| Acceptance tests precede implementation; begin a milestone | The numbered OpenSpec tasks record each acceptance case before its Go, C, and Rust implementation tasks. | Manual: inspect `openspec/changes/build-three-language-http-course/tasks.md` and Git history. |
| Language-native Red-Green-Refactor | Native C, Go, and Rust unit tests live beside their implementations; the task history records the reference-first sequence. | Manual: inspect task order and language-native tests. |
| Shared cross-language contract | `testdata/` fixtures are loaded by C `fixture-test`, Go `internal/testfixture`, and Rust `tests/fixture_loader.rs`. | Automated by `make verify`. |
| Fragmentation coverage | Stream, request-line, header, body, chunked, and binary acceptance suites use whole, byte, CRLF, field, or fixed partitions. | Automated by the corresponding C, Go, and Rust acceptance tests. |
| Cumulative regression gate and milestone Definition of Done | Root `make verify` runs strict C tests/sanitizers, Go vet/tests, and Rust fmt/clippy/tests. | Automated by `make verify`. |
| Defect regression test | The TCP listener cleanup regression is represented by Go `TestStartOnceTransfersBytesAndCleansUp` and equivalent C/Rust TCP acceptance cases. | Automated by `make verify`. |
| Immutable milestone reference | `git show-ref --tags` resolves every `step-*-v*` ref; each tag was made only after its WSL verification. | Manual: run `git show-ref --tags`. |
| Milestone-focused branch documentation | Each completed `step-*` branch has its focused README snapshot; `main` is reserved for the final overview. | Manual: inspect branch README files on GitHub or with `git show <branch>:README.md`. |
| Common implementation boundary | C uses POSIX sockets, Go uses `net`/standard I/O, Rust uses `std::net`; no HTTP framework or third-party runtime is declared. | Manual: inspect `c/`, `go/go.mod`, and `rust/Cargo.toml`. |
| Function-level comparability | [Rust Module Map](rust-module-map.md) identifies C, Go, and Rust responsibility counterparts. | Manual documentation check. |
| Milestone 1: CRLF stream | Shared `testdata/streams`; C `line_reader_*`, Go `httpstream`, Rust `line_reader_acceptance.rs`. | Automated by `make verify`. |
| Milestone 2: TCP lifecycle | C `tcp_server_*`, Go `tcpserver`, Rust `tcp_server_acceptance.rs`. | Automated by `make verify`. |
| Milestone 3: request states | Shared request fixtures; C `request_parser_*`, Go `httprequest/request_*`, Rust `request_parser_acceptance.rs`. | Automated by `make verify`. |
| Milestone 4: request lines | Shared request-line fixtures; C `request_line_*`, Go `httprequest/request_line_*`, Rust `request_line_acceptance.rs`. | Automated by `make verify`. |
| Milestone 5: headers | Shared header fixtures; C `header_parser_*`, Go `httprequest/header_*`, Rust `header_parser_acceptance.rs`. | Automated by `make verify`. |
| Milestone 6: Content-Length body | Shared body fixtures; C `body_parser_*`, Go `httprequest/body_*`, Rust `body_parser_acceptance.rs`. | Automated by `make verify`. |
| Milestone 7: responses | Byte-exact response fixtures plus C/Go/Rust TCP response acceptance tests. | Automated by `make verify`. |
| Milestone 8: chunked decoding | Shared chunked fixtures; C `chunked_*`, Go `httprequest/chunked_*`, Rust `chunked_acceptance.rs`. | Automated by `make verify`. |
| Milestone 9: binary integration | Real BMP fixture; C `binary_acceptance_test.c`, Go `binary_echo_acceptance_test.go`, Rust `binary_acceptance.rs` cover fragmented Content-Length, chunked, and ambiguous framing. | Automated by `make verify`. |

## Deliberate manual boundaries

The course does not claim production RFC conformance. The documented manual
checks above therefore verify process and repository structure, while protocol
behavior is covered by automated native and black-box TCP tests.
