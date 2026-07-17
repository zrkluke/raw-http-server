use std::io::{self, Read, Write};
use std::net::{SocketAddr, TcpListener};
use std::sync::mpsc::{self, Receiver, RecvTimeoutError, SyncSender};
use std::thread;
use std::time::Duration;

#[derive(Default)]
pub struct LineReader {
    pending: Vec<u8>,
}

impl LineReader {
    pub fn new() -> Self {
        Self::default()
    }

    /// Adds a TCP byte chunk and returns every complete CRLF-delimited line.
    /// Returned lines own their bytes and do not include the CRLF terminator.
    pub fn feed(&mut self, chunk: &[u8]) -> Vec<Vec<u8>> {
        self.pending.extend_from_slice(chunk);

        let mut lines = Vec::new();
        let mut consumed = 0;

        while let Some(index) = find_crlf(&self.pending[consumed..]) {
            let end = consumed + index;
            lines.push(self.pending[consumed..end].to_vec());
            consumed = end + 2;
        }

        if consumed > 0 {
            let _ = self.pending.drain(..consumed);
        }

        lines
    }
}

fn find_crlf(bytes: &[u8]) -> Option<usize> {
    bytes.windows(2).position(|window| window == b"\r\n")
}

/// A course-defined HTTP/1.1 request line with owned byte fields.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct RequestLine {
    method: Vec<u8>,
    target: Vec<u8>,
    version: Vec<u8>,
}

/// The request line did not match this course's limited HTTP/1.1 grammar.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum RequestLineError {
    Invalid,
}

impl RequestLine {
    pub fn parse(line: &[u8]) -> Result<Self, RequestLineError> {
        if line.contains(&b'\r') || line.contains(&b'\n') || line.contains(&b'\t') {
            return Err(RequestLineError::Invalid);
        }

        let mut fields = line.split(|&byte| byte == b' ');
        let (Some(method), Some(target), Some(version)) =
            (fields.next(), fields.next(), fields.next())
        else {
            return Err(RequestLineError::Invalid);
        };

        if method.is_empty()
            || target.is_empty()
            || version != b"HTTP/1.1"
            || fields.next().is_some()
        {
            return Err(RequestLineError::Invalid);
        }

        Ok(Self {
            method: method.to_vec(),
            target: target.to_vec(),
            version: version.to_vec(),
        })
    }
}

impl std::fmt::Display for RequestLine {
    fn fmt(&self, formatter: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(
            formatter,
            "{}|{}|{}",
            String::from_utf8_lossy(&self.method),
            String::from_utf8_lossy(&self.target),
            String::from_utf8_lossy(&self.version)
        )
    }
}

/// The observable framing state of an incremental HTTP request head.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum RequestState {
    Incomplete,
    Complete,
    Invalid,
}

impl std::fmt::Display for RequestState {
    fn fmt(&self, formatter: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            Self::Incomplete => formatter.write_str("incomplete"),
            Self::Complete => formatter.write_str("complete"),
            Self::Invalid => formatter.write_str("invalid"),
        }
    }
}

/// One normalized HTTP header field for this course milestone.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Header {
    name: Vec<u8>,
    value: Vec<u8>,
}

/// The observable state of an incremental HTTP header section parser.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum HeaderState {
    Incomplete,
    Complete,
    Invalid,
}

impl std::fmt::Display for HeaderState {
    fn fmt(&self, formatter: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            Self::Incomplete => formatter.write_str("incomplete"),
            Self::Complete => formatter.write_str("complete"),
            Self::Invalid => formatter.write_str("invalid"),
        }
    }
}

/// Incrementally parses HTTP header fields after a request line.
pub struct HeaderParser {
    state: HeaderState,
    line: Vec<u8>,
    pending_cr: bool,
    headers: Vec<Header>,
}

impl Default for HeaderParser {
    fn default() -> Self {
        Self {
            state: HeaderState::Incomplete,
            line: Vec::new(),
            pending_cr: false,
            headers: Vec::new(),
        }
    }
}

impl HeaderParser {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn state(&self) -> HeaderState {
        self.state
    }

