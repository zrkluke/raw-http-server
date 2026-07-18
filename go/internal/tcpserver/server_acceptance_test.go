package tcpserver

import (
	"bytes"
	"net"
	"path/filepath"
	"testing"
	"time"

	"github.com/zrkluke/raw-http-server/go/internal/testfixture"
)

func TestStartOnceTransfersBytesAndCleansUp(t *testing.T) {
	root := filepath.Join("..", "..", "..", "testdata")
	fixture, err := testfixture.Load(root, "tcp", "listen-connect")
	if err != nil {
		t.Fatal(err)
	}

	server, err := StartOnce("127.0.0.1:0")
	if err != nil {
		t.Fatal(err)
	}

	connection, err := net.DialTimeout("tcp", server.Addr, time.Second)
	if err != nil {
		t.Fatal(err)
	}
	if _, err := connection.Write(fixture.Input); err != nil {
		t.Fatal(err)
	}
	if err := connection.Close(); err != nil {
		t.Fatal(err)
	}

	select {
	case received := <-server.Received:
		if !bytes.Equal(received, fixture.Expected) {
			t.Fatalf("unexpected bytes: got %q, want %q", received, fixture.Expected)
		}
	case <-time.After(time.Second):
		t.Fatal("server did not receive client bytes")
	}

	select {
	case err := <-server.Done:
		if err != nil {
			t.Fatal(err)
		}
	case <-time.After(time.Second):
		t.Fatal("server did not close after one connection")
	}

	if connection, err := net.DialTimeout("tcp", server.Addr, time.Second); err == nil {
		connection.Close()
		t.Fatal("listener still accepted a connection")
	}
}
