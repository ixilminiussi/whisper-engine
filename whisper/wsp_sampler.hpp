#ifndef WSP_SAMPLER
#define WSP_SAMPLER

#include <filesystem>

#include <vulkan/vulkan.hpp>

class cgltf_sampler;

namespace wsp
{

class Sampler
{
  public:
    class Builder
    {
      public:
        Builder();
        Builder &GlTF(cgltf_sampler const *);
        Builder &AddressMode(vk::SamplerAddressMode);
        Builder &Depth();
        Builder &Name(std::string const &);
        Sampler *Build(class Device const *);

      protected:
        vk::SamplerCreateInfo _createInfo;
        std::string _name;
    };

    ~Sampler();

    Sampler(Sampler const &) = delete;
    Sampler &operator=(Sampler const &) = delete;

    vk::Sampler GetSampler() const;

  protected:
    Sampler(class Device const *, vk::SamplerCreateInfo const &createInfo, std::string const &name = "");

    std::string _name;

    vk::Sampler _sampler;

    std::filesystem::path _path; // used for comparaison
};

} // namespace wsp

#endif
