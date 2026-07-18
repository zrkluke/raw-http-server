use std::io::{self, Read, Write};
use std::net::{SocketAddr, TcpListener, TcpStream};
use std::sync::mpsc::{self, Receiver, RecvTimeoutError, SyncSender};
use std::thread;
use std::time::Duration;

use crate::{
    BodyParser, BodyState, ChunkedParser, ChunkedState, Header, HeaderParser, HeaderState,
    RequestLine, RequestParser, RequestState, Response,
};

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

    /// Binds an address and serves one fully framed POST /echo request.
    pub fn start_echo_once(address: &str) -> io::Result<Self> {
        let listener = TcpListener::bind(address)?;
        let address = listener.local_addr()?;
        let (_received_sender, received) = mpsc::sync_channel(1);
        let (done_sender, done) = mpsc::sync_channel(1);

        let _server_thread = thread::spawn(move || {
            let result = serve_echo_once(listener);
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

fn invalid_request() -> io::Error {
    io::Error::new(
        io::ErrorKind::InvalidData,
        "invalid course HTTP/1.1 request",
    )
}

fn serve_echo_once(listener: TcpListener) -> io::Result<()> {
    let (mut connection, _) = listener.accept()?;
    drop(listener);

    match read_echo_request(&mut connection) {
        Ok(body) => connection.write_all(&Response::image_bmp_ok(body).bytes()),
        Err(error) if error.kind() == io::ErrorKind::InvalidData => {
            connection.write_all(&Response::bad_request().bytes())
        }
        Err(error) => Err(error),
    }
}

fn read_echo_request(connection: &mut TcpStream) -> io::Result<Vec<u8>> {
    let (head, initial) = read_request_head(connection)?;
    let line_end = head
        .windows(2)
        .position(|window| window == b"\r\n")
        .ok_or_else(invalid_request)?;
    let line = RequestLine::parse(&head[..line_end]).map_err(|_| invalid_request())?;
    if line.method != b"POST" || line.target != b"/echo" || line.version != b"HTTP/1.1" {
        return Err(invalid_request());
    }
    let mut headers = HeaderParser::new();
    if headers.feed(&head[line_end + 2..]) != HeaderState::Complete {
        return Err(invalid_request());
    }
    match select_echo_framing(headers.headers())? {
        EchoFraming::ContentLength(value) => read_content_length_body(connection, initial, &value),
        EchoFraming::Chunked => read_chunked_body(connection, initial),
    }
}

fn read_request_head(connection: &mut TcpStream) -> io::Result<(Vec<u8>, Vec<u8>)> {
    let mut request = Vec::new();
    let mut buffer = [0_u8; 4096];
    loop {
        if let Some(end) = request.windows(4).position(|window| window == b"\r\n\r\n") {
            let end = end + 4;
            let mut parser = RequestParser::new();
            if parser.feed(&request[..end]) != RequestState::Complete {
                return Err(invalid_request());
            }
            return Ok((request[..end].to_vec(), request[end..].to_vec()));
        }
        let count = connection.read(&mut buffer)?;
        if count == 0 {
            return Err(invalid_request());
        }
        request.extend_from_slice(&buffer[..count]);
    }
}

enum EchoFraming {
    ContentLength(Vec<u8>),
    Chunked,
}

fn select_echo_framing(headers: &[Header]) -> io::Result<EchoFraming> {
    let mut content_length = None;
    let mut transfer_encoding: Option<&[u8]> = None;
    for header in headers {
        if header.name() == b"content-length" {
            if content_length.is_some() {
                return Err(invalid_request());
            }
            content_length = Some(header.value().to_vec());
        } else if header.name() == b"transfer-encoding" {
            if transfer_encoding.is_some() {
                return Err(invalid_request());
            }
            transfer_encoding = Some(header.value());
        }
    }
    match (content_length, transfer_encoding) {
        (Some(_), Some(_)) | (None, None) => Err(invalid_request()),
        (Some(value), None) => Ok(EchoFraming::ContentLength(value)),
        (None, Some(value)) if value.eq_ignore_ascii_case(b"chunked") => Ok(EchoFraming::Chunked),
        (None, Some(_)) => Err(invalid_request()),
    }
}

fn read_content_length_body(
    connection: &mut TcpStream,
    initial: Vec<u8>,
    value: &[u8],
) -> io::Result<Vec<u8>> {
    let value = std::str::from_utf8(value).map_err(|_| invalid_request())?;
    let mut parser = BodyParser::new(Some(value));
    let mut state = parser.feed(&initial);
    let mut buffer = [0_u8; 4096];
    while state == BodyState::Incomplete {
        let count = connection.read(&mut buffer)?;
        if count == 0 {
            return Err(invalid_request());
        }
        state = parser.feed(&buffer[..count]);
    }
    if state != BodyState::Complete {
        return Err(invalid_request());
    }
    Ok(parser.body().to_vec())
}

fn read_chunked_body(connection: &mut TcpStream, initial: Vec<u8>) -> io::Result<Vec<u8>> {
    let mut parser = ChunkedParser::new();
    let mut state = parser.feed(&initial);
    let mut buffer = [0_u8; 4096];
    while state == ChunkedState::Incomplete {
        let count = connection.read(&mut buffer)?;
        if count == 0 {
            return Err(invalid_request());
        }
        state = parser.feed(&buffer[..count]);
    }
    if state != ChunkedState::Complete {
        return Err(invalid_request());
    }
    Ok(parser.body().to_vec())
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
    use super::TcpServer;

    #[test]
    fn tcp_server_rejects_an_invalid_address() {
        assert!(TcpServer::start_once("not-a-tcp-address").is_err());
    }
}
