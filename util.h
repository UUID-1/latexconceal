/*  unitex: TeX-to-Unicode converter.
 *  Copyright (C) 2022 Juiyung Hsu
 *  License: GNU General Public License v3.0
 *  You should have received a copy of the license along with this
 *  file. If not, see <http://www.gnu.org/licenses>.
 */

extern char *program_invocation_name;

void error(int status, int errnum, const char *format, ...);
void error_at_line(int status, int errnum, const char *filename,
                   unsigned int linenum, const char *format, ...);

void *xmalloc(size_t size);
void *xcalloc(size_t nmemb, size_t size);
void *xrealloc(void *p, size_t size);
void *xreallocarray(void *p, size_t nmemb, size_t size);
