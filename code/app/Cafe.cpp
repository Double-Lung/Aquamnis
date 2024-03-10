#include "Cafe.h"

#include "ApplicationConstants.h"
#include <AM_Camera.h>
#include <AM_Entity.h>
#include <AM_EntityStorage.h>
#include <AM_SimpleTimer.h>
#include <AM_TempScene.h>
#include <AM_VkRenderCore.h>
#include <glm/glm.hpp>
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_vulkan.h"

Cafe::Cafe()
	: myWindowInstance{}
	, myRenderCore{ nullptr }
	, myEntityStorage{ nullptr }
	, myMainCamera{ nullptr }
	, myDefaultScene{ nullptr }
{
}

Cafe::~Cafe()
{
	delete myMainCamera;
	delete myEntityStorage;
	delete myDefaultScene;
	delete myRenderCore;
}

void Cafe::Engage()
{
	InitWindow();
	myRenderCore = new AM_VkRenderCore(myWindowInstance);
	myRenderCore->Setup();
	myRenderCore->CreateImguiContext();
	LoadDefaultScene();
	MainLoop();
	CleanUp();
}

void Cafe::InitWindow()
{
	AM_WindowCreateInfo info{};
	info.width = ApplicationConstants::MIN_WIDTH;
	info.height = ApplicationConstants::MIN_HEIGHT;
	info.minWidth = ApplicationConstants::MIN_WIDTH;
	info.minHeight = ApplicationConstants::MIN_HEIGHT;
	info.maxWidth = ApplicationConstants::MAX_WIDTH;
	info.maxHeight = ApplicationConstants::MAX_HEIGHT;
	info.windowName = ApplicationConstants::WINDOWNAME;

	myWindowInstance.Init(info);
}

void Cafe::MainLoop()
{
	SDL_Event event;
	bool shouldKeepWindow = true;
	while (shouldKeepWindow)
	{
		while (SDL_PollEvent(&event) != 0)
		{
			if (event.type == SDL_EventType::SDL_EVENT_QUIT)
			{
				shouldKeepWindow = false;
				break;
			}

			if (event.type == SDL_EventType::SDL_EVENT_WINDOW_RESIZED)
				myWindowInstance.SetFramebufferResized();

			ImGui_ImplSDL3_ProcessEvent(&event);
			SetCameraVelocity(event);
		}

		float deltaTime = AM_SimpleTimer::GetInstance().GetDeltaTime();

		bool cameraUpdated = UpdateCameraTransform(deltaTime);
		if (cameraUpdated)
			myDefaultScene->UpdateUBO_Camera();

		// entity update logic should be here
		// it is not needed right now since the scene is fully static

		myRenderCore->Render(*myMainCamera, *myDefaultScene, *myEntityStorage);

		if (myWindowInstance.ShouldUpdateCamera())
		{
			int width, height;
			myWindowInstance.GetFramebufferSize(width, height);
			float aspectRatio = (float)width / (float)height;

			myMainCamera->SetPerspectiveProjection(0.7854f, aspectRatio, 0.1f, 200.f);
			myDefaultScene->UpdateUBO_Camera();

			auto& pointLightIds = myDefaultScene->GetPointLights();
			for (uint64_t id : pointLightIds)
			{
				AM_Entity* light = myEntityStorage->GetIfExist(id);
				float newScale = 1.f / ((float)width / (float)ApplicationConstants::MIN_WIDTH) * 0.1f;
				light->myScale = { newScale, newScale, newScale };
				light->UpdateUBO_Transform();
			}
			myWindowInstance.ResetCameraUpdateFlag();
		}
	}
}

void Cafe::CleanUp()
{
	myRenderCore->OnEnd();
	myRenderCore->DestroyEntities(*myEntityStorage);
	myRenderCore->DestroyScene(*myDefaultScene);
	myRenderCore->DestroyImguiContext();
}

