#makefile for all .cpp and .h files in current directory

CXX = riscv64-unknown-elf-g++
override CXXFLAGS += -g -Wall -std=c++11

#find all sources and headers
SRCS = $(shell find . -name '.ccls-cache' -type d -prune -o -type f -name '*.cpp' -print | sed -e 's/ /\\ /g')
HEADERS = $(shell find . -name '.ccls-cache' -type d -prune -o -type f -name '*.h' -print)

#build with release optimizations
main: $(SRCS) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(SRCS) -o "$@"

#build debug version (no optimizations)
main-debug: $(SRCS) $(HEADERS)
	$(CXX) $(CXXFLAGS) -O0 $(SRCS) -o "$@"

#remove any builds
clean:
	rm -f main main-debug
	
all: main
	./main
