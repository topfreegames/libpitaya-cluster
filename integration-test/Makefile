# Makefile for Pitaya Integration Test (Conan2)

CMAKE = cmake
BUILD_DIR = build

.PHONY: all clean test-lame-duck test-zombie help setup-nats teardown-nats check-deps

all: $(BUILD_DIR)/simple_test

help:
	@echo "Available targets:"
	@echo "  all                - Build all integration tests (default)"
	@echo "  clean              - Clean build directory"
	@echo "  test-lame-duck     - Run the lame duck mode integration test"
	@echo "  test-zombie        - Run the zombie state reproduction test"
	@echo "  setup-nats         - Start NATS cluster via Docker Compose"
	@echo "  teardown-nats      - Stop NATS cluster"

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Install Conan dependencies and generate CMake files
$(BUILD_DIR)/conan_toolchain.cmake: conanfile.py | $(BUILD_DIR)
	conan install . --output-folder=$(BUILD_DIR) --build=missing -s build_type=Release

# Configure and build
$(BUILD_DIR)/simple_test: $(BUILD_DIR)/conan_toolchain.cmake
	cd $(BUILD_DIR) && $(CMAKE) .. -DCMAKE_TOOLCHAIN_FILE=$(CURDIR)/$(BUILD_DIR)/conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release
	cd $(BUILD_DIR) && $(CMAKE) --build . --config Release

clean:
	rm -rf $(BUILD_DIR)

setup-nats:
	docker-compose -f docker-compose-nats.yml up -d nats-1 nats-2 nats-3
	@echo "Waiting for NATS cluster to be ready..."
	@sleep 5

teardown-nats:
	docker-compose -f docker-compose-nats.yml down

test-lame-duck: $(BUILD_DIR)/simple_test setup-nats
	@echo "Starting lame duck mode integration test..."
	./scripts/test-lame-duck.sh

test-zombie: $(BUILD_DIR)/simple_test setup-nats
	@echo "Starting zombie state reproduction test..."
	chmod +x ./scripts/test-zombie-reproduction.sh
	./scripts/test-zombie-reproduction.sh

check-deps:
	@command -v docker >/dev/null 2>&1 || { echo "docker is required but not installed." >&2; exit 1; }
	@command -v docker-compose >/dev/null 2>&1 || { echo "docker-compose is required but not installed." >&2; exit 1; }
	@command -v cmake >/dev/null 2>&1 || { echo "cmake is required but not installed." >&2; exit 1; }
	@command -v conan >/dev/null 2>&1 || { echo "conan is required but not installed." >&2; exit 1; }
	@echo "All dependencies are installed."
