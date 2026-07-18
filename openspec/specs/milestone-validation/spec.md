# Purpose

Define the validation, regression, and immutable-reference rules for the
three-language HTTP learning milestones.

## Requirements

### Requirement: Acceptance tests precede implementation
Each milestone SHALL have executable acceptance cases defining its observable completion criteria before production implementation for that milestone begins.

#### Scenario: Begin a milestone
- **WHEN** work starts on a new milestone
- **THEN** at least one relevant acceptance case exists and fails because the required behavior is not yet implemented

### Requirement: Language-native TDD
New function-level behavior SHALL be developed in each language using a Red-Green-Refactor cycle and the language's native test facilities.

#### Scenario: Add function behavior
- **WHEN** a function gains a new externally observable case
- **THEN** a failing unit test is demonstrated before the minimum implementation is added and refactoring occurs only with tests passing

### Requirement: Shared cross-language contract
Equivalent protocol behavior SHALL be exercised from shared byte fixtures and expected outcomes for C, Go, and Rust.

#### Scenario: Run a shared fixture
- **WHEN** the repository test command executes a language-neutral fixture
- **THEN** all three implementations produce the fixture's expected observable result

### Requirement: Fragmentation coverage
Every incremental parsing milestone SHALL verify behavior under multiple equivalent chunk partitions.

#### Scenario: Partition a protocol message
- **WHEN** the same message is supplied whole, one byte at a time, across CRLF, across field boundaries, and in fixed-size chunks
- **THEN** every partition produces the same final result or the same specified error

### Requirement: Cumulative regression gate
A milestone SHALL NOT be declared complete unless all tests for that milestone and all earlier milestones pass for all three languages.

#### Scenario: Earlier behavior regresses
- **WHEN** a current change causes any previously passing milestone test to fail
- **THEN** the current milestone remains incomplete until the regression is resolved or the governing specification is explicitly changed

### Requirement: Defect regression test
Every corrected behavioral defect SHALL first be represented by a test that fails against the defective implementation.

#### Scenario: Correct a late-discovered parser defect
- **WHEN** a defect originating in an earlier milestone is discovered
- **THEN** a shared or language-specific regression test reproduces it before the implementation is corrected

### Requirement: Milestone Definition of Done
A milestone SHALL be complete only when all three implementations provide the required behavior, native unit tests pass, applicable shared acceptance and TCP integration tests pass, all previous tests pass, and required quality checks succeed in WSL.

#### Scenario: Mark milestone complete
- **WHEN** the project attempts to create a milestone Git ref
- **THEN** the unified WSL verification command succeeds for C, Go, Rust, shared fixtures, integrations, and configured static or sanitizer checks

### Requirement: Immutable milestone reference
Each completed milestone SHALL have an immutable Git tag that identifies the exact verified source state.

#### Scenario: Reference code from an article
- **WHEN** documentation links to a completed milestone
- **THEN** it can link to the immutable tag rather than relying solely on a mutable branch

### Requirement: Milestone-focused branch documentation
Each milestone branch README SHALL prioritize the current step's purpose, completed behavior, acceptance method, important files, deliberate exclusions, and version navigation instead of repeating the full project overview from `main`.

#### Scenario: Open a milestone branch on GitHub
- **WHEN** a learner reads the README on a completed step branch
- **THEN** the learner can identify what changed in that step, how to verify it, and which files to inspect before following links to shared project documentation
