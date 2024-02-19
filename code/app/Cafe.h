#pragma once

class Cafe
{
public:
	Cafe() = default;
	~Cafe();
	void Engage();
private:
	void Update();
	void LoadDefaultScene();

	AM_VkRenderCore* myRenderCore;
};