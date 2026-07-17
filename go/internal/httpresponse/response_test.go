package httpresponse

import (
	"bytes"
	"testing"
)

func TestBasicOKSerializesAByteExactResponse(t *testing.T) {
	expected := []byte("HTTP/1.1 200 OK\r\n" +
		"Content-Type: text/plain\r\n" +
		"Content-Length: 13\r\n" +
		"Connection: close\r\n" +
		"\r\n" +
		"Hello, World!")

	if actual := BasicOK().Bytes(); !bytes.Equal(actual, expected) {
		t.Fatalf("unexpected response: got %q, want %q", actual, expected)
	}
}
