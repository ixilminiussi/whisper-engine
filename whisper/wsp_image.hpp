#ifndef WSP_IMAGE
#define WSP_IMAGE

#include <wsp_custom_types.hpp>

#include <filesystem>
#include <optional>

#include <vulkan/vulkan.hpp>

class cgltf_image;

namespace wsp
{

class Image
{
  public:
    static Image *BuildGlTF(class Device const *, cgltf_image const *, std::filesystem::path const &parentDirectory);

    Image(class Device const *, std::filesystem::path const &, std::string const &name = "");
    Image(class Device const *, vk::ImageCreateInfo createInfo, std::string const &name = "");
    ~Image();

    Image(Image const &) = delete;
    Image &operator=(Image const &) = delete;

    [[nodiscard]] bool AskForVariant(vk::Format format);
    vk::Image GetImage(vk::Format format) const;

#ifndef NDEBUG
#endif
  protected:
    vk::Image BuildImage(vk::Format format);

    std::string _name;
    std::optional<std::filesystem::path> _filepath;

    struct ImageData
    {
        vk::Image image;
        vk::DeviceMemory deviceMemory;
    };

    dictionary<vk::Format, ImageData> _images;

    class Device const *_device; // exceptional, typically not done but here we're "lying" to the user so yes
};

} // namespace wsp

#endif
