// Matrix4x4.h: interface for the Matrix4x4 class.
//
//////////////////////////////////////////////////////////////////////

#ifndef __MATRIX4X4_H__
#define __MATRIX4X4_H__

#include "Vector3D.h"
#include "Vector4D.h"
#include <cstring>
#include <climits>

#ifndef Pi
#define Pi static_cast<float>(3.14159265358979323846)
#endif

//
#define degrad static_cast<float>(Pi) / 180.0f
#define EPS 1e-3f

struct D3DXMATRIX;

/*
	a d h l
	b e i m
	c f j n
	p q r s

	a d h
	b e i
	c f j - rotation + scale maxtrix
	l
	m
	n - translate
	[p,q,r] - perspective
	s 

*/
class ALIGN16 Matrix4x4 {
private:
	//
	INLINE float det2x2(float a1, float a2, float b1, float b2)
	{
		return a1 * b2 - b1 * a2;
	}
	//
	INLINE float det3x3(float a1, float a2, float a3,
								float b1, float b2, float b3,
								float c1, float c2, float c3)
	{
		return a1 * det2x2(b2, b3, c2, c3) - b1 * det2x2(a2, a3, c2, c3) + c1 * det2x2(a2, a3, b2, b3);
	}
	public:
#ifdef SSE
	union {
		struct {
			__m128 m0;
			__m128 m1;
			__m128 m2;
			__m128 m3;
		};
#else
	union {
#endif
		float Matrix[16];
		struct {
			float x[3];
			float wx;
			float y[3];
			float wy;
			float z[3];
			float wz;
		};
		struct {
			float _11, _12, _13, _14;
			float _21, _22, _23, _24;
			float _31, _32, _33, _34;
			float _41, _42, _43, _44;
		};
		struct {
			float m[4][4];
		};
	};
	INLINE Matrix4x4(const float* mat)
	{
		assert(mat && "NULL Pointer");
		memcpy(Matrix, mat, sizeof(Matrix4x4));
	}
	INLINE Matrix4x4(float a)
	{
		Matrix[4] = Matrix[8] = Matrix[12] =
		Matrix[1] = Matrix[9] = Matrix[13] = Matrix[2] =
		Matrix[6] = Matrix[14] = Matrix[3] = Matrix[7] =
		Matrix[11] = 0;
		Matrix[0] = Matrix[5] = Matrix[10] = Matrix[15] = a;
	}
	INLINE D3DXMATRIX* D3D()
	{
		return reinterpret_cast<D3DXMATRIX *>(Matrix);
	}
	INLINE const D3DXMATRIX* D3D () const
	{
		return reinterpret_cast<const D3DXMATRIX *>(Matrix);
	}
	INLINE operator float * ()
	{
		return &Matrix[0];
	}
	INLINE operator const float * () const
	{
		return &Matrix[0];
	}
	INLINE operator float & ()
	{
		return Matrix[0];
	}
	INLINE operator const float & () const
	{
		return Matrix[0];
	}
	INLINE float& operator[](int i)
	{
		assert(i >= 0 && i < 16 && "Out of Range");
		return Matrix[i];
	}
	INLINE const float& operator[](int i) const
	{
		assert(i >= 0 && i < 16 && "Out of Range");
		return Matrix[i];
	}
	INLINE bool IsIdentity() const
	{
		if (!Matrix[0] || !Matrix[5] || !Matrix[10] || !Matrix[15]) {
			return false;
		}
		if (Matrix[1] || Matrix[2] || Matrix[3] || Matrix[4]) {
			return false;
		}
		if (Matrix[6] || Matrix[7] || Matrix[8] || Matrix[9]) {
			return false;
		}
		if (Matrix[11] || Matrix[12] || Matrix[13] || Matrix[14]) {
			return false;
		}
		return true;
	}
	// 
	INLINE Matrix4x4 Identity()
	{
		return Matrix4x4(1.f);
	}
	// 
	INLINE Matrix4x4& Scale(float x, float y, float z)
	{
		Matrix[0] = x;
		Matrix[5] = y;
		Matrix[10] = z;
		return *this;
	}
	INLINE Matrix4x4& Scale(float scale)
	{
		return Scale(scale, scale, scale);
	}
	// Rotation 0X axis
	// [ 1	0	  0	0 ]
	// [ 0  cos a  -sin a  0 ]
	// [ 0	sin a  cos a  0 ]
	// [ 0	0	  0	1 ]
	void RotateX(float angle)
	{
		float Cos = cosf(angle);
		float Sin = sinf(angle);
		Matrix[5] = Cos;
		Matrix[9] = -Sin;
		Matrix[6] = Sin;
		Matrix[10] = Cos;
	}
	// Rotation 0Y axis
	// [ cos a  0 sin a  0 ]
	// [   0	1	0	0 ]
	// [ -sin a  0  cos a  0 ]
	// [   0	0	0	1 ]
	void RotateY(float angle)
	{
		float Cos = cosf(angle);
		float Sin = sinf(angle);
		Matrix[0] = Cos;
		Matrix[8] = Sin;
		Matrix[2] = -Sin;
		Matrix[10] = Cos;
	}
	// Rotation 0Z axis
	// [ cos a  -sin a  0  0 ]
	// [ sin a  cos a  0  0 ]
	// [   0	  0	1  0 ]
	// [   0	  0	0  1 ]
	void RotateZ(float angle)
	{
		float Cos = cosf(angle);
		float Sin = sinf(angle);
		Matrix[0] = Cos;
		Matrix[4] = -Sin;
		Matrix[1] = Sin;
		Matrix[5] = Cos;
	}
	//
	void ClearRotate()
	{
		Matrix[0] = 1.0f;
		Matrix[1] = 0.0f;
		Matrix[2] = 0.0f;
		Matrix[4] = 0.0f;
		Matrix[5] = 1.0f;
		Matrix[6] = 0.0f;
		Matrix[8] = 0.0f;
		Matrix[9] = 0.0f;
		Matrix[10] = 1.0f;
	}
	//
	INLINE void RotateYawPitchRollMatrix(float pitch, float yaw, float roll)
	{
		Matrix4x4 matPitch;
		Matrix4x4 matYaw;
		Matrix4x4 matRoll;
		matPitch.RotateX(pitch);
		matYaw.RotateX(yaw);
		matRoll.RotateX(roll);
		*this = matPitch * matYaw * matRoll;
	}
	//
	INLINE void MatrixRotationAxis(const Vector3D& v, float angle)
	{
		float Cos = cosf(angle);
		float Sin  = sinf(angle);
		float t = 1.0f - Cos;
		float tx = t * v.x;
		float ty = t * v.y;
		float tz = t * v.z;
		float tzy = ty * v.z;
		float txy = tx * v.y;
		float txz = tx * v.z;
		float sx = Sin * v.x;
		float sy = Sin * v.y;
		float sz = Sin * v.z;
		Matrix[0] = tx * v.x + Cos;
		Matrix[4] = txy - sz;
		Matrix[8] = txz + sy;
		Matrix[1] = txy + sz;
		Matrix[5] = ty * v.y + Cos;
		Matrix[9] = tzy - sx;
		Matrix[2] = txz - sy;
		Matrix[6] = tzy + sx;
		Matrix[10] = tz * v.z + Cos;
	}
	INLINE Matrix4x4& FromQuaternion(const float* quat)
	{
		assert(quat && "NULL Pointer");
		struct Quat {
			float x;
			float y;
			float z;
			float w;
		};
		const Quat& q = *reinterpret_cast<const Quat *>(quat);
		float x2 = q.x + q.x;
		float y2 = q.y + q.y;
		float z2 = q.z + q.z;
		float xx = q.x * x2;
		float yy = q.y * y2;
		float zz = q.z * z2;
		float xy = q.x * y2;
		float yz = q.y * z2;
		float xz = q.z * x2;
		float wx = q.w * x2;
		float wy = q.w * y2;
		float wz = q.w * z2;
		Matrix[0] = 1.0f - (yy + zz);
		Matrix[4] = xy - wz;
		Matrix[8] = xz + wy;
		Matrix[1] = xy + wz;
		Matrix[5] = 1.0f - (xx + zz);
		Matrix[9] = yz - wx;
		Matrix[2] = xz - wy;
		Matrix[6] = yz + wx;
		Matrix[10] = 1.0f - (xx + yy);
		Matrix[15] = 1.0f;
		return *this;
	}
	//
	INLINE void RotateMatrix(const Vector3D& v, float angle)
	{
		MatrixRotationAxis(v, degrad * angle);
	}
	// 
	INLINE void RotateXYZ(float pitch, float yaw, float roll)
	{
		float CosPitch = cosf(pitch);
		float SinPitch = sinf(pitch);
		float CosYaw = cosf(yaw);
		float SinYaw = sinf(yaw);
		float CosRoll = cosf(roll);
		float SinRoll = sinf(roll);
		Matrix[0] =  CosYaw*CosRoll;
		Matrix[4] =	SinRoll*CosYaw;
		Matrix[8] =  -SinYaw;
		Matrix[12] =  0.0f;
		Matrix[1] =  SinPitch * SinYaw * CosRoll - SinRoll * CosPitch;
		Matrix[5] =  SinPitch * SinYaw * SinRoll + CosPitch * CosRoll;
		Matrix[9] =  -SinPitch * CosYaw;
		Matrix[13] =  0.0f;

		Matrix[2] =	SinYaw * CosPitch * CosRoll + SinPitch * SinRoll;
		Matrix[6] =  SinRoll * SinYaw * CosPitch - CosRoll * SinPitch;
		Matrix[10] =  CosPitch * CosYaw;
		Matrix[14] =  0.0f;


		Matrix[3] =  0.0f;
		Matrix[7] =  0.0f;
		Matrix[11] =  0.0f;
		Matrix[15] =  1.0f;
	}
	//
	INLINE Matrix4x4 MirrorX()
	{
		Matrix4x4 a(1.0f);
		a[0] = - 1.0f;
		return a;
	}
	//
	INLINE Matrix4x4 MirrorY()
	{
		Matrix4x4 a(1.0f);
		a[5] = - 1.0f;

		return a;
	}
	//
	INLINE Matrix4x4 MirrorZ()
	{
		Matrix4x4 a(1.0f);
		a[10] = - 1.0f;
		return a;
	}
	//
	INLINE void Translate(float x, float y, float z)
	{
		Matrix[12] = x;
		Matrix[13] = y;
		Matrix[14] = z;
		Matrix[15] = 1.0f;
	}
	//
	INLINE void Translate(const Vector3D& v)
	{
		Translate(v.x, v.y, v.z);
	}
	//
	INLINE Vector3D GetRollPitchYaw() const
	{
		Vector3D angles;
		GetRollPitchYaw(angles.x, angles.y, angles.z);
		return angles;
	}
	//
	INLINE void GetRollPitchYaw(float& pitch, float& yaw, float& roll) const
	{
		float D = asinf(Matrix[8]);  // Calculate Y-axis angle
		float C = cosf(D);

		yaw = D;

		if (fabs(C) > EPS) {
			float invC = 1.0f / C;
			float x = Matrix[10] * invC;	// x = cos x * cos x / cos x
			float y = -Matrix[9] * invC;	// y = sin x * cos x / cos x
			pitch = atan2f(y, x);			// atan2f(y, x) = atanf(y / x)
			x = Matrix[0] * invC;
			y = -Matrix[4] * invC;
			roll = atan2f(y, x);
		}
		else {
			pitch = 0.0f;				// Set X-axis angle to zero
			float x = Matrix[0];	  // And calculate Z-axis angle
			float y = -Matrix[4];
			roll = atan2f(y, x);
		}
	}
	//
	INLINE Matrix4x4 InvertRotTrans(const Matrix4x4& M)
	{
		return Matrix4x4(
			// Transpoze rotating
			M[0], M[4], M[8], 0.0f,
			M[1], M[5], M[9], 0.0f,
			M[2], M[6], M[10], 0.0f,
			// Translation - minus moving's and rotatin's point 
			-(M[12]*M[0]) - (M[13]*M[1]) - (M[14]*M[2]),
			-(M[12]*M[4]) - (M[13]*M[5]) - (M[14]*M[6]),
			-(M[12]*M[8]) - (M[13]*M[9]) - (M[14]*M[10]),
			1.0f
		);
	}
	INLINE Matrix4x4(const Matrix4x4& a)
	{
		*this = a;
	}
	//
	INLINE Matrix4x4& operator = (const Matrix4x4& a)
	{
		assert(this != &a && "Apropriation to itself");
		memcpy(Matrix, a.Matrix, sizeof(Matrix));
		return *this;
	}
	template<typename MatrixType>
	INLINE Matrix4x4& operator = (const MatrixType& mat)
	{
		assert(sizeof(Matrix4x4) <= sizeof(mat) && "Invalid Size");
		memcpy(Matrix, &mat, sizeof(Matrix));
		return *this;
	}
	//
	INLINE friend Matrix4x4 operator * (const Matrix4x4& A, const Matrix4x4& B)
	{
		Matrix4x4 C;
#ifdef SSE
		assert(!(reinterpret_cast<size_t>(&C) & 15) && "Not Aligned memory");
		assert(!(reinterpret_cast<size_t>(&A) & 15) && "Not Aligned memory");
		assert(!(reinterpret_cast<size_t>(&B) & 15) && "Not Aligned memory");

		C.m0 = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(B.m0, B.m0, 0x00), A.m0),

