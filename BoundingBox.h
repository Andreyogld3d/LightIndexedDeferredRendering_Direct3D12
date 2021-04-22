#ifndef __BOUNDINGBOX_H__
#define __BOUNDINGBOX_H__

#include "Vector3D.h"
#include <cstdint>
#include <cfloat>

class BoundingBox {
private:
	typedef unsigned int uint;
	INLINE float GetMin(float a, float b) const
	{
		return a < b ? a : b;
	}
public:
	enum BoxPoints {
		LEFT_DOWN_FRONT,
		LEFT_UP_FRONT,
		RIGHT_UP_FRONT,
		RIGHT_DOWN_FRONT,
		LEFT_DOWN_BACK,
		LEFT_UP_BACK,
		RIGHT_UP_BACK,
		RIGHT_DOWN_BACK,
		TOTAL_POINTS
	};
	Vector3D minPoint;
	Vector3D maxPoint;
	//
	static void GetVertexIndices(uint16_t* indices)
	{
		//		5----6
		//	  / |	/|
		//	 /	|  / |
		//	/	| /  |
		//	1--4- 2  |
		//	|   / |  7
		//	| /   | /
		//	0---- 3
		const uint16_t Indices[] = {
			0, 1, 2, 2, 3, 0,	// front
			4, 5, 1, 1, 0, 4,	// left
			7, 6, 5, 5, 4, 7,	// back
			3, 2, 6, 6, 7, 3,	// right
			1, 5, 6, 6, 2, 1,	// top
			3, 7, 4, 4, 0, 3,	// bottom
		};
		assert(indices && "NULL Pointer");
		memcpy(indices, Indices, sizeof(Indices));
	}
	//
	INLINE BoundingBox(const Vector3D& minP, const Vector3D& maxP)
	{
		minPoint = minP;
		maxPoint = maxP;
	}
	INLINE BoundingBox()
	{
		Reset();
	}
	void Reset()
	{
		maxPoint = Vector3D(- FLT_MAX, - FLT_MAX, - FLT_MAX);
		minPoint = Vector3D(FLT_MAX, FLT_MAX, FLT_MAX);
	}
	void Clear()
	{
		Reset();
	}
	//
	Vector3D GetNearestPointFromPosition(const Vector3D& pos) const
	{
		Vector3D points[TOTAL_POINTS];
		GetPoints(points);
		float minDist = FLT_MAX;
		int index = 0;
		for (int i = 0; i < TOTAL_POINTS; ++i)
		{
			float dist = pos.GetDistanceSqFrom(points[i]);
			if (dist < minDist)
			{
				minDist = dist;
				index = i;
			}
		}
		assert(index < TOTAL_POINTS && "Out Of Range");
		return points[index];
	}
	//
	Vector3D GetFarPointFromPosition(const Vector3D& pos) const
	{
		Vector3D points[TOTAL_POINTS];
		GetPoints(points);
		float maxDist = 0;
		int index = 0;
		for (int i = 0; i < TOTAL_POINTS; ++i)
		{
			float dist = pos.GetDistanceSqFrom(points[i]);
			if (dist > maxDist)
			{
				maxDist = dist;
				index = i;
			}
		}
		assert(index < TOTAL_POINTS && "Out Of Range");
		return points[index];
	}

