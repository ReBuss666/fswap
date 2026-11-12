CXX ?= g++
CXXFLAGS ?= -std=c++17 -O3 -Wall -Wextra
LDFLAGS ?=

PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin
DATADIR ?= $(PREFIX)/share
BASH_COMPLETION_DIR ?= $(DATADIR)/bash-completion/completions
ZSH_COMPLETION_DIR ?= $(DATADIR)/zsh/site-functions

TARGET = fswap
SRC = main.cpp

.PHONY: all clean install uninstall

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)

clean:
	rm -f $(TARGET)

install: $(TARGET)
	install -Dm755 $(TARGET) $(DESTDIR)$(BINDIR)/$(TARGET)
	install -Dm644 fswap.bash $(DESTDIR)$(BASH_COMPLETION_DIR)/$(TARGET)
	install -Dm644 _fswap $(DESTDIR)$(ZSH_COMPLETION_DIR)/_$(TARGET)

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(TARGET)
	rm -f $(DESTDIR)$(BASH_COMPLETION_DIR)/$(TARGET)
	rm -f $(DESTDIR)$(ZSH_COMPLETION_DIR)/_$(TARGET)
