#include "AM_TempScene.h"
#include "AM_Camera.h"
#include "AM_EntityStorage.h"

void AM_TempScene::UpdateUBO_Camera()
{
	myUBO.viewMat = myCamera->GetViewMatrix();
	myUBO.invViewMat = myCamera->GetInverseViewMatrix();
	myUBO.projectionMat = myCamera->GetProjectionMatrix();

	myShouldUpdateUniformBuffer = true;
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

		myUBO.pointLightData[numLights].position = pointLight->myTranslation;
		myUBO.pointLightData[numLights].color = glm::vec4(pointLight->GetColor(), pointLight->GetLightIntensity());
		++numLights;
	}

	myUBO.numPointLight = numLights;

	myShouldUpdateUniformBuffer = true;
}

void AM_TempScene::UpdateUBO_DirectLighting(const glm::vec3& aLightDirection)
{
	myUBO.directLightDirection = aLightDirection;
	myShouldUpdateUniformBuffer = true;
}

void AM_TempScene::UpdateUBO_AmbientColor(const glm::vec4& aColor)
{
	myUBO.ambientColor = aColor;
	myShouldUpdateUniformBuffer = true;
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
