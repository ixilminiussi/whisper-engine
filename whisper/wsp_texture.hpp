#ifndef WSP_TEXTURE_HPP
#define WSP_TEXTURE_HPP

#include <cstddef>
#include <stb_image.h>
#include <vulkan/vulkan.hpp>

#ifndef NDEBUG
#include <imgui.h>
#endif

namespace wsp
{

class Texture
{
  public:
    Texture(class Device const *, stbi_uc *pixels, int width, int height, int channels);
    ~Texture();

    Texture(Texture const &) = delete;
    Texture &operator=(Texture const &) = delete;

    void Free(class Device const *);

    vk::ImageView GetImageView() const;

#ifndef NDEBUG
    ImTextureID GetImTextureID(vk::Sampler); // generates

  protected:
    ImTextureID _imguiID;
    bool _imguiFlag;
#endif

  protected:
    vk::Image _image;
    vk::ImageView _imageView;
    vk::DeviceMemory _deviceMemory;

    bool _freed;
};

} // namespace wsp

#endif
