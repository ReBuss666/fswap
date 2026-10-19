CXX ?= g++
CXXFLAGS ?= -O3 -std=c++17 -Wall -Wextra
PREFIX ?= /usr

all: fswap

fswap: fswap.cpp
	$(CXX) $(CXXFLAGS) fswap.cpp -o fswap

install: fswap
	install -Dm755 fswap $(DESTDIR)$(PREFIX)/bin/fswap

clean:
	rm -f fswap

.PHONY: all install clean
