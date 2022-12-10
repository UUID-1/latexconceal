/*  unitex: TeX-to-Unicode converter.
 *  Copyright (C) 2022 Juiyung Hsu
 *  License: GNU General Public License (version 3 or later)
 *  You should have received a copy of the license along with this
 *  file. If not, see <http://www.gnu.org/licenses>.
 */

/* <stdbool.h> <stdio.h> "strmap.h" "vec.h" should be included before this header */

enum { NUL, EOFBYTE, ILSEQ };
#define tr(C) ((char)((C) ^ 0xf8))

typedef struct {
	Strmap *br;
	const char *key;
} Node;

bool readutf8tail(Charv *cv, unsigned char head, FILE *f);

size_t readtk(Charv *cv, FILE *f);

Node *nd_new(void);
void nd_delete(Node *nd);
void br_delete(Strmap *br);

#ifdef NDEBUG
#define clear_at_exit(P, M) ((void)0)
#else
typedef enum { FREE, CV_DELETE, IV_DELETE, SV_DELETE, BR_DELETE } CLEAR_METHOD;
void clear_at_exit(void *p, CLEAR_METHOD m);
#endif