    /// Consumes a TCP byte chunk and returns the parser's state.
    pub fn feed(&mut self, chunk: &[u8]) -> HeaderState {
        if self.state != HeaderState::Incomplete {
            return self.state;
        }

        for &byte in chunk {
            if self.pending_cr {
                if byte != b'\n' {
                    self.state = HeaderState::Invalid;
                    return self.state;
                }

                self.pending_cr = false;
                if self.line.is_empty() {
                    self.state = HeaderState::Complete;
                    return self.state;
                }
                if !self.add_header() {
                    self.state = HeaderState::Invalid;
                    return self.state;
                }
                self.line.clear();
                continue;
            }

            match byte {
                b'\r' => self.pending_cr = true,
                b'\n' => {
                    self.state = HeaderState::Invalid;
                    return self.state;
                }
                _ => self.line.push(byte),
            }
        }

        self.state
    }

    fn add_header(&mut self) -> bool {
        if self
            .line
            .first()
            .is_some_and(|byte| is_optional_whitespace(*byte))
        {
            return false;
        }

        let Some(separator) = self.line.iter().position(|&byte| byte == b':') else {
            return false;
        };
        if separator == 0
            || self.line[..separator]
                .iter()
                .any(|&byte| byte <= b' ' || byte == 0x7f)
        {
            return false;
        }

        let mut name = self.line[..separator].to_vec();
        name.make_ascii_lowercase();
        let value = trim_optional_whitespace(&self.line[separator + 1..]).to_vec();
        self.headers.push(Header { name, value });
        true
    }
}

impl std::fmt::Display for HeaderParser {
    fn fmt(&self, formatter: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        if self.state != HeaderState::Complete {
            return self.state.fmt(formatter);
        }

        formatter.write_str("complete")?;
        for header in &self.headers {
            write!(
                formatter,
                "\n{}|{}",
                String::from_utf8_lossy(&header.name),
                String::from_utf8_lossy(&header.value)
            )?;
        }
        Ok(())
    }
}

fn is_optional_whitespace(byte: u8) -> bool {
    byte == b' ' || byte == b'\t'
}

fn trim_optional_whitespace(bytes: &[u8]) -> &[u8] {
    let mut start = 0;
    let mut end = bytes.len();

    while start < end && is_optional_whitespace(bytes[start]) {
        start += 1;
    }
    while end > start && is_optional_whitespace(bytes[end - 1]) {
        end -= 1;
    }

    &bytes[start..end]
}

/// The observable state of incremental `Content-Length` body handling.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum BodyState {
    Incomplete,
    Complete,
    Invalid,
}

/// Incrementally collects an HTTP request body without consuming trailing bytes.
pub struct BodyParser {
    state: BodyState,
    expected_length: Option<usize>,
    body: Vec<u8>,
    remaining: Vec<u8>,
}

impl BodyParser {
    /// Creates a parser for a validated `Content-Length` value.
    ///
    /// An absent value represents a request without a body. A malformed value
    /// puts the parser in the terminal invalid state.
    pub fn new(content_length: Option<&str>) -> Self {
        match content_length {
            None => Self {
                state: BodyState::Complete,
                expected_length: None,
                body: Vec::new(),
                remaining: Vec::new(),
            },
            Some(value) => match parse_content_length(value) {
                Some(expected_length) => Self {
                    state: if expected_length == 0 {
                        BodyState::Complete
                    } else {
                        BodyState::Incomplete
                    },
                    expected_length: Some(expected_length),
                    body: Vec::new(),
                    remaining: Vec::new(),
                },
                None => Self {
                    state: BodyState::Invalid,
                    expected_length: None,
                    body: Vec::new(),
                    remaining: Vec::new(),
                },
            },
        }
    }

    pub fn state(&self) -> BodyState {
        self.state
    }

    pub fn body(&self) -> &[u8] {
        &self.body
    }

    pub fn remaining(&self) -> &[u8] {
        &self.remaining
    }

    /// Consumes at most the declared body length and retains later bytes.
    pub fn feed(&mut self, chunk: &[u8]) -> BodyState {
        if self.state == BodyState::Invalid || self.state == BodyState::Complete {
            self.remaining.extend_from_slice(chunk);
            return self.state;
        }

        let expected_length = self
            .expected_length
            .expect("incomplete parser has a length");
        let needed = expected_length - self.body.len();
        let consumed = needed.min(chunk.len());
        self.body.extend_from_slice(&chunk[..consumed]);

        if self.body.len() == expected_length {
            self.state = BodyState::Complete;
            self.remaining.extend_from_slice(&chunk[consumed..]);
        }

        self.state
    }
}

