// camera->cpp: implementation of the Camera class.
//
//////////////////////////////////////////////////////////////////////
#include "PrecompileHeaders.h"
#include "Camera.h"
#include "Quaternion.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

#ifndef USE_QUATERNIONS
Matrix4x4 Camera::Mat;
#endif

//
Camera::Camera() : flags(0), changeCameraFlag(CHANGE_ORIENTATION | CHANGE_POSITION), Speed(8.0f), aspect(1.0f),
					zNear(0.16f), zFar(2000.0f), Fov(45.0f), halfTanFov(0.0f),
					CameraHeight(3.5f), isMove(true),
					isRotate(true), Fly(false), lastMoveType(STOP)
#ifdef ORBITAL_CAMERA
					, orbitOffsetDistance(0.0f),
					accumPitchDegrees(0),
					cameraType(Camera::FREE_CAMERA)
#endif
{
#ifdef ORBITAL_CAMERA
	Target = Vector3D::Zero();
	orbitAngles = Vector2D(-FLT_MAX, FLT_MAX);
	accumPitchDegrees = 0.0f;
	orientation.LoadIdentity();
#endif
	Up = Vector3D::Y();
	Position.x = 20.0f;
	Position.z = -10.0f;
	Position.y = -0.7f;
	Right = Vector3D::X();
	Direction = Vector3D::Z();
	Matrix.LoadIdentity();
	proj.LoadIdentity();
	CalcBoundingBox();
}

void Camera::CalcBoundingBox()
{
	const float dx = 0.01f;
	const float dz = 0.01f;
	//
	Vector3D v;
	v.x = Position.x - dx;
	v.y = Position.y - CameraHeight;
	v.z = Position.z - dz;
	bbox.SetMinPoint(v);
	v.x = Position.x + dx;
	v.y = Position.y + CameraHeight;
	v.z = Position.z + dz;
	bbox.SetMaxPoint(v);
}

//
void Camera::Scroll(int key, float speed)
{
	if (!isMove) {
		return;
	}
	const float k  = 1.0f / 100.0f;
	speed *= 1000.0f;
	//
	float dx = Speed * speed * k;
	// Save old position
	oldPos = Position;
	// Check moving type
	if (SCROLL_LEFT == key) {
		ForceChangeCameraPosition();
		Position.x -= dx;
	}
	else if (SCROLL_RIGHT == key) {
		ForceChangeCameraPosition();
		Position.x += dx;
	}
	else if (SCROLL_DOWN == key) {
		ForceChangeCameraPosition();
		Position.y -= dx;
	}
	else if (SCROLL_UP == key) {
		ForceChangeCameraPosition();
		Position.y += dx;
	}
	CalcBoundingBox();
}

// Move camera
void Camera::Move(int key, float speed)
{
	if (!isMove) {
		return;
	}
	const float k  = 1.0f / 100.0f;
	speed *= 1000.0f;
	// Calc Coords
	float dz = Speed * speed * k;
	float dx = Speed * speed * k;
	oldPos = Position;
	//
	if (MOVE_LEFT == key) {
		Position.x -= Right.x * dx;
		if (Fly) {
			Position.y -= Right.y * dx;
		}
		Position.z -= Right.z * dx;
		ForceChangeOrbitDistance();
	}
	else if (MOVE_RIGHT == key) {
		Position.x += Right.x * dx;
		if (Fly) {
			Position.y += Right.y * dx;
		}
		Position.z += Right.z * dx;
		ForceChangeOrbitDistance();
	}
	else if (MOVE_BACK == key) {
		Position.x -= (Direction.x * dx);
		if (Fly) {
			Position.y -= (Direction.y * dx);
		}
		Position.z -= (Direction.z * dz);
		ForceChangeOrbitDistance();
	}
	else if (MOVE_FORWARD == key) {
		Position.x += (Direction.x * dx);
		if (Fly) {
			Position.y += (Direction.y * dx);
		}
		Position.z += (Direction.z * dz);
		lastMoveType = key;
		ForceChangeOrbitDistance();
	}
	lastMoveType = key;
	if (lastMoveType >= MOVE_LEFT && lastMoveType <= MOVE_FORWARD) {
		CalcBoundingBox();
		ForceChangeCameraPosition();
	}
}

