#include "Triangle.h"
#include <iostream>

Triangle::Triangle()
{
	title = "Triangle";

	settings.overlay = false;
	settings.validation = true;

	camera.type = Camera::CameraType::lookat;
	camera.setPosition(glm::vec3(0.0f, 0.0f, -2.5f));
	camera.setRotation(glm::vec3(0.0f));
	camera.setPerspective(60.0f, (float)width / (float)height, 1.0f, 256.0f);
}

/// <summary>
/// *** 非常重要的流程
/// </summary>
void Triangle::createVertexBuffer()
{
	// 流程大概是
	// 1. vertex index 的原始数据
	// 2. vertex index 的 staging buffer mem（create 和 allocate）
	// 3. map memory, copy data, unmap memory, bindBufferAndMem (为什么需要map的原因是，stagingbuffermem是CPU可见的GPU内存，而不是CPU上的)
	// 4. vertex index 的 gpu buffer mem（create 和 allocate） (纯血GPU端)
	// 5. vkcmdcopybuffer (staging -> gpu) (当然整体cmd流程还得遵循， cmdbuffer, submitqueue这些东西)
	// 后续例子中的2 3 4 5都会放到vulkanDevice里面的createBuffer和copyBuffer去
	// 甚至都会没有一个函数，因为会直接集成到gltfmodel里面去做，** 但是只是包装，实际上都需要走下这些步骤 **


	// 1. vertex index 的原始数据
	std::vector<Vertex> vertexBuffer{
	{ {  1.0f,  1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
	{ { -1.0f,  1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
	{ {  0.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } }
	};
	uint32_t vertexBufferSize = static_cast<uint32_t>(vertexBuffer.size()) * sizeof(Vertex);
	std::vector<uint32_t> indexBuffer{ 0, 1, 2 };
	indices.count = static_cast<uint32_t>(indexBuffer.size());
	uint32_t indexBufferSize = indices.count * sizeof(uint32_t);

	// 2. vertex index 的 staging buffer mem（create 和 allocate）
	// 3. map memory, copy data, unmap memory, bindBufferAndMem
	VkMemoryAllocateInfo memAlloc = vks::initializers::memoryAllocateInfo();
	VkMemoryRequirements memReqs;

	struct StagingBuffer 
	{
		VkDeviceMemory memory;
		VkBuffer buffer;
	};

	struct 
	{
		StagingBuffer vertices;
		StagingBuffer indices;
	} stagingBuffers{};

	void* data;

	VkBufferCreateInfo vertexBufferCI = vks::initializers::bufferCreateInfo();
	vertexBufferCI.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	vertexBufferCI.size = vertexBufferSize;
	VK_CHECK_RESULT(vkCreateBuffer(device, &vertexBufferCI, nullptr, &stagingBuffers.vertices.buffer));
	vkGetBufferMemoryRequirements(device, stagingBuffers.vertices.buffer, &memReqs);
	memAlloc.allocationSize = memReqs.size;
	memAlloc.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, &stagingBuffers.vertices.memory));
	VK_CHECK_RESULT(vkMapMemory(device, stagingBuffers.vertices.memory, 0, memAlloc.allocationSize, 0, &data));
	memcpy(data, vertexBuffer.data(), vertexBufferSize);
	vkUnmapMemory(device, stagingBuffers.vertices.memory);
	VK_CHECK_RESULT(vkBindBufferMemory(device, stagingBuffers.vertices.buffer, stagingBuffers.vertices.memory, 0));

	VkBufferCreateInfo indexBufferCI = vks::initializers::bufferCreateInfo();
	indexBufferCI.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	indexBufferCI.size = indexBufferSize;
	VK_CHECK_RESULT(vkCreateBuffer(device, &indexBufferCI, nullptr, &stagingBuffers.indices.buffer));
	vkGetBufferMemoryRequirements(device, stagingBuffers.indices.buffer, &memReqs);
	memAlloc.allocationSize = memReqs.size;
	memAlloc.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, &stagingBuffers.indices.memory));
	VK_CHECK_RESULT(vkMapMemory(device, stagingBuffers.indices.memory, 0, memAlloc.allocationSize, 0, &data));
	memcpy(data, indexBuffer.data(), indexBufferSize);
	vkUnmapMemory(device, stagingBuffers.indices.memory);
	VK_CHECK_RESULT(vkBindBufferMemory(device, stagingBuffers.indices.buffer, stagingBuffers.indices.memory, 0));

	// 4. vertex index 的 gpu buffer mem（create 和 allocate） (纯血GPU端)
	vertexBufferCI.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	VK_CHECK_RESULT(vkCreateBuffer(device, &vertexBufferCI, nullptr, &vertices.buffer));
	vkGetBufferMemoryRequirements(device, vertices.buffer, &memReqs);
	memAlloc.allocationSize = memReqs.size;
	memAlloc.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, &vertices.memory));
	VK_CHECK_RESULT(vkBindBufferMemory(device, vertices.buffer, vertices.memory, 0));

	indexBufferCI.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	VK_CHECK_RESULT(vkCreateBuffer(device, &vertexBufferCI, nullptr, &indices.buffer));
	vkGetBufferMemoryRequirements(device, indices.buffer, &memReqs);
	memAlloc.allocationSize = memReqs.size;
	memAlloc.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, &indices.memory));
	VK_CHECK_RESULT(vkBindBufferMemory(device, indices.buffer, indices.memory, 0));

	// 5. vkcmdcopybuffer (staging -> gpu) (当然整体cmd流程还得遵循， cmdbuffer, fence, submitqueue这些东西)
	VkCommandBuffer copyCmd = vulkanDevice->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
	VkBufferCopy copyRegion{};
	copyRegion.size = vertexBufferSize;
	vkCmdCopyBuffer(copyCmd, stagingBuffers.vertices.buffer, vertices.buffer, 1, &copyRegion);
	copyRegion.size = indexBufferSize;
	vkCmdCopyBuffer(copyCmd, stagingBuffers.indices.buffer, indices.buffer, 1, &copyRegion);
	VK_CHECK_RESULT(vkEndCommandBuffer(copyCmd));

	VkSubmitInfo submitInfo = vks::initializers::submitInfo();
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &copyCmd;
	VkFenceCreateInfo fenceInfo = vks::initializers::fenceCreateInfo();
	VkFence copyFence;
	VK_CHECK_RESULT(vkCreateFence(device, &fenceInfo, nullptr, &copyFence));
	VK_CHECK_RESULT(vkQueueSubmit(graphicsQueue, 1, &submitInfo, copyFence));
	VK_CHECK_RESULT(vkWaitForFences(device, 1, &copyFence, VK_TRUE, DEFAULT_FENCE_TIMEOUT));

	vkDestroyFence(device, copyFence, nullptr);
	vkFreeCommandBuffers(device, vulkanDevice->commandPool, 1, &copyCmd);

	vkDestroyBuffer(device, stagingBuffers.vertices.buffer, nullptr);
	vkFreeMemory(device, stagingBuffers.vertices.memory, nullptr);
	vkDestroyBuffer(device, stagingBuffers.indices.buffer, nullptr);
	vkFreeMemory(device, stagingBuffers.indices.memory, nullptr);
}

