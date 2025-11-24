#ifndef WSP_STATIC_TEXTURES
#define WSP_STATIC_TEXTURES

#include <vulkan/vulkan.hpp>

#include <string>

namespace wsp
{

class StaticTextures
{
  public:
    StaticTextures(size_t size, std::string const &name = "");
    ~StaticTextures();

    void BuildDummy();
    void Push(std::vector<class Texture *> const &);
    void Clear();

    vk::DescriptorSetLayout GetDescriptorSetLayout() const;
    vk::DescriptorSet GetDescriptorSet() const;

  protected:
    std::string _name;
    size_t _size;
    size_t _offset;

    class Texture *_dummyTexture;
    class Image *_dummyImage;
    class Sampler *_dummySampler;

    vk::DescriptorPool _descriptorPool;
    vk::DescriptorSet _descriptorSet;
    vk::DescriptorSetLayout _descriptorSetLayout;
};

} // namespace wsp

#endif
