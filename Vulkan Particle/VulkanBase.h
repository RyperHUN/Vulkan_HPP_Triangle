#pragma once
/*
* Vulkan Example base class
*
* Copyright (C) 2016 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#pragma once

#pragma comment(linker, "/subsystem:windows")
#include <windows.h>
#include <fcntl.h>
#include <io.h>


#include <iostream>
#include <chrono>
#include <sys/stat.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <string>
#include <array>
#include <vector>

#include "vulkan/vulkan.h"
#include <vulkan/vulkan.hpp>

#include "keycodes.h"
#include "vksTools.h"


#include "VulkanDevice.hpp"
#include "VulkanSwapChain.hpp"



class VulkanExampleBase
{
private:
	// fps timer (one second interval)
	float fpsTimer = 0.0f;
	// Get window title with example name, device, et.
	std::string getWindowTitle();
	/** brief Indicates that the view (position, rotation) has changed and */
	bool viewUpdated = false;
	// Destination dimensions for resizing the window
	uint32_t destWidth;
	uint32_t destHeight;
	bool resizing = false;
	// Called if the window is resized and some resources have to be recreatesd
	void windowResize();
protected:
	// Frame counter to display fps
	uint32_t frameCounter = 0;
	uint32_t lastFPS = 0;
	// Vulkan instance, stores all per-application states
	vk::Instance instance;
	// Physical device (GPU) that Vulkan will ise
	vk::PhysicalDevice physicalDevice;
	/**
	* Set of physical device features to be enabled for this example (must be set in the derived constructor)
	*
	* @note By default no phyiscal device features are enabled
	*/
	vk::PhysicalDeviceFeatures enabledFeatures;
	/** @brief Set of device extensions to be enabled for this example (must be set in the derived constructor) */
	std::vector<const char*> enabledExtensions;
	VkDevice device;
	// Handle to the device graphics queue that command buffers are submitted to
	vk::Queue queue;
	// Depth buffer format (selected during Vulkan initialization)
	VkFormat depthFormat;
	/** @brief Pipeline stages used to wait at for graphics queue submissions */
	VkPipelineStageFlags submitPipelineStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	// Command buffers used for rendering
	std::vector<vk::CommandBuffer> drawCmdBuffers;
	// Global render pass for frame buffer writes
	VkRenderPass renderPass;
	// List of available frame buffers (same as number of swap chain images)
	std::vector<VkFramebuffer>frameBuffers;
	// Active frame buffer index
	uint32_t currentBuffer = 0;
	// Descriptor set pool
	vk::DescriptorPool descriptorPool;
	// List of shader modules created (stored for cleanup)
	std::vector<VkShaderModule> shaderModules;
	// Pipeline cache object
	vk::PipelineCache pipelineCache;
	// Wraps the swap chain to present images (framebuffers) to the windowing system
	VulkanSwapChain swapChain;

	vk::SubmitInfo submitInfo;
	// Synchronization semaphores
	struct {
		// Swap chain image presentation
		vk::Semaphore presentComplete;
		// Command buffer submission and execution
		vk::Semaphore renderComplete;
	} semaphores;
