/*
* Vulkan Example base class
*
* Copyright (C) 2016 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#include "VulkanBase.h"
#include "VulkanInitializers.h"

std::vector<const char*> VulkanExampleBase::args;

void VulkanExampleBase::createInstance(bool enableValidation)
{
	this->settings.validation = enableValidation;

	// Validation can also be forced via a define
#if defined(_VALIDATION)
	this->settings.validation = true;
#endif	

	vk::ApplicationInfo appInfo;
	appInfo.setPApplicationName (name.c_str())
		.setPEngineName (name.c_str())
		.setApiVersion (VK_API_VERSION_1_0);

	std::vector<const char*> instanceExtensions = { VK_KHR_SURFACE_EXTENSION_NAME };

	// Enable surface extensions depending on os
	instanceExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);

	vk::InstanceCreateInfo instanceCreateInfo {};
	instanceCreateInfo.pApplicationInfo = &appInfo;
	if (instanceExtensions.size() > 0)
	{
		if (settings.validation)
		{
			instanceExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
		}
		instanceCreateInfo.enabledExtensionCount = (uint32_t)instanceExtensions.size();
		instanceCreateInfo.setPpEnabledExtensionNames(instanceExtensions.data());
	}
	///TODO
	//if (settings.validation)
	//{
	//	instanceCreateInfo.enabledLayerCount = vks::debug::validationLayerCount;
	//	instanceCreateInfo.ppEnabledLayerNames = vks::debug::validationLayerNames;
	//}
	instance = CHECK(vk::createInstance (instanceCreateInfo, nullptr));
}

std::string VulkanExampleBase::getWindowTitle()
{
	std::string device(vulkanDevice->properties.deviceName);
	std::string windowTitle;
	windowTitle = title + " - " + device;
	///TODO
	//if (!enableTextOverlay)
	//{
	//	windowTitle += " - " + std::to_string(frameCounter) + " fps";
	//}
	return windowTitle;
}


const std::string VulkanExampleBase::getAssetPath()
{
	///TODO
	return "./../data/";
}

bool VulkanExampleBase::checkCommandBuffers()
{
	for (auto& cmdBuffer : drawCmdBuffers)
	{
		if (!cmdBuffer)
		{
			return false;
		}
	}
	return true;
}

void VulkanExampleBase::createCommandBuffers()
{
	// Create one command buffer for each swap chain image and reuse for rendering

	vk::CommandBufferAllocateInfo cmdBufAllocateInfo;
	cmdBufAllocateInfo.setCommandPool (cmdPool)
		.setLevel (vk::CommandBufferLevel::ePrimary)
		.setCommandBufferCount (swapChain.imageCount);

	drawCmdBuffers = CHECK(vulkanDevice->D().allocateCommandBuffers (cmdBufAllocateInfo));
}

void VulkanExampleBase::destroyCommandBuffers()
{
	vulkanDevice->D().freeCommandBuffers (cmdPool, drawCmdBuffers);
}

VkCommandBuffer VulkanExampleBase::createCommandBuffer(VkCommandBufferLevel level, bool begin)
{
	VkCommandBuffer cmdBuffer;

	VkCommandBufferAllocateInfo cmdBufAllocateInfo =
		vks::initializers::commandBufferAllocateInfo(
			cmdPool,
			level,
			1);

	VK_CHECK_RESULT(vkAllocateCommandBuffers(vulkanDevice->GetDevice(), &cmdBufAllocateInfo, &cmdBuffer));

	// If requested, also start the new command buffer
	if (begin)
	{
		VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();
		VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo));
	}

	return cmdBuffer;
}

void VulkanExampleBase::flushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, bool free)
{
	if (commandBuffer == VK_NULL_HANDLE)
	{
		return;
	}

	VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
	VK_CHECK_RESULT(vkQueueWaitIdle(queue));

	if (free)
	{
		vkFreeCommandBuffers(vulkanDevice->GetDevice(), cmdPool, 1, &commandBuffer);
	}
}

void VulkanExampleBase::createPipelineCache()
{
	vk::PipelineCacheCreateInfo pipelineCacheCreateInfo = {};
	pipelineCache = CHECK(vulkanDevice->D().createPipelineCache (pipelineCacheCreateInfo));
}

void VulkanExampleBase::prepare()
{
	///TODO
	//if (vulkanDevice->enableDebugMarkers)
	//{
	//	vks::debugmarker::setup(device);
	//}
	createCommandPool();
	setupSwapChain();
	createCommandBuffers();
	setupDepthStencil();
	setupRenderPass();
	createPipelineCache();
	setupFrameBuffer();

}

VkPipelineShaderStageCreateInfo VulkanExampleBase::loadShader(std::string fileName, VkShaderStageFlagBits stage)
{
	VkPipelineShaderStageCreateInfo shaderStage = {};
	shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStage.stage = stage;

	///TODO
	shaderStage.module = vks::tools::loadShader(fileName.c_str(), vulkanDevice->GetDevice());

	shaderStage.pName = "main"; // todo : make param
	assert(shaderStage.module != VK_NULL_HANDLE);
	shaderModules.push_back(shaderStage.module);
	return shaderStage;
}

void VulkanExampleBase::renderFrame()
{
}

void VulkanExampleBase::renderLoop()
{
	destWidth = width;
	destHeight = height;

	MSG msg;
	bool quitMessageReceived = false;
	while (!quitMessageReceived)
	{
		auto tStart = std::chrono::high_resolution_clock::now();
		if (viewUpdated)
		{
			viewUpdated = false;
			viewChanged();
		}

		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);

			if (msg.message == WM_QUIT)
			{
				quitMessageReceived = true;
				break;
			}
		}

		render();
		frameCounter++;
		auto tEnd = std::chrono::high_resolution_clock::now();
		auto tDiff = std::chrono::duration<double, std::milli>(tEnd - tStart).count();
		frameTimer = (float)tDiff / 1000.0f;
		///TODO Add cam
		///camera.update(frameTimer);
		//if (camera.moving())
		//{
		//	viewUpdated = true;
		//}
		// Convert to clamped timer value
		if (!paused)
		{
			timer += timerSpeed * frameTimer;
			if (timer > 1.0)
			{
				timer -= 1.0f;
			}
		}
		fpsTimer += (float)tDiff;
		if (fpsTimer > 1000.0f)
		{

			lastFPS = static_cast<uint32_t>(1.0f / frameTimer);
			updateTextOverlay();
			fpsTimer = 0.0f;
			frameCounter = 0;
		}
	}

	// Flush device to make sure all resources can be freed 
	vkDeviceWaitIdle(vulkanDevice->GetDevice());
}

void VulkanExampleBase::updateTextOverlay()
{
	
}

void VulkanExampleBase::prepareFrame()
{
	// Acquire the next image from the swap chain
	VkResult err = swapChain.acquireNextImage(semaphores.presentComplete, &currentBuffer);
	// Recreate the swapchain if it's no longer compatible with the surface (OUT_OF_DATE) or no longer optimal for presentation (SUBOPTIMAL)
	if ((err == VK_ERROR_OUT_OF_DATE_KHR) || (err == VK_SUBOPTIMAL_KHR)) {
		windowResize();
	}
	else {
		VK_CHECK_RESULT(err);
	}
}

void VulkanExampleBase::submitFrame()
{
	// Present the current buffer to the swap chain
	// Pass the semaphore signaled by the command buffer submission from the submit info as the wait semaphore for swap chain presentation
	// This ensures that the image is not presented to the windowing system until all commands have been submitted
	VK_CHECK_RESULT(swapChain.queuePresent(queue, currentBuffer, semaphores.renderComplete));

	//VK_CHECK_RESULT(queue.waitIdle ()); //is equivalent to submitting a fence to a queue and waiting with an infinite timeout for that fence to signal.
}

VulkanExampleBase::VulkanExampleBase(bool enableValidation)
{

	// Check for a valid asset path
	//struct stat info;
	//if (stat(getAssetPath().c_str(), &info) != 0)
	//{

	//	std::string msg = "Could not locate asset path in \"" + getAssetPath() + "\" !";
	//	MessageBox(NULL, msg.c_str(), "Fatal error", MB_OK | MB_ICONERROR);

	//	exit(-1);
	//}

	settings.validation = enableValidation;

	// Parse command line arguments
	for (size_t i = 0; i < args.size(); i++)
	{
		if (args[i] == std::string("-validation"))
		{
			settings.validation = true;
		}
		if (args[i] == std::string("-vsync"))
		{
			settings.vsync = true;
		}
		if (args[i] == std::string("-fullscreen"))
		{
			settings.fullscreen = true;
		}
		if ((args[i] == std::string("-w")) || (args[i] == std::string("-width")))
		{
			char* endptr;
			uint32_t w = strtol(args[i + 1], &endptr, 10);
			if (endptr != args[i + 1]) { width = w; };
		}
		if ((args[i] == std::string("-h")) || (args[i] == std::string("-height")))
		{
			char* endptr;
			uint32_t h = strtol(args[i + 1], &endptr, 10);
			if (endptr != args[i + 1]) { height = h; };
		}
	}




	// Enable console if validation is active
	// Debug message callback will output to it
	/*if (this->settings.validation)
	{
		
	}*/
	setupConsole("Vulkan validation output");

}

