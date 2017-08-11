// Vulkan Particle.cpp : Defines the entry point for the console application.
//

#include <vulkan/vulkan.h>
#include <vulkan/vulkan.hpp>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "VulkanBase.h"

class VulkanExample : public VulkanExampleBase 
{
public:

	// Vertex layout used in this example
	struct Vertex {
		float position[3];
		float color[3];
	};

	// Vertex buffer and attributes
	struct {
		vk::DeviceMemory memory;															// Handle to the device memory for this buffer
		vk::Buffer buffer;																// Handle to the Vulkan buffer object that the memory is bound to
	} vertices;

	// Index buffer
	struct
	{
		VkDeviceMemory memory;
		VkBuffer buffer;
		uint32_t count;
	} indices;

	// Uniform buffer block object
	struct {
		vk::DeviceMemory memory;
		vk::Buffer buffer;
		vk::DescriptorBufferInfo descriptor;
	}  uniformBufferVS;

	// For simplicity we use the same uniform block layout as in the shader:
	//
	//	layout(set = 0, binding = 0) uniform UBO
	//	{
	//		mat4 projectionMatrix;
	//		mat4 modelMatrix;
	//		mat4 viewMatrix;
	//	} ubo;
	//
	// This way we can just memcopy the ubo data to the ubo
	// Note: You should use data types that align with the GPU in order to avoid manual padding (vec4, mat4)
	struct {
		glm::mat4 projectionMatrix;
		glm::mat4 modelMatrix;
		glm::mat4 viewMatrix;
	} uboVS;

	// The pipeline layout is used by a pipline to access the descriptor sets 
	// It defines interface (without binding any actual data) between the shader stages used by the pipeline and the shader resources
	// A pipeline layout can be shared among multiple pipelines as long as their interfaces match
	vk::PipelineLayout pipelineLayout;

	// Pipelines (often called "pipeline state objects") are used to bake all states that affect a pipeline
	// While in OpenGL every state can be changed at (almost) any time, Vulkan requires to layout the graphics (and compute) pipeline states upfront
	// So for each combination of non-dynamic pipeline states you need a new pipeline (there are a few exceptions to this not discussed here)
	// Even though this adds a new dimension of planing ahead, it's a great opportunity for performance optimizations by the driver
	vk::Pipeline pipeline;

	// The descriptor set layout describes the shader binding layout (without actually referencing descriptor)
	// Like the pipeline layout it's pretty much a blueprint and can be used with different descriptor sets as long as their layout matches
	vk::DescriptorSetLayout descriptorSetLayout;

	// The descriptor set stores the resources bound to the binding points in a shader
	// It connects the binding points of the different shaders with the buffers and images used for those bindings
	vk::DescriptorSet descriptorSet;

	// Synchronization primitives

	// Fences
	// Used to check the completion of queue operations (e.g. command buffer execution)
	std::vector<VkFence> waitFences;


	VulkanExample ()
		: VulkanExampleBase (false)
	{
		zoom = -2.5f;
		title = "Example particle system";
	}

	~VulkanExample()
	{
		// Clean up used Vulkan resources 
		// Note: Inherited destructor cleans up resources stored in base class
		vkDestroyPipeline(device, pipeline, nullptr);

		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

		vkDestroyBuffer(device, vertices.buffer, nullptr);
		vkFreeMemory(device, vertices.memory, nullptr);

		vkDestroyBuffer(device, indices.buffer, nullptr);
		vkFreeMemory(device, indices.memory, nullptr);

		vkDestroyBuffer(device, uniformBufferVS.buffer, nullptr);
		vkFreeMemory(device, uniformBufferVS.memory, nullptr);

		for (auto& fence : waitFences)
		{
			vkDestroyFence(device, fence, nullptr);
		}
	}

	// Create the Vulkan synchronization primitives used in this example
	void prepareSynchronizationPrimitives()
	{
		// Fences (Used to check draw command buffer completion)
		vk::FenceCreateInfo fenceCreateInfo;
		// Create in signaled state so we don't wait on first render of each command buffer
		fenceCreateInfo.flags = vk::FenceCreateFlagBits::eSignaled;
		waitFences.resize(drawCmdBuffers.size());
		for (auto& fence : waitFences)
		{
			fence = CHECK(vulkanDevice->D().createFence (fenceCreateInfo));
		}
	}

