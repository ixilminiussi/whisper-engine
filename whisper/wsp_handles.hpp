#ifndef WSP_HANDLES
#define WSP_HANDLES

#include <cstddef>
#include <functional>
#include <vector>

// lib
#include <vulkan/vulkan.hpp>

namespace wsp
{

struct Pass
{
    size_t index;
    constexpr explicit Pass(size_t i) : index(i) {};

    constexpr bool operator<(Pass const &other) const
    {
        return index < other.index;
    }
    constexpr bool operator==(Pass const &other) const
    {
        return index == other.index;
    }
    constexpr bool operator!=(Pass const &other) const
    {
        return index != other.index;
    }
};

struct Resource
{
    size_t index;
    constexpr explicit Resource(size_t i) : index(i) {};

    constexpr bool operator<(Resource const &other) const
    {
        return index < other.index;
    }
    constexpr bool operator==(Resource const &other) const
    {
        return index == other.index;
    }
    constexpr bool operator!=(Resource const &other) const
    {
        return index != other.index;
    }
};

struct PassCreateInfo
{
    std::vector<Resource> reads;
    std::vector<Resource> writes;
    bool readsUniform;
    bool readsStaticTextures;

    size_t pushConstantSize{0};

    std::string vertFile;
    std::string fragFile;

    std::string debugName{""};

    std::function<void(vk::CommandBuffer)> execute;
    vk::PipelineVertexInputStateCreateInfo vertexInputInfo{{}, 0, nullptr, 0, nullptr};

    friend class Graph;

  protected:
    bool rebuildOnChange{false};
};

enum ResourceUsage
{
    eColor,
    eDepth,
};

struct ResourceCreateInfo
{
    // for regular resources
    vk::Format format;
    vk::Extent2D extent{0, 0}; // {0, 0} to follow screen extent
    vk::ClearValue clear;

    ResourceUsage usage;
    size_t sampler{0};

    std::string debugName{""};

    friend class Graph;

  protected:
    std::vector<Pass> writers;
    std::vector<Pass> readers;
};

class StaticTextureAllocator
{
  public:
    StaticTextureAllocator(vk::Sampler sampler, vk::DescriptorSet const *descriptorSet, class Device const *device)
        : _sampler{sampler}, _descriptorSet{descriptorSet}, _device{device}
    {
    }
    StaticTextureAllocator() = default;

    void BindStaticTexture(size_t id, class Texture const *texture) const;

  protected:
    vk::Sampler _sampler;
    vk::DescriptorSet const *_descriptorSet;
    class Device const *_device;
};

} // namespace wsp

#endif