VulkanExampleBase::~VulkanExampleBase()
{
	// Clean up Vulkan resources
	swapChain.cleanup();
	if (descriptorPool)
	{
		vkDestroyDescriptorPool(vulkanDevice->GetDevice(), descriptorPool, nullptr);
	}
	destroyCommandBuffers();
	vkDestroyRenderPass(vulkanDevice->GetDevice(), renderPass, nullptr);
	for (uint32_t i = 0; i < frameBuffers.size(); i++)
	{
		vkDestroyFramebuffer(vulkanDevice->GetDevice(), frameBuffers[i], nullptr);
	}

	for (auto& shaderModule : shaderModules)
	{
		vkDestroyShaderModule(vulkanDevice->GetDevice(), shaderModule, nullptr);
	}
	vkDestroyImageView(vulkanDevice->GetDevice(), depthStencil.view, nullptr);
	vkDestroyImage(vulkanDevice->GetDevice(), depthStencil.image, nullptr);
	vkFreeMemory(vulkanDevice->GetDevice(), depthStencil.mem, nullptr);

	vkDestroyPipelineCache(vulkanDevice->GetDevice(), pipelineCache, nullptr);

	vkDestroyCommandPool(vulkanDevice->GetDevice(), cmdPool, nullptr);

	vkDestroySemaphore(vulkanDevice->GetDevice(), semaphores.presentComplete, nullptr);
	vkDestroySemaphore(vulkanDevice->GetDevice(), semaphores.renderComplete, nullptr);

	delete vulkanDevice;

	//if (settings.validation)
	//{
	//	vks::debug::freeDebugCallback(instance);
	//}

	vkDestroyInstance(instance, nullptr);
}

