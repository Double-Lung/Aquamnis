#include "Cafe.h"
#include <AM_VkRenderCore.h>

Cafe::~Cafe()
{
	delete myRenderCore;
}

void Cafe::Engage()
{
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
	// load entities
}

