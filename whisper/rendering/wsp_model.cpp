#include "wsp_model.h"

using namespace wsp;

vk::PipelineVertexInputStateCreateInfo Model::Vertex::GetVertexInputInfo()
{
    std::vector<vk::VertexInputBindingDescription> bindingDescriptions(1);
    bindingDescriptions[0].binding = 0;
    bindingDescriptions[0].stride = sizeof(Vertex);
    bindingDescriptions[0].inputRate = vk::VertexInputRate::eVertex;

    std::vector<vk::VertexInputAttributeDescription> attributeDescriptions{};

    attributeDescriptions.push_back({0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, position)});
    attributeDescriptions.push_back({1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color)});
    attributeDescriptions.push_back({2, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, normal)});
    attributeDescriptions.push_back({3, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, uv)});
    attributeDescriptions.push_back({4, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, tangent)});

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
    vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();

    return vertexInputInfo;
}

bool Model::Vertex::operator==(Vertex const &other) const
{
    return position == other.position && color == other.color && normal == other.normal && uv == other.uv &&
           tangent == other.tangent;
}