	void prepareVertices ()
	{
		// A note on memory management in Vulkan in general:
		//	This is a very complex topic and while it's fine for an example application to to small individual memory allocations that is not
		//	what should be done a real-world application, where you should allocate large chunkgs of memory at once isntead.

		// Setup vertices
		std::vector<Vertex> vertexBuffer =
		{
			{ { 1.0f,  1.0f, 0.0f },{ 1.0f, 0.0f, 0.0f } },
			{ { -1.0f,  1.0f, 0.0f },{ 0.0f, 1.0f, 0.0f } },
			{ { 0.0f, -1.0f, 0.0f },{ 0.0f, 0.0f, 1.0f } }
		};
		uint32_t vertexBufferSize = static_cast<uint32_t>(vertexBuffer.size()) * sizeof(Vertex);

		// Setup indices
		std::vector<uint32_t> indexBuffer = { 0, 1, 2 };
		indices.count = static_cast<uint32_t>(indexBuffer.size());
		uint32_t indexBufferSize = indices.count * sizeof(uint32_t);

		///TODO Staging is faster
		{
			// Don't use staging
			// Create host-visible buffers only and use these for rendering. This is not advised and will usually result in lower rendering performance

			//Vertex Buffer
			BuffMem vertexBuff = vulkanDevice->createBuffer (vk::BufferUsageFlagBits::eVertexBuffer, 
				vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, vertexBufferSize, vertexBuffer.data());
			vertices.buffer = vertexBuff.buff;
			vertices.memory = vertexBuff.mem;

			// Index buffer
			BuffMem result = vulkanDevice->createBuffer(vk::BufferUsageFlagBits::eVertexBuffer,
				vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, indexBufferSize, indexBuffer.data());
			indices.buffer = result.buff;
			indices.memory = result.mem;
		}
	}

	void prepareUniformBuffers()
	{
		// Prepare and initialize a uniform buffer block containing shader uniforms
		// Single uniforms like in OpenGL are no longer present in Vulkan. All Shader uniforms are passed via uniform buffer blocks

		vk::DeviceSize uboSize = sizeof(uboVS);

		BuffMem result = vulkanDevice->createBuffer (vk::BufferUsageFlagBits::eUniformBuffer, 
			vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, uboSize);
		uniformBufferVS.buffer = result.buff;
		uniformBufferVS.memory = result.mem;

		// Store information in the uniform's descriptor that is used by the descriptor set
		uniformBufferVS.descriptor	.setBuffer	(uniformBufferVS.buffer)
									.setOffset	(0)
									.setRange	(uboSize);

		updateUniformBuffers();
	}

