// Package httpresponse builds the limited HTTP/1.1 responses used in this
// course without relying on net/http.
package httpresponse

import (
	"io"
	"strconv"
	"strings"
)

// Header is one HTTP response header field.
type Header struct {
	Name  string
	Value string
}

// Response is a byte-oriented HTTP/1.1 response description.
type Response struct {
	statusCode int
	reason     string
	headers    []Header
	body       []byte
}

// New creates a response. Bytes serializes it with a generated Content-Length.
func New(statusCode int, reason string, headers []Header, body []byte) Response {
	return Response{
		statusCode: statusCode,
		reason:     reason,
		headers:    append([]Header(nil), headers...),
		body:       append([]byte(nil), body...),
	}
}

// BasicOK returns the course's fixed text response.
func BasicOK() Response {
	return New(200, "OK", []Header{
		{Name: "Content-Type", Value: "text/plain"},
		{Name: "Connection", Value: "close"},
	}, []byte("Hello, World!"))
}

// Bytes serializes status line, headers, a generated Content-Length, and body.
func (r Response) Bytes() []byte {
	bytes := make([]byte, 0, len(r.body)+128)
	bytes = append(bytes, "HTTP/1.1 "...)
	bytes = strconv.AppendInt(bytes, int64(r.statusCode), 10)
	bytes = append(bytes, ' ')
	bytes = append(bytes, r.reason...)
	bytes = append(bytes, '\r', '\n')

	connectionHeaders := make([]Header, 0, 1)
	for _, header := range r.headers {
		if strings.EqualFold(header.Name, "Content-Length") {
			continue
		}
		if strings.EqualFold(header.Name, "Connection") {
			connectionHeaders = append(connectionHeaders, header)
			continue
		}
		bytes = appendHeader(bytes, header)
	}

	bytes = append(bytes, "Content-Length: "...)
	bytes = strconv.AppendInt(bytes, int64(len(r.body)), 10)
	bytes = append(bytes, '\r', '\n')
	for _, header := range connectionHeaders {
		bytes = appendHeader(bytes, header)
	}
	bytes = append(bytes, '\r', '\n')
	return append(bytes, r.body...)
}

func appendHeader(bytes []byte, header Header) []byte {
	bytes = append(bytes, header.Name...)
	bytes = append(bytes, ':', ' ')
	bytes = append(bytes, header.Value...)
	return append(bytes, '\r', '\n')
}

// Write serializes response and continues until all bytes have been written.
func Write(writer io.Writer, response Response) error {
	bytes := response.Bytes()
	for len(bytes) > 0 {
		count, err := writer.Write(bytes)
		if err != nil {
			return err
		}
		if count == 0 {
			return io.ErrShortWrite
		}
		bytes = bytes[count:]
	}
	return nil
}
