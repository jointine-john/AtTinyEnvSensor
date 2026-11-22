CXX ?= g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -I../common -I../receiver/src

test: test_decoder
	./test_decoder

SRCS = ../receiver/src/decoder.cpp ../receiver/src/packet.cpp
HDRS = ../receiver/src/decoder.h ../receiver/src/packet.h ../common/protocol.h

test_decoder: test_decoder.cpp $(SRCS) $(HDRS)
	$(CXX) $(CXXFLAGS) -o $@ test_decoder.cpp $(SRCS)

clean:
	rm -f test_decoder

.PHONY: test clean
