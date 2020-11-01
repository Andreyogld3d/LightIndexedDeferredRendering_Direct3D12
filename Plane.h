// Plane.h: interface for the Plane class.
//
//////////////////////////////////////////////////////////////////////

#ifndef __PLANE_H__
#define __PLANE_H__

#include "Vector3D.h"

#define FLT_AS_DW(F) (*reinterpret_cast<unsigned int *>(&(F)))
#define ALMOST_ZERO(F) ((FLT_AS_DW(F) & 0x7f800000L)==0)
#define IS_SPECIAL(F)  ((FLT_AS_DW(F) & 0x7f800000L)==0x7f800000L)

struct D3DXPLANE;

// Plane implementation

class Plane  {
public:
	// normal
	Vector3D normal;
	// distance
	float dist;
private:
	typedef unsigned int uint;
	// 
	INLINE float dotProductPointNormal(float x, float y, float z) const
	{
		return x * normal.x + y * normal.y + z * normal.z;
	}
public:
	// 
	enum {
		FRONT_PLANE = 1, 
		BACK_PLANE = -1, 
		IN_PLANE = 2, 
		INTERSECT_PLANE = 0
	};
	//
	INLINE float GetD() const
	{
		return dist;
	}
	//
	INLINE const Vector3D& GetNormal() const
	{
		return normal;
	}
	//
	INLINE int ClassifyPoint(const Vector3D& point) const
	{
		return ClassifyPoint(point.x, point.y, point.z);
	}
	//
	INLINE int ClassifyPoint(float x, float y, float z) const
	{
		//
		float Dist = dotProductPointNormal(x, y, z) + dist;
		if (Dist > 0) {
			//
			return FRONT_PLANE;
		}
		if (Dist < 0) {
			//
			return BACK_PLANE;
		}
		//
		return INTERSECT_PLANE;
	}
	// Plane equation a * x + b * y + c * z + d = 0
	INLINE void ComputePlane(float a, float b, float c, float d)
	{
		normal.x = a;
		normal.y = b;
		normal.z = c;
		dist = d;
	}
	// Plane equation for 3 points
	INLINE void ComputePlane(const Vector3D& p1, const Vector3D& p2, const Vector3D& p3)
	{
		normal = crossProduct((p3 - p1), (p2 - p1));
		normal.Normalize();
		//
		// A*x + B*y + C*z + dist = 0;
		// dist =  -(A*x + B*y + C*z);
		dist = - dotProduct(normal, p1);
	}
	//
	INLINE void ComputePlane(const Vector3D *vertex)
	{
		ComputePlane(vertex[0], vertex[1], vertex[2]);
	}
	//
	INLINE void ComputePlane(const Vector3D& Normal, const Vector3D& point)
	{
		normal = Normal;
		dist = - dotProduct(Normal, point);
	}
	//
	INLINE float DistanceToPoint(const Vector3D& point) const
	{
		return fabsf(dotProduct(point, normal) + dist);
	}
	//
	INLINE float SignedDistanceToPoint(const Vector3D& point) const
	{
		return dotProduct(point, normal) + dist;
	}
	//
	INLINE Plane(const Vector3D& p1, const Vector3D& p2,const Vector3D& p3)
	{
		ComputePlane(p1, p2, p3);
	}
	//
	INLINE Vector3D Point() const
	{
		return - dist * normal;
	}
	//
	INLINE void Flip()
	{
		normal = -normal;
		dist = -dist;
	}
	//
	INLINE float ClosestPoint(const Vector3D& p, Vector3D& res)
	{
		float distanceToPlane = -dist - dotProduct(p, normal);
		res = p + distanceToPlane * normal;
		return distanceToPlane;
	}
	//
	INLINE void ReflectPos(Vector3D& v) const
	{
		v -= (2.0f * (dotProduct(v, normal) + dist)) * normal;
	}
	//
	INLINE void ReflectDir(Vector3D& v) const
	{
		v -= 2.0f * dotProduct(v, normal) * normal;
	}
	//
	INLINE void ReflectPlane(Plane& plane)
	{
		Vector3D p;
		p = -plane.dist * plane.normal;
		ReflectDir(plane.normal);
		ReflectPos(p);
		plane.dist = - dotProduct(p, plane.normal);
	}
	//
	void ComputeNearPointMask(int& nearPointMask) const
	{
		if (normal.x > 0.0f) {
			if (normal.y > 0.0f) {
				if (normal.z > 0.0f) {
					nearPointMask = 0;
				}
				else {
					nearPointMask = 4;
				}
			}
			else {
				if (normal.z > 0.0f) {
					nearPointMask = 2;
				}
				else {
					nearPointMask = 6;
				}
			}
		}
		else {
			if (normal.y > 0.0f) {
				if (normal.z > 0.0f) {
					nearPointMask = 1;
				}
				else {
					nearPointMask = 5;
				}
			}
			else {
				if (normal.z > 0.0f)  {
					nearPointMask = 3;
				}
				else {
					nearPointMask = 7;
				}
			}
		}
	}
	//
	Vector3D MakeNearPoint(int nearPointMask, const Vector3D& minPoint, const Vector3D& maxPoint) const
	{
		return Vector3D(nearPointMask & 1 ? maxPoint.x : minPoint.x,
						nearPointMask & 2 ? maxPoint.y : minPoint.y,
						nearPointMask & 4 ? maxPoint.z : minPoint.z);
	}
	//
	Vector3D MakeFarPoint(int nearPointMask, const Vector3D& minPoint, const Vector3D& maxPoint ) const
	{
		return Vector3D(nearPointMask & 1 ? minPoint.x : maxPoint.x,
							nearPointMask & 2 ? minPoint.y : maxPoint.y,
							nearPointMask & 4 ? minPoint.z : maxPoint.z);
	}
	//
	INLINE friend bool PlaneIntersection(Vector3D& intersectPt, const Plane& p0, const Plane& p1, const Plane& p2)
	{
		Vector3D n0( p0.normal.x, p0.normal.y, p0.normal.z);
		Vector3D n1( p1.normal.x, p1.normal.y, p1.normal.z);
		Vector3D n2( p2.normal.x, p2.normal.y, p2.normal.z);

		Vector3D n1_n2 = crossProduct(n1, n2);
		Vector3D n2_n0 = crossProduct(n2, n0);
		Vector3D n0_n1 = crossProduct(n0, n1);

		float cosTheta = dotProduct(n0, n1_n2);

		if (ALMOST_ZERO(cosTheta) || IS_SPECIAL(cosTheta)) {
			return false;
		}

		float secTheta = 1.0f / cosTheta;

		n1_n2 = n1_n2 * p0.dist;
		n2_n0 = n2_n0 * p1.dist;
		n0_n1 = n0_n1 * p2.dist;

		intersectPt = -(n1_n2 + n2_n0 + n0_n1) * secTheta;
		return true;
	}
	// if ray intersect plane
	INLINE bool Intersect(const Vector3D& org, const Vector3D& dir) const
	{
		// A*x + B*y + C*z + d = 0 normal * point + d = 0
		//point = origin + direction * t = 0:
		// t = (normal * origin + d)/ (normal * direction)
		float d = dotProduct(normal, dir);
		//
		return d >= 0.0f;
	}
	//
	INLINE bool Intersect(const Vector3D& org, const Vector3D& dir, Vector3D& point) const
	{
		//A*x + B*y + C*z + d = 0 normal * point + d = 0
		//point = origin + direction * t = 0 имеем:
		// t = (normal * origin + d)/ (normal * direction)
		float d = dotProduct(normal, dir);
		//
		if (d) {
			//
			float t = -(dotProduct(normal, org) + dist) / d;
			//
			point = org + t * dir;
			return true;
		}
		return false;
	}
	INLINE operator float * ()
	{
		return &normal.x;
	}
	INLINE operator const float * () const
	{
		return &normal.x;
	}
	// for D3D
	INLINE operator D3DXPLANE * ()
	{
		return reinterpret_cast<D3DXPLANE *>(&normal);
	}
	// for D3D
	INLINE operator const D3DXPLANE * ()
	{
		return reinterpret_cast<const D3DXPLANE *>(&normal);
	}
	//
	INLINE Plane& operator = (const Plane& plane)
	{
		assert(this != &plane && "Apropriation to itself");
		dist = plane.dist;
		normal = plane.normal;
		return *this;
	}
	//
	INLINE Plane& operator *= (float value)
	{
		dist *= value;
		normal *= value;
		return *this;
	}
	//
	static Plane DirY()
	{
		return Plane(0.0f, 1.0f, 0.0f, 0.0f);
	}
	static Plane DirX()
	{
		return Plane(1.0f, 01.0f, 0.0f, 0.0f);
	}
	static Plane DirZ()
	{
		return Plane(0.0f, 01.0f, 1.0f, 0.0f);
	}
	INLINE Plane(const Plane& plane) : normal(plane.normal), dist(plane.dist)
	{
		normal.Normalize();
	}
	INLINE Plane(float a, float b, float c, float d) : dist(d)
	{
		normal = Vector3D(a, b, c);
		normal.Normalize();
	}
	INLINE Plane(const float* plane)
	{
		assert(plane && "NULL Pointer");
		normal = Vector3D(plane[0], plane[1], plane[2]);
		normal.Normalize();
		dist = plane[3];
	}
	INLINE Plane()
	{
	}
};

#endif
