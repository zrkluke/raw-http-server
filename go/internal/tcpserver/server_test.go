package tcpserver

import (
	"testing"

	"github.com/zrkluke/raw-http-server/go/internal/httprequest"
)

func TestStartOnceRejectsInvalidAddress(t *testing.T) {
	if _, err := StartOnce("not-a-tcp-address"); err == nil {
		t.Fatal("StartOnce accepted an invalid address")
	}
}

func TestSelectRequestFraming(t *testing.T) {
	tests := []struct {
		name    string
		headers []httprequest.Header
		want    string
		length  string
		invalid bool
	}{
		{
			name:    "content length",
			headers: []httprequest.Header{{Name: "content-length", Value: "60"}},
			want:    "content-length",
			length:  "60",
		},
		{
			name:    "chunked is case insensitive",
			headers: []httprequest.Header{{Name: "transfer-encoding", Value: "Chunked"}},
			want:    "chunked",
		},
		{
			name: "both framings are rejected",
			headers: []httprequest.Header{
				{Name: "content-length", Value: "60"},
				{Name: "transfer-encoding", Value: "chunked"},
			},
			invalid: true,
		},
		{
			name:    "unsupported transfer coding is rejected",
			headers: []httprequest.Header{{Name: "transfer-encoding", Value: "gzip"}},
			invalid: true,
		},
	}

	for _, test := range tests {
		t.Run(test.name, func(t *testing.T) {
			got, length, err := selectRequestFraming(test.headers)
			if test.invalid {
				if err == nil {
					t.Fatal("selectRequestFraming succeeded")
				}
				return
			}
			if err != nil {
				t.Fatal(err)
			}
			if got != test.want {
				t.Fatalf("framing = %q, want %q", got, test.want)
			}
			if test.length == "" {
				if length != nil {
					t.Fatalf("content length = %q, want nil", *length)
				}
			} else if length == nil || *length != test.length {
				t.Fatalf("content length = %v, want %q", length, test.length)
			}
		})
	}
}
