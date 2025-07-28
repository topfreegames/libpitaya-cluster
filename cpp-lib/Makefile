SRC_FILES = $(shell find src test -name '*.cpp')
HDR_FILES = $(shell find include test -not -path 'include/pitaya/protos/*' -not -path 'test/doctest.h' -name '*.h')
ARCHES = x86_64 armv8
LOCAL_ARCH = $(shell uname -m)
ifeq ($(LOCAL_ARCH),arm64)
LOCAL_ARCH=armv8
endif

format:
	@for f in ${SRC_FILES} ; do \
		clang-format -i $$f ; \
	done
	@for f in ${HDR_FILES} ; do \
		clang-format -i $$f ; \
	done

clean-docker-container:
	@if [[ `docker ps -aqf "name=pitaya-cluster"` != "" ]]; then \
		docker rm `docker ps -aqf "name=pitaya-cluster"` ; \
	fi

clean-linux-release:
	@rm -rf _builds/linux

clean-xcode:
	@rm -rf _builds/xcode

clean-mac-release:
	@rm -rf _builds/mac

clean-mac-unity:
	@rm -rf _builds/mac-unity

clean-mac-debug:
	@rm -rf _builds/mac-debug
	@rm -rf build

clean-all: clean-mac-debug clean-mac-release clean-mac-unity clean-xcode clean-linux-release clean-build

clean-build:
	@rm -rf build build-x86_64 build-armv8

generate-xcode:
	@conan install -of _builds/xcode . --build=missing -pr ./conanprofile -c tools.cmake.cmaketoolchain:generator=Xcode
	@cmake -H. -B_builds/xcode -GXcode --preset npitaya-release -DBUILD_TESTS=ON -DBUILD_SHARED_LIBS=OFF

build-linux-release:
	@conan install . -of build -s build_type=Release -s arch=x86_64 -s:b compiler.cppstd=gnu17 -s compiler.cppstd=gnu17 -o "*:shared=False" -o "protobuf/*:debug_suffix=False" -o "openssl/*:shared=False" -o "cpprestsdk/*:with_websockets=False" -o "cnats/*:shared=False" -o "cnats/*:with_streaming=False" -o "cnats/*:with_tls=True" -o "cnats/*:tls_use_openssl_1_1_api=True" -o "etcd-cpp-apiv3/*:with_nats=True" -o "etcd-cpp-apiv3/*:nats_package_name=cnats" --build=missing --build=outdated
	@cmake --preset conan-release
	@cmake --build --preset conan-release

build-mac-release:
	for arch in $(ARCHES); do \
		rm -f CMakeUserPresets.json; \
		conan install . -of build-$${arch} -s build_type=Release -s arch=$${arch} -s:b compiler.cppstd=gnu17 -s compiler.cppstd=gnu17 -s:b compiler.libcxx=libc++ -s compiler.libcxx=libc++ -s:b compiler.version=15 -s compiler.version=15 -o "*:shared=False" -o "protobuf/*:debug_suffix=False" -o "openssl/*:shared=False" -o "cpprestsdk/*:with_websockets=False" -o "cnats/*:shared=False" -o "cnats/*:with_streaming=False" -o "cnats/*:with_tls=True" -o "cnats/*:tls_use_openssl_1_1_api=True" -o "etcd-cpp-apiv3/*:with_nats=True" -o "etcd-cpp-apiv3/*:nats_package_name=cnats" --build=missing --build=outdated; \
		cmake --preset conan-release -B build-$${arch}/_builds/Release; \
		cmake --build build-$${arch}/_builds/Release --preset conan-release; \
	done

build-mac-debug:
	@conan install . -of build -s build_type=Debug -s arch=$(LOCAL_ARCH) -s:b compiler.cppstd=gnu17 -s compiler.cppstd=gnu17 -s:b compiler.libcxx=libc++ -s compiler.libcxx=libc++ -s:b compiler.version=15 -s compiler.version=15 -o "*:shared=False" -o "protobuf/*:debug_suffix=False" -o "openssl/*:shared=False" -o "cpprestsdk/*:with_websockets=False" -o "cnats/*:shared=False" -o "cnats/*:with_streaming=False" -o "cnats/*:with_tls=True" -o "cnats/*:tls_use_openssl_1_1_api=True" -o "etcd-cpp-apiv3/*:with_nats=True" -o "etcd-cpp-apiv3/*:nats_package_name=cnats" --build=missing --build=outdated
	@cmake --preset conan-debug -DBUILD_GMOCK=ON
	@cmake --build --preset conan-debug

build-mac-unity:
	for arch in $(ARCHES); do \
		rm -f CMakeUserPresets.json; \
		conan install . -of build-$${arch} -s build_type=Release -s arch=$${arch} -s:b compiler.cppstd=gnu17 -s compiler.cppstd=gnu17 -s:b compiler.libcxx=libc++ -s compiler.libcxx=libc++ -s:b compiler.version=15 -s compiler.version=15 -o "*:shared=False" -o "protobuf/*:debug_suffix=False" -o "openssl/*:shared=False" -o "cpprestsdk/*:with_websockets=False" -o "cnats/*:shared=False" -o "cnats/*:with_streaming=False" -o "cnats/*:with_tls=True" -o "cnats/*:tls_use_openssl_1_1_api=True" -o "etcd-cpp-apiv3/*:with_nats=True" -o "etcd-cpp-apiv3/*:nats_package_name=cnats" -o "macosx_bundle=True" --build=missing --build=outdated; \
		cmake --preset conan-release -B build-$${arch}/_builds/Release; \
		cmake --build build-$${arch}/_builds/Release --preset conan-release; \
	done

build-mac-unity-debug:
	@conan install . -of build -s build_type=Debug -s arch=$(LOCAL_ARCH) -s:b compiler.cppstd=gnu17 -s compiler.cppstd=gnu17 -s:b compiler.libcxx=libc++ -s compiler.libcxx=libc++ -s:b compiler.version=15 -s compiler.version=15 -o "*:shared=False" -o "protobuf/*:debug_suffix=False" -o "openssl/*:shared=False" -o "cpprestsdk/*:with_websockets=False" -o "cnats/*:shared=False" -o "cnats/*:with_streaming=False" -o "cnats/*:with_tls=True" -o "cnats/*:tls_use_openssl_1_1_api=True" -o "etcd-cpp-apiv3/*:with_nats=True" -o "etcd-cpp-apiv3/*:nats_package_name=cnats" -o "macosx_bundle=True" --build=missing --build=outdated
	@cmake --preset conan-debug
	@cmake --build --preset conan-debug

build-docker-image:
	@docker build . -t pitaya-cluster

build-linux-docker:
	@echo "Building Linux library in Docker container (Ubuntu 22.04) to ensure GLIBC 2.35 compatibility..."
	@$(MAKE) build-docker-image
	@docker run -v $(shell pwd):/app -v conan:/home/conan/.conan2/p -ti pitaya-cluster sh -c "cd /app && make build-linux-release"
	@echo "Linux library built successfully in Docker container"

build-all-mac: build-mac-release build-mac-unity build-linux-docker

run-mac-debug:
	cmake --build --preset conan-debug && ./run-tests-with-coverage.sh _builds/macos-$(LOCAL_ARCH)-macosx_bundle_false-debug/tests
