// Quaternion.h: interface for the Quaternion class.
//

#ifndef __QUATERNION_H__
#define __QUATERNION_H__

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef Pi
#define Pi M_PI
#endif

//
//   w+ x*i+y*j+z*k
//   i*j= -j*i = i*k = j*k = -k*j = -1
//

#include "Vector3D.h"

struct D3DXQUATERNION;

class Quaternion {// quaternion
public:
	float x;
	float y;
	float z;
	float w;

	// help function for arcball orthogonal projection on sphere of radius 1, standing in (x = 0, y = 0)
	INLINE static Vector3D OrthoProjectOnSphere(float x, float y)
	{
		Vector3D p(x, y, 0);
		float ls = p.LengthSq();
		if (ls >= 1.0f) {
			p.Normalize();
		}
		else {
			p.z = sqrtf(1.0f - ls);
		}
		return p;
	}
	//
	D3DXQUATERNION* D3D()
	{
		return reinterpret_cast<D3DXQUATERNION *>(this);
	}
	const D3DXQUATERNION* D3D() const
	{
		return reinterpret_cast<const D3DXQUATERNION *>(this);
	}
	INLINE bool operator == (const Quaternion& q) const
	{
		return x == q.x && y == q.y && z == q.z && w == q.w;
	}
	INLINE bool Equal(const Quaternion& q) const
	{
		const float eps = 0.00001f;
		return fabsf(x - q.x) < eps && fabsf(y - q.y) < eps && fabsf(z - q.z) < eps && fabsf(w - q.w) < eps;
	}
	INLINE Quaternion& operator = (const Quaternion& q)
	{
		assert(this != &q && "Appropriation to itself");
		x = q.x;
		y = q.y;
		z = q.z;
		w = q.w;
		return *this;
	}
	INLINE operator const float * () const
	{
		return &x;
	}
	INLINE float& operator [] (int index)
	{
		assert(index >= 0 && "Out of Range");
		assert(index < 4 && "Out of Range");
		return *(&x + index);
	}
	INLINE const float& operator [] (int index) const
	{
		assert(index >= 0 && "Out of Range");
		assert(index < 4 && "Out of Range");
		return *(&x + index);
	}
	INLINE Quaternion& operator /= (const Quaternion& q)
	{
		x /= q.x;
		y /= q.y;
		z /= q.z;
		w /= q.w;
		return *this;
	}
	INLINE Quaternion& operator /= (float value)
	{
		assert(value != 0.0f && "Invalid Value");
		value = 1.0f / value;
		x *= value;
		y *= value;
		z *= value;
		w *= value;
		return *this;
	}
	INLINE Quaternion& operator *= (float value)
	{
		x *= value;
		y *= value;
		z *= value;
		w *= value;
		return *this;
	}
	INLINE Quaternion& operator *= (const Quaternion& q)
	{
		*this = *this * q;
		return *this;
	}
	Quaternion& LoadIdentity()
	{
		x = 0.0f;
		y = 0.0f;
		z = 0.0f;
		w = 1.0f;
		return *this;
	}
	bool IsIdentity() const
	{
		return !x && !y && !z;
	}
	//
	INLINE float Length() const
	{
		return sqrtf(x * x + y * y + z * z + w * w);
	}
	//
	INLINE void Normalize()
	{
		// compute magnitude of the quaternion
		float mag = Length();
		assert(mag > 0.0f && "Zero length");
		// normalize it
		float oneOverMag = 1.0f / mag;
		x *= oneOverMag;
		y *= oneOverMag;
		z *= oneOverMag;
		w *= oneOverMag;
	}
	INLINE friend Quaternion operator + (const Quaternion& q1, const Quaternion& q2)
	{
		Quaternion q;
		q.x = q1.x + q2.x;
		q.y = q1.y + q2.y;
		q.z = q1.z + q2.z;
		q.w = q1.w + q2.w;
		return q;
	}
	INLINE friend Quaternion operator - (const Quaternion& q1, const Quaternion& q2)
	{
		Quaternion q;
		q.x = q1.x - q2.x;
		q.y = q1.y - q2.y;
		q.z = q1.z - q2.z;
		q.w = q1.w - q2.w;
		return q;
	}
	//
	INLINE friend Quaternion operator * (const Quaternion& q1, const Quaternion& q2)
	{
#if 1
		// qq' = [ cross(v,v') + wv' + w'v, ww' – dot(v,v')]
		Quaternion q;
		q.x = q2.w * q1.x + q2.x * q1.w + q2.y * q1.z - q2.z * q1.y;
		q.y = q2.w * q1.y - q2.x * q1.z + q2.y * q1.w + q2.z * q1.x;
		q.z = q2.w * q1.z + q2.x * q1.y - q2.y * q1.x + q2.z * q1.w;
		q.w = q2.w * q1.w - q2.x * q1.x - q2.y * q1.y - q2.z * q1.z;
		return q;
#else
		float t0 = (q1.x - q1.y) * (q2.y - q2.x);
		float t1 = (q1.w + q1.z) * (q2.w + q2.z);
		float t2 = (q1.w - q1.z) * (q2.y + q2.x);
		float t3 = (q1.x + q1.y) * (q2.w - q2.z);
		float t4 = (q1.x - q1.z) * (q2.z - q2.y);
		float t5 = (q1.x + q1.z) * (q2.z + q2.y);
		float t6 = (q1.w + q1.y) * (q2.w - q2.x);
		float t7 = (q1.w - q1.y) * (q2.w + q2.x);

		float t8 = t5 + t6 + t7;
		float t9 = (t4 + t8) * 0.5f;
		return Quaternion (t3 + t9 - t6, t2 + t9 - t7, t1 + t9 - t8, t0 + t9 - t5);
#endif
	}
	//
	INLINE friend Vector3D operator * (const Vector3D& v, const Quaternion& q)
	{
		Vector3D V = v;
		RotateVector(V, q);
		return V;
	}
	//
	INLINE friend Vector3D operator * (const Quaternion& q, const Vector3D& v)
	{
		return v * q;
	}
	//
	INLINE void RotateAroundX(float a)
	{
		a *= 0.5f;
		float c = cosf(a);
		float s = sinf(a);
		x = x * c + w * s;
		y = y * c + z * s;
		z = z * c - y * s;
		w = w * c - x * s;
	}
	//
	INLINE void RotateAroundY(float a)
	{
		a *= 0.5f;
		float c = cosf(a);
		float s = sinf(a);
		x = x * c + z * s;
		y = y * c + w * s;
		z = z * c + x * s;
		w = w * c - y * s;
	}
	//
	INLINE void RotateAroundZ(float a)
	{
		a *= 0.5f;
		float c = cosf(a);
		float s = sinf(a);
		w = w * c - z * s;
		x = x * c + y * s;
		y = y * c - x * s;
		z = z * c + w * s;
	}
	//
	INLINE void CreateFromAxisAngle(const Vector3D& v, float angle)
	{
		CreateFromAxisAngle(v.x, v.y, v.z, angle);
	}
	//
	INLINE void CreateFromAxisAngle(float X, float Y, float Z, float angle)
	{
		// This function takes an angle and an axis of rotation, then converts
		// it to a quaternion.  An example of an axis and angle is what we pass into
		// glRotatef().  That is an axis angle rotation.  It is assumed an angle in
		// degrees is being passed in.  Instead of using glRotatef(), we can now handle
		// the rotations our self.
		// The equations for axis angle to quaternions are such:
		// w = cos( theta / 2 )
		// x = X * sin( theta / 2 )
		// y = Y * sin( theta / 2 )
		// z = Z * sin( theta / 2 )
		// First we want to convert the degrees to radians
		// since the angle is assumed to be in radians
		angle *= 0.5f;
		// Here we calculate the sin( theta / 2) once for optimization
		float result = sinf(angle);
		// Calculate the w value by cos( theta / 2 )
		w = cosf(angle);

		// Calculate the x, y and z of the quaternion
		x = X * result;
		y = Y * result;
		z = Z * result;
		Normalize();
	}
	//
	INLINE void Conjugate()
	{
		x = - x;
		y = - y;
		z = - z;
	}
	//
	INLINE Quaternion& InitWithAngles(float yaw, float pitch, float roll)
	{
		yaw   *= 0.5f;
		pitch *= 0.5f;
		roll  *= 0.5f;
		float cr = cosf(yaw);
		float cp = cosf(pitch);
		float cy = cosf(roll);

		float sr = sinf(yaw);
		float sp = sinf(pitch);
		float sy = sinf(roll);

		float cpcy = cp * cy;
		float cpsy = cp * sy;
		float spcy = sp * cy;
		float spsy = sp * sy;

		w = cr * cpcy + sr * spsy;

		x = cr * spcy + sr * cpsy;
		y = sr * cpcy - cr * spsy;
		z = cr * cpsy - sr * spcy;

		return *this;
	}
	//
	INLINE Quaternion& InitWithAngles(float yaw, float pitch)
	{
		yaw *= 0.5f;
		pitch *= 0.5f;

		float cr = cosf(yaw);
		float cp = cosf(pitch);

		float sr = sinf(yaw);
		float sp = sinf(pitch);

		w = cr * cp;

		x = cr * sp;
		y = sr * cp;
		z = - sr * sp;

		return *this;
	}
	//
	INLINE void QuaternionFromMatrix4x3(const float* m)
	{
		// 0 1 2 3
		// 4 5 6 7
		// 8 9 10 11

		// 0 4 8 12
		// 1 5 9 13
		// 2 6 10 14
		// 3 7 11 15
		assert(m && "NULL Pointer");
		assert(m && "NULL Pointer");
		float trace = m[0] + m[5] + m[10] + 1.0f;
		if (trace > 0.0f) {
			float sqrtTr = sqrtf(trace);
			float InvSwoSqrtTr = 1.0f / (2.0f * sqrtTr);
			x = (m[9] - m[6]) * InvSwoSqrtTr;
			y = (m[2] - m[8]) * InvSwoSqrtTr;
			z = (m[4] - m[1]) * InvSwoSqrtTr;
			w = sqrtTr / 2.0f;
			return;
		}
		int maxi = 0;
		float maxdiag = m[0];	
		for (int i = 5; i < 10; i += 5) {
			if (m[i] > maxdiag) {
				maxi = i;
				maxdiag = m[i];
			}
		}
		switch (maxi) {
			case 0: {	
				float S = 2.0f * sqrtf(1.0f + m[0] - m[5] - m[10]);
				x = 0.25f * S;
				assert(S && "Invalid Value");
				S = 1.0f / S; 
				y = (m[4] + m[1]) * S;
				z = (m[8] + m[2]) * S;
				w = (m[9] - m[6]) * S;
			}
			break;
			case 1: {	
				float S = 2.0f * sqrtf(1.0f + m[5] - m[0] - m[10]);
				y = 0.25f * S;
				assert(S && "Invalid Value");
				S = 1.0f / S; 
				x = (m[4] + m[1]) * S;
				z = (m[9] + m[6]) * S;
				w = (m[2] - m[8]) * S;
			}
			break;
			case 2: {
				float S = 2.0f * sqrtf(1.0f + m[10] - m[0] - m[5]);
				z = 0.25f * S;
				assert(S && "Invalid Value");
				S = 1.0f / S;
				x = (m[8] + m[2]) * S;
				y = (m[9] + m[6]) * S;
				w = (m[4] - m[1]) * S;
			}
			break;
		}
	}
	//
	INLINE void QuaternionFromMatrix(const float* m)
	{
		assert(m && "NULL Pointer");
		float trace = m[0] + m[5] + m[10] + 1.0f;
		if (trace > 0.0f) {
			float sqrtTr = sqrtf(trace);
			float InvSwoSqrtTr = 1.0f / (2.0f * sqrtTr);
			x = (m[6] - m[9]) * InvSwoSqrtTr;
			y = (m[8] - m[2]) * InvSwoSqrtTr;
			z = (m[1] - m[4]) * InvSwoSqrtTr;
			w = sqrtTr / 2.0f;
			return;
		}
		int maxi = 0;
		float maxdiag = m[0];
		for (int i = 5; i < 15; i += 5) {
			if (m[i] > maxdiag) {
				maxi = i;
				maxdiag = m[i];
			}
		}
		switch (maxi) {
			case 0: {
				float S = 2.0f * sqrtf(1.0f + m[0] - m[5] - m[10]);
				x = 0.25f * S;
				assert(S && "Invalid Value");
				S = 1.0f / S; 
				y = (m[1] + m[4]) * S;
				z = (m[2] + m[8]) * S;
				w = (m[6] - m[9]) * S;
			}
			break;
			case 1: {	
				float S = 2.0f * sqrtf(1.0f + m[5] - m[0] - m[10]);
				y = 0.25f * S;
				assert(S && "Invalid Value");
				S = 1.0f / S; 
				x = (m[1] + m[4]) * S;
				z = (m[6] + m[9]) * S;
				w = (m[8] - m[2]) * S;
			}
			break;
			case 2: {
				float S = 2.0f * sqrtf(1.0f + m[10] - m[0] - m[5]);
				z = 0.25f * S;
				assert(S && "Invalid Value");
				S = 1.0f / S;
				x = (m[2] + m[8]) * S;
				y = (m[6] + m[9]) * S;
				w = (m[1] - m[4]) * S;
			}
			break;
		}
	}
	//
	INLINE void Set(float X, float Y, float Z, float W)
	{
		x = X;
		y = Y;
		z = Z;
		w = W;
	}
	//
	INLINE friend Vector3D QuaternionMultiply(const Quaternion& q, const Vector3D& v)
	{
		// nVidia SDK implementation
		Vector3D uv, uuv;
		Vector3D qvec(q.x, q.y, q.z);
		uv = crossProduct(qvec, v);
		uuv = crossProduct(qvec, uv);
		uv *= (2.0f * q.w);
		uuv *= 2.0f;
		return v + uv + uuv;
	}	
	//
	INLINE friend float dotProduct(const Quaternion& q1, const Quaternion& q2)
	{
		return q1.x * q2.x + q1.y * q2.y + q1.z * q2.z + q1.w * q2.w;
	}	
	INLINE friend void QuaternionMultiplyVector(const Quaternion& q, const Vector3D& v, Quaternion& out)
	{
		out.w = - q.x * v.x - q.y * v.y - q.z * v.z;
		out.x =   q.w * v.x + q.y * v.z - q.z * v.y;
		out.y =   q.w * v.y + q.z * v.x - q.x * v.z;
		out.z =   q.w * v.z + q.x * v.y - q.y * v.x;
	}
	//
	INLINE friend void QuaternionComputeW(Quaternion& q)
	{
		float t = 1.0f - q.x * q.x - q.y * q.y - q.z * q.z;
		if (t < 0.0f) {
			q.w = 0.0f;
		}
		else {
			q.w = -sqrtf(t);
		}
	}
	//
	INLINE friend void QuaternionRotatePoint(const Quaternion& q, const Vector3D& in, Vector3D& out)
	{
		Quaternion tmp;
		Quaternion inv;
		Quaternion final;
		inv.x = -q.x;
		inv.y = -q.y;
		inv.z = -q.z;
		inv.w = q.w;
		inv.Normalize();
		QuaternionMultiplyVector(q, in, tmp);
		// Quat_multQuat (tmp, inv, final);
		final = tmp * inv;
		out.x = final.x;
		out.y = final.y;
		out.z = final.z;
	}
	//
	INLINE friend Vector3D RotateVector(Vector3D& v, const Quaternion& q)
	{
		// Quaternion	p(v);
		// Quaternion	qConj(-x, -y, -z, w);
		// p = *this * p * qConj; // optimization p.w == 0
		// Vector3D(p.x, p.y, p.z);

		Quaternion r(v.x * q.w + v.z * q.y - v.y * q.z, v.y * q.w + v.x * q.z - v.z * q.x,
			v.z * q.w + v.y * q.x - v.x * q.y, v.x * q.x + v.y * q.y + v.z * q.z);
		v.x = q.w * r.x + q.x * r.w + q.y * r.z - q.z * r.y;
		v.y = q.w * r.y + q.y * r.w + q.z * r.x - q.x * r.z;
		v.z = q.w * r.z + q.z * r.w + q.x * r.y - q.y * r.x;
		return v;
	}
	// spherical linear interpolation
	INLINE friend void Slerp(const Quaternion& qa, const Quaternion& qb, float t, Quaternion& out)
	{
		/* check for out-of range parameter and return edge points if so */
		if (t <= 0.0) {
			out = qa;
			return;
		}
		if (t >= 1.0) {
			out = qb;
			return;
		}
		/* compute "cosine of angle between quaternions" using dot product */
		float cosOmega = dotProduct(qa, qb);
		/* if negative dot, use -q1.  two quaternions q and -q
		 represent the same rotation, but may produce
		 different slerp.  we chose q or -q to rotate using
		 the acute angle. */
		float q1w = qb.w;
		float q1x = qb.x;
		float q1y = qb.y;
		float q1z = qb.z;
		if (cosOmega < 0.0f) {
			q1w = -q1w;
			q1x = -q1x;
			q1y = -q1y;
			q1z = -q1z;
			cosOmega = -cosOmega;
		}
		/* we should have two unit quaternions, so dot should be <= 1.0 */
		assert (cosOmega < 1.1f);
		/* compute interpolation fraction, checking for quaternions
		 almost exactly the same */
		float k0, k1;
		if (cosOmega > 0.9999f) {
			/* very close - just use linear interpolation,
			 which will protect againt a divide by zero */
			k0 = 1.0f - t;
			k1 = t;
		}
		else {
			/* compute the sin of the angle using the
			 trig identity sin^2(omega) + cos^2(omega) = 1 */
			float sinOmega = sqrtf(1.0f - (cosOmega * cosOmega));
			/* compute the angle from its sin and cosine */
			float omega = atan2f(sinOmega, cosOmega);
			/* compute inverse of denominator, so we only have
			 to divide once */
			float oneOverSinOmega = 1.0f / sinOmega;
			/* Compute interpolation parameters */
			k0 = sinf((1.0f - t) * omega) * oneOverSinOmega;
			k1 = sinf(t * omega) * oneOverSinOmega;
		}
		/* interpolate and return new quaternion */
		out.w = (k0 * qa[3]) + (k1 * q1w);
		out.x = (k0 * qa[0]) + (k1 * q1x);
		out.y = (k0 * qa[1]) + (k1 * q1y);
		out.z = (k0 * qa[2]) + (k1 * q1z);
	}
	//
	INLINE Quaternion(const Vector3D& v) : x(v.x), y(v.y), z(v.z), w(0.0f)
	{
	}
	INLINE Quaternion(const Vector3D& vec, float W): x(vec.x), y(vec.y), z(vec.z), w(W)
	{
	}
	INLINE Quaternion(float X, float Y, float Z, float W) : x (X), y(Y), z(Z), w(W)
	{
	}
	INLINE Quaternion(const Quaternion& q) : x(q.x), y(q.y), z(q.z), w(q.w)
	{
	}
	INLINE Quaternion(float yaw, float pitch, float roll)
	{
		InitWithAngles(yaw, pitch, roll);
	}
	INLINE Quaternion(float yaw, float pitch)
	{
		InitWithAngles(yaw, pitch);
	}
	INLINE Quaternion()
	{
	}
};

