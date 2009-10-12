#ifndef _KPSTDLIB_H_
#define _KPSTDLIB_H_

#ifndef __KERNEL__
    #define __KERNEL__
#endif

#if !defined(UNIX) && (defined(SOLARIS) || defined(LINUX))
    #define UNIX
#endif

#if defined(WIN40)
    #ifndef WIN95
        #define WIN95
    #endif
#endif

#if defined(UNIX)
    #include "windrvr.h" // for use of KDGB DWORD parameter.
#endif

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus 

#if defined(WIN95) || defined(WINNT) || defined(WINCE) || defined(WIN32)
    typedef unsigned long ULONG;
    typedef unsigned short USHORT;
    typedef unsigned char UCHAR;
    typedef long LONG;
    typedef short SHORT;
    typedef char CHAR;
    typedef ULONG DWORD;
    typedef USHORT WORD;
    typedef void *PVOID;
    typedef char *PCHAR;
    typedef PVOID HANDLE;
    #if !(defined(WIN95) && defined(NOBASEDEFS))
        typedef ULONG BOOL;
    #endif
    #ifndef WINAPI
        #define WINAPI
    #endif
#elif defined(UNIX) 
    #ifndef __cdecl
        #define __cdecl 
    #endif
#endif

#if defined(WIN95) || defined(WINNT) || defined(WINCE) || defined(WIN32)
    #define OS_needs_copy_from_user(fKernelMode) FALSE
    #define COPY_FROM_USER(dst,src,n) memcpy(dst,src,n)
    #define COPY_TO_USER(dst,src,n) memcpy(dst,src,n)
#elif defined(SOLARIS)
    #define OS_needs_copy_from_user(fKernelMode) (!fKernelMode)
    #define COPY_FROM_USER(dst,src,n) copyin(src,dst,n)
    #define COPY_TO_USER(dst,src,n) copyout(src,dst,n)
#elif defined(LINUX)
    #define OS_needs_copy_from_user(fKernelMode) (!fKernelMode && LINUX_need_copy_from_user())
    #define COPY_FROM_USER(dst,src,n) LINUX_copy_from_user(dst,src,n)
    #define COPY_TO_USER(dst,src,n) LINUX_copy_to_user(dst,src,n)
#endif

#if defined(WINCE)
    #define CE_map_ptr(ptr,fKernelMode) (fKernelMode ? (ptr) : (MapPtrToProcess((ptr),GetCallerProcess())))
#else
    #define CE_map_ptr(ptr,fKernelMode) (ptr)
#endif

#define COPY_FROM_USER_OR_KERNEL(dst, src, n, fKernelMode) \
{ \
    if (OS_needs_copy_from_user(fKernelMode)) \
        COPY_FROM_USER(dst, src, n); \
    else \
        memcpy (dst, CE_map_ptr(src,fKernelMode), n);  \
}

#define COPY_TO_USER_OR_KERNEL(dst, src, n, fKernelMode) \
{ \
    if (OS_needs_copy_from_user(fKernelMode)) \
        COPY_TO_USER(dst, src, n); \
    else \
        memcpy (CE_map_ptr(dst,fKernelMode), src, n);  \
}

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef NULL
#define NULL 0
#endif

int __cdecl KDBG(DWORD dwLevel, DWORD dwSection, const char *format, ...);

#if defined(WIN95)
    // Define varargs ANSI style

    #define _INTSIZEOF(n)    ( (sizeof(n) + sizeof(int) - 1) & ~(sizeof(int) - 1) )

    #define va_start(ap,v) ap = (char *)&v + _INTSIZEOF(v)
    #define va_arg(ap,t) ( *(t *)((ap += _INTSIZEOF(t)) - _INTSIZEOF(t)) )
    #define va_end(ap) ap = (char *)0
    typedef char * va_list;

    void* __cdecl memchr(void *buf, int c, unsigned int count);
    void* __cdecl memmove(void *dest, void *src, unsigned int count);
    int __cdecl sscanf(const char* buf, const char* fmt, ...);
    long __cdecl strtoul(const char* s, char** pend, int radix);
    char* __cdecl _ultoa(unsigned long u, char* s, int radix);
    long __cdecl strtol(const char* s, char** pend, int radix);
    char* __cdecl _ltoa(long value, char* s, int radix);
    long __cdecl atol(const char* s);
    char* __cdecl strstr( const char* s1, const char* s2);
    char* __cdecl strrchr( const char* s, int c);
    char* __cdecl _strlwr(char* s);
    char* __cdecl _strupr( char* s);
    char* __cdecl strncpy( char* s1, const char* s2, unsigned int c);
    int __cdecl strncmp(const char* s1, const char* s2, unsigned int c);
    char* __cdecl strncat( char* s1, const char* s2, unsigned int c);
    unsigned int __cdecl strlen( const char* s);
    int __cdecl _stricmp( const char* s1, const char* s2);
    int __cdecl strcmp( char* s1, const char* s2);
    char* __cdecl _strdup( const char* s);
    unsigned int __cdecl strcspn( const char* s1, const char* s2);
    char* __cdecl strchr( const char* s, int c);
    char* __cdecl strcat( char* d, const char* s);
    int __cdecl sprintf(char *buffer, const char *format, ...);
    void __cdecl dprintf(const char *format, ...);
    void* __cdecl malloc(unsigned int size);
    void* __cdecl calloc(unsigned int num, unsigned int size);
    void* __cdecl realloc(void *memblock, unsigned int size);
    int __cdecl _snprintf(char *buffer, unsigned int Limit, const char *format, ...);
    int __cdecl _vsnprintf(char *buffer, unsigned int Limit, const char *format, va_list Next);
    int __cdecl vsprintf(char *buffer, const char *format, va_list Next);
    int __cdecl memcmp(const char* s1, const char* s2, unsigned int c);
    void* __cdecl memset( void *dest, int c, unsigned int count);
    void* __cdecl memcpy( void *dest, const void *src, unsigned int count);
    int toupper(int c);
    int tolower(int c);

#endif // WIN95

char* __cdecl strcpy( char* s1, const char* s2);
void* __cdecl malloc(unsigned int size);
void __cdecl free(void* buf);

#if defined(SOLARIS)
    #if defined(SPARC)
        #include <sys/types.h>
        // since stacture copy is impleneted in gcc with memcpy on SPARC 
        // this is needed
        static void *memcpy(void *s1, const void *s2, size_t n)
        {
            bcopy( s2, s1, n );
            return s1;
        }

        static void *memset(void *s, int c, size_t n)
        {
            char *p = s;
            
            for( ; n ; --n )
                *p++ = (char)c ;
        }
    #else
        #define memset(x, y, z) bzero(x, z)
        #define memcpy(dst, src, size)  bcopy( src, dst, size)
    #endif
#endif 

#ifdef __cplusplus
}
#endif  // __cplusplus 

#endif // _KPSTDLIB_H_

