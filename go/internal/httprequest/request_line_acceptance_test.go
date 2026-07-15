package httprequest

import (
	"bytes"
	"path/filepath"
	"testing"

	"github.com/zrkluke/raw-http-server/go/internal/httpstream"
	"github.com/zrkluke/raw-http-server/go/internal/testfixture"
)

func TestRequestLineSharedFixtures(t *testing.T) {
	root := filepath.Join("..", "..", "..", "testdata")
	for _, name := range []string{
		"valid-basic",
		"valid-query",
		"valid-crlf-boundary",
		"valid-field-split",
		"valid-fixed-four",
		"valid-one-byte",
		"missing-target",
		"leading-space",
		"trailing-space",
		"double-space",
		"tab-separator",
		"unsupported-version",
	} {
		t.Run(name, func(t *testing.T) {
			fixture, err := testfixture.Load(root, "request-lines", name)
			if err != nil {
				t.Fatal(err)
			}
			sizes, err := testfixture.LoadChunkSizes(
				root,
				"request-lines",
				name,
				len(fixture.Input),
			)
			if err != nil {
				t.Fatal(err)
			}

			var reader httpstream.LineReader
			offset := 0
			var lines [][]byte
			for _, size := range sizes {
				lines = append(lines, reader.Feed(fixture.Input[offset:offset+size])...)
				offset += size
			}
			if len(lines) != 1 {
				t.Fatalf("got %d complete lines, want 1", len(lines))
			}

			actual := ParseRequestLine(lines[0]).String()
			if !bytes.Equal([]byte(actual+"\n"), fixture.Expected) {
				t.Fatalf("got %q, want %q", actual+"\n", fixture.Expected)
			}
		})
	}
}
