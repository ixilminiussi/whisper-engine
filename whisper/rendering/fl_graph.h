#ifndef FL_GRAPH
#define FL_GRAPH

// flow
#include "fl_handles.h"

// lib
#include <vulkan/vulkan.hpp>

// set
#include <map>
#include <set>
#include <vulkan/vulkan_handles.hpp>

namespace wsp
{
class Window;
class Device;
} // namespace wsp

namespace fl
{

class Graph
{
  public:
    static const size_t SAMPLER_DEPTH;
    static const size_t SAMPLER_COLOR_CLAMPED;
    static const size_t SAMPLER_COLOR_REPEATED;
    static const size_t MAX_SAMPLER_SETS;

    Graph(const wsp::Device *, size_t width, size_t height);
    ~Graph();

    Graph(const Graph &) = delete;
    Graph &operator=(const Graph &) = delete;

    [[nodiscard]] Resource NewResource(const struct ResourceCreateInfo &);
    Pass NewPass(const struct PassCreateInfo &);

    void Compile(const wsp::Device *, Resource target);
    void Run(vk::CommandBuffer);

    static void WindowResizeCallback(void *graph, const wsp::Device *, size_t width, size_t height);

    void Free(const wsp::Device *);

    vk::Image GetTargetImage();

  protected:
    struct PipelineHolder
    {
        vk::ShaderModule vertShaderModule;
        vk::ShaderModule fragShaderModule;
        vk::PipelineLayout pipelineLayout;
        vk::Pipeline pipeline;
    };

    void OnResize(const wsp::Device *, size_t width, size_t height);

    void KhanFindOrder(const std::set<Resource> &, const std::set<Pass> &);

    void Build(const wsp::Device *, Resource);
    void Build(const wsp::Device *, Pass);

    void Free(const wsp::Device *, Resource);
    void Free(const wsp::Device *, Pass);

    void CreatePipeline(const wsp::Device *, Pass);
    void CreateSamplers(const wsp::Device *);
    void CreateSamplerDescriptor(const wsp::Device *, Resource resource);

    void FindDependencies(std::set<Resource> *validResources, std::set<Pass> *validPasses, Resource resource);
    void FindDependencies(std::set<Resource> *validResources, std::set<Pass> *validPasses, Pass resource);

    void Reset(const wsp::Device *);

    const PassCreateInfo &GetPassInfo(Pass) const;
    const ResourceCreateInfo &GetResourceInfo(Resource) const;
    vk::RenderPass GetRenderPass(Pass) const;

    std::vector<PassCreateInfo> _passInfos;
    std::vector<ResourceCreateInfo> _resourceInfos;
    Resource _target;

    std::map<Resource, vk::Image> _images;
    std::map<Resource, vk::DeviceMemory> _deviceMemories;
    std::map<Resource, vk::ImageView> _imageViews;

    std::map<Pass, vk::Framebuffer> _framebuffers;
    std::map<Pass, vk::RenderPass> _renderPasses;
    std::map<Pass, PipelineHolder> _pipelines; // in future, maybe point to an array of pipelines..? Or focus on the ONE
                                               // GIGA SHADER Unity approach?

    std::map<size_t, vk::Sampler> _samplers;
    vk::DescriptorPool _samplerDescriptorPool;
    vk::DescriptorSetLayout _samplerDescriptorSetLayout;
    std::map<Resource, vk::DescriptorSet> _samplerDescriptorSets;

    std::vector<Pass> _orderedPasses;
    std::set<Resource> _validResources;
    std::set<Pass> _validPasses;

    size_t _width, _height;

    bool _freed;
};

} // namespace fl

#endif
