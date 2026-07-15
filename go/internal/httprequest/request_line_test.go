package httprequest

import "testing"

func TestParseRequestLine(t *testing.T) {
	tests := []struct {
		name string
		line []byte
		want RequestLine
	}{
		{
			name: "valid request line",
			line: []byte("GET /search?q=rust HTTP/1.1"),
			want: RequestLine{
				Method:  "GET",
				Target:  "/search?q=rust",
				Version: "HTTP/1.1",
				Valid:   true,
			},
		},
		{
			name: "missing target",
			line: []byte("GET HTTP/1.1"),
		},
		{
			name: "repeated space",
			line: []byte("GET  / HTTP/1.1"),
		},
		{
			name: "tab separator",
			line: []byte("GET\t/ HTTP/1.1"),
		},
		{
			name: "unsupported version",
			line: []byte("GET / HTTP/1.0"),
		},
		{
			name: "line ending is not part of a request line",
			line: []byte("GET / HTTP/1.1\r\n"),
		},
	}

	for _, test := range tests {
		t.Run(test.name, func(t *testing.T) {
			if got := ParseRequestLine(test.line); got != test.want {
				t.Fatalf("ParseRequestLine(%q) = %#v, want %#v", test.line, got, test.want)
			}
		})
	}
}
