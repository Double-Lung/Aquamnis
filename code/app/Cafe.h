#pragma once
#include <AM_Window.h>

class AM_Camera;
class AM_EntityStorage;
class AM_TempScene;
class AM_VkRenderCore;

class Cafe
{
public:
	Cafe();
	~Cafe();
	void Engage();
private:
	void InitWindow();
	void MainLoop();
	void CleanUp();
	bool UpdateCameraTransform(float aDeltaTime);
	void LoadDefaultScene();
	void SetCameraVelocity(SDL_Event& anEvent);

	AM_Window myWindowInstance;
	AM_VkRenderCore* myRenderCore;
	AM_EntityStorage* myEntityStorage;
	AM_Camera* myMainCamera;
	AM_TempScene* myDefaultScene;
};