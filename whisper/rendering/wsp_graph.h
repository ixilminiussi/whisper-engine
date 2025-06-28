#ifndef WSP_GRAPH
#define WSP_GRAPH

// flow
#include "wsp_handles.h"

// lib
#include <vulkan/vulkan.hpp>

// set
#include <map>
#include <set>
#include <vulkan/vulkan_handles.hpp>

namespace wsp
{

class Graph
{
  public:
    enum GraphGoal
    {
        eToTransfer = 0,
        eToDescriptorSet = 1
    };

    static const size_t SAMPLER_DEPTH;
    static const size_t SAMPLER_COLOR_CLAMPED;
    static const size_t SAMPLER_COLOR_REPEATED;
    static const size_t MAX_SAMPLER_SETS;

    Graph(const class Device *, size_t width, size_t height);
    ~Graph();

    Graph(const Graph &) = delete;
    Graph &operator=(const Graph &) = delete;

    [[nodiscard]] Resource NewResource(const struct ResourceCreateInfo &);
    Pass NewPass(const struct PassCreateInfo &);

    void Compile(const wsp::Device *, Resource target, GraphGoal);
    void Run(vk::CommandBuffer);

    static void WindowResizeCallback(void *graph, const wsp::Device *, size_t width, size_t height);

    void Free(const wsp::Device *);

    vk::Image GetTargetImage();
    vk::DescriptorSet GetTargetDescriptorSet();

    void ChangeGoal(const class Device *, GraphGoal);

  protected:
    struct PipelineHolder
    {
        vk::ShaderModule vertShaderModule;
        vk::ShaderModule fragShaderModule;
        vk::PipelineLayout pipelineLayout;
        vk::Pipeline pipeline;
    };

    void OnResize(const wsp::Device *, size_t width, size_t height);

    bool isSampled(Resource resouce);

    void KhanFindOrder(const std::set<Resource> &, const std::set<Pass> &);

    void Build(const wsp::Device *, Resource);
    void Build(const wsp::Device *, Pass);

    void Free(const wsp::Device *, Resource);
    void Free(const wsp::Device *, Pass);

    void CreatePipeline(const wsp::Device *, Pass);
    void CreateSamplers(const wsp::Device *);
    void CreateDescriptor(const wsp::Device *, Resource resource);

    void FindDependencies(std::set<Resource> *validResources, std::set<Pass> *validPasses, Resource resource);
    void FindDependencies(std::set<Resource> *validResources, std::set<Pass> *validPasses, Pass resource);

    void Reset(const wsp::Device *);

    const PassCreateInfo &GetPassInfo(Pass) const;
    const ResourceCreateInfo &GetResourceInfo(Resource) const;
    vk::RenderPass GetRenderPass(Pass) const;

    std::vector<PassCreateInfo> _passInfos;
    std::vector<ResourceCreateInfo> _resourceInfos;
    Resource _target;
    GraphGoal _graphGoal;

    std::map<Resource, vk::Image> _images;
    std::map<Resource, vk::DeviceMemory> _deviceMemories;
    std::map<Resource, vk::ImageView> _imageViews;

    std::map<Pass, vk::Framebuffer> _framebuffers;
    std::map<Pass, vk::RenderPass> _renderPasses;
    std::map<Pass, PipelineHolder> _pipelines; // in future, maybe point to an array of pipelines..? Or focus on the ONE
                                               // GIGA SHADER Unity approach?

    std::map<size_t, vk::Sampler> _samplers;
    vk::DescriptorPool _descriptorPool;
    vk::DescriptorSetLayout _descriptorSetLayout;
    std::map<Resource, vk::DescriptorSet> _descriptorSets;

    std::vector<Pass> _orderedPasses;
    std::set<Resource> _validResources;
    std::set<Pass> _validPasses;

    size_t _width, _height;

    bool _freed;
};

} // namespace wsp

#endif
