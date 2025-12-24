#ifndef WSP_TEXTURE_HPP
#define WSP_TEXTURE_HPP

#include <wsp_image.hpp>
#include <wsp_sampler.hpp>

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
    struct CreateInfo
    {
        Sampler *pSampler{nullptr};
        Image *pImage{nullptr};
        vk::Format format{vk::Format::eUndefined};
        bool depth{false};
        bool cubemap{false};

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

  protected:
    std::string _name;

    class Sampler const *_sampler;

    vk::ImageView _imageView;
};

} // namespace wsp

#endif
