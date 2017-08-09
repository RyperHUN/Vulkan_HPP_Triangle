// Vulkan Particle.cpp : Defines the entry point for the console application.
//

#include <vulkan/vulkan.h>
#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

#include "VulkanBase.h"

// Windows entry point
VulkanExampleBase *vulkanExample;
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (vulkanExample != NULL)
	{
		vulkanExample->handleMessages(hWnd, uMsg, wParam, lParam);
	}
	return (DefWindowProc(hWnd, uMsg, wParam, lParam));
}
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow)
{
	/*vulkanExample = new VulkanExampleBase();
	vulkanExample->initVulkan();
	vulkanExample->setupWindow(hInstance, WndProc);
	vulkanExample->initSwapchain();
	vulkanExample->prepare();
	vulkanExample->renderLoop();
	delete(vulkanExample);*/
	return 0;
}

