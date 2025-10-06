#include <wsp_mesh.hpp>

#include <wsp_device.hpp>
#include <wsp_devkit.hpp>

#include <glm/geometric.hpp>

#include <spdlog/spdlog.h>

#include <tracy/Tracy.hpp>

#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_handles.hpp>

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

#include <stdexcept>

using namespace wsp;

cgltf_accessor const *Mesh::FindAccessor(cgltf_primitive const *primitive, cgltf_attribute_type type)
{
    for (size_t i = 0; i < primitive->attributes_count; ++i)
    {
        cgltf_attribute const &attribute = primitive->attributes[i];
        if (attribute.type == type)
        {
            return attribute.data;
        }
    }
    return nullptr;
}

Mesh::Mesh(Device const *device, cgltf_mesh const *mesh) : _freed{false}
{
    check(device);

    ZoneScopedN("load mesh");

    std::vector<Vertex> vertices{};
    std::vector<uint32_t> indices{};

    vertices.clear();
    indices.clear();

    uint32_t vertexOffset = 0;
    uint32_t indexOffset = 0;
    uint32_t totalVertexCount = 0;

    for (size_t i = 0; i < mesh->primitives_count; ++i)
    {
        cgltf_primitive *primitive = &mesh->primitives[i];
        check(primitive);

        cgltf_accessor const *positionAccessor = FindAccessor(primitive, cgltf_attribute_type_position);
        check(positionAccessor);
        cgltf_accessor const *normalAccessor = FindAccessor(primitive, cgltf_attribute_type_normal);
        check(normalAccessor);

        // optional
        cgltf_accessor const *tangentAccessor = FindAccessor(primitive, cgltf_attribute_type_tangent);
        cgltf_accessor const *colorAccessor = FindAccessor(primitive, cgltf_attribute_type_color);
        // optional

        cgltf_accessor const *uvAccessor = FindAccessor(primitive, cgltf_attribute_type_texcoord);
        check(uvAccessor);

        uint32_t const vertexCount = positionAccessor->count;
        uint32_t const indexCount = primitive->indices ? primitive->indices->count : 0;

        if (vertexCount < 3)
        {
            throw std::invalid_argument("Mesh: primitive must have at least 3 vertices");
        }

        vertices.reserve(vertices.size() + vertexCount);
        indices.reserve(indices.size() + indexCount);

        totalVertexCount += vertexCount;

        _primitives.emplace_back(Primitive{0, indexCount, indexOffset, vertexCount, vertexOffset});

        for (cgltf_size j = 0; j < static_cast<cgltf_size>(vertexCount); j++)
        {
            glm::vec3 position, normal, uv;
            cgltf_accessor_read_float(positionAccessor, j, &position.x, 3);
            cgltf_accessor_read_float(normalAccessor, j, &normal.x, 3);
            cgltf_accessor_read_float(uvAccessor, j, &uv.x, 2);

            glm::vec3 color{1.0f, 1.0f, 1.0f};
            if (colorAccessor)
            {
                cgltf_accessor_read_float(colorAccessor, j, &color.x, 3);
            }

            glm::vec3 tangent{0.0f, -1.0f, 0.0f};
            if (tangentAccessor)
            {
                cgltf_accessor_read_float(tangentAccessor, j, &tangent.x, 3);
            }

            vertices.emplace_back(Vertex{position, normal, tangent, color, uv});
        }

        for (size_t j = 0; j < indexCount; j++)
        {
            cgltf_size index = cgltf_accessor_read_index(primitive->indices, j);
            indices.emplace_back(static_cast<uint32_t>(index));
            spdlog::critical("{},", index);
        }

        indexOffset += indexCount;
        vertexOffset += vertexCount;

        size_t const bufferSize = sizeof(uint32_t) * indices.size();

        vk::Buffer stagingBuffer;
        vk::DeviceMemory stagingDeviceMemory;

        vk::BufferCreateInfo stagingBufferInfo{};
        stagingBufferInfo.size = bufferSize;
        stagingBufferInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;

        device->CreateBufferAndBindMemory(
            stagingBufferInfo, &stagingBuffer, &stagingDeviceMemory,
            {vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent},
            "mesh staging buffer");

        void *mappedMemory;
        device->MapMemory(stagingDeviceMemory, &mappedMemory);
        memcpy(mappedMemory, indices.data(), bufferSize);

        vk::BufferCreateInfo bufferInfo{};
        bufferInfo.size = bufferSize;
        bufferInfo.usage = {vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst};

        device->CreateBufferAndBindMemory(bufferInfo, &_indexBuffer, &_indexDeviceMemory,
                                          {vk::MemoryPropertyFlagBits::eDeviceLocal}, "mesh buffer");

        device->CopyBuffer(stagingBuffer, &_indexBuffer, bufferSize);
        device->DestroyBuffer(stagingBuffer);
        device->FreeDeviceMemory(stagingDeviceMemory);
    }

    size_t const bufferSize = totalVertexCount * sizeof(Vertex);

    vk::Buffer stagingBuffer;
    vk::DeviceMemory stagingDeviceMemory;

    vk::BufferCreateInfo stagingBufferInfo{};
    stagingBufferInfo.size = bufferSize;
    stagingBufferInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;

    device->CreateBufferAndBindMemory(
        stagingBufferInfo, &stagingBuffer, &stagingDeviceMemory,
        {vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent},
        "mesh vertex staging buffer");

    void *mappedMemory;
    device->MapMemory(stagingDeviceMemory, &mappedMemory);
    memcpy(mappedMemory, vertices.data(), bufferSize);

    vk::BufferCreateInfo bufferInfo{};
    bufferInfo.size = bufferSize;
    bufferInfo.usage = {vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst};

    device->CreateBufferAndBindMemory(bufferInfo, &_vertexBuffer, &_vertexDeviceMemory,
                                      {vk::MemoryPropertyFlagBits::eDeviceLocal}, "mesh vertex buffer");

    device->CopyBuffer(stagingBuffer, &_vertexBuffer, bufferSize);
    device->DestroyBuffer(stagingBuffer);
    device->FreeDeviceMemory(stagingDeviceMemory);

    spdlog::info("Mesh: {} vertices, {} indices, {} primitives", vertices.size(), indices.size(), _primitives.size());
}