/// <summary>
/// 创建描述符池
/// </summary>
void Triangle::createDescriptorPool()
{
	VkDescriptorPoolSize poolSizes[1]{};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = 1;

	VkDescriptorPoolCreateInfo descriptorPoolInfo = vks::initializers::descriptorPoolCreateInfo(1, poolSizes, c_maxConcurrentFrames);
	VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool));
}

/// <summary>
/// 创建描述符集布局
/// </summary>
void Triangle::createDescriptorSetLayout()
{
	VkDescriptorSetLayoutBinding layoutBinding = vks::initializers::descriptorSetLayoutBinding(
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		VK_SHADER_STAGE_VERTEX_BIT,
		0);

	VkDescriptorSetLayoutCreateInfo descriptorLayout = vks::initializers::descriptorSetLayoutCreateInfo(&layoutBinding, 1);
	VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &descriptorSetLayout));
}

/// <summary>
/// *** 创建描述符集
/// </summary>
void Triangle::createDescriptorSets()
{
	for (uint32_t i = 0; i < c_maxConcurrentFrames; i++)
	{
		VkDescriptorSetAllocateInfo allocInfo = vks::initializers::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayout, 1);
		VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &uniformBuffers[i].descriptorSet));

		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = uniformBuffers[i].buffer;
		bufferInfo.range = sizeof(UniformData);

		VkWriteDescriptorSet writeDescriptorSet = vks::initializers::writeDescriptorSet(uniformBuffers[i].descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &bufferInfo);
		vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, nullptr);
	}
}

