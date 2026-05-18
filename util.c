#include "util.h"
#include <stdio.h>

void print_indent(FILE *out, int indent)
{
	for (int i = 0; i < indent; i++) {
		fputc('\t', out);
	}
}