bool Cafe::UpdateCameraTransform(float aDeltaTime)
{
	bool rotated = glm::dot(myMainCamera->myRotationDir, myMainCamera->myRotationDir) > 0.0001f;
	if (rotated)
	{
		myMainCamera->myRotation += 1.5f * aDeltaTime * glm::normalize(myMainCamera->myRotationDir);
		myMainCamera->myRotation.x = glm::clamp(myMainCamera->myRotation.x, -1.5f, 1.5f);
		myMainCamera->myRotation.y = glm::mod(myMainCamera->myRotation.y, glm::two_pi<float>());
	}

	bool moved = glm::dot(myMainCamera->myVelocity, myMainCamera->myVelocity) > 0.0001f;
	if (moved)
	{
		const float yaw = myMainCamera->myRotation.y;
		glm::vec3 forwardDir{ -sin(yaw), 0.f, -cos(yaw) }; // camera is facing -z axis by default
		glm::vec3 rightDir{ -forwardDir.z, 0.f, forwardDir.x };
		glm::vec3 upDir{ 0.f, 1.f, 0.f };
		glm::vec3 translationDelta{0.f};
		translationDelta += forwardDir * myMainCamera->myVelocity.z;
		translationDelta += upDir * myMainCamera->myVelocity.y;
		translationDelta += rightDir * myMainCamera->myVelocity.x;
		myMainCamera->myTranslation += 5.f * aDeltaTime * glm::normalize(translationDelta);
	}
 	
	if (rotated || moved)
	{
		myMainCamera->SetRotation(myMainCamera->myTranslation, myMainCamera->myRotation);
		return true;
	}
	return false;
}

void Cafe::LoadDefaultScene()
{
	myDefaultScene = new AM_TempScene();
	myRenderCore->InitScene(*myDefaultScene);

	myMainCamera = new AM_Camera();
	myMainCamera->myTranslation = { 0.f, 15.f, 35.f };
	myMainCamera->myRotation = { 0.f, 0.f, 0.f };

	int width, height;
	myWindowInstance.GetFramebufferSize(width, height);
	myMainCamera->SetPerspectiveProjection(0.7854f, (float)width / (float)height, 0.1f, 100.f);
	myMainCamera->SetRotation(myMainCamera->myTranslation, myMainCamera->myRotation);
	myDefaultScene->SetCamera(myMainCamera);

	myEntityStorage = new AM_EntityStorage();
	static const char* vikingRoomTextures[] = { "../data/textures/vikingroom.png" };
	AM_Entity* vikingRoom = myRenderCore->LoadEntity(vikingRoomTextures, "../data/models/vikingroom.obj", *myEntityStorage, AM_Entity::MESH);
	vikingRoom->myTranslation = { 0.f, 0.f, 0.f };
	vikingRoom->UpdateUBO_Transform();
	myDefaultScene->AddMeshObject(vikingRoom->GetId());

	AM_Entity* vase = myRenderCore->LoadEntity(nullptr, "../data/models/smooth_vase.obj", *myEntityStorage, AM_Entity::MESH);
	vase->myTranslation = { 0.f, 3.f, 3.f };
	vase->myRotation = { 3.14159f, 0.f, 0.f };
	vase->myScale = { 22.f, 22.f, 22.f };

	vase->UpdateUBO_Transform();
	myDefaultScene->AddMeshObject(vase->GetId());

	AM_Entity* quad = myRenderCore->LoadEntity(nullptr, "../data/models/quad.obj", *myEntityStorage, AM_Entity::MESH);
	quad->myTranslation = { 0.f, -1.5f, 0.f };
	quad->myScale = { 42.f, 1.f, 42.f };
	quad->UpdateUBO_Transform();
	myDefaultScene->AddMeshObject(quad->GetId());

	const char* CUBEMAP_TEXTURE_PATH[6] =
	{
		"../data/textures/cubemaps/Yokohama/posx.jpg",
		"../data/textures/cubemaps/Yokohama/negx.jpg",
		"../data/textures/cubemaps/Yokohama/posy.jpg",
		"../data/textures/cubemaps/Yokohama/negy.jpg",
		"../data/textures/cubemaps/Yokohama/posz.jpg",
		"../data/textures/cubemaps/Yokohama/negz.jpg"
	};

	AM_Entity* skybox = myRenderCore->LoadSkybox(CUBEMAP_TEXTURE_PATH, *myEntityStorage);
	myDefaultScene->AddSkybox(skybox->GetId());

	AM_Entity* pointLight1 = myRenderCore->LoadEntity(nullptr, nullptr, *myEntityStorage, AM_Entity::BILLBOARD);
	pointLight1->myTranslation = { -5.f, 12.f, 5.f };
	pointLight1->myScale = { .1f, .1f, .1f };
	pointLight1->SetIsEmissive(true);
	pointLight1->SetTransparency(true);
	pointLight1->SetColor({ 1.f, 0.1f, 0.1f });
	pointLight1->SetLightIntensity(1.f);
	pointLight1->UpdateUBO_Transform();
	pointLight1->UpdateUBO_Color();

 	AM_Entity* pointLight2 = myRenderCore->LoadEntity(nullptr, nullptr, *myEntityStorage, AM_Entity::BILLBOARD);
 	pointLight2->myTranslation = { 5.f, 8.f, 0.f };
 	pointLight2->myScale = { .1f, .1f, .1f };
 	pointLight2->SetIsEmissive(true);
 	pointLight2->SetTransparency(true);
 	pointLight2->SetColor({ 1.f, 1.0f, 0.1f });
 	pointLight2->SetLightIntensity(1.f);
 	pointLight2->UpdateUBO_Transform();
 	pointLight2->UpdateUBO_Color();

	myDefaultScene->AddPointLight(pointLight1->GetId());
	myDefaultScene->AddPointLight(pointLight2->GetId());

	myDefaultScene->UpdateUBO_DirectLighting({ 1.f, 1.f, 1.f });
	myDefaultScene->UpdateUBO_AmbientColor({ 1.f, 1.f, 1.f, 0.03f });
	myDefaultScene->UpdateUBO_Camera();
	myDefaultScene->UpdateUBO_PointLights(*myEntityStorage);
}

