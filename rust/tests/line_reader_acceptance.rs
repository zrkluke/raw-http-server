mod support;

use std::path::PathBuf;

use raw_http_server_rust::LineReader;

#[test]
fn reads_shared_crlf_stream_fixtures() {
    let root = PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("../testdata");

    for name in ["complete-line", "crlf-split", "one-byte"] {
        let fixture =
            support::load(&root, "http-streams", name).expect("shared fixture should load");
        let chunks = support::load_chunk_sizes(&root, "http-streams", name, fixture.input.len())
            .expect("chunk sizes should load");
        let mut reader = LineReader::new();
        let mut actual = Vec::new();
        let mut offset = 0;

        for size in chunks {
            for line in reader.feed(&fixture.input[offset..offset + size]) {
                actual.extend_from_slice(&line);
                actual.push(b'\n');
            }
            offset += size;
        }

        assert_eq!(actual, fixture.expected, "fixture {name} differed");
    }
}
