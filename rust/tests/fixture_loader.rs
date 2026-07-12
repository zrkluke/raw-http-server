mod support;

use std::path::PathBuf;

#[test]
fn loads_shared_fixture() {
    let root = PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("../testdata");
    let fixture =
        support::load(&root, "foundation", "loader-smoke").expect("shared fixture should load");

    assert_eq!(fixture.input, b"shared fixture input\n");
    assert_eq!(fixture.expected, b"shared fixture expected\n");
}
