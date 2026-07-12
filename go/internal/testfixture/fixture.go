package testfixture

import (
	"fmt"
	"os"
	"path/filepath"
	"strconv"
	"strings"
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

func LoadChunkSizes(root, group, name string, inputLength int) ([]int, error) {
	path := filepath.Join(root, group, name, "chunks.txt")
	data, err := os.ReadFile(path)
	if err != nil {
		return nil, fmt.Errorf("read fixture chunks: %w", err)
	}

	fields := strings.Fields(string(data))
	if len(fields) == 0 {
		return nil, fmt.Errorf("read fixture chunks: no chunk sizes")
	}

	sizes := make([]int, 0, len(fields))
	total := 0
	for _, field := range fields {
		size, err := strconv.Atoi(field)
		if err != nil || size <= 0 {
			return nil, fmt.Errorf("read fixture chunks: invalid size %q", field)
		}
		sizes = append(sizes, size)
		total += size
	}

	if total != inputLength {
		return nil, fmt.Errorf(
			"read fixture chunks: total %d does not match input length %d",
			total,
			inputLength,
		)
	}

	return sizes, nil
}
