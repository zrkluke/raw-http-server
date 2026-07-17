mod support;

use std::io::{Read, Write};
use std::net::TcpStream;
use std::path::PathBuf;
use std::time::Duration;

use raw_http_server_rust::TcpServer;

#[test]
fn sends_the_shared_basic_response_over_tcp() {
    let root = PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("../testdata");
    let fixture = support::load(&root, "responses", "basic").expect("fixture should load");
    let server = TcpServer::start_response_once("127.0.0.1:0").expect("listener should start");
    let mut client = TcpStream::connect_timeout(&server.address(), Duration::from_secs(1))
        .expect("client should connect");

    client
        .write_all(&fixture.input)
        .expect("client should send request");

    let mut response = Vec::new();
    client
        .read_to_end(&mut response)
        .expect("client should receive response");
    assert_eq!(response, fixture.expected);

    server
        .wait_timeout(Duration::from_secs(1))
        .expect("server should finish cleanly");
}
