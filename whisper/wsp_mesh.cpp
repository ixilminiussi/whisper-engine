#include "wsp_assets_manager.hpp"
#include <wsp_mesh.hpp>

#include <wsp_device.hpp>
#include <wsp_devkit.hpp>
#include <wsp_material.hpp>

#include <glm/geometric.hpp>

#include <spdlog/spdlog.h>

#include <tracy/Tracy.hpp>

#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_handles.hpp>

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

#include <stdexcept>

using namespace wsp;

cgltf_accessor const *FindAccessor(cgltf_primitive const *primitive, cgltf_attribute_type type)
{
    for (uint32_t i = 0; i < primitive->attributes_count; ++i)
    {
        cgltf_attribute const &attribute = primitive->attributes[i];
        if (attribute.type == type)
        {
            return attribute.data;
        }
    }
    return nullptr;
}

Mesh *Mesh::BuildGlTF(Device const *device, cgltf_mesh const *mesh, cgltf_material const *pMaterial,
                      std::vector<MaterialID> const &materials, bool recenter)
{
    check(device);
    check(mesh);

    ZoneScopedN("load mesh");

    std::vector<Vertex> vertices{};
    std::vector<uint32_t> indices{};

    std::vector<Primitive> primitives{};

    uint32_t vertexOffset = 0;
    uint32_t indexOffset = 0;
    uint32_t totalVertexCount = 0;

    glm::vec3 averageVertex{};
    for (uint32_t i = 0; i < mesh->primitives_count; ++i)
    {
        cgltf_primitive *primitive = &mesh->primitives[i];
        check(primitive);

        cgltf_accessor const *positionAccessor = FindAccessor(primitive, cgltf_attribute_type_position);
        check(positionAccessor);

        // optional
        cgltf_accessor const *normalAccessor = FindAccessor(primitive, cgltf_attribute_type_normal);
        cgltf_accessor const *tangentAccessor = FindAccessor(primitive, cgltf_attribute_type_tangent);
        cgltf_accessor const *colorAccessor = FindAccessor(primitive, cgltf_attribute_type_color);
        cgltf_accessor const *uvAccessor = FindAccessor(primitive, cgltf_attribute_type_texcoord);
        // optional

        uint32_t const vertexCount = positionAccessor->count;
        uint32_t const indexCount = primitive->indices ? primitive->indices->count : 0;

        if (vertexCount < 3)
        {
            throw std::invalid_argument("Mesh: primitive must have at least 3 vertices");
        }

        vertices.reserve(vertices.size() + vertexCount);
        indices.reserve(indices.size() + indexCount);

        totalVertexCount += vertexCount;

        MaterialID material;
        if (pMaterial)
        {
            uint32_t const materialID = primitive->material - pMaterial;
            check(materialID < materials.size());
            material = materials.at(materialID);
        }

        primitives.emplace_back(Primitive{material, indexCount, indexOffset, vertexCount, vertexOffset});

        for (cgltf_size j = 0; j < static_cast<cgltf_size>(vertexCount); j++)
        {
            glm::vec3 position;
            cgltf_accessor_read_float(positionAccessor, j, &position.x, 3);

            glm::vec3 normal{0.f, 0.f, 1.f};
            glm::vec2 uv{0.f, 0.f};
            glm::vec3 color{1.0f, 1.0f, 1.0f};
            glm::vec4 tangent{0.0f, -1.0f, 0.0f, 1.0f};

            if (normalAccessor)
            {
                cgltf_accessor_read_float(normalAccessor, j, &normal.x, 3);
            }
            if (uvAccessor)
            {
                cgltf_accessor_read_float(uvAccessor, j, &uv.x, 2);
            }
            if (colorAccessor)
            {
                cgltf_accessor_read_float(colorAccessor, j, &color.x, 3);
            }
            if (tangentAccessor)
            {
                cgltf_accessor_read_float(tangentAccessor, j, &tangent.x, 4);
            }

            vertices.emplace_back(Vertex{tangent, position, normal, color, uv});
            averageVertex += position;
        }

        for (uint32_t j = 0; j < indexCount; j++)
        {
            cgltf_size index = cgltf_accessor_read_index(primitive->indices, j);
            indices.emplace_back(static_cast<uint32_t>(index));
        }

        indexOffset += indexCount;
        vertexOffset += vertexCount;
    }

    if (recenter)
    {
        averageVertex /= vertices.size();
        for (Vertex &vertex : vertices)
        {
            vertex.position -= averageVertex;
        }
    }

    return new Mesh{device, vertices, indices, primitives, mesh->name};
}

