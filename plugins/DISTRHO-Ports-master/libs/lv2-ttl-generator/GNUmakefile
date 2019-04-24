#!/usr/bin/makefile -f

all: build

build: ../lv2_ttl_generator
mingw: ../lv2_ttl_generator.exe

../lv2_ttl_generator: lv2_ttl_generator.cpp
	$(CXX) lv2_ttl_generator.cpp -o ../lv2_ttl_generator -ldl

../lv2_ttl_generator.exe: lv2_ttl_generator.cpp
	$(CXX) lv2_ttl_generator.cpp -o ../lv2_ttl_generator.exe -static
	touch ../lv2_ttl_generator

clean:
	rm -f ../lv2_ttl_generator ../lv2_ttl_generator.exe
