mod support;

use std::io::Write;
use std::net::TcpStream;
use std::path::PathBuf;
use std::time::Duration;

use raw_http_server_rust::TcpServer;

#[test]
fn start_once_transfers_bytes_and_releases_the_listener() {
    let root = PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("../testdata");
    let fixture =
        support::load(&root, "tcp", "listen-connect").expect("shared fixture should load");
    let server = TcpServer::start_once("127.0.0.1:0").expect("listener should start");
    let mut client = TcpStream::connect_timeout(&server.address(), Duration::from_secs(1))
        .expect("client should connect");

    client
        .write_all(&fixture.input)
        .expect("client should send fixture input");
    drop(client);

    assert_eq!(
        server
            .receive_timeout(Duration::from_secs(1))
            .expect("server should receive bytes"),
        fixture.expected
    );
    server
        .wait_timeout(Duration::from_secs(1))
        .expect("server should finish cleanly");
    assert!(TcpStream::connect_timeout(&server.address(), Duration::from_secs(1)).is_err());
}
