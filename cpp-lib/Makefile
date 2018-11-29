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

build-linux: clean-docker-container
	@docker build -t pitaya-cluster .
	@docker run --name pitaya-cluster pitaya-cluster:latest
	@docker cp `docker ps -aqf "name=pitaya-cluster"`:/cpp-lib/_builds/linux-make-release/libpitaya_cluster.so ../NugetOutput/binaries/linux
