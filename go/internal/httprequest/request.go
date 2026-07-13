// Package httprequest tracks the framing state of an incremental HTTP request
// head. Request-line and header-field semantics are intentionally deferred to
// later milestones.
package httprequest

// State describes the observable result after feeding available TCP bytes.
type State uint8

const (
	// Incomplete means more bytes are required to finish or reject the head.
	Incomplete State = iota
	// Complete means a non-empty first line was followed by an empty CRLF line.
	Complete
	// Invalid means the input contains a line ending that is not strict CRLF.
	Invalid
)

func (s State) String() string {
	switch s {
	case Complete:
		return "complete"
	case Invalid:
		return "invalid"
	default:
		return "incomplete"
	}
}

// Parser incrementally recognizes the boundary of an HTTP request head.
type Parser struct {
	state        State
	sawFirstLine bool
	lineHasBytes bool
	pendingCR    bool
}

// NewParser returns a parser waiting for the first request-head byte.
func NewParser() *Parser {
	return &Parser{state: Incomplete}
}

// Feed consumes the next TCP byte chunk and returns the parser's state.
func (p *Parser) Feed(chunk []byte) State {
	if p.state != Incomplete {
		return p.state
	}

	for _, b := range chunk {
		if p.pendingCR {
			if b != '\n' {
				p.state = Invalid
				return p.state
			}
			p.pendingCR = false
			if !p.sawFirstLine {
				if !p.lineHasBytes {
					p.state = Invalid
					return p.state
				}
				p.sawFirstLine = true
			} else if !p.lineHasBytes {
				p.state = Complete
				return p.state
			}
			p.lineHasBytes = false
			continue
		}

		switch b {
		case '\r':
			p.pendingCR = true
		case '\n':
			p.state = Invalid
			return p.state
		default:
			p.lineHasBytes = true
		}
	}

	return p.state
}
