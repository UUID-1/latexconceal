/*  unitex: TeX-to-Unicode converter.
 *  Copyright (C) 2022 Juiyung Hsu
 *  License: GNU General Public License (version 3 or later)
 *  You should have received a copy of the license along with this
 *  file. If not, see <http://www.gnu.org/licenses>.
 */

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

char *program_invocation_name;

static void
v_error_at_line(int status, int errnum, const char *filename,
                unsigned int linenum, const char *format, va_list ap)
{
	fflush(stdout);

	if (program_invocation_name)
		fprintf(stderr, "%s:", program_invocation_name);

	if (filename)
		fprintf(stderr, "%s:%u:", filename, linenum);

	putc(' ', stderr);

	vfprintf(stderr, format, ap);

	if (errnum)
		fprintf(stderr, ": %s", strerror(errnum));

	putc('\n', stderr);

	if (status)
		exit(status);
}

void
error(int status, int errnum, const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	v_error_at_line(status, errnum, NULL, 0, format, ap);
	va_end(ap);
}

void
error_at_line(int status, int errnum, const char *filename,
              unsigned int linenum, const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	v_error_at_line(status, errnum, filename, linenum, format, ap);
	va_end(ap);
}

void *
xmalloc(size_t size)
{
	void *p;

	if (!(p = malloc(size)))
		error(EXIT_FAILURE, errno, "malloc");

	return p;
}

void *
xcalloc(size_t nmemb, size_t size)
{
	size_t blksiz = nmemb * size;

	if (blksiz / size != nmemb)
		error(EXIT_FAILURE, ENOMEM, "xcalloc");

	return xmalloc(blksiz);
}

void *
xrealloc(void *p, size_t size)
{
	if (!(p = realloc(p, size)))
		error(EXIT_FAILURE, errno, "realloc");

	return p;
}

void *
xreallocarray(void *p, size_t nmemb, size_t size)
{
	size_t blksiz = nmemb * size;

	if (blksiz / size != nmemb)
		error(EXIT_FAILURE, ENOMEM, "xreallocarray: %s");

	return xrealloc(p, blksiz);
}
