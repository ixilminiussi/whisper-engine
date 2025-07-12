#include "wsp_model.hpp"

#include "wsp_device.hpp"
#include "wsp_devkit.hpp"

// lib
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_handles.hpp>
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

using namespace wsp;

Model::Model(Device const *device, std::vector<Vertex> const &vertices, std::vector<uint32_t> const &indices)
{
    CreateVertexBuffers(device, vertices);
    CreateIndexBuffers(device, indices);
}

Model::Model(Device const *device, std::string const &filepath) : _freed{false}
{
    std::vector<Vertex> vertices{};
    std::vector<uint32_t> indices{};
    LoadFromFile(filepath, &vertices, &indices);
    CreateVertexBuffers(device, vertices);
    CreateIndexBuffers(device, indices);
}

Model::~Model()
{
    if (!_freed)
    {
        spdlog::critical("Model: forgot to Free before deletion");
    }
}

void Model::Free(Device const *device)
{
    if (_freed)
    {
        spdlog::error("Model: already freed model");
        return;
    }
    _freed = true;

    device->DestroyBuffer(_vertexBuffer);
    device->FreeDeviceMemory(_vertexDeviceMemory);

    device->DestroyBuffer(_indexBuffer);
    device->FreeDeviceMemory(_indexDeviceMemory);

    spdlog::info("Model: freed");
}

vk::PipelineVertexInputStateCreateInfo Model::Vertex::GetVertexInputInfo()
{
    std::vector<vk::VertexInputBindingDescription> bindingDescriptions(1);
    bindingDescriptions[0].binding = 0;
    bindingDescriptions[0].stride = sizeof(Vertex);
    bindingDescriptions[0].inputRate = vk::VertexInputRate::eVertex;

    std::vector<vk::VertexInputAttributeDescription> attributeDescriptions{};

    attributeDescriptions.push_back({0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, position)});
    attributeDescriptions.push_back({1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, normal)});
    attributeDescriptions.push_back({2, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, tangent)});
    attributeDescriptions.push_back({3, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, uv)});
    attributeDescriptions.push_back({4, 0, vk::Format::eR32Sint, offsetof(Vertex, material)});

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
    vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();

    return vertexInputInfo;
}

template <> struct std::hash<wsp::Model::Vertex>
{
    std::size_t operator()(wsp::Model::Vertex const &vertex) const noexcept
    {
        size_t h1 = std::hash<float>{}(vertex.position.x) ^ (std::hash<float>{}(vertex.position.y) << 1) ^
                    (std::hash<float>{}(vertex.position.z) << 2);
        size_t h2 = std::hash<float>{}(vertex.normal.x) ^ (std::hash<float>{}(vertex.normal.y) << 1) ^
                    (std::hash<float>{}(vertex.normal.z) << 2);
        size_t h3 = std::hash<float>{}(vertex.tangent.x) ^ (std::hash<float>{}(vertex.tangent.y) << 1) ^
                    (std::hash<float>{}(vertex.tangent.z) << 2);
        size_t h4 = std::hash<float>{}(vertex.uv.x) ^ (std::hash<float>{}(vertex.uv.y) << 1);
        size_t h5 = std::hash<int>{}(vertex.material);

        return (((((h1 ^ (h2 << 1)) >> 1) ^ h3) << 1) ^ h4) ^ (h5 << 1);
    }
};

bool Model::Vertex::operator==(Vertex const &other) const
{
    return position == other.position && normal == other.normal && uv == other.uv && tangent == other.tangent &&
           material == other.material;
}

void Model::LoadFromFile(std::string const &filepath, std::vector<Vertex> *vertices, std::vector<uint32_t> *indices)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, (std::string(GAME_FILES) + filepath).c_str()))
    {
        throw std::runtime_error("Model: tinyobj ->" + err);
    }

    vertices->clear();
    indices->clear();

    std::unordered_map<Vertex, uint32_t> uniqueVertices{};
    for (tinyobj::shape_t const &shape : shapes)
    {
        size_t index_offset = 0;

        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++)
        {
            int fv = shape.mesh.num_face_vertices[f];     // usually 3
            int material_id = shape.mesh.material_ids[f]; // -1 means no material

            std::vector<Vertex> face(fv);
            for (int v = 0; v < fv; v++)
            {
                tinyobj::index_t const &index = shape.mesh.indices[index_offset + v];
                Vertex vertex{};

                if (index.vertex_index >= 0)
                {
                    vertex.position = {
                        attrib.vertices[3 * index.vertex_index + 0],
                        attrib.vertices[3 * index.vertex_index + 1],
                        attrib.vertices[3 * index.vertex_index + 2],
                    };
                }

                if (index.normal_index >= 0)
                {
                    vertex.normal = {
                        attrib.normals[3 * index.normal_index + 0],
                        attrib.normals[3 * index.normal_index + 1],
                        attrib.normals[3 * index.normal_index + 2],
                    };
                }

                if (index.texcoord_index >= 0)
                {
                    vertex.uv = {
                        attrib.texcoords[2 * index.texcoord_index + 0],
                        attrib.texcoords[2 * index.texcoord_index + 1],
                    };
                }

                vertex.material = material_id;

                face[v] = vertex;
            }

            // triangle via fan
            for (int v = 1; v < fv - 1; ++v)
            {
                Vertex v0 = face[0];
                Vertex v1 = face[v];
                Vertex v2 = face[v + 1];

                // Tangent calculation
                glm::vec3 edge1 = v1.position - v0.position;
                glm::vec3 edge2 = v2.position - v0.position;
                glm::vec2 deltaUV1 = v1.uv - v0.uv;
                glm::vec2 deltaUV2 = v2.uv - v0.uv;

                float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
                glm::vec3 tangent = f * (edge1 * deltaUV2.y - edge2 * deltaUV1.y);

                std::array<Vertex, 3> triVerts = {v0, v1, v2};
                for (auto &vtx : triVerts)
                {
                    if (uniqueVertices.count(vtx) == 0)
                    {
                        vtx.tangent = tangent;
                        uniqueVertices[vtx] = static_cast<uint32_t>(vertices->size());
                        vertices->push_back(vtx);
                    }
                    else
                    {
                        uint32_t idx = uniqueVertices[vtx];
                        vertices->at(idx).tangent += tangent;
                    }

                    indices->push_back(uniqueVertices[vtx]);
                }
            }

            index_offset += fv;
        }
    }
}

