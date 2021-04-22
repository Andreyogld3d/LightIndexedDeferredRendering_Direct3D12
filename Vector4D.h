// Vector4D.h: interface for the Vector4D class.
//
//////////////////////////////////////////////////////////////////////

#ifndef __VECTOR4D_H__
#define __VECTOR4D_H__

#include <cmath>
#include <cassert>
#include <climits>

#ifdef STLPORT
#ifndef __GNUC__
#define SSE
#endif
#endif

#ifdef SSE
#ifdef __GNUC__
#define __SSE__
#define  __MMX__
#ifndef __inline
#ifdef __MINGW32__
#define __inline inline
#else
//#define __inline __attribute__((always_inline))
#endif
#endif
#endif
#include <xmmintrin.h>
#ifdef _MSC_VER
#define ALIGN16 _MM_ALIGN16
#elif __GNUC__
#define ALIGN16 __attribute__(aligned(16))
#else
#endif
#else
#define ALIGN16
#endif

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

class Vector3D;

class Vector4D  {
private:
	typedef unsigned char uchar;
	typedef unsigned int uint;
public:
#ifdef SSE
	union ALIGN16 {
		__m128 v;
		struct {
			float x;
			float y;
			float z;
			float w;
		};
		struct 
		{
			float r;
			float g;
			float b;
			float a;
		};
	};
#else
	union  {
		struct {
			float x;
			float y;
			float z;
			float w;
		};
		struct {
			float r;
			float g;
			float b;
			float a;
		};
		float v[4];
	};
#endif
	INLINE static const Vector4D Zero()
	{
		return Vector4D(0.0f, 0.0f, 0.0f, 0.0f);
	}
	const float& xyz() const
	{
		return x;
	}
	const float& xyz()
	{
		return x;
	}
	void Set(float X, float Y, float Z, float W)
	{
		x = X;
		y = Y;
		z = Z;
		w = W;
	}
	INLINE Vector4D& operator = (const Vector4D& vector4D)
	{
		assert(this != &vector4D && "Apropriation to itself");
		x = vector4D.x;
		y = vector4D.y;
		z = vector4D.z;
		w = vector4D.w;
		return *this;
	}
	uint RGBA() const
	{
		uint dwR = r >= 1.0f ? 0xff : r <= 0.0f ? 0x00 : static_cast<uint>(r * 255.0f + 0.5f);
		uint dwG = g >= 1.0f ? 0xff : g <= 0.0f ? 0x00 : static_cast<uint>(g * 255.0f + 0.5f);
		uint dwB = b >= 1.0f ? 0xff : b <= 0.0f ? 0x00 : static_cast<uint>(b * 255.0f + 0.5f);
		uint dwA = a >= 1.0f ? 0xff : a <= 0.0f ? 0x00 : static_cast<uint>(a * 255.0f + 0.5f);

		return (dwA << 24) | (dwB << 16) | (dwG << 8) | dwR;
	}
	operator uint () const
	{
		uint dwR = r >= 1.0f ? 0xff : r <= 0.0f ? 0x00 : static_cast<uint>(r * 255.0f + 0.5f);
		uint dwG = g >= 1.0f ? 0xff : g <= 0.0f ? 0x00 : static_cast<uint>(g * 255.0f + 0.5f);
		uint dwB = b >= 1.0f ? 0xff : b <= 0.0f ? 0x00 : static_cast<uint>(b * 255.0f + 0.5f);
		uint dwA = a >= 1.0f ? 0xff : a <= 0.0f ? 0x00 : static_cast<uint>(a * 255.0f + 0.5f);

		return (dwA << 24) | (dwR << 16) | (dwG << 8) | dwB;
	}
	INLINE operator float * ()
	{
		return &x;
	}
	INLINE operator const float * () const
	{
		return &x;
	}
	INLINE Vector4D operator - () const
	{
		return Vector4D(- x, - y, - z, - w);
	}
	INLINE Vector4D& operator += (const Vector4D& vector)
	{
		x += vector.x;
		y += vector.y;
		z += vector.z;
		w += vector.w;
		return *this;
	}
	INLINE Vector4D& operator -= (const Vector4D& vector)
	{
		x -= vector.x;
		y -= vector.y;
		z -= vector.z;
		w -= vector.w;
		return *this;
	}
	INLINE Vector4D& operator *= (float Value)
	{
		x *= Value;
		y *= Value;
		z *= Value;
		w *= Value;
		return *this;
	}
	INLINE Vector4D& operator /= (float Value)
	{
		x /= Value;
		y /= Value;
		z /= Value;
		w /= Value;
		return *this;
	}
	INLINE Vector4D& operator /= (const Vector4D& vector)
	{
		x /= vector.x;
 		y /= vector.y;
 		z /= vector.z;
		w /= vector.w;
		return *this;
	}
	INLINE Vector4D& operator *= (const Vector4D& vector)
	{
		x *= vector.x;
 		y *= vector.y;
 		z *= vector.z;
		w *= vector.w;
		return *this;
	}
	INLINE Vector4D& operator += (float Value)
	{
		x += Value;
		y += Value;
		z += Value;
		return *this;
	}
	INLINE Vector4D& operator -= (float Value)
	{
		x -= Value;
		y -= Value;
		z -= Value;
		return *this;
	}
	INLINE bool operator == (const Vector4D& v)
	{
		return (x == v.x && y == v.y && z == v.z && w == v.w);
	}
	INLINE bool operator != (const Vector4D& v) const
	{
		return (x != v.x || y != v.y || z != v.z || w != v.w);
	}
	//
	INLINE float& operator [](int index)
	{
		assert(index < 4 && "Out of Range");
		return *(index + &x);
	}
	INLINE float operator [] (int index) const
	{
		assert(index < 4 && "Out of Range");
		return * (index + &x);
	}
	INLINE bool operator > (const Vector4D& v) const
	{
		return x > v.x && y > v.y && z > v.z && w > v.w;
	}
	INLINE bool operator < (const Vector4D& v) const
	{
		return x < v.x && y < v.y && z < v.z && w < v.w;
	}
	INLINE friend Vector4D operator + ( const Vector4D& vector1, const Vector4D& vector2)
	{
#ifdef SSE
		return Vector4D(_mm_add_ps(vector1.v, vector2.v));
#else
		return Vector4D (vector1.x+vector2.x, vector1.y+vector2.y, vector1.z+vector2.z, vector1.w+vector2.w );
#endif
	}
	INLINE friend Vector4D operator + ( const Vector4D& vector, float value)
	{
		return Vector4D(vector.x + value, vector.y + value, vector.z + value, vector.w);
	}
	INLINE friend Vector4D operator - ( const Vector4D& vector1, const Vector4D& vector2)
	{
#ifdef SSE
		return Vector4D(_mm_sub_ps(vector1.v, vector2.v));
#else
		return Vector4D(vector1.x-vector2.x, vector1.y-vector2.y, vector1.z-vector2.z, vector1.w-vector2.w );
#endif
	}
	INLINE friend Vector4D operator - ( const Vector4D& vector, float value)
	{
		return Vector4D(vector.x - value, vector.y - value, vector.z - value, vector.w);
	}
	INLINE friend Vector4D operator * (float Value, const Vector4D& vector)
	{
		return Vector4D (vector.x * Value, vector.y * Value, vector.z * Value, vector.w * Value);
	}
	INLINE friend Vector4D operator * (const Vector4D& vector, float Value)
	{
		return Vector4D(vector.x * Value, vector.y * Value, vector.z * Value, vector.w * Value);
	}
	INLINE friend Vector4D operator * (const Vector4D& vector1, const Vector4D& vector2)
	{
#ifdef SSE
		return Vector4D(_mm_mul_ps(vector1.v, vector2.v));
#else
		return Vector4D(vector1.x *vector2.x, vector1.y * vector2.y, vector1.z * vector2.z, vector1.w * vector2.w );
#endif
	}
	INLINE friend Vector4D operator / (const Vector4D& vector, float Value)
	{
		return Vector4D(vector.x / Value, vector.y / Value, vector.z / Value, vector.w / Value);
	}
	INLINE friend Vector4D operator / (float Value, const Vector4D& vector)
	{
		return Vector4D ( Value / vector.x, Value / vector.y, Value / vector.z, Value / vector.w);
	}
	//
	bool IsZero() const
	{
		return !x && !y && !z && !w;
	}
	INLINE float Length() const
	{
		return sqrtf(LengthSq());
	}
	INLINE float LengthSq() const
	{
		return x * x + y * y + z * z;
	}
	//
	INLINE Vector4D& Normalize()
	{
		float l = Length();
		assert(l && "Invalid Value");
		l = 1.0f / l;
		x *= l;
		y *= l;
		z *= l;
		w *= l;
		return *this;
	};
	Vector4D& FromARGB(uint color)
	{
		const float f = 1.0f / 255.0f;
		r = f * static_cast<uchar>((color >> 16) & 0xff);
		g = f * static_cast<uchar>((color >> 8) & 0xff);
		b = f * static_cast<uchar>(color & 0xff);
		a = f * static_cast<uchar>((color >> 24) & 0xff);
		return *this;
	}
	Vector4D& FromRGBA(uint color)
	{
		const float f = 1.0f / 255.0f;
		r = f * static_cast<uchar>((color >> 24) & 0xff);
		g = f * static_cast<uchar>((color >> 16) & 0xff);
		b = f * static_cast<uchar>((color >> 8) & 0xff);
		a = f * static_cast<uchar>(color & 0xff);
		return *this;
	}
	void FromColor(const unsigned char* color)
	{
		assert(color && "NULL Pointer");
		const float ColorFloatToUchar = 1.0f / UCHAR_MAX;
		x = color[0] * ColorFloatToUchar;
		y = color[1] * ColorFloatToUchar;
		z = color[2] * ColorFloatToUchar;
		w = color[3] * ColorFloatToUchar;
	}
	INLINE Vector4D()
	{
	}
#ifdef SSE
	INLINE Vector4D (const __m128& V) : v(V)
	{
	}
#endif
	INLINE Vector4D (float X, float Y, float Z, float W ): x(X), y(Y), z(Z), w(W)
	{
	}
	INLINE Vector4D (const Vector4D& vector) : x(vector.x), y(vector.y), z(vector.z),
		w(vector.w)
	{
	}
	INLINE Vector4D (const uchar* color)
	{
		FromColor(color);
	}
};

#endif // __VECTOR4D_H__

