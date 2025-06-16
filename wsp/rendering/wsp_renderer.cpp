#include "wsp_renderer.h"

namespace wsp
{

Renderer *Renderer::_instance{nullptr};

Renderer *Renderer::Get()
{
    if (!_instance)
    {
        _instance = new Renderer();
    }

    return _instance;
}

} // namespace wsp
