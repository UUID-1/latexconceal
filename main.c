/*  unitex: TeX-to-Unicode converter.
 *  Copyright (C) 2022 Juiyung Hsu
 *
 *  Unitex is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Unitex is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this file. If not, see <http://www.gnu.org/licenses/>.
 */

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "strmap.h"
#include "util.h"
#include "vec.h"

#include "misc.h"
#include "rules.h"
#include "restore.h"

typedef struct {
	size_t span;
	const char *cchar;
} CChar;

static size_t
mark(Node *nd, const char *const *tks, CChar *cchars, size_t i)
{
	size_t j = i + 1, n = 0;
	for (;;) {
		if (nd->key) {
			n = j - i;
			cchars[i].cchar = nd->key;
		}
		if (!nd->br || !(nd = sm_get(nd->br, tks[j])))
			break;
		++j;
	}
	cchars[i].span = n;
	return n;
}

static CChar *
conceal(const Strmap *rtbr, const Strmap *subsbr, const Strmap *supsbr,
        const bool test_rtbr_initial[static 256], const char **tks, size_t ntks)
{
	CChar *cchars = xcalloc(ntks, sizeof(*cchars));
	size_t i, j, n;
	Node *nd;
	const Strmap *ssbr;
	char c;
	for (i = 0; i < ntks; ) {
		assert(i < ntks);
		c = *tks[i];
		if (test_rtbr_initial[(unsigned char)c]) {
			if ((nd = sm_get(rtbr, tks[i]))
			    && (n = mark(nd, tks, cchars, i))
			   ) {
				i += n;
				continue;
			}
			if ((c == tr('_') || c == tr('^')) && *tks[i + 1] == tr('{')) {
				ssbr = (c == tr('_')? subsbr: supsbr);
				i = j = i + 2;
				for (;;) {
					if ((nd = sm_get(ssbr, tks[i]))
					    && (n = mark(nd, tks, cchars, i))) {
						i += n;
						if (*tks[i] == tr('}')) {
							cchars[j - 2] = (CChar){ .span = 2, .cchar = "" };
							cchars[i++] = (CChar){ .span = 1, .cchar = "" };
							break;
						}
					} else {
						cchars[j - 2].span = 0;
						cchars[j - 1].span = 0;
						i = j;
						break;
					}
				}
				continue;
			}
		}

		cchars[i].span = 0;
		++i;
	}
	return cchars;
}

static int
puttks(const char *const *tks, const CChar *cchars)
{
	const char *s, *t;
	char c;
	size_t i;

	for (i = 0; ; ) {
		s = tks[i];
		if (s[-1] != NUL) {
			for (t = s - 1; t[-1] != NUL; --t);
			do {
				if (putchar(*t++) == EOF)
					return EOF;
			} while (t != s);
		}
		if (cchars && cchars[i].span) {
			s = cchars[i].cchar;
			while ((c = *s++) != NUL) {
				do {
					if (putchar(tr(c)) == EOF)
						return EOF;
				} while ((c = *s++) != NUL);
			}
			i += cchars[i].span;
		} else {
			c = *s++;
			if (c == tr('\n')) {
				if (putchar('\n') == EOF)
					return EOF;
				return 0;
			} else if (c == EOFBYTE) {
				return 0;
			} else if (c == ILSEQ) {
				while ((c = *s++) != NUL) {
					if (putchar(c) == EOF)
						return EOF;
				}
			} else {
				while (c != NUL) {
					if (putchar(tr(c)) == EOF)
						return EOF;
					c = *s++;
				}
			}
			++i;
		}
	}
}