fn parse_content_length(value: &str) -> Option<usize> {
    if value.is_empty() || !value.bytes().all(|byte| byte.is_ascii_digit()) {
        return None;
    }

    value.bytes().try_fold(0usize, |length, byte| {
        length
            .checked_mul(10)?
            .checked_add(usize::from(byte - b'0'))
    })
}

/// Incrementally recognizes the boundary of an HTTP request head.
pub struct RequestParser {
    state: RequestState,
    saw_first_line: bool,
    line_has_bytes: bool,
    pending_cr: bool,
}

impl Default for RequestParser {
    fn default() -> Self {
        Self {
            state: RequestState::Incomplete,
            saw_first_line: false,
            line_has_bytes: false,
            pending_cr: false,
        }
    }
}

impl RequestParser {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn state(&self) -> RequestState {
        self.state
    }

    /// Consumes the next TCP byte chunk and returns the parser's state.
    pub fn feed(&mut self, chunk: &[u8]) -> RequestState {
        if self.state != RequestState::Incomplete {
            return self.state;
        }

        for &byte in chunk {
            if self.pending_cr {
                if byte != b'\n' {
                    self.state = RequestState::Invalid;
                    return self.state;
                }
                self.pending_cr = false;
                if !self.saw_first_line {
                    if !self.line_has_bytes {
                        self.state = RequestState::Invalid;
                        return self.state;
                    }
                    self.saw_first_line = true;
                } else if !self.line_has_bytes {
                    self.state = RequestState::Complete;
                    return self.state;
                }
                self.line_has_bytes = false;
                continue;
            }

            match byte {
                b'\r' => self.pending_cr = true,
                b'\n' => {
                    self.state = RequestState::Invalid;
                    return self.state;
                }
                _ => self.line_has_bytes = true,
            }
        }

        self.state
    }
}

/// Observes one accepted TCP connection without exposing the listener itself.
pub struct TcpServer {
    address: SocketAddr,
    received: Receiver<io::Result<Vec<u8>>>,
    done: Receiver<io::Result<()>>,
}

/// A limited byte-oriented HTTP/1.1 response used by this course.
pub struct Response {
    status_code: u16,
    reason: &'static str,
    content_type: &'static str,
    body: Vec<u8>,
}

impl Response {
    /// Returns the course's fixed text response.
    pub fn basic_ok() -> Self {
        Self {
            status_code: 200,
            reason: "OK",
            content_type: "text/plain",
            body: b"Hello, World!".to_vec(),
        }
    }

    /// Serializes a status line, headers, header terminator, and body.
    pub fn bytes(&self) -> Vec<u8> {
        let header = format!(
            "HTTP/1.1 {} {}\r\nContent-Type: {}\r\nContent-Length: {}\r\nConnection: close\r\n\r\n",
            self.status_code,
            self.reason,
            self.content_type,
            self.body.len()
        );
        let mut bytes = Vec::with_capacity(header.len() + self.body.len());

        bytes.extend_from_slice(header.as_bytes());
        bytes.extend_from_slice(&self.body);
        bytes
    }
}

impl TcpServer {
    /// Binds an address and starts a background task that accepts one client.
    pub fn start_once(address: &str) -> io::Result<Self> {
        let listener = TcpListener::bind(address)?;
        let address = listener.local_addr()?;
        let (received_sender, received) = mpsc::sync_channel(1);
        let (done_sender, done) = mpsc::sync_channel(1);

        let _server_thread = thread::spawn(move || {
            let result = serve_once(listener, received_sender);

            let _ = done_sender.send(result);
        });

        Ok(Self {
            address,
            received,
            done,
        })
    }

    /// Binds an address and writes the course's basic response to one client.
    pub fn start_response_once(address: &str) -> io::Result<Self> {
        let listener = TcpListener::bind(address)?;
        let address = listener.local_addr()?;
        let (_received_sender, received) = mpsc::sync_channel(1);
        let (done_sender, done) = mpsc::sync_channel(1);

        let _server_thread = thread::spawn(move || {
            let result = serve_response_once(listener, Response::basic_ok());

            let _ = done_sender.send(result);
        });

        Ok(Self {
            address,
            received,
            done,
        })
    }

    pub fn address(&self) -> SocketAddr {
        self.address
    }

    pub fn receive_timeout(&self, timeout: Duration) -> io::Result<Vec<u8>> {
        self.received.recv_timeout(timeout).map_err(channel_error)?
    }

    pub fn wait_timeout(&self, timeout: Duration) -> io::Result<()> {
        self.done.recv_timeout(timeout).map_err(channel_error)?
    }
}

