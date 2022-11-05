#include "Model.h"

#include <cassert>
#include <cstring>
#include <unordered_map>

#include "Utils.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "../libs/TinyObjLoader.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

namespace std
{
	template<>
	struct hash<Model::Vertex>
	{
		size_t operator()(Model::Vertex const& vertex) const
		{
			size_t seed = 0;
			HashCombine(seed, vertex.position, vertex.color, vertex.normal, vertex.uv);
			return seed;
		}
	};
}

Model::Model(Device& device, const Data& data): m_device(device)
{
	createVertexBuffer(data.vertices);
	createIndexBuffer(data.indices);
}

Model::~Model()
{

}

std::unique_ptr<Model> Model::CreateModelFromFile(Device& device, const std::string& filePath)
{
	Data data{};
	data.loadModel(filePath);

	//std::cout << "Vertex count: " << data.vertices.size() << std::endl;

	return std::make_unique<Model>(device, data);
}

void Model::bind(VkCommandBuffer commandBuffer)
{
	VkBuffer buffers[] = { m_vertexBuffer->getBuffer() };
	VkDeviceSize offsets[] = { 0 };

	vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);

	if (m_hasIndexBuffer)
	{
		vkCmdBindIndexBuffer(commandBuffer, m_indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);
	}
}

void Model::draw(VkCommandBuffer commandBuffer)
{
	if (m_hasIndexBuffer)
	{
		vkCmdDrawIndexed(commandBuffer, m_indexCount, 1, 0, 0, 0);
	} else
	{
		vkCmdDraw(commandBuffer, m_vertexCount, 1, 0, 0);
	}
}

void Model::createVertexBuffer(const std::vector<Vertex>& vertices)
{
	m_vertexCount = static_cast<uint32_t>(vertices.size());
	assert(m_vertexCount >= 3 && "Vertex count must be at least 3");

	VkDeviceSize bufferSize = sizeof(vertices[0]) * m_vertexCount;
	uint32_t vertexSize = sizeof(vertices[0]);

	// Create temporary buffer on the device (GPU) which we can write to from the host (CPU)
	Buffer stagingBuffer 
	{
		m_device,
		vertexSize,
		m_vertexCount,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	};

	stagingBuffer.map();
	stagingBuffer.writeToBuffer((void*) vertices.data());

	// Create the final buffer on the device and copy the data from the temp buffer into it 
	// (this final buffer is better optimized then the temp buffer because of VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	// but you cannot directly copy data into this buffer from a host, so this kind of buffer would not
	// be ideal if you want to change the data from the buffer a lot)
	m_vertexBuffer = std::make_unique<Buffer>
	(
		m_device,
		vertexSize,
		m_vertexCount,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	m_device.copyBuffer(stagingBuffer.getBuffer(), m_vertexBuffer->getBuffer(), bufferSize);
}

void Model::createIndexBuffer(const std::vector<uint32_t>& indices)
{
	m_indexCount = static_cast<uint32_t>(indices.size());
	m_hasIndexBuffer = m_indexCount > 0;

	if (!m_hasIndexBuffer)
	{
		return;
	}

	VkDeviceSize bufferSize = sizeof(indices[0]) * m_indexCount;
	uint32_t indexSize = sizeof(indices[0]);

	// Create temporary buffer on the device (GPU) which we can write to from the host (CPU)
	Buffer stagingBuffer
	{
		m_device,
		indexSize,
		m_indexCount,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	};

	stagingBuffer.map();
	stagingBuffer.writeToBuffer((void*) indices.data());

	// Create the final buffer on the device and copy the data from the temp buffer into it 
	// (this final buffer is better optimized then the temp buffer because of VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	// but you cannot directly copy data into this buffer from a host, so this kind of buffer would not
	// be ideal if you want to change the data from the buffer a lot)
	m_indexBuffer = std::make_unique<Buffer>
	(
		m_device,
		indexSize,
		m_indexCount,
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	m_device.copyBuffer(stagingBuffer.getBuffer(), m_indexBuffer->getBuffer(), bufferSize);
}

std::vector<VkVertexInputBindingDescription> Model::Vertex::getBindingDescriptions()
{
	// This is a vbo
	std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
	bindingDescriptions[0].binding = 0;
	bindingDescriptions[0].stride = sizeof(Vertex);
	bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	return bindingDescriptions;
}

std::vector<VkVertexInputAttributeDescription> Model::Vertex::getAttributeDescriptions()
{
	std::vector<VkVertexInputAttributeDescription> attributeDescriptions(4);

	attributeDescriptions[0].binding = 0;
	attributeDescriptions[0].location = 0;
	attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[0].offset = offsetof(Vertex, position);

	attributeDescriptions[1].binding = 0;
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[1].offset = offsetof(Vertex, color);

	attributeDescriptions[2].binding = 0;
	attributeDescriptions[2].location = 2;
	attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[2].offset = offsetof(Vertex, normal);

	attributeDescriptions[3].binding = 0;
	attributeDescriptions[3].location = 3;
	attributeDescriptions[3].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptions[3].offset = offsetof(Vertex, uv);

	return attributeDescriptions;
}

void Model::Data::loadModel(const std::string& filePath)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, error;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &error, filePath.c_str()))
	{
		throw std::runtime_error(warn + error);
	}

	vertices.clear();
	indices.clear();

	std::unordered_map<Vertex, uint32_t> uniqueVertices{};

	for (const auto& shape : shapes)
	{
		for (const auto& index : shape.mesh.indices)
		{
			Vertex vertex{};

			if (index.vertex_index >= 0)
			{
				vertex.position =
				{
					attrib.vertices[3 * index.vertex_index + 0],
					attrib.vertices[3 * index.vertex_index + 1],
					attrib.vertices[3 * index.vertex_index + 2],
				};

				vertex.color =
				{
					attrib.colors[3 * index.vertex_index + 0],
					attrib.colors[3 * index.vertex_index + 1],
					attrib.colors[3 * index.vertex_index + 2],
				};
			}

			if (index.normal_index >= 0)
			{
				vertex.normal =
				{
					attrib.normals[3 * index.normal_index + 0],
					attrib.normals[3 * index.normal_index + 1],
					attrib.normals[3 * index.normal_index + 2],
				};
			}

			if (index.texcoord_index >= 0)
			{
				vertex.uv =
				{
					attrib.texcoords[2 * index.texcoord_index + 0],
					attrib.texcoords[2 * index.texcoord_index + 1],
				};
			}

			if (uniqueVertices.count(vertex) == 0)
			{
				uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
				vertices.push_back(vertex);
			}

			indices.push_back(uniqueVertices[vertex]);
		}
	}
}
