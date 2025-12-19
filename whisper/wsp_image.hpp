#ifndef WSP_IMAGE
#define WSP_IMAGE

#include <wsp_types/dictionary.hpp>

#include <filesystem>
#include <optional>

#include <vulkan/vulkan.hpp>

class cgltf_image;

namespace wsp
{

class Image
{
  public:
    struct CreateInfo
    {
        std::filesystem::path filepath{};
        vk::Format format{vk::Format::eR8G8B8Srgb};
        bool cubemap = false;

        inline bool operator<(CreateInfo const &b) const
        {
            return std::tie(filepath, format, cubemap) < std::tie(b.filepath, b.format, b.cubemap);
        }
    };

    Image(class Device const *, CreateInfo const &createInfo);

  public:
    ~Image();

    Image(Image const &) = delete;
    Image &operator=(Image const &) = delete;
    vk::Image GetImage() const;
    std::string GetName() const;

    friend class Graph;

  protected:
    Image(class Device const *, vk::ImageCreateInfo const &createInfo, std::string const &name);

    std::string _name;

    vk::Image _image;
    vk::DeviceMemory _deviceMemory;

    class Device const *_device; // exceptional, typically not done but here we're "lying" to the user so yes
};

} // namespace wsp

#endif