fn serve_once(listener: TcpListener, received: SyncSender<io::Result<Vec<u8>>>) -> io::Result<()> {
    let (mut connection, _) = listener.accept()?;

    drop(listener);
    let mut bytes = Vec::new();
    match connection.read_to_end(&mut bytes) {
        Ok(_) => {
            let _ = received.send(Ok(bytes));
            Ok(())
        }
        Err(error) => {
            let received_error = io::Error::new(error.kind(), error.to_string());

            let _ = received.send(Err(received_error));
            Err(error)
        }
    }
}

fn serve_response_once(listener: TcpListener, response: Response) -> io::Result<()> {
    let (mut connection, _) = listener.accept()?;

    drop(listener);
    let mut buffer = [0_u8; 4096];
    let count = connection.read(&mut buffer)?;
    if count == 0 {
        return Err(io::Error::new(
            io::ErrorKind::UnexpectedEof,
            "client closed before sending a request",
        ));
    }

    connection.write_all(&response.bytes())
}

fn channel_error(error: RecvTimeoutError) -> io::Error {
    match error {
        RecvTimeoutError::Timeout => io::Error::new(io::ErrorKind::TimedOut, "server timed out"),
        RecvTimeoutError::Disconnected => {
            io::Error::new(io::ErrorKind::BrokenPipe, "server task stopped")
        }
    }
}

#[cfg(test)]
mod tests {
    use super::{
        BodyParser, BodyState, HeaderParser, HeaderState, LineReader, RequestLine, RequestParser,
        RequestState, Response, TcpServer,
    };

    #[test]
    fn waits_for_a_complete_line() {
        let mut reader = LineReader::new();

        assert!(reader.feed(b"hello").is_empty());
    }

    #[test]
    fn recognizes_crlf_across_chunks() {
        let mut reader = LineReader::new();

        assert!(reader.feed(b"hello\r").is_empty());
        assert_eq!(reader.feed(b"\n"), vec![b"hello".to_vec()]);
    }

    #[test]
    fn returns_every_complete_line() {
        let mut reader = LineReader::new();

        assert_eq!(
            reader.feed(b"one\r\ntwo\r\n"),
            vec![b"one".to_vec(), b"two".to_vec()]
        );
    }

    #[test]
    fn keeps_an_incomplete_suffix() {
        let mut reader = LineReader::new();

        assert_eq!(reader.feed(b"one\r\ntw"), vec![b"one".to_vec()]);
        assert_eq!(reader.feed(b"o\r\n"), vec![b"two".to_vec()]);
    }

    #[test]
    fn returns_an_empty_line() {
        let mut reader = LineReader::new();

        assert_eq!(reader.feed(b"\r\n"), vec![Vec::new()]);
    }

    #[test]
    fn emitted_lines_own_their_bytes() {
        let mut reader = LineReader::new();
        let line = reader.feed(b"hello\r\n").remove(0);

        let _ = reader.feed(b"next\r\n");

        assert_eq!(line, b"hello");
    }

    #[test]
    fn tcp_server_rejects_an_invalid_address() {
        assert!(TcpServer::start_once("not-a-tcp-address").is_err());
    }

    #[test]
    fn request_parser_waits_for_an_incomplete_head() {
        let mut parser = RequestParser::new();

        assert_eq!(
            parser.feed(b"GET / HTTP/1.1\r\nHost: example\r"),
            RequestState::Incomplete
        );
    }

    #[test]
    fn request_parser_completes_across_crlf_boundaries() {
        let mut parser = RequestParser::new();

        assert_eq!(parser.feed(b"GET / HTTP/1.1\r"), RequestState::Incomplete);
        assert_eq!(
            parser.feed(b"\nHost: example\r\n\r"),
            RequestState::Incomplete
        );
        assert_eq!(parser.feed(b"\n"), RequestState::Complete);
    }

    #[test]
    fn request_parser_rejects_bare_line_endings() {
        let mut parser = RequestParser::new();

        assert_eq!(parser.feed(b"GET / HTTP/1.1\n"), RequestState::Invalid);

        let mut parser = RequestParser::new();
        assert_eq!(parser.feed(b"GET / HTTP/1.1\r"), RequestState::Incomplete);
        assert_eq!(parser.feed(b"x"), RequestState::Invalid);
    }

