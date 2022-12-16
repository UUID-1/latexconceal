/*  unitex: TeX-to-Unicode converter.
 *  Copyright (C) 2022 Juiyung Hsu
 *  License: GNU General Public License v3.0
 *  You should have received a copy of the license along with this
 *  file. If not, see <http://www.gnu.org/licenses>.
 */

#define VEC_CAT2(X, Y) X ## Y
#define VEC_CAT(X, Y) VEC_CAT2(X, Y)

#define VEC_new      VEC_CAT(VEC_PREFIX, new)
#define VEC_delete   VEC_CAT(VEC_PREFIX, delete)
#define VEC_to_block VEC_CAT(VEC_PREFIX, to_block)
#define VEC_size     VEC_CAT(VEC_PREFIX, size)
#define VEC_reserve  VEC_CAT(VEC_PREFIX, reserve)
#define VEC_resize   VEC_CAT(VEC_PREFIX, resize)
#define VEC_push     VEC_CAT(VEC_PREFIX, push)
#define VEC_pop      VEC_CAT(VEC_PREFIX, pop)
#define VEC_top      VEC_CAT(VEC_PREFIX, top)
#define VEC_get      VEC_CAT(VEC_PREFIX, get)
#define VEC_getptr   VEC_CAT(VEC_PREFIX, getptr)
#define VEC_set      VEC_CAT(VEC_PREFIX, set)
#define VEC_insert   VEC_CAT(VEC_PREFIX, insert)
#define VEC_erasen   VEC_CAT(VEC_PREFIX, erasen)

#define VEC_T      char
#define VEC_STRUCT Charv
#define VEC_PREFIX cv_

#include "vec.h.tmpl"

#ifdef VEC_C
#include "vec.c.tmpl"
#endif

#undef VEC_T
#undef VEC_STRUCT
#undef VEC_PREFIX

#define VEC_T      size_t
#define VEC_STRUCT Idxv
#define VEC_PREFIX iv_

#include "vec.h.tmpl"

#ifdef VEC_C
#include "vec.c.tmpl"
#endif

#undef VEC_T
#undef VEC_STRUCT
#undef VEC_PREFIX

/* Typedef compound type so that it behaves as expected in the template code. */
typedef const char *strv_value_type;
#define VEC_T      strv_value_type
#define VEC_STRUCT Strv
#define VEC_PREFIX sv_

#include "vec.h.tmpl"

#ifdef VEC_C
#include "vec.c.tmpl"
#endif

#undef VEC_T
#undef VEC_STRUCT
#undef VEC_PREFIX

#undef VEC_new
#undef VEC_delete
#undef VEC_to_block
#undef VEC_size
#undef VEC_reserve
#undef VEC_resize
#undef VEC_push
#undef VEC_pop
#undef VEC_top
#undef VEC_get
#undef VEC_getptr
#undef VEC_set
#undef VEC_insert
#undef VEC_erasen

#undef VEC_CAT
#undef VEC_CAT2
