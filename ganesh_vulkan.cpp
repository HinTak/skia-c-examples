#include "include/core/SkAlphaType.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkColor.h"
#include "include/core/SkColorType.h"
#include "include/core/SkImageInfo.h"
#include "include/core/SkRRect.h"
#include "include/core/SkRefCnt.h"
#include "include/core/SkSurface.h"
#include "include/gpu/ganesh/GrDirectContext.h"
#include "include/gpu/ganesh/SkSurfaceGanesh.h"
#include "include/gpu/ganesh/vk/GrVkDirectContext.h"
#include "include/gpu/vk/VulkanBackendContext.h"
#include "include/gpu/vk/VulkanExtensions.h"

int main(int argc, char** argv) {
    skgpu::VulkanBackendContext backendContext;
    std::unique_ptr<skgpu::VulkanExtensions> extensions(new skgpu::VulkanExtensions());
    backendContext.fInstance = VK_NULL_HANDLE;
    backendContext.fDevice = VK_NULL_HANDLE;
    backendContext.fMemoryAllocator = nullptr;

    sk_sp<GrDirectContext> ctx = GrDirectContexts::MakeVulkan(backendContext);
    if (!ctx) {
        return 1;
    }

    SkImageInfo imageInfo =
            SkImageInfo::Make(200, 400, kRGBA_8888_SkColorType, kPremul_SkAlphaType);

    sk_sp<SkSurface> surface =
            SkSurfaces::RenderTarget(ctx.get(), skgpu::Budgeted::kYes, imageInfo);
    
    SkCanvas* canvas = surface->getCanvas();
    canvas->clear(SK_ColorYELLOW);
    SkRRect rrect = SkRRect::MakeRectXY(SkRect::MakeLTRB(10, 20, 50, 70), 10, 10);

    SkPaint paint;
    paint.setColor(SK_ColorMAGENTA);
    paint.setAntiAlias(true);

    canvas->drawRRect(rrect, paint);
    ctx->flush();

    return 0;
}