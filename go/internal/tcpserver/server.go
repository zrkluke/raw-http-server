package tcpserver

import (
	"bytes"
	"errors"
	"io"
	"net"
	"strings"

	"github.com/zrkluke/raw-http-server/go/internal/httprequest"
	"github.com/zrkluke/raw-http-server/go/internal/httpresponse"
)

var errInvalidRequest = errors.New("invalid course HTTP/1.1 request")

// Server exposes the observable result of one accepted TCP connection.
type Server struct {
	Addr     string
	Received <-chan []byte
	Done     <-chan error
}

// StartOnce listens on address, accepts one client, reads until it closes, and
// then releases both the client connection and listener.
func StartOnce(address string) (Server, error) {
	listener, err := net.Listen("tcp", address)
	if err != nil {
		return Server{}, err
	}

	received := make(chan []byte, 1)
	done := make(chan error, 1)
	server := Server{
		Addr:     listener.Addr().String(),
		Received: received,
		Done:     done,
	}

	go serveOnce(listener, received, done)
	return server, nil
}

// StartResponseOnce accepts one request, writes the course's basic HTTP/1.1
// response, then releases the connection and listener.
func StartResponseOnce(address string) (Server, error) {
	return startResponseOnce(address, httpresponse.BasicOK())
}

// StartEchoOnce serves one complete, framed POST /echo request. It is the
// connection-level integration point for the course's HTTP/1.1 components.
func StartEchoOnce(address string) (Server, error) {
	listener, err := net.Listen("tcp", address)
	if err != nil {
		return Server{}, err
	}

	received := make(chan []byte, 1)
	done := make(chan error, 1)
	server := Server{
		Addr:     listener.Addr().String(),
		Received: received,
		Done:     done,
	}

	go serveEchoOnce(listener, received, done)
	return server, nil
}

func startResponseOnce(address string, response httpresponse.Response) (Server, error) {
	listener, err := net.Listen("tcp", address)
	if err != nil {
		return Server{}, err
	}

	received := make(chan []byte, 1)
	done := make(chan error, 1)
	server := Server{
		Addr:     listener.Addr().String(),
		Received: received,
		Done:     done,
	}

	go serveResponseOnce(listener, response, received, done)
	return server, nil
}

func serveOnce(listener net.Listener, received chan<- []byte, done chan<- error) {
	connection, err := listener.Accept()
	if err != nil {
		_ = listener.Close()
		done <- err
		return
	}
	if err := listener.Close(); err != nil {
		_ = connection.Close()
		done <- err
		return
	}
	defer connection.Close()

	bytes, err := io.ReadAll(connection)
	if err == nil {
		received <- bytes
	}
	done <- err
}

func serveResponseOnce(listener net.Listener, response httpresponse.Response, received chan<- []byte, done chan<- error) {
	connection, err := listener.Accept()
	if err != nil {
		_ = listener.Close()
		done <- err
		return
	}
	if err := listener.Close(); err != nil {
		_ = connection.Close()
		done <- err
		return
	}
	defer connection.Close()

	buffer := make([]byte, 4096)
	count, readErr := connection.Read(buffer)
	if count > 0 {
		received <- append([]byte(nil), buffer[:count]...)
	}
	if readErr != nil && readErr != io.EOF {
		done <- readErr
		return
	}
	if count == 0 {
		done <- io.ErrUnexpectedEOF
		return
	}

	done <- httpresponse.Write(connection, response)
}

func serveEchoOnce(listener net.Listener, received chan<- []byte, done chan<- error) {
	connection, err := listener.Accept()
	if err != nil {
		_ = listener.Close()
		done <- err
		return
	}
	if err := listener.Close(); err != nil {
		_ = connection.Close()
		done <- err
		return
	}
	defer connection.Close()

	body, request, err := readEchoRequest(connection)
	if len(request) > 0 {
		received <- request
	}
	if err != nil {
		if errors.Is(err, errInvalidRequest) {
			done <- writeBadRequest(connection)
			return
		}
		done <- err
		return
	}

	done <- httpresponse.Write(connection, httpresponse.New(200, "OK", []httpresponse.Header{
		{Name: "Content-Type", Value: "image/bmp"},
		{Name: "Connection", Value: "close"},
	}, body))
}

