package httpstream

import "bytes"

// LineReader accumulates TCP byte chunks until they contain CRLF-delimited
// lines. Feed returns complete lines without their CRLF terminators.
type LineReader struct {
	pending []byte
}

// Feed adds a TCP byte chunk and returns every complete line now available.
// Bytes after the final incomplete line remain buffered for a later call.
func (r *LineReader) Feed(chunk []byte) [][]byte {
	r.pending = append(r.pending, chunk...)

	var lines [][]byte
	for {
		index := bytes.Index(r.pending, []byte("\r\n"))
		if index < 0 {
			return lines
		}

		line := append([]byte(nil), r.pending[:index]...)
		lines = append(lines, line)
		r.pending = r.pending[index+2:]
	}
}
