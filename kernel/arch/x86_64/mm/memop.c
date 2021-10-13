#include <common/types.h>

void memcpy(void *dst, const void *src, size_t size)
{
	char *d;
	char *s;
	u64 i;

	d = (char *)dst;
	s = (char *)src;
	for (i = 0; i < size; ++i)
		d[i] = s[i];
}

void memset(void *dst, const char ch, size_t size)
{
	char *d;
	u64 i;

	d = (char *)dst;
	for (i = 0; i < size; ++i)
		d[i] = ch;
}

void memmove(void *dst, const void *src, size_t size)
{
	char *d;
	char *s;
	s64 i;

	d = (char *)dst;
	s = (char *)src;
	for (i = size; i >= 0; --i)
		d[i] = s[i];
}

void bzero(void *p, size_t size)
{
	char *d;
	u64 i;

	d = (char *)p;
	for (i = 0; i < size; ++i)
		d[i] = 0;
}

