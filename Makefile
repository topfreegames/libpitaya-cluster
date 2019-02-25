MODULE_NAME = LibPitayaClusterCpp

nuget-clean:
	@rm -rf NugetOutput/Temp/bin/Release
	@rm -rf NugetOutput/obj
	@rm -rf NugetOutput/*.nupkg

build-cpp-unity:
	@cd cpp-lib && $(MAKE) build-linux-docker
	@cd cpp-lib && $(MAKE) build-mac-unity
	@cp cpp-lib/_builds/mac-unity/libpitaya_cluster.bundle NugetOutput/binaries/mac
	@cp cpp-lib/_builds/linux/libpitaya_cluster.so NugetOutput/binaries/linux

build-all: nuget-clean build-csharp-lib-release build-cpp-unity

pack-only:
	@cd NugetOutput && nuget pack *.nuspec -OutputDirectory .

pack: nuget-clean build-all
	@cp csharp-lib/cluster-lib/bin/Release/cluster-lib.dll NugetOutput/lib
	@cd NugetOutput && nuget pack *.nuspec -OutputDirectory .

default: build

ensure-out-dir:
	@mkdir -p out

build: ensure-out-dir
	@go build -o out/libpitaya_cluster.dylib -buildmode=c-shared .

build-csharp-example:
	@cd ./csharp-example && msbuild

build-csharp-lib:
	@cd ./csharp-lib && msbuild

build-csharp-lib-release:
	@cd ./csharp-lib && msbuild /p:Configuration=Release
	@cp ./csharp-lib/cluster-lib/bin/Release/cluster-lib.dll NugetOutput/lib

build-csharp-example-release:
	@cd ./csharp-example && msbuild /p:Configuration=Release

run-csharp-example:
	@cd ./csharp-example/csharp-example && mono ./bin/Debug/csharp-example.exe

run-csharp-example-release:
	@cd ./csharp-example/csharp-example && mono ./bin/Release/csharp-example.exe

build-go-server:
	@cd ./go-server && dep ensure

run-go-server:
	@go run ./go-server/main.go

submodules:
	@git submodule update --init --recursive --remote

protos-compile: submodules
	@protoc --csharp_out=./csharp-example/csharp-example/gen/ ./go-server/protos/*.proto
	@protoc --csharp_out=./unity-example/Assets/Gen/ ./go-server/protos/*.proto
	@protoc --proto_path=pitaya-protos --csharp_out=./csharp-lib/cluster-lib/gen ./pitaya-protos/*.proto
	@protoc --proto_path=pitaya-protos --python_out=./python-lib/gen ./pitaya-protos/*.proto
	@protoc --proto_path=./go-server/protos --python_out=./python-lib/gen/ ./go-server/protos/cluster.proto
	@protoc --gogofaster_out=. ./go-server/protos/*.proto

start-deps:
	@docker-compose up -d

stop-deps:
	@docker-compose down
