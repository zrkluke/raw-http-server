.PHONY: verify verify-c verify-go verify-rust

verify: verify-c verify-go verify-rust

verify-c:
	$(MAKE) -C c test sanitize

verify-go:
	@files="$$(gofmt -l $$(find go -type f -name '*.go'))"; \
	if [ -n "$$files" ]; then \
		echo "Go files need formatting:"; \
		echo "$$files"; \
		exit 1; \
	fi
	go -C go vet ./...
	go -C go test ./...

verify-rust:
	cargo fmt --manifest-path rust/Cargo.toml --all -- --check
	cargo clippy --manifest-path rust/Cargo.toml --all-targets -- -D warnings
	cargo test --manifest-path rust/Cargo.toml
