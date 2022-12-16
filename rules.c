/*  unitex: TeX-to-Unicode converter.
 *  Copyright (C) 2022 Juiyung Hsu
 *  License: GNU General Public License v3.0
 *  You should have received a copy of the license along with this
 *  file. If not, see <http://www.gnu.org/licenses>.
 */

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "strmap.h"
#include "util.h"
#include "vec.h"

#include "misc.h"
#include "rules.h"

static size_t
gettk(Charv *cv, Idxv *iv, FILE *f)
{
	size_t ret = readtk(cv, f);
	iv_push(iv, ret);
	return ret;
}

static const char **
getrules(const Strv *files)
{
	Charv *cv = cv_new();
	Idxv *iv = iv_new();
	char *data;
	char **ret;
	size_t i;

	cv_push(cv, NUL);

	for (i = 0; i < sv_size(files); ++i) {
		const char *fname = sv_get(files, i);
		FILE *f = fopen(fname, "r");
		unsigned int lnum;
		int c;
		const unsigned char bom[] = {0xef, 0xbb, 0xbf};
		int j;

		if (!f) error(EXIT_FAILURE, errno, "couldn't open %s", fname);

		for (j = 0; j < sizeof(bom); ++j) {
			if ((c = getc(f)) != bom[j]) {
				ungetc(c, f);
				while (j--) {
					if (ungetc(bom[j], f) == EOF)
						error(EXIT_FAILURE, 0, "ungetc failed");
				}
				break;
			}
		}

#define BADRULE(...) error_at_line(EXIT_FAILURE, 0, fname, lnum, __VA_ARGS__)

		for (lnum = 1; !feof(f); ++lnum) {
			size_t tk_i;
			size_t group_level;
			size_t ascii, nonascii;

			c = getc(f);
			if (c == '#') {
				for (c = getc(f); c != '\n' && c != EOF; c = getc(f));
				continue;
			} else if (c == '\n' || c == EOF) {
				continue;
			}
			while (c == ' ') c = getc(f);
			if (c == '\t')
				BADRULE("missing substituend in the first field");
			ungetc(c, f);

			group_level = 0;
			for (;;) {
				tk_i = gettk(cv, iv, f);
				c = cv_get(cv, tk_i);
				if (c == ILSEQ)
					BADRULE("detected invalid UTF-8 encoding");
				if ((c == EOFBYTE && !ferror(f)) || c == tr('\n'))
					BADRULE("missing second field");
				if (c == tr('{')) {
					++group_level;
				} else if (c == tr('}')) {
					if (!group_level--) break;
				}
				c = getc(f);
				if (c == '\t') break;
				if (c == ' ') {
					do {
						cv_push(cv, tr(c));
						c = getc(f);
					} while (c == ' ');
					if (c == '\t') {
						do {
							cv_pop(cv);
						} while (cv_top(cv) != NUL);
						break;
					}
				}
				ungetc(c, f);
			}
			if (group_level)
				BADRULE("unbalanced curly brace in the substituend");
			cv_push(cv, NUL);
			iv_push(iv, -1);

			ascii = nonascii = 0;
			for (c = getc(f); c != '\n' && c != EOF; c = getc(f)) {
				if (c == '\t') {
					for (c = getc(f); c != '\n' && c != EOF; c = getc(f));
					break;
				}
				iv_push(iv, cv_size(cv));
				cv_push(cv, tr(c));
				if ((unsigned char)c < 0x80) {
					if (ascii > 0)
						BADRULE("expect only one character in the second field");
					++ascii;
				} else {
					if (!readutf8tail(cv, c, f))
						BADRULE("detected invalid UTF-8 encoding");
					++nonascii;
				}
				cv_push(cv, NUL);
			}
			if (!nonascii) {
				if (ascii)
					BADRULE("expect a non-ASCII character in the second field");
				else
					BADRULE("missing character in the second field");
			}
			cv_push(cv, NUL);
			iv_push(iv, -1);

			if (ferror(f))
				error(EXIT_FAILURE, 0, "input error during reading %s", fname);
		}
#undef BADRULE

		fclose(f);
	}

	data = cv_to_block(cv);
	clear_at_exit(data, FREE);

	ret = xcalloc(iv_size(iv) + 1, sizeof(char *));
	clear_at_exit(ret, FREE);

	i = iv_size(iv);
	ret[i] = NULL;
	while (i--) {
		size_t j = iv_get(iv, i);
		ret[i] = (j == -1? NULL: data + j);
	}

	iv_delete(iv);

	return (const char **)ret;
}

void
parserules(const Strv *files, Strmap **pinvbr, Strmap **prtbr,
           Strmap **psubsbr, Strmap **psupsbr, bool *test_rtbr_initial)
{
	Strmap *invbr, *rtbr, *subsbr, *supsbr;
	const char **tks = getrules(files);
	size_t i, j, k;
	Node *newnd, *curnd;
	Strmap *ssbr;
	char c;

	assert(pinvbr && (prtbr || (!psubsbr && !psupsbr)));

	invbr = *pinvbr = sm_new();
	rtbr = (prtbr? (*prtbr = sm_new()): NULL);
	subsbr = (psubsbr? (*psubsbr = sm_new()): NULL);
	supsbr = (psupsbr? (*psupsbr = sm_new()): NULL);

	newnd = nd_new();

	j = 0;
	while (tks[j]) {
		i = j;
		if (rtbr) {
			test_rtbr_initial[(unsigned char)*tks[j]] = true;

			curnd = sm_insert(rtbr, tks[j++], newnd);
			for (;;) {
				if (curnd == newnd)
					newnd = nd_new();
				if (!tks[j]) {
					curnd->key = tks[++j];
					assert(tks[j]);
					break;
				}
				if (!curnd->br)
					curnd->br = sm_new();
				curnd = sm_insert(curnd->br, tks[j++], newnd);
			}

			c = *tks[i];
			if ((c == tr('_') || c == tr('^'))
			     && (ssbr = (c == tr('_')? subsbr: supsbr))
			   ) {
				k = i + 1;
				if (*tks[k] == tr('{')) ++k;
				curnd = sm_insert(ssbr, tks[k++], newnd);
				for (;;) {
					if (curnd == newnd)
						newnd = nd_new();
					if (!tks[k]) {
						curnd->key = tks[k + 1];
						break;
					}
					if (*tks[k] == tr('}') && !tks[k + 1]) {
						curnd->key = tks[k + 2];
						break;
					}
					if (!curnd->br)
						curnd->br = sm_new();
					curnd = sm_insert(curnd->br, tks[k++], newnd);
				}
			}
		} else {
			while (tks[++j]);
			++j;
		}

		curnd = sm_insert(invbr, tks[j++], newnd);
		for (;;) {
			if (curnd == newnd)
				newnd = nd_new();
			if (!tks[j]) {
				curnd->key = tks[i];
				++j;
				break;
			}
			if (!curnd->br)
				curnd->br = sm_new();
			curnd = sm_insert(curnd->br, tks[j++], newnd);
		}
	}

	nd_delete(newnd);
}
