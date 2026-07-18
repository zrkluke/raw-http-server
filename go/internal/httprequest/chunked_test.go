package httprequest

import (
	"bytes"
	"testing"
)

func TestChunkedParserRejectsBareLineFeed(t *testing.T) {
	parser := NewChunkedParser()

	if got := parser.Feed([]byte("1\n")); got != Invalid {
		t.Fatalf("state = %s, want invalid", got)
	}
}

func TestChunkedParserPreservesBytesAfterTerminator(t *testing.T) {
	parser := NewChunkedParser()
	parser.Feed([]byte("1\r\na\r\n0\r\n\r\nnext"))

	if got := parser.State(); got != Complete {
		t.Fatalf("state = %s, want complete", got)
	}
	if got, want := parser.Body(), []byte("a"); !bytes.Equal(got, want) {
		t.Fatalf("body = %q, want %q", got, want)
	}
	if got, want := parser.Remaining(), []byte("next"); !bytes.Equal(got, want) {
		t.Fatalf("remaining = %q, want %q", got, want)
	}
}
