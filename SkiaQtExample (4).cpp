#include <QApplication>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QTimer>
#include <QSurfaceFormat>
#include <QDebug>

// Skia
#include "include/core/SkCanvas.h"
#include "include/core/SkSurface.h"
#include "include/gpu/GrDirectContext.h"
#include "include/gpu/gl/GrGLInterface.h"
#include "include/gpu/gl/GrGLTypes.h"
#include "include/gpu/ganesh/SkSurfaceGanesh.h"

class SkiaQtWidget : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT
public:
    SkiaQtWidget(QWidget* parent = nullptr)
        : QOpenGLWidget(parent),
          skContext(nullptr),
          skSurface(nullptr)
    {
        setMouseTracking(true);
        setMinimumSize(640, 480);
        // Timer for periodic redraws, if you want animation
        QTimer* timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, QOverload<>::of(&SkiaQtWidget::update));
        timer->start(16); // ~60fps
    }

    ~SkiaQtWidget() override {
        makeCurrent();
        skSurface.reset();
        skContext.reset();
        doneCurrent();
    }

protected:
    void initializeGL() override {
        initializeOpenGLFunctions();
        sk_sp<const GrGLInterface> interface(GrGLMakeNativeInterface());
        skContext = GrDirectContext::MakeGL(interface);
        createSkiaSurface();
    }

    void resizeGL(int w, int h) override {
        createSkiaSurface();
    }

    void paintGL() override {
        if (!skSurface) return;
        SkCanvas* canvas = skSurface->getCanvas();
        canvas->clear(SK_ColorWHITE);

        // Draw something (replace with your drawing code)
        SkPaint paint;
        paint.setColor(SK_ColorRED);
        canvas->drawRect(SkRect::MakeXYWH(50, 50, 200, 100), paint);

        paint.setColor(SK_ColorBLUE);
        canvas->drawCircle(lastMousePos.x(), lastMousePos.y(), 40, paint);

        skContext->flush();
        // Qt will swap buffers automatically
    }

    void mouseMoveEvent(QMouseEvent* event) override {
        lastMousePos = event->pos();
        update();
    }

    void resizeEvent(QResizeEvent* event) override {
        QOpenGLWidget::resizeEvent(event);
        // Skia surface is recreated in resizeGL
    }

private:
    void createSkiaSurface() {
        if (!skContext) return;
        skContext->flush();
        skSurface.reset();

        GrGLFramebufferInfo fbInfo;
        fbInfo.fFBOID = defaultFramebufferObject();
        fbInfo.fFormat = GL_RGBA8;

        SkColorType colorType = kRGBA_8888_SkColorType;

        GrBackendRenderTarget backendRenderTarget(
            width(), height(),
            this->format().samples(),  // sample count
            this->format().stencilBufferSize(),  // stencil bits
            fbInfo
        );

        skSurface = SkSurfaces::WrapBackendRenderTarget(
            skContext.get(),
            backendRenderTarget,
            kBottomLeft_GrSurfaceOrigin,
            colorType,
            nullptr,
            nullptr
        );
    }

    sk_sp<GrDirectContext> skContext;
    sk_sp<SkSurface> skSurface;
    QPoint lastMousePos = QPoint(100, 100);
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    QSurfaceFormat format;
    format.setRenderableType(QSurfaceFormat::OpenGL);
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setAlphaBufferSize(8);
    format.setStencilBufferSize(8);
    format.setDepthBufferSize(24);
    QSurfaceFormat::setDefaultFormat(format);

    SkiaQtWidget widget;
    widget.setWindowTitle("Skia + Qt6 + OpenGL Example");
    widget.resize(800, 600);
    widget.show();

    return app.exec();
}

#include "SkiaQtExample.moc"