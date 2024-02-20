#pragma once
#include <AM_Window.h>

class AM_EntityStorage;
class AM_TempScene;
class AM_VkRenderCore;
class Cafe
{
public:
	Cafe() = default;
	~Cafe();
	void Engage();
private:
	void MainLoop();
	bool UpdateCameraTransform(float aDeltaTime);
	void LoadDefaultScene();

	AM_Window myWindowInstance;
	AM_EntityStorage* myEntityStorage;
	AM_VkRenderCore* myRenderCore;
	AM_TempScene* myDefaultScene;
};