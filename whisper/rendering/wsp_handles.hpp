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

    bool operator<(const Pass &other) const
    {
        return index < other.index;
    }
    bool operator==(const Pass &other) const
    {
        return index == other.index;
    }
    bool operator!=(const Pass &other) const
    {
        return index != other.index;
    }
};

struct Resource
{
    size_t index;
    explicit Resource(size_t i) : index(i) {};

    bool operator<(const Resource &other) const
    {
        return index < other.index;
    }
    bool operator==(const Resource &other) const
    {
        return index == other.index;
    }
    bool operator!=(const Resource &other) const
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

enum ResourceRole
{
    eColor,
    eDepth
};

struct ResourceCreateInfo
{
    vk::Format format;
    vk::Extent2D extent{0, 0}; // {0, 0} to follow screen extent
    vk::ClearValue clear;

    ResourceRole role;
    size_t sampler{0};

    std::string debugName{""};

    friend class Graph;

  protected:
    std::vector<Pass> writers;
    std::vector<Pass> readers;
};

} // namespace wsp

#endif
