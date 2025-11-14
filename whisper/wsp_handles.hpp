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
    explicit Pass(size_t i) : index(i) {};

    bool operator<(Pass const &other) const
    {
        return index < other.index;
    }
    bool operator==(Pass const &other) const
    {
        return index == other.index;
    }
    bool operator!=(Pass const &other) const
    {
        return index != other.index;
    }
};

struct Resource
{
    size_t index;
    explicit Resource(size_t i) : index(i) {};

    bool operator<(Resource const &other) const
    {
        return index < other.index;
    }
    bool operator==(Resource const &other) const
    {
        return index == other.index;
    }
    bool operator!=(Resource const &other) const
    {
        return index != other.index;
    }
};

struct PassCreateInfo
{
    std::vector<Resource> reads;
    std::vector<Resource> writes;
    bool readsUniform;

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
    eTexture
};

struct ResourceCreateInfo
{
    // for regular resources
    vk::Format format;
    vk::Extent2D extent{0, 0}; // {0, 0} to follow screen extent
    vk::ClearValue clear;

    ResourceUsage usage;
    size_t sampler{0};

    size_t textureCount{1};

    std::string debugName{""};

    friend class Graph;

  protected:
    std::vector<Pass> writers;
    std::vector<Pass> readers;
};

} // namespace wsp

#endif