void VulkanExampleBase::initVulkan()
{
	// Vulkan instance
	createInstance(settings.validation);

	
	// If requested, we enable the default validation layers for debugging
	if (settings.validation)
	{
		// The report flags determine what type of messages for the layers will be displayed
		// For validating (debugging) an appplication the error and warning bits should suffice
		VkDebugReportFlagsEXT debugReportFlags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
		// Additional flags include performance info, loader and layer debug messages, etc.
		//vks::debug::setupDebugging(instance, debugReportFlags, VK_NULL_HANDLE);
	}

	std::vector<vk::PhysicalDevice> physicalDevices = CHECK(instance.enumeratePhysicalDevices ());
	size_t gpuCount = physicalDevices.size();

	// Select physical device to be used for the Vulkan example
	// Defaults to the first device unless specified by command line
	uint32_t selectedDevice = 0;

	// GPU selection via command line argument
	for (size_t i = 0; i < args.size(); i++)
	{
		// Select GPU
		if ((args[i] == std::string("-g")) || (args[i] == std::string("-gpu")))
		{
			char* endptr;
			uint32_t index = strtol(args[i + 1], &endptr, 10);
			if (endptr != args[i + 1])
			{
				if (index > gpuCount - 1)
				{
					std::cerr << "Selected device index " << index << " is out of range, reverting to device 0 (use -listgpus to show available Vulkan devices)" << std::endl;
				}
				else
				{
					std::cout << "Selected Vulkan device " << index << std::endl;
					selectedDevice = index;
				}
			};
			break;
		}
		// List available GPUs
		if (args[i] == std::string("-listgpus"))
		{
			uint32_t gpuCount = 0;
			VK_CHECK_RESULT(vkEnumeratePhysicalDevices(instance, &gpuCount, nullptr));
			if (gpuCount == 0)
			{
				std::cerr << "No Vulkan devices found!" << std::endl;
			}
			else
			{
				// Enumerate devices
				std::cout << "Available Vulkan devices" << std::endl;
				std::vector<VkPhysicalDevice> devices(gpuCount);
				VK_CHECK_RESULT(vkEnumeratePhysicalDevices(instance, &gpuCount, devices.data()));
				for (uint32_t i = 0; i < gpuCount; i++) {
					VkPhysicalDeviceProperties deviceProperties;
					vkGetPhysicalDeviceProperties(devices[i], &deviceProperties);
					std::cout << "Device [" << i << "] : " << deviceProperties.deviceName << std::endl;
					std::cout << " Type: " << vks::tools::physicalDeviceTypeString(deviceProperties.deviceType) << std::endl;
					std::cout << " API: " << (deviceProperties.apiVersion >> 22) << "." << ((deviceProperties.apiVersion >> 12) & 0x3ff) << "." << (deviceProperties.apiVersion & 0xfff) << std::endl;
				}
			}
		}
	}

	physicalDevice = physicalDevices[selectedDevice];

	// Derived examples can override this to set actual features (based on above readings) to enable for logical device creation
	getEnabledFeatures();

	// Vulkan device creation
	// This is handled by a separate class that gets a logical device representation
	// and encapsulates functions related to a device
	vulkanDevice = new vks::VulkanDevice(physicalDevice);
	vulkanDevice->createLogicalDevice(enabledFeatures, enabledExtensions);
	device = vulkanDevice->GetDevice();
	

	// Get a graphics queue from the device
	queue = vulkanDevice->D().getQueue(vulkanDevice->queueFamilyIndices.graphics, 0);
	// Find a suitable depth format
	VkBool32 validDepthFormat = vks::tools::getSupportedDepthFormat(physicalDevice, &depthFormat);
	assert(validDepthFormat);

	swapChain.connect(instance, physicalDevice, vulkanDevice->GetDevice ());

	// Create synchronization objects
	vk::SemaphoreCreateInfo semaphoreCreateInfo; //Arguments reserved for future use
	// Create a semaphore used to synchronize image presentation
	// Ensures that the image is displayed before we start submitting new commands to the queu
	semaphores.presentComplete = CHECK(vulkanDevice->D().createSemaphore(semaphoreCreateInfo, nullptr));
	// Create a semaphore used to synchronize command submission
	// Ensures that the image is not presented until all commands have been sumbitted and executed
	semaphores.renderComplete = CHECK(vulkanDevice->D().createSemaphore(semaphoreCreateInfo, nullptr));

	// Set up submit info structure
	// Semaphores will stay the same during application lifetime
	// Command buffer submission info is set by each example
	submitInfo.setPWaitSemaphores(&semaphores.presentComplete)			// Semaphore(s) to wait upon before the submitted command buffer starts executing
		.setWaitSemaphoreCount(1)
		.setPSignalSemaphores(&semaphores.renderComplete)		// Semaphore(s) to be signaled when command buffers have completed
		.setSignalSemaphoreCount(1);
}

