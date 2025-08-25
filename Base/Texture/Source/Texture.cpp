#include "Texture.h"
#include <iostream>

TextureExample::TextureExample()
{
	title = "Texture";

	settings.overlay = true;
	settings.validation = true;

	camera.type = Camera::CameraType::lookat;
	camera.setPosition(glm::vec3(0.0f, 0.0f, -2.5f));
	camera.setRotation(glm::vec3(0.0f, 15.0f, 0.0f)); // 这里为什么要有一个初始Y rotate 
	camera.setPerspective(60.0f, (float)width / (float)height, 1.0f, 256.0f);
}

TextureExample::~TextureExample()
{
	if (device)
	{
		// - image
		destroyTextureImage();

		// - pipeline
		vkDestroyPipeline(device, pipeline, nullptr);

		// - pipeline layout
		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);

		// - descriptor set layout
		vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

		// - vertex index buffer mem
		vertexBuffer.destroy();
		indexBuffer.destroy();

		// - uniform buffer mem
		for (auto& uniform : uniformBuffers)
		{
			uniform.destroy();
		}
	}
}

void TextureExample::getEnabledFeatures() // override
{
	if (deviceFeatures.samplerAnisotropy)
	{
		enabledFeatures.samplerAnisotropy = VK_TRUE;
	}
}

void TextureExample::loadAndCreateTexture()
{
	// - KTX文件加载texture到CPU内存

	// - createimage, texture mem 到 GPU mem

	// - generate mipmap (barrier)

	// - create sampler

	// - create imageview
}

void TextureExample::destroyTextureImage()
{
	vkDestroyImageView(device, texture.imageView, nullptr);
	vkDestroyImage(device, texture.image, nullptr);
	vkDestroySampler(device, texture.sampler, nullptr);
	vkFreeMemory(device, texture.memory, nullptr);
}

/// <summary>
/// *** 非常重要的流程
/// </summary>
void TextureExample::createVertexBuffer()
{
	// 流程大概是
	// 1. vertex index 的原始数据
	// 2. vertex index 的 staging buffer mem（create 和 allocate）
	// 3. map memory, copy data, unmap memory, bindBufferAndMem (为什么需要map的原因是，stagingbuffermem是CPU可见的GPU内存，而不是CPU上的)
	// 4. vertex index 的 gpu buffer mem（create 和 allocate） (纯血GPU端)
	// 5. vkcmdcopybuffer (staging -> gpu) (当然整体cmd流程还得遵循， cmdbuffer, submitqueue这些东西)
	// 后续例子中的2 3 4 5都会放到vulkanDevice里面的createBuffer和copyBuffer去
	// 甚至都会没有一个函数，因为会直接集成到gltfmodel里面去做，** 但是只是包装，实际上都需要走下这些步骤 **
	
	// 直接调用 vulkanDevice里面的createBuffer和copyBuffer
	std::vector<Vertex> vertices =
	{
		{ {  1.0f,  1.0f, 0.0f }, { 1.0f, 1.0f },{ 0.0f, 0.0f, 1.0f } }, // 注意这个法线定义
		{ { -1.0f,  1.0f, 0.0f }, { 0.0f, 1.0f },{ 0.0f, 0.0f, 1.0f } },
		{ { -1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f },{ 0.0f, 0.0f, 1.0f } },
		{ {  1.0f, -1.0f, 0.0f }, { 1.0f, 0.0f },{ 0.0f, 0.0f, 1.0f } }
	};

	std::vector<uint32_t> indices = { 0, 1, 2,  2, 3, 0 };
	indexCount = (uint32_t)indices.size();

	// Create buffers and upload data to the GPU
	struct StagingBuffers 
	{
		vks::Buffer vertices;
		vks::Buffer indices;
	} stagingBuffers;

	// 请不要小看下面的这些接口，里面还是都有大量工作的，需要多看几次去理解，不能只会调接口
	// cpu --map--> stagingbuffer
	VK_CHECK_RESULT(vulkanDevice->createBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffers.vertices, vertices.size() * sizeof(Vertex), vertices.data()));
	VK_CHECK_RESULT(vulkanDevice->createBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffers.indices, indices.size() * sizeof(uint32_t), indices.data()));

	// stagingbuffer --copy--> GPUbuffer
	vulkanDevice->copyBuffer(&stagingBuffers.vertices, &vertexBuffer, graphicsQueue);
	vulkanDevice->copyBuffer(&stagingBuffers.indices, &indexBuffer, graphicsQueue);

	// clean stage
	stagingBuffers.vertices.destroy();
	stagingBuffers.indices.destroy();
}

/// <summary>
/// 创建描述符相关的所有
/// 要注意这三个count的区别和联系
/// </summary>
void TextureExample::createDescriptors()
{
	// - 创建描述符池
	std::vector<VkDescriptorPoolSize> poolSizes
	{
		vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, c_maxConcurrentFrames),
		vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, c_maxConcurrentFrames)
	};

	VkDescriptorPoolCreateInfo descriptorPoolInfo = vks::initializers::descriptorPoolCreateInfo(poolSizes, c_maxConcurrentFrames);
	VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool));

	// - 创建描述符集布局
	std::vector<VkDescriptorSetLayoutBinding> layoutBindings =
	{
		vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0, 1),
		vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, 1)
	};

	VkDescriptorSetLayoutCreateInfo descriptorLayout = vks::initializers::descriptorSetLayoutCreateInfo(layoutBindings);
	VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &descriptorSetLayout));

	// -  创建描述符集
	VkDescriptorImageInfo textureDescriptor{};
	textureDescriptor.imageLayout = texture.imageLayout;
	textureDescriptor.imageView = texture.imageView;
	textureDescriptor.sampler = texture.sampler;

	VkDescriptorSetAllocateInfo allocInfo = vks::initializers::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayout, 1);
	for (uint32_t i = 0; i < c_maxConcurrentFrames; i++)
	{
		VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets[i]));

		std::vector<VkWriteDescriptorSet> writeDescriptorSets
		{
			vks::initializers::writeDescriptorSet(descriptorSets[i], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uniformBuffers[i].descriptorBufferInfo),
			vks::initializers::writeDescriptorSet(descriptorSets[i], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &textureDescriptor)
		};

		vkUpdateDescriptorSets(device, (uint32_t)writeDescriptorSets.size(), writeDescriptorSets.data(), 0, nullptr);
	}
}

