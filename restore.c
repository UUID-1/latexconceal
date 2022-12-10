/*  unitex: TeX-to-Unicode converter.
 *  Copyright (C) 2022 Juiyung Hsu
 *  License: GNU General Public License (version 3 or later)
 *  You should have received a copy of the license along with this
 *  file. If not, see <http://www.gnu.org/licenses>.
 */

#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>

#include "strmap.h"
#include "vec.h"

#include "misc.h"
#include "restore.h"

static size_t
gettk(Charv *cv, Idxv *iv, Idxv *ib, FILE *f)
{
	size_t i = iv_size(ib)? iv_pop(ib): readtk(cv, f);
	iv_push(iv, i);
	return i;
}

static size_t
peektk(Charv *cv, Idxv *ib, FILE *f)
{
	if (iv_size(ib)) {
		return iv_top(ib);
	} else {
		size_t i = readtk(cv, f);
		iv_push(ib, i);
		return i;
	}
}

static size_t
getrestoredtk(const Strmap *invbr, Charv *cv, Idxv *iv, Idxv *ib, FILE *f, bool *p_did_restore)
{
	size_t iifirst, iilast;
	Node *nd;
	size_t i, j, ii;
	const char *p;
	size_t ret = gettk(cv, iv, ib, f);
	char c;

	c = cv_get(cv, ret);
	if (((unsigned char)tr(c) < 0x80
	     && (c == tr('\n')
	         || (unsigned char)tr(cv_get(cv, peektk(cv, ib, f))) < 0x80
	        )
	    ) || !(nd = sm_get(invbr, cv_getptr(cv, ret)))
	   ) {
		*p_did_restore = false;
		return ret;
	}

	iilast = iv_size(iv);
	iifirst = iilast - 1;

	if (nd->br) {
		Node *nd2 = nd;
		do {
			i = gettk(cv, iv, ib, f);
			nd2 = sm_get(nd2->br, cv_getptr(cv, i));
			if (!nd2) break;
			if (nd2->key) {
				nd = nd2;
				iilast = iv_size(iv);
			}
		} while (nd2->br);

		while (iv_size(iv) > iilast)
			iv_push(ib, iv_pop(iv));

		if (!nd->key) {
			*p_did_restore = false;
			return ret;
		}
	}

	ii = iifirst;
	do {
		i = iv_get(iv, ii) - 1;
		if (cv_get(cv, i) != NUL) {
			for (j = i; cv_get(cv, j - 1) != NUL; --j);
			do {
				cv_push(cv, cv_get(cv, j));
			} while (j++ != i);
		}
	} while (++ii < iilast);
	iv_erasen(iv, iifirst, iilast - iifirst);

	p = nd->key;
	while (isblank(*p)) cv_push(cv, *p++);
	iv_push(iv, ret = cv_size(cv));
	for (;;) {
		do cv_push(cv, *p); while (*p++ != NUL);
		if (*p == NUL) break;
		while (isblank(*p)) cv_push(cv, *p++);
		iv_push(iv, cv_size(cv));
	}

	*p_did_restore = true;
	return ret;
}

static bool
getrestgrp(Charv *cv, Idxv *iv, Idxv *ib, FILE *f)
{
	size_t depth = 1;
	size_t tk_i;
	char c;

	assert(cv_get(cv, iv_top(iv)) == tr('{'));

	for (;;) {
		tk_i = gettk(cv, iv, ib, f);
		c = cv_get(cv, tk_i);
		if (c == tr('{')) {
			++depth;
		} else if (c == tr('}')) {
			if (!--depth) return true;
		} else if (c == tr('\n') || c == EOFBYTE) {
			return false;
		}
	}
}

static bool
getrestss(Charv *cv, Idxv *iv, Idxv *ib, FILE *f)
{
	assert(cv_get(cv, iv_top(iv)) == tr('_') || cv_get(cv, iv_top(iv)) == tr('^'));

	size_t tk_i = gettk(cv, iv, ib, f);
	char c = cv_get(cv, tk_i);

	if (c == tr('{')) {
		return getrestgrp(cv, iv, ib, f);
	} else if (c == tr('\\')) {
		for (;;) {
			tk_i = peektk(cv, ib, f);
			if (cv_get(cv, tk_i) != tr('{') || cv_get(cv, tk_i - 1) != NUL)
				break;
			iv_push(iv, iv_pop(ib));
			if (!getrestgrp(cv, iv, ib, f))
				return false;
		}
	} else if (c == tr('\n') || c == EOFBYTE) {
		return false;
	}

	return true;
}