// Win32 : Sets up a console window and redirects standard output to it
void VulkanExampleBase::setupConsole(std::string title)
{
	AllocConsole();
	AttachConsole(GetCurrentProcessId());
	FILE *stream;
	freopen_s(&stream, "CONOUT$", "w+", stdout);
	freopen_s(&stream, "CONOUT$", "w+", stderr);
	SetConsoleTitle(TEXT(title.c_str()));
}

HWND VulkanExampleBase::setupWindow(HINSTANCE hinstance, WNDPROC wndproc)
{
	this->windowInstance = hinstance;

	WNDCLASSEX wndClass;

	wndClass.cbSize = sizeof(WNDCLASSEX);
	wndClass.style = CS_HREDRAW | CS_VREDRAW;
	wndClass.lpfnWndProc = wndproc;
	wndClass.cbClsExtra = 0;
	wndClass.cbWndExtra = 0;
	wndClass.hInstance = hinstance;
	wndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wndClass.lpszMenuName = NULL;
	wndClass.lpszClassName = name.c_str();
	wndClass.hIconSm = LoadIcon(NULL, IDI_WINLOGO);

	if (!RegisterClassEx(&wndClass))
	{
		std::cout << "Could not register window class!\n";
		fflush(stdout);
		exit(1);
	}

	int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = GetSystemMetrics(SM_CYSCREEN);

	if (settings.fullscreen)
	{
		DEVMODE dmScreenSettings;
		memset(&dmScreenSettings, 0, sizeof(dmScreenSettings));
		dmScreenSettings.dmSize = sizeof(dmScreenSettings);
		dmScreenSettings.dmPelsWidth = screenWidth;
		dmScreenSettings.dmPelsHeight = screenHeight;
		dmScreenSettings.dmBitsPerPel = 32;
		dmScreenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

		if ((width != (uint32_t)screenWidth) && (height != (uint32_t)screenHeight))
		{
			if (ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
			{
				if (MessageBox(NULL, "Fullscreen Mode not supported!\n Switch to window mode?", "Error", MB_YESNO | MB_ICONEXCLAMATION) == IDYES)
				{
					settings.fullscreen = false;
				}
				else
				{
					return nullptr;
				}
			}
		}

	}

	DWORD dwExStyle;
	DWORD dwStyle;

	if (settings.fullscreen)
	{
		dwExStyle = WS_EX_APPWINDOW;
		dwStyle = WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
	}
	else
	{
		dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
		dwStyle = WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
	}

	RECT windowRect;
	windowRect.left = 0L;
	windowRect.top = 0L;
	windowRect.right = settings.fullscreen ? (long)screenWidth : (long)width;
	windowRect.bottom = settings.fullscreen ? (long)screenHeight : (long)height;

	AdjustWindowRectEx(&windowRect, dwStyle, FALSE, dwExStyle);

	std::string windowTitle = getWindowTitle();
	window = CreateWindowEx(0,
		name.c_str(),
		windowTitle.c_str(),
		dwStyle | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
		0,
		0,
		windowRect.right - windowRect.left,
		windowRect.bottom - windowRect.top,
		NULL,
		NULL,
		hinstance,
		NULL);

	if (!settings.fullscreen)
	{
		// Center on screen
		uint32_t x = (GetSystemMetrics(SM_CXSCREEN) - windowRect.right) / 2;
		uint32_t y = (GetSystemMetrics(SM_CYSCREEN) - windowRect.bottom) / 2;
		SetWindowPos(window, 0, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
	}

	if (!window)
	{
		printf("Could not create window!\n");
		fflush(stdout);
		return nullptr;
		exit(1);
	}

	ShowWindow(window, SW_SHOW);
	SetForegroundWindow(window);
	SetFocus(window);

	return window;
}

void VulkanExampleBase::handleMessages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CLOSE:
		prepared = false;
		DestroyWindow(hWnd);
		PostQuitMessage(0);
		break;
	case WM_PAINT:
		ValidateRect(window, NULL);
		break;
	case WM_KEYDOWN:
		switch (wParam)
		{
		case KEY_P:
			paused = !paused;
			break;
		case KEY_F1:
			break;
		case KEY_ESCAPE:
			PostQuitMessage(0);
			break;
		}

		//if (camera.firstperson)
		//{
		//	switch (wParam)
		//	{
		//	case KEY_W:
		//		camera.keys.up = true;
		//		break;
		//	case KEY_S:
		//		camera.keys.down = true;
		//		break;
		//	case KEY_A:
		//		camera.keys.left = true;
		//		break;
		//	case KEY_D:
		//		camera.keys.right = true;
		//		break;
		//	}
		//}

		keyPressed((uint32_t)wParam);
		break;
	case WM_KEYUP:
		//if (camera.firstperson)
		//{
		//	switch (wParam)
		//	{
		//	case KEY_W:
		//		camera.keys.up = false;
		//		break;
		//	case KEY_S:
		//		camera.keys.down = false;
		//		break;
		//	case KEY_A:
		//		camera.keys.left = false;
		//		break;
		//	case KEY_D:
		//		camera.keys.right = false;
		//		break;
		//	}
		//}
		break;
	case WM_RBUTTONDOWN:
	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
		mousePos.x = (float)LOWORD(lParam);
		mousePos.y = (float)HIWORD(lParam);
		break;
	case WM_MOUSEWHEEL:
	{
		short wheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);
		zoom += (float)wheelDelta * 0.005f * zoomSpeed;
		//camera.translate(glm::vec3(0.0f, 0.0f, (float)wheelDelta * 0.005f * zoomSpeed));
		viewUpdated = true;
		break;
	}
	case WM_MOUSEMOVE:
		if (wParam & MK_RBUTTON)
		{
			int32_t posx = LOWORD(lParam);
			int32_t posy = HIWORD(lParam);
			zoom += (mousePos.y - (float)posy) * .005f * zoomSpeed;
			//camera.translate(glm::vec3(-0.0f, 0.0f, (mousePos.y - (float)posy) * .005f * zoomSpeed));
			mousePos = glm::vec2((float)posx, (float)posy);
			viewUpdated = true;
		}
		if (wParam & MK_LBUTTON)
		{
			int32_t posx = LOWORD(lParam);
			int32_t posy = HIWORD(lParam);
			rotation.x += (mousePos.y - (float)posy) * 1.25f * rotationSpeed;
			rotation.y -= (mousePos.x - (float)posx) * 1.25f * rotationSpeed;
			//camera.rotate(glm::vec3((mousePos.y - (float)posy) * camera.rotationSpeed, -(mousePos.x - (float)posx) * camera.rotationSpeed, 0.0f));
			mousePos = glm::vec2((float)posx, (float)posy);
			viewUpdated = true;
		}
		if (wParam & MK_MBUTTON)
		{
			int32_t posx = LOWORD(lParam);
			int32_t posy = HIWORD(lParam);
			cameraPos.x -= (mousePos.x - (float)posx) * 0.01f;
			cameraPos.y -= (mousePos.y - (float)posy) * 0.01f;
			//camera.translate(glm::vec3(-(mousePos.x - (float)posx) * 0.01f, -(mousePos.y - (float)posy) * 0.01f, 0.0f));
			viewUpdated = true;
			mousePos.x = (float)posx;
			mousePos.y = (float)posy;
		}
		break;
	case WM_SIZE:
		if ((prepared) && (wParam != SIZE_MINIMIZED))
		{
			if ((resizing) || ((wParam == SIZE_MAXIMIZED) || (wParam == SIZE_RESTORED)))
			{
				destWidth = LOWORD(lParam);
				destHeight = HIWORD(lParam);
				windowResize();
			}
		}
		break;
	case WM_ENTERSIZEMOVE:
		resizing = true;
		break;
	case WM_EXITSIZEMOVE:
		resizing = false;
		break;
	}
}


