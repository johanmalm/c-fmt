#include <stdio.h>
#include <stdlib.h>
#include <tree_sitter/api.h>

#include "formatter.h"

extern const TSLanguage *tree_sitter_c(void);

static char *
read_file(const char *path, size_t *size_out)
{
	FILE *f = fopen(path, "rb");
	if (!f) {
		perror(path);
		exit(1);
	}
	fseek(f, 0, SEEK_END);
	size_t size = ftell(f);
	rewind(f);
	char *buffer = malloc(size + 1);
	fread(buffer, 1, size, f);
	buffer[size] = '\0';
	fclose(f);
	*size_out = size;
	return buffer;
}

int
main(int argc, char **argv)
{
	if (argc < 2) {
		fprintf(stderr, "usage: %s file.c\n", argv[0]);
		return 1;
	}

	size_t size;
	char *source = read_file(argv[1], &size);
	TSParser *parser = ts_parser_new();
	ts_parser_set_language(parser, tree_sitter_c());
	TSTree *tree = ts_parser_parse_string(parser, NULL, source, size);
	TSNode root = ts_tree_root_node(tree);

	format_file(source, (uint32_t)size, root, stdout);
	ts_tree_delete(tree);
	ts_parser_delete(parser);
	free(source);

	return 0;
}
