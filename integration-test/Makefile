# Makefile for Pitaya Integration Test

CXX = clang++
CMAKE = cmake
BUILD_DIR = build
INSTALL_DIR = $(BUILD_DIR)/install

.PHONY: all clean build build-all client server test-lame-duck help setup-nats teardown-nats

all: build-all

help:
	@echo "Available targets:"
	@echo "  build        - Build the simple integration test"
	@echo "  clean        - Clean build directory"
	@echo "  test-lame-duck - Run the lame duck mode integration test"
	@echo "  setup-nats   - Start NATS cluster via Docker Compose"
	@echo "  teardown-nats - Stop NATS cluster"

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

build-all: $(BUILD_DIR)
	cd $(BUILD_DIR) && $(CMAKE) .. -DCMAKE_BUILD_TYPE=Release
	cd $(BUILD_DIR) && $(CMAKE) --build .

# Convenience alias for build-all
build: build-all

clean:
	rm -rf $(BUILD_DIR)

setup-nats:
	docker-compose -f docker-compose-nats.yml up -d
	@echo "Waiting for NATS cluster to be ready..."
	sleep 5

teardown-nats:
	docker-compose -f docker-compose-nats.yml down

test-lame-duck: build-all setup-nats
	@echo "Starting lame duck mode integration test..."
	./scripts/test-lame-duck.sh

.PHONY: check-deps
check-deps:
	@command -v docker >/dev/null 2>&1 || { echo "docker is required but not installed." >&2; exit 1; }
	@command -v docker-compose >/dev/null 2>&1 || { echo "docker-compose is required but not installed." >&2; exit 1; }
	@command -v cmake >/dev/null 2>&1 || { echo "cmake is required but not installed." >&2; exit 1; }