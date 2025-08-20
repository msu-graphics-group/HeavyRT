#include <cstring>
#include <cstdlib>

#include "IRenderer.h"
#include "01_eye_rays/eye_ray.h"
#include "02_rtao/rtao.h"

#include "vk_context.h"

std::shared_ptr<EyeRayCaster> CreateEyeRayCaster_GPU_SW(vk_utils::VulkanContext a_ctx, size_t a_maxThreadsGenerated);
vk_utils::VulkanDeviceFeatures EyeRayCaster_GPU_SW_ListRequiredDeviceFeatures();

std::shared_ptr<EyeRayCaster> CreateEyeRayCaster_GPU_RQ(vk_utils::VulkanContext a_ctx, size_t a_maxThreadsGenerated);
vk_utils::VulkanDeviceFeatures EyeRayCaster_GPU_RQ_ListRequiredDeviceFeatures();

std::shared_ptr<RTAO> CreateRTAO_GPU_CS(vk_utils::VulkanContext a_ctx, size_t a_maxThreadsGenerated);
vk_utils::VulkanDeviceFeatures RTAO_GPU_CS_ListRequiredDeviceFeatures();

vk_utils::VulkanDeviceFeatures GetRenderGPUFeatures(const char* a_renderName, const char* a_accelStruct)
{
  std::string renderName(a_renderName);
  std::string accelStruct(a_accelStruct);
  
  if(renderName == "RTAO" || renderName == "AO")
  {
    return RTAO_GPU_CS_ListRequiredDeviceFeatures();
  }
  else
  {
    if(accelStruct == "RTX" || accelStruct == "HW")
      return EyeRayCaster_GPU_RQ_ListRequiredDeviceFeatures();
    else
      return EyeRayCaster_GPU_SW_ListRequiredDeviceFeatures();
  }
} 

std::shared_ptr<IRenderer> CreateRenderGPU(const char* a_renderName, const char* a_accelStruct, const char* buildFormat, const char* layout, 
                                           vk_utils::VulkanContext a_ctx, size_t a_maxThreadsGenerated)
{
  std::string renderName(a_renderName);
  std::string accelStruct(a_accelStruct);
  
  if(renderName == "RTAO" || renderName == "AO")
  {
    return CreateRTAO_GPU_CS(a_ctx, a_maxThreadsGenerated);
  }
  else
  {
    if(accelStruct == "RTX" || accelStruct == "HW")
      return CreateEyeRayCaster_GPU_RQ(a_ctx, a_maxThreadsGenerated);
    else
      return CreateEyeRayCaster_GPU_SW(a_ctx, a_maxThreadsGenerated);
  }
}                                           

