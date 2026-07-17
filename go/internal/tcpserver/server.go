package tcpserver

import (
	"io"
	"net"

	"github.com/zrkluke/raw-http-server/go/internal/httpresponse"
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

// StartResponseOnce accepts one request, writes the course's basic HTTP/1.1
// response, then releases the connection and listener.
func StartResponseOnce(address string) (Server, error) {
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

	go serveResponseOnce(listener, received, done)
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

func serveResponseOnce(listener net.Listener, received chan<- []byte, done chan<- error) {
	defer listener.Close()

	connection, err := listener.Accept()
	if err != nil {
		done <- err
		return
	}
	defer connection.Close()

	buffer := make([]byte, 4096)
	count, readErr := connection.Read(buffer)
	if count > 0 {
		received <- append([]byte(nil), buffer[:count]...)
	}
	if readErr != nil && readErr != io.EOF {
		done <- readErr
		return
	}
	if count == 0 {
		done <- io.ErrUnexpectedEOF
		return
	}

	done <- httpresponse.Write(connection, httpresponse.BasicOK())
}
