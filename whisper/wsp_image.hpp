#ifndef WSP_IMAGE
#define WSP_IMAGE

#include <wsp_types/dictionary.hpp>

#include <filesystem>

#include <vulkan/vulkan.hpp>

namespace wsp
{

class Image
{
  public:
    struct CreateInfo
    {
        std::filesystem::path filepath{};
        vk::Format format{vk::Format::eUndefined};
        bool cubemap{false};

        inline bool operator<(CreateInfo const &b) const
        {
            return std::tie(filepath, format, cubemap) < std::tie(b.filepath, b.format, cubemap);
        }
    };

    Image(class Device const *, CreateInfo const &createInfo);
    ~Image();

    Image(Image const &) = delete;
    Image &operator=(Image const &) = delete;

    vk::Image GetImage() const;
    vk::Format GetFormat() const;
    std::string GetName() const;
    bool IsCubemap() const;

    friend class Graph;

  protected:
    Image(class Device const *, vk::ImageCreateInfo const &createInfo, std::string const &name);

    void BuildImage(class Device const *, void *pixels, uint32_t width, uint32_t height, size_t size, uint32_t channels,
                    vk::Format format);
    void BuildCubemap(class Device const *, void *pixels, uint32_t width, uint32_t height, size_t size,
                      uint32_t channels, vk::Format format);

    static void CopyFaceToFace(uint32_t left, uint32_t top, uint32_t faceID, void *source, uint32_t s_width,
                               uint32_t s_height, uint32_t s_channels, size_t s_size, void *target, uint32_t t_width,
                               uint32_t t_height, uint32_t t_channels, size_t t_size);

    std::string _name;

    vk::Image _image;
    vk::DeviceMemory _deviceMemory;
    vk::Format _format;

    bool _cubemap;
};

} // namespace wsp

#endif
