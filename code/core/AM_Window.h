#pragma once
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

struct AM_WindowCreateInfo
{
	int width;
	int height;
	int minWidth;
	int minHeight;
	int maxWidth;
	int maxHeight;
	const char* windowName;
};

class AM_Window
{
public:
	AM_Window()
		: myWindow(nullptr)
		, myIsFramebufferResized(false)
		, myShouldUpdateCamera(false)
	{
	}

	~AM_Window()
	{
		SDL_DestroyWindow(myWindow);
	}

	void Init(AM_WindowCreateInfo& someInfo)
	{
		SDL_Init(SDL_INIT_VIDEO);
		myWindow = SDL_CreateWindow(
			someInfo.windowName,
			someInfo.minWidth,
			someInfo.minHeight,
			SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE
		);
	}

	void WaitForFramebufferSize(int& aWidth, int& aHeight)
	{
		SDL_GetWindowSizeInPixels(myWindow, &aWidth, &aHeight);
		while (aWidth == 0 || aHeight == 0)
		{
			SDL_GetWindowSizeInPixels(myWindow, &aWidth, &aHeight);
			SDL_WaitEvent(nullptr);
		}
	}

	SDL_Window* GetSDLWindow() const { return myWindow; }

	bool WasWindowResized() const { return myIsFramebufferResized; }
	void ResetResizeFlag() { myIsFramebufferResized = false; }
	bool ShouldUpdateCamera() const { return myShouldUpdateCamera; }
	void ResetCameraUpdateFlag() { myShouldUpdateCamera = false; }

	void GetFramebufferSize(int& aWidth, int& aHeight)
	{
		SDL_GetWindowSizeInPixels(myWindow, &aWidth, &aHeight);
	}

	void SetFramebufferResized()
	{
		myIsFramebufferResized = true;
		myShouldUpdateCamera = true;
	}
private:
	bool myIsFramebufferResized;
	bool myShouldUpdateCamera;
	SDL_Window* myWindow = nullptr;
};