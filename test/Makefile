CXX ?= g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -I../common -I../receiver/src

test: test_decoder
	./test_decoder

test_decoder: test_decoder.cpp ../receiver/src/decoder.cpp ../receiver/src/decoder.h ../common/protocol.h
	$(CXX) $(CXXFLAGS) -o $@ test_decoder.cpp ../receiver/src/decoder.cpp

clean:
	rm -f test_decoder

.PHONY: test clean
