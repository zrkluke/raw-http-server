## Context

`rust/src/lib.rs` currently contains all course responsibilities, from CRLF
line reading through the one-request binary echo server. C and Go already use
responsibility-focused files, so the Rust layout weakens the intended
function-to-function learning comparison.

## Goals / Non-Goals

**Goals:**

- Give each existing Rust responsibility a stable, focused source module.
- Preserve the library's public surface and every observable HTTP behavior.
- Keep unit tests close to the responsibility they exercise and retain
  black-box acceptance tests under `rust/tests/`.

**Non-Goals:**

- Change HTTP parsing, framing, response bytes, socket lifecycle, or errors.
- Introduce dependencies, async code, HTTP/2, HTTP/3, or TLS.
- Rewrite historical milestone branches or immutable tags.

## Decisions

### Re-export the existing public boundary from `lib.rs`

`lib.rs` SHALL declare private implementation modules and re-export the
existing public types and functions. This keeps integration tests and callers
stable while making the internal responsibility boundaries visible.

### Split by course responsibility

The library SHALL use `line_reader`, `request_line`, `header`, `body`,
`chunked`, `request`, `response`, and `tcp_server` modules. The binary echo
decoder remains in `tcp_server` because it is the connection-level composition
of the prior parser and response responsibilities, rather than a new protocol
primitive.

An alternative single `http` module was rejected because it would recreate the
same comparison problem at a coarser level. Creating a dedicated `echo` module
was rejected because it would split the one-shot server lifecycle across two
files without improving the course mapping.

### Preserve test scope and verification

Module unit tests SHALL move with the code they exercise. Existing integration
tests in `rust/tests/` SHALL remain black-box tests. The refactor is accepted
only when the complete WSL verification command passes unchanged.

## Risks / Trade-offs

- [Visibility or import changes accidentally alter the public API] → Compile
  the Rust crate and run all existing Rust tests before unified verification.
- [Moving parser code changes byte-level behavior] → Do not alter algorithms
  or fixtures; retain unit and fragmented TCP acceptance coverage.
- [More files make a small learning project feel fragmented] → Use one module
  per existing course responsibility and keep `lib.rs` as the navigation hub.

## Migration Plan

1. Add module declarations and move code without changing behavior.
2. Run formatting, Rust tests, lints, and the unified WSL verification suite.
3. Review the move-only diff and commit it on `refactor-rust-modules`.
4. Later merge the refactor into the cumulative `main` branch during course
   completion; leave milestone refs unchanged.

## Open Questions

- None. The public API and the existing acceptance behavior are preserved.