// Setup camera speed
bool Camera::SetSpeed(float speed)
{
	const float MAX_SPEED = 100.0f;
	//
	const float MIN_SPEED = 0.01f;
	//
	if (speed < MIN_SPEED || speed > MAX_SPEED + 3) {
		return false;
	}
	Speed = speed;
	return true;
}

// Set camera height
bool Camera::SetCameraHeight(float height)
{
	assert(height > 0 && "Invalid Value");
	if (height <= 0) {
		return false;
	}
	CameraHeight = height;
	return true;
}

// Set Aspect
void Camera::SetAspect(float Aspect)
{
	assert(Aspect > 0.0f && "Invalid Value");
	if (Aspect <= 0.0f) {
		return;
	}
	aspect = Aspect;
}

#ifdef ORBITAL_CAMERA
void Camera::SetOrbitLimitAngles(const Vector2D& Angles)
{
	orbitAngles = Angles;
	accumPitchDegrees = Angles.x;
}

bool Camera::CheckOrbitalLimitRotation(float& y, bool checkAngle)
{
	if (FLT_MAX == orbitAngles.x && FLT_MAX == orbitAngles.y) {
		return false;
	}
	// TO DO
	if (checkAngle) {
		float sign = y >= 0.0f ? 1.0f : -1.0f;
		//float angle = std::max(orbitAngles.x, orbitAngles.y);
		//float angleLimit = 0.5f * angle;
		if (fabsf(y) >= 3.0f) {
			y = sign * 2.0f;
		}
	}
	float pitchDegrees = y;
	accumPitchDegrees += pitchDegrees;
	const float maxAngle = orbitAngles.x;
	const float maxAngle2 = orbitAngles.y;
	if (maxAngle2 != FLT_MAX && accumPitchDegrees > maxAngle2) {
		pitchDegrees = maxAngle2 - (accumPitchDegrees - pitchDegrees);
		if (pitchDegrees > maxAngle2) {
			pitchDegrees = maxAngle2;
		}
		accumPitchDegrees = maxAngle2;
	}
	if (maxAngle != FLT_MAX && accumPitchDegrees < -maxAngle) {
		pitchDegrees = -maxAngle - (accumPitchDegrees - pitchDegrees);
		if (pitchDegrees < -maxAngle) {
			pitchDegrees = -maxAngle;
		}
		accumPitchDegrees = -maxAngle;
	}
	y = pitchDegrees;
	return true;
}
#endif

//
void Camera::ForceChangeOrbitDistance()
{
	changeCameraFlag |= CHANGE_ORBIT_DISTANCE;
}

// Rotate camera
void Camera::Rotate(const Vector2D& point, bool checkAngle)
{
	UNUSED(checkAngle);
	if (!isRotate) {
		return;
	}
#ifdef USE_QUATERNIONS
	Quaternion quat;
#endif
	if (point.y) {
		ForceChangeCamera();
		//Up
		Vector3D up = Up;
		float y = point.y;
#ifdef ORBITAL_CAMERA
		bool res = CheckOrbitalLimitRotation(y, checkAngle);
#endif
#ifdef USE_QUATERNIONS
		quat.CreateFromAxisAngle(Right, y * degrad);
		Up = Up * quat;
#ifdef ORBITAL_CAMERA
		orientation = orientation * quat;
		if (!res)
#endif
		{
			if (Up.y < 0.0f) {// Check angle to avoid camera revert
				Up = up;
			}
			else {
				Direction = Direction * quat;
			}
		}
#ifdef ORBITAL_CAMERA
		else {
			Direction = Direction * quat;
		}
#endif
#else
		Mat.RotateMatrix(Right, y);
		Up = Up * Mat;
		if (Up.y < 0) {
			Up = up;
		}
		else {
			Direction = Direction * Mat;
		}
#endif
	}
	if (point.x) {
		ForceChangeCamera();
		const Vector3D a = Vector3D::Y();
#ifdef USE_QUATERNIONS
		quat.CreateFromAxisAngle(a, degrad * point.x);
#ifdef ORBITAL_CAMERA
		orientation = orientation * quat;
#endif
		Direction = Direction * quat;
		Right = Right * quat;
#else
		Mat.RotateMatrix(a, point.x);
		Direction = Direction * Mat;
		Right = Right * Mat;
#endif
	}
}