void VulkanExampleBase::viewChanged() {}

void VulkanExampleBase::keyPressed(uint32_t) {}

void VulkanExampleBase::createCommandPool()
{
	cmdPool = vulkanDevice->createCommandPool (swapChain.queueNodeIndex); //Create graphics command pool
}

void VulkanExampleBase::setupDepthStencil()
{
	VkImageCreateInfo image = {};
	image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image.pNext = NULL;
	image.imageType = VK_IMAGE_TYPE_2D;
	image.format = depthFormat;
	image.extent = { width, height, 1 };
	image.mipLevels = 1;
	image.arrayLayers = 1;
	image.samples = VK_SAMPLE_COUNT_1_BIT;
	image.tiling = VK_IMAGE_TILING_OPTIMAL;
	image.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	image.flags = 0;

	VkMemoryAllocateInfo mem_alloc = {};
	mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	mem_alloc.pNext = NULL;
	mem_alloc.allocationSize = 0;
	mem_alloc.memoryTypeIndex = 0;

	VkImageViewCreateInfo depthStencilView = {};
	depthStencilView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	depthStencilView.pNext = NULL;
	depthStencilView.viewType = VK_IMAGE_VIEW_TYPE_2D;
	depthStencilView.format = depthFormat;
	depthStencilView.flags = 0;
	depthStencilView.subresourceRange = {};
	depthStencilView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	depthStencilView.subresourceRange.baseMipLevel = 0;
	depthStencilView.subresourceRange.levelCount = 1;
	depthStencilView.subresourceRange.baseArrayLayer = 0;
	depthStencilView.subresourceRange.layerCount = 1;

	VkMemoryRequirements memReqs;

	VK_CHECK_RESULT(vkCreateImage(vulkanDevice->GetDevice (), &image, nullptr, &depthStencil.image));
	vkGetImageMemoryRequirements(vulkanDevice->GetDevice(), depthStencil.image, &memReqs);
	mem_alloc.allocationSize = memReqs.size;
	mem_alloc.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);
	VK_CHECK_RESULT(vkAllocateMemory(vulkanDevice->GetDevice(), &mem_alloc, nullptr, &depthStencil.mem));
	VK_CHECK_RESULT(vkBindImageMemory(vulkanDevice->GetDevice(), depthStencil.image, depthStencil.mem, 0));

	depthStencilView.image = depthStencil.image;
	VK_CHECK_RESULT(vkCreateImageView(vulkanDevice->GetDevice(), &depthStencilView, nullptr, &depthStencil.view));
}

