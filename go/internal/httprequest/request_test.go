package httprequest

import "testing"

func TestParserWaitsForAnIncompleteHead(t *testing.T) {
	parser := NewParser()

	if got := parser.Feed([]byte("GET / HTTP/1.1\r\nHost: example\r")); got != Incomplete {
		t.Fatalf("state = %s, want incomplete", got)
	}
}

func TestParserCompletesAcrossCRLFBoundaries(t *testing.T) {
	parser := NewParser()

	if got := parser.Feed([]byte("GET / HTTP/1.1\r")); got != Incomplete {
		t.Fatalf("state = %s, want incomplete", got)
	}
	if got := parser.Feed([]byte("\nHost: example\r\n\r")); got != Incomplete {
		t.Fatalf("state = %s, want incomplete", got)
	}
	if got := parser.Feed([]byte("\n")); got != Complete {
		t.Fatalf("state = %s, want complete", got)
	}
}

func TestParserRejectsBareLineEndings(t *testing.T) {
	tests := []struct {
		name   string
		chunks [][]byte
	}{
		{
			name:   "bare LF",
			chunks: [][]byte{[]byte("GET / HTTP/1.1\n")},
		},
		{
			name:   "bare CR",
			chunks: [][]byte{[]byte("GET / HTTP/1.1\r"), []byte("x")},
		},
	}

	for _, test := range tests {
		t.Run(test.name, func(t *testing.T) {
			parser := NewParser()
			var state State
			for _, chunk := range test.chunks {
				state = parser.Feed(chunk)
			}

			if state != Invalid {
				t.Fatalf("state = %s, want invalid", state)
			}
		})
	}
}

func TestParserKeepsItsTerminalState(t *testing.T) {
	parser := NewParser()

	if got := parser.Feed([]byte("GET / HTTP/1.1\r\n\r\n")); got != Complete {
		t.Fatalf("state = %s, want complete", got)
	}
	if got := parser.Feed([]byte("unexpected")); got != Complete {
		t.Fatalf("state = %s, want complete", got)
	}
}
