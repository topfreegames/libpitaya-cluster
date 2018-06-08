default: build

ensure-out-dir:
	@mkdir -p out

build: ensure-out-dir
	@go build -o out/libpitaya_cluster.dylib -buildmode=c-shared .

run-csharp-example:
	@cd ./csharp && GODEBUG=cgocheck=0 dotnet run