		_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(B.m0, B.m0, 0x55), A.m1),

		_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(B.m0, B.m0, 0xAA), A.m2),

		_mm_mul_ps(_mm_shuffle_ps(B.m0, B.m0, 0xFF), A.m3))));

		C.m1 = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(B.m1, B.m1, 0x00), A.m0),

		_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(B.m1, B.m1, 0x55), A.m1),

		_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(B.m1, B.m1, 0xAA), A.m2),

		_mm_mul_ps(_mm_shuffle_ps(B.m1, B.m1,0xFF), A.m3))));


		C.m2 = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(B.m2, B.m2, 0x00), A.m0),

		_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(B.m2, B.m2, 0x55), A.m1),

		_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(B.m2, B.m2, 0xAA), A.m2),

		_mm_mul_ps(_mm_shuffle_ps(B.m2, B.m2, 0xFF), A.m3))));


		C.m3 = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(B.m3, B.m3, 0x00), A.m0),

		_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(B.m3, B.m3, 0x55), A.m1),

		_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(B.m3, B.m3,0xAA), A.m2),

		_mm_mul_ps(_mm_shuffle_ps(B.m3, B.m3, 0xFF), A.m3))));
#else
		C[0] = A[0] * B[0] + A[4] * B[1] + A[8] * B[2] + A[12] * B[3];
		C[1] = A[1] * B[0] + A[5] * B[1] + A[9] * B[2] + A[13] * B[3];
		C[2] = A[2] * B[0] + A[6] * B[1] + A[10] * B[2] + A[14] * B[3];
		C[3] = A[3] * B[0] + A[7] * B[1] + A[11] * B[2] + A[15] * B[3];
		C[4] = A[0] * B[4] + A[4] * B[5] + A[8] * B[6] + A[12] * B[7];
		C[5] = A[1] * B[4] + A[5] * B[5] + A[9] * B[6] + A[13] * B[7];
		C[6] = A[2] * B[4] + A[6] * B[5] + A[10] * B[6] + A[14] * B[7];
		C[7] = A[3] * B[4] + A[7] * B[5] + A[11] * B[6] + A[15] * B[7];
		C[8] = A[0] * B[8] + A[4] * B[9] + A[8] * B[10] + A[12] * B[11];
		C[9] = A[1] * B[8] + A[5] * B[9] + A[9] * B[10] + A[13] * B[11];
		C[10] = A[2] * B[8] + A[6] * B[9] + A[10] * B[10] + A[14] * B[11];
		C[11] = A[3] * B[8] + A[7] * B[9] + A[11] * B[10] + A[15] * B[11];
		C[12] = A[0] * B[12] + A[4] * B[13] + A[8] * B[14] + A[12] * B[15];
		C[13] = A[1] * B[12] + A[5] * B[13] + A[9] * B[14] + A[13] * B[15];
		C[14] = A[2] * B[12] + A[6] * B[13] + A[10] * B[14] + A[14] * B[15];
		C[15] = A[3] * B[12] + A[7] * B[13] + A[11] * B[14] + A[15] * B[15];
