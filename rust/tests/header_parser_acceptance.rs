mod support;

use std::path::PathBuf;

use raw_http_server_rust::HeaderParser;

#[test]
fn reads_shared_header_fixtures() {
    let root = PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("../testdata");

    for name in [
        "valid-basic",
        "repeated-name",
        "case-insensitive",
        "optional-whitespace",
        "empty-section",
        "fragmented",
        "incomplete-value",
        "missing-colon",
        "leading-whitespace",
        "bare-lf",
        "bare-cr",
    ] {
        let fixture = support::load(&root, "headers", name).expect("shared fixture should load");
        let chunks = support::load_chunk_sizes(&root, "headers", name, fixture.input.len())
            .expect("chunk sizes should load");
        let mut parser = HeaderParser::new();
        let mut offset = 0;

        for size in chunks {
            let _ = parser.feed(&fixture.input[offset..offset + size]);
            offset += size;
        }

        let actual = format!("{parser}\n");
        assert_eq!(
            actual.as_bytes(),
            fixture.expected,
            "fixture {name} differed"
        );
    }
}
