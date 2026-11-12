/* lilac compat for Mesa's arm64 build. */
#include <string.h>
#include <stddef.h>

void
util_streaming_load_memcpy(void *restrict dst, void *restrict src, size_t len)
{
	memcpy(dst, src, len);
}
