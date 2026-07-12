package testfixture

import (
	"bytes"
	"path/filepath"
	"testing"
)

func TestLoad(t *testing.T) {
	root := filepath.Join("..", "..", "..", "testdata")
	fixture, err := Load(root, "foundation", "loader-smoke")
	if err != nil {
		t.Fatal(err)
	}
	if !bytes.Equal(fixture.Input, []byte("shared fixture input\n")) {
		t.Fatalf("unexpected input: %q", fixture.Input)
	}
	if !bytes.Equal(fixture.Expected, []byte("shared fixture expected\n")) {
		t.Fatalf("unexpected expected output: %q", fixture.Expected)
	}
}

func TestLoadChunkSizes(t *testing.T) {
	root := filepath.Join("..", "..", "..", "testdata")
	tests := []struct {
		name     string
		expected []int
	}{
		{name: "complete-line", expected: []int{7}},
		{name: "crlf-split", expected: []int{6, 1}},
		{name: "one-byte", expected: []int{1, 1, 1, 1, 1, 1, 1}},
	}

	for _, test := range tests {
		t.Run(test.name, func(t *testing.T) {
			fixture, err := Load(root, "http-streams", test.name)
			if err != nil {
				t.Fatal(err)
			}

			sizes, err := LoadChunkSizes(
				root,
				"http-streams",
				test.name,
				len(fixture.Input),
			)
			if err != nil {
				t.Fatal(err)
			}
			if !equalInts(sizes, test.expected) {
				t.Fatalf("unexpected chunk sizes: %v", sizes)
			}
		})
	}
}

func equalInts(left, right []int) bool {
	if len(left) != len(right) {
		return false
	}
	for i := range left {
		if left[i] != right[i] {
			return false
		}
	}
	return true
}
