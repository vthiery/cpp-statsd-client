# simple makefile to build, test and clean

BUILD_MODE ?= Release

build: clean
	@echo "Build in ${BUILD_MODE} mode"
	mkdir -p bin/${BUILD_MODE}
	@cd bin/${BUILD_MODE}; cmake ../../ -DCMAKE_BUILD_TYPE=${BUILD_MODE} -DCMAKE_CXX_FLAGS="${CMAKE_CXX_FLAGS} -std=c++17"
	@cd bin/${BUILD_MODE}; make

test: build
	@cd bin/${BUILD_MODE}; make test

install: build
	@cd bin/${BUILD_MODE}; make install

clean:
	@rm -rf bin

