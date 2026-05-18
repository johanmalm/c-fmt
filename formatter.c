#include "formatter.h"

void
format_file(const char *source, uint32_t source_len, TSNode root, FILE *out)
{
	TokenStream ts;

	token_stream_build(&ts, root, source, source_len);
	context_build(&ts);
	format_render(&ts, out);
	token_stream_free(&ts);
}
