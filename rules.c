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
	/* Assignment operators are always binary; never treat them as unary. */
	if (token_is(left, "=") || token_is(left, "+=") || token_is(left, "-=")
			|| token_is(left, "*=") || token_is(left, "/=")
			|| token_is(left, "%=") || token_is(left, "&=")
			|| token_is(left, "|=") || token_is(left, "^=")
			|| token_is(left, "<<=") || token_is(left, ">>=")) {
		return false;
	}
	/*
	 * Use the AST parent type to reliably identify unary/pointer operators.
	 * This handles cases like dereference (*ptr), address-of (&x), logical
	 * not (!x), and pointer declarators (**pp) where token-level heuristics
	 * would incorrectly classify them as binary operators.
	 */
	if (ts->contexts && left_index < ts->count) {
		const char *pt = ts->contexts[left_index].parent_type;
		if (!strcmp(pt, "pointer_declarator")
				|| !strcmp(pt, "abstract_pointer_declarator")
				|| !strcmp(pt, "pointer_expression")
				|| !strcmp(pt, "unary_expression")
				|| !strcmp(pt, "update_expression")) {
			return true;
		}
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

	/* Preserve spacing around preprocessor tokens and within preproc blocks. */
	if (ctx->in_preproc || right->kind == TOK_PREPROC
			|| (left && left->kind == TOK_PREPROC)) {
		return (WsDecision){ .kind = WS_PRESERVE };
	}

	/* Space after comma; preserve newlines for multi-line argument/initializer lists. */
	if (left && token_is(left, ",")) {
		if (gap_has_newline(ts, gap_start, gap_end)) {
			return (WsDecision){ .kind = WS_PRESERVE };
		}
		return (WsDecision){ .kind = WS_SPACE };
	}

	/* Preserve intentional blank lines at file scope.
	 * Inside struct/union bodies, use WS_PRESERVE so member indentation
	 * is not lost. */
	if (ctx->indent_depth == 0 && gap_has_blank_line(ts, gap_start, gap_end)) {
		if (ctx->in_field_declaration_list) {
			return (WsDecision){ .kind = WS_PRESERVE };
		}
		return (WsDecision){ .kind = WS_BLANK_LINE };
	}

	/* Tight punctuation: no space before closing/separators. */
	if (right->kind == TOK_PUNCT) {
		char c = lex_first(ts, right);
		if (c == ';' || c == ')' || c == ']' || c == ',') {
			/* Preserve intentional line breaks before closing delimiters. */
			if ((c == ')' || c == ']') && gap_has_newline(ts, gap_start, gap_end)) {
				return (WsDecision){ .kind = WS_PRESERVE };
			}
			return (WsDecision){ .kind = WS_NONE };
		}
		if (c == ':') {
			/* Space before ':' in ternary expressions; preserve newlines. */
			if (!strcmp(ctx->parent_type, "conditional_expression")) {
				if (gap_has_newline(ts, gap_start, gap_end)) {
					return (WsDecision){ .kind = WS_PRESERVE };
				}
				return (WsDecision){ .kind = WS_SPACE };
			}
			return (WsDecision){ .kind = WS_NONE };
		}
	}

	/* Space between pointer '*' and a type qualifier (e.g. 'char * const p').
	 * This must come before the unary-context check which would otherwise
	 * suppress the space when '*' follows a keyword. */
	if (left && token_is(left, "*") && right->kind == TOK_KEYWORD
			&& (token_is(right, "const") || token_is(right, "volatile")
				|| token_is(right, "restrict"))) {
		return (WsDecision){ .kind = WS_SPACE };
	}

	/* Pointer declarator spacing: type and '*' are separated by a space
	 * ('char *'), while '*' and the identifier have no space ('char *x').
	 * At file scope inside a function definition, emit a newline between
	 * '*' and the function name so the name starts on its own line.
	 * Skip this rule when '*' is a binary operator (e.g. 'a * b'). */
	if (left && token_is(left, "*") && right->kind == TOK_IDENTIFIER) {
		const FormatCtx *left_ctx = &ts->contexts[index - 1];
		if (strcmp(left_ctx->parent_type, "binary_expression") != 0
				&& strcmp(left_ctx->parent_type, "pointer_expression") != 0) {
			/* indent_depth == 0 means file scope.
			 * Preserve an intentional newline between '*' and the function
			 * name; do not force one when they are on the same line. */
			if (ctx && ctx->in_function_definition && !ctx->in_parameter_list
					&& ctx->indent_depth == 0) {
				if (gap_has_newline(ts, gap_start, gap_end)) {
					return (WsDecision){ .kind = WS_NEWLINE };
				}
			}
			return (WsDecision){ .kind = WS_NONE };
		}
	}

	/* No space after opening delimiters or unary operators. */
	if (left) {
		char lc = lex_first(ts, left);
		if (lc == '(' || lc == '[') {
			/* Preserve intentional line breaks after opening delimiters. */
			if (gap_has_newline(ts, gap_start, gap_end)) {
				return (WsDecision){ .kind = WS_PRESERVE };
			}
			return (WsDecision){ .kind = WS_NONE };
		}
		if (is_unary_context(ts, index - 1, left)) {
			/* Postfix ++/-- is unary before the operator, but the operand
			 * that follows is outside update_expression and needs normal
			 * spacing (e.g. '*dst++ = x'). */
			if ((token_is(left, "++") || token_is(left, "--"))
					&& ts->contexts && index < ts->count
					&& !strcmp(ts->contexts[index - 1].parent_type,
						"update_expression")
					&& strcmp(ts->contexts[index - 1].parent_type,
						ts->contexts[index].parent_type)) {
				/* fall through to binary operator spacing below */
			} else {
				/* Exception: '*' followed by a number is a binary multiply
				 * mis-classified as a dereference (e.g. 'sizeof(float) * 4'). */
				if (token_is(left, "*") && right && right->kind == TOK_NUMBER) {
					return (WsDecision){ .kind = WS_PRESERVE };
				}
				return (WsDecision){ .kind = WS_NONE };
			}
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
	 * Compound literals '(Type){ ... }' have no space before '{'.
	 * Macro calls used as loop constructs (e.g. wl_list_for_each) keep
	 * the original spacing when used inside function bodies. */
	if (left && token_is(left, ")") && token_is(right, "{")) {
		if (!strcmp(ctx->parent_type, "initializer_list")) {
			/* Compound literal: preserve the author's spacing choice. */
			return (WsDecision){ .kind = WS_PRESERVE };
		}
		const FormatCtx *left_ctx = &ts->contexts[index - 1];
		if (left_ctx->in_condition || left_ctx->in_for_header) {
			return (WsDecision){ .kind = WS_SPACE };
		}
		/* Inside a function body, preserve the original spacing so that
		 * macro calls like wl_list_for_each(...) { keep their '{' inline. */
		if (left_ctx->indent_depth > 0
				&& !gap_has_newline(ts, gap_start, gap_end)) {
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
			/* Comments and other tokens at the start of a switch body should
			 * be at the same indent level as case labels. */
			if (ctx->in_switch_body) {
				return with_indent(ctx, -1);
			}
			return newline_indent(ctx->indent_depth);
		}
	}

	/* Statement boundaries. */
	if (left && token_is(left, ";") && ctx->in_compound_statement) {
		const FormatCtx *left_ctx = &ts->contexts[index - 1];
		if (left_ctx->in_for_header) {
			if (gap_has_newline(ts, gap_start, gap_end)) {
				return (WsDecision){ .kind = WS_PRESERVE };
			}
			return (WsDecision){ .kind = WS_SPACE };
		}
		/* Preserve trailing comments on the same line as a statement. */
		if (!gap_has_newline(ts, gap_start, gap_end)) {
			return (WsDecision){ .kind = WS_PRESERVE };
		}
		if (right && (token_is(right, "case") || token_is(right, "default"))) {
			if (gap_has_blank_line(ts, gap_start, gap_end)) {
				return (WsDecision){
					.kind = WS_BLANK_LINE,
					.indent = ctx->indent_depth - 1,
				};
			}
			return with_indent(ctx, -1);
		}
		/* Goto labels are at column 0, preserving blank lines before them. */
		if (!strcmp(ctx->parent_type, "labeled_statement")) {
			if (gap_has_blank_line(ts, gap_start, gap_end)) {
				return (WsDecision){ .kind = WS_BLANK_LINE, .indent = 0 };
			}
			return newline_indent(0);
		}
		/* Comments and tokens between switch cases stay at case-label indent. */
		if (ctx->in_switch_body) {
			if (gap_has_blank_line(ts, gap_start, gap_end)) {
				return (WsDecision){
					.kind = WS_BLANK_LINE,
					.indent = ctx->indent_depth - 1,
				};
			}
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
			if (gap_has_newline(ts, gap_start, gap_end)) {
				return (WsDecision){ .kind = WS_PRESERVE };
			}
			return (WsDecision){ .kind = WS_SPACE };
		}
		/* Unary operator immediately after a closing paren takes no space.
		 * E.g. '(int *)&val', '(void *)!cond'.
		 * Exception: if the operand is a number literal, the parser has
		 * misclassified a binary multiply (e.g. 'sizeof(float) * 4')
		 * as a pointer dereference; preserve the original spacing. */
		if (left && lex_first(ts, left) == ')'
				&& (!strcmp(ctx->parent_type, "pointer_expression")
					|| !strcmp(ctx->parent_type, "unary_expression"))) {
			const Token *operand = (index + 1 < ts->count) ? &ts->tokens[index + 1] : NULL;
			if (operand && operand->kind == TOK_NUMBER) {
				return (WsDecision){ .kind = WS_PRESERVE };
			}
			return (WsDecision){ .kind = WS_NONE };
		}
		if (gap_has_newline(ts, gap_start, gap_end)) {
			return (WsDecision){ .kind = WS_PRESERVE };
		}
		return (WsDecision){ .kind = WS_SPACE };
	}

	/* Top-level and declaration boundaries.
	 * Skip inside struct/union bodies (field_declaration_list) so that
	 * member indentation is preserved. */
	if (ctx->indent_depth == 0 && !ctx->in_field_declaration_list && left) {
		if ((token_is(left, "}") || token_is(left, ";"))
				&& right->kind != TOK_PUNCT
				&& gap_has_newline(ts, gap_start, gap_end)) {
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
			&& (right->kind == TOK_KEYWORD || right->kind == TOK_IDENTIFIER)
			&& gap_has_newline(ts, gap_start, gap_end)) {
		return (WsDecision){ .kind = WS_BLANK_LINE };
	}

	/* Leading file newline. */
	if (!left && gap_start < gap_end && ts->source[gap_start] == '\n') {
		return (WsDecision){ .kind = WS_NEWLINE };
	}

	return (WsDecision){ .kind = WS_PRESERVE };
}
