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
        uint32_t mipLevels{1u};

        inline bool operator<(CreateInfo const &b) const
        {
            return std::tie(filepath, format, cubemap, mipLevels) < std::tie(b.filepath, b.format, cubemap, mipLevels);
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
    uint32_t GetMipLevels() const;

    friend class Graph;

  protected:
    Image(class Device const *, vk::ImageCreateInfo const &createInfo, std::string const &name);

    void GenerateMipmaps(class Device const *, vk::Format, int32_t width, int32_t height, uint32_t mipLevels,
                         uint32_t layerCount = 1);
    void BuildImage(class Device const *, void *pixels, uint32_t width, uint32_t height, size_t size, uint32_t channels,
                    vk::Format format, uint32_t mipLevels);
    void BuildCubemap(class Device const *, void *pixels, uint32_t width, uint32_t height, size_t size,
                      uint32_t channels, vk::Format format, uint32_t mipLevels);

    static void CopyFaceToFace(uint32_t left, uint32_t top, uint32_t faceID, void *source, uint32_t s_width,
                               uint32_t s_height, uint32_t s_channels, size_t s_size, void *target, uint32_t t_width,
                               uint32_t t_height, uint32_t t_channels, size_t t_size);

    std::string _name;

    vk::Image _image;
    vk::DeviceMemory _deviceMemory;
    vk::Format _format;

    bool _cubemap;
    uint32_t _mipLevels;
};

} // namespace wsp

#endif
