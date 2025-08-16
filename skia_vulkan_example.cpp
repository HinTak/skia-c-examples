// Minimal Skia + Vulkan example: rotating rectangle

#include <GLFW/glfw3.h>
#include "include/core/SkCanvas.h"
#include "include/core/SkSurface.h"
#include "include/core/SkPaint.h"
#include "include/gpu/vk/GrVkBackendContext.h"
#include "include/gpu/GrDirectContext.h"
#include "include/gpu/vk/GrVkExtensions.h"
#include "include/gpu/vk/GrVkTypes.h"
#include <vulkan/vulkan.h>
#include <cmath>

// Helper: Error check macro
#define VK_CHECK(result) if ((result) != VK_SUCCESS) { printf("Vulkan error: %d\n", result); exit(1); }

int main() {
    // 1. Initialize GLFW (for window and Vulkan surface)
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(640, 480, "Skia + Vulkan Example", nullptr, nullptr);

    // 2. Vulkan instance and surface
    VkInstance instance;
    VkApplicationInfo appInfo = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
    appInfo.pApplicationName = "SkiaVulkan";
    appInfo.apiVersion = VK_API_VERSION_1_0;
    VkInstanceCreateInfo createInfo = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    createInfo.pApplicationInfo = &appInfo;
    VK_CHECK(vkCreateInstance(&createInfo, nullptr, &instance));

    VkSurfaceKHR surface;
    VK_CHECK(glfwCreateWindowSurface(instance, window, nullptr, &surface));

    // 3. (Omitted) Create device, queue, swapchain, etc. (see Skia docs)

    // 4. Skia Vulkan backend context (fill in with your VkDevice, VkQueue, etc.)
    GrVkBackendContext backendContext = {};
    // backendContext.fInstance = instance; // fill in other fields...

    // 5. Create Skia GPU context
    sk_sp<GrDirectContext> grContext = GrDirectContext::MakeVulkan(backendContext);

    // 6. Create Skia surface (each frame, from swapchain image)
    SkImageInfo info = SkImageInfo::MakeN32Premul(640, 480);
    SkSurfaceProps props;
    sk_sp<SkSurface> surfaceSkia = SkSurface::MakeRenderTarget(grContext.get(), SkBudgeted::kNo, info, 0, &props);

    float angle = 0.0f;
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Draw with Skia
        SkCanvas* canvas = surfaceSkia->getCanvas();
        canvas->clear(SK_ColorWHITE);

        // Rotating rectangle
        SkPaint paint;
        paint.setColor(SK_ColorBLUE);
        canvas->save();
        canvas->translate(320, 240);
        canvas->rotate(angle);
        canvas->drawRect(SkRect::MakeLTRB(-100, -50, 100, 50), paint);
        canvas->restore();

        // Flush Skia and present
        surfaceSkia->flushAndSubmit();
        // Present swapchain image (omitted: real Vulkan present code)

        angle += 1.0f;
    }

    // Cleanup (destroy Skia objects, Vulkan, GLFW)
    glfwDestroyWindow(window);
    glfwTerminate();
    // ...destroy Vulkan resources
    return 0;
}