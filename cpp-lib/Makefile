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
	@conan install -of _builds/xcode . --build=missing -s compiler.cppstd=gnu17  -c tools.cmake.cmaketoolchain:generator=Xcode
	@cmake -H. -B_builds/xcode -GXcode --preset npitaya-release -DBUILD_TESTS=ON -DBUILD_SHARED_LIBS=OFF

build-linux-release:
	@conan install -of _builds/linux . --build=missing -s compiler.cppstd=gnu17 -c tools.cmake.cmaketoolchain:generator=Ninja
	@cmake -H. -B_builds/linux --preset npitaya-release -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=OFF -DBUILD_SHARED_LIBS=OFF -GNinja
	@cmake --build _builds/linux

build-mac-release:
	@conan install . -of _builds/mac --build=missing -s compiler.cppstd=gnu17 -c tools.cmake.cmaketoolchain:generator=Ninja -c tools.cmake.cmake_layout:build_folder_vars="['settings.compiler', 'options.shared']"
	@cmake -H. -B_builds/mac -GNinja --preset npitaya-apple-clang-release -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=OFF -DBUILD_SHARED_LIBS=OFF
	@cmake --build _builds/mac

build-mac-debug:
	@conan install . -of _builds/mac-debug --build=missing -s compiler.cppstd=gnu17 -s build_type=Debug -c tools.cmake.cmaketoolchain:generator=Ninja -c tools.cmake.cmake_layout:build_folder_vars="['settings.compiler', 'options.shared']"
	@cmake -H. -B _builds/mac-debug --preset npitaya-apple-clang-debug -DBUILD_GMOCK=ON -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON -DBUILD_SHARED_LIBS=OFF
	@cmake --build _builds/mac-debug

build-mac-unity:
	@conan install -of _builds/mac-unity . --build=missing -s compiler.cppstd=gnu17 -o macosx_bundle=True -c tools.cmake.cmaketoolchain:generator=Ninja -c tools.cmake.cmake_layout:build_folder_vars="['settings.compiler', 'options.shared', 'options.macosx_bundle']"
	@cmake -H. -B_builds/mac-unity -GNinja --preset npitaya-apple-clang-macosx_bundle_true-release -DCMAKE_BUILD_TYPE=Release -DBUILD_MACOSX_BUNDLE=ON -DBUILD_TESTS=OFF -DBUILD_SHARED_LIBS=OFF
	@cmake --build _builds/mac-unity

build-mac-unity-debug:
	@conan install -of _builds/mac-unity-debug .  --build=missing -s compiler.cppstd=gnu17 -s build_type=Debug -o macosx_bundle=True -c tools.cmake.cmaketoolchain:generator=Ninja -c tools.cmake.cmake_layout:build_folder_vars="['settings.compiler', 'options.shared', 'options.macosx_bundle']"
	@cmake -H. -B_builds/mac-unity-debug -GNinja --preset npitaya-apple-clang-macosx_bundle_true-debug -DCMAKE_BUILD_TYPE=Debug -DBUILD_MACOSX_BUNDLE=ON -DBUILD_TESTS=OFF -DBUILD_SHARED_LIBS=OFF
	@cmake --build _builds/mac-unity-debug

build-docker-image:
	@docker build . -t pitaya-cluster

build-linux-docker:
	@docker run -v $(shell pwd):/app -v conan:/home/conan/.conan2/p -ti pitaya-cluster sh -c "cd /app && make build-linux-release"

build-all-mac: build-mac-release build-mac-unity build-linux-docker

run-mac-debug:
	cmake --build _builds/mac-debug && ./run-tests-with-coverage.sh _builds/mac-debug/tests
