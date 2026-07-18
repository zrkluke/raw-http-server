# Purpose

Define the responsibility-focused Rust source layout while preserving the
course implementation's observable behavior and historical milestones.

## Requirements

### Requirement: Responsibility-focused Rust module layout
The Rust library SHALL separate the existing stream, request-line, header, body, chunked, request, response, and TCP server responsibilities into focused source modules. `lib.rs` SHALL serve as the module declaration and public re-export boundary.

#### Scenario: Learner locates a course responsibility
- **WHEN** a learner follows the Rust implementation for a course responsibility
- **THEN** the implementation is located in its focused source module rather than an unrelated aggregate file

### Requirement: Observable behavior preservation
The module refactor SHALL preserve the existing Rust public API and observable HTTP behavior, including byte-exact response behavior and the binary echo acceptance cases.

#### Scenario: Existing regression suite runs after the refactor
- **WHEN** the repository runs the unified WSL verification command
- **THEN** the C, Go, and Rust builds, tests, TCP integrations, sanitizers, format checks, and lints pass without altered fixtures or protocol outcomes

### Requirement: Historical milestone preservation
The module refactor SHALL not rewrite existing milestone branches or immutable tags.

#### Scenario: Existing Step 9 tag is inspected
- **WHEN** `step-9-binary-data-v1` is resolved after the refactor
- **THEN** it continues to identify the verified Step 9 commit