/// <summary>
/// *** 创建uniform buffer
/// </summary>
void Triangle::createUniformBuffers()
{
	VkMemoryRequirements memReqs{};
	VkMemoryAllocateInfo memAlloc = vks::initializers::memoryAllocateInfo();
	VkBufferCreateInfo bufferCI = vks::initializers::bufferCreateInfo(
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		sizeof(UniformData));

	for (uint32_t i = 0; i < c_maxConcurrentFrames; i++)
	{
		// create buffer
		VK_CHECK_RESULT(vkCreateBuffer(device, &bufferCI, nullptr, &uniformBuffers[i].buffer));

		// allocate mem
		vkGetBufferMemoryRequirements(device, uniformBuffers[i].buffer, &memReqs);
		memAlloc.allocationSize = memReqs.size;
		memAlloc.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, &uniformBuffers[i].memory));

		// bind buffer mem
		VK_CHECK_RESULT(vkBindBufferMemory(device, uniformBuffers[i].buffer, uniformBuffers[i].memory, 0));

		// mapmem
		VK_CHECK_RESULT(vkMapMemory(device, uniformBuffers[i].memory, 0, sizeof(UniformData), 0, (void**)&uniformBuffers[i].mapped));
	}
}

/// <summary>
/// *** 创建Pipeline
/// </summary>
void Triangle::createPipelines()
{
	// - pipeline layout
	VkPipelineLayoutCreateInfo pipelineLayoutCI = vks::initializers::pipelineLayoutCreateInfo(&descriptorSetLayout, 1);
	VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCI, nullptr, &pipelineLayout));

	// - input assembly state
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);

	// - rasterization state
	VkPipelineRasterizationStateCreateInfo rasterizationState = vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);

	// - color blend state
	VkPipelineColorBlendAttachmentState blendAttachmentState = vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
	VkPipelineColorBlendStateCreateInfo colorBlendState = vks::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);

	// - viewport state(dynamic)

	// - dynameic states
	std::vector<VkDynamicState> dynamicStates = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};
	VkPipelineDynamicStateCreateInfo dynamicState = vks::initializers::pipelineDynamicStateCreateInfo(dynamicStates.data(), static_cast<uint32_t>(dynamicStates.size()));

	// - depth stencil state
	VkPipelineDepthStencilStateCreateInfo depthStencilState = vks::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS);

	// - multisample state
	VkPipelineMultisampleStateCreateInfo multisampleState = vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);

	// - vertex input state
	VkVertexInputBindingDescription vertexInputBinding = vks::initializers::vertexInputBindingDescription(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX);
	std::array<VkVertexInputAttributeDescription, 2> vertexInputAttributes = {
		vks::initializers::vertexInputAttributeDescription(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)),	// position
		vks::initializers::vertexInputAttributeDescription(0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color))		// color
	};

	VkPipelineVertexInputStateCreateInfo vertexInputState = vks::initializers::pipelineVertexInputStateCreateInfo(1, &vertexInputBinding, 2, vertexInputAttributes.data());

	// - shader stages
	std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{};
	shaderStages[0] = loadShader(getShadersPath() + "triangle/triangle.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	shaderStages[1] = loadShader(getShadersPath() + "triangle/triangle.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

	// - create graphics pipeline
	VkGraphicsPipelineCreateInfo pipelineCI = vks::initializers::pipelineCreateInfo(pipelineLayout, renderPass, 0);
	pipelineCI.pInputAssemblyState = &inputAssemblyState;
	pipelineCI.pRasterizationState = &rasterizationState;
	pipelineCI.pColorBlendState = &colorBlendState;
	pipelineCI.pMultisampleState = &multisampleState;
	pipelineCI.pDepthStencilState = &depthStencilState;
	pipelineCI.pVertexInputState = &vertexInputState;
	pipelineCI.stageCount = 2;
	pipelineCI.pStages = shaderStages.data();
	VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipeline));

	// - destroy shader modules
	for (auto& shaderStage : shaderStages)
	{
		vkDestroyShaderModule(device, shaderStage.module, nullptr);
	}
}

