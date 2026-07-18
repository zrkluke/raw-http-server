mod support;

use std::path::PathBuf;

use raw_http_server_rust::{LineReader, RequestLine};

#[test]
fn parses_shared_request_line_fixtures() {
    let root = PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("../testdata");

    for name in [
        "valid-basic",
        "valid-query",
        "valid-crlf-boundary",
        "valid-field-split",
        "valid-fixed-four",
        "valid-one-byte",
        "missing-target",
        "leading-space",
        "trailing-space",
        "double-space",
        "tab-separator",
        "unsupported-version",
    ] {
        let fixture =
            support::load(&root, "request-lines", name).expect("shared fixture should load");
        let chunks = support::load_chunk_sizes(&root, "request-lines", name, fixture.input.len())
            .expect("chunk sizes should load");
        let mut reader = LineReader::new();
        let mut offset = 0;
        let mut lines = Vec::new();

        for size in chunks {
            lines.extend(reader.feed(&fixture.input[offset..offset + size]));
            offset += size;
        }

        let actual = match lines.as_slice() {
            [line] => RequestLine::parse(line)
                .map(|request_line| request_line.to_string())
                .unwrap_or_else(|_| String::from("invalid")),
            _ => String::from("invalid"),
        };

        assert_eq!(
            format!("{actual}\n").as_bytes(),
            fixture.expected,
            "fixture {name} differed"
        );
    }
}