// shortest arc quaternion rotate one vector to another by shortest path.
// create rotation from -> to, for any length vectors.
INLINE static Quaternion ShortestArc(const Vector3D& from, const Vector3D& to)
{
	Vector3D c = crossProduct(from, to);
	Quaternion q(c.x, c.y, c.z, dotProduct(from, to));
	q.Normalize();	// if "from" or "to" not unit, normalize quat
	q.w += 1.0f;	  // reducing angle to halfangle
	const float TINY = 0.001f;
	if (q.w <= TINY) {// angle close to PI
		if (from.z * from.z > from.x * from.x) {
			q = Quaternion(0.0f, from.z, - from.y, q.w); //from*vector3(1,0,0) 
		}
		else {
			q = Quaternion(from.y, - from.x, 0.0f, q.w); //from*vector3(0,0,1) 
		}
		q.Normalize(); 
	}
	return q;
}

// calculate rotation of arcball user input, used to perform object rotation by mouse.
// "from" and "to" the mouse on screen coordinates (with - x, y on screen)  in range of -1 to +1 
// the arcball radius is 1.0 and it stand in a center (x = 0, y = 0)
INLINE static Quaternion ArcBall(const Vector3D& from, const Vector3D& to)
{
	Vector3D p_f = Quaternion::OrthoProjectOnSphere(from.x, from.y );
	Vector3D p_t = Quaternion::OrthoProjectOnSphere(to.x, to.y );
	Vector3D c   = crossProduct(p_f, p_t);
	float d = dotProduct(p_f, p_t);
	return Quaternion(c.x, c.y, c.z, d);
}

