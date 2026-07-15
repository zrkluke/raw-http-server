package httprequest

import "testing"

func TestHeaderParser(t *testing.T) {
	tests := []struct {
		name   string
		chunks [][]byte
		want   string
	}{
		{
			name:   "preserves repeated field order",
			chunks: [][]byte{[]byte("Set-Cookie: one\r\nSet-Cookie: two\r\n\r\n")},
			want:   "complete\nset-cookie|one\nset-cookie|two",
		},
		{
			name:   "normalizes names and trims optional whitespace",
			chunks: [][]byte{[]byte("HOST: example.com\r\nX-Note: \t hello \t\r\n\r\n")},
			want:   "complete\nhost|example.com\nx-note|hello",
		},
		{
			name:   "waits for a CRLF split across chunks",
			chunks: [][]byte{[]byte("Host: example.com\r"), []byte("\n\r"), []byte("\n")},
			want:   "complete\nhost|example.com",
		},
		{
			name:   "keeps an incomplete value incomplete",
			chunks: [][]byte{[]byte("Host: ex")},
			want:   "incomplete",
		},
		{
			name:   "rejects malformed header lines",
			chunks: [][]byte{[]byte(" Host: example.com\r\n\r\n")},
			want:   "invalid",
		},
		{
			name:   "rejects a confirmed bare carriage return",
			chunks: [][]byte{[]byte("Host: example.com\r"), []byte("X")},
			want:   "invalid",
		},
		{
			name:   "rejects bare line feeds",
			chunks: [][]byte{[]byte("Host: example.com\n\n")},
			want:   "invalid",
		},
	}

	for _, test := range tests {
		t.Run(test.name, func(t *testing.T) {
			parser := NewHeaderParser()
			for _, chunk := range test.chunks {
				parser.Feed(chunk)
			}
			if got := parser.String(); got != test.want {
				t.Fatalf("String() = %q, want %q", got, test.want)
			}
		})
	}
}
