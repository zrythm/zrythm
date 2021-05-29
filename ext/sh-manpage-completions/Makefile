LEXERS = $(wildcard *.l)
IDIR = include
INCS = $(wildcard $(IDIR)/*.hpp)
SRCS = $(wildcard src/*.cpp)

CC = g++
CFLAGS = --std=c++11

all: scanner

lex.yy.cc: $(LEXERS)
	flex $^

scanner: lex.yy.cc $(SRCS) $(INCS)
	$(CC) -I$(IDIR) $(CFLAGS) -o $@ lex.yy.cc $(SRCS)

clean:
	$(RM) $(TARGET) lex.yy.cc

.PHONY: clean
