use std::io::{self, Read};
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
    use super::{LineReader, RequestLine, RequestParser, RequestState, TcpServer};

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
}
