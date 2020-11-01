// Vector3D.h: interface for the Vector3D class.
//
//////////////////////////////////////////////////////////////////////

#ifndef __VECTOR3D_H__
#define __VECTOR3D_H__

#include <cmath>
#include <cassert>
#include <cstring>

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

struct D3DXVECTOR3;

// Invert of sqrtf
INLINE float InvSqrt(float x)
{
	float xhalf = 0.5f * x;
	int i = *reinterpret_cast<int *>(&x);
	i = 0x5f3759df - (i >> 1);
	x = *reinterpret_cast<float *>(&i);
	x = x * (1.5f - xhalf * x * x);
	return x;
}

class Vector3D { //
private:
	INLINE void Rotate(float& val1, float& val2, float angle) const
	{
		float cosa = cosf(angle);
		float sina = sinf(angle);
		float Val1 = val1 * cosa - val2 * sina;
		float Val2 = val1 * sina + val2 * cosa;
		val1 = Val1;
		val2 = Val2;
	}
public:
	float x;
	float y;
	float z;
	INLINE static const Vector3D Zero()
	{
		return Vector3D(0.0f, 0.0f, 0.0f);
	}
	INLINE static const Vector3D Z()
	{
		return Vector3D(0.0f, 0.0f, 1.0f);
	}
	INLINE static const Vector3D Y()
	{
		return Vector3D(0.0f, 1.0f, 0.0f);
	}
	INLINE static const Vector3D X()
	{
		return Vector3D(1.0f, 0.0f, 0.0f);
	}
	INLINE Vector3D& operator = (const Vector3D& vector3D)
	{
		assert(this != &vector3D && "Apropriation to itself");
		x = vector3D.x;
		y = vector3D.y;
		z = vector3D.z;
		return *this;
	}
	template<typename T>
	INLINE Vector3D& operator = (const T& data)
	{
		*this = reinterpret_cast<const Vector3D& >(data);
		return *this;
	}
	template<typename T>
	INLINE Vector3D& operator = (const T* data)
	{
		assert(data && "NULL Pointer");
		*this = reinterpret_cast<const Vector3D &>(data);
		return *this;
	}
	INLINE D3DXVECTOR3* D3D()
	{
		return reinterpret_cast<D3DXVECTOR3 *>(this);
	}
	INLINE const D3DXVECTOR3* D3D() const
	{
		return reinterpret_cast<const D3DXVECTOR3 *>(this);
	}
	INLINE operator float * ()
	{
		return &x;
	}
	INLINE operator const float * () const
	{
		return &x;
	}
	INLINE Vector3D operator - () const
	{
		return Vector3D(- x, - y, - z);
	}
	INLINE bool operator == (const Vector3D& vector) const
	{
		return (x == vector.x && y == vector.y && z == vector.z);
	}
	INLINE bool operator != (const Vector3D& vector) const
	{
		return (x != vector.x || y != vector.y || z != vector.z);
	}
	INLINE Vector3D& operator += (const Vector3D& Vector)
	{
		x += Vector.x;
 		y += Vector.y;
 		z += Vector.z;
 		return *this;
	}
	INLINE Vector3D& operator -= (const Vector3D& Vector)
	{
		x -= Vector.x;
 		y -= Vector.y;
 		z -= Vector.z;
 		return *this;
	}
	INLINE Vector3D& operator *= (float Value)
	{
		x *= Value;
 		y *= Value;
 		z *= Value;
 		return *this;
	}
	INLINE Vector3D& operator /= (float Value)
	{
		assert(Value && "Zero Value");
		*this *= (1.0f / Value);
		return *this;
	}
	INLINE Vector3D& operator += (float Value)
	{
		x += Value;
		y += Value;
		z += Value;
		return *this;
	}
	INLINE Vector3D& operator -= (float Value)
	{
		x -= Value;
		y -= Value;
		z -= Value;
		return *this;
	}
	INLINE Vector3D& operator /= (const Vector3D& vector)
	{
		x /= vector.x;
 		y /= vector.y;
 		z /= vector.z;
 		return *this;
	}
	INLINE Vector3D& operator *= (const Vector3D& vector)
	{
		x *= vector.x;
		y *= vector.y;
		z *= vector.z;
		return *this;
	}
	//
	INLINE float& operator [](int index)
	{
		assert(index < 3 && "Out Of Range");
		return *(index + (&x));
	}
	INLINE float operator [] (int index) const
	{
		assert(index < 3 && "Out Of Range");
		return * (index + (&x));
	}
	INLINE bool operator > (const Vector3D& v) const
	{
		return x > v.x && y > v.y && z > v.z;
	}
	INLINE bool operator >= (const Vector3D& v) const
	{
		return x >= v.x && y >= v.y && z >= v.z;
	}
	INLINE bool operator < (const Vector3D& v) const
	{
		return x < v.x && y < v.y && z < v.z;
	}
	INLINE bool operator <= (const Vector3D& v) const
	{
		return x <= v.x && y <= v.y && z <= v.z;
	}
	INLINE friend Vector3D operator + (const Vector3D& vector1, const Vector3D& vector2)
	{
		return Vector3D(vector1.x + vector2.x, vector1.y + vector2.y, vector1.z + vector2.z);
	}
	INLINE friend Vector3D operator + (const Vector3D& vector, float value)
	{
		return Vector3D(vector.x + value, vector.y + value, vector.z + value);
	}
	INLINE friend Vector3D operator - (const Vector3D& vector1, const Vector3D& vector2)
	{
		return Vector3D(vector1.x - vector2.x, vector1.y - vector2.y, vector1.z - vector2.z);
	}
	INLINE friend Vector3D operator - (const Vector3D& vector, float value)
	{
		return Vector3D(vector.x - value, vector.y - value, vector.z - value);
	}
	INLINE friend Vector3D operator * (const Vector3D& vector, float Value)
	{
		return Vector3D(vector.x * Value, vector.y * Value, vector.z * Value);
	}
	INLINE friend Vector3D operator * (float Value, const Vector3D& vector)
	{
		return Vector3D(vector.x * Value, vector.y * Value, vector.z * Value);
	}
	//
	INLINE friend Vector3D operator * (const Vector3D& v1, const Vector3D& v2)
	{
		return Vector3D(v1.x * v2.x, v1.y * v2.y, v1.z * v2.z);
	}
	INLINE friend Vector3D operator / (const Vector3D& vector, float Value)
	{
		assert(Value && "Zero Value");
		Value = 1.0f / Value;
		return vector * Value;
	}
	INLINE friend Vector3D operator / (float Value, const Vector3D& vector)
	{
		return Vector3D ( Value / vector.x, Value / vector.y, Value / vector.z );
	}
	// Vector length
	INLINE float Length() const
	{
		return sqrtf(LengthSq());
	}
	// Square of vector length
	INLINE float LengthSq() const
	{
		return x * x + y * y + z * z;
	}
	INLINE float GetDistanceSqFrom(const Vector3D& other) const
	{
		Vector3D d = *this - other;
		float lenSq = d.LengthSq();
		return lenSq;
	}
	INLINE float GetDistanceFrom(const Vector3D& other) const
	{
		float lenSq = GetDistanceSqFrom(other);
		return sqrtf(lenSq);
	}
	//
	INLINE bool IsZero() const
	{
		return !x && !y && !z;
	}
	//
	INLINE Vector3D& Normalize()
	{
		float l = LengthSq();
		//assert(l && "Zero Vector");
		l = InvSqrt(l);
		x *= l;
		y *= l;
		z *= l;
		return *this;
	}
	Vector3D& RotateXZ(float angle)
	{
		Rotate(x, z, angle);
		return *this;
	}
	Vector3D& RotateXY(float angle)
	{
		Rotate(x, y, angle);
		return *this;
	}
	Vector3D& RotateYZ(float angle)
	{
		Rotate(y, z, angle);
		return *this;
	}
	INLINE Vector3D()// : x(0), y(0), z(0)
	{
	}
	INLINE Vector3D(float X, float Y, float Z) : x(X), y(Y), z(Z)
	{
	}
	template<typename T>
	INLINE Vector3D(const T& data)
	{
		*this = reinterpret_cast<const Vector3D &>(data);
	}
	template<typename T>
	INLINE Vector3D(const T* data)
	{
		*this = *reinterpret_cast<const Vector3D *>(data);
	}
	INLINE Vector3D(const Vector3D& vector) : x(vector.x), y(vector.y), z(vector.z)
	{
	}
	INLINE Vector3D(const float* vector) : x(vector[0]), y(vector[1]), z(vector[2])
	{
	}
};

INLINE void CopyVector(Vector3D* outVector, const Vector3D* inVector)
{
	assert(outVector && "NULL Pointer");
	assert(inVector && "NULL Pointer");
	memcpy(outVector, inVector, sizeof(Vector3D));
}

INLINE void CopyVector(Vector3D& outVector, const Vector3D& inVector)
{
	CopyVector(&outVector, &inVector);
}

INLINE void CopyVector(Vector3D* outVector, const Vector3D& inVector)
{
	CopyVector(outVector, &inVector);
}

INLINE void CopyVector(Vector3D& outVector, const Vector3D* inVector)
{
	CopyVector(&outVector, inVector);
}

// 
INLINE float dotProduct(const Vector3D& vector1, const Vector3D& vector2)
{
	return vector1.x * vector2.x + vector1.y * vector2.y + vector1.z * vector2.z;
}

// a x b equal square are created from its vectors
INLINE Vector3D crossProduct(const Vector3D& v1, const Vector3D& v2)
{
	return Vector3D(v1.y * v2.z - v1.z * v2.y, -v1.x * v2.z + v1.z * v2.x, v1.x * v2.y - v1.y * v2.x);		
}

#endif // __VECTOR3D_H__
