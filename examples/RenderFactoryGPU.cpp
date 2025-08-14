#include <cstring>
#include <cstdlib>

#include "IRenderer.h"
#include "01_eye_rays/eye_ray.h"

#include "vk_context.h"

vk_utils::VulkanDeviceFeatures EyeRayCaster_GPU_SW_ListRequiredDeviceFeatures();
std::shared_ptr<EyeRayCaster> CreateEyeRayCaster_GPU_SW(vk_utils::VulkanContext a_ctx, size_t a_maxThreadsGenerated);

vk_utils::VulkanDeviceFeatures GetRenderGPUFeatures(const char* renderName, const char* accelStruct)
{
  return EyeRayCaster_GPU_SW_ListRequiredDeviceFeatures();
} 

std::shared_ptr<IRenderer> CreateRenderGPU(const char* renderName, const char* accelStruct, const char* buildFormat, const char* layout, 
                                           vk_utils::VulkanContext a_ctx, size_t a_maxThreadsGenerated)
{
  return CreateEyeRayCaster_GPU_SW(a_ctx, a_maxThreadsGenerated);
}                                           

