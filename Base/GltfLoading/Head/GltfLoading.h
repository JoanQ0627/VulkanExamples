#pragma once

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE

#include "tiny_gltf.h"
#include "VulkanExampleBase.h"

class VulkanGltfModelExample
{
public:
	vks::VulkanDevice* vulkanDevice;
	VkQueue copyQueue;

	struct Vertex
	{
		glm::vec3 pos;
		glm::vec3 normal;
		glm::vec2 uv;
		glm::vec3 color;
	};

	struct
	{
		VkBuffer buffer;
		VkDeviceMemory memory;
	} vertices;

	struct
	{
		int count;
		VkBuffer buffer;
		VkDeviceMemory memory;
	} indices;

	struct Primitive
	{
		uint32_t firstIndex;
		uint32_t indexCount;
		uint32_t materialIndex;
	};

	struct Mesh
	{
		std::vector<Primitive> primitives;
	};

	struct Node
	{
		Node* parent;
		std::vector<Node*> children;
		Mesh mesh;
		glm::mat4 matrix;
		~Node()
		{
			for (auto& child : children)
			{
				delete child;
			}
		}
	};

	struct Material
	{
		glm::vec4 baseColorFactor = glm::vec4(1.0f);
		uint32_t baseColorTextureIndex;
	};

	struct Image
	{
		vks::Texture2D texture;
		VkDescriptorSet descriptorSet;
	};

	struct Texture
	{
		uint32_t imageIndex;
	};

	std::vector<Image> images;
	std::vector<Texture> textures;
	std::vector<Material> materials;
	std::vector<Node*> nodes;

	~VulkanGltfModelExample()
	{
		for (auto& node : nodes)
		{
			delete node;
		}
		
		vkDestroyBuffer(vulkanDevice->logicalDevice, vertices.buffer, nullptr);
		vkDestroyBuffer(vulkanDevice->logicalDevice, indices.buffer, nullptr);
		vkFreeMemory(vulkanDevice->logicalDevice, vertices.memory, nullptr);
		vkFreeMemory(vulkanDevice->logicalDevice, indices.memory, nullptr);

		for (auto& image : images )
		{
			vkDestroyImageView(vulkanDevice->logicalDevice, image.texture.view, nullptr);
			vkDestroyImage(vulkanDevice->logicalDevice, image.texture.image, nullptr);
			vkDestroySampler(vulkanDevice->logicalDevice, image.texture.sampler, nullptr);
			vkFreeMemory(vulkanDevice->logicalDevice, image.texture.deviceMemory, nullptr);
		}
	}

	void loadTexture(tinygltf::Model& input)
	{
		textures.resize(input.textures.size());
		for (size_t i = 0; i < textures.size(); i++)
		{
			textures[i].imageIndex = input.textures[i].source;
		}
	}

	void loadMaterial(tinygltf::Model& input)
	{
		materials.resize(input.materials.size());
		for (size_t i = 0; i < materials.size(); i++)
		{
			tinygltf::Material gltfMaterial = input.materials[i];

			if (gltfMaterial.values.find("baseColorFactor") != gltfMaterial.values.end())
			{
				materials[i].baseColorFactor = glm::make_vec4(gltfMaterial.values["baseColorFactor"].ColorFactor().data());
			}

			if (gltfMaterial.values.find("baseColorTexture") != gltfMaterial.values.end())
			{
				materials[i].baseColorTextureIndex = gltfMaterial.values["baseColorTexture"].TextureIndex();
			}
		}
	}

	void loadImages(tinygltf::Model& input)
	{
		images.resize(input.images.size());

		for (size_t i = 0; i < images.size(); i++)
		{
			tinygltf::Image gltfImage = input.images[i];

			unsigned char* buffer = nullptr;
			bool deleteBuffer = false;
			VkDeviceSize buffersize = 0;

			// rgb -> rgba
			if (gltfImage.component == 3)
			{
				buffersize = gltfImage.width * gltfImage.height * 4;
				buffer = new unsigned char[buffersize];
				unsigned char* rgba = buffer;
				unsigned char* rgb = &gltfImage.image[0];
				for (size_t i = 0; i < gltfImage.width * gltfImage.height; i++)
				{
					memcpy(rgba, rgb, sizeof(unsigned char) * 3);
					rgba += 4;
					rgb += 3;
				}
				deleteBuffer = true;
			}
			else
			{
				buffer = &gltfImage.image[0];
				buffersize = gltfImage.image.size();
			}

			// 这个里面包含了createimage sampler imageview descriptorImageInfo
			images[i].texture.fromBuffer(buffer, buffersize, VK_FORMAT_R8G8B8A8_UNORM, gltfImage.width, gltfImage.height, vulkanDevice, copyQueue);
			if (deleteBuffer)
			{
				delete[] buffer;
			}
		}
	}


};

class Triangle : public VulkanExampleBase
{
public:		
	Triangle();
	~Triangle() override;
	void prepare() override;
	void render() override;
private:
	// - 类内结构体定义
// （后面这些基本都会被集成在 gltfmodel 和 一些包装类如VulkanTexture, VulkanBuffer等里面）
	struct Vertex
	{
		float position[3];
		float color[3];
	};

	struct
	{
		VkDeviceMemory memory{ VK_NULL_HANDLE };
		VkBuffer buffer{ VK_NULL_HANDLE };
	} vertices;

	struct
	{
		VkDeviceMemory memory{ VK_NULL_HANDLE };
		VkBuffer buffer{ VK_NULL_HANDLE };
		uint32_t count{ 0 };
	} indices;

	struct UniformBuffer
	{
		VkDeviceMemory memory{ VK_NULL_HANDLE };
		VkBuffer buffer{ VK_NULL_HANDLE };
		VkDescriptorSet descriptorSet{ VK_NULL_HANDLE };
		uint8_t* mapped{ nullptr };
	};

	struct UniformData
	{
		glm::mat4 projection;
		glm::mat4 model;
		glm::mat4 view;
	};

	std::array<UniformBuffer, c_maxConcurrentFrames> uniformBuffers;
	VkPipelineLayout pipelineLayout{ VK_NULL_HANDLE };
	VkPipeline pipeline{ VK_NULL_HANDLE };
	VkDescriptorSetLayout descriptorSetLayout{ VK_NULL_HANDLE };


	void createVertexBuffer();
	void createUniformBuffers();

	void createDescriptorPool(); 
	void createDescriptorSetLayout();
	void createDescriptorSets();

	void createPipelines();

	void updateUniformBuffers();
	void buildCommandBuffers();
};
 