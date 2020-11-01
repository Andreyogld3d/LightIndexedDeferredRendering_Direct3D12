#ifndef __TYPES_H__
#define __TYPES_H__

#ifdef __GNUC__
#include <inttypes.h>
#endif

typedef unsigned int uint, u32;
typedef unsigned long ulong;
typedef unsigned char uchar, uint8, ubyte, u8;
typedef unsigned short ushort, uint16, u16;

typedef uint UINT;
typedef ushort USHORT;
typedef uchar UCHAR;
typedef ushort WORD;
#ifndef __GNUC__
typedef unsigned __int64 uint64;
typedef __int64 int64;
#else
typedef uint DWORD;
typedef uint64_t uint64;
typedef int64_t int64;
#endif

#endif // __TYPES_H__
