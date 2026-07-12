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
