#ifndef WSP_SAMPLER
#define WSP_SAMPLER

#include <vulkan/vulkan.hpp>

class cgltf_sampler;

namespace wsp
{

class Sampler
{
  public:
    struct CreateInfo
    {
        vk::Filter magFilter{vk::Filter::eLinear};
        vk::Filter minFilter{vk::Filter::eLinear};
        vk::SamplerAddressMode addressModeU{vk::SamplerAddressMode::eClampToEdge};
        vk::SamplerAddressMode addressModeV{vk::SamplerAddressMode::eClampToEdge};
        vk::SamplerAddressMode addressModeW{vk::SamplerAddressMode::eClampToEdge};
        bool compareEnable{false};
        vk::SamplerMipmapMode mipmapMode{vk::SamplerMipmapMode::eLinear};
        bool depth{false};

        inline bool operator<(CreateInfo const &b) const
        {
            return std::tie(magFilter, minFilter, addressModeU, addressModeV, addressModeW, compareEnable, mipmapMode,
                            depth) < std::tie(b.magFilter, b.minFilter, b.addressModeU, b.addressModeV, b.addressModeW,
                                              b.compareEnable, b.mipmapMode, b.depth);
        }
    };

    static CreateInfo GetCreateInfoFromGlTF(cgltf_sampler const *);

    ~Sampler();
    Sampler(class Device const *, Sampler::CreateInfo const &createInfo);

    Sampler(Sampler const &) = delete;
    Sampler &operator=(Sampler const &) = delete;

    vk::Sampler GetSampler() const;

  protected:
    vk::Sampler _sampler;
};

} // namespace wsp

#endif
