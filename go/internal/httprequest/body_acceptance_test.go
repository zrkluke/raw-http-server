package httprequest

import (
	"bytes"
	"os"
	"path/filepath"
	"strings"
	"testing"

	"github.com/zrkluke/raw-http-server/go/internal/testfixture"
)

func TestBodySharedFixtures(t *testing.T) {
	root := filepath.Join("..", "..", "..", "testdata")
	for _, name := range []string{
		"absent",
		"complete",
		"partial",
		"invalid-length",
		"over-delivered",
	} {
		t.Run(name, func(t *testing.T) {
			fixture, err := testfixture.Load(root, "bodies", name)
			if err != nil {
				t.Fatal(err)
			}
			sizes, err := testfixture.LoadChunkSizes(root, "bodies", name, len(fixture.Input))
			if err != nil {
				t.Fatal(err)
			}

			directory := filepath.Join(root, "bodies", name)
			contentLength, err := os.ReadFile(filepath.Join(directory, "content-length.txt"))
			if err != nil {
				t.Fatal(err)
			}
			expectedState, err := os.ReadFile(filepath.Join(directory, "state.txt"))
			if err != nil {
				t.Fatal(err)
			}
			expectedRemaining, err := os.ReadFile(filepath.Join(directory, "remaining.bin"))
			if err != nil {
				t.Fatal(err)
			}

			value := strings.TrimSpace(string(contentLength))
			var length *string
			if value != "absent" {
				length = &value
			}

			parser := NewBodyParser(length)
			offset := 0
			for _, size := range sizes {
				parser.Feed(fixture.Input[offset : offset+size])
				offset += size
			}

			if got := parser.State().String(); got != strings.TrimSpace(string(expectedState)) {
				t.Fatalf("state: got %q, want %q", got, expectedState)
			}
			if got := parser.Body(); !bytes.Equal(got, fixture.Expected) {
				t.Fatalf("body: got %q, want %q", got, fixture.Expected)
			}
			if got := parser.Remaining(); !bytes.Equal(got, expectedRemaining) {
				t.Fatalf("remaining: got %q, want %q", got, expectedRemaining)
			}
		})
	}
}
