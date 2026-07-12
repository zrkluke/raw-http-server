package tcpserver

import (
	"io"
	"net"
)

// Server exposes the observable result of one accepted TCP connection.
type Server struct {
	Addr     string
	Received <-chan []byte
	Done     <-chan error
}

// StartOnce listens on address, accepts one client, reads until it closes, and
// then releases both the client connection and listener.
func StartOnce(address string) (Server, error) {
	listener, err := net.Listen("tcp", address)
	if err != nil {
		return Server{}, err
	}

	received := make(chan []byte, 1)
	done := make(chan error, 1)
	server := Server{
		Addr:     listener.Addr().String(),
		Received: received,
		Done:     done,
	}

	go serveOnce(listener, received, done)
	return server, nil
}

func serveOnce(listener net.Listener, received chan<- []byte, done chan<- error) {
	defer listener.Close()

	connection, err := listener.Accept()
	if err != nil {
		done <- err
		return
	}
	defer connection.Close()

	bytes, err := io.ReadAll(connection)
	if err == nil {
		received <- bytes
	}
	done <- err
}