public:
	bool prepared = false;
	uint32_t width = 1280;
	uint32_t height = 720;

	/** @brief Last frame time measured using a high performance timer (if available) */
	float frameTimer = 1.0f;
	/** @brief Returns os specific base asset path (for shaders, models, textures) */
	const std::string getAssetPath();

	/** @brief Encapsulated physical and logical vulkan device */
	vks::VulkanDevice *vulkanDevice;

	/** @brief Example settings that can be changed e.g. by command line arguments */
	struct Settings {
		/** @brief Activates validation layers (and message output) when set to true */
		bool validation = true;
		/** @brief Set to true if fullscreen mode has been requested via command line */
		bool fullscreen = false;
		/** @brief Set to true if v-sync will be forced for the swapchain */
		bool vsync = false;
	} settings;

	VkClearColorValue defaultClearColor = { { 0.025f, 0.025f, 0.025f, 1.0f } };

	float zoom = 0;

	static std::vector<const char*> args;

	// Defines a frame rate independent timer value clamped from -1.0...1.0
	// For use in animations, rotations, etc.
	float timer = 0.0f;
	// Multiplier for speeding up (or slowing down) the global timer
	float timerSpeed = 0.25f;

	bool paused = false;

	// Use to adjust mouse rotation speed
	float rotationSpeed = 1.0f;
	// Use to adjust mouse zoom speed
	float zoomSpeed = 1.0f;

	//Camera camera;

	glm::vec3 rotation = glm::vec3();
	glm::vec3 cameraPos = glm::vec3();
	glm::vec2 mousePos;

	std::string title = "Vulkan Example";
	std::string name = "vulkanExample";

	struct
	{
		VkImage image;
		VkDeviceMemory mem;
		VkImageView view;
	} depthStencil;

	// Gamepad state (only one pad supported)
	struct
	{
		glm::vec2 axisLeft = glm::vec2(0.0f);
		glm::vec2 axisRight = glm::vec2(0.0f);
	} gamePadState;

	HWND window;
	HINSTANCE windowInstance;


	// Default ctor
	VulkanExampleBase(bool enableValidation);

	// dtor
	virtual ~VulkanExampleBase();

	// Setup the vulkan instance, enable required extensions and connect to the physical device (GPU)
	void initVulkan();


	void setupConsole(std::string title);
	HWND setupWindow(HINSTANCE hinstance, WNDPROC wndproc);
	void handleMessages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	/**
	* Create the application wide Vulkan instance
	*
	* @note Virtual, can be overriden by derived example class for custom instance creation
	*/
	virtual void createInstance(bool enableValidation);

	// Pure virtual render function (override in derived class)
	virtual void render() = 0;
	// Called when view change occurs
	// Can be overriden in derived class to e.g. update uniform buffers 
	// Containing view dependant matrices
	virtual void viewChanged();
	// Called if a key is pressed
	/** @brief (Virtual) Called after a key was pressed, can be used to do custom key handling */
	virtual void keyPressed(uint32_t);
	// Called when the window has been resized
	// Can be overriden in derived class to recreate or rebuild resources attached to the frame buffer / swapchain
	virtual void windowResized();
	// Pure virtual function to be overriden by the dervice class
	// Called in case of an event where e.g. the framebuffer has to be rebuild and thus
	// all command buffers that may reference this
	virtual void buildCommandBuffers() = 0;

	// Setup default depth and stencil views
	virtual void setupDepthStencil();
	// Create framebuffers for all requested swap chain images
	// Can be overriden in derived class to setup a custom framebuffer (e.g. for MSAA)
	virtual void setupFrameBuffer();
	// Setup a default render pass
	// Can be overriden in derived class to setup a custom render pass (e.g. for MSAA)
	virtual void setupRenderPass();

	/** @brief (Virtual) Called after the physical device features have been read, can be used to set features to enable on the device */
	virtual void getEnabledFeatures();

	// Connect and prepare the swap chain
	void initSwapchain();
	// Create swap chain images
	void setupSwapChain();

	// Check if command buffers are valid (!= VK_NULL_HANDLE)
	bool checkCommandBuffers();
	// Create command buffers for drawing commands
	void createCommandBuffers();
	// Destroy all command buffers and set their handles to VK_NULL_HANDLE
	// May be necessary during runtime if options are toggled 
	void destroyCommandBuffers();

	// End the command buffer, submit it to the queue and free (if requested)
	// Note : Waits for the queue to become idle
	void flushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, bool free);

	// Create a cache pool for rendering pipelines
	void createPipelineCache();

	// Prepare commonly used Vulkan functions
	virtual void prepare();

	// Load a SPIR-V shader
	VkPipelineShaderStageCreateInfo loadShader(std::string fileName, VkShaderStageFlagBits stage);

	// Start the main render loop
	void renderLoop();

	// Render one frame of a render loop on platforms that sync rendering
	void renderFrame();

	void updateTextOverlay();

	/** @brief (Virtual) Called when the text overlay is updating, can be used to add custom text to the overlay */
	//virtual void getOverlayText(VulkanTextOverlay*);

	// Prepare the frame for workload submission
	// - Acquires the next image from the swap chain 
	// - Sets the default wait and signal semaphores
	void prepareFrame();

	// Submit the frames' workload 
	// - Submits the text overlay (if enabled)
	void submitFrame();

};

// Windows entry point
#define VULKAN_EXAMPLE_MAIN()																		\
VulkanExample *vulkanExample;																		\
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)						\
{																									\
	if (vulkanExample != NULL)																		\
	{																								\
		vulkanExample->handleMessages(hWnd, uMsg, wParam, lParam);									\
	}																								\
	return (DefWindowProc(hWnd, uMsg, wParam, lParam));												\
}																									\
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)									\
{																									\
	for (int32_t i = 0; i < __argc; i++) { VulkanExample::args.push_back(__argv[i]); };  			\
	vulkanExample = new VulkanExample();															\
	vulkanExample->initVulkan();																	\
	vulkanExample->setupWindow(hInstance, WndProc);													\
	vulkanExample->initSwapchain();																	\
	vulkanExample->prepare();																		\
	vulkanExample->renderLoop();																	\
	delete(vulkanExample);																			\
	return 0;																						\
}																									
