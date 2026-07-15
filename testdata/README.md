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

## Requests metadata

Fixtures under `testdata/requests/` use the same byte files and `chunks.txt`
metadata. Their `expected.bin` contains one ASCII state name followed by LF:
`incomplete`, `complete`, or `invalid`.

At this stage, `complete` means only that the request head ends with an empty
CRLF-delimited line. Request-line validation, header validation, and body
framing deliberately belong to later steps. The course parser uses strict
CRLF: a bare LF, or a confirmed bare CR, is invalid; a final CR is still
incomplete because the next chunk might contain LF.

The `complete-head` cases deliberately provide the same complete request head
under multiple partitions: the existing CRLF and field-boundary case,
`complete-head-whole` as one read, `complete-head-one-byte` as one byte per
read, and `complete-head-fixed` in fixed-size reads. Every case must produce
the same `complete` result.

## HTTP Headers metadata

Fixtures under `testdata/headers/` represent only the header-section bytes that
follow a request line; `input.bin` therefore starts at the first header field
or the terminating empty CRLF line. Their `chunks.txt` files use the same
positive decimal-byte-count convention as HTTP Streams.

For complete sections, `expected.bin` begins with `complete` and then has one
`lowercase-name|trimmed-value` line per parsed header. `incomplete` and
`invalid` are represented by their state name alone. The format is an
acceptance-test serialization, not an HTTP wire format.

## HTTP Body metadata

Fixtures under `testdata/bodies/` begin immediately after the terminating empty
header line. `content-length.txt` is test metadata describing the already parsed
`Content-Length` field: a decimal value is present, `absent` means the field was
not supplied, and any other value is invalid for this milestone.

`expected.bin` is the body accumulated by the parser, while `remaining.bin` is
the input that must remain unconsumed after the parser reaches its terminal
state. `state.txt` contains `incomplete`, `complete`, or `invalid`. This makes
the over-delivered case observable without treating the body as text.
