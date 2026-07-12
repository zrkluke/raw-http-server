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

The repository marks `testdata/**` as `-text` in `.gitattributes`, preventing
Git from changing line endings on Windows.
