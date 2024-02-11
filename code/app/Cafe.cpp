#include "Cafe.h"
#include <AM_VkRenderCore.h>

void Cafe::Engage()
{
	AM_VkRenderCore* vkRenderCore = new AM_VkRenderCore();
	vkRenderCore->Setup();
	vkRenderCore->MainLoop();
	delete vkRenderCore;
}

