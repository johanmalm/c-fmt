#include "formatter.h"

#include <stdlib.h>
#include <string.h>

static bool
is_syntax_leaf(TSNode node)
{
	return ts_node_child_count(node) == 0;
}

static bool
type_is_keyword(const char *type)
{
	return !strcmp(type, "if") || !strcmp(type, "else")
		|| !strcmp(type, "while") || !strcmp(type, "for")
		|| !strcmp(type, "switch") || !strcmp(type, "case")
		|| !strcmp(type, "default") || !strcmp(type, "do")
		|| !strcmp(type, "return") || !strcmp(type, "break")
		|| !strcmp(type, "continue") || !strcmp(type, "goto")
		|| !strcmp(type, "sizeof") || !strcmp(type, "static")
		|| !strcmp(type, "extern") || !strcmp(type, "typedef")
		|| !strcmp(type, "const") || !strcmp(type, "volatile")
		|| !strcmp(type, "register") || !strcmp(type, "inline")
		|| !strcmp(type, "restrict") || !strcmp(type, "struct")
		|| !strcmp(type, "union") || !strcmp(type, "enum")
		|| !strcmp(type, "auto") || !strcmp(type, "signed")
		|| !strcmp(type, "unsigned") || !strcmp(type, "void")
		|| !strcmp(type, "short") || !strcmp(type, "long")
		|| !strcmp(type, "int") || !strcmp(type, "char")
		|| !strcmp(type, "float") || !strcmp(type, "double")
		|| !strcmp(type, "_Bool") || !strcmp(type, "_Complex")
		|| !strcmp(type, "_Imaginary");
}

static TokenKind
classify_token(const char *type)
{
	if (strncmp(type, "preproc_", 8) == 0 || type[0] == '#') {
		return TOK_PREPROC;
	}
	if (type_is_keyword(type)) {
		return TOK_KEYWORD;
	}
	if (!strcmp(type, "primitive_type")) {
		return TOK_KEYWORD;
	}
	if (!strcmp(type, "identifier") || !strcmp(type, "type_identifier")
			|| !strcmp(type, "field_identifier")) {
		return TOK_IDENTIFIER;
	}
	if (!strcmp(type, "number_literal")) {
		return TOK_NUMBER;
	}
	if (!strcmp(type, "string_literal")
			|| !strcmp(type, "string_content")
			|| !strcmp(type, "system_lib_string")
			|| !strcmp(type, "concatenated_string")) {
		return TOK_STRING;
	}
	if (!strcmp(type, "char_literal")) {
		return TOK_CHAR;
	}
	if (!strcmp(type, "(") || !strcmp(type, ")")
			|| !strcmp(type, "{") || !strcmp(type, "}")
			|| !strcmp(type, "[") || !strcmp(type, "]")
			|| !strcmp(type, ";") || !strcmp(type, ",")
			|| !strcmp(type, ".") || !strcmp(type, "->")
			|| !strcmp(type, ":") || !strcmp(type, "?")) {
		return TOK_PUNCT;
	}
	if (strlen(type) <= 3) {
		const char *ops = "<>=!+-*/%&|^~";
		if (strchr(ops, type[0])) {
			return TOK_OPERATOR;
		}
	}
	return TOK_OTHER;
}

static int
token_stream_push(TokenStream *ts, TSNode node)
{
	if (ts->count >= ts->capacity) {
		size_t cap = ts->capacity ? ts->capacity * 2 : 256;
		Token *grown = realloc(ts->tokens, cap * sizeof(Token));
		if (!grown) {
			return -1;
		}
		ts->tokens = grown;
		ts->capacity = cap;
	}

	const char *type = ts_node_type(node);
	Token *tok = &ts->tokens[ts->count++];
	tok->start = ts_node_start_byte(node);
	tok->end = ts_node_end_byte(node);
	tok->type = type;
	tok->kind = classify_token(type);
	return 0;
}

static int
collect_tokens(TokenStream *ts, TSNode node)
{
	if (is_syntax_leaf(node)) {
		return token_stream_push(ts, node);
	}

	uint32_t count = ts_node_child_count(node);
	for (uint32_t i = 0; i < count; i++) {
		TSNode child = ts_node_child(node, i);
		if (ts_node_is_null(child)) {
			continue;
		}
		if (!strcmp(ts_node_type(child), "ERROR")) {
			continue;
		}
		if (collect_tokens(ts, child) != 0) {
			return -1;
		}
	}
	return 0;
}

void
token_stream_build(TokenStream *ts, TSNode root, const char *source,
		uint32_t source_len)
{
	ts->tokens = NULL;
	ts->contexts = NULL;
	ts->count = 0;
	ts->capacity = 0;
	ts->source = source;
	ts->source_len = source_len;
	ts->root = root;
	ts->root_end = ts_node_end_byte(root);
	collect_tokens(ts, root);
}

void
token_stream_free(TokenStream *ts)
{
	free(ts->tokens);
	free(ts->contexts);
	ts->tokens = NULL;
	ts->contexts = NULL;
	ts->count = 0;
	ts->capacity = 0;
}