void Cafe::SetCameraVelocity(SDL_Event& anEvent)
{
	glm::vec3& velocity = myMainCamera->myVelocity;
	glm::vec3& rotation = myMainCamera->myRotationDir;
	if (anEvent.type == SDL_EVENT_KEY_DOWN)
	{
		if (anEvent.key.keysym.sym == SDLK_w) { velocity.z = 1.f; }
		if (anEvent.key.keysym.sym == SDLK_s) { velocity.z = -1.f; }
		if (anEvent.key.keysym.sym == SDLK_a) { velocity.x = -1.f; }
		if (anEvent.key.keysym.sym == SDLK_d) { velocity.x = 1.f; }
		if (anEvent.key.keysym.sym == SDLK_q) { velocity.y = 1.f; }
		if (anEvent.key.keysym.sym == SDLK_e) { velocity.y = -1.f; }
		if (anEvent.key.keysym.sym == SDLK_LEFT) { rotation.y = 1.f; }
		if (anEvent.key.keysym.sym == SDLK_RIGHT) { rotation.y = -1.f; }
		if (anEvent.key.keysym.sym == SDLK_UP) { rotation.x = 1.f; }
		if (anEvent.key.keysym.sym == SDLK_DOWN) { rotation.x = -1.f; }
    }

    if (anEvent.type == SDL_EVENT_KEY_UP) {
		if (anEvent.key.keysym.sym == SDLK_w) { velocity.z = 0.f; }
		if (anEvent.key.keysym.sym == SDLK_s) { velocity.z = 0.f; }
		if (anEvent.key.keysym.sym == SDLK_a) { velocity.x = 0.f; }
		if (anEvent.key.keysym.sym == SDLK_d) { velocity.x = 0.f; }
		if (anEvent.key.keysym.sym == SDLK_q) { velocity.y = 0.f; }
		if (anEvent.key.keysym.sym == SDLK_e) { velocity.y = 0.f; }
		if (anEvent.key.keysym.sym == SDLK_LEFT) { rotation.y = 0.f; }
		if (anEvent.key.keysym.sym == SDLK_RIGHT) { rotation.y = 0.f; }
		if (anEvent.key.keysym.sym == SDLK_UP) { rotation.x = 0.f; }
		if (anEvent.key.keysym.sym == SDLK_DOWN) { rotation.x = 0.f; }
    }
}