int
main(int argc, char **argv)
{
	bool reverse = false;
	Strv *rulesfiles = sv_new(),
	     *files = sv_new();
	Strmap *rtbr, *invbr, *subsbr, *supsbr;
	static bool test_rtbr_initial[256];

	program_invocation_name = argv[0];

	clear_at_exit(rulesfiles, SV_DELETE);
	clear_at_exit(files, SV_DELETE);

	{
		static const char s1[] = "/.config";
		static const char s2[] = "/unitex/rules.tsv";
		const char *s;
		char *p;

		if ((s = getenv("UNITEX_RULES_FILE")) && *s != NUL) {
			if (!access(s, F_OK)) sv_push(rulesfiles, s);
		} else {
			p = NULL;
			if ((s = getenv("XDG_CONFIG_HOME")) && *s != NUL) {
				p = xcalloc(strlen(s) + sizeof(s2), 1);
				strcat(strcpy(p, s), s2);
			} else if ((s = getenv("HOME")) && *s != NUL) {
				p = xcalloc(strlen(s) + sizeof(s1) + sizeof(s2) - 1, 1);
				strcat(strcat(strcpy(p, s), s1), s2);
			}
			if (p) {
				if (!access(p, F_OK)) {
					sv_push(rulesfiles, p);
					clear_at_exit(p, FREE);
				} else {
					free(p);
				}
			}
		}
	}

	{
		int opt;
		FILE *hf;

		while ((opt = getopt(argc, argv, "ru:f:vh")) != -1) {
			switch (opt) {
			case 'r':
				reverse = true;
				break;
			case 'u':
				sv_resize(rulesfiles, 0);
				/* FALLTHROUGH */
			case 'f':
				sv_push(rulesfiles, optarg);
				break;
			case 'v':
				puts("1");
				return 0;
			case 'h':
			default:
				hf = (opt == 'h'? stdout: stderr);
				fprintf(hf, "usage: %s [-r|-h|-v] [-u rules_file]... [-f rules_file]... [input_files...]\n", argv[0]);
				fputs("options:\n"
				      "  -r         convert in reverse\n"
				      "  -u <file>  specify the rules file to use\n"
				      "  -f <file>  specify an additional rules file\n"
				      "  -h         print this help and exit\n"
				      "  -v         print version number and exit\n", hf);
				return opt != 'h';
			}
		}
	}

	if (!sv_size(rulesfiles))
		error(EXIT_FAILURE, 0, "couldn't find any rules file");

	while (optind < argc)
		sv_push(files, argv[optind++]);

	if (reverse) {
		parserules(rulesfiles, &invbr, NULL, NULL, NULL, NULL);
	} else {
		parserules(rulesfiles, &invbr, &rtbr, &subsbr, &supsbr, test_rtbr_initial);
		clear_at_exit(rtbr, BR_DELETE);
		clear_at_exit(subsbr, BR_DELETE);
		clear_at_exit(supsbr, BR_DELETE);
	}

	clear_at_exit(invbr, BR_DELETE);

	setvbuf(stdout, NULL, _IOLBF, 0);

	if (!sv_size(files)) sv_push(files, "-");

	{
		FILE *f;
		const char *fname;
		size_t i;
		Charv *cv = cv_new();
		Idxv *iv = iv_new();
		Idxv *ib = iv_new();

		clear_at_exit(cv, CV_DELETE);
		clear_at_exit(iv, IV_DELETE);
		clear_at_exit(ib, IV_DELETE);

		for (i = 0; i < sv_size(files); ++i) {
			fname = sv_get(files, i);

			if (!strcmp(fname, "-")) {
				f = stdin;
				fname = "standard input";
			} else if (!(f = fopen(fname, "r"))) {
				error(EXIT_FAILURE, errno, "couldn't open %s", fname);
			}

			do {
				int leadingchar;
				const char **tks;
				size_t ntks, j;
				CChar *cchars;

				leadingchar = getc(f);
				if (leadingchar != '\x03')
					ungetc(leadingchar, f);

				getrestoredline(invbr, cv, iv, ib, f);
				if (ferror(f))
					error(EXIT_FAILURE, 0, "input error during reading %s", fname);

				assert(cv_get(cv, iv_top(iv)) == tr('\n') || cv_get(cv, iv_top(iv)) == EOFBYTE);

				ntks = iv_size(iv);
				tks = xcalloc(ntks, sizeof(*tks));
				for (j = ntks; j--; )
					tks[j] = cv_getptr(cv, iv_get(iv, j));

				if (!reverse && leadingchar != '\x03')
					cchars = conceal(rtbr, subsbr, supsbr, test_rtbr_initial, (const char**)tks, ntks);
				else
					cchars = NULL;
					
				if (puttks(tks, cchars) == EOF)
					error(EXIT_FAILURE, 0, "output error");

				free(cchars);
				free(tks);

				cv_resize(cv, 0);
				iv_resize(iv, 0);
			} while (!feof(f));

			if (f == stdin)
				clearerr(f);
			else if (fclose(f) == EOF)
				error(EXIT_FAILURE, errno, "couldn't close %s", fname);
		}
	}

	return 0;
}
