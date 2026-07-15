Step 4 shared request-line fixtures.

Each input.bin is raw TCP bytes and ends in CRLF. chunks.txt describes the
decimal byte count supplied to each LineReader feed. The valid cases cover
whole input, CRLF, field-boundary, fixed-size, and one-byte partitions.
expected.bin is the observable parser result in the form
method|target|version, or invalid.
