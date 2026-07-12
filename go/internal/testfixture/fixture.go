package testfixture

import (
	"fmt"
	"os"
	"path/filepath"
)

type Fixture struct {
	Input    []byte
	Expected []byte
}

func Load(root, group, name string) (Fixture, error) {
	dir := filepath.Join(root, group, name)
	input, err := os.ReadFile(filepath.Join(dir, "input.bin"))
	if err != nil {
		return Fixture{}, fmt.Errorf("read fixture input: %w", err)
	}
	expected, err := os.ReadFile(filepath.Join(dir, "expected.bin"))
	if err != nil {
		return Fixture{}, fmt.Errorf("read fixture expected output: %w", err)
	}
	return Fixture{Input: input, Expected: expected}, nil
}
