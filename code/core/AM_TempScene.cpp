#include "AM_TempScene.h"

#include "AM_Camera.h"
#include "AM_Entity.h"
#include "AM_EntityStorage.h"

void AM_TempScene::UpdateUBO_Camera()
{
	myUBO.viewMat = myCamera->GetViewMatrix();
	myUBO.projectionMat = myCamera->GetProjectionMatrix();
	myUBO.invViewMat = myCamera->GetInverseViewMatrix();

	myShouldUpdateUniformBuffer = 0x3;
}

void AM_TempScene::UpdateUBO_PointLights(AM_EntityStorage& anEntityStorage)
{
	int numLights = 0;
	for (uint64_t id : myPointLights)
	{
		AM_Entity* pointLight = anEntityStorage.GetIfExist(id);
		float lightIntensity = pointLight->GetLightIntensity();
		if (!pointLight || !pointLight->IsEmissive() || lightIntensity < 0.0001f)
			continue;

		myUBO.pointLightData[numLights].position = glm::vec4(pointLight->myTranslation, 1.f);
		myUBO.pointLightData[numLights].color = glm::vec4(pointLight->GetColor(), pointLight->GetLightIntensity());
		++numLights;
	}

	myUBO.numPointLight = numLights;

	myShouldUpdateUniformBuffer = 0x3;
}

void AM_TempScene::UpdateUBO_DirectLighting(const glm::vec3& aLightDirection)
{
	myUBO.directLightDirection = aLightDirection;
	myShouldUpdateUniformBuffer = 0x3;
}

void AM_TempScene::UpdateUBO_AmbientColor(const glm::vec4& aColor)
{
	myUBO.ambientColor = aColor;
	myShouldUpdateUniformBuffer = 0x3;
}

void AM_TempScene::AddMeshObject(uint64_t anId)
{
	myMeshObjects.push_back(anId);
}

void AM_TempScene::AddPointLight(uint64_t anId)
{
	myPointLights.push_back(anId);
}

void AM_TempScene::AddSkybox(uint64_t anId)
{
	mySkybox = anId;
}

bool AM_TempScene::GetShouldUpdateUniformBuffer(uint32_t aFrameIndex) const
{
	return myShouldUpdateUniformBuffer & (0x1 << aFrameIndex);
}

void AM_TempScene::ResetUpdateFlag(uint32_t aFrameIndex)
{
	myShouldUpdateUniformBuffer &= ~(0x1 << aFrameIndex);
}
