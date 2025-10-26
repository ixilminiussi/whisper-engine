#ifndef NDEBUG
#ifndef WSP_EDITOR
#define WSP_EDITOR

#include <vulkan/vulkan.hpp>

#include <functional>
#include <memory>

#define REGULAR_FONT 0u
#define THUMBNAILS_FONT 1u

struct ImFont;

using WindowID = size_t;

namespace wsp
{

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

    void Free();

    void PopulateUbo(class GlobalUbo *);

  protected:
    static void InitDockspace(unsigned int dockspaceID);

    static void RenderDockspace();
    void RenderViewport();
    void RenderContentBrowser();

    bool _freed;

    std::unique_ptr<class ViewportCamera> _viewportCamera;
    std::unique_ptr<class AssetsManager> _assetsManager;
    std::unique_ptr<class InputManager> _inputManager;

    std::vector<std::function<void()>> _deferredQueue;

    std::vector<class Drawable const *> *_drawList;

    WindowID _windowID;
};

} // namespace wsp

#endif
#endif
