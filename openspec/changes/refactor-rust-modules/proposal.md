## Why

The Rust implementation has accumulated every course responsibility in one
`rust/src/lib.rs` file. This obscures the function-to-function comparison with
the C and Go implementations and makes the final learning map harder to use.

## What Changes

- Split the Rust library into responsibility-focused source modules for stream
  reading, request parsing, response writing, and TCP server behavior.
- Keep the existing public behavior, fixtures, and black-box TCP acceptance
  cases unchanged.
- Keep `lib.rs` as the module declaration and public re-export boundary.
- Record the module responsibilities and verification requirements for the
  refactor.

## Capabilities

### New Capabilities

- `rust-module-layout`: Defines responsibility-focused Rust source modules
  while preserving the existing HTTP learning behavior.

### Modified Capabilities

- None.

## Impact

- Affected code: `rust/src/lib.rs`, new Rust source modules, and Rust unit
  test placement.
- No HTTP protocol behavior, public learning scenarios, dependencies, C code,
  or Go code change.
- The final README learning map can link to stable Rust responsibility files.
