MODULE_NAME = LibPitayaClusterCpp

nuget-clean:
	@rm -rf NugetOutput/Temp/bin/Release
	@rm -rf NugetOutput/obj
	@rm -rf NugetOutput/*.nupkg

build-cpp-unity:
	@cd cpp-lib && $(MAKE) build-linux-docker
	@cd cpp-lib && $(MAKE) build-mac-unity

build-all: nuget-clean build-csharp-lib-release build-cpp-unity

pack-only:nuget-clean
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
	@git submodule update --init --recursive --remote pitaya-protos

protos-compile-cpp: submodules
	@mkdir -p ./cpp-lib/include/pitaya/protos
	@protoc -I pitaya-protos --grpc_out=generate_mock_code=true:./cpp-lib/include/pitaya/protos \
		--plugin=protoc-gen-grpc=$(shell which grpc_cpp_plugin) ./pitaya-protos/*.proto \
		--cpp_out=./cpp-lib/include/pitaya/protos

protos-compile: protos-compile-cpp
	@protoc --csharp_out=./csharp-example/csharp-example/gen/ ./go-server/protos/*.proto
	@protoc --csharp_out=./unity-example/Assets/Gen/ ./go-server/protos/*.proto
	@protoc --proto_path=pitaya-protos --csharp_out=./csharp-lib/cluster-lib/gen ./pitaya-protos/*.proto
	@protoc --proto_path=pitaya-protos --python_out=./python-lib/pitayaserver/gen ./pitaya-protos/*.proto
	@protoc --proto_path=./go-server/protos --python_out=./python-lib/pitayaserver/gen ./go-server/protos/cluster.proto
	@sed -i '.old' 's/^\(import.*_pb2\)/from . \1/' ./python-lib/pitayaserver/gen/*.py && rm ./python-lib/pitayaserver/gen/*.old ## dirty python hack, see https://github.com/protocolbuffers/protobuf/issues/1491
	@protoc --go_out=. ./go-server/protos/*.proto

start-deps:
	@docker-compose up -d

stop-deps:
	@docker-compose down
