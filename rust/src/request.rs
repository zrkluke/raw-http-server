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

#[cfg(test)]
mod tests {
    use super::{RequestParser, RequestState};

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
}
