# Step 5 shared header fixtures

Each `input.bin` starts with the first header field or the empty CRLF line that
terminates the header section. It does not include the request line.

`chunks.txt` lists positive decimal byte counts for incremental feeds. Complete
results serialize as `complete` followed by `lowercase-name|trimmed-value`
lines. `incomplete` and `invalid` contain only that state name.
