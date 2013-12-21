/*
 * custom.h
 * Added at end of config.h using AH_BOTTOM macro
 */

/*
 * A recommended way to include commonly needed headers
 * Reference: AC_INCLUDE_DEFAULTS
 */
#include <stdio.h>
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif
#ifdef STDC_HEADERS
# include <stdlib.h>
# include <stddef.h>
#else
# ifdef HAVE_STDLIB_H
#  include <stdlib.h>
# endif
#endif
#ifdef HAVE_STRING_H
# if !defined STDC_HEADERS && defined HAVE_MEMORY_H
#  include <memory.h>
# endif
# include <string.h>
#endif
#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif
#ifdef HAVE_INTTYPES_H
# include <inttypes.h>
#endif
#ifdef HAVE_STDINT_H
# include <stdint.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#ifdef HAVE_STDARG_H
# include <stdarg.h>
#endif

/*
 * time-related
 * Reference: AC_FUNC_MKTIME (internal)
 */
/* #ifdef defined TIME_WITH_SYS_TIME */
/* # include <sys/time.h> */
/* # include <time.h> */
/* #else */
/* # ifdef HAVE_SYS_TIME_H */
/* #  include <sys/time.h> */
/* # else */
/* #  include <time.h> */
/* # endif */
/* #endif */

#if defined HAVE_TIME_H
# include <time.h>
#elif defined HAVE_SYS_TIME_H
# include <sys/time.h>
#endif

#include <assert.h>
#include <math.h>
#include <errno.h>


/*
 * legacy 'byte'
 */
#if defined HAVE_STDINT_H
	typedef uint8_t byte;
#else
	typedef unsigned char byte;
#endif

/*
 * Legacy Quake qboolean
 */
#if defined HAVE_STDBOOL_H
# include <stdbool.h>
#endif

#if defined HAVE__BOOL
	typedef  _Bool  qboolean;
#else
typedef int  qboolean
# if !defined true
#  define true 1
#  define false 0
# endif    
#endif

