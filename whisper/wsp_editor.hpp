#ifndef WSP_EDITOR
#define WSP_EDITOR

#include <wsp_devkit.hpp>

#include <wsp_handles.hpp>
#include <wsp_typedefs.hpp>

#include <vulkan/vulkan.hpp>

#include <functional>
#include <memory>

#define REGULAR_FONT 0u
#define THUMBNAILS_FONT 1u

struct ImFont;

namespace wsp
{

namespace ubo
{
class Ubo;
}

class Editor
{
  public:
    Editor();
    ~Editor();

    Editor(Editor const &) = delete;
    Editor &operator=(Editor const &) = delete;

    bool ShouldClose() const;
    void Render();
    void Update(double dt);

    void PopulateUbo(ubo::Ubo *) const;

    void OnLeftClick(double dt, int);
    void OnRightClick(double dt, int);

  protected:
    static void InitDockspace(unsigned int dockspaceID);

    static void RenderDockspace();
    void RenderViewport(bool *show);
    bool _isHoveringViewport;

    void RenderContentBrowser(bool *show);
    void RenderFrameInfoMenuBar() const;

    std::unique_ptr<class ViewportCamera> _viewportCamera;
    std::unique_ptr<class InputManager> _inputManager;

    void SelectEnvironment(int i);
    std::vector<std::pair<std::string, std::unique_ptr<class Environment>>> _environments;
    int _selectedEnvironment;

    std::vector<std::function<void()>> _deferredQueue;

    class Node *_scene;

    class Sampler *_previewSampler;

    std::function<void()> _rebuild;

    double _deltaTime; // dont depend on it, just for rendering fps to editor

    WindowID _windowID;
};

} // namespace wsp

#endif
