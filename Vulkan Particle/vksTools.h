#pragma once

#include <string>
#include <vector>
#include <fstream>

#include <windows.h>
#include <fcntl.h>
#include <io.h>

#include <assert.h>

#include <vulkan/vulkan.hpp>

// Custom define for better code readability
#define VK_FLAGS_NONE 0
// Default fence timeout in nanoseconds
#define DEFAULT_FENCE_TIMEOUT 100000000000

namespace vks {namespace tools {
	static inline std::string errorString(VkResult errorCode)
	{
		switch (errorCode)
		{
	#define STR(r) case VK_ ##r: return #r
			STR(NOT_READY);
			STR(TIMEOUT);
			STR(EVENT_SET);
			STR(EVENT_RESET);
			STR(INCOMPLETE);
			STR(ERROR_OUT_OF_HOST_MEMORY);
			STR(ERROR_OUT_OF_DEVICE_MEMORY);
			STR(ERROR_INITIALIZATION_FAILED);
			STR(ERROR_DEVICE_LOST);
			STR(ERROR_MEMORY_MAP_FAILED);
			STR(ERROR_LAYER_NOT_PRESENT);
			STR(ERROR_EXTENSION_NOT_PRESENT);
			STR(ERROR_FEATURE_NOT_PRESENT);
			STR(ERROR_INCOMPATIBLE_DRIVER);
			STR(ERROR_TOO_MANY_OBJECTS);
			STR(ERROR_FORMAT_NOT_SUPPORTED);
			STR(ERROR_SURFACE_LOST_KHR);
			STR(ERROR_NATIVE_WINDOW_IN_USE_KHR);
			STR(SUBOPTIMAL_KHR);
			STR(ERROR_OUT_OF_DATE_KHR);
			STR(ERROR_INCOMPATIBLE_DISPLAY_KHR);
			STR(ERROR_VALIDATION_FAILED_EXT);
			STR(ERROR_INVALID_SHADER_NV);
	#undef STR
		default:
			return "UNKNOWN_ERROR";
		}
	}
} //NS tools
} //NS vks

#define VK_CHECK_RESULT(f)																				\
{																										\
	VkResult res = (f);																					\
	if (res != VK_SUCCESS)																				\
	{																									\
		std::cout << "Fatal : VkResult is \"" << vks::tools::errorString(res) << "\" in " << __FILE__ << " at line " << __LINE__ << std::endl; \
		assert(res == VK_SUCCESS);																		\
	}																									\
}

template <typename T>
T CHECK(vk::ResultValue<T> resultValue)
{
	VkResult result = (VkResult)resultValue.result;
	VK_CHECK_RESULT(result);

	return resultValue.value;
}

namespace vks {
	namespace tools {

		inline VkBool32 getSupportedDepthFormat(VkPhysicalDevice physicalDevice, VkFormat *depthFormat)
		{
			// Since all depth formats may be optional, we need to find a suitable depth format to use
			// Start with the highest precision packed format
			std::vector<VkFormat> depthFormats = {
				VK_FORMAT_D32_SFLOAT_S8_UINT,
				VK_FORMAT_D32_SFLOAT,
				VK_FORMAT_D24_UNORM_S8_UINT,
				VK_FORMAT_D16_UNORM_S8_UINT,
				VK_FORMAT_D16_UNORM
			};

			for (auto& format : depthFormats)
			{
				VkFormatProperties formatProps;
				vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProps);
				// Format must support depth stencil attachment for optimal tiling
				if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
				{
					*depthFormat = format;
					return true;
				}
			}

			return false;
		}

		inline VkShaderModule loadShader(const char *fileName, VkDevice device)
		{
			std::ifstream is(fileName, std::ios::binary | std::ios::in | std::ios::ate);

			if (is.is_open())
			{
				size_t size = is.tellg();
				is.seekg(0, std::ios::beg);
				char* shaderCode = new char[size];
				is.read(shaderCode, size);
				is.close();

				assert(size > 0);

				VkShaderModule shaderModule;
				VkShaderModuleCreateInfo moduleCreateInfo{};
				moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
				moduleCreateInfo.codeSize = size;
				moduleCreateInfo.pCode = (uint32_t*)shaderCode;

				VK_CHECK_RESULT(vkCreateShaderModule(device, &moduleCreateInfo, NULL, &shaderModule));

				delete[] shaderCode;

				return shaderModule;
			}
			else
			{
				std::cerr << "Error: Could not open shader file \"" << fileName << "\"" << std::endl;
				return VK_NULL_HANDLE;
			}
		}

