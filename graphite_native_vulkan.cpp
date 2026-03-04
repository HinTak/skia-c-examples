#include "include/core/SkAlphaType.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkColor.h"
#include "include/core/SkColorType.h"
#include "include/core/SkImageInfo.h"
#include "include/core/SkRRect.h"
#include "include/core/SkRefCnt.h"
#include "include/core/SkSurface.h"
#include "include/gpu/graphite/Context.h"
#include "include/gpu/graphite/Recorder.h"
#include "include/gpu/graphite/ContextOptions.h"
#include "include/gpu/vk/VulkanBackendContext.h"
#include "include/gpu/graphite/Surface.h"
#include "include/gpu/vk/VulkanExtensions.h"
#include "include/gpu/graphite/vk/VulkanGraphiteContext.h"

#define WIDTH 200
#define HEIGHT 400

int main(int argc, char** argv) {
    skgpu::VulkanBackendContext backendContext;
    std::unique_ptr<skgpu::VulkanExtensions> extensions(new skgpu::VulkanExtensions());
    backendContext.fInstance = VK_NULL_HANDLE;
    backendContext.fDevice = VK_NULL_HANDLE;
    skgpu::graphite::ContextOptions options;

    std::unique_ptr<skgpu::graphite::Context> context =
        skgpu::graphite::ContextFactory::MakeVulkan(backendContext, options);
    if (!context) {
        printf("Could not make Graphite Native Vulkan context\n");
        return 1;
    }

    // ... create surfaces and draw
    return 0;
}