package tcpserver

import "testing"

func TestStartOnceRejectsInvalidAddress(t *testing.T) {
	if _, err := StartOnce("not-a-tcp-address"); err == nil {
		t.Fatal("StartOnce accepted an invalid address")
	}
}
