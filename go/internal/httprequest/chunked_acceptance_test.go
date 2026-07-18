package httprequest

import (
	"bytes"
	"os"
	"path/filepath"
	"strings"
	"testing"

	"github.com/zrkluke/raw-http-server/go/internal/testfixture"
)

func TestChunkedSharedFixtures(t *testing.T) {
	root := filepath.Join("..", "..", "..", "testdata")

	for _, name := range []string{"basic", "fragmented-trailers", "invalid-size", "missing-data-delimiter"} {
		t.Run(name, func(t *testing.T) {
			fixture, err := testfixture.Load(root, "chunked", name)
			if err != nil {
				t.Fatal(err)
			}
			chunks, err := testfixture.LoadChunkSizes(root, "chunked", name, len(fixture.Input))
			if err != nil {
				t.Fatal(err)
			}
			directory := filepath.Join(root, "chunked", name)
			expectedState, err := os.ReadFile(filepath.Join(directory, "state.txt"))
			if err != nil {
				t.Fatalf("read state: %v", err)
			}
			expectedTrailers, err := os.ReadFile(filepath.Join(directory, "trailers.txt"))
			if err != nil {
				t.Fatalf("read trailers: %v", err)
			}
			expectedRemaining, err := os.ReadFile(filepath.Join(directory, "remaining.bin"))
			if err != nil {
				t.Fatalf("read remaining bytes: %v", err)
			}

			parser := NewChunkedParser()
			offset := 0
			for _, size := range chunks {
				parser.Feed(fixture.Input[offset : offset+size])
				offset += size
			}

			if got, want := parser.State().String(), strings.TrimSpace(string(expectedState)); got != want {
				t.Fatalf("state = %q, want %q", got, want)
			}
			if !bytes.Equal(parser.Body(), fixture.Expected) {
				t.Fatalf("decoded body = %q, want %q", parser.Body(), fixture.Expected)
			}
			if got, want := serializeChunkedHeaders(parser.Trailers()), expectedTrailers; !bytes.Equal(got, want) {
				t.Fatalf("trailers = %q, want %q", got, want)
			}
			if got := parser.Remaining(); !bytes.Equal(got, expectedRemaining) {
				t.Fatalf("remaining bytes = %q, want %q", got, expectedRemaining)
			}
		})
	}
}

func serializeChunkedHeaders(headers []Header) []byte {
	var output strings.Builder
	for _, header := range headers {
		output.WriteString(header.Name)
		output.WriteByte('|')
		output.WriteString(header.Value)
		output.WriteByte('\n')
	}
	return []byte(output.String())
}
