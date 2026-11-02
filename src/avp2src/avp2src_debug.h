/*
 * avp2src_debug.h -- diagnostics for the debug build, and nothing in release.
 *
 * The release build has to compile to the retail bytes, so nothing here may
 * reach it: without AVP2_DEBUG (which only the debug build defines, see
 * build/variant.sh) every macro below expands to nothing and this header
 * includes nothing. Arguments are not evaluated in release either, so never
 * put a side effect in them.
 *
 * Any file of any target can include it: src/avp2src/ is on every
 * compiler's include path (build/env.sh).
 *
 *   #include "avp2src_debug.h"
 *
 *   AVP2SRC_LOG(("loading world %s, %d objects", pName, nObjects));
 *   AVP2SRC_ASSERT(pObj != NULL);
 *
 *   #ifdef AVP2_DEBUG
 *       ... any other debug-only code ...
 *   #endif
 *
 * AVP2SRC_LOG takes printf arguments in a second pair of parentheses (VC6
 * has no variadic macros). Each line is written, with the time in
 * milliseconds and the source file and line, to avp2src.log in the current
 * directory (the game directory; $AVP2SRC_LOG names another file) and to
 * OutputDebugString (shown by Wine with WINEDEBUG=+debugstr). AVP2SRC_ASSERT
 * logs a failed condition and carries on.
 *
 * Plain C and C++, VC6 and Intel C/C++ 4.0, MFC or not: it declares the two
 * Win32 functions it calls rather than including <windows.h>, which MFC
 * files must not see before their own headers.
 */
#ifndef AVP2_DEBUG_H
#define AVP2_DEBUG_H

#ifdef AVP2_DEBUG

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif
__declspec(dllimport) void __stdcall OutputDebugStringA(const char *lpOutputString);
__declspec(dllimport) unsigned long __stdcall GetTickCount(void);
#ifdef __cplusplus
}
#endif

/* where the next Avp2srcLogf line comes from (set by AVP2SRC_LOG) */
static const char *g_Avp2srcLogFile;
static int g_Avp2srcLogLine;

static void Avp2srcLogAt(const char *file, int line)
{
    g_Avp2srcLogFile = file;
    g_Avp2srcLogLine = line;
}

static void __cdecl Avp2srcLogf(const char *fmt, ...)
{
    char buf[1024];
    int n;
    va_list ap;
    FILE *f;
    const char *path;

    if (g_Avp2srcLogLine > 0)
        n = _snprintf(buf, sizeof(buf) - 2, "[%8lu] %s(%d): ", GetTickCount(),
                      g_Avp2srcLogFile ? g_Avp2srcLogFile : "?", g_Avp2srcLogLine);
    else            /* no source position: just a tag */
        n = _snprintf(buf, sizeof(buf) - 2, "[%8lu] %s: ", GetTickCount(),
                      g_Avp2srcLogFile ? g_Avp2srcLogFile : "?");
    if (n < 0)
        n = 0;
    va_start(ap, fmt);
    if (_vsnprintf(buf + n, sizeof(buf) - 2 - n, fmt, ap) < 0)
        n = sizeof(buf) - 2;            /* truncated: keep what fits */
    else
        n += (int)strlen(buf + n);
    va_end(ap);
    buf[n++] = '\n';
    buf[n] = '\0';

    OutputDebugStringA(buf);
    path = getenv("AVP2SRC_LOG");
    f = fopen(path && *path ? path : "avp2src.log", "a");
    if (f) {
        fputs(buf, f);
        fclose(f);
    }
}

#define AVP2SRC_LOG(args) (Avp2srcLogAt(__FILE__, __LINE__), Avp2srcLogf args)
#define AVP2SRC_ASSERT(expr) \
    ((expr) ? (void)0 : (Avp2srcLogAt(__FILE__, __LINE__), \
                         Avp2srcLogf("assertion failed: %s", #expr)))

#else /* release: nothing */

#define AVP2SRC_LOG(args) ((void)0)
#define AVP2SRC_ASSERT(expr) ((void)0)

#endif /* AVP2_DEBUG */

#endif /* AVP2_DEBUG_H */
