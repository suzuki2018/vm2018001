.PHONY: all test clean deps tags 

CXX=g++
CXXFLAGS += -g -rdynamic -Wall -std=c++11
LDFLAGS += -pthread

MULTIPART_PARSER_INC_PATH=-I deps/multipart-parser-c/
MULTIPART_PARSER_LIB_PATH=deps/multipart-parser-c/multipart_parser.so

DEPS_INCLUDE_PATH=-I ../atb_sak/src/ -I deps/http-parser/ $(MULTIPART_PARSER_INC_PATH)
DEPS_LIB_PATH= ../atb_sak/libatb_sak.a deps/http-parser/libhttp_parser.a $(MULTIPART_PARSER_LIB_PATH)

SRC_INCLUDE_PATH=-I src
OUTPUT_INCLUDE_PATH=-I output/include
OUTPUT_LIB_PATH=output/lib/libatb_http.a

objects := $(patsubst %.cpp,%.o,$(wildcard src/*.cpp))

all: prepare deps libatb_http.a
	cp src/*.h output/include/
	mv libatb_http.a output/lib/

prepare: 
	mkdir -p output/include output/lib output/bin

tags:
	ctags -R /usr/include src deps

deps:
	make -C deps/http-parser package
	make -C deps/multipart-parser-c

libatb_http.a: $(objects)
	ar -rcs libatb_http.a src/*.o

test:  http_server_test

%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) $(DEPS_INCLUDE_PATH) $(SRC_INCLUDE_PATH) $< -o $@

http_server_test: test/http_server_test.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(DEPS_INCLUDE_PATH) $(OUTPUT_INCLUDE_PATH) $< $(OUTPUT_LIB_PATH) $(DEPS_LIB_PATH) -o output/bin/$@

clean:
	rm -rf src/*.o
	rm -rf output/*
	make -C deps/multipart-parser-c clean