void Model::CreateVertexBuffers(Device const *device, std::vector<Vertex> const &vertices)
{
    check(device);

    _vertexCount = static_cast<uint32_t>(vertices.size());
    if (_vertexCount < 3)
    {
        throw std::invalid_argument("Model: model must have at least 3 vertices");
    }

    size_t const bufferSize = _vertexCount * sizeof(Vertex);

    vk::Buffer stagingBuffer;
    vk::DeviceMemory stagingDeviceMemory;

    vk::BufferCreateInfo stagingBufferInfo{};
    stagingBufferInfo.size = bufferSize;
    stagingBufferInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;

    device->CreateBufferAndBindMemory(
        stagingBufferInfo, &stagingBuffer, &stagingDeviceMemory,
        {vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent},
        "model vertex staging buffer");

    void *mappedMemory;
    device->MapMemory(stagingDeviceMemory, &mappedMemory);
    memcpy(mappedMemory, vertices.data(), bufferSize);

    vk::BufferCreateInfo bufferInfo{};
    bufferInfo.size = bufferSize;
    bufferInfo.usage = {vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst};

    device->CreateBufferAndBindMemory(bufferInfo, &_vertexBuffer, &_vertexDeviceMemory,
                                      {vk::MemoryPropertyFlagBits::eDeviceLocal}, "model vertex buffer");

    device->CopyBuffer(stagingBuffer, &_vertexBuffer, bufferSize);
    device->DestroyBuffer(stagingBuffer);
    device->FreeDeviceMemory(stagingDeviceMemory);
}

void Model::CreateIndexBuffers(Device const *device, std::vector<uint32_t> const &indices)
{
    if (!device)
        return;

    _indexCount = static_cast<uint32_t>(indices.size());
    _hasIndexBuffer = _indexCount > 0;

    if (!_hasIndexBuffer)
        return;

    size_t const bufferSize = sizeof(uint32_t) * _indexCount;

    vk::Buffer stagingBuffer;
    vk::DeviceMemory stagingDeviceMemory;

    vk::BufferCreateInfo stagingBufferInfo{};
    stagingBufferInfo.size = bufferSize;
    stagingBufferInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;

    device->CreateBufferAndBindMemory(
        stagingBufferInfo, &stagingBuffer, &stagingDeviceMemory,
        {vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent}, "model staging buffer");

    void *mappedMemory;
    device->MapMemory(stagingDeviceMemory, &mappedMemory);
    memcpy(mappedMemory, indices.data(), bufferSize);

    vk::BufferCreateInfo bufferInfo{};
    bufferInfo.size = bufferSize;
    bufferInfo.usage = {vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst};

    device->CreateBufferAndBindMemory(bufferInfo, &_indexBuffer, &_indexDeviceMemory,
                                      {vk::MemoryPropertyFlagBits::eDeviceLocal}, "model buffer");

    device->CopyBuffer(stagingBuffer, &_indexBuffer, bufferSize);
    device->DestroyBuffer(stagingBuffer);
    device->FreeDeviceMemory(stagingDeviceMemory);
}

void Model::Bind(vk::CommandBuffer commandBuffer)
{
    vk::Buffer buffers[] = {_vertexBuffer};
    vk::DeviceSize offsets[] = {0};
    commandBuffer.bindVertexBuffers(0, 1, buffers, offsets);

    if (_hasIndexBuffer)
    {
        commandBuffer.bindIndexBuffer(_indexBuffer, 0, vk::IndexType::eUint32);
    }
}

void Model::Draw(vk::CommandBuffer commandBuffer)
{
    if (_hasIndexBuffer)
    {
        commandBuffer.drawIndexed(_indexCount, 1, 0, 0, 0);
    }
    else
    {
        commandBuffer.draw(_vertexCount, 1, 0, 0);
    }
}
