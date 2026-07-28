#include "formatter.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

static bool
is_preproc_ancestor(const char *type)
{
	return strncmp(type, "preproc_", 8) == 0;
}

static bool
is_condition_parent(const char *type)
{
	return !strcmp(type, "if_statement") || !strcmp(type, "while_statement")
		|| !strcmp(type, "for_statement")
		|| !strcmp(type, "switch_statement")
		|| !strcmp(type, "do_statement");
}

static bool
is_case_body_compound(TSNode node)
{
	if (strcmp(ts_node_type(node), "compound_statement") != 0) {
		return false;
	}
	TSNode parent = ts_node_parent(node);
	return !ts_node_is_null(parent)
		&& !strcmp(ts_node_type(parent), "case_statement");
}

static struct format_ctx
context_at(TSNode node)
{
	struct format_ctx ctx = {
		.indent_depth = 0,
		.in_argument_list = false,
		.in_parameter_list = false,
		.in_preproc = false,
		.in_condition = false,
		.in_compound_statement = false,
		.in_function_definition = false,
		.in_string_literal = false,
		.in_for_header = false,
		.in_field_declaration_list = false,
		.in_switch_body = false,
		.parent_type = "",
	};

	TSNode parent = ts_node_parent(node);
	if (!ts_node_is_null(parent)) {
		ctx.parent_type = ts_node_type(parent);

		/* Token is a direct child of a switch compound_statement. */
		if (!strcmp(ctx.parent_type, "compound_statement")) {
			TSNode grandparent = ts_node_parent(parent);
			if (!ts_node_is_null(grandparent)
					&& !strcmp(ts_node_type(grandparent), "switch_statement")) {
				ctx.in_switch_body = true;
			}
		}
	}

	for (TSNode anc = node; !ts_node_is_null(anc); anc = ts_node_parent(anc)) {
		const char *type = ts_node_type(anc);

		if (!strcmp(type, "translation_unit")) {
			break;
		}

		if (!strcmp(type, "compound_statement")) {
			if (is_case_body_compound(anc)) {
				/* case FOO: { ... } scopes variables but does not add
				 * an extra indent level in the author's style. */
				ctx.in_compound_statement = true;
				continue;
			}
			ctx.indent_depth++;
			ctx.in_compound_statement = true;
		} else if (!strcmp(type, "field_declaration_list")) {
			ctx.in_field_declaration_list = true;
		} else if (!strcmp(type, "argument_list")) {
			ctx.in_argument_list = true;
		} else if (!strcmp(type, "parameter_list")) {
			ctx.in_parameter_list = true;
		} else if (!strcmp(type, "function_definition")) {
			ctx.in_function_definition = true;
		} else if (is_preproc_ancestor(type)) {
			ctx.in_preproc = true;
		} else if (!strcmp(type, "string_literal")) {
			ctx.in_string_literal = true;
		} else if (!strcmp(type, "parenthesized_expression")) {
			TSNode cond_parent = ts_node_parent(anc);
			if (!ts_node_is_null(cond_parent)
				&& is_condition_parent(ts_node_type(cond_parent))) {
				ctx.in_condition = true;
			}
		} else if (!strcmp(type, "for_statement")) {
			/* Mark tokens in for-loop init/condition/update (not body). */
			if (!ctx.in_compound_statement) {
				ctx.in_for_header = true;
			}
		}
	}

	return ctx;
}

void
context_build(struct token_stream *ts)
{
	ts->contexts = calloc(ts->count, sizeof(struct format_ctx));
	if (!ts->contexts) {
		return;
	}

	for (size_t i = 0; i < ts->count; i++) {
		uint32_t start = ts->tokens[i].start;
		uint32_t end = ts->tokens[i].end;
		if (end <= start) {
			end = start + 1;
		}

		TSNode node = ts_node_descendant_for_byte_range(ts->root, start, end);
		ts->contexts[i] = context_at(node);
	}
}
