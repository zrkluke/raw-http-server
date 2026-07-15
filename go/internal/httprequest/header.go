package httprequest

import "strings"

// Header is one normalized HTTP header field for this course milestone.
type Header struct {
	Name  string
	Value string
}

// HeaderParser incrementally parses CRLF-delimited HTTP header fields.
type HeaderParser struct {
	state     State
	line      []byte
	pendingCR bool
	headers   []Header
}

// NewHeaderParser returns a parser waiting for the first header byte.
func NewHeaderParser() *HeaderParser {
	return &HeaderParser{state: Incomplete}
}

// Feed consumes the next TCP byte chunk and returns the parser state.
func (p *HeaderParser) Feed(chunk []byte) State {
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
			if len(p.line) == 0 {
				p.state = Complete
				return p.state
			}
			if !p.addHeader(p.line) {
				p.state = Invalid
				return p.state
			}
			p.line = p.line[:0]
			continue
		}

		switch b {
		case '\r':
			p.pendingCR = true
		case '\n':
			p.state = Invalid
			return p.state
		default:
			p.line = append(p.line, b)
		}
	}

	return p.state
}

// Headers returns a copy of the normalized fields parsed so far.
func (p *HeaderParser) Headers() []Header {
	return append([]Header(nil), p.headers...)
}

// String serializes the observable parser result used by shared fixtures.
func (p *HeaderParser) String() string {
	if p.state != Complete {
		return p.state.String()
	}

	lines := make([]string, 1, len(p.headers)+1)
	lines[0] = Complete.String()
	for _, header := range p.headers {
		lines = append(lines, header.Name+"|"+header.Value)
	}
	return strings.Join(lines, "\n")
}

func (p *HeaderParser) addHeader(line []byte) bool {
	if line[0] == ' ' || line[0] == '\t' {
		return false
	}

	separator := -1
	for index, b := range line {
		if b == ':' {
			separator = index
			break
		}
		if b <= ' ' || b == '\x7f' {
			return false
		}
	}
	if separator <= 0 {
		return false
	}

	name := make([]byte, separator)
	for index, b := range line[:separator] {
		if b >= 'A' && b <= 'Z' {
			name[index] = b + ('a' - 'A')
		} else {
			name[index] = b
		}
	}

	value := strings.Trim(string(line[separator+1:]), " \t")
	p.headers = append(p.headers, Header{Name: string(name), Value: value})
	return true
}