void VulkanExampleBase::setupFrameBuffer()
{
	VkImageView attachments[2];

	// Depth/Stencil attachment is the same for all frame buffers
	attachments[1] = depthStencil.view;

	VkFramebufferCreateInfo frameBufferCreateInfo = {};
	frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	frameBufferCreateInfo.pNext = NULL;
	frameBufferCreateInfo.renderPass = renderPass;
	frameBufferCreateInfo.attachmentCount = 2;
	frameBufferCreateInfo.pAttachments = attachments;
	frameBufferCreateInfo.width = width;
	frameBufferCreateInfo.height = height;
	frameBufferCreateInfo.layers = 1;

	// Create frame buffers for every swap chain image
	frameBuffers.resize(swapChain.imageCount);
	for (uint32_t i = 0; i < frameBuffers.size(); i++)
	{
		attachments[0] = swapChain.buffers[i].view;
		VK_CHECK_RESULT(vkCreateFramebuffer(vulkanDevice->GetDevice(), &frameBufferCreateInfo, nullptr, &frameBuffers[i]));
	}
}

void VulkanExampleBase::setupRenderPass()
{
	std::array<vk::AttachmentDescription, 2> attachments = {};
	// Color attachment
	attachments[0].setFormat((vk::Format)swapChain.colorFormat)
		.setSamples			(vk::SampleCountFlagBits::e1)
		.setLoadOp			(vk::AttachmentLoadOp::eClear)
		.setStoreOp			(vk::AttachmentStoreOp::eStore)
		.setStencilLoadOp	(vk::AttachmentLoadOp::eDontCare)
		.setStencilStoreOp	(vk::AttachmentStoreOp::eDontCare)
		.setInitialLayout	(vk::ImageLayout::eUndefined)
		.setFinalLayout		(vk::ImageLayout::ePresentSrcKHR);

	// Depth attachment
	attachments[1].setFormat ((vk::Format)depthFormat)
		.setSamples			(vk::SampleCountFlagBits::e1)
		.setLoadOp			(vk::AttachmentLoadOp::eClear)
		.setStoreOp			(vk::AttachmentStoreOp::eStore)
		.setStencilLoadOp	(vk::AttachmentLoadOp::eDontCare)
		.setStencilStoreOp	(vk::AttachmentStoreOp::eDontCare)
		.setInitialLayout	(vk::ImageLayout::eUndefined)
		.setFinalLayout		(vk::ImageLayout::eDepthStencilAttachmentOptimal);

	vk::AttachmentReference colorReference;
	colorReference.setAttachment (0)
		.setLayout (vk::ImageLayout::eColorAttachmentOptimal);

	vk::AttachmentReference depthReference = {};
	depthReference.setAttachment (1)
		.setLayout (vk::ImageLayout::eDepthStencilAttachmentOptimal);

	vk::SubpassDescription subpassDescription = {};
	subpassDescription.setPipelineBindPoint (vk::PipelineBindPoint::eGraphics)
		.setColorAttachmentCount (1)
		.setPColorAttachments (&colorReference)
		.setPDepthStencilAttachment (&depthReference)
		.setInputAttachmentCount (0)
		.setPInputAttachments (nullptr)
		.setPreserveAttachmentCount (0)
		.setPPreserveAttachments (nullptr)
		.setPResolveAttachments (nullptr);

	// Subpass dependencies for layout transitions
	std::array<vk::SubpassDependency, 2> dependencies;

	dependencies[0].setSrcSubpass (VK_SUBPASS_EXTERNAL)
		.setDstSubpass		(0)
		.setSrcStageMask	(vk::PipelineStageFlagBits::eBottomOfPipe)
		.setDstStageMask	(vk::PipelineStageFlagBits::eColorAttachmentOutput)
		.setSrcAccessMask	(vk::AccessFlagBits::eMemoryRead)
		.setDstAccessMask	(vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite)
		.setDependencyFlags (vk::DependencyFlagBits::eByRegion);

	dependencies[1].setSrcSubpass(0)
		.setDstSubpass(VK_SUBPASS_EXTERNAL)
		.setSrcStageMask	(vk::PipelineStageFlagBits::eColorAttachmentOutput)
		.setDstStageMask	(vk::PipelineStageFlagBits::eBottomOfPipe)
		.setSrcAccessMask	(vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite)
		.setDstAccessMask	(vk::AccessFlagBits::eMemoryRead)
		.setDependencyFlags	(vk::DependencyFlagBits::eByRegion);

	

	vk::RenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.setAttachmentCount ((uint32_t)attachments.size())
		.setPAttachments (attachments.data())
		.setSubpassCount (1)
		.setPSubpasses (&subpassDescription)
		.setDependencyCount ((uint32_t)dependencies.size())
		.setPDependencies (dependencies.data());

	renderPass = CHECK(vulkanDevice->D().createRenderPass (renderPassInfo));
}

