#include "formatter.h"

#include <string.h>

static bool
token_is(const Token *tok, const char *s)
{
	return !strcmp(tok->type, s);
}

static char
lex_first(const TokenStream *ts, const Token *tok)
{
	return ts->source[tok->start];
}

static bool
gap_has_blank_line(const TokenStream *ts, uint32_t start, uint32_t end)
{
	int newlines = 0;
	for (uint32_t i = start; i < end; i++) {
		if (ts->source[i] == '\n' && ++newlines >= 2) {
			return true;
		}
	}
	return false;
}

static bool
gap_has_newline(const TokenStream *ts, uint32_t start, uint32_t end)
{
	for (uint32_t i = start; i < end; i++) {
		if (ts->source[i] == '\n') {
			return true;
		}
	}
	return false;
}

static bool
is_operator_token(const Token *tok)
{
	return tok->kind == TOK_OPERATOR;
}

static bool
is_unary_context(const TokenStream *ts, size_t left_index, const Token *left)
{
	if (!is_operator_token(left)) {
		return false;
	}
	if (left_index == 0) {
		return true;
	}
	const Token *prev = &ts->tokens[left_index - 1];
	char c = lex_first(ts, prev);
	if (c == '(' || c == ',' || c == '[' || c == '=' || c == '{' || c == ';' || c == ':' || c == '?') {
		return true;
	}
	if (prev->kind == TOK_KEYWORD) {
		return true;
	}
	if (is_operator_token(prev)) {
		return true;
	}
	return false;
}

static bool
needs_space_after_keyword(const Token *tok)
{
	/* sizeof uses no space before its argument: sizeof(Type) not sizeof (Type). */
	return token_is(tok, "if") || token_is(tok, "while") || token_is(tok, "for")
		|| token_is(tok, "switch") || token_is(tok, "return")
		|| token_is(tok, "case");
}

static WsDecision
newline_indent(int depth)
{
	return (WsDecision){ .kind = WS_NEWLINE_INDENT, .indent = depth };
}

static WsDecision
with_indent(const FormatCtx *ctx, int adjust)
{
	int depth = ctx->indent_depth + adjust;
	if (depth < 0) {
		depth = 0;
	}
	return newline_indent(depth);
}

