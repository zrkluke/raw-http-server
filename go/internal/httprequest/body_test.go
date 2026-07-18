package httprequest

import (
	"bytes"
	"testing"
)

func TestBodyParserAbsentContentLengthPreservesBytes(t *testing.T) {
	parser := NewBodyParser(nil)
	if got := parser.Feed([]byte("next")); got != Complete {
		t.Fatalf("state: got %s, want %s", got, Complete)
	}
	if got := parser.Body(); len(got) != 0 {
		t.Fatalf("body: got %q, want empty", got)
	}
	if got := parser.Remaining(); !bytes.Equal(got, []byte("next")) {
		t.Fatalf("remaining: got %q, want %q", got, "next")
	}
}

func TestBodyParserAccumulatesAcrossChunks(t *testing.T) {
	length := "5"
	parser := NewBodyParser(&length)
	if got := parser.Feed([]byte("he")); got != Incomplete {
		t.Fatalf("state after first chunk: got %s, want %s", got, Incomplete)
	}
	if got := parser.Feed([]byte("llo")); got != Complete {
		t.Fatalf("state after second chunk: got %s, want %s", got, Complete)
	}
	if got := parser.Body(); !bytes.Equal(got, []byte("hello")) {
		t.Fatalf("body: got %q, want %q", got, "hello")
	}
}

func TestBodyParserRejectsInvalidContentLength(t *testing.T) {
	length := "-1"
	parser := NewBodyParser(&length)
	if got := parser.Feed([]byte("body")); got != Invalid {
		t.Fatalf("state: got %s, want %s", got, Invalid)
	}
	if got := parser.Remaining(); !bytes.Equal(got, []byte("body")) {
		t.Fatalf("remaining: got %q, want %q", got, "body")
	}
}

func TestBodyParserPreservesOverDeliveredBytes(t *testing.T) {
	length := "3"
	parser := NewBodyParser(&length)
	if got := parser.Feed([]byte("catnext")); got != Complete {
		t.Fatalf("state: got %s, want %s", got, Complete)
	}
	if got := parser.Body(); !bytes.Equal(got, []byte("cat")) {
		t.Fatalf("body: got %q, want %q", got, "cat")
	}
	if got := parser.Remaining(); !bytes.Equal(got, []byte("next")) {
		t.Fatalf("remaining: got %q, want %q", got, "next")
	}
}