void
getrestoredline(const Strmap *invbr, Charv *cv, Idxv *iv, Idxv *ib, FILE *f)
{
	bool did_restore;
	size_t tk_i, tk_ii;
	size_t ssleader_i, ssleader_ii;
	bool ssended;
	size_t ii, jj, kk;
	size_t i, j;
	char c;

	assert(cv_size(cv) == 0);
	assert(iv_size(iv) == 0);
	cv_push(cv, NUL);

	for (;;) {
		tk_ii = iv_size(iv);
		tk_i = getrestoredtk(invbr, cv, iv, ib, f, &did_restore);
		c = cv_get(cv, tk_i);

		assert(tk_i == iv_get(iv, tk_ii));

		if (!did_restore) {
			if (c == tr('\n') || c == EOFBYTE)
				return;
		} else if (cv_get(cv, i = iv_top(iv)) == tr('\\')
		           && isalpha(cv_get(cv, i + 1))
		           && isalpha(cv_get(cv, i = peektk(cv, ib, f)))
		           && cv_get(cv, i - 1) == NUL) {
			cv_push(cv, ' ');
			iv_set(ib, iv_size(ib) - 1, cv_size(cv));
			do {
				cv_push(cv, c = cv_get(cv, i++));
			} while (c != NUL);
		}

		if (c == tr('_') || c == tr('^')) {
			ssleader_ii = tk_ii;
			ssleader_i = tk_i;
			ssended = false;
		} else {
			continue;
		}

		for (;;) {
			if (!ssended && !did_restore) {
				getrestoredtk(invbr, cv, iv, ib, f, &did_restore);
				if (!did_restore) {
					while (iv_size(iv) > tk_ii + 1)
						iv_push(ib, iv_pop(iv));
					if (!getrestss(cv, iv, ib, f))
						ssended = true;
				}
			}

			if (ssended) {
				if (tk_ii != ssleader_ii
				    && cv_get(cv, iv_get(iv, tk_ii - 1)) != tr('}')
				    && cv_get(cv, iv_get(iv, ssleader_ii + 1)) == tr('{')) {
					while (iv_size(iv) > tk_ii)
						iv_push(ib, iv_pop(iv));
					iv_push(ib, cv_size(cv));
					cv_push(cv, tr('}'));
					cv_push(cv, NUL);
				}
				while (iv_size(iv) > ssleader_ii + 1)
					iv_push(ib, iv_pop(iv));
				break;
			}

			assert(tk_i == iv_get(iv, tk_ii));
			assert(cv_get(cv, tk_i) == cv_get(cv, ssleader_i));

			if (tk_ii != ssleader_ii) {
				ii = tk_ii - 1;
				if (cv_get(cv, iv_get(iv, ii)) != tr('}')) ++ii;
				jj = tk_ii + 1;
				if (cv_get(cv, iv_get(iv, jj)) == tr('{')) ++jj;

				assert(ii > ssleader_ii);
				assert(jj < iv_size(iv));

				/* Note: Beware that pointers if used may be
				 * invalidated by vector operations. */
				kk = ii;
				do {
					i = iv_get(iv, kk) - 1;
					if (cv_get(cv, i) != NUL) {
						for (j = i; cv_get(cv, j - 1) != NUL; --j);
						do {
							cv_push(cv, cv_get(cv, j));
						} while (j++ != i);
					}
				} while (++kk <= jj);

				if (cv_top(cv) == NUL
				    && isalpha(tr(cv_get(cv, iv_get(iv, ii))))
				    && cv_get(cv, i = iv_get(iv, ii - 1)) == tr('\\')
				    && isalpha(tr(cv_get(cv, i + 1))))
					cv_push(cv, ' ');

				if (cv_top(cv) != NUL) {
					i = iv_get(iv, jj);
					iv_set(iv, jj, cv_size(cv));
					do {
						cv_push(cv, c = cv_get(cv, i++));
					} while (c != NUL);
				}

				/* Note: Be aware of invalidation of indicies. */
				iv_erasen(iv, ii, jj - ii);

				if (cv_get(cv, iv_get(iv, ssleader_ii + 1)) != tr('{')) {
					iv_insert(iv, ssleader_ii + 1, cv_size(cv));
					cv_push(cv, tr('{'));
					cv_push(cv, NUL);
				}
			}

			tk_ii = iv_size(iv);
			tk_i = getrestoredtk(invbr, cv, iv, ib, f, &did_restore);

			if (cv_get(cv, tk_i) != cv_get(cv, ssleader_i))
				ssended = true;
		}
	}
}
