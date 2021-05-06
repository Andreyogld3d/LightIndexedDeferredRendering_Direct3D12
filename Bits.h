#ifndef __BITS_H__
#define __BITS_H__

#include "types.h"

#undef INLINE
#ifdef _WIN32
#ifdef _MSC_VER
#define INLINE __forceinline
#endif
#else
#ifdef __GNUC__
//#define INLINE __attribute__((always_inline))
#endif
#endif

#ifndef INLINE
#define INLINE inline
#endif

//
template<typename Type, typename TypeMask, typename ShiftType>
uint GetBitValue(Type bits, TypeMask mask, ShiftType shift)
{
	return (bits & mask) >> shift;
}

template<typename Type, typename BitType>
void ClearBitByNumber(Type& value, BitType bit)
{
	value &= ~(1 << bit);
}

template<typename Type, typename MaskType>
void ClearBits(Type& value, MaskType mask)
{
	value &= ~mask;
}

template<typename ValueType, typename FlagTypeType, typename MaskType>
void ChangeBits(ValueType& value, FlagTypeType flag, MaskType mask)
{
	if (flag) {
		value |= mask;
	}
	else {
		ClearBits(value, mask);
	}
}

#if 1
typedef uint BitsBool;

template<typename ValueType, typename MaskType>
INLINE BitsBool IsBitsSet(ValueType value, MaskType mask)
{
	return value & mask;
}

#else

typedef bool BitsBool;

template<typename ValueType, typename MaskType>
BitsBool IsBitsSet(ValueType value, uint mask)
{
	return (value & mask) != 0;
}


#endif

#endif // __BITS_H__