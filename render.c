#include "formatter.h"

#include <stdio.h>
#include <string.h>

static const struct format_ctx *
ctx_before_token(const struct token_stream *ts, size_t index)
{
	if (!ts->contexts || ts->count == 0) {
		return NULL;
	}
	if (index < ts->count) {
		return &ts->contexts[index];
	}
	return &ts->contexts[ts->count - 1];
}

static void
emit_preserve_gap(const struct token_stream *ts, uint32_t start, uint32_t end, FILE *out)
{
	const char *s = ts->source + start;
	uint32_t len = end - start;
	uint32_t i = 0;
	bool at_line_start = true;

	while (i < len) {
		if (at_line_start) {
			uint32_t spaces = 0;

			while (i < len && s[i] == ' ') {
				spaces++;
				i++;
			}
			for (uint32_t t = 0; t < spaces / 4; t++) {
				fputc('\t', out);
			}
			for (uint32_t r = 0; r < spaces % 4; r++) {
				fputc(' ', out);
			}
			at_line_start = false;
			if (i < len && s[i] == '\n') {
				fputc(s[i++], out);
				at_line_start = true;
				continue;
			}
			if (i >= len) {
				break;
			}
		}

		char c = s[i++];

		fputc(c, out);
		if (c == '\n') {
			at_line_start = true;
		}
	}
}

static void
print_indent(FILE *out, int indent)
{
	for (int i = 0; i < indent; i++) {
		fputc('\t', out);
	}
}

static void
emit_gap(const struct token_stream *ts, uint32_t start, uint32_t end, struct whitespace_decision ws,
		const struct format_ctx *ctx, FILE *out)
{
	if (start >= end && ws.kind == WS_NONE) {
		return;
	}

	switch (ws.kind) {
	case WS_NONE:
		break;
	case WS_SPACE:
		fputc(' ', out);
		break;
	case WS_NEWLINE:
		fputc('\n', out);
		break;
	case WS_BLANK_LINE:
		fputs("\n\n", out);
		if (ws.indent > 0) {
			print_indent(out, ws.indent);
		}
		break;
	case WS_NEWLINE_INDENT:
		fputc('\n', out);
		print_indent(out, ws.indent == WS_INDENT_USE_CTX ? ctx->indent_depth : ws.indent);
		break;
	case WS_PRESERVE:
		emit_preserve_gap(ts, start, end, out);
		break;
	}
}

static void
emit_token(const struct token_stream *ts, const struct token *tok, FILE *out)
{
	fwrite(ts->source + tok->start, 1, tok->end - tok->start, out);
}

void
format_render(const struct token_stream *ts, FILE *out)
{
	uint32_t pos = 0;

	for (size_t i = 0; i < ts->count; i++) {
		const struct token *tok = &ts->tokens[i];
		const struct format_ctx *ctx = ctx_before_token(ts, i);
		struct whitespace_decision ws = rules_gap_decision(ts, i, pos, tok->start, ctx);

		emit_gap(ts, pos, tok->start, ws, ctx, out);
		emit_token(ts, tok, out);
		pos = tok->end;
	}

	const struct format_ctx *tail_ctx = ctx_before_token(ts, ts->count);
	struct whitespace_decision tail = rules_gap_decision(ts, ts->count, pos, ts->root_end, tail_ctx);
	emit_gap(ts, pos, ts->root_end, tail, tail_ctx, out);

	if (ts->root_end < ts->source_len) {
		struct whitespace_decision extra = rules_gap_decision(ts, ts->count, ts->root_end,
				ts->source_len, tail_ctx);
		emit_gap(ts, ts->root_end, ts->source_len, extra, tail_ctx, out);
	}
}
