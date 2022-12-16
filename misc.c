/*  unitex: TeX-to-Unicode converter.
 *  Copyright (C) 2022 Juiyung Hsu
 *  License: GNU General Public License v3.0
 *  You should have received a copy of the license along with this
 *  file. If not, see <http://www.gnu.org/licenses>.
 */

#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "strmap.h"
#include "util.h"
#include "vec.h"

#include "misc.h"

bool
readutf8tail(Charv *cv, unsigned char c, FILE *f)
{
	int n;

	assert(c >= 0x80);
	if (c < 0xc0) {
		return false;
	} else if (c < 0xe0) {
		n = 1;
	} else if (c < 0xf0) {
		n = 2;
	} else if (c < 0xf8) {
		n = 3;
	} else {
		return false;
	}

	do {
		c = getc(f);
		if (c == EOF || (c & 0xc0) != 0x80) {
			ungetc(c, f);
			return false;
		}
		cv_push(cv, tr(c));
	} while (--n);

	return true;
}

size_t
readtk(Charv *cv, FILE *f)
{
	size_t ret;
	int c;

	while (isblank(c = getc(f)))
		cv_push(cv, tr(c));

	ret = cv_size(cv);

	if (c == EOF) {
		cv_push(cv, EOFBYTE);
	} else {
		if (c == '\\') {
			cv_push(cv, tr(c));
			c = getc(f);
			if (isalpha(c)) {
				do {
					cv_push(cv, tr(c));
				} while (isalpha(c = getc(f)));
				ungetc(c, f);
			} else if (c == '\n') {
				ungetc(c, f);
			} else {
				cv_push(cv, tr(c));
				if (c >= 0x80 && !readutf8tail(cv, c, f))
					cv_insert(cv, ret, ILSEQ);
			}
		} else {
			cv_push(cv, tr(c));
			if (c >= 0x80 && !readutf8tail(cv, c, f)) {
				cv_set(cv, ret, c);
				cv_insert(cv, ret, ILSEQ);
			}
		}
	}
	cv_push(cv, NUL);

	return ret;
}

Node *
nd_new()
{
	Node *ret = xmalloc(sizeof(*ret));
	*ret = (Node){ .br = NULL, .key = NULL };
	return ret;
}

void
nd_delete(Node *nd)
{
	free(nd);
}

static void
delete_br_pair(const char *key, void *value)
{
#define node ((Node *)value)
	if (node->br)
		br_delete(node->br);
	nd_delete(node);
#undef node
}

void
br_delete(Strmap *br)
{
	sm_foreach(br, delete_br_pair);
	sm_delete(br);
}

#ifndef NDEBUG

#define list clear_at_exit_list
#define len clear_at_exit_list_len
#define MAXLEN 100

static struct { void *p; CLEAR_METHOD m; } list[MAXLEN];
size_t len;

static void
do_clear_at_exit()
{
	while (len--) {
		void *p = list[len].p;
		switch (list[len].m) {
		case FREE: free(p); break;
		case CV_DELETE: cv_delete(p); break;
		case IV_DELETE: iv_delete(p); break;
		case SV_DELETE: sv_delete(p); break;
		case BR_DELETE: br_delete(p); break;
		default: assert(0);
		}
	}
}

void
clear_at_exit(void *p, CLEAR_METHOD m)
{
	assert(len < MAXLEN);
	if (!len && atexit(do_clear_at_exit))
		error_at_line(EXIT_FAILURE, 0, __FILE__, __LINE__, "atexit failed");
	list[len].p = p;
	list[len].m = m;
	++len;
}

#undef MAXLEN
#undef len
#undef list

#endif
