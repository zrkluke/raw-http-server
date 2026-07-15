## ADDED Requirements

### Requirement: Common implementation boundary
The C, Go, and Rust implementations SHALL begin at TCP byte-stream APIs and SHALL NOT use a prebuilt HTTP server, HTTP parser, Web framework, async runtime, or third-party implementation dependency. Tests SHALL default to the standard toolchains and libraries.

#### Scenario: Permitted platform APIs
- **WHEN** an implementation opens, reads, writes, or closes a TCP connection
- **THEN** C may use POSIX sockets, Go may use its standard `net` and I/O packages, and Rust may use `std::net` and standard I/O traits

#### Scenario: Prohibited HTTP abstraction
- **WHEN** dependencies and imports are reviewed
- **THEN** no dependency performs HTTP request parsing, HTTP response generation, or HTTP server dispatch for the implementation

#### Scenario: Proposed third-party test tool
- **WHEN** test infrastructure cannot reasonably be implemented with the standard toolchains and libraries
- **THEN** the development tool requires explicit approval and remains isolated from all HTTP implementation code

### Requirement: Function-level comparability
Each language SHALL expose equivalent functional responsibilities for the behavior introduced by a milestone while retaining idiomatic signatures, types, errors, and resource management.

#### Scenario: Compare an implemented responsibility
- **WHEN** a learner examines the same completed milestone in C, Go, and Rust
- **THEN** the corresponding responsibility can be identified in each implementation without requiring line-by-line correspondence

### Requirement: Milestone 1 HTTP Streams
Each implementation SHALL incrementally consume byte chunks and recognize CRLF-delimited lines without assuming that one read contains a complete line.

#### Scenario: Delimiter crosses a chunk boundary
- **WHEN** carriage return is the final byte of one chunk and line feed is the first byte of the next
- **THEN** the implementation emits exactly one completed line after the second chunk

### Requirement: Milestone 2 TCP
Each implementation SHALL listen on a configurable local TCP address, accept a client, read bytes, and release connection resources.

#### Scenario: Local client sends bytes
- **WHEN** a client connects to the configured address and sends data
- **THEN** the server accepts the connection and makes the received bytes available to its stream-processing layer

### Requirement: Milestone 3 Requests
Each implementation SHALL represent incremental request parsing with distinguishable incomplete, complete, and invalid outcomes.

For this milestone, "complete" means that the request head has a non-empty
first line and is terminated by an empty CRLF-delimited line. Request-line
semantics, header syntax, and message-body framing are deliberately deferred
to later milestones. The course parser SHALL require CRLF line endings; a bare
LF, or a bare CR once its following non-LF byte is available, is invalid. A CR
at the end of currently available input remains incomplete.

#### Scenario: Partial request remains incomplete
- **WHEN** the available bytes do not contain all data required by the current parser state
- **THEN** the parser reports incomplete without treating the input as invalid or discarding required bytes

#### Scenario: Request head is complete
- **WHEN** a non-empty first line and the terminating empty CRLF line have been received
- **THEN** the parser reports complete without yet interpreting the request line or header fields

#### Scenario: Invalid line ending
- **WHEN** a bare LF or a bare CR followed by a non-LF byte is received
- **THEN** the parser reports invalid

### Requirement: Milestone 4 Request Lines
Each implementation SHALL parse and validate the request method, request target, and HTTP version from a CRLF-terminated request line.

For this learning milestone, a request line consists of exactly three non-empty
byte fields separated by single SP bytes: method, request target, and HTTP
version. The implementation SHALL accept only the case-sensitive version
`HTTP/1.1`. It SHALL reject leading, trailing, repeated, or tab separators.
It SHALL not yet validate the individual request-target forms or implement the
full HTTP method-token grammar; those protocol details are deliberately deferred.

#### Scenario: Valid request line
- **WHEN** the parser receives `GET /example HTTP/1.1` followed by CRLF
- **THEN** it returns method `GET`, target `/example`, and version `HTTP/1.1`

#### Scenario: Malformed request line
- **WHEN** the request line has missing fields, invalid separators, or an unsupported version
- **THEN** the parser reports an invalid request-line outcome

#### Scenario: Request line crosses TCP read boundaries
- **WHEN** the same valid CRLF-terminated request line is divided at field boundaries or within its final CRLF
- **THEN** parsing produces the same method, target, and version as when all bytes arrive at once

### Requirement: Milestone 5 HTTP Headers
Each implementation SHALL incrementally parse header field names and values until the terminating empty line and SHALL treat field names case-insensitively.

#### Scenario: Headers cross read boundaries
- **WHEN** header names, values, or CRLF delimiters are divided across input chunks
- **THEN** the parsed headers match the result produced when the same bytes are supplied at once

### Requirement: Milestone 6 HTTP Body
Each implementation SHALL read the request body according to the validated `Content-Length` and SHALL not consume bytes beyond that body.

#### Scenario: Body arrives incrementally
- **WHEN** fewer body bytes than `Content-Length` are currently available
- **THEN** parsing remains incomplete until exactly the declared number of bytes has been accumulated

### Requirement: Milestone 7 HTTP Responses
Each implementation SHALL construct and transmit an HTTP/1.1 response containing a status line, headers, header terminator, and body with a correct content length where applicable.

#### Scenario: Client receives a basic response
- **WHEN** a valid supported request is handled over TCP
- **THEN** the client receives syntactically valid HTTP/1.1 response bytes with the declared body length

### Requirement: Milestone 8 Chunked Encoding
Each implementation SHALL process the course-defined HTTP/1.1 chunked transfer format incrementally, including the terminating chunk and supported trailers.

#### Scenario: Chunk metadata and data are fragmented
- **WHEN** chunk-size lines, chunk data, delimiters, and trailers cross arbitrary input boundaries
- **THEN** the decoded payload and trailers equal those produced from the unfragmented message

### Requirement: Milestone 9 Binary Data
Each implementation SHALL transmit and receive protocol payloads as bytes without corrupting non-text data.

#### Scenario: Binary payload round trip
- **WHEN** a known binary fixture is transferred through the completed server behavior
- **THEN** the received length and cryptographic digest match the original fixture
