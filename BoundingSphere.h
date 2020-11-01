// BoundingSphere.h: interface for the BoundingSphere class.
//
//////////////////////////////////////////////////////////////////////

#ifndef __BOUNDINGSPHERE_H__
#define __BOUNDINGSPHERE_H__

// Bounding Sphere - this is Bounding Volume via Shere

#include "Vector3D.h"

class BoundingSphere  {
private:
	// center of sphere
	Vector3D center;
	// Radius
	float radius;
public:
	//
	INLINE const Vector3D& GetCenter() const
	{
		return center;
	}
	//
	INLINE float GetRadius() const
	{
		return radius;
	}
	//
	INLINE void SetRadius(float Radius)
	{
		radius = Radius;
	}
	//
	INLINE void SetCenter(const Vector3D& Center)
	{
		center = Center;
	}
	INLINE void Translate(float x, float y, float z)
	{
		center.x += x;
		center.y += y;
		center.z += z;
	}
	INLINE void Translate(const Vector3D& pos)
	{
		Translate(pos.x, pos.y, pos.z);
	}
	//
	INLINE bool OverlapsSphere(const BoundingSphere& sphere) const
	{
		//
		const Vector3D d = center - sphere.GetCenter();
		//
		float r = radius + sphere.GetRadius();
		//
		return dotProduct(d, d) < (r * r);
	}
	INLINE BoundingSphere(const Vector3D& Center, float Radius) : center(Center), radius(Radius) {}
	INLINE BoundingSphere(const BoundingSphere& boundingsphere)
		: center(boundingsphere.center), radius(boundingsphere.radius) {}
	BoundingSphere()
	{
	}
};

#endif
