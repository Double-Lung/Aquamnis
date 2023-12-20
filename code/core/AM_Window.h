#pragma once
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include "ApplicationConstants.h"
#include <GLFW/glfw3.h>

class AM_Window
{
public:
	AM_Window()
		: myWindow(nullptr)
		, myIsFramebufferResized(false)
		, myShouldUpdateCamera(false)
	{
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
		myWindow = glfwCreateWindow(ApplicationConstants::MIN_WIDTH,
			ApplicationConstants::MIN_HEIGHT,
			ApplicationConstants::WINDOWNAME,
			nullptr, nullptr);
	}

	~AM_Window()
	{
		glfwDestroyWindow(myWindow);
		glfwTerminate();
	}

	void Init()
	{
		glfwSetWindowUserPointer(myWindow, this);
		glfwSetFramebufferSizeCallback(myWindow, FramebufferResizeCallback);
		glfwSetWindowSizeLimits
		(
			myWindow,
			ApplicationConstants::MIN_WIDTH,
			ApplicationConstants::MIN_HEIGHT,
			ApplicationConstants::MAX_WIDTH,
			ApplicationConstants::MAX_HEIGHT
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
};