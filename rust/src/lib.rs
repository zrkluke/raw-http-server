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
    use super::{LineReader, TcpServer};

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
}
