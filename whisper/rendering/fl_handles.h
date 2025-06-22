#ifndef FL_HANDLES
#define FL_HANDLES

#include <cstddef>
#include <functional>
#include <vector>

// lib
#include <vulkan/vulkan.hpp>

namespace fl
{

struct Pass
{
    size_t index;
    explicit Pass(size_t i) : index(i) {};

    bool operator<(const Pass &other) const
    {
        return index < other.index;
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
};

struct PassCreateInfo
{
    std::vector<Resource> reads;
    std::vector<Resource> writes;
    std::function<void(vk::CommandBuffer)> execute;
};

enum ResourceRole
{
    eColor,
    eDepth
};

struct ResourceCreateInfo
{
    vk::Format format{};
    vk::Extent2D extent{};
    ResourceRole role{};
    bool isTarget{false};

    friend class Graph;

  protected:
    std::vector<Pass> writers;
    std::vector<Pass> readers;
};

} // namespace fl

#endif