	void CalcPoints()
	{
	}
	INLINE void Grow(float x, float y, float z)
	{
		minPoint.x -= x;
		minPoint.y -= y;
		minPoint.z -= z;
		maxPoint.x += x;
		maxPoint.y += y;
		maxPoint.z += z;
	}
	INLINE void Grow(float val)
	{
		Grow(val, val, val);
	}
	INLINE void Grow(const Vector3D& offset)
	{
		Grow(offset.x, offset.y, offset.z);
	}
	INLINE void Translate(float x, float y, float z)
	{
		minPoint.x += x;
		minPoint.y += y;
		minPoint.z += z;
		maxPoint.x += x;
		maxPoint.y += y;
		maxPoint.z += z;
	}
	INLINE void Translate(const Vector3D& pos)
	{
		Translate(pos.x, pos.y, pos.z);
	}
	const Vector3D GetVectorSize() const
	{
		return maxPoint - minPoint;
	}
	float GetRadiusSq() const
	{
		const Vector3D& vSize = GetVectorSize();
		return vSize.LengthSq();
	}
	float GetRadius() const
	{
		float rSq = GetRadiusSq();
		return sqrtf(rSq);
	}
	const Vector3D GetPoint(uint index) const
	{
		switch (index) {
			case LEFT_DOWN_FRONT: {
				return minPoint;
			}
			break;
			case LEFT_UP_FRONT: {
				return Vector3D(minPoint.x, maxPoint.y, minPoint.z);
			}
			break;
			case RIGHT_UP_FRONT: {
				return Vector3D(maxPoint.x, maxPoint.y, minPoint.z);
			}
			break;
			case RIGHT_DOWN_FRONT: {
				return Vector3D(maxPoint.x, minPoint.y, minPoint.z);
			}
			break;
			case LEFT_DOWN_BACK: {
				return Vector3D(minPoint.x, minPoint.y, maxPoint.z);
			}
			break;
			case LEFT_UP_BACK: {
				return Vector3D(minPoint.x, maxPoint.y, maxPoint.z);
			}
			break;
			case RIGHT_UP_BACK: {
				return maxPoint;
			}
			break;
			case RIGHT_DOWN_BACK: {
				return Vector3D(maxPoint.x, minPoint.y, maxPoint.z);
			}
			break;
		};
		assert(index < 8 && "Out Of Range");
		return Vector3D::Zero();
	}
	//
	void GetPoints(Vector3D* outPoints) const
	{
		const Vector3D points[] = {minPoint,
									Vector3D(minPoint.x, maxPoint.y, minPoint.z),
									Vector3D(maxPoint.x, maxPoint.y, minPoint.z),
									Vector3D(maxPoint.x, minPoint.y, minPoint.z),
									Vector3D(minPoint.x, minPoint.y, maxPoint.z),
									Vector3D(minPoint.x, maxPoint.y, maxPoint.z),
									maxPoint,
									Vector3D(maxPoint.x, minPoint.y, maxPoint.z)
		};
		assert(outPoints && "NULL Pointer");
		memcpy(reinterpret_cast<void *>(outPoints), points, sizeof(points));
	}
	//
	BoundingBox Union(const BoundingBox& box) const
	{
		assert(box.IsValid() && "Invalid Bounding box");
		assert(IsValid() && "Invalid Bounding box");

		BoundingBox Box;
		if (!OverlapsAABB(box) && !box.OverlapsAABB(*this)) {
			return Box;
		}
		Box.minPoint.x = GetMin(box.minPoint.x, minPoint.x);
		Box.minPoint.y = GetMin(box.minPoint.y, minPoint.y);
		Box.minPoint.z = GetMin(box.minPoint.z, minPoint.z);

		Box.maxPoint.x = GetMin(box.maxPoint.x, maxPoint.x);
		Box.maxPoint.y = GetMin(box.maxPoint.y, maxPoint.y);
		Box.maxPoint.z = GetMin(box.maxPoint.z, maxPoint.z);

		assert(Box.IsValid() && "Invalid Bounding box");

		return Box;
	}
	template<typename T>
	// Проверка столкновений AABB - AABB
	INLINE bool OverlapsAABB(const T& aabb) const
	{
		if (minPoint.x > aabb.GetMaxPoint().x) {
			return false;
		}
		if (maxPoint.x < aabb.GetMinPoint().x) {
			return false;
		}

		if (minPoint.y > aabb.GetMaxPoint().y) {
			return false;
		}
		if (maxPoint.y < aabb.GetMinPoint().y) {
			return false;
		}
		if (minPoint.z > aabb.GetMaxPoint().z) {
			return false;
		}
		if (maxPoint.z < aabb.GetMinPoint().z) {
			return false;
		}
		return true;
	}
	INLINE void Move(float x, float y, float z)
	{
		Translate(x, y, z);
	}
	INLINE void Move(const Vector3D& pos)
	{
		Translate(pos);
	}
	INLINE bool IsValid() const
	{
		return maxPoint >= minPoint;
	}
	//
	INLINE bool Contains(const BoundingBox& aabb) const
	{
		assert(IsValid() && "Invalid BoundingBox");
		const Vector3D& maxP = aabb.maxPoint;
		const Vector3D& minP = aabb.minPoint;
		return maxPoint.x >= maxP.x && maxPoint.y >= maxP.y && maxPoint.z >= maxP.z &&
			minPoint.x <= minP.x && minPoint.y <= minP.y && minPoint.z <= minP.z;
	}
	//
	INLINE bool Contains(float x, float z) const
	{
		//
		if (x < minPoint.x) {
			return false;
		}
		if (z < minPoint.z) {
			return false;
		}
		if (x > maxPoint.x) {
			return false;
		}
		if (z > maxPoint.z) {
			return false;
		}
		return true;
	}
	//
	INLINE bool Contains(float x, float y, float z) const
	{
		//
		if (x < minPoint.x) {
			return false;
		}
		if (y < minPoint.y) {
			return false;
		}
		if (z < minPoint.z) {
			return false;
		}
		if (x > maxPoint.x) {
			return false;
		}
		if (y > maxPoint.y) {
			return false;
		}
		if (z > maxPoint.z) {
			return false;
		}
		return true;
	}
	//
	INLINE bool Contains(const Vector3D& p) const
	{
		return Contains(p.x, p.y, p.z);
	}
	//
	INLINE const Vector3D& GetMinPoint() const
	{
		return minPoint;
	}
	//
	INLINE const Vector3D& GetMaxPoint() const
	{
		return maxPoint;
	}
	template<typename BoxType>
	void Add(const BoxType& box)
	{
		AddVertex(box.GetMinPoint());
		AddVertex(box.GetMaxPoint());
	}
	//
	void AddVertex(float x, float y, float z)
	{
		//
		if (minPoint.x > x) {
			minPoint.x = x;
		}
		if (minPoint.y > y) {
			minPoint.y = y;
		}
		if (minPoint.z > z) {
			minPoint.z = z;
		}

		if (maxPoint.x < x) {
			maxPoint.x = x;
		}
		if (maxPoint.y < y) {
			maxPoint.y = y;
		}
		if (maxPoint.z < z) {
			maxPoint.z = z;
		}
	}
	//
	void AddVertex(const float* vertex)
	{
		assert(vertex && "NULL Pointer");
		AddVertex(vertex[0], vertex[1], vertex[2]);
	}
	void AddVertex(const Vector3D& vert)
	{
		AddVertex(vert.x, vert.y, vert.z);
	}
	void AddVertices(const Vector3D* vertices, uint n)
	{
		assert(vertices && "NULL Pointer");
		for(uint i = 0; i < n; ++i) {
			AddVertex(vertices[i]);
		}
	}
	template<typename T>
	void Merge(const T& box)
	{
		Add(box);
	}
	//
	INLINE void SetMinPoint(const Vector3D& newPoint)
	{
		minPoint = newPoint;
	}
	//
	INLINE void SetMaxPoint(const Vector3D& newPoint)
	{
		maxPoint = newPoint;
	}
	const Vector3D GetCenter() const
	{
		return (maxPoint + minPoint) * 0.5f;
	}
	//
	INLINE void InitBoundingBox(const Vector3D& MinPoint, const Vector3D& MaxPoint)
	{
		assert(MinPoint.x <= MaxPoint.x && "Invalid Value");
		assert(MinPoint.y <= MaxPoint.y && "Invalid Value");
		assert(MinPoint.z <= MaxPoint.z && "Invalid Value");
		minPoint = MinPoint;
		maxPoint = MaxPoint;
	}
};

#endif
