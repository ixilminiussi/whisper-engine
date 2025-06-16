#ifndef WSP_RENDERER_H
#define WSP_RENDERER_H

namespace wsp
{

class Renderer
{
  public:
    static Renderer *Get();

    ~Renderer() = default;

  private:
    Renderer() = default;

    static Renderer *_instance;
};

} // namespace wsp

#endif
