/*  unitex: TeX-to-Unicode converter.
 *  Copyright (C) 2022 Juiyung Hsu
 *  License: GNU General Public License (version 3 or later)
 *  You should have received a copy of the license along with this
 *  file. If not, see <http://www.gnu.org/licenses>.
 */

/* <stdbool.h> "strmap.h" "vec.h" should be included before this header */

void parserules(const Strv *files, Strmap **pinvbr, Strmap **prtbr,
                Strmap **psubsbr, Strmap **psupsbr, bool *test_rtbr_initial);