void VulkanExampleBase::getEnabledFeatures()
{
	// Can be overriden in derived class
}

void VulkanExampleBase::windowResize()
{
	if (!prepared)
	{
		return;
	}
	prepared = false;

	// Ensure all operations on the device have been finished before destroying resources
	vkDeviceWaitIdle(vulkanDevice->GetDevice());

	// Recreate swap chain
	width = destWidth;
	height = destHeight;
	setupSwapChain();

	// Recreate the frame buffers

	vkDestroyImageView(vulkanDevice->GetDevice(), depthStencil.view, nullptr);
	vkDestroyImage(vulkanDevice->GetDevice(), depthStencil.image, nullptr);
	vkFreeMemory(vulkanDevice->GetDevice(), depthStencil.mem, nullptr);
	setupDepthStencil();

	for (uint32_t i = 0; i < frameBuffers.size(); i++)
	{
		vkDestroyFramebuffer(vulkanDevice->GetDevice(), frameBuffers[i], nullptr);
	}
	setupFrameBuffer();

	// Command buffers need to be recreated as they may store
	// references to the recreated frame buffer
	destroyCommandBuffers();
	createCommandBuffers();
	buildCommandBuffers();

	vkDeviceWaitIdle(vulkanDevice->GetDevice());

	///TODO
	//camera.updateAspectRatio((float)width / (float)height);

	// Notify derived class
	windowResized();
	viewChanged();

	prepared = true;
}

void VulkanExampleBase::windowResized()
{
	// Can be overriden in derived class
}

void VulkanExampleBase::initSwapchain()
{
	swapChain.initSurface(windowInstance, window);
}

void VulkanExampleBase::setupSwapChain()
{
	swapChain.create(&width, &height, settings.vsync);
}
