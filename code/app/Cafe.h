#pragma once

class AM_EntityStorage;
class AM_VkRenderCore;
class Cafe
{
public:
	Cafe() = default;
	~Cafe();
	void Engage();
private:
	void Update();
	void LoadDefaultScene();

	AM_EntityStorage* myEntityStorage;
	AM_VkRenderCore* myRenderCore;
};