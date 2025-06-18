#ifndef WSP_RENDERER_H
#define WSP_RENDERER_H

namespace wsp
{

class Renderer
{
  public:
    Renderer(const class Device *, const class Window *);
    ~Renderer();

    void Free(const class Device *);

  private:
    const class Window *_window;

    bool _freed;
};

} // namespace wsp

#endif