//
void Camera::UpdateViewMatrix(float sign)
{
	Matrix[0] = Right.x;
	Matrix[4] = Right.y;
	Matrix[8] = Right.z;

	Matrix[1] = Up.x;
	Matrix[5] = Up.y;
	Matrix[9] = Up.z;
	Matrix[2] = sign * Direction.x;
	Matrix[6] = sign * Direction.y;
	Matrix[10] = sign * Direction.z;
#ifdef ORBITAL_CAMERA
	if (Camera::ORBIT_CAMERA == cameraType) {
		if (!orbitOffsetDistance || IsChangedOrbitDistance()) {
			orbitOffsetDistance = (Position - Target).Length();
		}
		Position = Target - Direction * orbitOffsetDistance;
	}
#endif
	Matrix[12] = - dotProduct(Right, Position);
	Matrix[13] = - dotProduct(Up, Position);
	Matrix[14] = -sign * dotProduct(Direction, Position);
}

//
void Camera::ResetDefaultVectors()
{
	Up = Vector3D::Y();
	Right = Vector3D::X();
	Direction = Vector3D::Z(); //crossProduct(Up, Right);
}

//
void Camera::ClearRotate()
{
	Matrix.ClearRotate();
	ResetDefaultVectors();
	ForceChangeCamera();
#ifdef ORBITAL_CAMERA
	accumPitchDegrees = 0.0f;
	orientation.LoadIdentity();
#endif
}

//
bool Camera::UpdateCamera(float sign)
{
	if (IsChangeCamera() && !IsLock()) {
		// Calculate Camera vectors
		Direction.Normalize();
		Up = crossProduct(Direction, Right);
		Up.Normalize();
		Right = crossProduct(Up, Direction);
		UpdateViewMatrix(sign);
		return true;
	}
	return false;
}

// set camera field of view
bool Camera::SetFov(float fov)
{
	assert(fov > 0.0f && "Invalid Value");
	Fov = fov;
	halfTanFov = tanf(degrad * fov * 0.5f);
	return true;
}

// Setup near plane clipping
bool Camera::SetzNear(float znear)
{
	zNear = znear;
	return true;
}

// Setup far plane clipping
bool Camera::SetzFar(float zfar)
{
	zFar = zfar;
	return true;
}

//
void Camera::SetTarget(const Vector3D& target)
{
	UNUSED(target);
#ifdef ORBITAL_CAMERA
	if (IsChangePosition()) {
		Target = target;
		ForceChangeOrbitDistance();
	}
#endif
}

//
void Camera::SetCameraType(uint type)
{
	UNUSED(type);
#ifdef ORBITAL_CAMERA
	cameraType = type;
	ForceChangeCamera();
#endif
}

//#define USE_REFLECT_MATRIX

//
void Camera::Reflect(const Camera& camera, Plane& plane, bool calcFrustum)
{
#ifdef USE_REFLECT_MATRIX
	Matrix = camera.GetViewMatrix();
#else
	memcpy(&Up, &camera.Up, 4 * sizeof(Vector3D));
#endif
	Reflect(plane, calcFrustum);
}

//
void Camera::Reflect(const Plane& plane, bool calcFrustum)
{
#ifdef USE_REFLECT_MATRIX
	const Matrix4x4 reflectMatrix = Matrix4x4(1.0f, 0.0f, 0.0f, 0.0f,
												0.0f, -1.0f, 0.0f, 0.0f,
												0.0f, 0.0f, 1.0f, 0.0f,
												0.0f, 0.0f, 0.0f, 1.0f);
	Matrix = Matrix * reflectMatrix;
#else
	plane.ReflectPos(Position);
	plane.ReflectDir(Direction);
	plane.ReflectDir(Right);
	plane.ReflectDir(Up);
	UpdateViewMatrix();
#endif
	ClearChangeCamera();
	if (calcFrustum) {
		ExtractFrustum();
	}
}
