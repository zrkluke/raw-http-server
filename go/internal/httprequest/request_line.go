package httprequest

import "bytes"

// RequestLine is the course-defined representation of an HTTP/1.1 request
// line. Target and method-token grammar are intentionally deferred.
type RequestLine struct {
	Method  string
	Target  string
	Version string
	Valid   bool
}

// ParseRequestLine validates one CRLF-free request line.
func ParseRequestLine(line []byte) RequestLine {
	if bytes.ContainsAny(line, "\r\n\t") {
		return RequestLine{}
	}

	parts := bytes.Split(line, []byte(" "))
	if len(parts) != 3 || len(parts[0]) == 0 || len(parts[1]) == 0 || len(parts[2]) == 0 {
		return RequestLine{}
	}
	if !bytes.Equal(parts[2], []byte("HTTP/1.1")) {
		return RequestLine{}
	}

	return RequestLine{
		Method:  string(parts[0]),
		Target:  string(parts[1]),
		Version: string(parts[2]),
		Valid:   true,
	}
}

// String returns the shared-fixture representation of a parsed request line.
func (line RequestLine) String() string {
	if !line.Valid {
		return "invalid"
	}

	return line.Method + "|" + line.Target + "|" + line.Version
}
