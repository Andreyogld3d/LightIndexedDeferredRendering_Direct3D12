// Vector2D.h: interface for the Vector2D class.
//
//////////////////////////////////////////////////////////////////////

#ifndef __VECTOR2D_H__
#define __VECTOR2D_H__

#include <cmath>
#include <cassert>

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

#ifndef Pi
#define Pi static_cast<float>(3.14159265)
#endif

class Vector2D  { // 
public:
	float x;
	float y;
	INLINE static const Vector2D Zero()
	{
		return Vector2D(0.0f, 0.0f);
	}
	INLINE static const Vector2D X()
	{
		return Vector2D(1.0f, 0.0f);
	}
	INLINE static const Vector2D Y()
	{
		return Vector2D(0.0f, 1.0f);
	}
	INLINE Vector2D& operator = (const Vector2D& vector2D)
	{
		assert(this != &vector2D && "Apropriation to itself");
		x = vector2D.x;
		y = vector2D.y;
		return *this;
	}
	template<typename T>
	INLINE Vector2D& operator = (const T& data)
	{
		*this = reinterpret_cast<const Vector2D& >(data);
		return *this;
	}
	template<typename T>
	INLINE Vector2D& operator = (const T* data)
	{
		assert(data && "NULL Pointer");
		*this = reinterpret_cast<const Vector2D &>(data);
		return *this;
	}
	INLINE operator float * ()
	{
		return &x;
	}
	INLINE operator const float * () const
	{
	return &x;
	}
	INLINE Vector2D operator - () const
	{
		return Vector2D(- x, - y);
	}
	INLINE Vector2D& operator += (const Vector2D& Vector)
	{
		x += Vector.x;
		y += Vector.y;
		return *this;
	}
	INLINE Vector2D& operator -= (const Vector2D& Vector)
	{
		x -= Vector.x;
		y -= Vector.y;
		return *this;
	}
	INLINE Vector2D& operator *= (float Value)
	{
		x *= Value;
		y *= Value;
		return *this;
	}
	INLINE Vector2D& operator /= (float Value)
	{
		x /= Value;
		y /= Value;
		return *this;
	}
	INLINE Vector2D& operator -= (float Value)
	{
		x -= Value;
		y -= Value;
		return *this;
	}
	INLINE Vector2D& operator += (float Value)
	{
		x += Value;
		y += Value;
	return *this;
	}
	INLINE Vector2D& operator /= (const Vector2D& vector)
	{
		x /= vector.x;
		y /= vector.y;
		return *this;
	}
	INLINE Vector2D& operator *= (const Vector2D& vector)
	{
		x *= vector.x;
		y *= vector.y;
		return *this;
	}
	INLINE bool operator == (const Vector2D& vector) const
	{
		return (x == vector.x && y == vector.y);
	}
	INLINE bool operator != (const Vector2D& vector) const
	{
		return (x != vector.x || y != vector.y);
	}
	// 
	INLINE float& operator [](int index)
	{
		return *(index + &x);
	}
	INLINE float operator [] (int index) const
	{
		return *(index + &x);
	}
	INLINE bool operator > (const Vector2D& v) const
	{
		return x > v.x && y > v.y;
	}
	INLINE bool operator < (const Vector2D& v) const
	{
		return x < v.x && y < v.y;
	}
	INLINE friend Vector2D operator + (const Vector2D& vector1, const Vector2D& vector2)
	{
		return Vector2D(vector1.x + vector2.x, vector1.y + vector2.y);
	}
	INLINE friend Vector2D operator + (const Vector2D& vector, float value)
	{
		return Vector2D(vector.x + value, vector.y + value);
	}
	INLINE friend Vector2D operator - (const Vector2D& vector1, const Vector2D& vector2)
	{
		return Vector2D(vector1.x - vector2.x, vector1.y - vector2.y);
	}
	INLINE friend Vector2D operator - (const Vector2D& vector, float value)
	{
		return Vector2D(vector.x - value, vector.y - value);
	}
	INLINE friend Vector2D operator * (const Vector2D& vector, float Value)
	{
		return Vector2D(vector.x * Value, vector.y * Value);
	}
	INLINE friend Vector2D operator * (float Value, const Vector2D& vector)
	{
		return Vector2D(vector.x * Value, vector.y * Value);
	}
	INLINE friend Vector2D operator * (const Vector2D& vector1, const Vector2D& vector2)
	{
		return Vector2D(vector1.x * vector2.x, vector1.y * vector2.y);
	}
	INLINE friend Vector2D operator / (const Vector2D& vector, float Value)
	{
		return Vector2D(vector.x / Value, vector.y / Value);
	}
	INLINE friend Vector2D operator / (float Value, const Vector2D& vector)
	{
		return Vector2D(Value / vector.x, Value / vector.y);
	}
	// 
	INLINE float Length() const
	{
		return static_cast<float>(LengthSq());
	}
	// 
	INLINE float LengthSq() const
	{
		return x * x + y * y;
	}
	INLINE float GetDistanceFrom(const Vector2D& other) const
	{
		float vx = x - other.x;
		float vy = y - other.y;
		return sqrtf(vx * vx + vy * vy);
	}
	Vector2D& Rotate(float angle)
	{
		return RotateRad(angle);
	}
	// 
	INLINE Vector2D& Normalize()
	{
		float l = Length();
		assert(l && "Zero Vector");
#ifdef _DEBUG
		if (l) {
#endif
		l = 1.0f / l;
		x *= l;
		y *= l;
#ifdef _DEBUG
	}
#endif
	return *this;
	}
	INLINE Vector2D& RotateRad(float angle)
	{
		float cosa = cosf(angle);
		float sina = sinf(angle);
		float X = x * cosa - y * sina;
		float Y = x * sina + y * cosa;
		x = X;
		y = Y;
		return *this;
	}
	INLINE Vector2D RotateDeg(float angle)
	{
		return RotateRad(static_cast<float>(angle * Pi / 180.0f));
	}
	INLINE Vector2D()// : x(0), y(0)
	{
	}
	INLINE Vector2D(const Vector2D &vector) : x(vector.x), y(vector.y)
	{
	}
	INLINE Vector2D(float X, float Y) : x(X), y(Y)
	{
	}
	template<typename T>
	INLINE Vector2D(T X, T Y) : x(static_cast<float>(X)), y(static_cast<float>(Y))
	{
	}
	template<typename T>
	INLINE Vector2D(const T& data)
	{
		assert(sizeof(data) >= sizeof(Vector2D) && "Invalid Data");
		*this = reinterpret_cast<const Vector2D &>(data);
	}
	template<typename T>
	INLINE Vector2D(const T* data)
	{
		assert(sizeof(*data) >= sizeof(Vector2D) && "Invalid Data");
		*this = reinterpret_cast<const Vector2D &>(*data);
	}
	INLINE ~Vector2D() {}
};

// 
INLINE float dotProduct(const Vector2D& vector1, const Vector2D& vector2)
{
	return vector1.x * vector2.x + vector1.y * vector2.y;
}

#endif
