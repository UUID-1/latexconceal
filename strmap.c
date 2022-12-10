/*  unitex: TeX-to-Unicode converter.
 *  Copyright (C) 2022 Juiyung Hsu
 *  License: GNU General Public License (version 3 or later)
 *  You should have received a copy of the license along with this
 *  file. If not, see <http://www.gnu.org/licenses>.
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "strmap.h"
#include "util.h"

#define NEW_TABLE_LEN 2

typedef struct StrmapCell {
	const char *key;
	void *value;
	struct StrmapCell *next;
} Cell;

static size_t
maxpayload(size_t len)
{
	return len;
}

static size_t
expansion_len(size_t len)
{
	return len + len / 2 + 1;
}

static void
init_cellarr(Cell *cellarr, size_t len)
{
	Cell *pend = cellarr + len;
	Cell *pcell;
	for (pcell = cellarr; pcell != pend; ++pcell)
		*pcell = (Cell){ .key = NULL, .value = NULL, .next = NULL };
}

static unsigned
hash(const char *s)
{
	unsigned ret = 0;
	while (*s != '\0')
		ret = 257 * ret + *s++;
	return ret;
}

typedef enum {
	EMPTYHEAD,
	BEFOREHEAD,
	HEAD,
	BEFORENEXTCELL,
	NEXTCELL,
	ENDOFCHAIN
} FindCellPos;

static Cell *
findcell(Cell *cellarr, size_t len, const char *key, FindCellPos *pos)
{
	Cell *cell = cellarr + hash(key) % len;
	if (!cell->key) {
		*pos = EMPTYHEAD;
	} else {
		int cmp = strcmp(key, cell->key);
		if (cmp < 0) {
			*pos = BEFOREHEAD;
		} else if (cmp == 0) {
			*pos = HEAD;
		} else {
			Cell *nextcell;
			for (nextcell = cell->next; nextcell; nextcell = cell->next) {
				cmp = strcmp((key), nextcell->key);
				if (cmp < 0) {
					*pos = BEFORENEXTCELL;
					return cell;
				} else if (cmp == 0) {
					*pos = NEXTCELL;
					return cell;
				}
				cell = nextcell;
			}
			*pos = ENDOFCHAIN;
		}
	}
	return cell;
}

static void *
cellarr_insert(Cell *cellarr, size_t len, const char *key, void *value)
{
	FindCellPos pos;
	Cell *cell = findcell(cellarr, len, key, &pos);
	Cell *newcell;
	switch (pos) {
	case BEFOREHEAD:
		newcell = xmalloc(sizeof(*newcell));
		*newcell = *cell;
		cell->key = key;
		cell->value = value;
		cell->next = newcell;
		return value;
	case EMPTYHEAD:
		cell->key = key;
		cell->value = value;
		return value;
	case BEFORENEXTCELL:
	case ENDOFCHAIN:
		newcell = xmalloc(sizeof(*newcell));
		newcell->key = key;
		newcell->value = value;
		newcell->next = cell->next;
		cell->next = newcell;
		return value;
	case HEAD:
		return cell->value;
	case NEXTCELL:
		return cell->next->value;
	default:
		assert(0);
		return NULL;
	}
}

static int
cellarr_set(Cell *cellarr, size_t len, const char *key, void *value)
{
	FindCellPos pos;
	Cell *cell = findcell(cellarr, len, key, &pos);
	Cell *newcell, *nextcell;
	switch (pos) {
	case EMPTYHEAD:
		if (value) {
			cell->key = key;
			cell->value = value;
			return 1;
		} else {
			return 0;
		}
	case BEFORENEXTCELL:
	case ENDOFCHAIN:
		if (value) {
			newcell = xmalloc(sizeof(*newcell));
			newcell->key = key;
			newcell->value = value;
			newcell->next = cell->next;
			cell->next = newcell;
			return 1;
		} else {
			return 0;
		}
	case HEAD:
		if (value) {
			cell->value = value;
			return 0;
		} else {
			nextcell = cell->next;
			if (nextcell) {
				*cell = *nextcell;
				free(nextcell);
			} else {
				cell->key = cell->value = NULL;
			}
			return -1;
		}
	case NEXTCELL:
		nextcell = cell->next;
		if (value) {
			nextcell->value = value;
			return 0;
		} else {
			cell->next = nextcell->next;
			free(nextcell);
			return -1;
		}
	default:
		assert(0);
		return 0;
	}
}

static void
resize(Strmap *sm, size_t newlen)
{
	Cell *new_cellarr = xcalloc(newlen, sizeof(*new_cellarr));
	Cell *cell, *nextcell;
	size_t i = sm->len;

	init_cellarr(new_cellarr, newlen);
	while (i--) {
		cell = sm->cellarr + i;
		if (cell->key) {
			cellarr_insert(new_cellarr, newlen, cell->key, cell->value);
			nextcell = cell->next;
			while (nextcell) {
				cell = nextcell;
				nextcell = cell->next;
				cellarr_insert(new_cellarr, newlen, cell->key, cell->value);
				free(cell);
			}
		}
	}
	free(sm->cellarr);
	sm->cellarr = new_cellarr;
	sm->len = newlen;
}

void
sm_init(Strmap *sm)
{
	sm->cellarr = xcalloc(NEW_TABLE_LEN, sizeof(*sm->cellarr));
	init_cellarr(sm->cellarr, NEW_TABLE_LEN);
	sm->len = NEW_TABLE_LEN;
	sm->maxpayload = maxpayload(NEW_TABLE_LEN);
	sm->payload = 0;
}

void
sm_uninit(Strmap *sm)
{
	size_t i = sm->len;
	Cell *cell, *nextcell;

	while (i--) {
		cell = sm->cellarr + i;
		nextcell = cell->next;
		while (nextcell) {
			cell = nextcell;
			nextcell = cell->next;
			free(cell);
		}
	}
	free(sm->cellarr);
}

Strmap *
sm_new()
{
	Strmap *ret = xmalloc(sizeof(*ret));
	sm_init(ret);
	return ret;
}

void
sm_delete(Strmap *sm)
{
	sm_uninit(sm);
	free(sm);
}

size_t
sm_size(const Strmap *sm)
{
	return sm->payload;
}

void *
sm_insert(Strmap *sm, const char *key, void *value)
{
	void *ret = cellarr_insert(sm->cellarr, sm->len, key, value);
	assert(value != NULL);
	if (ret == value && ++sm->payload > sm->maxpayload) {
		size_t newlen = expansion_len(sm->len);
		resize(sm, newlen);
		sm->maxpayload = maxpayload(newlen);
	}
	return ret;
}

void *
sm_set(Strmap *sm, const char *key, void *value)
{
	int n = cellarr_set(sm->cellarr, sm->len, key, value);
	switch (n) {
	case -1:
		--sm->payload;
		break;
	case 0:
		break;
	case 1:
		if (++sm->payload > sm->maxpayload) {
			size_t newlen = expansion_len(sm->len);
			resize(sm, newlen);
			sm->maxpayload = maxpayload(newlen);
		}
		break;
	default:
		assert(0);
	}
	return value;
}

void *
sm_get(const Strmap *sm, const char *key)
{
	FindCellPos pos;
	Cell *cell = findcell(sm->cellarr, sm->len, key, &pos);
	switch (pos) {
	case HEAD:
		return cell->value;
	case NEXTCELL:
		return cell->next->value;
	default:
		return NULL;
	}
}

void
sm_foreach(Strmap *sm, void (*func)(const char *key, void *value))
{
	size_t i = sm->len;
	Cell *cell, *nextcell;

	while (i--) {
		cell = sm->cellarr + i;
		if (cell->key) {
			nextcell = cell->next;
			func(cell->key, cell->value);
			while (nextcell) {
				cell = nextcell;
				nextcell = cell->next;
				func(cell->key, cell->value);
			}
		}
	}
}
