use crate::header::{Header, HeaderParser, HeaderState};

/// The observable state of an incremental HTTP chunked transfer parser.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ChunkedState {
    Incomplete,
    Complete,
    Invalid,
}

impl std::fmt::Display for ChunkedState {
    fn fmt(&self, formatter: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            Self::Incomplete => formatter.write_str("incomplete"),
            Self::Complete => formatter.write_str("complete"),
            Self::Invalid => formatter.write_str("invalid"),
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
enum ChunkedPhase {
    Size,
    SizeLf,
    Data,
    DataCr,
    DataLf,
    Trailers,
}

/// Incrementally parses HTTP/1.1 chunked transfer coding without extensions.
pub struct ChunkedParser {
    state: ChunkedState,
    phase: ChunkedPhase,
    size_line: Vec<u8>,
    chunk_remaining: usize,
    body: Vec<u8>,
    trailers: HeaderParser,
    remaining: Vec<u8>,
}

impl Default for ChunkedParser {
    fn default() -> Self {
        Self {
            state: ChunkedState::Incomplete,
            phase: ChunkedPhase::Size,
            size_line: Vec::new(),
            chunk_remaining: 0,
            body: Vec::new(),
            trailers: HeaderParser::new(),
            remaining: Vec::new(),
        }
    }
}

impl ChunkedParser {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn state(&self) -> ChunkedState {
        self.state
    }

    pub fn body(&self) -> &[u8] {
        &self.body
    }

    pub fn trailers(&self) -> &[Header] {
        self.trailers.headers()
    }

    pub fn remaining(&self) -> &[u8] {
        &self.remaining
    }

    /// Consumes a TCP byte chunk and returns the parser's state.
    pub fn feed(&mut self, chunk: &[u8]) -> ChunkedState {
        for &byte in chunk {
            if self.state == ChunkedState::Complete {
                self.remaining.push(byte);
                continue;
            }
            if self.state == ChunkedState::Invalid {
                continue;
            }
            self.feed_byte(byte);
        }
        self.state
    }

    fn feed_byte(&mut self, byte: u8) {
        match self.phase {
            ChunkedPhase::Size => match byte {
                b'\r' => match self.parse_chunk_size() {
                    Some(size) => {
                        self.chunk_remaining = size;
                        self.phase = ChunkedPhase::SizeLf;
                    }
                    None => self.state = ChunkedState::Invalid,
                },
                b'\n' => self.state = ChunkedState::Invalid,
                _ => self.size_line.push(byte),
            },
            ChunkedPhase::SizeLf => {
                if byte != b'\n' {
                    self.state = ChunkedState::Invalid;
                } else if self.chunk_remaining == 0 {
                    self.phase = ChunkedPhase::Trailers;
                } else {
                    self.phase = ChunkedPhase::Data;
                }
            }
            ChunkedPhase::Data => {
                self.body.push(byte);
                self.chunk_remaining -= 1;
                if self.chunk_remaining == 0 {
                    self.phase = ChunkedPhase::DataCr;
                }
            }
            ChunkedPhase::DataCr => {
                if byte == b'\r' {
                    self.phase = ChunkedPhase::DataLf;
                } else {
                    self.state = ChunkedState::Invalid;
                }
            }
            ChunkedPhase::DataLf => {
                if byte == b'\n' {
                    self.size_line.clear();
                    self.phase = ChunkedPhase::Size;
                } else {
                    self.state = ChunkedState::Invalid;
                }
            }
            ChunkedPhase::Trailers => match self.trailers.feed(&[byte]) {
                HeaderState::Incomplete => {}
                HeaderState::Complete => self.state = ChunkedState::Complete,
                HeaderState::Invalid => self.state = ChunkedState::Invalid,
            },
        }
    }

    fn parse_chunk_size(&self) -> Option<usize> {
        let size = std::str::from_utf8(&self.size_line).ok()?;
        if size.is_empty() || size.contains(';') {
            return None;
        }
        usize::from_str_radix(size, 16).ok()
    }
}

#[cfg(test)]
mod tests {
    use super::{ChunkedParser, ChunkedState};

    #[test]
    fn chunked_parser_rejects_a_bare_line_feed() {
        let mut parser = ChunkedParser::new();
        assert_eq!(parser.feed(b"3\nabc"), ChunkedState::Invalid);
    }

    #[test]
    fn chunked_parser_keeps_bytes_after_the_terminal_chunk() {
        let mut parser = ChunkedParser::new();
        assert_eq!(parser.feed(b"0\r\n\r\nnext"), ChunkedState::Complete);
        assert_eq!(parser.body(), b"");
        assert_eq!(parser.remaining(), b"next");
    }
}
