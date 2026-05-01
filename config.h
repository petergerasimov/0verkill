/* Platform configuration header.
 *
 * Replaces the autoconf-generated config.h. Sets the HAVE_* macros the
 * source uses, based on the platform we are compiling for. The compiler's
 * own predefined macros (__linux__, __APPLE__, _WIN32, __EMSCRIPTEN__, ...)
 * select the right block.
 */

#ifndef OVERKILL_CONFIG_H
#define OVERKILL_CONFIG_H

/* All modern toolchains support these. */
/* HAVE_INLINE is intentionally not defined: the original code uses gnu89 inline
 * semantics (inline emits an external definition); modern C99/C11 inverts that
 * and we'd need -fgnu89-inline to keep it. Plain non-inline functions work
 * fine — the compiler still inlines small ones on its own. */
#define HAVE_TYPEOF
#define HAVE_CALLOC
#define HAVE_GETOPT
#define HAVE_UNISTD_H
#define HAVE_SYS_SELECT_H
#define HAVE_FLOAT_H
#define HAVE_GETTIMEOFDAY
#define HAVE_SELECT
#define HAVE_STRTOL
#define HAVE_STRTOUL
#define HAVE_RANDOM
#define HAVE_SRANDOM
#define HAVE_SOCKET
#define HAVE_ACCESS

#if defined(__linux__)
	#define HAVE_LINUX_KD_H
	#define HAVE_LINUX_VT_H
	#define HAVE_PTHREAD_H
	#define HAVE_LIBPTHREAD
	#define HAVE_PSIGNAL
#elif defined(__APPLE__)
	#define HAVE_PTHREAD_H
	#define HAVE_LIBPTHREAD
	#define HAVE_PSIGNAL
#elif defined(__EMSCRIPTEN__)
	/* No psignal, no raw keyboard, no pthread by default. */
	#undef HAVE_SYS_SELECT_H
#elif defined(_WIN32) || defined(WIN32)
	#undef HAVE_UNISTD_H
	#undef HAVE_SYS_SELECT_H
	#undef HAVE_GETOPT
#elif defined(__EMX__)
	/* OS/2 EMX — keep generic POSIX, the source provides its own keyboard. */
#else
	/* Generic UNIX. */
	#define HAVE_PTHREAD_H
	#define HAVE_LIBPTHREAD
#endif

/* Optional X11 graphics. Defined by the Makefile when building X variants. */
/* #define HAVE_LIBX11 */
/* #define HAVE_LIBXPM */

/* Optional SDL2 graphics. Defined by the Makefile when building SDL variants. */
/* #define HAVE_LIBSDL2 */

#endif /* OVERKILL_CONFIG_H */
