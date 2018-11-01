SRC_FILES = $(shell find src -name '*.cpp')
HDR_FILES = $(shell find include -not -path 'include/protos/*' -name '*.h')

format:
	@for f in ${SRC_FILES} ; do \
		clang-format -i $$f ; \
	done
	@for f in ${HDR_FILES} ; do \
		clang-format -i $$f ; \
	done
