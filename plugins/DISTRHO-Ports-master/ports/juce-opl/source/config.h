#define VERSION "0.74"

/* Define to 1 to enable internal debugger, requires libcurses */
#define C_DEBUG 0

/* Define to 1 to enable screenshots, requires libpng */
#define C_SSHOT 1

/* Define to 1 to use opengl display output support */
#define C_OPENGL 1

/* Define to 1 to enable internal modem support, requires SDL_net */
#define C_MODEM 1

/* Define to 1 to enable IPX networking support, requires SDL_net */
#define C_IPX 1

/* Enable some heavy debugging options */
#define C_HEAVY_DEBUG 0

/* The type of cpu this host has */
#define C_TARGETCPU X86
//#define C_TARGETCPU X86_64

/* Define to 1 to use x86 dynamic cpu core */
#define C_DYNAMIC_X86 1

/* Define to 1 to use recompiling cpu core. Can not be used together with the dynamic-x86 core */
#define C_DYNREC 0

/* Enable memory function inlining in */
#define C_CORE_INLINE 0

/* Enable the FPU module, still only for beta testing */
#define C_FPU 1

/* Define to 1 to use a x86 assembly fpu core */
#define C_FPU_X86 1

/* Define to 1 to use a unaligned memory access */
#define C_UNALIGNED_MEMORY 1

/* environ is defined */
#define ENVIRON_INCLUDED 1

/* environ can be linked */
#define ENVIRON_LINKED 1

/* Define to 1 if you have the <ddraw.h> header file. */
#define HAVE_DDRAW_H 1

/* Define to 1 if you want serial passthrough support (Win32 only). */
#define C_DIRECTSERIAL 1

#define GCC_ATTRIBUTE(x) /* attribute not supported */
#define GCC_UNLIKELY(x) (x)
#define GCC_LIKELY(x) (x)

#define INLINE __forceinline
#define DB_FASTCALL __fastcall

#if defined(_MSC_VER) && (_MSC_VER >= 1400) 
#pragma warning(disable : 4996) 
#endif

#ifdef _WIN32
	typedef         double		Real64;
	/* The internal types */
	typedef  unsigned char      BYTE;
	typedef  unsigned char		Bit8u;
	typedef    signed char		Bit8s;
	typedef unsigned short		Bit16u;
	typedef   signed short		Bit16s;
	typedef  unsigned long		Bit32u;
	typedef    signed long		Bit32s;
	typedef unsigned __int64	Bit64u;
	typedef   signed __int64	Bit64s;
	typedef unsigned int		Bitu;
	typedef signed int			Bits;
#endif

#if __APPLE__ || __linux__
	/// Jeff-Russ modified to be uniform across C++ implementations:
	// The internal types:
	#include <stdint.h>
	typedef unsigned char	BYTE;
	typedef   int64_t       __int64;
	typedef   double        Real64;
	typedef   unsigned char Bit8u;
	typedef   signed char   Bit8s;
	typedef   uint16_t      Bit16u;
	typedef   int16_t       Bit16s;
	typedef   uint32_t 		Bit32u;
	typedef   int32_t       Bit32s;
	typedef   uint64_t      Bit64u;
	typedef   int64_t       Bit64s;
	typedef   Bit32u        Bitu;
	typedef   Bit32s        Bits;
#endif


/// Jeff-Russ PUT PLATFORM SPECIFIC STUFF HERE:
#ifndef _WIN32                      /// __forceinline likely not needed 
    #define __forceinline inline    /// outside of windows.
#endif

#ifdef _WIN32 // covers both 32 and 64-bit
    #define INLINE __forceinline
#endif 

//#elif __APPLE__
//    #include "TargetConditionals.h"
//    #define INLINE inline /// apple has no forceinline
//#elif __linux
//    #define INLINE inline
//#elif __unix
//    #define INLINE inline
//#elif __posix
//    #define INLINE inline
//#else
//    #error Unsupported Operating System
//#endif
