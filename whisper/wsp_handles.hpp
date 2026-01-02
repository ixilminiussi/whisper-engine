#ifndef WSP_HANDLES
#define WSP_HANDLES

#include <functional>
#include <vector>

// lib
#include <vulkan/vulkan.hpp>

namespace wsp
{

struct Pass
{
    uint32_t index;
    constexpr explicit Pass(uint32_t i) : index(i) {};

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
    uint32_t index;
    constexpr explicit Resource(uint32_t i) : index(i) {};

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
    std::vector<class StaticTextures const *> staticTextures{};
    std::vector<Resource> reads{};
    std::vector<Resource> writes{};
    bool readsUniform{false};

    std::vector<Pass> passDependencies{};

    uint32_t pushConstantSize{0};

    std::string vertFile;
    std::string fragFile;

    std::string debugName{""};

    std::function<void(vk::CommandBuffer, vk::PipelineLayout)> execute;
    vk::PipelineVertexInputStateCreateInfo vertexInputInfo{{}, 0, nullptr, 0, nullptr};

    friend class Graph;

  protected:
    bool rebuildOnChange{false};

    std::vector<Pass> passDependants{};
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

    std::string debugName{""};

    friend class Graph;

  protected:
    std::vector<Pass> writers{};
    std::vector<Pass> readers{};
};

class StaticTextureAllocator
{
  public:
    StaticTextureAllocator(vk::DescriptorSet const *descriptorSet, class Device const *device)
        : _descriptorSet{descriptorSet}, _device{device}
    {
    }
    StaticTextureAllocator() = default;

    void BindStaticTexture(class Texture const *texture) const;

  protected:
    vk::DescriptorSet const *_descriptorSet;
    class Device const *_device;

    uint32_t _ID;
};

} // namespace wsp

#endif
