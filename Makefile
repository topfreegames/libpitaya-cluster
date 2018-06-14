default: build

ensure-out-dir:
	@mkdir -p out

build: ensure-out-dir
	@go build -o out/libpitaya_cluster.dylib -buildmode=c-shared .

run-csharp-example:
	@cd ./csharp-example && GODEBUG=cgocheck=0 dotnet run

protos-compile:
	@protoc --csharp_out=./csharp-example/gen/ ./go-server/protos/*.proto
	@protoc --gogofaster_out=. ./go-server/protos/*.proto
