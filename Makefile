CFLAGS += -g -Wall
LDFLAGS += `pkg-config --libs tree-sitter`
CC = gcc

PROG = c-fmt
OBJ = \
      main.o \
      formatter.o \
      tokenize.o \
      context.o \
      rules.o \
      render.o \
      tree-sitter-c/src/parser.o

all: $(PROG)

c-fmt: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

.ONESHELL:
test:
	@./c-fmt main.c | diff -u main.c -
	@./c-fmt formatter.c | diff -u formatter.c -
	@./c-fmt tokenize.c | diff -u tokenize.c -
	@./c-fmt tokenize.c | diff -u tokenize.c -
	@./c-fmt context.c | diff -u context.c -
	@./c-fmt rules.c | diff -u rules.c -
	@./c-fmt render.c | diff -u render.c -

clean:
	$(RM) -f *.o c-fmt