		inline std::string readTextFile(const char *fileName)
		{
			std::string fileContent;
			std::ifstream fileStream(fileName, std::ios::in);
			if (!fileStream.is_open()) {
				printf("File %s not found\n", fileName);
				return "";
			}
			std::string line = "";
			while (!fileStream.eof()) {
				getline(fileStream, line);
				fileContent.append(line + "\n");
			}
			fileStream.close();
			return fileContent;
		}

		inline VkShaderModule loadShaderGLSL(const char *fileName, VkDevice device, VkShaderStageFlagBits stage)
		{
			std::string shaderSrc = readTextFile(fileName);
			const char *shaderCode = shaderSrc.c_str();
			size_t size = strlen(shaderCode);
			assert(size > 0);

			VkShaderModule shaderModule;
			VkShaderModuleCreateInfo moduleCreateInfo;
			moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			moduleCreateInfo.pNext = NULL;
			moduleCreateInfo.codeSize = 3 * sizeof(uint32_t) + size + 1;
			moduleCreateInfo.pCode = (uint32_t*)malloc(moduleCreateInfo.codeSize);
			moduleCreateInfo.flags = 0;

			// Magic SPV number
			((uint32_t *)moduleCreateInfo.pCode)[0] = 0x07230203;
			((uint32_t *)moduleCreateInfo.pCode)[1] = 0;
			((uint32_t *)moduleCreateInfo.pCode)[2] = stage;
			memcpy(((uint32_t *)moduleCreateInfo.pCode + 3), shaderCode, size + 1);

			VK_CHECK_RESULT(vkCreateShaderModule(device, &moduleCreateInfo, NULL, &shaderModule));

			return shaderModule;
		}

		// Vulkan loads it's shaders from an immediate binary representation called SPIR-V
		// Shaders are compiled offline from e.g. GLSL using the reference glslang compiler
		// This function loads such a shader from a binary file and returns a shader module structure
		static inline VkShaderModule loadSPIRVShader(std::string filename, VkDevice device)
		{
			size_t shaderSize;
			char* shaderCode;


			std::ifstream is(filename, std::ios::binary | std::ios::in | std::ios::ate);

			if (is.is_open())
			{
				shaderSize = is.tellg();
				is.seekg(0, std::ios::beg);
				// Copy file contents into a buffer
				shaderCode = new char[shaderSize];
				is.read(shaderCode, shaderSize);
				is.close();
				assert(shaderSize > 0);
			}

			if (shaderCode)
			{
				// Create a new shader module that will be used for pipeline creation
				VkShaderModuleCreateInfo moduleCreateInfo{};
				moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
				moduleCreateInfo.codeSize = shaderSize;
				moduleCreateInfo.pCode = (uint32_t*)shaderCode;

				VkShaderModule shaderModule;
				VK_CHECK_RESULT(vkCreateShaderModule(device, &moduleCreateInfo, NULL, &shaderModule));

				delete[] shaderCode;

				return shaderModule;
			}
			else
			{
				std::cerr << "Error: Could not open shader file \"" << filename << "\"" << std::endl;
				return VK_NULL_HANDLE;
			}
		}

		static inline void exitFatal(std::string message, std::string caption)
		{
			MessageBox(NULL, message.c_str(), caption.c_str(), MB_OK | MB_ICONERROR);

			exit(1);
		}

		static inline std::string physicalDeviceTypeString(VkPhysicalDeviceType type)
		{
			switch (type)
			{
#define STR(r) case VK_PHYSICAL_DEVICE_TYPE_ ##r: return #r
				STR(OTHER);
				STR(INTEGRATED_GPU);
				STR(DISCRETE_GPU);
				STR(VIRTUAL_GPU);
#undef STR
			default: return "UNKNOWN_DEVICE_TYPE";
			}
		}
	} //NS tools
} //NS vks