Mesh::Mesh(Device const *device, std::vector<Vertex> const &vertices, std::vector<uint32_t> const &indices,
           std::vector<Primitive> const &primitives, std::string const &name)
    : _name{name}, _primitives{primitives}
{
    check(device);

    ZoneScopedN("build mesh");

    for (Vertex const &vertex : vertices)
    {
        _radius = std::max(_radius, glm::length(vertex.position));
    }

    { // index buffer
        uint32_t const bufferSize = sizeof(uint32_t) * indices.size();

        vk::Buffer stagingBuffer;
        vk::DeviceMemory stagingDeviceMemory;

        vk::BufferCreateInfo stagingBufferInfo{};
        stagingBufferInfo.size = bufferSize;
        stagingBufferInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;

        device->CreateBufferAndBindMemory(
            stagingBufferInfo, &stagingBuffer, &stagingDeviceMemory,
            {vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent},
            _name + std::string("_index_staging_buffer"));

        void *mappedMemory;
        device->MapMemory(stagingDeviceMemory, &mappedMemory);
        memcpy(mappedMemory, indices.data(), bufferSize);

        vk::BufferCreateInfo bufferInfo{};
        bufferInfo.size = bufferSize;
        bufferInfo.usage = {vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst};

        device->CreateBufferAndBindMemory(bufferInfo, &_indexBuffer, &_indexDeviceMemory,
                                          {vk::MemoryPropertyFlagBits::eDeviceLocal},
                                          name + std::string("_index_buffer"));

        device->CopyBuffer(stagingBuffer, &_indexBuffer, bufferSize);
        device->DestroyBuffer(&stagingBuffer);
        device->FreeDeviceMemory(&stagingDeviceMemory);
    }

    { // vertex buffer
        uint32_t const bufferSize = sizeof(Vertex) * vertices.size();
        vk::Buffer stagingBuffer;
        vk::DeviceMemory stagingDeviceMemory;

        vk::BufferCreateInfo stagingBufferInfo{};
        stagingBufferInfo.size = bufferSize;
        stagingBufferInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;

        device->CreateBufferAndBindMemory(
            stagingBufferInfo, &stagingBuffer, &stagingDeviceMemory,
            {vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent},
            _name + std::string("_vertex_staging_buffer"));

        void *mappedMemory;
        device->MapMemory(stagingDeviceMemory, &mappedMemory);
        memcpy(mappedMemory, vertices.data(), bufferSize);

        vk::BufferCreateInfo bufferInfo{};
        bufferInfo.size = bufferSize;
        bufferInfo.usage = {vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst};

        device->CreateBufferAndBindMemory(bufferInfo, &_vertexBuffer, &_vertexDeviceMemory,
                                          {vk::MemoryPropertyFlagBits::eDeviceLocal},
                                          _name + std::string("_vertex_buffer"));

        device->CopyBuffer(stagingBuffer, &_vertexBuffer, bufferSize);
        device->DestroyBuffer(&stagingBuffer);
        device->FreeDeviceMemory(&stagingDeviceMemory);
    }

    spdlog::info("Mesh: <{}>, {} vertices, {} indices, {} primitives", _name, vertices.size(), indices.size(),
                 _primitives.size());
}

Mesh::~Mesh()
{
    Device const *device = SafeDeviceAccessor::Get();
    check(device);

    device->DestroyBuffer(&_vertexBuffer);
    device->FreeDeviceMemory(&_vertexDeviceMemory);

    device->DestroyBuffer(&_indexBuffer);
    device->FreeDeviceMemory(&_indexDeviceMemory);

    spdlog::debug("Mesh: <{}> freed", _name);
}

vk::PipelineVertexInputStateCreateInfo Mesh::Vertex::GetVertexInputInfo()
{
    static std::vector<vk::VertexInputBindingDescription> bindingDescriptions = []() {
        std::vector<vk::VertexInputBindingDescription> desc(1);
        desc[0].binding = 0;
        desc[0].stride = sizeof(Vertex);
        desc[0].inputRate = vk::VertexInputRate::eVertex;
        return desc;
    }();

    static std::vector<vk::VertexInputAttributeDescription> attributeDescriptions = []() {
        std::vector<vk::VertexInputAttributeDescription> attrs;
        attrs.emplace_back(0, 0, vk::Format::eR32G32B32A32Sfloat, offsetof(Vertex, tangent));
        attrs.emplace_back(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, position));
        attrs.emplace_back(2, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, normal));
        attrs.emplace_back(3, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color));
        attrs.emplace_back(4, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, uv));
        return attrs;
    }();

    static vk::PipelineVertexInputStateCreateInfo const vertexInputInfo = []() {
        vk::PipelineVertexInputStateCreateInfo info{};
        info.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        info.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescriptions.size());
        info.pVertexAttributeDescriptions = attributeDescriptions.data();
        info.pVertexBindingDescriptions = bindingDescriptions.data();
        return info;
    }();

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
    uint32_t operator()(IndexKey const &k) const noexcept
    {
        uint32_t const h = (uint64_t(k.v) << 32) ^ (uint64_t(k.n) << 16) ^ (uint64_t(k.t) << 4) ^ uint64_t(k.m & 0xF);
        return h;
    }
};
} // namespace std

bool Mesh::Vertex::operator==(Vertex const &other) const
{
    return color == other.color && position == other.position && normal == other.normal && uv == other.uv &&
           tangent == other.tangent;
}

void Mesh::Bind(vk::CommandBuffer commandBuffer) const
{
    vk::Buffer buffers[] = {_vertexBuffer};
    vk::DeviceSize offsets[] = {0};

    commandBuffer.bindVertexBuffers(0, 1, buffers, offsets);
}

void Mesh::Draw(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout) const
{
    for (Primitive const &primitive : _primitives)
    {
        PushConstant(primitive, commandBuffer, pipelineLayout);
        commandBuffer.bindIndexBuffer(_indexBuffer, 0, vk::IndexType::eUint32);
        commandBuffer.drawIndexed(primitive.indexCount, 1, primitive.indexOffset, primitive.vertexOffset, 0);
    }
}

float Mesh::GetRadius() const
{
    return _radius;
}

void Mesh::PushConstant(Primitive const &primitive, vk::CommandBuffer commandBuffer,
                        vk::PipelineLayout pipelineLayout) const
{
    Material const *material = AssetsManager::Get()->GetMaterial(primitive.material);

    PushData pushData{};
    pushData.modelMatrix = _transform.GetMatrix();
    pushData.normalMatrix = _transform.GetNormalMatrix();
    pushData.normalMatrix[3][3] = material ? material->GetID() : INVALID_ID;

    commandBuffer.pushConstants(pipelineLayout, vk::ShaderStageFlagBits::eAllGraphics, 0, sizeof(PushData), &pushData);
}
