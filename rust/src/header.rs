/// One normalized HTTP header field for this course milestone.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Header {
    name: Vec<u8>,
    value: Vec<u8>,
}

impl Header {
    pub fn name(&self) -> &[u8] {
        &self.name
    }

    pub fn value(&self) -> &[u8] {
        &self.value
    }
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

    pub fn headers(&self) -> &[Header] {
        &self.headers
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

#[cfg(test)]
mod tests {
    use super::{HeaderParser, HeaderState};

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
}
