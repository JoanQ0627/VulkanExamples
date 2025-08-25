#include "Texture.h"
#include "VulkanTools.h"
#include <iostream>
#include <ktx.h>
#include <ktxVulkan.h>

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
	std::string fileName = getAssetPath() + "textures/metalplate01_rgba.ktx";
	VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;

	if (!vks::tools::fileExists)
	{
		vks::tools::exitFatal("Could not load texture from " + fileName, -1);
	}

	ktxTexture* ktxTexture;
	ktxResult result = ktxTexture_CreateFromNamedFile(fileName.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxTexture);

	assert(result == KTX_SUCCESS);

	texture.height = ktxTexture->baseHeight;
	texture.width = ktxTexture->baseWidth;
	texture.mipLevel = ktxTexture->numLevels;
	ktx_uint8_t* ktxTextureData = ktxTexture_GetData(ktxTexture);	
	ktx_size_t ktxTextureSize = ktxTexture_GetSize(ktxTexture);

	// - createimage, texture mem 到 GPU mem
	VkMemoryAllocateInfo memAllocInfo = vks::initializers::memoryAllocateInfo();
	VkMemoryRequirements memReqs{};

	vks::Buffer stagingBuffer; // 为什么例子中的stage 要和 linerTilling相关？这里需要理解一下

	VK_CHECK_RESULT(vulkanDevice->createBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, ktxTextureSize, ktxTextureData));
	stagingBuffer.unmap();

	VkImageCreateInfo imageCreateInfo = vks::initializers::imageCreateInfo();
	imageCreateInfo.arrayLayers = 1;
	imageCreateInfo.extent.depth = 1;
	imageCreateInfo.extent.height = texture.height;
	imageCreateInfo.extent.width = texture.width;
	imageCreateInfo.format = format;
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageCreateInfo.mipLevels = texture.mipLevel;
	imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	VK_CHECK_RESULT(vkCreateImage(device, &imageCreateInfo, nullptr, &texture.image));

	vkGetImageMemoryRequirements(device, texture.image, &memReqs);
	memAllocInfo.allocationSize = memReqs.size;
	memAllocInfo.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VK_CHECK_RESULT(vkAllocateMemory(device, &memAllocInfo, nullptr, &texture.memory));
	VK_CHECK_RESULT(vkBindImageMemory(device, texture.image, texture.memory, 0));

	// - generate mipmap (barrier)  
	// 这一段流程是非常固定的，暂时没有理解也没有关系，其实重点是在理解barrier
	// 因为Mipmap在游戏里都是引擎生成的静态资源了，不会动态生成的，最主要的还是理解barrier
	std::vector<VkBufferImageCopy> bufferCopyRegions;

	for (uint32_t i = 0; i < texture.mipLevel; i++)
	{
		ktx_size_t offset;
		KTX_error_code result = ktxTexture_GetImageOffset(ktxTexture, i, 0, 0, &offset);
		assert(result == KTX_SUCCESS);

		VkBufferImageCopy bufferCopyRegion = {};
		bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		bufferCopyRegion.imageSubresource.mipLevel = i;
		bufferCopyRegion.imageSubresource.layerCount = 1;
		bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
		bufferCopyRegion.imageExtent.width = std::max(1u, ktxTexture->baseWidth >> i);
		bufferCopyRegion.imageExtent.height = std::max(1u, ktxTexture->baseHeight >> i);
		bufferCopyRegion.imageExtent.depth = 1;
		bufferCopyRegion.bufferOffset = offset;
		bufferCopyRegions.push_back(bufferCopyRegion);
	}

	VkCommandBuffer copyCmd = vulkanDevice->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
	VkImageSubresourceRange subresourceRange = {};
	subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresourceRange.baseMipLevel = 0;
	subresourceRange.levelCount = texture.mipLevel;
	subresourceRange.layerCount = 1;

	VkImageMemoryBarrier imageMemoryBarrier = vks::initializers::imageMemoryBarrier();
	imageMemoryBarrier.image = texture.image;
	imageMemoryBarrier.subresourceRange = subresourceRange;
	imageMemoryBarrier.srcAccessMask = 0;
	imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

	vkCmdPipelineBarrier(
		copyCmd,
		VK_PIPELINE_STAGE_HOST_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		0,
		0, nullptr,
		0, nullptr,
		1, &imageMemoryBarrier);

	// 真正执行copy的地方 实际按照上面bufferCopyRegions copy mipLevel次
	vkCmdCopyBufferToImage(
		copyCmd,
		stagingBuffer.buffer,
		texture.image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		static_cast<uint32_t>(bufferCopyRegions.size()),
		bufferCopyRegions.data());

	imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	vkCmdPipelineBarrier(
		copyCmd,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		0,
		0, nullptr,
		0, nullptr,
		1, &imageMemoryBarrier); // 这个时候已经完成了image的layout转换

	// 这里只是本地缓存, 到descriptor那里用的
	texture.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	
	// 提交命令
	vulkanDevice->flushCommandBuffer(copyCmd, graphicsQueue, true);

	// clean up staging resources
	stagingBuffer.destroy();
	ktxTexture_Destroy(ktxTexture);

	// - create sampler
	VkSamplerCreateInfo samplerCreateInfo = vks::initializers::samplerCreateInfo();
	samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
	samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
	samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.mipLodBias = 0.0f;
	samplerCreateInfo.anisotropyEnable = enabledFeatures.samplerAnisotropy;
	samplerCreateInfo.maxAnisotropy = enabledFeatures.samplerAnisotropy ? vulkanDevice->properties.limits.maxSamplerAnisotropy : 1.0f;
	samplerCreateInfo.minLod = 0.0f;
	samplerCreateInfo.maxLod = (float)texture.mipLevel;
	samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	VK_CHECK_RESULT(vkCreateSampler(device, &samplerCreateInfo, nullptr, &texture.sampler));

	// - create imageview
	VkImageViewCreateInfo viewCreateInfo = vks::initializers::imageViewCreateInfo();
	viewCreateInfo.image = texture.image;
	viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewCreateInfo.format = format;
	viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewCreateInfo.subresourceRange.baseMipLevel = 0;
	viewCreateInfo.subresourceRange.levelCount = texture.mipLevel;
	viewCreateInfo.subresourceRange.baseArrayLayer = 0;
	viewCreateInfo.subresourceRange.layerCount = 1;
	VK_CHECK_RESULT(vkCreateImageView(device, &viewCreateInfo, nullptr, &texture.imageView));
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
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);

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