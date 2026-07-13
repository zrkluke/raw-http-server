package httprequest

import (
	"path/filepath"
	"testing"

	"github.com/zrkluke/raw-http-server/go/internal/testfixture"
)

func TestRequestSharedFixtures(t *testing.T) {
	root := filepath.Join("..", "..", "..", "testdata")
	for _, name := range []string{
		"incomplete-head",
		"complete-head",
		"complete-head-whole",
		"complete-head-one-byte",
		"complete-head-fixed",
		"bare-lf",
	} {
		t.Run(name, func(t *testing.T) {
			fixture, err := testfixture.Load(root, "requests", name)
			if err != nil {
				t.Fatal(err)
			}
			sizes, err := testfixture.LoadChunkSizes(root, "requests", name, len(fixture.Input))
			if err != nil {
				t.Fatal(err)
			}

			parser := NewParser()
			offset := 0
			var state State
			for _, size := range sizes {
				state = parser.Feed(fixture.Input[offset : offset+size])
				offset += size
			}

			if got := []byte(state.String() + "\n"); string(got) != string(fixture.Expected) {
				t.Fatalf("unexpected state: got %q, want %q", got, fixture.Expected)
			}
		})
	}
}
