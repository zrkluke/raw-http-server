mod support;

use std::fs;
use std::path::PathBuf;

use raw_http_server_rust::{ChunkedParser, ChunkedState};

#[test]
fn reads_shared_chunked_fixtures() {
    let root = PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("../testdata");

    for name in [
        "basic",
        "fragmented-trailers",
        "invalid-size",
        "missing-data-delimiter",
    ] {
        let fixture = support::load(&root, "chunked", name).expect("shared fixture should load");
        let chunks = support::load_chunk_sizes(&root, "chunked", name, fixture.input.len())
            .expect("chunk sizes should load");
        let directory = root.join("chunked").join(name);
        let expected_state =
            fs::read_to_string(directory.join("state.txt")).expect("state should load");
        let expected_trailers =
            fs::read_to_string(directory.join("trailers.txt")).expect("trailers should load");
        let expected_remaining =
            fs::read(directory.join("remaining.bin")).expect("remaining bytes should load");
        let expected_state = match expected_state.trim() {
            "incomplete" => ChunkedState::Incomplete,
            "complete" => ChunkedState::Complete,
            "invalid" => ChunkedState::Invalid,
            state => panic!("unexpected fixture state: {state}"),
        };
        let mut parser = ChunkedParser::new();
        let mut offset = 0;

        for size in chunks {
            let _ = parser.feed(&fixture.input[offset..offset + size]);
            offset += size;
        }

        let actual_trailers = parser
            .trailers()
            .iter()
            .map(|header| {
                format!(
                    "{}|{}",
                    String::from_utf8_lossy(header.name()),
                    String::from_utf8_lossy(header.value())
                )
            })
            .collect::<Vec<_>>()
            .join("\n");
        let expected_trailers = expected_trailers.trim_end();

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
            actual_trailers, expected_trailers,
            "fixture {name} trailers differed"
        );
        assert_eq!(
            parser.remaining(),
            expected_remaining.as_slice(),
            "fixture {name} remaining bytes differed"
        );
    }
}
