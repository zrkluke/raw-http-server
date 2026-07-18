/// A course-defined HTTP/1.1 request line with owned byte fields.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct RequestLine {
    pub(crate) method: Vec<u8>,
    pub(crate) target: Vec<u8>,
    pub(crate) version: Vec<u8>,
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

#[cfg(test)]
mod tests {
    use super::RequestLine;

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