    #[test]
    fn request_parser_keeps_its_terminal_state() {
        let mut parser = RequestParser::new();

        assert_eq!(
            parser.feed(b"GET / HTTP/1.1\r\n\r\n"),
            RequestState::Complete
        );
        assert_eq!(parser.feed(b"unexpected"), RequestState::Complete);
    }

    #[test]
    fn request_line_parses_three_single_space_fields() {
        let parsed = RequestLine::parse(b"GET /search?q=rust HTTP/1.1")
            .expect("request line should be valid");

        assert_eq!(parsed.method, b"GET");
        assert_eq!(parsed.target, b"/search?q=rust");
        assert_eq!(parsed.version, b"HTTP/1.1");
    }

    #[test]
    fn request_line_rejects_invalid_separators_and_versions() {
        for line in [
            b"GET  HTTP/1.1".as_slice(),
            b" GET / HTTP/1.1",
            b"GET / HTTP/1.1 ",
            b"GET  / HTTP/1.1",
            b"GET\t/ HTTP/1.1",
            b"GET / HTTP/1.0",
            b"GET / HTTP/1.1\r\n",
        ] {
            assert!(
                RequestLine::parse(line).is_err(),
                "{line:?} should be invalid"
            );
        }
    }

    #[test]
    fn header_parser_normalizes_names_and_trims_optional_whitespace() {
        let mut parser = HeaderParser::new();

        assert_eq!(parser.feed(b"HOST: example.com\r"), HeaderState::Incomplete);
        assert_eq!(
            parser.feed(b"\nX-Note: \t hello \t\r\n\r\n"),
            HeaderState::Complete
        );
        assert_eq!(
            parser.to_string(),
            "complete\nhost|example.com\nx-note|hello"
        );
    }

    #[test]
    fn header_parser_preserves_repeated_field_order() {
        let mut parser = HeaderParser::new();

        assert_eq!(
            parser.feed(b"Set-Cookie: one\r\nSet-Cookie: two\r\n\r\n"),
            HeaderState::Complete
        );
        assert_eq!(
            parser.to_string(),
            "complete\nset-cookie|one\nset-cookie|two"
        );
    }

    #[test]
    fn header_parser_rejects_invalid_line_endings_and_leading_whitespace() {
        for bytes in [
            b" Host: example.com\r\n\r\n".as_slice(),
            b"Host: example.com\n".as_slice(),
            b"Host: example.com\rX".as_slice(),
        ] {
            let mut parser = HeaderParser::new();

            assert_eq!(parser.feed(bytes), HeaderState::Invalid);
        }
    }

    #[test]
    fn body_parser_waits_for_the_declared_length() {
        let mut parser = BodyParser::new(Some("4"));

        assert_eq!(parser.feed(b"ca"), BodyState::Incomplete);
        assert_eq!(parser.body(), b"ca");
        assert_eq!(parser.feed(b"t\n"), BodyState::Complete);
        assert_eq!(parser.body(), b"cat\n");
    }

    #[test]
    fn body_parser_preserves_over_delivered_bytes() {
        let mut parser = BodyParser::new(Some("4"));

        assert_eq!(parser.feed(b"cat\nnext\n"), BodyState::Complete);
        assert_eq!(parser.body(), b"cat\n");
        assert_eq!(parser.remaining(), b"next\n");
    }

    #[test]
    fn body_parser_rejects_invalid_content_length() {
        let mut parser = BodyParser::new(Some("four"));

        assert_eq!(parser.feed(b"data"), BodyState::Invalid);
        assert_eq!(parser.body(), b"");
        assert_eq!(parser.remaining(), b"data");
    }

    #[test]
    fn body_parser_without_content_length_keeps_all_bytes_remaining() {
        let mut parser = BodyParser::new(None);

        assert_eq!(parser.feed(b"after\n"), BodyState::Complete);
        assert_eq!(parser.body(), b"");
        assert_eq!(parser.remaining(), b"after\n");
    }

    #[test]
    fn body_parser_completes_immediately_for_zero_content_length() {
        let mut parser = BodyParser::new(Some("0"));

        assert_eq!(parser.state(), BodyState::Complete);
        assert_eq!(parser.feed(b"next"), BodyState::Complete);
        assert_eq!(parser.body(), b"");
        assert_eq!(parser.remaining(), b"next");
    }

    #[test]
    fn basic_ok_serializes_a_byte_exact_response() {
        assert_eq!(
            Response::basic_ok().bytes(),
            b"HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 13\r\nConnection: close\r\n\r\nHello, World!"
        );
    }
}
