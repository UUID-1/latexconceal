/*  unitex: TeX-to-Unicode converter.
 *  Copyright (C) 2022 Juiyung Hsu
 *  License: GNU General Public License (version 3 or later)
 *  You should have received a copy of the license along with this
 *  file. If not, see <http://www.gnu.org/licenses>.
 */

typedef struct {
	struct StrmapCell *cellarr;
	size_t len;
	size_t payload;
	size_t maxpayload;
} Strmap;

void sm_init(Strmap *sm);
void sm_uninit(Strmap *sm);
Strmap *sm_new(void);
void sm_delete(Strmap *sm);
size_t sm_size(const Strmap *sm);
void *sm_insert(Strmap *sm, const char *key, void *value);
void *sm_set(Strmap *sm, const char *key, void *value);
void *sm_get(const Strmap *sm, const char *key);
void sm_foreach(Strmap *sm, void (*func)(const char *key, void *value));
