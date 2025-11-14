#ifndef WSP_GRAPH
#define WSP_GRAPH

#include <wsp_handles.hpp>

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_handles.hpp>

#include <map>
#include <set>
#include <variant>

namespace wsp
{

class Graph
{
  public:
    enum GraphUsage
    {
        eToTransfer = 0,
        eToDescriptorSet = 1
    };

    static size_t const SAMPLER_DEPTH;
    static size_t const SAMPLER_COLOR_CLAMPED;
    static size_t const SAMPLER_COLOR_REPEATED;
    static size_t const MAX_SAMPLER_SETS;

    Graph(size_t width, size_t height);
    ~Graph();

    Graph(Graph const &) = delete;
    Graph &operator=(Graph const &) = delete;

    void SetUboSize(size_t uboSize);
    void SetPopulateUboFunction(std::function<void *()>);
    [[nodiscard]] Resource NewResource(const struct ResourceCreateInfo &);
    Pass NewPass(const struct PassCreateInfo &);

    void SetTexture(Resource, size_t, class Texture const *);

    void Compile(Resource target, GraphUsage);
    void Render(vk::CommandBuffer, size_t frameIndex);

    void Free();

    vk::Image GetTargetImage() const;
    vk::DescriptorSet GetTargetDescriptorSet() const;

    void ChangeUsage(GraphUsage);
    void Resize(size_t width, size_t height);
    static void OnResizeCallback(void *, size_t width, size_t height);

  protected:
    void FlushUbo(void *ubo, size_t frameIndex);

    struct PipelineHolder
    {
        vk::ShaderModule vertShaderModule;
        vk::ShaderModule fragShaderModule;
        vk::PipelineLayout pipelineLayout;
        vk::Pipeline pipeline;
    };

    bool IsSampled(Resource resouce);

    void KhanFindOrder(std::set<Resource> const &, std::set<Pass> const &);

    void Build(Resource, bool silent = false);
    void Build(Pass, bool silent = false);
    void BuildUbo(bool silent = false);

    void Free(Resource);
    void Free(Pass);

    void CreatePipeline(Pass, bool silent = false);
    void CreateSamplers();
    void CreateDescriptor(Resource resource);

    bool FindDependencies(std::set<Resource> *validResources, std::set<Pass> *validPasses, Resource pass,
                          std::set<std::variant<Resource, Pass>> &visitingStack);
    bool FindDependencies(std::set<Resource> *validResources, std::set<Pass> *validPasses, Pass pass,
                          std::set<std::variant<Resource, Pass>> &visitingStack);

    void Reset();

    std::function<void *()> _populateUbo;

    PassCreateInfo const &GetPassInfo(Pass) const;
    ResourceCreateInfo const &GetResourceInfo(Resource) const;
    vk::RenderPass GetRenderPass(Pass) const;

    std::vector<PassCreateInfo> _passInfos;
    std::vector<ResourceCreateInfo> _resourceInfos;
    Resource _target;
    GraphUsage _usage;

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

    bool _requestsUniform;
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
