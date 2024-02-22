#pragma once
#include <glm/glm.hpp>

class AM_Camera
{
public:
	AM_Camera();
	~AM_Camera(){}

	void SetOrthoProjection(float aLeft, float aRight, float aTop, float aBottom, float aNear, float aFar);
	void SetPerspectiveProjection(float anFOVY, float anAspectRatio, float aNear, float aFar);
	//void SetLookAt(const glm::vec3& anEyePos, const glm::vec3& aTargetPos, const glm::vec3& anUpVector);
	//void SetDirection(const glm::vec3& anEyePos, const glm::vec3& aDirection, const glm::vec3& anUpVector);
	void SetRotation(const glm::vec3& anEyePos, const glm::vec3& aRotation);

	const glm::mat4& GetProjectionMatrix() const { return myProjectionMatrix; }
	const glm::mat4& GetViewMatrix() const { return myViewMatrix; }
	const glm::mat4& GetInverseViewMatrix() const { return myInverseViewMatrix; }
	glm::vec3 GetPosition() const { return myInverseViewMatrix[3]; }

	glm::vec3 myTranslation;
	glm::vec3 myRotation;

private:
	glm::mat4 myProjectionMatrix;
	glm::mat4 myViewMatrix;
	glm::mat4 myInverseViewMatrix;
};

