// SPDX-License-Identifier: GPL-2.0-only
#include <stdio.h>
#include <stdlib.h>
#include <tree_sitter/api.h>

#include "formatter.h"

static void
walk_tree(const char *source, uint32_t source_len, TSNode root, FILE *out)
{
	/*
	 * We perform two passes over the Tree Sitter (TS) syntax tree. The
	 * first pass walks the tree and creates a compact, linear array of
	 * formatting tokens. Each token references its corresponding TS node.
	 */
	struct token_stream token_stream;
	token_stream_build(&token_stream, root, source, source_len);

	/*
	 * The second pass traverses the token array to derive semantic state.
	 * Rather than parsing the C language, it examines the TS node (and its
	 * neighbours) to determine contextual information such as whether the
	 * current token forms part of a function definition, declaration,
	 * parameter list, cast expression, and so on. This semantic context is
	 * attached to the formatting tokens.
	 */
	context_build(&token_stream);

	/*
	 * Finally, we print the tokens to stdout, using the semantic context to
	 * drive decisions on whitespace, indentation and line-breaking.
	 */
	format_render(&token_stream, out);

	token_stream_free(&token_stream);
}

extern const TSLanguage *tree_sitter_c(void);

int
main(int argc, char **argv)
{
	if (argc < 2) {
		fprintf(stderr, "usage: %s <file>\n", argv[0]);
		return EXIT_FAILURE;
	}

	size_t size = 0;
	char *source = grab_file(argv[1], &size);

	TSParser *parser = ts_parser_new();
	ts_parser_set_language(parser, tree_sitter_c());
	TSTree *tree = ts_parser_parse_string(parser, NULL, source, size);
	TSNode root = ts_tree_root_node(tree);

	walk_tree(source, (uint32_t)size, root, stdout);

	ts_tree_delete(tree);
	ts_parser_delete(parser);
	free(source);

	return EXIT_SUCCESS;
}
