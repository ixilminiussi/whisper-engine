#ifndef WSP_GRAPH
#define WSP_GRAPH

#include <wsp_constants.hpp>
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

    void Compile(Resource target, GraphUsage);
    void Render(vk::CommandBuffer, size_t frameIndex);

    void Free();

    StaticTextureAllocator GenerateStaticTextureAllocator(size_t const samplerID) const;

    vk::Image GetTargetImage() const;
    vk::DescriptorSet GetTargetDescriptorSet() const;

    void ChangeUsage(GraphUsage);
    void Resize(size_t width, size_t height);
    static void OnResizeCallback(void *, size_t width, size_t height);

    vk::Sampler GetSampler(size_t) const;

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

    void Build(Resource);
    void Build(Pass);
    void BuildUbo();
    void BuildStaticTextures();

    void BuildDummyImage();

    void FreeUbo();
    void FreeStaticTextures();
    void FreeDummyImage();

    void Free(Resource);
    void Free(Pass);

    void BuildPipeline(Pass);
    void BuildSamplers();
    void BuildDescriptors(Resource resource);

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

    vk::Image _dummyImage;
    vk::DeviceMemory _dummyImageMemory;
    vk::ImageView _dummyImageView;

    std::map<Resource, std::array<vk::Image, MAX_FRAMES_IN_FLIGHT>> _images;
    std::map<Resource, std::array<vk::DeviceMemory, MAX_FRAMES_IN_FLIGHT>> _deviceMemories;
    std::map<Resource, std::array<vk::ImageView, MAX_FRAMES_IN_FLIGHT>> _imageViews;

    std::map<Pass, std::array<vk::Framebuffer, MAX_FRAMES_IN_FLIGHT>> _framebuffers;
    std::map<Pass, vk::RenderPass> _renderPasses;
    std::map<Pass, PipelineHolder> _pipelines; // in future, maybe point to an array of pipelines..? Or focus on the ONE
                                               // GIGA SHADER Unity approach?

    std::map<size_t, vk::Sampler> _samplers;
    vk::DescriptorPool _descriptorPool;
    vk::DescriptorSetLayout _descriptorSetLayout;
    std::map<Resource, std::array<vk::DescriptorSet, MAX_FRAMES_IN_FLIGHT>> _descriptorSets;

    vk::DescriptorPool _staticTexturesDescriptorPool;
    vk::DescriptorSet _staticTexturesDescriptorSet;
    vk::DescriptorSetLayout _staticTexturesDescriptorSetLayout;

    bool _requestsUniform;
    size_t _uboSize;
    std::array<vk::Buffer, MAX_FRAMES_IN_FLIGHT> _uboBuffers;
    std::array<vk::DeviceMemory, MAX_FRAMES_IN_FLIGHT> _uboDeviceMemories;
    std::array<vk::DescriptorSet, MAX_FRAMES_IN_FLIGHT> _uboDescriptorSets;
    std::array<void *, MAX_FRAMES_IN_FLIGHT> _uboMappedMemories;
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
