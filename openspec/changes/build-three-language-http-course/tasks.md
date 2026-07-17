## 1. Repository and Validation Foundation

- [x] 1.1 Initialize Git, root documentation, ignore rules, and C, Go, Rust project skeletons without HTTP implementation code
- [x] 1.2 Add a WSL-compatible unified command that builds and runs native tests for all three languages
- [x] 1.3 Create the shared `testdata/` conventions and language-native fixture loaders
- [x] 1.4 Configure strict C warnings and sanitizer checks, Go tests, and Rust tests plus lints
- [x] 1.5 Document the Red-Green-Refactor, cumulative regression, Definition of Done, branch, and immutable tag conventions
- [x] 1.6 Rewrite the Foundation README around its purpose, acceptance, important files, exclusions, and version navigation, and establish that focused format for later steps

## 2. Milestone 1 - HTTP Streams

- [x] 2.1 Add failing shared acceptance fixtures for complete and fragmented CRLF-delimited streams
- [x] 2.2 Implement and unit-test the stream line reader in Go
- [x] 2.3 Implement and unit-test the equivalent stream line reader in C
- [x] 2.4 Implement and unit-test the equivalent stream line reader in Rust
- [x] 2.5 Run unified regression and quality checks, document the comparison, and create the verified milestone ref

## 3. Milestone 2 - TCP

- [x] 3.1 Add a failing black-box acceptance test for listen, connect, accept, byte transfer, and connection cleanup
- [x] 3.2 Implement and unit-test the TCP listener path in Go
- [x] 3.3 Implement and unit-test the equivalent POSIX TCP listener path in C under WSL
- [x] 3.4 Implement and unit-test the equivalent TCP listener path in Rust
- [x] 3.5 Run unified regression and integration checks, document the comparison, and create the verified milestone ref

## 4. Milestone 3 - Requests

- [x] 4.1 Add failing shared cases for incomplete, complete, and invalid incremental request states
- [x] 4.2 Implement and unit-test the request state machine and result representation in Go
- [x] 4.3 Implement and unit-test the equivalent request state machine and result representation in C
- [x] 4.4 Implement and unit-test the equivalent request state machine and result representation in Rust
- [x] 4.5 Run unified regression checks, document the comparison, and create the verified milestone ref

## 5. Milestone 4 - Request Lines

- [x] 5.1 Add failing table-driven fixtures for valid lines, malformed fields, separators, versions, and chunk partitions
- [x] 5.2 Implement and unit-test request-line parsing and validation in Go
- [x] 5.3 Implement and unit-test equivalent request-line parsing and validation in C
- [x] 5.4 Implement and unit-test equivalent request-line parsing and validation in Rust
- [x] 5.5 Run unified regression checks, document the comparison, and create the verified milestone ref

## 6. Milestone 5 - HTTP Headers

- [x] 6.1 Add failing shared fixtures for field parsing, case-insensitive names, whitespace, termination, errors, and fragmentation
- [x] 6.2 Implement and unit-test incremental header parsing in Go
- [x] 6.3 Implement and unit-test equivalent incremental header parsing in C
- [x] 6.4 Implement and unit-test equivalent incremental header parsing in Rust
- [x] 6.5 Run unified regression checks, document the comparison, and create the verified milestone ref

## 7. Milestone 6 - HTTP Body

- [x] 7.1 Add failing shared fixtures for absent, complete, partial, invalid-length, and over-delivered bodies
- [x] 7.2 Implement and unit-test `Content-Length` body handling in Go
- [x] 7.3 Implement and unit-test equivalent `Content-Length` body handling in C
- [x] 7.4 Implement and unit-test equivalent `Content-Length` body handling in Rust
- [x] 7.5 Run unified regression checks, document the comparison, and create the verified milestone ref

## 8. Milestone 7 - HTTP Responses

- [x] 8.1 Add failing byte-exact response fixtures and black-box TCP response acceptance tests
- [x] 8.2 Implement and unit-test status line, headers, content length, body, and connection writing in Go
- [x] 8.3 Implement and unit-test equivalent HTTP response behavior in C
- [x] 8.4 Implement and unit-test equivalent HTTP response behavior in Rust
- [x] 8.5 Run unified regression and integration checks, document the comparison, and create the verified milestone ref

## 9. Milestone 8 - Chunked Encoding

- [ ] 9.1 Add failing shared fixtures for chunk sizes, data, terminator, trailers, malformed input, and fragmentation
- [ ] 9.2 Implement and unit-test the course-defined chunked transfer behavior in Go
- [ ] 9.3 Implement and unit-test equivalent chunked transfer behavior in C
- [ ] 9.4 Implement and unit-test equivalent chunked transfer behavior in Rust
- [ ] 9.5 Run unified regression and integration checks, document the comparison, and create the verified milestone ref

## 10. Milestone 9 - Binary Data

- [ ] 10.1 Add a known binary fixture and failing length-and-digest acceptance tests
- [ ] 10.2 Implement and unit-test byte-safe binary transfer in Go
- [ ] 10.3 Implement and unit-test equivalent byte-safe binary transfer in C
- [ ] 10.4 Implement and unit-test equivalent byte-safe binary transfer in Rust
- [ ] 10.5 Run the complete unified suite, document the final comparison, and create the verified milestone ref

## 11. Course Completion

- [ ] 11.1 Verify every milestone requirement and scenario is represented by an automated test or an explicitly documented manual check
- [ ] 11.2 Run all builds, unit tests, shared fixtures, TCP integrations, sanitizer checks, and lints from a clean WSL environment
- [ ] 11.3 Complete the README learning map with function-to-function links across C, Go, and Rust
- [ ] 11.4 Confirm immutable tags resolve to each verified milestone and `main` contains the completed cumulative implementation
