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
class Device;
}

namespace fl
{

class Graph
{
  public:
    static const size_t SAMPLER_DEPTH;
    static const size_t SAMPLER_COLOR_CLAMPED;
    static const size_t SAMPLER_COLOR_REPEATED;

    Graph(const wsp::Device *);
    ~Graph();

    Graph(const Graph &) = delete;
    Graph &operator=(const Graph &) = delete;

    [[nodiscard]] Resource NewResource(const struct ResourceCreateInfo &);
    Pass NewPass(const struct PassCreateInfo &);

    void Compile(const wsp::Device *, Resource target);
    void Run(vk::CommandBuffer);

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

    void KhanFindOrder(const std::set<Resource> &, const std::set<Pass> &);

    void Build(const wsp::Device *, Resource);
    void Build(const wsp::Device *, Pass);

    void CreatePipeline(const wsp::Device *, Pass);

    void CreateSamplers(const wsp::Device *);

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

    std::vector<Pass> _orderedPasses;

    bool _freed;
};

} // namespace fl

#endif
