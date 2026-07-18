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

#[cfg(test)]
mod tests {
    use super::{BodyParser, BodyState};

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
}
