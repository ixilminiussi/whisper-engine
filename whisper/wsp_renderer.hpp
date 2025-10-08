#ifndef WSP_RENDERER
#define WSP_RENDERER

#include <wsp_handles.hpp>

#include <vulkan/vulkan.hpp>

#include <tracy/TracyVulkan.hpp>

#include <vector>

namespace wsp
{

class Renderer
{
  public:
    Renderer();
    ~Renderer();

    Renderer(Renderer const &) = delete;
    Renderer &operator=(Renderer const &) = delete;

    void Initialize(std::vector<class Drawable const *> const *);
    void Free();
    void Render() const;
    void Update(double dt);

    static TracyVkCtx GetTracyCtx();

    bool ShouldClose();

  private:
    void Free(class Device const *);

    void CreateInstance();
    static std::vector<char const *> GetRequiredExtensions();
    static void ExtensionsCompatibilityTest();

#ifndef NDEBUG
    bool CheckValidationLayerSupport();
#endif

    std::vector<char const *> const _validationLayers;
    std::vector<char const *> const _deviceExtensions;

    std::unique_ptr<class Graph> _graph;
    std::unique_ptr<class Window> _window;
    std::unique_ptr<class Editor> _editor;
    std::vector<Resource> _resources;

    static vk::Instance _vkInstance;

    static TracyVkCtx tracyCtx;

    bool _freed;
};

} // namespace wsp

#endif
