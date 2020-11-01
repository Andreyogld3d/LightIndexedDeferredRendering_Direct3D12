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
Camera::Camera() : Speed(8.0f), aspect(1.0f),
					zNear(0.16f), zFar(2000.0f), Fov(45.0f), halfTanFov(0.0f),
					CameraHeight(3.5f)
{
	Up = Vector3D::Y();
	Position.x = 20.0f;
	Position.z = -10.0f;
	Position.y = -0.7f;
	Right = Vector3D::X();
	Direction = Vector3D::Z();
	Matrix.LoadIdentity();
	proj.LoadIdentity();
}

// Move camera
void Camera::Move(int key, float speed)
{
	const float k  = 1.0f / 100.0f;
	speed *= 1000.0f;
	// Calc Coords
	float dz = Speed * speed * k;
	float dx = Speed * speed * k;
	oldPos = Position;
	//
	if (MOVE_LEFT == key) {
		Position.x -= Right.x * dx;
		Position.y -= Right.y * dx;
		Position.z -= Right.z * dx;
	}
	else if (MOVE_RIGHT == key) {
		Position.x += Right.x * dx;
		Position.y += Right.y * dx;
		Position.z += Right.z * dx;
	}
	else if (MOVE_BACK == key) {
		Position.x -= (Direction.x * dx);
		Position.y -= (Direction.y * dx);
		Position.z -= (Direction.z * dz);
	}
	else if (MOVE_FORWARD == key) {
		Position.x += (Direction.x * dx);
		Position.y += (Direction.y * dx);
		Position.z += (Direction.z * dz);
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

// Rotate camera
void Camera::Rotate(const Vector2D& point, bool checkAngle)
{
#ifdef USE_QUATERNIONS
	Quaternion quat;
#endif
	if (point.y) {
		//Up
		Vector3D up = Up;
		float y = point.y;
#ifdef USE_QUATERNIONS
		quat.CreateFromAxisAngle(Right, y * degrad);
		Up = Up * quat;
		{
			if (Up.y < 0.0f) {// Check angle to avoid camera revert
				Up = up;
			}
			else {
				Direction = Direction * quat;
			}
		}
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
		const Vector3D a = Vector3D::Y();
#ifdef USE_QUATERNIONS
		quat.CreateFromAxisAngle(a, degrad * point.x);
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
}

//
void Camera::UpdateCamera(float sign)
{
	// Calculate Camera vectors
	Direction.Normalize();
	Up = crossProduct(Direction, Right);
	Up.Normalize();
	Right = crossProduct(Up, Direction);
	UpdateViewMatrix(sign);
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
#ifdef ORBITAL_CAMERA
	if (IsChangePosition()) {
		Target = target;
		ForceChangeOrbitDistance();
	}
#endif
}
