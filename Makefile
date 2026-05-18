CFLAGS += -g -Wall
LDFLAGS += `pkg-config --libs tree-sitter`
PREFIX  ?= $(HOME)
bindir  ?= $(PREFIX)/bin
CC = gcc
RM = rm -f

PROG = c-fmt

SCRIPT = \
	c-fmt-run \
	c-fmt-test-labwc

OBJ = \
      main.o \
      formatter.o \
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

install: $(PROG) $(SCRIPT)
	@install -d $(DESTDIR)$(bindir)
	@install -m755 $^ $(DESTDIR)$(bindir)

.ONESHELL:
test: $(PROG)
	@./c-fmt main.c | diff -u main.c -
	@./c-fmt formatter.c | diff -u formatter.c -
	@./c-fmt tokenize.c | diff -u tokenize.c -
	@./c-fmt tokenize.c | diff -u tokenize.c -
	@./c-fmt context.c | diff -u context.c -
	@./c-fmt rules.c | diff -u rules.c -
	@./c-fmt render.c | diff -u render.c -

clean:
	$(RM) $(OBJ) c-fmt
