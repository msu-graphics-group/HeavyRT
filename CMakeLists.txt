cmake_minimum_required(VERSION 3.15)

option(USE_VULKAN "Enable Vulkan" OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

project(loader_app CXX)
add_compile_definitions(HALFFLOAT)
#add_compile_definitions(USE_HEAVYRT)

add_library(project_warnings INTERFACE)
include(cmake/CompilerWarnings.cmake)
set_project_warnings(project_warnings)

if(CMAKE_SYSTEM_NAME STREQUAL Windows)
    link_directories(${CMAKE_SOURCE_DIR}/external/embree/lib_win64)
    set(RAYTRACING_EMBREE_LIBS embree3)
else()
    find_package(Threads REQUIRED)
    link_directories(${CMAKE_SOURCE_DIR}/external/embree/lib)
    set(RAYTRACING_EMBREE_LIBS embree3 embree_sse42 embree_avx embree_avx2 lexers simd sys tasking) # embree_avx embree_avx2
endif()

include_directories(${CMAKE_SOURCE_DIR}/external)
include_directories(${CMAKE_SOURCE_DIR}/external/embree)
include_directories(${CMAKE_SOURCE_DIR}/external/nanort)
include_directories(${CMAKE_SOURCE_DIR}/external/tiny_gltf)
include_directories(${CMAKE_SOURCE_DIR}/core)
include_directories(${CMAKE_SOURCE_DIR}/core/common)
include_directories(${CMAKE_SOURCE_DIR}/core/builders)
include_directories(${CMAKE_SOURCE_DIR}/examples)
include_directories(${CMAKE_SOURCE_DIR}/examples/loader)

find_package(OpenMP)

set(SCENE_LOADER_SRC
        ${CMAKE_SOURCE_DIR}/examples/loader/pugixml.cpp
        ${CMAKE_SOURCE_DIR}/examples/loader/hydraxml.cpp
        ${CMAKE_SOURCE_DIR}/examples/loader/cmesh.cpp
        ${CMAKE_SOURCE_DIR}/examples/loader/gltf_loader.cpp
        ${CMAKE_SOURCE_DIR}/examples/loader/gltf_loader_def.cpp)


set(CORE_SRC
        ${CMAKE_SOURCE_DIR}/core/common/Image2d.cpp
        ${CMAKE_SOURCE_DIR}/core/common/Timer.cpp
        ${CMAKE_SOURCE_DIR}/core/common/qmc_sobol_niederreiter.cpp
        ${CMAKE_SOURCE_DIR}/core/FactoryRT.cpp
        ${CMAKE_SOURCE_DIR}/core/EmbreeRT.cpp
        ${CMAKE_SOURCE_DIR}/core/BruteForceRT.cpp
        ${CMAKE_SOURCE_DIR}/core/BVH2CommonLoftRT.cpp
        ${CMAKE_SOURCE_DIR}/core/BVH2CommonLoftRT_host.cpp
        #${CMAKE_SOURCE_DIR}/core/BVH2FatRT.cpp
        #${CMAKE_SOURCE_DIR}/core/BVH2FatRT_host.cpp
        )

set(EXAMPLES_SRC
        ${CMAKE_SOURCE_DIR}/examples/01_eye_rays/eye_ray.cpp
        ${CMAKE_SOURCE_DIR}/examples/01_eye_rays/eye_ray_host.cpp
        ${CMAKE_SOURCE_DIR}/examples/02_rtao/rtao.cpp
        ${CMAKE_SOURCE_DIR}/examples/02_rtao/rtao_host.cpp
        ${CMAKE_SOURCE_DIR}/examples/RenderFactory.cpp
        )

set(BUILDERS_SRC
        ${CMAKE_SOURCE_DIR}/core/builders/lbvh.cpp
        ${CMAKE_SOURCE_DIR}/core/builders/lbvh_host.cpp
        ${CMAKE_SOURCE_DIR}/core/builders/cbvh.cpp
        ${CMAKE_SOURCE_DIR}/core/builders/cbvh_fat.cpp
        ${CMAKE_SOURCE_DIR}/core/builders/cbvh_core.cpp
        ${CMAKE_SOURCE_DIR}/core/builders/cbvh_embree2.cpp
        ${CMAKE_SOURCE_DIR}/core/builders/cbvh_split.cpp
        ${CMAKE_SOURCE_DIR}/core/builders/EXT_TriBoxOverlap.cpp
        ${CMAKE_SOURCE_DIR}/core/builders/cbvh_esc.cpp
        ${CMAKE_SOURCE_DIR}/core/builders/cbvh_esc2.cpp
        ${CMAKE_SOURCE_DIR}/core/builders/cbvh_hlbvh.cpp
        ${CMAKE_SOURCE_DIR}/core/builders/FatBVHReorder.cpp
        ${CMAKE_SOURCE_DIR}/core/builders/FatBVHClusterize.cpp
        ${CMAKE_SOURCE_DIR}/core/builders/FatBVHPrint.cpp
        ${CMAKE_SOURCE_DIR}/core/builders/refitter.cpp)

set(VULKAN_SRC "")
set(VULKAN_EXTERNAL_SRC "")

add_compile_definitions(ENABLE_SPHERES)    # enable spheres    prims
add_compile_definitions(ENABLE_MANDELBULB) # enable mundelbulb prims

if(USE_VULKAN)
  set(EXTERNAL "${CMAKE_SOURCE_DIR}/.." CACHE PATH "external source directory")

  if(CMAKE_SYSTEM_NAME STREQUAL Windows)
    set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_WIN32_KHR)
  elseif(CMAKE_SYSTEM_NAME STREQUAL Linux)
    set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_XLIB_KHR)
  elseif(CMAKE_SYSTEM_NAME STREQUAL Darwin)
    set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_MACOS_MVK)
  endif()

  add_subdirectory(${EXTERNAL}/volk ${EXTERNAL}/volk/build)

  add_compile_definitions(USE_VOLK)
  add_compile_definitions(USE_VULKAN)
  
  include_directories(${Vulkan_INCLUDE_DIR})
  include_directories(${EXTERNAL})
  include_directories(${EXTERNAL}/vkutils)
  include_directories(${EXTERNAL}/CrossRT) # for "VulkanRTX.h"

  set(VULKAN_EXTERNAL_SRC
      ${EXTERNAL}/vkutils/vk_utils.cpp
      ${EXTERNAL}/vkutils/vk_copy.cpp
      ${EXTERNAL}/vkutils/vk_buffers.cpp
      ${EXTERNAL}/vkutils/vk_images.cpp
      ${EXTERNAL}/vkutils/vk_context.cpp
      ${EXTERNAL}/vkutils/vk_alloc_simple.cpp
      ${EXTERNAL}/vkutils/vk_pipeline.cpp
      ${EXTERNAL}/vkutils/vk_descriptor_sets.cpp
      ${EXTERNAL}/vkutils/ray_tracing/vk_rt_utils.cpp
      ${EXTERNAL}/vkutils/geom/vk_mesh.cpp
      ${EXTERNAL}/CrossRT/VulkanRTX.cpp
      ${EXTERNAL}/LiteScene/scene_mgr.cpp
      ${EXTERNAL}/vkutils/geom/cmesh.cpp)

  set(VULKAN_SRC
      ${CMAKE_SOURCE_DIR}/examples/RenderFactoryGPU.cpp

      ${CMAKE_SOURCE_DIR}/examples/01_eye_rays/eye_ray_gpu_sw.cpp
      ${CMAKE_SOURCE_DIR}/examples/01_eye_rays/eye_ray_gpu_sw_init.cpp
      ${CMAKE_SOURCE_DIR}/examples/01_eye_rays/eye_ray_gpu_sw_ds.cpp 
      
      ${CMAKE_SOURCE_DIR}/examples/01_eye_rays/eye_ray_gpu_rq.cpp
      ${CMAKE_SOURCE_DIR}/examples/01_eye_rays/eye_ray_gpu_rq_init.cpp
      ${CMAKE_SOURCE_DIR}/examples/01_eye_rays/eye_ray_gpu_rq_ds.cpp 
      
      ${CMAKE_SOURCE_DIR}/examples/02_rtao/rtao_gpu_cs.cpp
      ${CMAKE_SOURCE_DIR}/examples/02_rtao/rtao_gpu_cs_init.cpp
      ${CMAKE_SOURCE_DIR}/examples/02_rtao/rtao_gpu_cs_ds.cpp )
