#include <QApplication>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QSurfaceFormat>
#include "include/core/SkSurface.h"
#include "include/gpu/GrDirectContext.h"
#include "include/gpu/gl/GrGLInterface.h"

class SkiaQtWidget : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT

public:
    SkiaQtWidget(QWidget *parent = nullptr)
        : QOpenGLWidget(parent), fContext(nullptr), fSurface(nullptr) {
        // Set the desired format (optional)
        QSurfaceFormat fmt;
        fmt.setRenderableType(QSurfaceFormat::OpenGL);
        fmt.setVersion(3, 3);
        fmt.setProfile(QSurfaceFormat::CoreProfile);
        setFormat(fmt);
    }

    ~SkiaQtWidget() override {
        makeCurrent();
        fSurface.reset();
        fContext.reset();
        doneCurrent();
    }

protected:
    void initializeGL() override {
        initializeOpenGLFunctions();

        auto interface = GrGLMakeNativeInterface();
        fContext = GrDirectContext::MakeGL(interface);

        resizeSkiaSurface(width(), height());
    }

    void resizeGL(int w, int h) override {
        resizeSkiaSurface(w, h);
    }

    void paintGL() override {
        if (!fSurface) return;

        SkCanvas* canvas = fSurface->getCanvas();
        canvas->clear(SK_ColorWHITE);

        // Example Skia drawing:
        SkPaint paint;
        paint.setColor(SK_ColorRED);
        paint.setAntiAlias(true);
        canvas->drawCircle(150, 100, 90, paint);

        // Flush Skia draw commands
        fSurface->flushAndSubmit();
        fContext->flush();
    }

private:
    void resizeSkiaSurface(int w, int h) {
        if (!fContext) return;

        GrGLFramebufferInfo fbInfo;
        fbInfo.fFBOID = defaultFramebufferObject();
        fbInfo.fFormat = GL_RGBA8;

        GrBackendRenderTarget backendRenderTarget(w, h, 
            /*sampleCnt*/ 0, /*stencilBits*/ 8, fbInfo);

        SkColorType colorType = kRGBA_8888_SkColorType;
        SkSurfaceProps props;

        fSurface = SkSurface::MakeFromBackendRenderTarget(
            fContext.get(),
            backendRenderTarget,
            kBottomLeft_GrSurfaceOrigin,
            colorType,
            nullptr,
            &props
        );
    }

    sk_sp<GrDirectContext> fContext;
    sk_sp<SkSurface> fSurface;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    SkiaQtWidget widget;
    widget.resize(400, 300);
    widget.show();
    return app.exec();
}

#include "SkiaQtExample.moc"