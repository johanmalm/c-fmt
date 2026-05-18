all:
	$(CC) main.c formatter.c tokenize.c context.c rules.c render.c util.c tree-sitter-c/src/parser.c -o c-fmt -ltree-sitter

test:
	@./c-fmt main.c | diff -u main.c -
	@./c-fmt formatter.c | diff -u formatter.c -
	@./c-fmt context.c | diff -u context.c -

clean:
	$(RM) -f *.o c-fmt