endif()

message(STATUS "CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  message(STATUS "Debug is ON!")
  add_definitions(-D_DEBUG)
  #uncomment this to detect broken memory problems via gcc sanitizers
  #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fsanitize-address-use-after-scope -fno-omit-frame-pointer -fsanitize=leak -fsanitize=undefined -fsanitize=bounds-strict")
else()
  add_definitions(-DNDEBUG)
endif()

add_executable(render_app)

target_sources(render_app PRIVATE main.cpp
           ${SCENE_LOADER_SRC}
           ${CORE_SRC}
           ${EXAMPLES_SRC} ${BUILDERS_SRC}
           ${VULKAN_EXTERNAL_SRC} ${VULKAN_SRC})

set_property(TARGET render_app PROPERTY CXX_STANDARD 17)

if(CMAKE_SYSTEM_NAME STREQUAL Windows)
    set_target_properties(render_app PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

    target_link_libraries(render_app PRIVATE project_warnings ${RAYTRACING_EMBREE_LIBS})

    add_custom_command(TARGET render_app POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/external/embree/bin_win64" $<TARGET_FILE_DIR:render_app>)
else()
    target_link_libraries(render_app PRIVATE project_warnings Threads::Threads dl ${RAYTRACING_EMBREE_LIBS})
endif()

if(OpenMP_CXX_FOUND)
  target_link_libraries(render_app PUBLIC OpenMP::OpenMP_CXX) ##
endif()

if(USE_VULKAN)
  target_link_libraries(render_app PUBLIC volk)
endif()
