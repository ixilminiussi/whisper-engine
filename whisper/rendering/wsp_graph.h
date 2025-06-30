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

    void SetUboSize(size_t uboSize);
    [[nodiscard]] Resource NewResource(const struct ResourceCreateInfo &);
    Pass NewPass(const struct PassCreateInfo &);

    void Compile(const Device *, Resource target, GraphGoal);
    void FlushUbo(void *ubo, size_t frameIndex, const Device *);
    void Render(vk::CommandBuffer);

    void Free(const Device *);

    vk::Image GetTargetImage() const;
    vk::DescriptorSet GetTargetDescriptorSet() const;

    void ChangeGoal(const class Device *, GraphGoal);
    void Resize(const Device *, size_t width, size_t height);
    static void OnResizeCallback(void *, const class Device *, size_t width, size_t height);

  protected:
    struct PipelineHolder
    {
        vk::ShaderModule vertShaderModule;
        vk::ShaderModule fragShaderModule;
        vk::PipelineLayout pipelineLayout;
        vk::Pipeline pipeline;
    };

    bool isSampled(Resource resouce);

    void KhanFindOrder(const std::set<Resource> &, const std::set<Pass> &);

    void Build(const Device *, Resource);
    void Build(const Device *, Pass);
    void BuildUbo(const Device *);

    void Free(const Device *, Resource);
    void Free(const Device *, Pass);

    void CreatePipeline(const Device *, Pass);
    void CreateSamplers(const Device *);
    void CreateDescriptor(const Device *, Resource resource);

    void FindDependencies(std::set<Resource> *validResources, std::set<Pass> *validPasses, Resource resource);
    void FindDependencies(std::set<Resource> *validResources, std::set<Pass> *validPasses, Pass resource);

    void Reset(const Device *);

    const PassCreateInfo &GetPassInfo(Pass) const;
    const ResourceCreateInfo &GetResourceInfo(Resource) const;
    vk::RenderPass GetRenderPass(Pass) const;

    std::vector<PassCreateInfo> _passInfos;
    std::vector<ResourceCreateInfo> _resourceInfos;
    Resource _target;
    GraphGoal _goal;

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

    size_t _uboSize;
    std::vector<vk::Buffer> _uboBuffers;
    std::vector<vk::DeviceMemory> _uboDeviceMemories;
    std::vector<vk::DescriptorSet> _uboDescriptorSets;
    std::vector<void *> _uboMappedMemories;
    vk::DescriptorPool _uboDescriptorPool;
    vk::DescriptorSetLayout _uboDescriptorSetLayout;

    std::vector<Pass> _orderedPasses;
    std::set<Resource> _validResources;
    std::set<Pass> _validPasses;

    size_t _width, _height;
    size_t _currentFrameIndex; // only accurate when calling FlushUbo to update

    bool _freed;
};

} // namespace wsp

#endif