// rotate as a half arcball, more realistic manner (like 3dmax).
INLINE static Quaternion TrackBall(const Vector3D& from, const Vector3D& to )
{
	Quaternion rot = ShortestArc(Quaternion::OrthoProjectOnSphere(from.x, from.y), 
		Quaternion::OrthoProjectOnSphere(to.x, to.y));
	return rot;
}

//
INLINE Quaternion GetRotationTo(const Vector3D& source, const Vector3D& dest)
{
	Quaternion qidentity(0.0f, 0.0f, 0.0f, 1.0f);
	// Copy, since cannot modify local
	Vector3D v0 = source;
	Vector3D v1 = dest;
	v0.Normalize();
	v1.Normalize();
	Vector3D c = v0 * v1;

	float d = dotProduct(v0, v1);
	// If dot == 1, vectors are the same
	if (d >= 1.0f) {
		return qidentity;
	}
	Quaternion q;
	float s = sqrtf((1.0f + d) * 2.0f);
	if (s < 1e-6f) {
		// Generate an axis
		Vector3D axis = Vector3D(1.0f, 0.0f, 0.0f) * source;
		// pick another if colinear
		if (!axis.Length()) {
			axis = Vector3D(0.0f, 1.0f, 0.0f) * source;
		}
		axis.Normalize();
		const float PI_value = 3.14159265f;
		q.CreateFromAxisAngle(axis, PI_value);
	}
	else {
		float invs = 1.0f / s;
		q.x = c.x * invs;
		q.y = c.y * invs;
		q.z = c.z * invs;
		q.w = s * 0.5f;
	}
	return q;
}

#endif // __QUATERNION_H__
