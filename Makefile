# Use bash as the default shell
SHELL := /usr/bin/env bash

ifdef $(LC_ALL)
	undefine LC_ALL
endif

UNAME := $(shell uname)
ifeq ($(UNAME), Linux)
NPROC = $(shell nproc)
endif
ifeq ($(UNAME), Darwin)
NPROC = $(shell sysctl -n hw.physicalcpu)
endif

ifeq ($(CPU_CORES),)
	CPU_CORES := $(NPROC)
	ifeq ($(CPU_CORES),)
		CPU_CORES := 2  # Good minimum assumption
	endif
endif

PREFIX ?= /usr/local
ADDITIONAL_CMAKE_OPTIONS ?=
export CTEST_PARALLEL_LEVEL = $(CPU_CORES)

release: run-cmake-release
	cmake --build build -j $(CPU_CORES)

debug: run-cmake-debug
	cmake --build build -j $(CPU_CORES)

run-cmake-release:
	cmake -DCMAKE_BUILD_TYPE=RELEASE -DCMAKE_INSTALL_PREFIX=$(PREFIX) -DBUILD_SHARED_LIBS=OFF $(ADDITIONAL_CMAKE_OPTIONS) -S . -B build

run-cmake-debug:
	cmake -DCMAKE_BUILD_TYPE=DEBUG -DCMAKE_INSTALL_PREFIX=$(PREFIX) -DBUILD_SHARED_LIBS=OFF $(ADDITIONAL_CMAKE_OPTIONS) -S . -B build

test:
	cd test && python3 run-tests.py

test_verific:
	cd test && export VERIFIC_PARSER=1 && python3 run-tests.py 

clean:
	rm -rf build
	cd third_party/verific/ && rm -f */*.o */*.a

.PHONY: test