#endif
		return C;
	}
	//
	INLINE Matrix4x4& operator *= (float Value)
	{
		Matrix[0] *= Value;
		Matrix[4] *= Value;
		Matrix[8] *= Value;
		Matrix[12] *= Value;
		Matrix[1] *= Value;
		Matrix[5] *= Value;
		Matrix[9] *= Value;
		Matrix[13] *= Value;
		Matrix[2] *= Value;
		Matrix[6] *= Value;
		Matrix[10] *= Value;
		Matrix[14] *= Value;
		return *this;
	}
	//
	INLINE Matrix4x4& operator /= (float Value)
	{
		return operator *= (1.0f / Value);
	}
	//
	INLINE Matrix4x4& operator += (const Matrix4x4& a)
	{
		Matrix[0] += a[0];
		Matrix[4] += a[4];
		Matrix[8] += a[8];
		Matrix[12] += a[12];
		Matrix[1] += a[1];
		Matrix[5] += a[5];
		Matrix[9] += a[9];
		Matrix[13] += a[13];

		Matrix[2] += a[2];
		Matrix[6] += a[6];
		Matrix[10] += a[10];
		Matrix[14] += a[14];
		Matrix[3] += a[3];
		Matrix[7] += a[7];
		Matrix[11] += a[11];
		Matrix[15] += a[15];
		return *this;
	}
	//
	INLINE Matrix4x4& operator -= (const Matrix4x4& a)
	{
		Matrix[0] -= a[0];
		Matrix[4] -= a[4];
		Matrix[8] -= a[8];
		Matrix[12] -= a[12];

		Matrix[1] -= a[1];
		Matrix[5] -= a[5];
		Matrix[9] -= a[9];
		Matrix[13] -= a[13];

		Matrix[2] -= a[2];
		Matrix[6] -= a[6];
		Matrix[10] -= a[10];
		Matrix[14] -= a[14];

		Matrix[3] -= a[3];
		Matrix[7] -= a[7];
		Matrix[11] -= a[11];
		Matrix[15] -= a[15];

		return *this;
	}
	//
	INLINE Vector4D Vector4Row(int index) const
	{
		assert(index >= 0 && index < 16 && "Out of Range");
		assert(index >= 0 && index + 4 < 16 && "Out of Range");
		assert(index >= 0 && index + 8 < 16 && "Out of Range");
		assert(index >= 0 && index + 12 < 16 && "Out of Range");
		return Vector4D(Matrix[index], Matrix[index + 4], Matrix[index + 8], Matrix[index + 12]);
	}
	//
	INLINE Vector3D Vector3Row(int index) const
	{
		assert(index >= 0 && index < 16 && "Out of Range");
		assert(index >= 0 && index + 4 < 16 && "Out of Range");
		assert(index >= 0 && index + 8 < 16 && "Out of Range");
		return Vector3D(Matrix[index], Matrix[index + 4], Matrix[index + 8]);
	}
	//
	INLINE const Vector4D& Vector4Column(int index) const
	{
		assert(index >= 0 && index < 16 && "Out of Range");
		return *reinterpret_cast<const Vector4D *>(&Matrix[index]);
	}
	//
	INLINE Vector4D& Vector4Column(int index)
	{
		assert(index >= 0 && index < 16  && index + 3 < 16 && "Out of Range");
		return *reinterpret_cast<Vector4D *>(&Matrix[index]);
	}
	//
	INLINE const Vector3D& Vector3Column(int index) const
	{
		assert(index >= 0 && index < 16 && index + 3 < 16 && "Out of Range");
		return *reinterpret_cast<const Vector3D *>(&Matrix[index]);
	}
	//
	INLINE Vector3D& Vector3Column(int index)
	{
		assert(index >= 0 && index < 16 && "Out of Range");
		return *reinterpret_cast<Vector3D *>(&Matrix[index]);
	}
	//
	INLINE Vector3D& GetPosition()
	{
		return Vector3Column(12);
	}
	//
	INLINE const Vector3D& GetPosition() const
	{
		return Vector3Column(12);
	}
	INLINE friend Vector3D operator * (const Matrix4x4& a, const Vector3D& b)
	{
		return Vector3D(a[0] * b.x + a[4] * b.y + a[8] * b.z,
						a[1] * b.x + a[5] * b.y + a[9] * b.z,
						a[2] * b.x + a[6] * b.y + a[10] * b.z);
	}
	INLINE friend Vector4D operator * (const Vector4D& b, const Matrix4x4& a)
	{
		return Vector4D(a[0] * b.x + a[4] * b.y + a[8] * b.z + a[12] * b.w,
				a[1] * b.x + a[5] * b.y + a[9] * b.z + a[13] * b.w,
				a[2] * b.x + a[6] * b.y + a[10] * b.z + a[14] * b.w,
				a[3] * b.x + a[7] * b.y + a[11] * b.z + a[15] * b.w);
	}
	INLINE friend Vector4D operator * (const Matrix4x4& a, const Vector4D& b)
	{
		return b * a;
	}
	INLINE friend Vector3D operator * (const Vector3D& b, const Matrix4x4& a)
	{
		return a * b;
	}
	//
	INLINE friend Matrix4x4 operator + (const Matrix4x4& a, const Matrix4x4& b)
	{
		Matrix4x4 c;
		c = a;
		c += b;
		return c;
	}
	//
	INLINE friend Matrix4x4 operator - (const Matrix4x4& a, const Matrix4x4& b)
	{
		Matrix4x4 c;
		c = a;
		c -= b;
		return c;
	}
	//
	INLINE Matrix4x4& Invert(const Matrix4x4& A)
	{
		Matrix[0] =  det3x3(A[5], A[6], A[7], A[9], A[10], A[11], A[13], A[14], A[15]);
		Matrix[1] = -det3x3(A[1], A[2], A[3], A[9], A[10], A[11], A[13], A[14], A[15]);
		Matrix[2] =  det3x3(A[1], A[2], A[3], A[5], A[6], A[7], A[13], A[14], A[15]);
		Matrix[3] = -det3x3(A[1], A[2], A[3], A[5], A[6], A[7], A[9], A[10], A[11]);

		Matrix[4] = -det3x3(A[4], A[6], A[7], A[8], A[10], A[11], A[12], A[14], A[15]);
		Matrix[5] =  det3x3(A[0], A[2], A[3], A[8], A[10], A[11], A[12], A[14], A[15]);
		Matrix[6] = -det3x3(A[0], A[2], A[3], A[4], A[6], A[7], A[12], A[14], A[15]);
		Matrix[7] =  det3x3(A[0], A[2], A[3], A[4], A[6], A[7], A[8], A[10], A[11]);

		Matrix[8] =  det3x3(A[4], A[5], A[7], A[8], A[9], A[11], A[12], A[13], A[15]);
		Matrix[9] = -det3x3(A[0], A[1], A[3], A[8], A[9], A[11], A[12], A[13], A[15]);
		Matrix[10] =  det3x3(A[0], A[1], A[3], A[4], A[5], A[7], A[12], A[13], A[15]);
		Matrix[11] = -det3x3(A[0], A[1], A[3], A[4], A[5], A[7], A[8], A[9], A[11]);

		Matrix[12] = -det3x3(A[4], A[5], A[6], A[8], A[9], A[10], A[12], A[13], A[14]);
		Matrix[13] =  det3x3(A[0], A[1], A[2], A[8], A[9], A[10], A[12], A[13], A[14]);
		Matrix[14] = -det3x3(A[0], A[1], A[2], A[4], A[5], A[6], A[12], A[13], A[14]);
		Matrix[15] =  det3x3(A[0], A[1], A[2], A[4], A[5], A[6], A[8], A[9], A[10]);

		float det = (A[0] * Matrix[0]) + (A[4] * Matrix[1]) + (A[8] * Matrix[2]) + (A[12] * Matrix[3]);
									// The following divions goes unchecked for division
									// by zero. We should consider throwing Matrixn exception
									// if det < eps.
		float oodet = 1 / det;
		Matrix[0] *= oodet;
		Matrix[1] *= oodet;
		Matrix[2] *= oodet;
		Matrix[3] *= oodet;

		Matrix[4] *= oodet;
		Matrix[5] *= oodet;
		Matrix[6] *= oodet;
		Matrix[7] *= oodet;

		Matrix[8] *= oodet;
		Matrix[9] *= oodet;
		Matrix[10] *= oodet;
		Matrix[11] *= oodet;

		Matrix[12] *= oodet;
		Matrix[13] *= oodet;
		Matrix[14] *= oodet;
		Matrix[15] *= oodet;

		return *this;
	}
	INLINE friend Matrix4x4 Invert(const Matrix4x4& A)
	{
		Matrix4x4 invertA;
		return invertA.Invert(A);
	}
	//
	INLINE Matrix4x4& Invert()
	{
		Matrix4x4 A = *this;
		Invert(A);
		return *this;
	}
	//
	INLINE Matrix4x4& Transpose()
	{
		Matrix4x4 a;

		a[0] = Matrix[0];
		a[4] = Matrix[1];
		a[8] = Matrix[2];
		a[12] = Matrix[3];

		a[1] = Matrix[4];
		a[5] = Matrix[5];
		a[9] = Matrix[6];
		a[13] = Matrix[7];

		a[2] = Matrix[8];
		a[6] = Matrix[9];
		a[10] = Matrix[10];
		a[14] = Matrix[11];

		a[3] = Matrix[12];
		a[7] = Matrix[13];
		a[11] = Matrix[14];
		a[15] = Matrix[15];

		return *this = a;
	}
	// Perspective Matrix for OpenGL
	INLINE void PerspectiveFovOpenGL(float fov, float aspect, float zNear, float zFar)
	{
		assert(fov > 0 && "Invalid Value");
		assert(aspect > 0 && "Invalid Value");
		assert(zFar > zNear && "Invalid Value");
		assert(zNear != zFar && "Invalid Value");
		
		float yScale = 1.0f / tanf(degrad * fov * 0.5f);

		Matrix[0] = yScale / aspect;
		Matrix[1] = 0.0f;
		Matrix[2] = 0.0f;
		Matrix[3] = 0.0f;

		Matrix[4] = 0.0f;
		Matrix[5] = yScale;
		Matrix[6] = 0.0f;
		Matrix[7] = 0.0f;

		Matrix[8] = 0.0f;
		Matrix[9] = 0.0f;
		Matrix[10] = (zFar + zNear) / (zNear - zFar);
		Matrix[11] = -1.0f;

		Matrix[12] = 0.0f;
		Matrix[13] = 0.0f;
		Matrix[14] = 2.0f * zNear * zFar / (zNear - zFar);
		Matrix[15] = 0.0f;
	}
	INLINE void PerspectiveOpenGL(float width, float height, float zNear, float zFar)
	{
		assert(width > 0 && "Invalid Value");
		assert(height > 0 && "Invalid Value");
		assert(zFar > zNear && "Invalid Value");
		assert(zNear != zFar && "Invalid Value");

		Matrix[0] = 2.0f * zNear / width;
		Matrix[1] = 0.0f;
		Matrix[2] = 0.0f;
		Matrix[3] = 0.0f;

		Matrix[4] = 0.0f;
		Matrix[5] = 2.0f * zNear / height;
		Matrix[6] = 0.0f;
		Matrix[7] = 0.0f;

		Matrix[8] = 0.0f;
		Matrix[9] = 0.0f;
		Matrix[10] = (zFar + zNear) / (zNear - zFar);
		Matrix[11] = -1.0f;

		Matrix[12] = 0.0f;
		Matrix[13] = 0.0f;
		Matrix[14] = 2.0f * zNear * zFar / (zNear - zFar);
		Matrix[15] = 0.0f;
	}
	// Perspective Matrix for Direct3D
	INLINE void PerspectiveFovDirect3D(float fov, float aspect, float zNear, float zFar)
	{
		assert(fov > 0 && "Invalid Value");
		assert(aspect > 0 && "Invalid Value");
		assert(zFar > zNear && "Invalid Value");
		assert(zNear != zFar && "Invalid Value");
		
		float yScale = 1.0f / tanf(degrad * fov * 0.5f);

		Matrix[0] = yScale / aspect;
		Matrix[1] = 0.0f;
		Matrix[2] = 0.0f;
		Matrix[3] = 0.0f;

		Matrix[4] = 0.0f;
		Matrix[5] = yScale;
		Matrix[6] = 0.0f;
		Matrix[7] = 0.0f;

		Matrix[8] = 0.0f;
		Matrix[9] = 0.0f;
		Matrix[10] = zFar / (zFar - zNear);
		Matrix[11] = 1.0f;

		Matrix[12] = 0.0f;
		Matrix[13] = 0.0f;
		Matrix[14] = zNear * zFar / (zNear - zFar);
		Matrix[15] = 0.0f;
	}
	INLINE void PerspectiveDirect3D(float width, float height, float zNear, float zFar)
	{
		assert(width > 0 && "Invalid Value");
		assert(height > 0 && "Invalid Value");
		assert(zFar > zNear && "Invalid Value");
		assert(zNear != zFar && "Invalid Value");

		Matrix[0] = 2.0f * zNear / width;
		Matrix[1] = 0.0f;
		Matrix[2] = 0.0f;
		Matrix[3] = 0.0f;

		Matrix[4] = 0.0f;
		Matrix[5] = 2.0f * zNear / height;
		Matrix[6] = 0.0f;
		Matrix[7] = 0.0f;

		Matrix[8] = 0.0f;
		Matrix[9] = 0.0f;
		Matrix[10] = zFar / (zFar - zNear);
		Matrix[11] = 1.0f;

		Matrix[12] = 0.0f;
		Matrix[13] = 0.0f;
		Matrix[14] = zNear * zFar / (zNear - zFar);
		Matrix[15] = 0.0f;
	}
	INLINE void PerspectiveDirect3D(float left, float right, float top, float bottom, float zNear, float zFar)
	{
		float w = right - left;
		float h = top - bottom;
		float znzf = zFar - zNear;

		// 2 * zn / (r - l)		0					0					0
		//	0					2 * zn / (t - b)	0					0
		//	(l + r) / (l - r)	(t + b) / (b - t)	zf / (zf - zn)		1
		//	0					0					zn*zf / (zn - zf)	0

		Matrix[0] = 2.0f * zNear / (right - left);
		Matrix[1] = 0.0f;
		Matrix[2] = -(left + right) / w;
		Matrix[3] = 0.0f;
		Matrix[4] = 0.0f;
		Matrix[5] = 2.0f * zNear / h;
		Matrix[6] = -(top + bottom) / h;
		Matrix[7] = 0.0f;
		Matrix[8] = 0.0f;
		Matrix[9] = 0.0f;
		Matrix[10] = zFar / znzf;
		Matrix[11] = -zNear * zFar / znzf;
		Matrix[12] = 0.0f;
		Matrix[13] = 0.0f;
		Matrix[14] = 1.0f;
		Matrix[15] = 0.0f;
	}
	// Perspective Matrix for Vulkan
	INLINE void PerspectiveFovVulkan(float fov, float aspect, float zNear, float zFar)
	{
		PerspectiveFovOpenGL(fov, aspect, zNear, zFar);
		Matrix[5] = - Matrix[5];
		Matrix[14] *= 0.5f;
	}
	// Ortho Matrix for OpenGL
	INLINE void OrthoOpenGL(float left, float right, float top, float bottom, float zNear, float zFar)
	{
		assert(right != left && "Invalid Value");
		assert(top != bottom && "Invalid Value");
		assert(zNear != zFar && "Invalid Value");
		assert(zFar > zNear && "Invalid Value");
		assert(right > left && "Invalid Value");
		
		float drl = right - left;
		float dtb = top - bottom;
		float dfn = zFar - zNear;

		Matrix[0] = 2.0f / drl;
		Matrix[1] = 0.0f;
		Matrix[2] = 0.0f;
		Matrix[3] = 0.0f;

		Matrix[4] = 0.0f;
		Matrix[5] = 2.0f / dtb;
		Matrix[6] = 0.0f;
		Matrix[7] = 0.0f;

		Matrix[8] = 0.0f;
		Matrix[9] = 0.0f;
		Matrix[10] = -2.0f / dfn;
		Matrix[11] = 0.0f;

		Matrix[12] = -(right + left) / drl;
		Matrix[13] = -(top + bottom) / dtb;
		Matrix[14] = -(zFar + zNear) / dfn;
		Matrix[15] = 1.0f;
	}
	// Ortho Matrix for OpenGL
	INLINE void OrthoOpenGL(float width, float height, float zNear, float zFar)
	{
		assert(width > 0.0f && "Invalid Value");
		assert(height > 0.0f && "Invalid Value");
		assert(zNear != zFar && "Invalid Value");
		assert(zFar > zNear && "Invalid Value");
		
		float dfn = zFar - zNear;

		Matrix[0] = 2.0f / width;
		Matrix[1] = 0.0f;
		Matrix[2] = 0.0f;
		Matrix[3] = 0.0f;

		Matrix[4] = 0.0f;
		Matrix[5] = 2.0f / height;
		Matrix[6] = 0.0f;
		Matrix[7] = 0.0f;

		Matrix[8] = 0.0f;
		Matrix[9] = 0.0f;
		Matrix[10] = -2.0f / dfn;
		Matrix[11] = 0.0f;

		Matrix[12] = 0.0f;
		Matrix[13] = 0.0f;
		Matrix[14] = -(zFar + zNear) / dfn;
		Matrix[15] = 1.0f;
	}
	// Ortho Matrix for Direct3D
	INLINE void OrthoDirect3D(float left, float right, float top, float bottom, float zNear, float zFar)
	{
		assert(right != left && "Invalid Value");
		assert(top != bottom && "Invalid Value");
		assert(zNear != zFar && "Invalid Value");
		assert(zFar > zNear && "Invalid Value");
		assert(right > left && "Invalid Value");
		
		float drl = right - left;
		float dtb = top - bottom;
		float dfn = zFar - zNear;

		Matrix[0] = 2.0f / drl;
		Matrix[1] = 0.0f;
		Matrix[2] = 0.0f;
		Matrix[3] = 0.0f;

		Matrix[4] = 0.0f;
		Matrix[5] = 2.0f / dtb;
		Matrix[6] = 0.0f;
		Matrix[7] = 0.0f;

		Matrix[8] = 0.0f;
		Matrix[9] = 0.0f;
		Matrix[10] = 1.0f / dfn;
		Matrix[11] = 0.0f;

		Matrix[12] = -(right + left) / drl;
		Matrix[13] = -(top + bottom) / dtb;
		Matrix[14] = -zNear / dfn;
		Matrix[15] = 1.0f;
	}
	// Ortho Matrix for Direct3D
	INLINE void OrthoDirect3D(float width, float height, float zNear, float zFar)
	{
		assert(width > 0.0f && "Invalid Value");
		assert(height > 0.0f && "Invalid Value");
		assert(zNear != zFar && "Invalid Value");
		assert(zFar > zNear && "Invalid Value");
		
		float dfn = zFar - zNear;

		Matrix[0] = 2.0f / width;
		Matrix[1] = 0.0f;
		Matrix[2] = 0.0f;
		Matrix[3] = 0.0f;

		Matrix[4] = 0.0f;
		Matrix[5] = 2.0f / height;
		Matrix[6] = 0.0f;
		Matrix[7] = 0.0f;

		Matrix[8] = 0.0f;
		Matrix[9] = 0.0f;
		Matrix[10] = 1.0f / dfn;
		Matrix[11] = 0.0f;

		Matrix[12] = 0.0f;
		Matrix[13] = 0.0f;
		Matrix[14] = -zNear / dfn;
		Matrix[15] = 1.0f;
	}
	// Ortho Matrix for Vulkan
	INLINE void OrthoVulkan(float width, float height, float zNear, float zFar)
	{
		assert(width > 0.0f && "Invalid Value");
		assert(height > 0.0f && "Invalid Value");
		assert(zNear != zFar && "Invalid Value");
		assert(zFar > zNear && "Invalid Value");

		float dnf = zNear - zFar;

		Matrix[0] = 2.0f / width;
		Matrix[1] = 0.0f;
		Matrix[2] = 0.0f;
		Matrix[3] = 0.0f;

		Matrix[4] = 0.0f;
		Matrix[5] = -2.0f / height;
		Matrix[6] = 0.0f;
		Matrix[7] = 0.0f;

		Matrix[8] = 0.0f;
		Matrix[9] = 0.0f;
		Matrix[10] = 1.0f / dnf;
		Matrix[11] = 0.0f;

		Matrix[12] = 0.0f;
		Matrix[13] = 0.0f;
		Matrix[14] = zNear / dnf;
		Matrix[15] = 1.0f;
	}
	// Ortho Matrix for Vulkan
	INLINE void OrthoVulkan(float left, float right, float top, float bottom, float zNear, float zFar)
	{
		assert(right != left && "Invalid Value");
		assert(top != bottom && "Invalid Value");
		assert(zNear != zFar && "Invalid Value");
		assert(zFar > zNear && "Invalid Value");
		assert(right > left && "Invalid Value");

		float drl = right - left;
		float dbt = bottom - top;
		float dnf = zNear - zFar;

		Matrix[0] = 2.0f / drl;
		Matrix[1] = 0.0f;
		Matrix[2] = 0.0f;
		Matrix[3] = 0.0f;

		Matrix[4] = 0.0f;
		Matrix[5] = 2.0f / dbt;
		Matrix[6] = 0.0f;
		Matrix[7] = 0.0f;

		Matrix[8] = 0.0f;
		Matrix[9] = 0.0f;
		Matrix[10] = 1.0f / dnf;
		Matrix[11] = 0.0f;

		Matrix[12] = -(right + left) / drl;
		Matrix[13] = -(bottom + top) / dbt;
		Matrix[14] = zNear / dnf;
		Matrix[15] = 1.0f;
	}
	//
	INLINE void LookAtDirect3D(const Vector3D& eye, const Vector3D& lookat, const Vector3D& up)
	{
		Vector3D zaxis = lookat - eye;
		zaxis.Normalize();
		Vector3D xaxis = crossProduct(up, zaxis);
		Vector3D yaxis = crossProduct(zaxis, xaxis);

		Matrix[0] = xaxis.x;
		Matrix[4] = xaxis.y;
		Matrix[8] = xaxis.z;

		Matrix[1] = yaxis.x;
		Matrix[5] = yaxis.y;
		Matrix[9] = yaxis.z;
		
		Matrix[2] = zaxis.x;
		Matrix[6] = zaxis.y;
		Matrix[10] = zaxis.z;

		Matrix[3] = 0.0f;
		Matrix[7] = 0.0f;
		Matrix[11] = 0.0f;
			
		Matrix[12] = -dotProduct(xaxis, eye);
		Matrix[13] = -dotProduct(yaxis, eye);
		Matrix[14] = -dotProduct(zaxis, eye);
		Matrix[15] = 1.0f;
	}
	INLINE void LookAtVulkan(const Vector3D& eye, const Vector3D& lookat, const Vector3D& up)
	{
		Vector3D zaxis = lookat - eye;
		zaxis.Normalize();
		Vector3D xaxis = crossProduct(zaxis, up);
		Vector3D yaxis = crossProduct(xaxis, zaxis);

		Matrix[0] = xaxis.x;
		Matrix[4] = xaxis.y;
		Matrix[8] = xaxis.z;

		Matrix[1] = yaxis.x;
		Matrix[5] = yaxis.y;
		Matrix[9] = yaxis.z;

		Matrix[2] = -zaxis.x;
		Matrix[6] = -zaxis.y;
		Matrix[10] = -zaxis.z;

		Matrix[3] = 0.0f;
		Matrix[7] = 0.0f;
		Matrix[11] = 0.0f;

		Matrix[12] = -dotProduct(xaxis, eye);
		Matrix[13] = -dotProduct(yaxis, eye);
		Matrix[14] = dotProduct(zaxis, eye);
		Matrix[15] = 1.0f;
	}
	//
	INLINE void LookAtOpenGL(const Vector3D& eye, const Vector3D& lookat, const Vector3D& up)
	{
		Vector3D zaxis = lookat - eye;
		zaxis.Normalize();
		Vector3D xaxis = crossProduct(up, zaxis);
		Vector3D yaxis = crossProduct(zaxis, xaxis);

		Matrix[0] = xaxis.x;
		Matrix[4] = xaxis.y;
		Matrix[8] = xaxis.z;

		Matrix[1] = yaxis.x;
		Matrix[5] = yaxis.y;
		Matrix[9] = yaxis.z;
		
		Matrix[2] = -zaxis.x;
		Matrix[6] = -zaxis.y;		
		Matrix[10] = -zaxis.z;

		Matrix[3] = 0.0f;
		Matrix[7] = 0.0f;		
		Matrix[11] = 0.0f;
			
		Matrix[12] = -dotProduct(xaxis, eye);
		Matrix[13] = -dotProduct(yaxis, eye);
		Matrix[14] = dotProduct(zaxis, eye);
		Matrix[15] = 1.0f;
	}

	INLINE Matrix4x4(float a, float d, float h, float l,
					float b, float e, float i, float m,
					float c, float f, float j, float n,
					float p, float q, float r, float s)
	{
		Matrix[0] = a;Matrix[4] = d; Matrix[8] = h; Matrix[12] =l;
		Matrix[1] = b;Matrix[5] = e; Matrix[9] = i; Matrix[13] = m;
		Matrix[2] = c;Matrix[6] = f; Matrix[10] = j; Matrix[14] = n;
		Matrix[3] = p;Matrix[7] = q; Matrix[11] = r; Matrix[15] = s;
	}
	//
	INLINE void LoadIdentity()
	{
		memset(&Matrix[1], 0, sizeof(Matrix) - 2 * sizeof(float));
		Matrix[0] = Matrix[5] = Matrix[10] = Matrix[15] = 1.0f;
	}
#ifdef SSE
#if 0
	INLINE void* operator new(size_t s)
	{
		if (s > (UINT_MAX - 16)) {
			return NULL;
		}
		char* p = ::new char[s + 16];
		if (p) {
			char offset = static_cast<char>(16 - (reinterpret_cast<intptr_t>(p) & 15));
			p += offset;
			p[-1] = offset;
		}
		return p;
	}
	INLINE void* operator new[](size_t s )
	{
		if (s > (UINT_MAX - 16)) {
			return NULL;
		}
		char* p = ::new char[s + 16];
		if (p)
		{
			char offset = static_cast<char>(16 - (reinterpret_cast<intptr_t>(p) & 15));
			p += offset;
			p[-1] = offset;
		}
		return p;
	}
	INLINE void operator delete(void* p)
	{
		if(p) {
			char* pb = static_cast<char *>(p);
			pb -= pb[-1];
			::delete [] pb;
		}
	}
	INLINE void operator delete[](void* p)
	{
		if(p) {
			char* pb = static_cast<char *>(p);
			pb -= pb[-1];
			::delete [] pb;
		}
	}
#endif
#endif
	INLINE Matrix4x4()
	{
	}
};

#endif
