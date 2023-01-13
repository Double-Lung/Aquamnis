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

	void Init(GLFWframebuffersizefun aFramebuffersizefun)
	{
		glfwSetWindowUserPointer(myWindow, this);
		glfwSetFramebufferSizeCallback(myWindow, aFramebuffersizefun);
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

	void GetFramebufferSize(int& aWidth, int& aHeight)
	{
		glfwGetFramebufferSize(myWindow, &aWidth, &aHeight);
	}
private:
	GLFWwindow* myWindow;
};