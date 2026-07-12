# Shared test fixtures

`testdata/` stores language-neutral, byte-exact fixtures consumed by C, Go,
and Rust tests.

Each fixture uses this structure:

```text
testdata/<group>/<case>/
├── input.bin
└── expected.bin
```

- `<group>` identifies the foundation or milestone that owns the fixture.
- `<case>` is a stable, descriptive case identifier written in kebab-case.
- `input.bin` contains the exact bytes supplied to the behavior under test.
- `expected.bin` contains the exact expected output bytes.
- Empty files and non-text bytes are valid; loaders must not decode or
  normalize their contents.
- Milestone-specific metadata may be added as separate documented files, but
  the two byte files remain the common baseline.

## HTTP Streams metadata

Fixtures under `testdata/http-streams/` also contain `chunks.txt`. Each line is
a positive decimal byte count describing one call that feeds the next bytes
from `input.bin` into the incremental line reader. The counts must sum exactly
to the size of `input.bin`.

For this milestone, `expected.bin` serializes every emitted line by appending a
single LF byte to the line content. The LF belongs to the acceptance output
format; the input delimiter remains the exact CRLF bytes stored in `input.bin`.

The initial cases exercise the same logical line with these partitions:

- `complete-line`: the complete input in one chunk.
- `crlf-split`: CR ends one chunk and LF begins the next.
- `one-byte`: every byte arrives in a separate chunk.

The repository marks `testdata/**` as `-text` in `.gitattributes`, preventing
Git from changing line endings on Windows.
