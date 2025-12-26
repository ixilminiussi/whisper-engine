#ifndef WSP_STATIC_TEXTURES
#define WSP_STATIC_TEXTURES

#include <wsp_constants.hpp>
#include <wsp_typedefs.hpp>
#include <wsp_types/dictionary.hpp>

#include <vulkan/vulkan.hpp>

#include <string>

namespace wsp
{

class StaticTextures
{
  public:
    StaticTextures(uint32_t size, bool cubemap = false, std::string const &name = "");
    ~StaticTextures();

    void Push(std::vector<TextureID> const &);
    void Clear();

    vk::DescriptorSetLayout GetDescriptorSetLayout() const;
    vk::DescriptorSet GetDescriptorSet() const;

    int GetID(TextureID) const;
    uint32_t GetSize() const;

  protected:
    std::string _name;
    uint32_t _size;
    uint32_t _offset;

    std::unordered_map<TextureID, size_t> _staticTextures;

    vk::DescriptorPool _descriptorPool;
    vk::DescriptorSet _descriptorSet;
    vk::DescriptorSetLayout _descriptorSetLayout;
};

} // namespace wsp

#endif
