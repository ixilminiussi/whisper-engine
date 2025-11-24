#ifndef WSP_TEXTURE_HPP
#define WSP_TEXTURE_HPP

#include <cstddef>
#include <stb_image.h>
#include <vulkan/vulkan.hpp>

#ifndef NDEBUG
#include <imgui.h>
#endif

class cgltf_texture;
class cgltf_image;
class cgltf_sampler;

namespace wsp
{

class Texture
{
  public:
    class Builder
    {
      public:
        Builder();
        Builder &GlTF(cgltf_texture const *texture, cgltf_image *const pImage, std::vector<class Image *> const &images,
                      cgltf_sampler *const pSampler, std::vector<class Sampler *> const &samplers);
        Builder &Depth();
        Builder &SetImage(class Image const *);
        Builder &SetSampler(class Sampler const *);
        Builder &Format(vk::Format);
        Builder &Name(std::string const &);
        Texture *Build(class Device const *);

      protected:
        vk::ImageViewCreateInfo _createInfo;
        class Sampler const *_sampler;
        std::string _name;
    };

    ~Texture();

    Texture(Texture const &) = delete;
    Texture &operator=(Texture const &) = delete;

    vk::ImageView GetImageView() const;
    vk::Sampler GetSampler() const;
    void SetID(size_t);
    size_t GetID() const;

  protected:
    Texture(class Device const *, vk::ImageViewCreateInfo const &, class Sampler const *, std::string const &name);

    std::string _name;

    size_t _ID;

    class Sampler const *_sampler;

    vk::ImageView _imageView;
};

} // namespace wsp

#endif
