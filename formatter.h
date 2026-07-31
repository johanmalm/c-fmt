/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef FORMATTER_H
#define FORMATTER_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <tree_sitter/api.h>

enum token_kind {
	TOK_IDENTIFIER,
	TOK_KEYWORD,
	TOK_NUMBER,
	TOK_STRING,
	TOK_CHAR,
	TOK_OPERATOR,
	TOK_PUNCT,
	TOK_PREPROC,
	TOK_OTHER,
};

struct format_ctx {
	int indent_depth;
	bool in_argument_list;
	bool in_parameter_list;
	bool in_preproc;
	bool in_condition;
	bool in_compound_statement;
	bool in_function_definition;
	bool in_string_literal;
	bool in_for_header;
	bool in_field_declaration_list;
	bool in_switch_body;
	const char *parent_type;
};

struct token {
	uint32_t start;
	uint32_t end;
	enum token_kind kind;
	const char *type;
	struct format_ctx format_ctx;
};

struct token_stream {
	struct token *tokens;
	size_t count;
	size_t capacity;
	const char *source;
	uint32_t source_len;
	uint32_t root_end;
	TSNode root;
};

enum whitespace_kind {
	WS_NONE,
	WS_SPACE,
	WS_NEWLINE,
	WS_BLANK_LINE,
	WS_NEWLINE_INDENT,
	WS_PRESERVE,
};

struct whitespace_decision {
	enum whitespace_kind kind;
	int indent;
};

#define WS_INDENT_USE_CTX (-1)

void token_stream_build(struct token_stream *ts, TSNode root, const char *source,
	uint32_t source_len);
void token_stream_free(struct token_stream *ts);

void context_build(struct token_stream *ts);

struct whitespace_decision rules_gap_decision(const struct token_stream *ts, size_t index,
	uint32_t gap_start, uint32_t gap_end, const struct format_ctx *ctx);

void format_render(const struct token_stream *ts, FILE *out);

static inline char *
grab_file(const char *path, size_t *size_out)
{
	FILE *f = fopen(path, "rb");
	if (!f) {
		perror(path);
		exit(EXIT_FAILURE);
	}
	if (fseek(f, 0, SEEK_END) == -1) {
		perror(path);
		fclose(f);
		exit(EXIT_FAILURE);
	}
	size_t size = ftell(f);
	if (size == -1) {
		perror(path);
		fclose(f);
		exit(EXIT_FAILURE);
	}
	rewind(f);
	char *buffer = malloc(size + 1);
	fread(buffer, 1, size, f);
	buffer[size] = '\0';
	fclose(f);
	*size_out = size;
	return buffer;
}

#endif
