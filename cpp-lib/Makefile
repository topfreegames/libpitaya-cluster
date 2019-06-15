SRC_FILES = $(shell find src test -name '*.cpp')
HDR_FILES = $(shell find include test -not -path 'include/pitaya/protos/*' -not -path 'test/doctest.h' -name '*.h')

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
	@conan install . -if _builds/xcode
	@cmake -H. -B_builds/xcode -GXcode -DBUILD_TESTING=ON -DBUILD_SHARED_LIBS=OFF

build-linux-release:
	@conan install -if _builds/linux .
	@cmake -H. -B_builds/linux -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF -DBUILD_SHARED_LIBS=OFF -GNinja
	@cmake --build _builds/linux

build-mac-release:
	@conan install . -if _builds/mac
	@cmake -H. -B_builds/mac -GNinja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF -DBUILD_SHARED_LIBS=OFF
	@cmake --build _builds/mac

build-mac-debug:
	@conan install . -if _builds/mac-debug
	@cmake -H. -B_builds/mac-debug -GNinja -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON -DBUILD_SHARED_LIBS=OFF
	@cmake --build _builds/mac-debug

build-mac-unity:
	@conan install -if _builds/mac-unity .
	@cmake -H. -B_builds/mac-unity -GNinja -DCMAKE_BUILD_TYPE=Release -DBUILD_MACOSX_BUNDLE=ON -DBUILD_TESTING=OFF -DBUILD_SHARED_LIBS=OFF
	@cmake --build _builds/mac-unity

build-mac-unity-debug:
	@conan install -if _builds/mac-unity-debug .
	@cmake -H. -B_builds/mac-unity-debug -GNinja -DCMAKE_BUILD_TYPE=Debug -DBUILD_MACOSX_BUNDLE=ON -DBUILD_TESTING=OFF -DBUILD_SHARED_LIBS=OFF
	@cmake --build _builds/mac-unity-debug

build-docker-image:
	@docker build . -t pitaya-cluster

build-linux-docker:
	@docker run -v $(shell pwd):/app -ti pitaya-cluster sh -c "cd /app && make build-linux-release"

build-all-mac: build-mac-release build-mac-unity build-linux-docker

