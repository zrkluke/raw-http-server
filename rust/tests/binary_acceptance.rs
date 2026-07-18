use std::fs;
use std::io::{Read, Write};
use std::net::TcpStream;
use std::path::PathBuf;
use std::time::Duration;

use raw_http_server_rust::TcpServer;

#[test]
fn echo_once_preserves_a_content_length_binary_body() {
    let payload = binary_payload();
    let mut request = format!(
        "POST /echo HTTP/1.1\r\nHost: fixture.test\r\nContent-Type: image/bmp\r\nContent-Length: {}\r\n\r\n",
        payload.len()
    )
    .into_bytes();
    request.extend_from_slice(&payload);

    assert_echo_response(send_request(&request), &payload);
}

#[test]
fn echo_once_preserves_a_chunked_binary_body() {
    let payload = binary_payload();
    let mut request = b"POST /echo HTTP/1.1\r\nHost: fixture.test\r\nContent-Type: image/bmp\r\nTransfer-Encoding: chunked\r\n\r\n".to_vec();
    for part in [&payload[..1], &payload[1..18], &payload[18..]] {
        request.extend_from_slice(format!("{:x}\r\n", part.len()).as_bytes());
        request.extend_from_slice(part);
        request.extend_from_slice(b"\r\n");
    }
    request.extend_from_slice(b"0\r\nX-Trailer: done\r\n\r\n");

    assert_echo_response(send_request(&request), &payload);
}

#[test]
fn echo_once_rejects_ambiguous_body_framing() {
    let response = send_request(
        b"POST /echo HTTP/1.1\r\nHost: fixture.test\r\nContent-Length: 60\r\nTransfer-Encoding: chunked\r\n\r\n",
    );
    let (head, body) = split_response(&response);
    assert!(head.starts_with("HTTP/1.1 400 Bad Request\r\n"));
    assert!(body.is_empty());
}

fn binary_payload() -> Vec<u8> {
    let root = PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("../testdata");
    let mut payload =
        fs::read(root.join("binary/echo-bmp/image.bmp")).expect("fixture should load");
    payload.extend_from_slice(b"\r\n");
    payload
}

fn send_request(request: &[u8]) -> Vec<u8> {
    let server = TcpServer::start_echo_once("127.0.0.1:0").expect("listener should start");
    let mut client = TcpStream::connect_timeout(&server.address(), Duration::from_secs(1))
        .expect("client should connect");
    let mut offset = 0;
    let mut size = 1;
    while offset < request.len() {
        let end = (offset + size).min(request.len());
        client
            .write_all(&request[offset..end])
            .expect("fragment should send");
        offset = end;
        size = size % 7 + 1;
    }
    let mut response = Vec::new();
    client
        .read_to_end(&mut response)
        .expect("client should receive response");
    server
        .wait_timeout(Duration::from_secs(1))
        .expect("server should finish cleanly");
    response
}

fn assert_echo_response(response: Vec<u8>, expected_body: &[u8]) {
    let (head, body) = split_response(&response);
    assert!(head.starts_with("HTTP/1.1 200 OK\r\n"));
    assert!(head.contains("Content-Type: image/bmp\r\n"));
    assert!(head.contains(&format!("Content-Length: {}\r\n", expected_body.len())));
    assert_eq!(body, expected_body);
}

fn split_response(response: &[u8]) -> (&str, &[u8]) {
    let end = response
        .windows(4)
        .position(|window| window == b"\r\n\r\n")
        .expect("response should contain a header terminator")
        + 4;
    (
        std::str::from_utf8(&response[..end]).expect("headers should be ASCII"),
        &response[end..],
    )
}
