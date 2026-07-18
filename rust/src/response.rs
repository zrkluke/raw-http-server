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

    pub(crate) fn image_bmp_ok(body: Vec<u8>) -> Self {
        Self {
            status_code: 200,
            reason: "OK",
            content_type: "image/bmp",
            body,
        }
    }

    pub(crate) fn bad_request() -> Self {
        Self {
            status_code: 400,
            reason: "Bad Request",
            content_type: "text/plain",
            body: Vec::new(),
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

#[cfg(test)]
mod tests {
    use super::Response;

    #[test]
    fn basic_ok_serializes_a_byte_exact_response() {
        assert_eq!(
            Response::basic_ok().bytes(),
            b"HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 13\r\nConnection: close\r\n\r\nHello, World!"
        );
    }
}