/// <summary>
/// *** 创建uniform buffer
/// </summary>
void TextureExample::createUniformBuffers()
{
	for (auto& buffer : uniformBuffers)
	{
		VK_CHECK_RESULT(vulkanDevice->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
			&buffer, sizeof(UniformData), &uniformData));
		VK_CHECK_RESULT(buffer.map());
	}
}

/// <summary>
/// *** 创建Pipeline
/// </summary>
void TextureExample::createPipelines()
{
	// - pipeline layout
	VkPipelineLayoutCreateInfo pipelineLayoutCI = vks::initializers::pipelineLayoutCreateInfo(&descriptorSetLayout, 1);
	VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCI, nullptr, &pipelineLayout));

	// - input assembly state
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TextureExample_LIST, 0, VK_FALSE);

	// - rasterization state
	VkPipelineRasterizationStateCreateInfo rasterizationState = vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);

	// - color blend state
	VkPipelineColorBlendAttachmentState blendAttachmentState = vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
	VkPipelineColorBlendStateCreateInfo colorBlendState = vks::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);

	// - viewport state(dynamic, 虽然是dynamic但是还是需要设置一下个数，而不能传空)
	VkPipelineViewportStateCreateInfo viewportState = vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);

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
	std::vector<VkVertexInputAttributeDescription> vertexInputAttributes = {
		vks::initializers::vertexInputAttributeDescription(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)),	// position
		vks::initializers::vertexInputAttributeDescription(0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv)),				// uv
		vks::initializers::vertexInputAttributeDescription(0, 2, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal))		// normal
	};

	VkPipelineVertexInputStateCreateInfo vertexInputState = vks::initializers::pipelineVertexInputStateCreateInfo(1, &vertexInputBinding,
		(uint32_t)vertexInputAttributes.size(), vertexInputAttributes.data());

	// - shader stages
	std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{};
	shaderStages[0] = loadShader(getShadersPath() + "Texture/texture.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	shaderStages[1] = loadShader(getShadersPath() + "Texture/texture.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

	// - create graphics pipeline
	VkGraphicsPipelineCreateInfo pipelineCI = vks::initializers::pipelineCreateInfo(pipelineLayout, renderPass, 0);
	pipelineCI.pInputAssemblyState = &inputAssemblyState;
	pipelineCI.pRasterizationState = &rasterizationState;
	pipelineCI.pColorBlendState = &colorBlendState;
	pipelineCI.pMultisampleState = &multisampleState;
	pipelineCI.pDepthStencilState = &depthStencilState;
	pipelineCI.pVertexInputState = &vertexInputState;
	pipelineCI.pViewportState = &viewportState;
	pipelineCI.stageCount = 2;
	pipelineCI.pStages = shaderStages.data();
	pipelineCI.pDynamicState = &dynamicState;
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
void  TextureExample::prepare() // override
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

	loadAndCreateTexture();
	createVertexBuffer();
	createUniformBuffers();
	createDescriptors();
	createPipelines();
	prepared = true;
};


/// <summary>
/// *** 实现纯虚render函数
/// </summary>
void TextureExample::render() // overide = 0
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
void TextureExample::updateUniformBuffers()
{
	uniformData.projection = camera.matrices.perspective;
	uniformData.view = camera.matrices.view;
	uniformData.model = glm::mat4(1.0f);
	memcpy(uniformBuffers[currentBuffer].mapped, &uniformData, sizeof(uniformData));
}

/// <summary>
/// *** 每帧记录cmd buffer
/// </summary>
void TextureExample::buildCommandBuffers()
{
	VkCommandBuffer cmdBuffer = drawCmdBuffers[currentBuffer];
	VK_CHECK_RESULT(vkResetCommandBuffer(cmdBuffer, 0));

	VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();

	VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo));

	VkClearValue clearValues[2];
	clearValues[0].color = { {0.0f, 0.0f, 1.0f, 1.0f} };
	clearValues[1].depthStencil = { 1.0f, 0 };

	VkRenderPassBeginInfo renderPassBeginInfo = vks::initializers::renderPassBeginInfo();
	renderPassBeginInfo.renderPass = renderPass;
	renderPassBeginInfo.framebuffer = frameBuffers[currentImageIndex];
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
	vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[currentBuffer], 0, nullptr);

	VkDeviceSize offsets[1] = { 0 };
	vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &vertexBuffer.buffer, offsets);
	vkCmdBindIndexBuffer(cmdBuffer, indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

	vkCmdDrawIndexed(cmdBuffer, indexCount, 1, 0, 0, 0);

	drawUI(cmdBuffer);
	vkCmdEndRenderPass(cmdBuffer);
	VK_CHECK_RESULT(vkEndCommandBuffer(cmdBuffer));
}

void TextureExample::OnUpdateUIOverlay(vks::UIOverlay* overlay)
{
	if (overlay->header("Settings")) {
		overlay->sliderFloat("LOD bias", &uniformData.lodBias, 0.0f, (float)texture.mipLevel);
	}
}