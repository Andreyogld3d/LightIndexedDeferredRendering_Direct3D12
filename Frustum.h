// Frustum.h: interface for the Frustum class.
//
//////////////////////////////////////////////////////////////////////

#ifndef __FRUSTUM_H__
#define __FRUSTUM_H__

#include "Plane.h"
#include "Matrix4x4.h"
#include "types.h"
#include <cstddef>

// Visibility piramid
class Frustum  {
public:
	enum PlaneType {
		RIGHTPLANE,		// Right clipping plane
		LEFTPLANE,		// Left Clipping plane
		TOPPLANE,		// Top clipping plane
		BOTTOMPLANE,	// Bottom clipping Plane
		NEARPLANE,		// Near clipping plane
		FARPLANE		// Far Clipping plane
	};
private:
	// 6 clippin g planes
	Plane planes[6];
	// 6 masks
	int nearPointMasks[6];

	INLINE float Sqr(float value) const
	{
		return value * value;
	}
	INLINE int Classify(const Plane& plane, int nearPointMask, const Vector3D& minPoint, const Vector3D& maxPoint) const
	{
		const Vector3D& nearPoint = plane.MakeNearPoint(nearPointMask, minPoint, maxPoint);

		if (plane.ClassifyPoint(nearPoint) == Plane::FRONT_PLANE) {
			return Plane::FRONT_PLANE;
		}

		const Vector3D &farPoint = plane.MakeFarPoint(nearPointMask, minPoint, maxPoint);

		if (plane.ClassifyPoint(farPoint) == Plane::BACK_PLANE) {
			return Plane::BACK_PLANE;
		}
		return Plane::IN_PLANE;
	}
	INLINE int Classify(const Plane& plane, int nearPointMask, const Vector3D& minPoint, const Vector3D& maxPoint, bool& isIntersect) const
	{
		const Vector3D& nearPoint = plane.MakeNearPoint(nearPointMask, minPoint, maxPoint);

		if (plane.ClassifyPoint(nearPoint) == Plane::FRONT_PLANE) {
			return Plane::FRONT_PLANE;
		}

		const Vector3D& farPoint = plane.MakeFarPoint(nearPointMask, minPoint, maxPoint);

		if (plane.ClassifyPoint(farPoint) == Plane::BACK_PLANE) {
			return Plane::BACK_PLANE;
		}
		isIntersect = true;
		return Plane::IN_PLANE;
	}
public:
	// Get plane by number
	const Plane& GetPlane(size_t index) const
	{
		assert(index < 6 && "Out of Range");
		return planes[index];
	}
	// Get plane by number
	Plane& GetPlane(size_t index)
	{
		assert(index < 6 && "Out of Range");
		return planes[index];
	}
	// Calculate frustum
	INLINE void ExtractFrustum(const float* matrix)
	{
		//  build a view frustum based on the current view & projection matrices...
		Vector4D column4(matrix[12], matrix[13], matrix[14], matrix[15]);
		Vector4D column1(matrix[0], matrix[1], matrix[2], matrix[3]);
		Vector4D column2(matrix[4], matrix[5], matrix[6], matrix[7]);
		Vector4D column3(matrix[8], matrix[9], matrix[10], matrix[11]);

		//Vector4D Planes[6];
		planes[0] = Plane(column4 - column1);  // left
		planes[1] = Plane(column4 + column1);  // right
		planes[2] = Plane(column4 - column2);  // bottom
		planes[3] = Plane(column4 + column2);  // top
		planes[4] = Plane(column4 - column3);  // near
		planes[5] = Plane(column4 + column3);  // far

		for (int p = 0; p < 6; ++p) {
			planes[p].ComputeNearPointMask(nearPointMasks[p]);
		}
	}
	// Calculate frustum
	INLINE void CalculateNearFarPlanes(const float* proj, const float* modl/*, bool IsNormalize = false*/)
	{
		assert(proj && "NULL Pointer");
		assert(modl && "NULL Pointer");
		float clip[9];
		// View * Proj
		clip[0] = modl[ 0] * proj[ 2] + modl[ 1] * proj[ 6] + modl[ 2] * proj[10] + modl[ 3] * proj[14];
		clip[1] = modl[ 0] * proj[ 3] + modl[ 1] * proj[ 7] + modl[ 2] * proj[11] + modl[ 3] * proj[15];

		clip[2] = modl[ 4] * proj[ 2] + modl[ 5] * proj[ 6] + modl[ 6] * proj[10] + modl[ 7] * proj[14];
		clip[3] = modl[ 4] * proj[ 3] + modl[ 5] * proj[ 7] + modl[ 6] * proj[11] + modl[ 7] * proj[15];

		clip[4] = modl[ 8] * proj[ 2] + modl[ 9] * proj[ 6] + modl[10] * proj[10] + modl[11] * proj[14];
		clip[5] = modl[ 8] * proj[ 3] + modl[ 9] * proj[ 7] + modl[10] * proj[11] + modl[11] * proj[15];

		clip[6] = modl[12] * proj[ 2] + modl[13] * proj[ 6] + modl[14] * proj[10] + modl[15] * proj[14];
		clip[7] = modl[12] * proj[ 3] + modl[13] * proj[ 7] + modl[14] * proj[11] + modl[15] * proj[15];

		// compute all palnes
		planes[FARPLANE].ComputePlane(clip[1] - clip[0],
			clip[3] - clip[2],
			clip[5] - clip[4],
			clip[7] - clip[6]);
		planes[FARPLANE].ComputeNearPointMask(nearPointMasks[FARPLANE]);
		// 
		planes[NEARPLANE].ComputePlane(clip[1] + clip[0],
			clip[3] + clip[2],
			clip[5] + clip[4],
			clip[7] + clip[6]);
		planes[NEARPLANE].ComputeNearPointMask(nearPointMasks[NEARPLANE]);

	}
	// Calculate Frustum planes
	INLINE void ExtractFrustum(const Matrix4x4& clip)
	{
		// compute all palnes
		Plane& right = planes[RIGHTPLANE];
		//
		right.ComputePlane(clip[ 3] - clip[ 0], clip[ 7] - clip[ 4],
			clip[11] - clip[ 8], clip[15] - clip[12]);
		right.ComputeNearPointMask(nearPointMasks[RIGHTPLANE]);
		Plane& left = planes[LEFTPLANE];
		//
		left.ComputePlane(clip[ 3] + clip[ 0],
			clip[ 7] + clip[ 4],
			clip[11] + clip[ 8],
			clip[15] + clip[12]);
		left.ComputeNearPointMask(nearPointMasks[LEFTPLANE]);
		Plane& bottom = planes[BOTTOMPLANE];
		//
		bottom.ComputePlane(clip[ 3] + clip[ 1],
			clip[ 7] + clip[ 5],
			clip[11] + clip[ 9],
			clip[15] + clip[13]);
		bottom.ComputeNearPointMask(nearPointMasks[BOTTOMPLANE]);
		Plane& top = planes[TOPPLANE];
		//
		top.ComputePlane(clip[ 3] - clip[ 1],
			clip[ 7] - clip[ 5],
			clip[11] - clip[ 9],
			clip[15] - clip[13]);
		top.ComputeNearPointMask(nearPointMasks[TOPPLANE]);
		Plane& Far = planes[FARPLANE];
		//
		Far.ComputePlane( clip[ 3] - clip[ 2],
			clip[ 7] - clip[ 6],
			clip[11] - clip[10],
			clip[15] - clip[14]);
		Far.ComputeNearPointMask(nearPointMasks[FARPLANE]);
		Plane& Near = planes[NEARPLANE];
		//
		Near.ComputePlane(clip[ 3] + clip[ 2],
			clip[ 7] + clip[ 6],
			clip[11] + clip[10],
			clip[15] + clip[14]);
		Near.ComputeNearPointMask(nearPointMasks[NEARPLANE]);
#if 1
		//if (IsNormalize) {  //
		float t = InvSqrt(Sqr(right.normal.x) + Sqr(right.normal.y) + Sqr(right.normal.z));
		right *= t;
		t = InvSqrt(Sqr(left.normal.x) + Sqr(left.normal.y) + Sqr(left.normal.z));
		left *= t;
		t = InvSqrt(Sqr(top.normal.x) + Sqr(top.normal.y) + Sqr(top.normal.z));
		top *= t;
		t = InvSqrt(Sqr(bottom.normal.x) + Sqr(bottom.normal.y) + Sqr(bottom.normal.z));
		bottom *= t;
		t = InvSqrt(Sqr(Near.normal.x) + Sqr(Near.normal.y) + Sqr(Near.normal.z));
		Near *= t;
		t = InvSqrt(Sqr(Far.normal.x) + Sqr(Far.normal.y) + Sqr(Far.normal.z));
		Far *= t;
		// }
#endif
	}
	INLINE void ExtractFrustum(const Matrix4x4& proj, const Matrix4x4& view)
	{
		const Matrix4x4 viewProj = proj * view;
		ExtractFrustum(viewProj);
	}
	// Sphere in frustum
	INLINE bool SphereInFrustum(const Vector3D& center, float radius) const
	{
		// Check sphere orienation for near plane
		if (planes[NEARPLANE].SignedDistanceToPoint(center) <= -radius) {
			return false;
		}
		// Check sphere orienation for far plane
		if (planes[FARPLANE].SignedDistanceToPoint(center) <= -radius) {
			return false;
		}
		// Check sphere orienation for left plane
		if (planes[LEFTPLANE].SignedDistanceToPoint(center) <= -radius) {
			return false;
		}
		// Check sphere orienation for rigth plane
		if (planes[RIGHTPLANE].SignedDistanceToPoint(center) <= -radius) {
			return false;
		}
		// Check sphere orienation for top plane
		if (planes[TOPPLANE].SignedDistanceToPoint(center) <= -radius) {
			return false;
		}
		// Check sphere orienation for bottom plane
		if (planes[BOTTOMPLANE].SignedDistanceToPoint(center) <= -radius) {
			return false;
		}
		return true;
	}
	INLINE bool TriangleInFrustum(const Vector3D& A, const Vector3D& B, const Vector3D& C) const
	{
		//
		if (planes[LEFTPLANE].ClassifyPoint(A) == Plane::BACK_PLANE && 
			planes[LEFTPLANE].ClassifyPoint(B) == Plane::BACK_PLANE &&
			planes[LEFTPLANE].ClassifyPoint(C) == Plane::BACK_PLANE) {
			return false;
		}
		//
		if (planes[RIGHTPLANE].ClassifyPoint(A) == Plane::BACK_PLANE &&
			planes[RIGHTPLANE].ClassifyPoint(B) == Plane::BACK_PLANE &&
			planes[RIGHTPLANE].ClassifyPoint(C) == Plane::BACK_PLANE) {
			return false;
		}
		//
		if (planes[TOPPLANE].ClassifyPoint(A) == Plane::BACK_PLANE &&
			planes[TOPPLANE].ClassifyPoint(B) == Plane::BACK_PLANE &&
			planes[TOPPLANE].ClassifyPoint(C) == Plane::BACK_PLANE) {
			return false;
		}
		//
		if (planes[BOTTOMPLANE].ClassifyPoint(A) == Plane::BACK_PLANE &&
			planes[BOTTOMPLANE].ClassifyPoint(B) == Plane::BACK_PLANE &&
			planes[BOTTOMPLANE].ClassifyPoint(C) == Plane::BACK_PLANE) {
			return false;
		}
		//
		if (planes[NEARPLANE].ClassifyPoint(A) == Plane::BACK_PLANE &&
			planes[NEARPLANE].ClassifyPoint(B) == Plane::BACK_PLANE &&
			planes[NEARPLANE].ClassifyPoint(C) == Plane::BACK_PLANE) {
			return false;
		}
		//
		if (planes[FARPLANE].ClassifyPoint(A) == Plane::BACK_PLANE &&
			planes[FARPLANE].ClassifyPoint(B) == Plane::BACK_PLANE &&
			planes[FARPLANE].ClassifyPoint(C) == Plane::BACK_PLANE) {
			return false;
		}
		return true;
	}
	// Check cube visibility in frustum
	INLINE bool PolygonInFrustum(const Vector3D& A, const Vector3D& B, const Vector3D& C, const Vector3D& D) const
	{
		const Plane& leftPlane = planes[LEFTPLANE];
		//
		if (leftPlane.ClassifyPoint(A) == Plane::BACK_PLANE &&
			leftPlane.ClassifyPoint(B) == Plane::BACK_PLANE &&
			leftPlane.ClassifyPoint(C) == Plane::BACK_PLANE &&
			leftPlane.ClassifyPoint(D) == Plane::BACK_PLANE) {
			return false;
		}
		const Plane& rightPlane = planes[RIGHTPLANE];
		//
		if (rightPlane.ClassifyPoint(A) == Plane::BACK_PLANE &&
			rightPlane.ClassifyPoint(B) == Plane::BACK_PLANE &&
			rightPlane.ClassifyPoint(C) == Plane::BACK_PLANE &&
			rightPlane.ClassifyPoint(D) == Plane::BACK_PLANE) {
			return false;
		}
		const Plane& topPlane = planes[TOPPLANE];
		//
		if (topPlane.ClassifyPoint(A) == Plane::BACK_PLANE &&
			topPlane.ClassifyPoint(B) == Plane::BACK_PLANE &&
			topPlane.ClassifyPoint(C) == Plane::BACK_PLANE &&
			topPlane.ClassifyPoint(D) == Plane::BACK_PLANE) {
			return false;
		}
		const Plane& bottomPlane = planes[BOTTOMPLANE];
		//
		if (bottomPlane.ClassifyPoint(A) == Plane::BACK_PLANE &&
			bottomPlane.ClassifyPoint(B) == Plane::BACK_PLANE &&
			bottomPlane.ClassifyPoint(C) == Plane::BACK_PLANE &&
			bottomPlane.ClassifyPoint(D) == Plane::BACK_PLANE) {
			return false;
		}
		const Plane& nearPlane = planes[NEARPLANE];
		//
		if (nearPlane.ClassifyPoint(A) == Plane::BACK_PLANE &&
			nearPlane.ClassifyPoint(B) == Plane::BACK_PLANE &&
			nearPlane.ClassifyPoint(C) == Plane::BACK_PLANE &&
			nearPlane.ClassifyPoint(D) == Plane::BACK_PLANE) {
			return false;
		}
		const Plane& farPlane = planes[FARPLANE];
		//
		if (farPlane.ClassifyPoint(A) == Plane::BACK_PLANE &&
			farPlane.ClassifyPoint(B) == Plane::BACK_PLANE &&
			farPlane.ClassifyPoint(C) == Plane::BACK_PLANE &&
			farPlane.ClassifyPoint(D) == Plane::BACK_PLANE) {
			return false;
		}
		return true;
	}
	// Check cube visibility in frustum
	INLINE bool CubeInFrustum( float x, float y, float z, float size ) const
	{
		float xSize = x + size;
		float x_Size = x - size;
		float ySize = y + size;
		float y_Size = y - size;
		float zSize = z + size;
		float z_Size = z - size;
		for(int i = 0; i < 6; i++ ) {
			if (planes[i].ClassifyPoint(x_Size, y_Size, z_Size) != Plane::BACK_PLANE) {
				continue;
			}
			if (planes[i].ClassifyPoint(xSize, y_Size, z_Size) != Plane::BACK_PLANE) {
				continue;
			}
			if (planes[i].ClassifyPoint(x_Size, ySize, z_Size) != Plane::BACK_PLANE) {
				continue;
			}
			if (planes[i].ClassifyPoint(xSize, y_Size, z_Size) != Plane::BACK_PLANE) {
				continue;
			}
			if (planes[i].ClassifyPoint(xSize, ySize, z_Size) != Plane::BACK_PLANE) {
				continue;
			}
			if (planes[i].ClassifyPoint(x_Size, y_Size, zSize) != Plane::BACK_PLANE) {
				continue;
			}
			if (planes[i].ClassifyPoint(xSize, y_Size, zSize) != Plane::BACK_PLANE) {
				continue;
			}
			if (planes[i].ClassifyPoint(x_Size, ySize, zSize) != Plane::BACK_PLANE) {
				continue;
			}
			if (planes[i].ClassifyPoint(xSize, ySize, zSize) != Plane::BACK_PLANE) {
				continue;
			}
			// If we get here, it isn't in the frustum
			return false;
		}
		return true;
	}
	// point in frustum
	INLINE bool PointInFrustum(const Vector3D& point) const
	{
		if (planes[LEFTPLANE].ClassifyPoint(point) == Plane::BACK_PLANE) {
			return false;
		}
		if (planes[RIGHTPLANE].ClassifyPoint(point) == Plane::BACK_PLANE) {
			return false;
		}
		if (planes[TOPPLANE].ClassifyPoint(point) == Plane::BACK_PLANE) {
			return false;
		}
		if (planes[BOTTOMPLANE].ClassifyPoint(point) == Plane::BACK_PLANE) {
			return false;
		}
		if (planes[NEARPLANE].ClassifyPoint(point) == Plane::BACK_PLANE) {
			return false;
		}
		if (planes[FARPLANE].ClassifyPoint(point) == Plane::BACK_PLANE) {
			return false;
		}
		return true;
	}
	// Figure visibility  in frustum
	INLINE bool InFrustum(const Vector3D* verts, uint num_verts) const
	{
		for (uint i = 0; i < 6; i++) {
			uint j = 0;
			for (; j <  num_verts; j++) {
				if (planes[i].ClassifyPoint(verts[j]) != Plane::BACK_PLANE) {
					break; // next i
				}
			}
			if (j == num_verts) {
				return false;
			}
		}
		return true;
	}
	// Check BoundingBox visibility in frustum
	INLINE bool BoundingBoxInFrustum(const Vector3D& minPoint, const Vector3D& maxPoint) const
	{
		if (Classify(planes[NEARPLANE], nearPointMasks[NEARPLANE], minPoint, maxPoint) == Plane::BACK_PLANE) {
			return false;
		}
		if (Classify(planes[FARPLANE], nearPointMasks[FARPLANE], minPoint, maxPoint) == Plane::BACK_PLANE) {
			return false;
		}
		if (Classify(planes[RIGHTPLANE], nearPointMasks[RIGHTPLANE], minPoint, maxPoint) == Plane::BACK_PLANE) {
			return false;
		}
		if (Classify(planes[LEFTPLANE], nearPointMasks[LEFTPLANE], minPoint, maxPoint) == Plane::BACK_PLANE) {
			 return false;
		}
		if (Classify(planes[TOPPLANE], nearPointMasks[TOPPLANE], minPoint, maxPoint) == Plane::BACK_PLANE) {
			return false;
		}
		if (Classify(planes[BOTTOMPLANE], nearPointMasks[BOTTOMPLANE], minPoint, maxPoint) == Plane::BACK_PLANE) {
			return false;
		}
		return true;
	}
	// Check BoundingBox visibility in frustum
	INLINE bool BoundingBoxInFrustum(const Vector3D& minPoint, const Vector3D& maxPoint, bool& isIntersect) const
	{
		if (Classify(planes[NEARPLANE], nearPointMasks[NEARPLANE], minPoint, maxPoint, isIntersect) == Plane::BACK_PLANE) {
			return false;
		}
		if (Classify(planes[FARPLANE], nearPointMasks[FARPLANE], minPoint, maxPoint, isIntersect) == Plane::BACK_PLANE) {
			return false;
		}
		if (Classify(planes[RIGHTPLANE], nearPointMasks[RIGHTPLANE], minPoint, maxPoint, isIntersect) == Plane::BACK_PLANE) {
			return false;
		}
		if (Classify(planes[LEFTPLANE], nearPointMasks[LEFTPLANE], minPoint, maxPoint, isIntersect) == Plane::BACK_PLANE) {
			return false;
		}
		if (Classify(planes[TOPPLANE], nearPointMasks[TOPPLANE], minPoint, maxPoint, isIntersect) == Plane::BACK_PLANE) {
			return false;
		}
		if (Classify(planes[BOTTOMPLANE], nearPointMasks[BOTTOMPLANE], minPoint, maxPoint, isIntersect) == Plane::BACK_PLANE) {
			return false;
		}
		return true;
	}
	// Calculate points for frustum
	INLINE void CalculatePoints(Vector3D* points) const
	{
		assert(points && "NULL Pointer");
		for (int i = 0; i < 8; ++i) { // compute extrema
			const Plane& p0 = (i & 1) ? planes[4] : planes[5];
			const Plane& p1 = (i & 2) ? planes[3] : planes[2];
			const Plane& p2 = (i & 4) ? planes[0] : planes[1];

			PlaneIntersection(points[i], p0, p1, p2);
		}
	}
	// Intersect plane by Ray
	bool IntersectByRay(const Vector3D& dir) const
	{
		if (planes[LEFTPLANE].Intersect(dir)) {
			return true;
		}
		if (planes[RIGHTPLANE].Intersect(dir)) {
			return true;
		}
		if (planes[TOPPLANE].Intersect(dir)) {
			return true;
		}
		if (planes[BOTTOMPLANE].Intersect(dir)) {
			return true;
		}
		if (planes[NEARPLANE].Intersect(dir)) {
			return true;
		}
		if (planes[FARPLANE].Intersect(dir)) {
			return true;
		}
		return false;
	}
	Frustum(const float* matrix) 
	{
		ExtractFrustum(matrix);
	}
	Frustum()
	{}
};

#endif // __FRUSTUM_H__
