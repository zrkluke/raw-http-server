package httprequest

import "strconv"

// BodyParser incrementally accumulates a request body framed by Content-Length.
type BodyParser struct {
	state     State
	length    uint64
	body      []byte
	remaining []byte
}

// NewBodyParser returns a body parser for an already parsed Content-Length.
// A nil value represents an absent Content-Length, so no body bytes are read.
func NewBodyParser(contentLength *string) *BodyParser {
	parser := &BodyParser{state: Complete}
	if contentLength == nil {
		return parser
	}

	length, ok := parseContentLength(*contentLength)
	if !ok {
		parser.state = Invalid
		return parser
	}

	parser.length = length
	if length > 0 {
		parser.state = Incomplete
	}
	return parser
}

// Feed consumes body bytes until Content-Length is reached and preserves any
// following bytes for the next message parser.
func (p *BodyParser) Feed(chunk []byte) State {
	if p.state == Invalid || p.state == Complete {
		p.remaining = append(p.remaining, chunk...)
		return p.state
	}

	needed := p.length - uint64(len(p.body))
	available := uint64(len(chunk))
	consume := available
	if consume > needed {
		consume = needed
	}

	p.body = append(p.body, chunk[:int(consume)]...)
	if consume < available {
		p.remaining = append(p.remaining, chunk[int(consume):]...)
	}
	if uint64(len(p.body)) == p.length {
		p.state = Complete
	}
	return p.state
}

// State reports whether the declared body is incomplete, complete, or invalid.
func (p *BodyParser) State() State {
	return p.state
}

// Body returns a copy of the body bytes accumulated so far.
func (p *BodyParser) Body() []byte {
	return append([]byte(nil), p.body...)
}

// Remaining returns a copy of bytes that belong after this request body.
func (p *BodyParser) Remaining() []byte {
	return append([]byte(nil), p.remaining...)
}

func parseContentLength(value string) (uint64, bool) {
	if value == "" {
		return 0, false
	}
	for _, byte := range []byte(value) {
		if byte < '0' || byte > '9' {
			return 0, false
		}
	}

	length, err := strconv.ParseUint(value, 10, 64)
	if err != nil || length > uint64(^uint(0)>>1) {
		return 0, false
	}
	return length, true
}
