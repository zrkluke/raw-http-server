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

#[cfg(test)]
mod tests {
    use super::LineReader;

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
}
