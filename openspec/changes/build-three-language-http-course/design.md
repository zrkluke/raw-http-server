## Context

The repository is initially empty except for OpenSpec configuration. The project is a learning exercise based on the nine chapters of ThePrimeagen/Boot.dev's "From TCP to HTTP" course, expanded from Go to C, Go, and Rust. Its primary stakeholders are the learner and future article readers; deployability and complete RFC conformance are not goals.

The C implementation will run in WSL and use POSIX sockets. Go and Rust may compile natively, but WSL is the common environment for cross-language verification. Implementations may use standard libraries, but may not delegate HTTP parsing or serving to a high-level library.

## Goals / Non-Goals

**Goals:**

- Teach TCP streams and the relevant HTTP/1.1 mechanisms by implementing them.
- Make equivalent responsibilities comparable function by function across three languages.
- Preserve idiomatic error, type, memory, and resource-management patterns in each language.
- Detect incorrect early assumptions through test-first contracts and cumulative regression tests.
- Preserve each completed chapter as an immutable, reproducible learning snapshot.

**Non-Goals:**

- Production readiness, security hardening, broad RFC conformance, or performance parity with established servers.
- Line-by-line translations between languages.
- Windows Winsock support in C.
- HTTP/2, HTTP/3, TLS, routing frameworks, or third-party implementation dependencies.

## Decisions

### Nine cumulative milestones

Development follows the course order: Streams, TCP, Requests, Request Lines, Headers, Body, Responses, Chunked Encoding, and Binary Data. Each milestone extends the previous one and retains all prior tests. This keeps the learning sequence recognizable and makes regressions visible.

Alternative considered: four broad milestones. Rejected because parser, body, and transfer-encoding mistakes would be discovered too late and the snapshots would be less useful for comparison.

### Binary Data as the connection-level integration milestone

The byte model begins in Milestone 1; every TCP read has always been bytes.
Milestone 9 does not introduce a second kind of transport data.  It completes
the HTTP/1.1 boundary that earlier milestones deliberately isolated: request
line and headers are delimiter-driven text, while the message body is opaque
octets whose boundary is selected by message framing.

The completed learning server handles one connection and one
`POST /echo HTTP/1.1` request.  It incrementally reads from the socket, validates
the request line and headers, selects either a validated `Content-Length` body
or the course-defined `Transfer-Encoding: chunked` body, and writes a
`200 OK` `image/bmp` response containing exactly the decoded request bytes.
It closes the connection after that response.  This is intentionally a fixed
handler, not a routing framework or a reusable production server.

The shared fixture is a real BMP containing NUL, LF, and non-text octets.  The
acceptance payload appends a raw `CRLF` byte pair after that image, so the body
also contains bytes meaningful to a header parser.  Acceptance clients split
both headers and body across writes and wait for the response without first
closing their write side.  Therefore success demonstrates that the server
stops at the declared message boundary, rather than treating EOF or a text
delimiter inside the body as the boundary.

Milestone 8 intentionally taught chunk decoding in isolation.  Milestone 9
adds the narrowly-scoped header-to-framing selection needed to compose it:
exactly one `Transfer-Encoding: chunked` is supported; it is mutually exclusive
with `Content-Length`; other transfer codings remain out of scope.  The
ambiguous combination is rejected before any body is echoed.

Alternative considered: return a preloaded binary response after any non-empty
read.  Rejected because it bypasses request parsing and cannot establish the
text/body boundary that Binary Data is meant to teach.

### Shared behavior, idiomatic implementations

Equivalent functions SHALL have the same responsibility and observable behavior, while signatures and internal representations follow language conventions. C will expose explicit buffers and ownership; Go will use slices and `error`; Rust will use borrowing, enums, `Result`, and RAII where appropriate.

Alternative considered: direct transliteration. Rejected because it would obscure the language characteristics that the project is intended to teach.

### Abstraction boundary

C may use POSIX and the C standard library, Go may use its standard library except `net/http`, and Rust may use `std`. Implementation and tests default to the standard toolchains and libraries. No implementation may use an HTTP server, parser, Web framework, async runtime, or other third-party implementation dependency. A third-party development tool for test infrastructure requires explicit approval and must not become an HTTP implementation dependency. Socket and byte-stream APIs are allowed because the learning boundary begins above TCP transport.

### Three-layer validation

Before implementation of a milestone, shared acceptance fixtures define observable behavior. Each language then uses unit-level Red-Green-Refactor cycles. Once networking is available, black-box TCP integration tests send identical input to all implementations. A milestone passes only when its tests and every earlier test pass for all languages.

### Reference-first implementation order

Within each milestone, implementation proceeds from the failing acceptance contract to Go, then C, then Rust, followed by unified verification and comparison documentation. Go provides the first readable reference implementation while the protocol behavior is still being learned. C then exposes explicit buffers, pointers, ownership, and cleanup for the same known responsibility. Rust completes the comparison by expressing that responsibility with ownership, borrowing, enums, `Result`, and RAII.

This order is a learning workflow, not a claim that one language is generally easier or more important. Each implementation still uses native unit-level Red-Green-Refactor and must remain idiomatic rather than becoming a transliteration of the Go version.

### Shared fixtures, language-native harnesses

Protocol bytes and expected results live in a language-neutral `testdata/` area. Each language uses its native test tooling to consume them. A repository-level WSL command orchestrates all builds, tests, and integration checks without becoming an HTTP implementation dependency.

### Git snapshots

Milestones are cumulative. A milestone ref is created only after its Definition of Done passes. Immutable tags are the canonical article references; optional branches may remain as convenient browsing pointers. The final `main` state contains the completed series.

### Milestone-focused README snapshots

The `main` README owns the stable project overview, learning goals, implementation
boundary, complete milestone map, and final navigation. Each `step-*` branch instead
leads with the current step's purpose, completed behavior, acceptance command,
important files, deliberate exclusions, and adjacent refs. Shared setup and workflow
details remain in `docs/` and are linked rather than copied into every milestone.

Alternative considered: carry the complete `main` README forward and append milestone
status. Rejected because repeated project-wide sections hide the information a learner
needs when switching branches to study one implementation stage.

## Risks / Trade-offs

- **[Common contracts force unnatural APIs]** → Compare responsibility and observable behavior, not identical signatures or data layouts.
- **[Tests encode an incorrect interpretation of HTTP]** → Derive contracts from the applicable HTTP/1.1 RFC sections and record deliberate simplifications in milestone specs.
- **[Happy-path tests miss TCP fragmentation bugs]** → Exercise whole-message, one-byte, CR/LF boundary, field boundary, and several fixed-size chunk partitions.
- **[C memory errors survive functional tests]** → Compile with strict warnings and run sanitizer-based checks in WSL.
- **[Nine milestones become one oversized implementation change]** → Treat this proposal as the course contract; implement and snapshot one milestone at a time.
- **[Later features require an earlier redesign]** → Keep parser states explicit, retain regression fixtures, and add a failing regression before correcting an earlier contract.