WsDecision
rules_gap_decision(const TokenStream *ts, size_t index,
		uint32_t gap_start, uint32_t gap_end, const FormatCtx *ctx)
{
	const Token *right = index < ts->count ? &ts->tokens[index] : NULL;
	const Token *left = index > 0 ? &ts->tokens[index - 1] : NULL;

	if (!right) {
		if (gap_has_blank_line(ts, gap_start, gap_end)) {
			return (WsDecision){ .kind = WS_BLANK_LINE };
		}
		return (WsDecision){ .kind = WS_NEWLINE };
	}

	if (!ctx) {
		return (WsDecision){ .kind = WS_PRESERVE };
	}

	if (ctx->in_string_literal) {
		return (WsDecision){ .kind = WS_PRESERVE };
	}

	/* Preprocessor lines. */
	if (ctx->in_preproc || right->kind == TOK_PREPROC) {
		if (left && (left->kind == TOK_PREPROC || token_is(left, "#include"))) {
			return (WsDecision){ .kind = WS_SPACE };
		}
		if (left) {
			if (gap_has_blank_line(ts, gap_start, gap_end)) {
				return (WsDecision){ .kind = WS_BLANK_LINE };
			}
			return (WsDecision){ .kind = WS_NEWLINE };
		}
		if (gap_start == 0 && gap_end > 0 && ts->source[0] == '\n') {
			return (WsDecision){ .kind = WS_NEWLINE };
		}
		return (WsDecision){ .kind = WS_NONE };
	}

	/* Preserve intentional blank lines at file scope. */
	if (ctx->indent_depth == 0 && gap_has_blank_line(ts, gap_start, gap_end)) {
		return (WsDecision){ .kind = WS_BLANK_LINE };
	}

	/* Space after comma; preserve newlines for multi-line argument/initializer lists. */
	if (left && token_is(left, ",")) {
		if (gap_has_newline(ts, gap_start, gap_end)) {
			return (WsDecision){ .kind = WS_PRESERVE };
		}
		return (WsDecision){ .kind = WS_SPACE };
	}

	/* Tight punctuation: no space before closing/separators. */
	if (right->kind == TOK_PUNCT) {
		char c = lex_first(ts, right);
		if (c == ';' || c == ')' || c == ']') {
			return (WsDecision){ .kind = WS_NONE };
		}
		if (c == ':') {
			/* Space before ':' in ternary expressions. */
			if (!strcmp(ctx->parent_type, "conditional_expression")) {
				return (WsDecision){ .kind = WS_SPACE };
			}
			return (WsDecision){ .kind = WS_NONE };
		}
	}

	/* Pointer declarator spacing: type and '*' are separated by a space
	 * ('char *'), while '*' and the identifier have no space ('char *x').
	 * At file scope inside a function definition, emit a newline between
	 * '*' and the function name so the name starts on its own line. */
	if (left && token_is(left, "*") && right->kind == TOK_IDENTIFIER) {
		/* indent_depth == 0 means file scope. */
		if (ctx && ctx->in_function_definition && !ctx->in_parameter_list
				&& ctx->indent_depth == 0) {
			return (WsDecision){ .kind = WS_NEWLINE };
		}
		return (WsDecision){ .kind = WS_NONE };
	}

	/* No space after opening delimiters or unary operators. */
	if (left) {
		char lc = lex_first(ts, left);
		if (lc == '(' || lc == '[') {
			return (WsDecision){ .kind = WS_NONE };
		}
		if (is_unary_context(ts, index - 1, left)) {
			return (WsDecision){ .kind = WS_NONE };
		}
	}

	/* Keyword before '('. */
	if (left && left->kind == TOK_KEYWORD && needs_space_after_keyword(left)
			&& lex_first(ts, right) == '(') {
		return (WsDecision){ .kind = WS_SPACE };
	}

	/* 'else' / 'while' (do-while) after closing brace. */
	if (left && token_is(left, "}") && right->kind == TOK_KEYWORD
			&& (token_is(right, "else") || token_is(right, "while"))) {
		return (WsDecision){ .kind = WS_SPACE };
	}

	/* Function body opening brace on its own line; if/else/for keep ') {'.
	 * Compound literals '(Type){ ... }' have no space before '{'. */
	if (left && token_is(left, ")") && token_is(right, "{")) {
		if (!strcmp(ctx->parent_type, "initializer_list")) {
			return (WsDecision){ .kind = WS_NONE };
		}
		const FormatCtx *left_ctx = &ts->contexts[index - 1];
		if (left_ctx->in_condition || left_ctx->in_for_header) {
			return (WsDecision){ .kind = WS_SPACE };
		}
		return (WsDecision){ .kind = WS_NEWLINE };
	}

	/* Closing brace alignment — only for compound statements. */
	if (token_is(right, "}") && !strcmp(ctx->parent_type, "compound_statement")) {
		return with_indent(ctx, -1);
	}

	/* First token inside a compound statement. */
	if (left && token_is(left, "{") && ctx->in_compound_statement) {
		const FormatCtx *left_ctx = &ts->contexts[index - 1];
		if (!strcmp(left_ctx->parent_type, "compound_statement")) {
			if (right && (token_is(right, "case") || token_is(right, "default"))) {
				return with_indent(ctx, -1);
			}
			return newline_indent(ctx->indent_depth);
		}
	}

	/* Statement boundaries. */
	if (left && token_is(left, ";") && ctx->in_compound_statement) {
		const FormatCtx *left_ctx = &ts->contexts[index - 1];
		if (left_ctx->in_for_header) {
			return (WsDecision){ .kind = WS_SPACE };
		}
		if (right && (token_is(right, "case") || token_is(right, "default"))) {
			return with_indent(ctx, -1);
		}
		if (gap_has_blank_line(ts, gap_start, gap_end)) {
			return (WsDecision){ .kind = WS_BLANK_LINE, .indent = ctx->indent_depth };
		}
		return newline_indent(ctx->indent_depth);
	}

	/* Postfix ++/-- has no space before it. */
	if (right && (token_is(right, "++") || token_is(right, "--")) && left
			&& (left->kind == TOK_IDENTIFIER
				|| (left->kind == TOK_PUNCT
					&& (lex_first(ts, left) == ')' || lex_first(ts, left) == ']')))) {
		return (WsDecision){ .kind = WS_NONE };
	}

	/* Operators: space on both sides unless unary; preserve multi-line chains. */
	if (left && is_operator_token(left) && !is_unary_context(ts, index - 1, left)) {
		if (gap_has_newline(ts, gap_start, gap_end)) {
			return (WsDecision){ .kind = WS_PRESERVE };
		}
		return (WsDecision){ .kind = WS_SPACE };
	}
	if (is_operator_token(right)) {
		if (token_is(right, "*") && left && token_is(left, "*")) {
			return (WsDecision){ .kind = WS_NONE };
		}
		if ((token_is(right, "*") || token_is(right, "&"))
				&& left
				&& (left->kind == TOK_KEYWORD || left->kind == TOK_IDENTIFIER)) {
			return (WsDecision){ .kind = WS_SPACE };
		}
		if (gap_has_newline(ts, gap_start, gap_end)) {
			return (WsDecision){ .kind = WS_PRESERVE };
		}
		return (WsDecision){ .kind = WS_SPACE };
	}

	/* Top-level and declaration boundaries. */
	if (ctx->indent_depth == 0 && left) {
		if (token_is(left, "}") || token_is(left, ";")) {
			return (WsDecision){ .kind = WS_NEWLINE };
		}
		if (left->kind == TOK_PREPROC || token_is(left, "#include")) {
			if (right->kind != TOK_PREPROC && lex_first(ts, right) != '#') {
				return (WsDecision){ .kind = WS_BLANK_LINE };
			}
			return (WsDecision){ .kind = WS_NEWLINE };
		}
	}

	/* Blank line between top-level declarations. */
	if (ctx->indent_depth == 0 && left && token_is(left, "}")
			&& (right->kind == TOK_KEYWORD || right->kind == TOK_IDENTIFIER)) {
		return (WsDecision){ .kind = WS_BLANK_LINE };
	}

	/* Leading file newline. */
	if (!left && gap_start < gap_end && ts->source[gap_start] == '\n') {
		return (WsDecision){ .kind = WS_NEWLINE };
	}

	return (WsDecision){ .kind = WS_PRESERVE };
}
