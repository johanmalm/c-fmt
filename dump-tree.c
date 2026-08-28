#include <stdio.h>
#include <string.h>
#include <tree_sitter/api.h>
#include "formatter.h"
extern const TSLanguage *tree_sitter_c(void);
static void
dump(const char *label, const char *src)
{
	TSParser *parser = ts_parser_new();
	ts_parser_set_language(parser, tree_sitter_c());
	TSTree *tree = ts_parser_parse_string(parser, NULL, src, strlen(src));
	TSNode root = ts_tree_root_node(tree);
	struct token_stream ts;
	token_stream_build(&ts, root, src, (uint32_t)strlen(src));
	context_build(&ts);
	printf("%s:\n", label);
	for (size_t i = 0; i < ts.count; i++) {
		char buf[32] = {0};
		memcpy(buf, src + ts.tokens[i].start,
			ts.tokens[i].end - ts.tokens[i].start < 31 ?
			ts.tokens[i].end - ts.tokens[i].start : 31);

		if (strcmp(buf, "-") == 0 || strcmp(buf, ")") == 0) {
			uint32_t start = ts.tokens[i].start;
			uint32_t end = ts.tokens[i].end;
			if (end <= start) {
				end = start + 1;
			}
			TSNode node = ts_node_descendant_for_byte_range(root, start, end);

			for (TSNode n = node; !ts_node_is_null(n); n = ts_node_parent(n)) {
				const char *t = ts_node_type(n);
				if (!strcmp(t, "translation_unit")) {
					break;
				}
				if (!strcmp(t, "cast_expression") || !strcmp(t, "parenthesized_expression")
						|| !strcmp(t, "binary_expression")
						|| !strcmp(t, "type_descriptor")) {
					printf("  %-10s ancestor=%s\n", buf, t);
				}
			}
		}
	}
	token_stream_free(&ts);
	ts_tree_delete(tree);
	ts_parser_delete(parser);
}

int
main(void)
{
	dump("cast_g", "x = (gunichar)-1;");
	dump("cast_i", "x = (int)-1;");
	dump("var", "a = (a)-1;");
	return 0;
}
