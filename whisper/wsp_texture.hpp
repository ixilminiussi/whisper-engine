#ifndef WSP_TEXTURE_HPP
#define WSP_TEXTURE_HPP

#include <wsp_image.hpp>

#include <stb_image.h>
#include <vulkan/vulkan.hpp>

#include <imgui.h>

class cgltf_texture;
class cgltf_image;
class cgltf_sampler;

namespace wsp
{

class Texture
{
  public:
    struct CreateInfo
    {
        class Sampler const *pSampler{nullptr};
        class Image const *pImage{nullptr};
        bool depth{false};

        Image::CreateInfo imageInfo{};
        bool deferredImageCreation{false};

        std::string name;
    };

    static CreateInfo GetCreateInfoFromGlTF(cgltf_texture const *texture, std::filesystem::path const &parentDirectory);

    Texture(class Device const *, CreateInfo const &createInfo);

    ~Texture();

    Texture(Texture const &) = delete;
    Texture &operator=(Texture const &) = delete;

    vk::ImageView GetImageView() const;
    vk::Sampler GetSampler() const;

    class Image const *GetImage() const;

  protected:
    std::string _name;

    class Sampler const *_sampler;
    class Image const *_image;

    vk::ImageView _imageView;
};

} // namespace wsp

#endif
