package httprequest

import (
	"bytes"
	"path/filepath"
	"testing"

	"github.com/zrkluke/raw-http-server/go/internal/testfixture"
)

func TestHeaderSharedFixtures(t *testing.T) {
	root := filepath.Join("..", "..", "..", "testdata")
	for _, name := range []string{
		"valid-basic",
		"repeated-name",
		"case-insensitive",
		"optional-whitespace",
		"empty-section",
		"fragmented",
		"incomplete-value",
		"missing-colon",
		"leading-whitespace",
		"bare-lf",
		"bare-cr",
	} {
		t.Run(name, func(t *testing.T) {
			fixture, err := testfixture.Load(root, "headers", name)
			if err != nil {
				t.Fatal(err)
			}
			sizes, err := testfixture.LoadChunkSizes(
				root,
				"headers",
				name,
				len(fixture.Input),
			)
			if err != nil {
				t.Fatal(err)
			}

			parser := NewHeaderParser()
			offset := 0
			for _, size := range sizes {
				parser.Feed(fixture.Input[offset : offset+size])
				offset += size
			}

			if got := []byte(parser.String() + "\n"); !bytes.Equal(got, fixture.Expected) {
				t.Fatalf("fixture %s: got %q, want %q", name, got, fixture.Expected)
			}
		})
	}
}
