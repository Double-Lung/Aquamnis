#include "AM_Camera.h"
#include <cassert>
#include <glm/gtc/matrix_transform.hpp>

AM_Camera::AM_Camera()
	: myProjectionMatrix{1.0f}
	, myViewMatrix{1.0f}
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

void AM_Camera::SetLookAt(const glm::vec3& anEyePos, const glm::vec3& aTargetPos, const glm::vec3& anUpVector)
{
	myViewMatrix = glm::lookAt(anEyePos, aTargetPos, anUpVector);
}

void AM_Camera::SetDirection(const glm::vec3& anEyePos, const glm::vec3& aDirection, const glm::vec3& anUpVector)
{
	const glm::vec3 w{ glm::normalize(aDirection) };
	const glm::vec3 u{ glm::normalize(glm::cross(w, anUpVector)) };
	const glm::vec3 v{ glm::cross(w, u) };

	myViewMatrix = glm::mat4{ 1.f };
	myViewMatrix[0][0] = u.x;
	myViewMatrix[1][0] = u.y;
	myViewMatrix[2][0] = u.z;
	myViewMatrix[0][1] = v.x;
	myViewMatrix[1][1] = v.y;
	myViewMatrix[2][1] = v.z;
	myViewMatrix[0][2] = w.x;
	myViewMatrix[1][2] = w.y;
	myViewMatrix[2][2] = w.z;
	myViewMatrix[3][0] = -glm::dot(u, anEyePos);
	myViewMatrix[3][1] = -glm::dot(v, anEyePos);
	myViewMatrix[3][2] = -glm::dot(w, anEyePos);
}

void AM_Camera::SetRotation(const glm::vec3& anEyePos, const glm::vec3& aRotation)
{
	const float c3 = glm::cos(aRotation.z);
	const float s3 = glm::sin(aRotation.z);
	const float c2 = glm::cos(aRotation.x);
	const float s2 = glm::sin(aRotation.x);
	const float c1 = glm::cos(aRotation.y);
	const float s1 = glm::sin(aRotation.y);
	const glm::vec3 u{ (c1 * c3 + s1 * s2 * s3), (c2 * s3), (c1 * s2 * s3 - c3 * s1) };
	const glm::vec3 v{ (c3 * s1 * s2 - c1 * s3), (c2 * c3), (c1 * c3 * s2 + s1 * s3) };
	const glm::vec3 w{ (c2 * s1), (-s2), (c1 * c2) };
	myViewMatrix = glm::mat4{ 1.f };
	myViewMatrix[0][0] = u.x;
	myViewMatrix[1][0] = u.y;
	myViewMatrix[2][0] = u.z;
	myViewMatrix[0][1] = v.x;
	myViewMatrix[1][1] = v.y;
	myViewMatrix[2][1] = v.z;
	myViewMatrix[0][2] = w.x;
	myViewMatrix[1][2] = w.y;
	myViewMatrix[2][2] = w.z;
	myViewMatrix[3][0] = -glm::dot(u, anEyePos);
	myViewMatrix[3][1] = -glm::dot(v, anEyePos);
	myViewMatrix[3][2] = -glm::dot(w, anEyePos);
}
