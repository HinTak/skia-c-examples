```cpp
// Sets up a Vulkan backend context with proper instance and device creation
int main(int argc, char** argv) {
    skgpu::VulkanBackendContext backendContext;
    VkDebugUtilsMessengerEXT debugMessenger;
    std::unique_ptr<skgpu::VulkanExtensions> extensions(new skgpu::VulkanExtensions());
    std::unique_ptr<sk_gpu_test::TestVkFeatures> features(new sk_gpu_test::TestVkFeatures);

    // Create VkInstance and VkDevice
    PFN_vkGetInstanceProcAddr instProc;
    if (!sk_gpu_test::LoadVkLibraryAndGetProcAddrFuncs(&instProc)) {
        return 1;
    }

    backendContext.fInstance = VK_NULL_HANDLE;
    backendContext.fDevice = VK_NULL_HANDLE;

    if (!sk_gpu_test::CreateVkBackendContext(
                instProc, &backendContext, extensions.get(), features.get(), &debugMessenger)) {
        return 1;
    }

    // Create Skia context using Vulkan backend
    auto getProc = backendContext.fGetProc;
    PFN_vkDestroyInstance fVkDestroyInstance;
    PFN_vkDestroyDebugUtilsMessengerEXT fVkDestroyDebugUtilsMessengerEXT = nullptr;
    PFN_vkDestroyDevice fVkDestroyDevice;
    
    // Create image info and surface
    SkImageInfo imageInfo = SkImageInfo::Make(16, 16, kRGBA_8888_SkColorType, kPremul_SkAlphaType);
    sk_sp<SkSurface> surface =
            SkSurfaces::RenderTarget(context.get(), skgpu::Budgeted::kYes, imageInfo);
    
    // Draw on the surface
    surface->getCanvas()->clear(SK_ColorRED);

    // Flush and submit work to GPU
    context->flush(surface.get());
    context->submit();

    // Cleanup
    fVkDestroyDevice(backendContext.fDevice, nullptr);
    if (debugMessenger != VK_NULL_HANDLE) {
        fVkDestroyDebugUtilsMessengerEXT(backendContext.fInstance, debugMessenger, nullptr);
    }
    fVkDestroyInstance(backendContext.fInstance, nullptr);
    return 0;
}