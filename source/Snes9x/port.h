#include "copyright.h"


#ifndef _PORT_H_
#define _PORT_H_

#include <limits.h>

#ifndef STORM
//#include <memory.h>
#include <string.h>
#else
#include <strings.h>
#include <clib/powerpc_protos.h>
#endif

#ifndef ACCEPT_SIZE_T
#ifdef __WIN32__
#define ACCEPT_SIZE_T int
#else
#define ACCEPT_SIZE_T unsigned int
#endif
#endif

#include <sys/types.h>

/* #define PIXEL_FORMAT RGB565 */
#define GFX_MULTI_FORMAT

#if defined(TARGET_OS_MAC) && TARGET_OS_MAC

#include "zlib.h"
#define ZLIB
#define EXECUTE_SUPERFX_PER_LINE
#define SOUND
#define VAR_CYCLES
//#define CPU_SHUTDOWN
#define SPC700_SHUTDOWN
#define PIXEL_FORMAT RGB555
#define CHECK_SOUND()
#define M_PI 3.14159265359
#undef  _MAX_PATH

#undef DEBUGGER /* Apple Universal Headers sometimes #define DEBUGGER */
#undef GFX_MULTI_FORMAT

int    strncasecmp(const char *s1, const char *s2, unsigned n);
int    strcasecmp(const char *s1, const char *s2 );

#endif /* TARGET_OS_MAC */

#ifndef NOASM
#define USE_X86_ASM
#endif

#ifndef snes9x_types_defined
#define snes9x_types_defined

typedef unsigned char bool8;

/* FIXME: Refactor this by moving out the BORLAND part and unifying typedefs */
#ifndef __WIN32__
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef signed char int8;
typedef short int16;
typedef int int32;
typedef unsigned int uint32;
# ifdef __GNUC__  /* long long is not part of ISO C++ */
__extension__
# endif
typedef long long int64;
#else /* __WIN32__ */

# ifdef __BORLANDC__
#   include <systypes.h>
# else

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef signed char int8;
typedef short int16;

# ifndef WSAAPI
/* winsock2.h typedefs int32 as well. */
typedef long int32;

#   define PLAT_SOUND_BUFFER SoundBuffer
#   define RIGHTSHIFT_IS_SAR
# endif

typedef unsigned int uint32;

# endif /* __BORLANDC__ */

typedef __int64 int64;

#endif /* __WIN32__ */
#endif /* snes9x_types_defined */


#include "pixform.h"

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifdef STORM
#define EXTERN_C
#define START_EXTERN_C
#define END_EXTERN_C
#else
#if defined(__cplusplus) || defined(c_plusplus)
#define EXTERN_C extern "C"
#define START_EXTERN_C extern "C" {
#define END_EXTERN_C }
#else
#define EXTERN_C extern
#define START_EXTERN_C
#define END_EXTERN_C
#endif
#endif

#ifndef __WIN32__

#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

#define _MAX_DIR PATH_MAX
#define _MAX_DRIVE 1
#define _MAX_FNAME PATH_MAX
#define _MAX_EXT PATH_MAX
#define _MAX_PATH PATH_MAX

#define ZeroMemory(a,b) memset((a),0,(b))

void _makepath (char *path, const char *drive, const char *dir,
		const char *fname, const char *ext);
void _splitpath (const char *path, char *drive, char *dir, char *fname,
		 char *ext);
#else /* __WIN32__ */
#define strcasecmp stricmp
#define strncasecmp strnicmp
#endif

EXTERN_C void S9xGenerateSound ();

#ifdef STORM
EXTERN_C int soundsignal;
EXTERN_C void MixSound(void);
/* Yes, CHECK_SOUND is getting defined correctly! */
#define CHECK_SOUND if (Settings.APUEnabled) if(SetSignalPPC(0L, soundsignal) & soundsignal) MixSound
#else
#define CHECK_SOUND()
#endif

#ifdef __DJGPP
#define SLASH_STR "\\"
#define SLASH_CHAR '\\'
#else
#define SLASH_STR "/"
#define SLASH_CHAR '/'
#endif

/* Taken care of in signal.h on Linux.
 * #ifdef __linux
 * typedef void (*SignalHandler)(int);
 * #define SIG_PF SignalHandler
 * #endif
 */

/* If including signal.h, do it before snes9.h and port.h to avoid clashes. */
#ifndef SIG_PF
#define SIG_PF void(*)(int)
#endif

#if defined(__i386__) || defined(__i486__) || defined(__i586__) || \
    defined(__WIN32__) || defined(__alpha__) || defined(ARM11)
#define LSB_FIRST
#define FAST_LSB_WORD_ACCESS
#else
#define MSB_FIRST
#endif

#ifdef __sun
#define TITLE "Snes9X: Solaris"
#endif

#ifdef __linux
#define TITLE "Snes9X: Linux"
#endif

#ifndef TITLE
#define TITLE "Snes9x"
#endif

#ifdef STORM
#define STATIC
#define strncasecmp strnicmp
#else
#define STATIC static
#endif

#endif