	void updateUniformBuffers()
	{
		// Update matrices
		uboVS.projectionMatrix = glm::perspective(glm::radians(60.0f), (float)width / (float)height, 0.1f, 256.0f);

		uboVS.viewMatrix = glm::translate(glm::mat4(), glm::vec3(0.0f, 0.0f, zoom));

		uboVS.modelMatrix = glm::mat4();
		uboVS.modelMatrix = glm::rotate(uboVS.modelMatrix, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
		uboVS.modelMatrix = glm::rotate(uboVS.modelMatrix, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
		uboVS.modelMatrix = glm::rotate(uboVS.modelMatrix, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

		// Map uniform buffer and update it
		
		uint8_t *pData = (uint8_t*) CHECK(vulkanDevice->D().mapMemory (uniformBufferVS.memory, 0, sizeof(uboVS)));
		memcpy(pData, &uboVS, sizeof(uboVS));
		// Unmap after data has been copied
		// Note: Since we requested a host coherent memory type for the uniform buffer, the write is instantly visible to the GPU
		vulkanDevice->D().unmapMemory (uniformBufferVS.memory);
	}

	void preparePipelines()
	{
		// Create the graphics pipeline used in this example
		// Vulkan uses the concept of rendering pipelines to encapsulate fixed states, replacing OpenGL's complex state machine
		// A pipeline is then stored and hashed on the GPU making pipeline changes very fast
		// Note: There are still a few dynamic states that are not directly part of the pipeline (but the info that they are used is)

		// Construct the differnent states making up the pipeline

		// Input assembly state describes how primitives are assembled
		// This pipeline will assemble vertex data as a triangle lists (though we only use one triangle)
		vk::PipelineInputAssemblyStateCreateInfo inputAssemblyState = {};
		inputAssemblyState.setTopology (vk::PrimitiveTopology::eTriangleList);

		// Rasterization state
		vk::PipelineRasterizationStateCreateInfo rasterizationState = {};
		rasterizationState.setPolygonMode	(vk::PolygonMode::eFill)
			.setCullMode					(vk::CullModeFlagBits::eNone)
			.setFrontFace (vk::FrontFace::eCounterClockwise)
			.setDepthClampEnable (false)
			.setRasterizerDiscardEnable (false)
			.setDepthBiasEnable (false)
			.setLineWidth (1.0f);

		// Color blend state describes how blend factors are calculated (if used)
		// We need one blend attachment state per color attachment (even if blending is not used
		vk::PipelineColorBlendAttachmentState blendAttachmentState[1] = {};
		auto val = vk::ColorComponentFlagBits::eA | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eR;
		blendAttachmentState[0].setColorWriteMask (val)
								.setBlendEnable (false);
		vk::PipelineColorBlendStateCreateInfo colorBlendState = {};
		colorBlendState.setAttachmentCount (1)
						.setPAttachments (blendAttachmentState);

		// Viewport state sets the number of viewports and scissor used in this pipeline
		// Note: This is actually overriden by the dynamic states (see below)
		vk::PipelineViewportStateCreateInfo viewportState = {};
		viewportState.setViewportCount (1)
					 .setScissorCount (1);

		// Enable dynamic states
		// Most states are baked into the pipeline, but there are still a few dynamic states that can be changed within a command buffer
		// To be able to change these we need do specify which dynamic states will be changed using this pipeline. Their actual states are set later on in the command buffer.
		// For this example we will set the viewport and scissor using dynamic states
		std::vector<vk::DynamicState> dynamicStateEnables;
		dynamicStateEnables.push_back(vk::DynamicState::eScissor);
		dynamicStateEnables.push_back(vk::DynamicState::eViewport);
		vk::PipelineDynamicStateCreateInfo dynamicState = {};
		dynamicState.setDynamicStateCount ((uint32_t)dynamicStateEnables.size())
					.setPDynamicStates (dynamicStateEnables.data());

		// Depth and stencil state containing depth and stencil compare and test operations
		// We only use depth tests and want depth tests and writes to be enabled and compare with less or equal
		vk::PipelineDepthStencilStateCreateInfo depthStencilState = {};
		vk::StencilOpState back;
		back.setFailOp (vk::StencilOp::eKeep)
			.setPassOp (vk::StencilOp::eKeep)
			.setCompareOp (vk::CompareOp::eAlways);
		depthStencilState.setDepthTestEnable (true)
			.setDepthWriteEnable (true)
			.setDepthCompareOp(vk::CompareOp::eLessOrEqual)
			.setDepthBoundsTestEnable (false)
			.setBack (back)
			.setFront (back)
			.setStencilTestEnable (false);

		// Multi sampling state
		// This example does not make use fo multi sampling (for anti-aliasing), the state must still be set and passed to the pipeline
		vk::PipelineMultisampleStateCreateInfo multisampleState = {};
		multisampleState.setRasterizationSamples(vk::SampleCountFlagBits::e1)
						.setPSampleMask			(nullptr);

		// Vertex input descriptions 
		// Specifies the vertex input parameters for a pipeline

		// Vertex input binding
		// This example uses a single vertex input binding at binding point 0 (see vkCmdBindVertexBuffers)
		vk::VertexInputBindingDescription vertexInputBinding = {};
		vertexInputBinding.setBinding (0)
						  .setStride (sizeof(Vertex))
						  .setInputRate (vk::VertexInputRate::eVertex);

		// Inpute attribute bindings describe shader attribute locations and memory layouts
		std::array<vk::VertexInputAttributeDescription, 2> vertexInputAttributs;
		// These match the following shader layout (see triangle.vert):
		//	layout (location = 0) in vec3 inPos;
		//	layout (location = 1) in vec3 inColor;
		// Attribute location 0: Position
		vertexInputAttributs[0].setBinding (0)
							   .setLocation (0)
							   .setFormat (vk::Format::eR32G32B32Sfloat) // Position attribute is three 32 bit signed (SFLOAT) floats (R32 G32 B32)
							   .setOffset (offsetof(Vertex, position));
		// Attribute location 1: Color
		vertexInputAttributs[1].setBinding (0)
								.setLocation (1)
								.setFormat (vk::Format::eR32G32B32Sfloat) // Color attribute is three 32 bit signed (SFLOAT) floats (R32 G32 B32)
								.setOffset (offsetof(Vertex, color));


		// Vertex input state used for pipeline creation
		vk::PipelineVertexInputStateCreateInfo vertexInputState = {};
		vertexInputState.setVertexBindingDescriptionCount (1)
						.setPVertexBindingDescriptions (&vertexInputBinding)
						.setVertexAttributeDescriptionCount (2)
						.setPVertexAttributeDescriptions (vertexInputAttributs.data());

		// Shaders
		std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStages{};

		// Vertex shader
		shaderStages[0].setStage (vk::ShaderStageFlagBits::eVertex)											// Set pipeline stage for this shader
						.setModule (vks::tools::loadSPIRVShader("shaders/triangle.vert.spv", device))
						.setPName ("main");																	// Main entry point for the shader

		// Fragment shader
		shaderStages[1].setStage (vk::ShaderStageFlagBits::eFragment)
						.setModule (vks::tools::loadShaderGLSL("shaders/triangle.frag", device, VK_SHADER_STAGE_FRAGMENT_BIT))
						.setPName ("main");


		// Set pipeline shader stage info
		vk::GraphicsPipelineCreateInfo pipelineCreateInfo = {};
		pipelineCreateInfo.setLayout(pipelineLayout);			// The layout used for this pipeline (can be shared among multiple pipelines using the same layout)
		pipelineCreateInfo.setStageCount (static_cast<uint32_t>(shaderStages.size()))
						  .setPStages (shaderStages.data());

		// Assign the pipeline states to the pipeline creation info structure
		pipelineCreateInfo.setPVertexInputState (&vertexInputState)
						  .setPInputAssemblyState (&inputAssemblyState)
						  .setPRasterizationState (&rasterizationState)
							.setPColorBlendState (&colorBlendState)
			.setPMultisampleState (&multisampleState)
			.setPViewportState (&viewportState)
			.setPDepthStencilState (&depthStencilState)
			.setRenderPass (renderPass)
			.setPDynamicState (&dynamicState);

		// Create rendering pipeline using the specified state
		pipeline = CHECK(vulkanDevice->D().createGraphicsPipeline (pipelineCache, pipelineCreateInfo));

		// Shader modules are no longer needed once the graphics pipeline has been created
		vkDestroyShaderModule(device, shaderStages[0].module, nullptr);
		vkDestroyShaderModule(device, shaderStages[1].module, nullptr);
	}

	void setupDescriptorPool()
	{
		// We need to tell the API the number of max. requested descriptors per type
		vk::DescriptorPoolSize typeCounts[1];
		// This example only uses one descriptor type (uniform buffer) and only requests one descriptor of this type
		typeCounts[0].setType (vk::DescriptorType::eUniformBuffer);
		typeCounts[0].setDescriptorCount (1);
		// For additional types you need to add new entries in the type count list
		// E.g. for two combined image samplers :
		// typeCounts[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		// typeCounts[1].descriptorCount = 2;

		// Create the global descriptor pool
		// All descriptors used in this example are allocated from this pool
		vk::DescriptorPoolCreateInfo descriptorPoolInfo;
		descriptorPoolInfo.setPoolSizeCount (1)
			.setPPoolSizes (typeCounts)
			.setMaxSets (1);
		// Set the max. number of descriptor sets that can be requested from this pool (requesting beyond this limit will result in an error)
		descriptorPoolInfo.maxSets = 1;

		descriptorPool = CHECK(vulkanDevice->D().createDescriptorPool (descriptorPoolInfo));
	}

	void setupDescriptorSetLayout()
	{
		// Setup layout of descriptors used in this example
		// Basically connects the different shader stages to descriptors for binding uniform buffers, image samplers, etc.
		// So every shader binding should map to one descriptor set layout binding!!!!!!!!!!!

		// Binding 0: Uniform buffer (Vertex shader)
		vk::DescriptorSetLayoutBinding layoutBinding;
		layoutBinding.setDescriptorType (vk::DescriptorType::eUniformBuffer)
			.setDescriptorCount			(1)
			.setStageFlags				(vk::ShaderStageFlagBits::eVertex)
			.setPImmutableSamplers		(nullptr);

		vk::DescriptorSetLayoutCreateInfo descriptorLayout = {};
		descriptorLayout.setBindingCount	(1)
			.setPBindings					(&layoutBinding);

		descriptorSetLayout = CHECK(vulkanDevice->D().createDescriptorSetLayout (descriptorLayout));

		// Create the pipeline layout that is used to generate the rendering pipelines that are based on this descriptor set layout
		// In a more complex scenario you would have different pipeline layouts for different descriptor set layouts that could be reused
		vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo;
		pipelineLayoutCreateInfo.setSetLayoutCount	(1)
			.setPSetLayouts							(&descriptorSetLayout);

		pipelineLayout = CHECK(vulkanDevice->D().createPipelineLayout (pipelineLayoutCreateInfo));
	}

	void setupDescriptorSet()
	{
		// Allocate a new descriptor set from the global descriptor pool
		vk::DescriptorSetAllocateInfo allocInfo = {};
		allocInfo.setDescriptorPool (descriptorPool)
			.setDescriptorSetCount	(1)
			.setPSetLayouts			(&descriptorSetLayout);

		descriptorSet = CHECK(vulkanDevice->D().allocateDescriptorSets (allocInfo)).front();

		// Update the descriptor set determining the shader binding points
		// For every binding point used in a shader there needs to be one
		// descriptor set matching that binding point

		vk::WriteDescriptorSet writeDescriptorSet = {};

		// Binding 0 : Uniform buffer
		writeDescriptorSet.dstBinding = 0; // Binds this uniform buffer to binding point 0
		writeDescriptorSet.setDstSet	(descriptorSet)
			.setDescriptorCount			(1)
			.setDescriptorType			(vk::DescriptorType::eUniformBuffer)
			.setPBufferInfo				(&uniformBufferVS.descriptor);

		vulkanDevice->D().updateDescriptorSets ({writeDescriptorSet}, {});
	}

	void buildCommandBuffers() override
	{

		// Set clear values for all framebuffer attachments with loadOp set to clear
		// We use two attachments (color and depth) that are cleared at the start of the subpass and as such we need to set clear values for both
		vk::ClearValue clearValues[2];
		clearValues[0].color = std::array<float, 4>{ { 0.0f, 0.0f, 0.2f, 1.0f } };
		clearValues[1].depthStencil = { 1.0f, 0 };

		vk::RenderPassBeginInfo renderPassBeginInfo = {};
		renderPassBeginInfo.renderPass = renderPass;
		renderPassBeginInfo.renderArea.offset.x = 0;
		renderPassBeginInfo.renderArea.offset.y = 0;
		renderPassBeginInfo.renderArea.extent.width = width;
		renderPassBeginInfo.renderArea.extent.height = height;
		renderPassBeginInfo.clearValueCount = 2;
		renderPassBeginInfo.pClearValues = clearValues;
		renderPassBeginInfo.setRenderPass	((vk::RenderPass)renderPass)
			.setRenderArea					(vk::Rect2D(vk::Offset2D (0, 0), vk::Extent2D (width,height)))
			.setClearValueCount				(2)
			.setPClearValues				(clearValues);

		vk::CommandBufferBeginInfo cmdBufInfo = {};
		for (int32_t i = 0; i < drawCmdBuffers.size(); ++i)
		{
			renderPassBeginInfo.setFramebuffer(frameBuffers[i]);	// Set target frame buffer

			VK_CHECK_RESULT(drawCmdBuffers[i].begin (cmdBufInfo));
			
			// Start the first sub pass specified in our default render pass setup by the base class
			// This will clear the color and depth attachment
			drawCmdBuffers[i].beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);

			
			vk::Viewport viewport {0, 0, (float)width, (float)height};
			viewport.setMaxDepth (1.0f)
					.setMinDepth (0.0f);
			vk::Rect2D scissor{ { 0,0 },{ width, height } };
	
			drawCmdBuffers[i].setViewport(0, { viewport }); // Update dynamic viewport state
			drawCmdBuffers[i].setScissor (0, {scissor});	// Update dynamic scissor state

			// Bind descriptor sets describing shader binding points
			drawCmdBuffers[i].bindDescriptorSets(vk::PipelineBindPoint::eGraphics,pipelineLayout, 0, descriptorSet, {});

			// Bind the rendering pipeline
			// The pipeline (state object) contains all states of the rendering pipeline, binding it will set all the states specified at pipeline creation time
			drawCmdBuffers[i].bindPipeline (vk::PipelineBindPoint::eGraphics, pipeline);

			drawCmdBuffers[i].bindVertexBuffers (0, vertices.buffer, {0});	// Bind triangle vertex buffer (contains position and colors)
			drawCmdBuffers[i].bindIndexBuffer (indices.buffer, 0, vk::IndexType::eUint32); // Bind triangle index buffer
			drawCmdBuffers[i].drawIndexed (indices.count, 1, 0, 0, 1);					   // Draw indexed triangle

			drawCmdBuffers[i].endRenderPass ();
			// Ending the render pass will add an implicit barrier transitioning the frame buffer color attachment to 
			// VK_IMAGE_LAYOUT_PRESENT_SRC_KHR for presenting it to the windowing system

			VK_CHECK_RESULT(drawCmdBuffers[i].end());
		}
	}

	void prepare ()
	{
		VulkanExampleBase::prepare();
		prepareSynchronizationPrimitives();
		prepareVertices();
		prepareUniformBuffers();
		setupDescriptorSetLayout();
		preparePipelines();
		setupDescriptorPool();
		setupDescriptorSet();
		buildCommandBuffers();
		prepared = true;
	}

	void draw()
	{
		VulkanExampleBase::prepareFrame (); //sets currentBuffer
		// Use a fence to wait until the command buffer has finished execution before using it again
		VK_CHECK_RESULT(vkWaitForFences(device, 1, &waitFences[currentBuffer], VK_TRUE, UINT64_MAX));
		VK_CHECK_RESULT(vkResetFences(device, 1, &waitFences[currentBuffer]));

		// Pipeline stage at which the queue submission will wait (via pWaitSemaphores)
		vk::PipelineStageFlags waitStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		// The submit info structure specifices a command buffer queue submission batch
		//SEMAPHORES ALREADY SET
		submitInfo.setPWaitDstStageMask (&waitStageMask)			
			.setPCommandBuffers (&drawCmdBuffers[currentBuffer])	// Pointer to the list of pipeline stages that the semaphore waits will occur at
			.setCommandBufferCount (1);								// Command buffers(s) to execute in this batch (submission)


		VK_CHECK_RESULT(queue.submit (submitInfo, waitFences[currentBuffer]));	// Submit to the graphics queue passing a wait fence
		VulkanExampleBase::submitFrame();
	}

	virtual void render() override
	{
		if (!prepared)
			return;
		draw();
	}

	virtual void viewChanged() override
	{
		// This function is called by the base example class each time the view is changed by user input
		//updateUniformBuffers();
	}
};

// Windows entry point
VulkanExample *vulkanExample;
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
	vulkanExample = new VulkanExample();
	vulkanExample->initVulkan();
	vulkanExample->setupWindow(hInstance, WndProc);
	vulkanExample->initSwapchain();
	vulkanExample->prepare();
	vulkanExample->renderLoop();
	delete(vulkanExample);
	return 0;
}

