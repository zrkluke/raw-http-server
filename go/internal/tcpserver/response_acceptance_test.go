package tcpserver

import (
	"bytes"
	"io"
	"net"
	"path/filepath"
	"testing"
	"time"

	"github.com/zrkluke/raw-http-server/go/internal/testfixture"
)

func TestStartResponseOnceWritesSharedResponse(t *testing.T) {
	root := filepath.Join("..", "..", "..", "testdata")
	fixture, err := testfixture.Load(root, "responses", "basic")
	if err != nil {
		t.Fatal(err)
	}

	server, err := StartResponseOnce("127.0.0.1:0")
	if err != nil {
		t.Fatal(err)
	}

	connection, err := net.DialTimeout("tcp", server.Addr, time.Second)
	if err != nil {
		t.Fatal(err)
	}
	defer connection.Close()

	if _, err := connection.Write(fixture.Input); err != nil {
		t.Fatal(err)
	}

	response, err := io.ReadAll(connection)
	if err != nil {
		t.Fatal(err)
	}
	if !bytes.Equal(response, fixture.Expected) {
		t.Fatalf("unexpected response: got %q, want %q", response, fixture.Expected)
	}

	select {
	case err := <-server.Done:
		if err != nil {
			t.Fatal(err)
		}
	case <-time.After(time.Second):
		t.Fatal("server did not finish after writing its response")
	}
}
