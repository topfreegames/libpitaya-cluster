MODULE_NAME = LibPitayaClusterCpp

clean-cpp-mac:
	@cd cpp-lib && $(MAKE) clean-all

# Builds the native libraries and copies them to the
# precompiled folder.
build-cpp-on-mac:
	@cd cpp-lib && $(MAKE) build-all-mac
	@lipo -create cpp-lib/_builds//macos-x86_64-macosx_bundle_false-release/libpitaya_cpp.dylib cpp-lib/_builds//macos-armv8-macosx_bundle_false-release/libpitaya_cpp.dylib -output ./pitaya-sharp/NPitaya/Runtime/Plugins/libpitaya_cpp.dylib
	@lipo -create cpp-lib/_builds//macos-x86_64-macosx_bundle_true-release/libpitaya_cpp.bundle cpp-lib/_builds//macos-armv8-macosx_bundle_true-release/libpitaya_cpp.bundle -output ./pitaya-sharp/NPitaya/Runtime/Plugins/libpitaya_cpp.bundle
	@cp cpp-lib/_builds/linux-x86_64-release/libpitaya_cpp.so pitaya-sharp/NPitaya/Runtime/Plugins/

build-all: build-csharp-lib-release build-cpp-unity

submodules:
	@git submodule update --init --recursive --remote pitaya-protos

protos-compile-cpp: submodules
	@mkdir -p ./cpp-lib/include/pitaya/protos
	@protoc -I pitaya-protos --grpc_out=generate_mock_code=true:./cpp-lib/include/pitaya/protos \
		--plugin=protoc-gen-grpc=$(shell which grpc_cpp_plugin) ./pitaya-protos/*.proto \
		--cpp_out=./cpp-lib/include/pitaya/protos

protos-compile: protos-compile-cpp
	@protoc --csharp_out=./pitaya-sharp/exampleapp/gen/ ./go-server/protos/*.proto
	@protoc --csharp_out=./unity/Assets/Gen/ ./go-server/protos/*.proto
	@protoc --proto_path=pitaya-protos --csharp_out=./pitaya-sharp/NPitaya/src/gen ./pitaya-protos/*.proto
	@protoc --proto_path=pitaya-protos --python_out=./python-lib/pitayaserver/gen ./pitaya-protos/*.proto
	@protoc --proto_path=./go-server/protos --python_out=./python-lib/pitayaserver/gen ./go-server/protos/cluster.proto
	@sed -i '.old' 's/^\(import.*_pb2\)/from . \1/' ./python-lib/pitayaserver/gen/*.py && rm ./python-lib/pitayaserver/gen/*.old ## dirty python hack, see https://github.com/protocolbuffers/protobuf/issues/1491
	@protoc --go_out=. ./go-server/protos/*.proto

start-deps:
	@docker-compose up -d

stop-deps:
	@docker-compose down

build-go-server:
	@cd ./go-server && go mod tidy

run-go-server:
	@go run ./go-server/main.go

nuget-pack:
	@rm -rf NugetOutput
	@nuget pack unity/NPitaya.nuspec -OutputDirectory NugetOutput

nuget-push:
	@nuget push NugetOutput/*.nupkg $(NUGET_API_KEY) -Source nuget.org

# Release commands
.PHONY: check-gh install-gh signin-gh release

check-gh:
	@echo "🔍 Checking GitHub CLI installation..."
	@if ! command -v gh >/dev/null 2>&1; then \
		echo "❌ GitHub CLI (gh) is not installed."; \
		echo "📦 Please install it first:"; \
		echo "   macOS: brew install gh"; \
		echo "   Ubuntu/Debian: sudo apt install gh"; \
		echo "   Windows: winget install GitHub.cli"; \
		echo "   Or visit: https://cli.github.com/"; \
		exit 1; \
	fi
	@echo "✅ GitHub CLI is installed"

signin-gh: check-gh
	@echo "🔐 Checking GitHub CLI authentication..."
	@if ! gh auth status >/dev/null 2>&1; then \
		echo "❌ Not signed in to GitHub CLI."; \
		echo "🔑 Please sign in:"; \
		echo "   gh auth login"; \
		exit 1; \
	fi
	@echo "✅ GitHub CLI is authenticated"

release: signin-gh
	@if [ -z "$(VERSION)" ]; then \
		echo "❌ VERSION is required. Usage: make release VERSION=v1.0.7"; \
		exit 1; \
	fi
	@echo "🚀 Starting release process for version $(VERSION)..."
	@echo "📝 Updating version references..."
	@VERSION=$(VERSION) ./update-version.sh
	@echo "📋 Creating GitHub release..."
	@if [ "$(PRERELEASE)" = "true" ]; then \
		gh release create $(VERSION) --title "Release $(VERSION)" --generate-notes --prerelease --target $(shell git branch --show-current); \
	else \
		gh release create $(VERSION) --title "Release $(VERSION)" --generate-notes --target $(shell git branch --show-current); \
	fi
	@echo "✅ Release $(VERSION) created successfully!"
	@echo "🎉 The GitHub Actions workflow will now build and publish the package automatically."
