package httprequest

import (
	"strconv"
)

type chunkedPhase uint8

const (
	chunkSizeLine chunkedPhase = iota
	chunkData
	chunkDataCR
	chunkDataLF
	chunkTrailers
)

// ChunkedParser incrementally decodes the course-defined HTTP/1.1 chunked
// transfer coding. It accepts hexadecimal sizes, a zero-size terminator, and
// trailers using the same grammar as request headers.
type ChunkedParser struct {
	state       State
	phase       chunkedPhase
	line        []byte
	pendingCR   bool
	chunkRemain int
	body        []byte
	trailers    []Header
	remaining   []byte
}

// NewChunkedParser returns a parser waiting for the first chunk-size line.
func NewChunkedParser() *ChunkedParser {
	return &ChunkedParser{state: Incomplete, phase: chunkSizeLine}
}

// Feed consumes available transfer-coding bytes and returns the parser state.
func (p *ChunkedParser) Feed(chunk []byte) State {
	if p.state == Complete {
		p.remaining = append(p.remaining, chunk...)
		return p.state
	}
	if p.state == Invalid {
		return p.state
	}

	for index, b := range chunk {
		switch p.phase {
		case chunkData:
			p.body = append(p.body, b)
			p.chunkRemain--
			if p.chunkRemain == 0 {
				p.phase = chunkDataCR
			}
		case chunkDataCR:
			if b != '\r' {
				p.state = Invalid
				return p.state
			}
			p.phase = chunkDataLF
		case chunkDataLF:
			if b != '\n' {
				p.state = Invalid
				return p.state
			}
			p.phase = chunkSizeLine
		case chunkSizeLine:
			if !p.consumeLineByte(b, p.finishChunkSize) {
				return p.state
			}
		case chunkTrailers:
			if !p.consumeLineByte(b, p.finishTrailer) {
				return p.state
			}
		}

		if p.state == Complete {
			p.remaining = append(p.remaining, chunk[index+1:]...)
			return p.state
		}
	}

	return p.state
}

// State reports whether the transfer coding is incomplete, complete, or invalid.
func (p *ChunkedParser) State() State {
	return p.state
}

// Body returns a copy of the decoded payload bytes.
func (p *ChunkedParser) Body() []byte {
	return append([]byte(nil), p.body...)
}

// Trailers returns a copy of the normalized trailer fields.
func (p *ChunkedParser) Trailers() []Header {
	return append([]Header(nil), p.trailers...)
}

// Remaining returns bytes received after the terminating trailer section.
func (p *ChunkedParser) Remaining() []byte {
	return append([]byte(nil), p.remaining...)
}

func (p *ChunkedParser) consumeLineByte(b byte, finish func([]byte) bool) bool {
	if p.pendingCR {
		if b != '\n' {
			p.state = Invalid
			return false
		}
		p.pendingCR = false
		if !finish(p.line) {
			p.state = Invalid
			return false
		}
		p.line = p.line[:0]
		return true
	}

	switch b {
	case '\r':
		p.pendingCR = true
	case '\n':
		p.state = Invalid
		return false
	default:
		p.line = append(p.line, b)
	}
	return true
}

func (p *ChunkedParser) finishChunkSize(line []byte) bool {
	if len(line) == 0 {
		return false
	}

	length, err := strconv.ParseUint(string(line), 16, 64)
	if err != nil || length > uint64(^uint(0)>>1) {
		return false
	}
	if length == 0 {
		p.phase = chunkTrailers
		return true
	}

	p.chunkRemain = int(length)
	p.phase = chunkData
	return true
}

func (p *ChunkedParser) finishTrailer(line []byte) bool {
	if len(line) == 0 {
		p.state = Complete
		return true
	}

	parser := NewHeaderParser()
	if !parser.addHeader(line) {
		return false
	}
	p.trailers = append(p.trailers, parser.Headers()...)
	return true
}
