#include "AM_Camera.h"

#include <glm/gtc/matrix_transform.hpp>
#include <cassert>

AM_Camera::AM_Camera()
	: myTranslation{}
	, myRotation{}
	, myProjectionMatrix{1.0f}
	, myViewMatrix{1.0f}
	, myInverseViewMatrix{1.f}
	, myVelocity{}
	, myRotationDir{}
{
}

void AM_Camera::SetOrthoProjection(float aLeft, float aRight, float aTop, float aBottom, float aNear, float aFar)
{
	myProjectionMatrix = glm::orthoZO(aLeft, aRight, aBottom, aTop, aNear, aFar);
}

void AM_Camera::SetPerspectiveProjection(float anFOVY, float anAspectRatio, float aNear, float aFar)
{
	myProjectionMatrix = glm::perspective(anFOVY, anAspectRatio, aNear, aFar);
}

// void AM_Camera::SetLookAt(const glm::vec3& anEyePos, const glm::vec3& aTargetPos, const glm::vec3& anUpVector)
// {
// 	myViewMatrix = glm::lookAt(anEyePos, aTargetPos, anUpVector);
// }

// void AM_Camera::SetDirection(const glm::vec3& anEyePos, const glm::vec3& aDirection, const glm::vec3& anUpVector)
// {
// 	const glm::vec3 f{ glm::normalize(aDirection) };
// 	const glm::vec3 s{ glm::normalize(glm::cross(f, anUpVector)) };
// 	const glm::vec3 u{ glm::cross(s, f) };
// 
// 	myViewMatrix = glm::mat4{ 1.f };
// 	myViewMatrix[0][0] = s.x;
// 	myViewMatrix[1][0] = s.y;
// 	myViewMatrix[2][0] = s.z;
// 	myViewMatrix[0][1] = u.x;
// 	myViewMatrix[1][1] = u.y;
// 	myViewMatrix[2][1] = u.z;
// 	myViewMatrix[0][2] = -f.x;
// 	myViewMatrix[1][2] = -f.y;
// 	myViewMatrix[2][2] = -f.z;
// 	myViewMatrix[3][0] = -glm::dot(s, anEyePos);
// 	myViewMatrix[3][1] = -glm::dot(u, anEyePos);
// 	myViewMatrix[3][2] = glm::dot(f, anEyePos);
// }

void AM_Camera::SetRotation(const glm::vec3& anEyePos, const glm::vec3& aRotation)
{
	const float c3 = glm::cos(aRotation.z);
	const float s3 = glm::sin(aRotation.z);
	const float c2 = glm::cos(aRotation.x);
	const float s2 = glm::sin(aRotation.x);
	const float c1 = glm::cos(aRotation.y);
	const float s1 = glm::sin(aRotation.y);
	const glm::vec3 s{ (c1 * c3 + s1 * s2 * s3), (c2 * s3), (c1 * s2 * s3 - c3 * s1) };
	const glm::vec3 u{ (c3 * s1 * s2 - c1 * s3), (c2 * c3), (c1 * c3 * s2 + s1 * s3) };
	const glm::vec3 f{ (c2 * s1), (-s2), (c1 * c2) };
	myViewMatrix = glm::mat4{ 1.f };
	myViewMatrix[0][0] = s.x;
	myViewMatrix[1][0] = s.y;
	myViewMatrix[2][0] = s.z;
	myViewMatrix[0][1] = u.x;
	myViewMatrix[1][1] = u.y;
	myViewMatrix[2][1] = u.z;
	myViewMatrix[0][2] = f.x;
	myViewMatrix[1][2] = f.y;
	myViewMatrix[2][2] = f.z;
	myViewMatrix[3][0] = -glm::dot(s, anEyePos);
	myViewMatrix[3][1] = -glm::dot(u, anEyePos);
	myViewMatrix[3][2] = -glm::dot(f, anEyePos);

	myInverseViewMatrix = glm::mat4{ 1.f };
	myInverseViewMatrix[0][0] = s.x;
	myInverseViewMatrix[0][1] = s.y;
	myInverseViewMatrix[0][2] = s.z;
	myInverseViewMatrix[1][0] = u.x;
	myInverseViewMatrix[1][1] = u.y;
	myInverseViewMatrix[1][2] = u.z;
	myInverseViewMatrix[2][0] = f.x;
	myInverseViewMatrix[2][1] = f.y;
	myInverseViewMatrix[2][2] = f.z;
	myInverseViewMatrix[3][0] = anEyePos.x;
	myInverseViewMatrix[3][1] = anEyePos.y;
	myInverseViewMatrix[3][2] = anEyePos.z;
}