/// <summary>
/// *** 在基类的基础上添加一下prepare工作
/// </summary>
void  Triangle::prepare() // override
{
	VulkanExampleBase::prepare();
	// 基类所做的如下
	//createSurface();
	//createSwapChain();
	//createCommandBuffers();
	//createSynchronizationPrimitives();
	//setupDepthStencil();
	//setupRenderPass();
	//createPipelineCache();
	//setupFrameBuffer();
	//prepareOverlay();

	createVertexBuffer();
	createUniformBuffers();
	createDescriptorPool();
	createDescriptorSetLayout();
	createDescriptorSets();
	createPipelines();
	prepared = true;
};


/// <summary>
/// *** 实现纯虚render函数
/// </summary>
void Triangle::render() // overide = 0
{
	if (!prepared)
		return;
	VulkanExampleBase::prepareFrame();
	updateUniformBuffers();
	buildCommandBuffers();
	VulkanExampleBase::submitFrame();
}

/// <summary>
/// *** 每帧更新uniform buffer
/// </summary>
void Triangle::updateUniformBuffers()
{
	UniformData ubo{};
	ubo.projection = camera.matrices.perspective;
	ubo.view = camera.matrices.view;
	ubo.model = glm::mat4(1.0f);
	memcpy(uniformBuffers[currentBuffer].mapped, &ubo, sizeof(ubo));
}

/// <summary>
/// *** 每帧记录cmd buffer
/// </summary>
void Triangle::buildCommandBuffers()
{
	VkCommandBuffer cmdBuffer = drawCmdBuffers[currentBuffer];
	VK_CHECK_RESULT(vkResetCommandBuffer(cmdBuffer, 0));

	VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();

	VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo));

	VkClearValue clearValues[2];
	clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
	clearValues[1].depthStencil = { 1.0f, 0 };

	VkRenderPassBeginInfo renderPassBeginInfo = vks::initializers::renderPassBeginInfo();
	renderPassBeginInfo.renderPass = renderPass;
	renderPassBeginInfo.framebuffer = frameBuffers[currentBuffer];
	renderPassBeginInfo.renderArea.extent.width = width;
	renderPassBeginInfo.renderArea.extent.height = height;
	renderPassBeginInfo.clearValueCount = 2;
	renderPassBeginInfo.pClearValues = clearValues;

	vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport = vks::initializers::viewport((float)width, (float)height, 0.0f, 1.0f);
	vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
	VkRect2D scissor = vks::initializers::rect2D(width, height, 0, 0);
	vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
	vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &uniformBuffers[currentBuffer].descriptorSet, 0, nullptr);

	VkDeviceSize offsets[1] = { 0 };
	vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &vertices.buffer, offsets);
	vkCmdBindIndexBuffer(cmdBuffer, indices.buffer, 0, VK_INDEX_TYPE_UINT32);

	vkCmdDrawIndexed(cmdBuffer, indices.count, 1, 0, 0, 0);

	drawUI(cmdBuffer);
	vkCmdEndRenderPass(cmdBuffer);
	VK_CHECK_RESULT(vkEndCommandBuffer(cmdBuffer));
}


Triangle::~Triangle()
{
	if (device)
	{
		// - pipeline
		vkDestroyPipeline(device, pipeline, nullptr);

		// - pipeline layout
		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);

		// - descriptor set layout
		vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

		// - vertex index buffer mem
		vkDestroyBuffer(device, vertices.buffer, nullptr);
		vkFreeMemory(device, vertices.memory, nullptr);
		vkDestroyBuffer(device, indices.buffer, nullptr);
		vkFreeMemory(device, indices.memory, nullptr);

		// - uniform buffer mem
		for (uint32_t i = 0; i < uniformBuffers.size(); i++)
		{
			vkDestroyBuffer(device, uniformBuffers[i].buffer, nullptr);
			vkFreeMemory(device, uniformBuffers[i].memory, nullptr);
		}
	}
}