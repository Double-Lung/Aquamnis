#include "Cafe.h"
#include <AM_EntityStorage.h>
#include <AM_VkRenderCore.h>

Cafe::~Cafe()
{
	delete myRenderCore;
}

void Cafe::Engage()
{
	myEntityStorage = new AM_EntityStorage();
	myRenderCore = new AM_VkRenderCore();
	LoadDefaultScene();
	Update();
}

void Cafe::Update()
{
	myRenderCore->MainLoop();
}

void Cafe::LoadDefaultScene()
{
	static const char* vikingRoomTextures[] = { "../data/textures/vikingroom.png" };
	AM_Entity* vikingRoom = myRenderCore->LoadEntity(vikingRoomTextures, "../data/models/vikingroom.obj", *myEntityStorage, AM_Entity::MESH);
	vikingRoom->myTranslation = { 12.f, 0.f, 0.f };

	AM_Entity* vase = myRenderCore->LoadEntity(nullptr, "../data/models/smooth_vase.obj", *myEntityStorage, AM_Entity::MESH);
	vase->myTranslation = { -8.f, 0.f, 0.f };
	vase->myScale = { 20.f, 20.f, 20.f };

	AM_Entity* quad = myRenderCore->LoadEntity(nullptr, "../data/models/quad.obj", *myEntityStorage, AM_Entity::MESH);
	quad->myTranslation = { 0.f, -1.f, 0.f };
	quad->myScale = { 42.f, 1.f, 42.f };

	static const char* CUBEMAP_TEXTURE_PATH[6] =
	{
		"../data/textures/cubemaps/Yokohama/posx.jpg",
		"../data/textures/cubemaps/Yokohama/negx.jpg",
		"../data/textures/cubemaps/Yokohama/posy.jpg",
		"../data/textures/cubemaps/Yokohama/negy.jpg",
		"../data/textures/cubemaps/Yokohama/posz.jpg",
		"../data/textures/cubemaps/Yokohama/negz.jpg"
	};
	AM_Entity* skybox = myRenderCore->LoadSkybox(CUBEMAP_TEXTURE_PATH, *myEntityStorage);

	AM_Entity* pointLight1 = myRenderCore->LoadEntity(nullptr, nullptr, *myEntityStorage, AM_Entity::BILLBOARD);
	pointLight1->myTranslation = { -5.f, 2.f, -.7f };
	pointLight1->SetIsEmissive(true);
	pointLight1->SetColor({ 1.f, 0.1f, 0.1f });
	pointLight1->SetLightIntensity(1.f);

	AM_Entity* pointLight2 = myRenderCore->LoadEntity(nullptr, nullptr, *myEntityStorage, AM_Entity::BILLBOARD);
	pointLight2->myTranslation = { -5.f, 2.f, .7f };
	pointLight2->SetIsEmissive(true);
}

