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

clean-all: clean-mac-debug clean-mac-release clean-mac-unity clean-xcode clean-linux-release

generate-xcode:
	@conan install -of _builds/xcode . --build=missing -pr ./conanprofile -c tools.cmake.cmaketoolchain:generator=Xcode
	@cmake -H. -B_builds/xcode -GXcode --preset npitaya-release -DBUILD_TESTS=ON -DBUILD_SHARED_LIBS=OFF

build-linux-release:
	@conan install . -s arch=x86_64 --build=missing -pr ./conanprofile -s compiler.version=11
	@cmake --preset npitaya-linux-x86_64-release
	@cmake --build --preset npitaya-linux-x86_64-release

build-mac-release:
	for arch in $(ARCHES); do \
		conan install . -s arch=$${arch} --build=missing -pr ./conanprofile; \
		cmake --preset npitaya-macos-$${arch}-macosx_bundle_false-release; \
		cmake --build --preset npitaya-macos-$${arch}-macosx_bundle_false-release; \
	done

build-mac-debug:
	@conan install . --build=missing -pr ./conanprofile -s build_type=Debug -c tools.build:skip_test=False
	@cmake --preset npitaya-macos-$(LOCAL_ARCH)-macosx_bundle_false-debug -DBUILD_GMOCK=ON
	@cmake --build --preset npitaya-macos-$(LOCAL_ARCH)-macosx_bundle_false-debug

build-mac-unity:
	for arch in $(ARCHES); do \
		conan install . -s arch=$${arch} --build=missing -pr ./conanprofile -o macosx_bundle=True; \
		cmake --preset npitaya-macos-$${arch}-macosx_bundle_true-release; \
		cmake --build --preset npitaya-macos-$${arch}-macosx_bundle_true-release; \
	done

build-mac-unity-debug:
	@conan install . --build=missing -pr ./conanprofile -s build_type=Debug -o macosx_bundle=True
	@cmake --preset npitaya-macos-$(LOCAL_ARCH)-macosx_bundle_true-debug
	@cmake --build --preset npitaya-macos-$(LOCAL_ARCH)-macosx_bundle_true-debug

build-docker-image:
	@docker build . -t pitaya-cluster

build-linux-docker:
	@docker run -v $(shell pwd):/app -v conan:/home/conan/.conan2/p -ti pitaya-cluster sh -c "cd /app && make build-linux-release"

build-all-mac: build-mac-release build-mac-unity build-linux-docker

run-mac-debug:
	cmake --build --preset npitaya-macos-$(LOCAL_ARCH)-macosx_bundle_false-debug && ./run-tests-with-coverage.sh _builds/macos-$(LOCAL_ARCH)-macosx_bundle_false-debug/tests
