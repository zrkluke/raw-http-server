mod support;

use std::path::PathBuf;

use raw_http_server_rust::RequestParser;

#[test]
fn reads_shared_incremental_request_fixtures() {
    let root = PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("../testdata");

    for name in [
        "incomplete-head",
        "complete-head",
        "complete-head-whole",
        "complete-head-one-byte",
        "complete-head-fixed",
        "bare-lf",
    ] {
        let fixture = support::load(&root, "requests", name).expect("shared fixture should load");
        let chunks = support::load_chunk_sizes(&root, "requests", name, fixture.input.len())
            .expect("chunk sizes should load");
        let mut parser = RequestParser::new();
        let mut offset = 0;

        for size in chunks {
            let _ = parser.feed(&fixture.input[offset..offset + size]);
            offset += size;
        }

        let actual = format!("{}\n", parser.state());

        assert_eq!(
            actual.as_bytes(),
            fixture.expected,
            "fixture {name} differed"
        );
    }
}