func readEchoRequest(connection net.Conn) ([]byte, []byte, error) {
	head, bodyBytes, request, err := readRequestHead(connection)
	if err != nil {
		return nil, request, err
	}

	lineEnd := bytes.Index(head, []byte("\r\n"))
	if lineEnd < 0 {
		return nil, request, errInvalidRequest
	}
	line := httprequest.ParseRequestLine(head[:lineEnd])
	if line.Method != "POST" || line.Target != "/echo" || line.Version != "HTTP/1.1" {
		return nil, request, errInvalidRequest
	}

	headParser := httprequest.NewHeaderParser()
	if headParser.Feed(head[lineEnd+2:]) != httprequest.Complete {
		return nil, request, errInvalidRequest
	}

	framing, contentLength, err := selectRequestFraming(headParser.Headers())
	if err != nil {
		return nil, request, err
	}
	if framing == "content-length" {
		return readContentLengthBody(connection, bodyBytes, request, contentLength)
	}
	return readChunkedBody(connection, bodyBytes, request)
}

func readRequestHead(connection net.Conn) (head, body, request []byte, err error) {
	buffer := make([]byte, 4096)
	for {
		if end := bytes.Index(request, []byte("\r\n\r\n")); end >= 0 {
			end += len("\r\n\r\n")
			parser := httprequest.NewParser()
			if parser.Feed(request[:end]) != httprequest.Complete {
				return nil, nil, request, errInvalidRequest
			}
			return request[:end], request[end:], request, nil
		}
		count, readErr := connection.Read(buffer)
		if count > 0 {
			request = append(request, buffer[:count]...)
		}
		if readErr != nil {
			if readErr == io.EOF {
				return nil, nil, request, errInvalidRequest
			}
			return nil, nil, request, readErr
		}
	}
}

func selectRequestFraming(headers []httprequest.Header) (string, *string, error) {
	var contentLength *string
	transferEncodingCount := 0
	transferEncoding := ""

	for _, header := range headers {
		switch header.Name {
		case "content-length":
			if contentLength != nil {
				return "", nil, errInvalidRequest
			}
			value := header.Value
			contentLength = &value
		case "transfer-encoding":
			transferEncodingCount++
			transferEncoding = header.Value
		}
	}

	if contentLength != nil && transferEncodingCount != 0 {
		return "", nil, errInvalidRequest
	}
	if contentLength != nil {
		return "content-length", contentLength, nil
	}
	if transferEncodingCount == 1 && strings.EqualFold(transferEncoding, "chunked") {
		return "chunked", nil, nil
	}
	return "", nil, errInvalidRequest
}

func readContentLengthBody(connection net.Conn, initial, request []byte, contentLength *string) ([]byte, []byte, error) {
	parser := httprequest.NewBodyParser(contentLength)
	state := parser.Feed(initial)
	for state == httprequest.Incomplete {
		chunk, err := readRequestChunk(connection)
		if err != nil {
			return nil, request, err
		}
		request = append(request, chunk...)
		state = parser.Feed(chunk)
	}
	if state != httprequest.Complete {
		return nil, request, errInvalidRequest
	}
	return parser.Body(), request, nil
}

func readChunkedBody(connection net.Conn, initial, request []byte) ([]byte, []byte, error) {
	parser := httprequest.NewChunkedParser()
	state := parser.Feed(initial)
	for state == httprequest.Incomplete {
		chunk, err := readRequestChunk(connection)
		if err != nil {
			return nil, request, err
		}
		request = append(request, chunk...)
		state = parser.Feed(chunk)
	}
	if state != httprequest.Complete {
		return nil, request, errInvalidRequest
	}
	return parser.Body(), request, nil
}

func readRequestChunk(connection net.Conn) ([]byte, error) {
	buffer := make([]byte, 4096)
	count, err := connection.Read(buffer)
	if count > 0 {
		return append([]byte(nil), buffer[:count]...), nil
	}
	if err == io.EOF {
		return nil, errInvalidRequest
	}
	if err != nil {
		return nil, err
	}
	return nil, errInvalidRequest
}

func writeBadRequest(connection net.Conn) error {
	return httpresponse.Write(connection, httpresponse.New(400, "Bad Request", []httpresponse.Header{
		{Name: "Content-Type", Value: "text/plain"},
		{Name: "Connection", Value: "close"},
	}, nil))
}
