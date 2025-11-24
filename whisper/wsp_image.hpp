#ifndef WSP_IMAGE
#define WSP_IMAGE

#include <filesystem>

#include <vulkan/vulkan.hpp>

class cgltf_image;

namespace wsp
{

class Image
{
  public:
    static Image *BuildGlTF(class Device const *, cgltf_image const *, std::filesystem::path const &parentDirectory);

    Image(class Device const *, char *pixels, size_t width, size_t height, size_t channels,
          std::string const &name = "");
    Image(class Device const *, vk::ImageCreateInfo createInfo, std::string const &name = "");
    ~Image();

    Image(Image const &) = delete;
    Image &operator=(Image const &) = delete;

    vk::Image GetImage() const;

#ifndef NDEBUG
#endif
  protected:
    std::string _name;

    vk::Image _image;
    vk::DeviceMemory _deviceMemory;
};

} // namespace wsp

#endif
