package httpstream

import (
	"bytes"
	"path/filepath"
	"testing"

	"github.com/zrkluke/raw-http-server/go/internal/testfixture"
)

func TestLineReaderSharedFixtures(t *testing.T) {
	root := filepath.Join("..", "..", "..", "testdata")
	for _, name := range []string{"complete-line", "crlf-split", "one-byte"} {
		t.Run(name, func(t *testing.T) {
			fixture, err := testfixture.Load(root, "http-streams", name)
			if err != nil {
				t.Fatal(err)
			}
			sizes, err := testfixture.LoadChunkSizes(
				root,
				"http-streams",
				name,
				len(fixture.Input),
			)
			if err != nil {
				t.Fatal(err)
			}

			var reader LineReader
			var actual bytes.Buffer
			offset := 0
			for _, size := range sizes {
				for _, line := range reader.Feed(fixture.Input[offset : offset+size]) {
					actual.Write(line)
					actual.WriteByte('\n')
				}
				offset += size
			}

			if !bytes.Equal(actual.Bytes(), fixture.Expected) {
				t.Fatalf("unexpected lines: got %q, want %q", actual.Bytes(), fixture.Expected)
			}
		})
	}
}