Mesh::~Mesh()
{
    if (!_freed)
    {
        spdlog::critical("Mesh: forgot to Free before deletion");
    }
}

void Mesh::Free(Device const *device)
{
    if (_freed)
    {
        spdlog::error("Mesh: already freed mesh");
        return;
    }
    _freed = true;

    check(device);

    device->DestroyBuffer(_vertexBuffer);
    device->FreeDeviceMemory(_vertexDeviceMemory);

    device->DestroyBuffer(_indexBuffer);
    device->FreeDeviceMemory(_indexDeviceMemory);

    spdlog::info("Mesh: freed");
}

vk::PipelineVertexInputStateCreateInfo Mesh::Vertex::GetVertexInputInfo()
{
    static std::vector<vk::VertexInputBindingDescription> bindingDescriptions(1);

    bindingDescriptions[0].binding = 0;
    bindingDescriptions[0].stride = sizeof(Vertex);
    bindingDescriptions[0].inputRate = vk::VertexInputRate::eVertex;

    static std::vector<vk::VertexInputAttributeDescription> attributeDescriptions{};

    attributeDescriptions.emplace_back(0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, position));
    attributeDescriptions.emplace_back(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, normal));
    attributeDescriptions.emplace_back(2, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, tangent));
    attributeDescriptions.emplace_back(3, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color));
    attributeDescriptions.emplace_back(4, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, uv));

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
    vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();

    return vertexInputInfo;
}

struct IndexKey
{
    int v, n, t, m;
    bool operator==(IndexKey const &o) const
    {
        return v == o.v && n == o.n && t == o.t && m == o.m;
    }
};

namespace std
{
template <> struct hash<IndexKey>
{
    size_t operator()(IndexKey const &k) const noexcept
    {
        size_t const h = (uint64_t(k.v) << 32) ^ (uint64_t(k.n) << 16) ^ (uint64_t(k.t) << 4) ^ uint64_t(k.m & 0xF);
        return h;
    }
};
} // namespace std

bool Mesh::Vertex::operator==(Vertex const &other) const
{
    return color == other.color && position == other.position && normal == other.normal && uv == other.uv &&
           tangent == other.tangent;
}

void Mesh::BindAndDraw(vk::CommandBuffer commandBuffer)
{
    vk::Buffer buffers[] = {_vertexBuffer};
    vk::DeviceSize offsets[] = {0};

    commandBuffer.bindVertexBuffers(0, 1, buffers, offsets);

    for (Primitive const &primitive : _primitives)
    {
        commandBuffer.bindIndexBuffer(_indexBuffer, 0, vk::IndexType::eUint32);
        static bool first = true;
        if (first)
        {
            spdlog::critical(
                "index count : {}, instance count : {}, index offset : {}, vertex offset {}, first instance : {}, ",
                primitive.indexCount, 1, primitive.indexOffset, primitive.vertexOffset, 0);
            first = false;
        }
        commandBuffer.drawIndexed(primitive.indexCount, 1, primitive.indexOffset, primitive.vertexOffset, 0);
    }
}
