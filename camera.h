// Camera.h: interface for the Camera class.
//
//////////////////////////////////////////////////////////////////////

#ifndef __CAMERA_H__
#define __CAMERA_H__

//Right.x		 Up.x		 Direction.x		 position.x
//Right.y		 Up.y		 Direction.y		 position..y
//Right.z		 Up.z		 Direction.z		 position.z
//0			   0			  0			  1

#include "Vector2D.h"
#include "Frustum.h"
#include "Matrix4x4.h"
#include "BoundingSphere.h"
#include "BoundingBox.h"
#include "Bits.h"

#define USE_QUATERNIONS

// Camera class
class Camera  {
private:
#ifndef USE_QUATERNIONS
	static Matrix4x4 Mat;
#endif
	// old flags
	uint flags;
	// Changing camera flags
	uint changeCameraFlag;
	//
	BoundingBox bbox;
	// camera speed
	float Speed;
	// width / height
	float aspect;
	// near plane distance
	float zNear;
	// far plane distance
	float zFar;
	// Field of view
	float Fov;
	//
	float halfTanFov;
	//
	float CameraHeight;
	//
	Frustum frustum;
	// view matrix
	Matrix4x4 Matrix;
	// projection matrix
	Matrix4x4 proj;
	// Up vector for Orthonormal basis
	Vector3D Up;
	// Direction vector for Orthonormal basis
	Vector3D Direction;
	// Right vector for Orthonormal basis
	Vector3D Right;
	// camera position
	Vector3D Position;
	// old camera position
	Vector3D oldPos;
#ifdef ORBITAL_CAMERA
	// orientation
	Quaternion orientation;
	//
	Vector2D orbitAngles;
	//
	float accumPitchDegrees;
	//
	float orbitOffsetDistance;
	//
	Vector3D Target;
	//
	uint cameraType;
#endif
	// is move enabled
	uint isMove;
	// is rotate enabled
	uint isRotate;

	// if Free camera moving
	uint Fly;
	// move Type
	uchar lastMoveType;

