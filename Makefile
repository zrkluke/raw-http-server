.PHONY: verify verify-c verify-go verify-rust

verify: verify-c verify-go verify-rust

verify-c:
	$(MAKE) -C c test

verify-go:
	go -C go test ./...

verify-rust:
	cargo test --manifest-path rust/Cargo.toml
