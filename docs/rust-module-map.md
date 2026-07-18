# Rust Module Map

This map supports the C, Go, and Rust function-to-function comparison. The
Rust module refactor preserves the existing public exports and HTTP behavior.

## Pre-refactor public export baseline

| Public type | Responsibility | Planned module |
| --- | --- | --- |
| `LineReader` | CRLF-delimited TCP stream lines | `line_reader` |
| `RequestLine`, `RequestLineError` | HTTP/1.1 request-line validation | `request_line` |
| `RequestState`, `RequestParser` | Incremental request-head boundary | `request` |
| `Header`, `HeaderState`, `HeaderParser` | Incremental HTTP header parsing | `header` |
| `BodyState`, `BodyParser` | `Content-Length` body handling | `body` |
| `ChunkedState`, `ChunkedParser` | Chunked transfer decoding | `chunked` |
| `Response` | Byte-oriented HTTP response serialization | `response` |
| `TcpServer` | One-shot TCP transfer, response, and binary echo composition | `tcp_server` |

The existing public constructors and methods remain available through the
crate root after the refactor. Black-box acceptance tests continue to import
`raw_http_server_rust::TcpServer`.

## Final module mapping

| Rust file | Course responsibility | Comparable C / Go area |
| --- | --- | --- |
| `rust/src/line_reader.rs` | CRLF stream lines | `c/line_reader.*` / `httpstream/line_reader.go` |
| `rust/src/request_line.rs` | Request-line parsing | `c/request_line.*` / `httprequest/request_line.go` |
| `rust/src/header.rs` | Header parsing | `c/header_parser.*` / `httprequest/header.go` |
| `rust/src/body.rs` | `Content-Length` bodies | `c/body_parser.*` / `httprequest/body.go` |
| `rust/src/chunked.rs` | Chunked decoding | `c/chunked_parser.*` / `httprequest/chunked.go` |
| `rust/src/request.rs` | Request-head boundary | `c/request_parser.*` / `httprequest/request.go` |
| `rust/src/response.rs` | Response serialization | `c/http_response.*` / `httpresponse/response.go` |
| `rust/src/tcp_server.rs` | TCP lifecycle and binary echo composition | `c/tcp_server.*`, `c/http_echo_server.*` / `tcpserver/server.go` |
| `rust/src/lib.rs` | Public re-export and module navigation | C headers / Go package exports |

Each module retains its focused unit tests. Shared fixture and black-box TCP
acceptance tests remain under `rust/tests/`.
