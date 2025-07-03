#ifndef WSP_GRAPH
#define WSP_GRAPH

// flow
#include "wsp_handles.h"

// lib
#include <variant>
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

    static size_t const SAMPLER_DEPTH;
    static size_t const SAMPLER_COLOR_CLAMPED;
    static size_t const SAMPLER_COLOR_REPEATED;
    static size_t const MAX_SAMPLER_SETS;

    Graph(const class Device *, size_t width, size_t height);
    ~Graph();

    Graph(Graph const &) = delete;
    Graph &operator=(Graph const &) = delete;

    void SetUboSize(size_t uboSize);
    [[nodiscard]] Resource NewResource(const struct ResourceCreateInfo &);
    Pass NewPass(const struct PassCreateInfo &);

    void Compile(Device const *, Resource target, GraphGoal);
    void FlushUbo(void *ubo, size_t frameIndex, Device const *);
    void Render(vk::CommandBuffer);

    void Free(Device const *);

    vk::Image GetTargetImage() const;
    vk::DescriptorSet GetTargetDescriptorSet() const;

    void ChangeGoal(const class Device *, GraphGoal);
    void Resize(Device const *, size_t width, size_t height);
    static void OnResizeCallback(void *, const class Device *, size_t width, size_t height);

  protected:
    struct PipelineHolder
    {
        vk::ShaderModule vertShaderModule;
        vk::ShaderModule fragShaderModule;
        vk::PipelineLayout pipelineLayout;
        vk::Pipeline pipeline;
    };

    bool IsSampled(Resource resouce);

    void KhanFindOrder(std::set<Resource> const &, std::set<Pass> const &);

    void Build(Device const *, Resource, bool silent = false);
    void Build(Device const *, Pass, bool silent = false);
    void BuildUbo(Device const *, bool silent = false);

    void Free(Device const *, Resource);
    void Free(Device const *, Pass);

    void CreatePipeline(Device const *, Pass, bool silent = false);
    void CreateSamplers(Device const *);
    void CreateDescriptor(Device const *, Resource resource);

    bool FindDependencies(std::set<Resource> *validResources, std::set<Pass> *validPasses, Resource pass,
                          std::set<std::variant<Resource, Pass>> &visitingStack);
    bool FindDependencies(std::set<Resource> *validResources, std::set<Pass> *validPasses, Pass pass,
                          std::set<std::variant<Resource, Pass>> &visitingStack);

    void Reset(Device const *);

    PassCreateInfo const &GetPassInfo(Pass) const;
    ResourceCreateInfo const &GetResourceInfo(Resource) const;
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
