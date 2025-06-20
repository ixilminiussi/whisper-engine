// lib
#include <spdlog/spdlog.h>
#include <vulkan/vulkan.hpp>

int main(int argc, const char *argv[])
{
    if (argc < 2)
    {
        spdlog::error("Usage: {0}", argv[0]);
        return 1;
    }

    const char *cstr = argv[1];
    const std::string str(cstr);

    if (str.compare("vk::ApplicationInfo") == 0)
    {
        spdlog::info("vk::ApplicationInfo: {0} vs {1}", sizeof(vk::ApplicationInfo), sizeof(void *));
    }
    if (str.compare("vk::InstanceCreateInfo") == 0)
    {
        spdlog::info("vk::InstanceCreateInfo: {0} vs {1}", sizeof(vk::InstanceCreateInfo), sizeof(void *));
    }
    if (str.compare("vk::PhysicalDeviceFeatures") == 0)
    {
        spdlog::info("vk::PhysicalDeviceFeatures: {0} vs {1}", sizeof(vk::PhysicalDeviceFeatures), sizeof(void *));
    }
    if (str.compare("vk::DeviceQueueCreateInfo") == 0)
    {
        spdlog::info("vk::DeviceQueueCreateInfo: {0} vs {1}", sizeof(vk::DeviceQueueCreateInfo), sizeof(void *));
    }
    if (str.compare("vk::DeviceCreateInfo") == 0)
    {
        spdlog::info("vk::DeviceCreateInfo: {0} vs {1}", sizeof(vk::DeviceCreateInfo), sizeof(void *));
    }
    if (str.compare("vk::PhysicalDeviceProperties") == 0)
    {
        spdlog::info("vk::PhysicalDeviceProperties: {0} vs {1}", sizeof(vk::PhysicalDeviceProperties), sizeof(void *));
    }
    if (str.compare("vk::PhysicalDeviceMemoryProperties") == 0)
    {
        spdlog::info("vk::PhysicalDeviceMemoryProperties: {0} vs {1}", sizeof(vk::PhysicalDeviceMemoryProperties),
                     sizeof(void *));
    }
    if (str.compare("vk::QueueFamilyProperties") == 0)
    {
        spdlog::info("vk::QueueFamilyProperties: {0} vs {1}", sizeof(vk::QueueFamilyProperties), sizeof(void *));
    }
    if (str.compare("vk::ExtensionProperties") == 0)
    {
        spdlog::info("vk::ExtensionProperties: {0} vs {1}", sizeof(vk::ExtensionProperties), sizeof(void *));
    }
    if (str.compare("vk::LayerProperties") == 0)
    {
        spdlog::info("vk::LayerProperties: {0} vs {1}", sizeof(vk::LayerProperties), sizeof(void *));
    }
    if (str.compare("vk::SurfaceCapabilitiesKHR") == 0)
    {
        spdlog::info("vk::SurfaceCapabilitiesKHR: {0} vs {1}", sizeof(vk::SurfaceCapabilitiesKHR), sizeof(void *));
    }
    if (str.compare("vk::SurfaceFormatKHR") == 0)
    {
        spdlog::info("vk::SurfaceFormatKHR: {0} vs {1}", sizeof(vk::SurfaceFormatKHR), sizeof(void *));
    }
    if (str.compare("vk::SwapchainCreateInfoKHR") == 0)
    {
        spdlog::info("vk::SwapchainCreateInfoKHR: {0} vs {1}", sizeof(vk::SwapchainCreateInfoKHR), sizeof(void *));
    }
    if (str.compare("vk::PresentInfoKHR") == 0)
    {
        spdlog::info("vk::PresentInfoKHR: {0} vs {1}", sizeof(vk::PresentInfoKHR), sizeof(void *));
    }
    if (str.compare("vk::ImageSubresourceRange") == 0)
    {
        spdlog::info("vk::ImageSubresourceRange: {0} vs {1}", sizeof(vk::ImageSubresourceRange), sizeof(void *));
    }
    if (str.compare("vk::PipelineShaderStageCreateInfo") == 0)
    {
        spdlog::info("vk::PipelineShaderStageCreateInfo: {0} vs {1}", sizeof(vk::PipelineShaderStageCreateInfo),
                     sizeof(void *));
    }
    if (str.compare("vk::PipelineVertexInputStateCreateInfo") == 0)
    {
        spdlog::info("vk::PipelineVertexInputStateCreateInfo: {0} vs {1}",
                     sizeof(vk::PipelineVertexInputStateCreateInfo), sizeof(void *));
    }
    if (str.compare("vk::PipelineInputAssemblyStateCreateInfo") == 0)
    {
        spdlog::info("vk::PipelineInputAssemblyStateCreateInfo: {0} vs {1}",
                     sizeof(vk::PipelineInputAssemblyStateCreateInfo), sizeof(void *));
    }
    if (str.compare("vk::PipelineViewportStateCreateInfo") == 0)
    {
        spdlog::info("vk::PipelineViewportStateCreateInfo: {0} vs {1}", sizeof(vk::PipelineViewportStateCreateInfo),
                     sizeof(void *));
    }
    if (str.compare("vk::PipelineRasterizationStateCreateInfo") == 0)
    {
        spdlog::info("vk::PipelineRasterizationStateCreateInfo: {0} vs {1}",
                     sizeof(vk::PipelineRasterizationStateCreateInfo), sizeof(void *));
    }
    if (str.compare("vk::PipelineMultisampleStateCreateInfo") == 0)
    {
        spdlog::info("vk::PipelineMultisampleStateCreateInfo: {0} vs {1}",
                     sizeof(vk::PipelineMultisampleStateCreateInfo), sizeof(void *));
    }
    if (str.compare("vk::PipelineColorBlendAttachmentState") == 0)
    {
        spdlog::info("vk::PipelineColorBlendAttachmentState: {0} vs {1}", sizeof(vk::PipelineColorBlendAttachmentState),
                     sizeof(void *));
    }
    if (str.compare("vk::PipelineColorBlendStateCreateInfo") == 0)
    {
        spdlog::info("vk::PipelineColorBlendStateCreateInfo: {0} vs {1}", sizeof(vk::PipelineColorBlendStateCreateInfo),
                     sizeof(void *));
    }
    if (str.compare("vk::PipelineDepthStencilStateCreateInfo") == 0)
    {
        spdlog::info("vk::PipelineDepthStencilStateCreateInfo: {0} vs {1}",
                     sizeof(vk::PipelineDepthStencilStateCreateInfo), sizeof(void *));
    }
    if (str.compare("vk::PipelineLayoutCreateInfo") == 0)
    {
        spdlog::info("vk::PipelineLayoutCreateInfo: {0} vs {1}", sizeof(vk::PipelineLayoutCreateInfo), sizeof(void *));
    }
    if (str.compare("vk::GraphicsPipelineCreateInfo") == 0)
    {
        spdlog::info("vk::GraphicsPipelineCreateInfo: {0} vs {1}", sizeof(vk::GraphicsPipelineCreateInfo),
                     sizeof(void *));
    }
    if (str.compare("vk::ComputePipelineCreateInfo") == 0)
    {
        spdlog::info("vk::ComputePipelineCreateInfo: {0} vs {1}", sizeof(vk::ComputePipelineCreateInfo),
                     sizeof(void *));
    }
    if (str.compare("vk::ShaderModuleCreateInfo") == 0)
    {
        spdlog::info("vk::ShaderModuleCreateInfo: {0} vs {1}", sizeof(vk::ShaderModuleCreateInfo), sizeof(void *));
    }
    if (str.compare("vk::AttachmentDescription") == 0)
    {
        spdlog::info("vk::AttachmentDescription: {0} vs {1}", sizeof(vk::AttachmentDescription), sizeof(void *));
    }
    if (str.compare("vk::AttachmentReference") == 0)
    {
        spdlog::info("vk::AttachmentReference: {0} vs {1}", sizeof(vk::AttachmentReference), sizeof(void *));
    }
    if (str.compare("vk::SubpassDescription") == 0)
    {
        spdlog::info("vk::SubpassDescription: {0} vs {1}", sizeof(vk::SubpassDescription), sizeof(void *));
    }
    if (str.compare("vk::SubpassDependency") == 0)
    {
        spdlog::info("vk::SubpassDependency: {0} vs {1}", sizeof(vk::SubpassDependency), sizeof(void *));
    }
    if (str.compare("vk::RenderPassCreateInfo") == 0)
    {
        spdlog::info("vk::RenderPassCreateInfo: {0} vs {1}", sizeof(vk::RenderPassCreateInfo), sizeof(void *));
    }
    if (str.compare("vk::FramebufferCreateInfo") == 0)
    {
        spdlog::info("vk::FramebufferCreateInfo: {0} vs {1}", sizeof(vk::FramebufferCreateInfo), sizeof(void *));
    }
    if (str.compare("vk::RenderPassBeginInfo") == 0)
    {
        spdlog::info("vk::RenderPassBeginInfo: {0} vs {1}", sizeof(vk::RenderPassBeginInfo), sizeof(void *));
    }
    if (str.compare("vk::CommandPoolCreateInfo") == 0)
    {
        spdlog::info("vk::CommandPoolCreateInfo: {0} vs {1}", sizeof(vk::CommandPoolCreateInfo), sizeof(void *));
    }
    if (str.compare("vk::CommandBufferAllocateInfo") == 0)
    {
        spdlog::info("vk::CommandBufferAllocateInfo: {0} vs {1}", sizeof(vk::CommandBufferAllocateInfo),
                     sizeof(void *));
    }
    if (str.compare("vk::CommandBufferBeginInfo") == 0)
    {
        spdlog::info("vk::CommandBufferBeginInfo: {0} vs {1}", sizeof(vk::CommandBufferBeginInfo), sizeof(void *));
    }
    if (str.compare("vk::CommandBufferInheritanceInfo") == 0)
    {
        spdlog::info("vk::CommandBufferInheritanceInfo: {0} vs {1}", sizeof(vk::CommandBufferInheritanceInfo),
                     sizeof(void *));
    }
    if (str.compare("vk::SubmitInfo") == 0)
    {
        spdlog::info("vk::SubmitInfo: {0} vs {1}", sizeof(vk::SubmitInfo), sizeof(void *));
    }
    if (str.compare("vk::FenceCreateInfo") == 0)
    {
        spdlog::info("vk::FenceCreateInfo: {0} vs {1}", sizeof(vk::FenceCreateInfo), sizeof(void *));
    }
    if (str.compare("vk::SemaphoreCreateInfo") == 0)
    {
        spdlog::info("vk::SemaphoreCreateInfo: {0} vs {1}", sizeof(vk::SemaphoreCreateInfo), sizeof(void *));
    }
    if (str.compare("vk::SemaphoreSubmitInfo") == 0)
    {
        spdlog::info("vk::SemaphoreSubmitInfo: {0} vs {1}", sizeof(vk::SemaphoreSubmitInfo), sizeof(void *));
    }
    if (str.compare("vk::TimelineSemaphoreSubmitInfo") == 0)
    {
        spdlog::info("vk::TimelineSemaphoreSubmitInfo: {0} vs {1}", sizeof(vk::TimelineSemaphoreSubmitInfo),
                     sizeof(void *));
    }
    if (str.compare("vk::SubmitInfo2") == 0)
    {
        spdlog::info("vk::SubmitInfo2: {0} vs {1}", sizeof(vk::SubmitInfo2), sizeof(void *));
    }
    if (str.compare("vk::CommandBufferSubmitInfo") == 0)
    {
        spdlog::info("vk::CommandBufferSubmitInfo: {0} vs {1}", sizeof(vk::CommandBufferSubmitInfo), sizeof(void *));
    }
    if (str.compare("vk::DescriptorSetLayoutBinding") == 0)
    {
        spdlog::info("vk::DescriptorSetLayoutBinding: {0} vs {1}", sizeof(vk::DescriptorSetLayoutBinding),
                     sizeof(void *));
    }
    if (str.compare("vk::DescriptorSetLayoutCreateInfo") == 0)
    {
        spdlog::info("vk::DescriptorSetLayoutCreateInfo: {0} vs {1}", sizeof(vk::DescriptorSetLayoutCreateInfo),
                     sizeof(void *));
    }
    if (str.compare("vk::DescriptorPoolSize") == 0)
    {
        spdlog::info("vk::DescriptorPoolSize: {0} vs {1}", sizeof(vk::DescriptorPoolSize), sizeof(void *));
    }
    if (str.compare("vk::DescriptorPoolCreateInfo") == 0)
    {
        spdlog::info("vk::DescriptorPoolCreateInfo: {0} vs {1}", sizeof(vk::DescriptorPoolCreateInfo), sizeof(void *));
    }
    if (str.compare("vk::DescriptorSetAllocateInfo") == 0)
    {
        spdlog::info("vk::DescriptorSetAllocateInfo: {0} vs {1}", sizeof(vk::DescriptorSetAllocateInfo),
                     sizeof(void *));
    }
    if (str.compare("vk::WriteDescriptorSet") == 0)
    {
        spdlog::info("vk::WriteDescriptorSet: {0} vs {1}", sizeof(vk::WriteDescriptorSet), sizeof(void *));
    }
    if (str.compare("vk::BufferCreateInfo") == 0)
    {
        spdlog::info("vk::BufferCreateInfo: {0} vs {1}", sizeof(vk::BufferCreateInfo), sizeof(void *));
    }
    if (str.compare("vk::MemoryAllocateInfo") == 0)
    {
        spdlog::info("vk::MemoryAllocateInfo: {0} vs {1}", sizeof(vk::MemoryAllocateInfo), sizeof(void *));
    }
    if (str.compare("vk::MemoryRequirements") == 0)
    {
        spdlog::info("vk::MemoryRequirements: {0} vs {1}", sizeof(vk::MemoryRequirements), sizeof(void *));
    }
    if (str.compare("vk::MemoryBarrier") == 0)
    {
        spdlog::info("vk::MemoryBarrier: {0} vs {1}", sizeof(vk::MemoryBarrier), sizeof(void *));
    }
    if (str.compare("vk::MappedMemoryRange") == 0)
    {
        spdlog::info("vk::MappedMemoryRange: {0} vs {1}", sizeof(vk::MappedMemoryRange), sizeof(void *));
    }
    if (str.compare("vk::BufferCopy") == 0)
    {
        spdlog::info("vk::BufferCopy: {0} vs {1}", sizeof(vk::BufferCopy), sizeof(void *));
    }
    if (str.compare("vk::ImageCreateInfo") == 0)
    {
        spdlog::info("vk::ImageCreateInfo: {0} vs {1}", sizeof(vk::ImageCreateInfo), sizeof(void *));
    }
    if (str.compare("vk::ImageViewCreateInfo") == 0)
    {
        spdlog::info("vk::ImageViewCreateInfo: {0} vs {1}", sizeof(vk::ImageViewCreateInfo), sizeof(void *));
    }
    if (str.compare("vk::ImageSubresourceRange") == 0)
    {
        spdlog::info("vk::ImageSubresourceRange: {0} vs {1}", sizeof(vk::ImageSubresourceRange), sizeof(void *));
    }
    if (str.compare("vk::ImageMemoryBarrier") == 0)
    {
        spdlog::info("vk::ImageMemoryBarrier: {0} vs {1}", sizeof(vk::ImageMemoryBarrier), sizeof(void *));
    }
    if (str.compare("vk::Viewport") == 0)
    {
        spdlog::info("vk::Viewport: {0} vs {1}", sizeof(vk::Viewport), sizeof(void *));
    }
    if (str.compare("vk::Rect2D") == 0)
    {
        spdlog::info("vk::Rect2D: {0} vs {1}", sizeof(vk::Rect2D), sizeof(void *));
    }
    if (str.compare("vk::Offset2D") == 0)
    {
        spdlog::info("vk::Offset2D: {0} vs {1}", sizeof(vk::Offset2D), sizeof(void *));
    }
    if (str.compare("vk::Extent2D") == 0)
    {
        spdlog::info("vk::Extent2D: {0} vs {1}", sizeof(vk::Extent2D), sizeof(void *));
    }
    if (str.compare("vk::ClearValue") == 0)
    {
        spdlog::info("vk::ClearValue: {0} vs {1}", sizeof(vk::ClearValue), sizeof(void *));
    }
    if (str.compare("vk::ClearColorValue") == 0)
    {
        spdlog::info("vk::ClearColorValue: {0} vs {1}", sizeof(vk::ClearColorValue), sizeof(void *));
    }
    if (str.compare("vk::ClearDepthStencilValue") == 0)
    {
        spdlog::info("vk::ClearDepthStencilValue: {0} vs {1}", sizeof(vk::ClearDepthStencilValue), sizeof(void *));
    }
    if (str.compare("vk::ImageSubresourceLayers") == 0)
    {
        spdlog::info("vk::ImageSubresourceLayers: {0} vs {1}", sizeof(vk::ImageSubresourceLayers), sizeof(void *));
    }
    if (str.compare("vk::CommandBuffer") == 0)
    {
        spdlog::info("vk::CommandBuffer: {0} vs {1}", sizeof(vk::CommandBuffer), sizeof(void *));
    }
    if (str.compare("vk::CommandPool") == 0)
    {
        spdlog::info("vk::CommandPool: {0} vs {1}", sizeof(vk::CommandPool), sizeof(void *));
    }
    if (str.compare("vk::Device") == 0)
    {
        spdlog::info("vk::Device: {0} vs {1}", sizeof(vk::Device), sizeof(void *));
    }
    if (str.compare("vk::PhysicalDevice") == 0)
    {
        spdlog::info("vk::PhysicalDevice: {0} vs {1}", sizeof(vk::PhysicalDevice), sizeof(void *));
    }
    if (str.compare("vk::Instance") == 0)
    {
        spdlog::info("vk::Instance: {0} vs {1}", sizeof(vk::Instance), sizeof(void *));
    }
    if (str.compare("vk::Queue") == 0)
    {
        spdlog::info("vk::Queue: {0} vs {1}", sizeof(vk::Queue), sizeof(void *));
    }
    if (str.compare("vk::Fence") == 0)
    {
        spdlog::info("vk::Fence: {0} vs {1}", sizeof(vk::Fence), sizeof(void *));
    }
    if (str.compare("vk::Semaphore") == 0)
    {
        spdlog::info("vk::Semaphore: {0} vs {1}", sizeof(vk::Semaphore), sizeof(void *));
    }
    if (str.compare("vk::ShaderModule") == 0)
    {
        spdlog::info("vk::ShaderModule: {0} vs {1}", sizeof(vk::ShaderModule), sizeof(void *));
    }
    if (str.compare("vk::Pipeline") == 0)
    {
        spdlog::info("vk::Pipeline: {0} vs {1}", sizeof(vk::Pipeline), sizeof(void *));
    }
    if (str.compare("vk::PipelineLayout") == 0)
    {
        spdlog::info("vk::PipelineLayout: {0} vs {1}", sizeof(vk::PipelineLayout), sizeof(void *));
    }
    if (str.compare("vk::RenderPass") == 0)
    {
        spdlog::info("vk::RenderPass: {0} vs {1}", sizeof(vk::RenderPass), sizeof(void *));
    }
    if (str.compare("vk::Framebuffer") == 0)
    {
        spdlog::info("vk::Framebuffer: {0} vs {1}", sizeof(vk::Framebuffer), sizeof(void *));
    }
    if (str.compare("vk::DescriptorSet") == 0)
    {
        spdlog::info("vk::DescriptorSet: {0} vs {1}", sizeof(vk::DescriptorSet), sizeof(void *));
    }
    if (str.compare("vk::DescriptorPool") == 0)
    {
        spdlog::info("vk::DescriptorPool: {0} vs {1}", sizeof(vk::DescriptorPool), sizeof(void *));
    }
    if (str.compare("vk::DescriptorSetLayout") == 0)
    {
        spdlog::info("vk::DescriptorSetLayout: {0} vs {1}", sizeof(vk::DescriptorSetLayout), sizeof(void *));
    }
    if (str.compare("vk::Buffer") == 0)
    {
        spdlog::info("vk::Buffer: {0} vs {1}", sizeof(vk::Buffer), sizeof(void *));
    }
    if (str.compare("vk::DeviceMemory") == 0)
    {
        spdlog::info("vk::DeviceMemory: {0} vs {1}", sizeof(vk::DeviceMemory), sizeof(void *));
    }
    if (str.compare("vk::Image") == 0)
    {
        spdlog::info("vk::Image: {0} vs {1}", sizeof(vk::Image), sizeof(void *));
    }
    if (str.compare("vk::ImageView") == 0)
    {
        spdlog::info("vk::ImageView: {0} vs {1}", sizeof(vk::ImageView), sizeof(void *));
    }
    if (str.compare("vk::SwapchainKHR") == 0)
    {
        spdlog::info("vk::SwapchainKHR: {0} vs {1}", sizeof(vk::SwapchainKHR), sizeof(void *));
    }
    if (str.compare("vk::SurfaceKHR") == 0)
    {
        spdlog::info("vk::SurfaceKHR: {0} vs {1}", sizeof(vk::SurfaceKHR), sizeof(void *));
    }
    if (str.compare("vk::Sampler") == 0)
    {
        spdlog::info("vk::Sampler: {0} vs {1}", sizeof(vk::Sampler), sizeof(void *));
    }
    if (str.compare("vk::Event") == 0)
    {
        spdlog::info("vk::Event: {0} vs {1}", sizeof(vk::Event), sizeof(void *));
    }
    if (str.compare("vk::QueryPool") == 0)
    {
        spdlog::info("vk::QueryPool: {0} vs {1}", sizeof(vk::QueryPool), sizeof(void *));
    }
}
