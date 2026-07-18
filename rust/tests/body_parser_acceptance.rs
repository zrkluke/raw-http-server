mod support;

use std::fs;
use std::path::PathBuf;

use raw_http_server_rust::{BodyParser, BodyState};

#[test]
fn reads_shared_body_fixtures() {
    let root = PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("../testdata");

    for name in [
        "absent",
        "complete",
        "partial",
        "invalid-length",
        "over-delivered",
    ] {
        let fixture = support::load(&root, "bodies", name).expect("shared fixture should load");
        let chunks = support::load_chunk_sizes(&root, "bodies", name, fixture.input.len())
            .expect("chunk sizes should load");
        let directory = root.join("bodies").join(name);
        let content_length = fs::read_to_string(directory.join("content-length.txt"))
            .expect("content length should load");
        let expected_state =
            fs::read_to_string(directory.join("state.txt")).expect("state should load");
        let expected_remaining =
            fs::read(directory.join("remaining.bin")).expect("remaining bytes should load");
        let content_length = content_length.trim();
        let expected_state = match expected_state.trim() {
            "incomplete" => BodyState::Incomplete,
            "complete" => BodyState::Complete,
            "invalid" => BodyState::Invalid,
            state => panic!("unexpected fixture state: {state}"),
        };
        let mut parser = BodyParser::new((content_length != "absent").then_some(content_length));
        let mut offset = 0;

        for size in chunks {
            let _ = parser.feed(&fixture.input[offset..offset + size]);
            offset += size;
        }

        assert_eq!(
            parser.state(),
            expected_state,
            "fixture {name} state differed"
        );
        assert_eq!(
            parser.body(),
            fixture.expected.as_slice(),
            "fixture {name} body differed"
        );
        assert_eq!(
            parser.remaining(),
            expected_remaining.as_slice(),
            "fixture {name} remaining bytes differed"
        );
    }
}
