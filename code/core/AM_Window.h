#pragma once
#include <GLFW/glfw3.h>
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
		glfwDestroyWindow(myWindow);
		glfwTerminate();

		SDL_DestroyWindow(mySDLWindow);
	}

	void Init(AM_WindowCreateInfo& someInfo)
	{
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
		myWindow = glfwCreateWindow(someInfo.width, someInfo.height, someInfo.windowName, nullptr, nullptr);

		glfwSetWindowUserPointer(myWindow, this);
		glfwSetFramebufferSizeCallback(myWindow, FramebufferResizeCallback);
		glfwSetWindowSizeLimits
		(
			myWindow,
			someInfo.minWidth,
			someInfo.minHeight,
			someInfo.maxWidth,
			someInfo.maxHeight
		);

		SDL_Init(SDL_INIT_VIDEO);
		mySDLWindow = SDL_CreateWindow(
			someInfo.windowName,
			someInfo.minWidth,
			someInfo.minHeight,
			SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE
		);
	}

	bool ShouldCloseWindow()
	{
		return glfwWindowShouldClose(myWindow);
	}

	void WaitForFramebufferSize(int& aWidth, int& aHeight)
	{
		glfwGetFramebufferSize(myWindow, &aWidth, &aHeight);
		while (!(aWidth & aHeight))
		{
			glfwGetFramebufferSize(myWindow, &aWidth, &aHeight);
			glfwWaitEvents();
		}
	}

	GLFWwindow* GetWindow() const { return myWindow; }

	bool WasWindowResized() const { return myIsFramebufferResized; }
	void ResetResizeFlag() { myIsFramebufferResized = false; }
	bool ShouldUpdateCamera() const { return myShouldUpdateCamera; }
	void ResetCameraUpdateFlag() { myShouldUpdateCamera = false; }

	void GetFramebufferSize(int& aWidth, int& aHeight)
	{
		glfwGetFramebufferSize(myWindow, &aWidth, &aHeight);
	}
private:
	static void FramebufferResizeCallback(GLFWwindow* window, int width, int height)
	{
		AM_Window* amWindow = reinterpret_cast<AM_Window*>(glfwGetWindowUserPointer(window));
		amWindow->myIsFramebufferResized = true;
		amWindow->myShouldUpdateCamera = true;
	}

	bool myIsFramebufferResized;
	bool myShouldUpdateCamera;
	GLFWwindow* myWindow;
	SDL_Window* mySDLWindow = nullptr;
};