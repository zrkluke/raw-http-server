package tcpserver

import (
	"bytes"
	"fmt"
	"io"
	"net"
	"os"
	"path/filepath"
	"strconv"
	"strings"
	"testing"
	"time"
)

func TestEchoOnceContentLengthPreservesBinaryBody(t *testing.T) {
	payload := binaryEchoPayload(t)
	request := append([]byte(fmt.Sprintf(
		"POST /echo HTTP/1.1\r\nHost: fixture.test\r\nContent-Type: image/bmp\r\nContent-Length: %d\r\n\r\n",
		len(payload),
	)), payload...)

	response := sendEchoRequest(t, request)
	assertEchoResponse(t, response, payload)
}

func TestEchoOnceChunkedPreservesBinaryBody(t *testing.T) {
	payload := binaryEchoPayload(t)
	request := []byte("POST /echo HTTP/1.1\r\nHost: fixture.test\r\nContent-Type: image/bmp\r\nTransfer-Encoding: chunked\r\n\r\n")
	for _, part := range [][]byte{payload[:1], payload[1:18], payload[18:]} {
		request = append(request, []byte(fmt.Sprintf("%x\r\n", len(part)))...)
		request = append(request, part...)
		request = append(request, "\r\n"...)
	}
	request = append(request, "0\r\nX-Trailer: done\r\n\r\n"...)

	response := sendEchoRequest(t, request)
	assertEchoResponse(t, response, payload)
}

func TestEchoOnceRejectsAmbiguousBodyFraming(t *testing.T) {
	request := []byte("POST /echo HTTP/1.1\r\nHost: fixture.test\r\nContent-Length: 60\r\nTransfer-Encoding: chunked\r\n\r\n")

	response := sendEchoRequest(t, request)
	head, body := splitEchoResponse(t, response)
	if !strings.HasPrefix(head, "HTTP/1.1 400 Bad Request\r\n") {
		t.Fatalf("status = %q, want 400 Bad Request", head)
	}
	if len(body) != 0 {
		t.Fatalf("error body = %x, want empty", body)
	}
}

func binaryEchoPayload(t *testing.T) []byte {
	t.Helper()
	root, err := filepath.Abs(filepath.Join("..", "..", ".."))
	if err != nil {
		t.Fatal(err)
	}
	image, err := os.ReadFile(filepath.Join(root, "testdata", "binary", "echo-bmp", "image.bmp"))
	if err != nil {
		t.Fatal(err)
	}
	return append(image, '\r', '\n')
}

func sendEchoRequest(t *testing.T, request []byte) []byte {
	t.Helper()
	server, err := StartEchoOnce("127.0.0.1:0")
	if err != nil {
		t.Fatal(err)
	}
	connection, err := net.DialTimeout("tcp", server.Addr, time.Second)
	if err != nil {
		t.Fatal(err)
	}
	defer connection.Close()

	for offset, size := 0, 1; offset < len(request); size = size%7 + 1 {
		end := offset + size
		if end > len(request) {
			end = len(request)
		}
		if _, err := connection.Write(request[offset:end]); err != nil {
			t.Fatal(err)
		}
		offset = end
	}

	response, err := io.ReadAll(connection)
	if err != nil {
		t.Fatal(err)
	}
	if err := <-server.Done; err != nil {
		t.Fatal(err)
	}
	return response
}

func assertEchoResponse(t *testing.T, response, wantBody []byte) {
	t.Helper()
	head, body := splitEchoResponse(t, response)
	if !strings.HasPrefix(head, "HTTP/1.1 200 OK\r\n") {
		t.Fatalf("status = %q, want 200 OK", head)
	}
	if !strings.Contains(head, "Content-Type: image/bmp\r\n") {
		t.Fatalf("headers = %q, want image/bmp", head)
	}
	if !strings.Contains(head, "Content-Length: "+strconv.Itoa(len(wantBody))+"\r\n") {
		t.Fatalf("headers = %q, want content length %d", head, len(wantBody))
	}
	if !bytes.Equal(body, wantBody) {
		t.Fatalf("body = %x, want %x", body, wantBody)
	}
}

func splitEchoResponse(t *testing.T, response []byte) (string, []byte) {
	t.Helper()
	separator := bytes.Index(response, []byte("\r\n\r\n"))
	if separator < 0 {
		t.Fatalf("response has no header terminator: %x", response)
	}
	return string(response[:separator+4]), response[separator+4:]
}
