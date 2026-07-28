CFLAGS += -g -Wall
LDFLAGS += `pkg-config --libs tree-sitter`
PREFIX  ?= $(HOME)
bindir  ?= $(PREFIX)/bin
CC = gcc
RM = rm -f

PROG = c-fmt

OBJ = \
      main.o \
      tokenize.o \
      context.o \
      rules.o \
      render.o \
      tree-sitter-c/src/parser.o

all: $(PROG)

$(PROG): $(OBJ)
	@echo '     LINK  '$@;$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o : %.c
	@echo '     CC    '$@;$(CC) $(CFLAGS) -c $< -o $@

install: $(PROG)
	@install -d $(DESTDIR)$(bindir)
	@install -m755 $^ $(DESTDIR)$(bindir)

test: $(PROG)
	$(MAKE) -C t/

clean:
	$(RM) $(OBJ) c-fmt
