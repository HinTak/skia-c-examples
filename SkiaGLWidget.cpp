#include "SkiaGLWidget.h"
#include <QOpenGLContext>
#include "include/gpu/gl/GrGLInterface.h"
#include "include/gpu/ganesh/SkSurfaceGanesh.h"

SkiaGLWidget::SkiaGLWidget(QWidget* parent)
    : QOpenGLWidget(parent)
{
}

SkiaGLWidget::~SkiaGLWidget() {}

void SkiaGLWidget::initializeGL()
{
    initializeOpenGLFunctions();

    auto interface = GrGLMakeNativeInterface();
    fContext = GrDirectContext::MakeGL(interface);

    resizeGL(width(), height());
}

void SkiaGLWidget::resizeGL(int w, int h)
{
    if (!fContext) return;

    GrGLFramebufferInfo fbInfo;
    fbInfo.fFBOID = defaultFramebufferObject();
    fbInfo.fFormat = GL_RGBA8;

    SkColorType colorType = kRGBA_8888_SkColorType;

    GrBackendRenderTarget backendRenderTarget(w, h, 
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
    fSurface->flushAndSubmit();
    fContext->flush();
}