	bool CheckOrbitalLimitRotation(float& y, bool checkAngle);
	//
	void ResetDefaultVectors();
	//
	void CalcBoundingBox();
	//
	void UpdateViewMatrix(float sign = 1.0f);
	//
	void ForceChangeOrbitDistance();
	//
	uint IsChangedOrbitDistance() const
	{
		return changeCameraFlag & CHANGE_ORBIT_DISTANCE;
	}
public:
 enum {
		MOVE_LEFT,	// moving to left
		MOVE_RIGHT,	 // moving to right
		MOVE_BACK,	 // moving back
		MOVE_FORWARD, // moving forward
		STOP
	};
	enum {
		FREE_CAMERA,
		ORBIT_CAMERA,
	};
	enum {
		SCROLL_LEFT  = STOP + 1,
		SCROLL_RIGHT,
		SCROLL_UP,
		SCROLL_DOWN,
	};
	enum CameraFlags {
		CHANGE_ORIENTATION = 0x00000001,
		CHANGE_POSITION = 0x00000010,
		CHANGE_ORBIT_DISTANCE = 1 << 3
	};
	INLINE int GetLastMoveType() const
	{
		return lastMoveType;
	}
	// change flag camera postion
	INLINE void ForceChangeCameraPosition()
	{
		changeCameraFlag |= CHANGE_POSITION;
	}
	// change flag camera
	INLINE void ForceChangeCamera()
	{
		changeCameraFlag |= CHANGE_ORIENTATION;
		ForceChangeCameraPosition();
	}
	//
	INLINE BitsBool IsChangeCamera() const
	{
		return IsChangeOrientation() | IsChangePosition();
	}
	//
	INLINE BitsBool IsChangePosition() const
	{
		return IsBitsSet(changeCameraFlag, CHANGE_POSITION);
	}
	//
	INLINE BitsBool IsChangeOrientation() const
	{
		return IsBitsSet(changeCameraFlag, CHANGE_ORIENTATION);
	}
	//
	INLINE void ClearChangeCamera()
	{
		changeCameraFlag = 0;
	}
	INLINE void ClearLastMoveType()
	{
		lastMoveType = STOP;
	}
	//
	INLINE uint GetFly() const
	{
		return Fly;
	}
	//
	INLINE bool EnableFly(uint flag)
	{
		Fly = flag;
		return true;
	}
	//
	INLINE bool EnableMove(uint flag)
	{
		isMove = flag;
		return true;
	}
	//
	INLINE bool EnableRotate(uint flag)
	{
		isRotate = flag;
		return true;
	}
	INLINE bool Lock(uint flag)
	{
		isRotate = !flag;
		isMove = !flag;
		return true;
	}
	INLINE bool IsLock() const
	{
		return !isRotate && !isMove;
	}
	//
	INLINE void SetOldPos()
	{
		Position = oldPos;
		CalcBoundingBox();
	}
	//
	INLINE const BoundingBox& GetBoundingBox() const
	{
		return bbox;
	}
	//
	INLINE Frustum& GetFrustum()
	{
		return frustum;
	}
	//
	INLINE const Frustum& GetFrustum() const
	{
		return frustum;
	}
	//
	INLINE bool SetPositionX(float X)
	{
		oldPos.x = Position.x;
		Position.x = X;
		return true;
	}
	//
	INLINE float GetCameraHeight() const
	{
		return CameraHeight;
	}
	//
	INLINE bool SetPositionY(float Y)
	{
		oldPos.y = Position.y;
		Position.y = Y;
		return true;
	}
	//
	INLINE bool SetPositionZ(float Z)
	{
		oldPos.z = Position.z;
		Position.z = Z;
		return true;
	}
	// Check bounding box visibility
	INLINE bool IsVisible(const Vector3D& minPoint, const Vector3D& maxPoint, bool& isIntersect) const
	{
		return frustum.BoundingBoxInFrustum(minPoint, maxPoint, isIntersect);
	}
	// Check bounding box visibility
	INLINE bool IsVisible(const Vector3D& minPoint, const Vector3D& maxPoint) const
	{
		return frustum.BoundingBoxInFrustum(minPoint, maxPoint);
	}
	// Check bounding box visibility
	template<typename BoxType>
	INLINE bool IsVisible(const BoxType& box) const
	{
		return frustum.BoundingBoxInFrustum(box.GetMinPoint(), box.GetMaxPoint());
	}
	template<typename BoxType>
	// Check bounding box visibility
	INLINE bool IsVisible(const BoxType& box, bool& isIntersect) const
	{
		return frustum.BoundingBoxInFrustum(box.GetMinPoint(), box.GetMaxPoint(), isIntersect);
	}
	// Check bounding sphere visibility
	INLINE bool IsVisible(const BoundingSphere& sphere) const
	{
		return frustum.SphereInFrustum(sphere.GetCenter(), sphere.GetRadius());
	}
	//
	float GetSpeed() const
	{
		return Speed;
	}
	// For shader constants
	const Vector3D& GetCameraParam() const
	{
		return reinterpret_cast<const Vector3D &>(zNear);
	}
	//
	void SetOrbitLimitAngles(const Vector2D& Angles);
	//
	bool SetFov(float fov);
	//
	bool SetzNear(float znear);
	//
	bool SetzFar(float zfar);
	//
	INLINE bool SetPosition(const Vector3D& pos)
	{
		return SetPosition(pos.x, pos.y, pos.z);
	}
	//
	INLINE bool SetPosition(float x, float y, float z)
	{
		oldPos = Position;
		Position.x = x;
		Position.y = CameraHeight + y;
		Position.z = z;
		ForceChangeCameraPosition();
		return true;
	}
	//
	INLINE void SetDirection(const Vector3D& dir)
	{
		SetDirection(dir.x, dir.y, dir.z);
	}
	//
	INLINE void SetDirection(float x, float y, float z)
	{
		Direction.x = x;
		Direction.y = y;
		Direction.z = z;
		ForceChangeCamera();
	}
	//
	INLINE void SetUp(const Vector3D& vec)
	{
		Up = vec;
	}
	//
	INLINE void SetRight(const Vector3D& vec)
	{
		Right = vec;
	}
	// Get Aspect
	INLINE float GetAspect() const
	{
		return aspect;
	}
	// Get camera Height
	INLINE float GetHeight() const
	{
		return CameraHeight;
	}
	//
	INLINE Matrix4x4& GetViewMatrix()
	{
		return Matrix;
	}
	//
	INLINE const Matrix4x4& GetViewMatrix() const
	{
		return Matrix;
	}
	//
	INLINE Matrix4x4& GetProjectionMatrix()
	{
		return proj;
	}
	//
	INLINE const Matrix4x4& GetProjectionMatrix() const
	{
		return proj;
	}
	//
	INLINE const Vector3D& GetPosition() const
	{
		return Position;
	}
	//
	INLINE Vector3D& GetPosition()
	{
		return Position;
	}
	//
	INLINE const Vector3D& GetOldPosition() const
	{
		return oldPos;
	}
	//
	INLINE const Vector3D& GetUp() const
	{
		return Up;
	}
	//
	INLINE Vector3D& GetUp()
	{
		return Up;
	}
	//
	INLINE const Vector3D& GetDirection() const
	{
		return Direction;
	}
	//
	INLINE Vector3D& GetDirection()
	{
		return Direction;
	}
	//
	INLINE const Vector3D& GetRight() const
	{
		return Right;
	}
	//
	float GetFov() const
	{
		return Fov;
	}
	//
	float GetHalfTanFov() const
	{
		return halfTanFov;
	}
	//
	float GetzNear() const
	{
		return zNear;
	}
	//
	float GetzFar() const
	{
		return zFar;
	}
	// Set Projection Matrix
	INLINE void SetProjectionMatrix(const Matrix4x4& matrix)
	{
		proj = matrix;
	}
	// Set View Matrix
	INLINE void SetViewMatrix(const Matrix4x4& matrix)
	{
		Matrix = matrix;
	}
	// Save Flags
	INLINE void SaveFlags()
	{
		flags = changeCameraFlag;
	}
	// Restore Flags
	INLINE void RestoreFlags()
	{
		changeCameraFlag = flags;
	}
	//
	bool SetSpeed(float speed);
	//
	bool SetCameraHeight(float height);
	//
	void Move(int KEY, float speed);
	//
	void Scroll(int key, float speed);
	// Set Aspect
	void SetAspect(float Aspect);
	//
	void Rotate(const Vector2D& point, bool checkAngle = false);
	// CalculateFrustum
	INLINE void ExtractFrustum()
	{
		frustum.ExtractFrustum(proj, Matrix);
	}
	INLINE void ExtractFrustum(const Matrix4x4& viewProj)
	{
		frustum.ExtractFrustum(viewProj);
	}
	// CalculateFrustum
	INLINE void CalculateNearFarPlanes()
	{
		frustum.CalculateNearFarPlanes(proj, Matrix);
	}
	//
#ifdef ORBITAL_CAMERA
	//
	float GetPitchAngle() const
	{
		return accumPitchDegrees;
	}
	//
	const Vector2D& GetPitchLimitAngles() const
	{
		return orbitAngles;
	}
	//
	const Quaternion& GetOrientation() const
	{
		return orientation;
	}
	//
	float GetOrbitDistance() const
	{
		return orbitOffsetDistance;
	}
	//
	void SetOrbitDistance(float value)
	{
		assert(value > 0.0f && "Invalid Value");
		if (orbitOffsetDistance != value) {
			orbitOffsetDistance = value;
			ForceChangeCamera();
		}
	}
#endif
	//
	void SetName(const char* Name);
	//
	void SetTarget(const Vector3D& target);
	//
	void ClearRotate();
	//
	bool UpdateCamera(float sign = 1.0f);
	//
	void SetCameraType(uint type);
	// Reflect camera by given plane
	void Reflect(const Plane& plane, bool calcFrustum = false);
	// Reflect camera by given plane
	void Reflect(const Camera& camera, Plane& plane, bool calcFrustum = false);
	Camera();
};

#endif
