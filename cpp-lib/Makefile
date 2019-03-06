SRC_FILES = $(shell find src test -name '*.cpp')
HDR_FILES = $(shell find include test -not -path 'include/protos/*' -not -path 'test/doctest.h' -name '*.h')

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

build-linux:
	@rm -rf _builds/linux
	@conan install -if _builds/linux .
	@cmake -H. -GNinja -B_builds/linux -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF -DBUILD_SHARED_LIBS=OFF
	@cmake --build _builds/linux

build-mac:
	@mkdir -p _builds/mac
	@conan install -if _builds/mac .
	@cmake -H. -B_builds/mac -GNinja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF -DBUILD_SHARED_LIBS=OFF
	@cmake --build _builds/mac

build-mac-unity:
	@mkdir -p _builds/mac-unity
	@conan install -if _builds/mac-unity .
	@cmake -H. -B_builds/mac-unity -GNinja -DCMAKE_BUILD_TYPE=Release -DBUILD_MACOSX_BUNDLE=ON -DBUILD_TESTING=OFF -DBUILD_SHARED_LIBS=OFF
	@cmake --build _builds/mac-unity

#build-mac:
#	@mkdir -p _builds/mac
#	@conan install -if _builds/mac .
#	@cmake -H. -B_builds/mac -GNinja -DCMAKE_BUILD_TYPE=Release -DBUILD_MACOSX_BUNDLE=ON -DBUILD_TESTING=OFF -DBUILD_SHARED_LIBS=OFF
#	@cmake --build _builds/mac

build-docker-image:
	@docker build . -t pitaya-cluster

build-linux-docker:
	@docker run -v $(shell pwd):/app -ti pitaya-cluster sh -c "cd /app && make build-linux"
