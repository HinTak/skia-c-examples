#include "SkiaGLWidget.h"
#include <QOpenGLContext>
#include "include/core/SkCanvas.h"
#include "include/core/SkColorSpace.h"
#include "include/gpu/ganesh/gl/GrGLInterface.h"
#include "include/gpu/ganesh/gl/GrGLDirectContext.h" // GrDirectContexts
#include "include/gpu/ganesh/gl/GrGLBackendSurface.h" // GrBackendRenderTargets
#include "include/gpu/ganesh/SkSurfaceGanesh.h"
#include "include/gpu/ganesh/GrDirectContext.h" // GrAsDirectContext
#include "include/gpu/ganesh/GrBackendSurface.h" // GrBackendRenderTarget

SkiaGLWidget::SkiaGLWidget(QWidget* parent)
    : QOpenGLWidget(parent)
{
}

SkiaGLWidget::~SkiaGLWidget() {}

void SkiaGLWidget::initializeGL()
{
    initializeOpenGLFunctions();

    auto interface = GrGLMakeNativeInterface();
    fContext = GrDirectContexts::MakeGL(interface);

    resizeGL(width(), height());
}

void SkiaGLWidget::resizeGL(int w, int h)
{
    if (!fContext) return;

    GrGLFramebufferInfo fbInfo;
    fbInfo.fFBOID = defaultFramebufferObject();
    fbInfo.fFormat = GL_RGBA8;

    SkColorType colorType = kRGBA_8888_SkColorType;

    GrBackendRenderTarget backendRenderTarget = GrBackendRenderTargets::MakeGL(w, h,
        this->format().samples(),
        this->format().stencilBufferSize(),
        fbInfo);

    fSurface = SkSurfaces::WrapBackendRenderTarget(
        fContext.get(),
        backendRenderTarget,
        kBottomLeft_GrSurfaceOrigin,
        colorType,
        nullptr,
        nullptr
    );
}

void SkiaGLWidget::paintGL()
{
    if (!fSurface) return;

    SkCanvas* canvas = fSurface->getCanvas();

    // Example Skia drawing, similar to SDL/Qt raster example
    canvas->clear(SK_ColorWHITE);

    SkPaint paint;
    paint.setColor(SK_ColorMAGENTA);
    paint.setAntiAlias(true);
    canvas->drawCircle(150, 150, 64, paint);

    paint.setColor(SK_ColorBLACK);
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStrokeWidth(6);
    canvas->drawRect(SkRect::MakeXYWH(50, 50, 200, 200), paint);

    // Flush Skia drawing
    auto direct = GrAsDirectContext(fSurface->recordingContext());
    if (direct) {
      direct->flush(fSurface.get(), SkSurfaces::BackendSurfaceAccess::kNoAccess, GrFlushInfo());
      direct->submit();
    }
    fContext->flush();
}