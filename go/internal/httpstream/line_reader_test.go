package httpstream

import (
	"bytes"
	"testing"
)

func TestLineReaderFeed(t *testing.T) {
	tests := []struct {
		name   string
		chunks [][]byte
		want   [][]byte
	}{
		{
			name:   "waits for a complete line",
			chunks: [][]byte{[]byte("hello")},
			want:   nil,
		},
		{
			name:   "recognizes CRLF across chunks",
			chunks: [][]byte{[]byte("hello\r"), []byte("\n")},
			want:   [][]byte{[]byte("hello")},
		},
		{
			name:   "returns every complete line",
			chunks: [][]byte{[]byte("one\r\ntwo\r\n")},
			want:   [][]byte{[]byte("one"), []byte("two")},
		},
		{
			name:   "keeps an incomplete suffix",
			chunks: [][]byte{[]byte("one\r\ntw"), []byte("o\r\n")},
			want:   [][]byte{[]byte("one"), []byte("two")},
		},
		{
			name:   "returns an empty line",
			chunks: [][]byte{[]byte("\r\n")},
			want:   [][]byte{[]byte{}},
		},
	}

	for _, test := range tests {
		t.Run(test.name, func(t *testing.T) {
			var reader LineReader
			var got [][]byte
			for _, chunk := range test.chunks {
				got = append(got, reader.Feed(chunk)...)
			}

			if !equalLines(got, test.want) {
				t.Fatalf("unexpected lines: got %q, want %q", got, test.want)
			}
		})
	}
}

func TestLineReaderFeedCopiesEmittedLine(t *testing.T) {
	var reader LineReader
	line := reader.Feed([]byte("hello\r\n"))[0]
	reader.Feed([]byte("next\r\n"))

	if !bytes.Equal(line, []byte("hello")) {
		t.Fatalf("emitted line changed: got %q", line)
	}
}

func equalLines(left, right [][]byte) bool {
	if len(left) != len(right) {
		return false
	}
	for index := range left {
		if !bytes.Equal(left[index], right[index]) {
			return false
		}
	}
	return true
}
