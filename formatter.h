#ifndef FORMATTER_H
#define FORMATTER_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <tree_sitter/api.h>

typedef enum {
	TOK_IDENTIFIER,
	TOK_KEYWORD,
	TOK_NUMBER,
	TOK_STRING,
	TOK_CHAR,
	TOK_OPERATOR,
	TOK_PUNCT,
	TOK_PREPROC,
	TOK_OTHER,
} TokenKind;

typedef struct Token {
	uint32_t start;
	uint32_t end;
	TokenKind kind;
	const char *type;
} Token;

typedef struct FormatCtx {
	int indent_depth;
	bool in_argument_list;
	bool in_parameter_list;
	bool in_preproc;
	bool in_condition;
	bool in_compound_statement;
	bool in_function_definition;
	bool in_string_literal;
	bool in_for_header;
	const char *parent_type;
} FormatCtx;

typedef struct TokenStream {
	Token *tokens;
	FormatCtx *contexts;
	size_t count;
	size_t capacity;
	const char *source;
	uint32_t source_len;
	uint32_t root_end;
	TSNode root;
} TokenStream;

typedef enum {
	WS_NONE,
	WS_SPACE,
	WS_NEWLINE,
	WS_BLANK_LINE,
	WS_NEWLINE_INDENT,
	WS_PRESERVE,
} WsKind;

typedef struct {
	WsKind kind;
	int indent;
} WsDecision;

#define WS_INDENT_USE_CTX (-1)

void token_stream_build(TokenStream *ts, TSNode root, const char *source,
	uint32_t source_len);
void token_stream_free(TokenStream *ts);

void context_build(TokenStream *ts);

WsDecision rules_gap_decision(const TokenStream *ts, size_t index,
	uint32_t gap_start, uint32_t gap_end, const FormatCtx *ctx);

void format_render(const TokenStream *ts, FILE *out);
void format_file(const char *source, uint32_t source_len, TSNode root, FILE *out);

#endif
