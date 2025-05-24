/*
 * Ported from SkiaSDLExample.cpp to Qt by Copilot, 2025.
 * This example demonstrates how to use Skia with the Qt framework
 * to render to a QWidget using OpenGL.
 * 
 * Features:
 *   - Uses QOpenGLWidget as the rendering surface.
 *   - Handles mouse events to draw rectangles.
 *   - Renders a rotating star, text, and rectangles with Skia.
 *   - Uses Skia's Ganesh OpenGL backend.
 * 
 * Requirements:
 *   - Skia built with Ganesh (OpenGL) support.
 *   - Qt >= 5.4 (for QOpenGLWidget)
 *   - Your compiler and Skia must be set up for C++17 or newer.
 * 
 * Note: Error handling and advanced integration (e.g. DPI, resizing, font fallback)
 *       are simplified for clarity.
 */

#include <QApplication>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QMouseEvent>
#include <QTimer>
#include <vector>

// Skia includes
#include "include/gpu/ganesh/gl/GrGLInterface.h"
#include "include/gpu/ganesh/GrDirectContext.h"
#include "include/gpu/ganesh/gl/GrGLDirectContext.h"
#include "include/gpu/ganesh/gl/GrGLBackendSurface.h"
#include "include/core/SkSurface.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkPaint.h"
#include "include/core/SkColorSpace.h"
#include "include/core/SkFont.h"
#include "include/core/SkPath.h"
#include "src/base/SkRandom.h"
#include "include/core/SkImage.h"
#include "include/core/SkFontMgr.h"
#include "include/ports/SkFontConfigInterface.h"
#include "include/ports/SkFontMgr_FontConfigInterface.h"
#include <include/gpu/ganesh/GrBackendSurface.h> // GrBackendRenderTarget
#include <include/gpu/ganesh/SkSurfaceGanesh.h> // SkSurfaces::WrapBackendRenderTarget

// Helper: create a star shape
static SkPath create_star() {
    static const int kNumPoints = 5;
    SkPath concavePath;
    SkPoint points[kNumPoints] = {{0, SkIntToScalar(-50)} };
    SkMatrix rot;
    rot.setRotate(SkIntToScalar(360) / kNumPoints);
    for (int i = 1; i < kNumPoints; ++i) {
        rot.mapPoints(points + i, points + i - 1, 1);
    }
    concavePath.moveTo(points[0]);
    for (int i = 0; i < kNumPoints; ++i) {
        concavePath.lineTo(points[(2 * i) % kNumPoints]);
    }
    concavePath.setFillType(SkPathFillType::kEvenOdd);
    concavePath.close();
    return concavePath;
}

// Our Skia-on-Qt widget
class SkiaQtExampleWidget : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT
public:
    SkiaQtExampleWidget(QWidget* parent = nullptr)
        : QOpenGLWidget(parent), grContext(nullptr), surface(nullptr), rotation(0) {
        setMouseTracking(true);
        setFocusPolicy(Qt::StrongFocus);

        // Animation timer
        QTimer* timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, [this]() {
            rotation = (rotation + 1) % 360;
            update();
        });
        timer->start(16); // ~60 fps
    }

    ~SkiaQtExampleWidget() override {
        surface.reset();
        grContext.reset();
    }

protected:
    void initializeGL() override {
        initializeOpenGLFunctions();

        auto interface = GrGLMakeNativeInterface();
        grContext = GrDirectContexts::MakeGL(interface);

        // We create the Skia surface in resizeGL.
    }

    void resizeGL(int w, int h) override {
        if (!grContext) return;

        GrGLFramebufferInfo fbInfo;
        fbInfo.fFBOID = defaultFramebufferObject();
        fbInfo.fFormat = GL_RGBA8;

        SkColorType colorType = kRGBA_8888_SkColorType;

        GrBackendRenderTarget backendRenderTarget = GrBackendRenderTargets::MakeGL(
            w, h,           // width, height
            0,              // sample count
            8,              // stencil bits
            fbInfo
        );

        SkSurfaceProps props;
        surface = SkSurfaces::WrapBackendRenderTarget(
            grContext.get(), backendRenderTarget,
            kBottomLeft_GrSurfaceOrigin,
            colorType, nullptr, &props
        );
    }

    void paintGL() override {
        if (!surface) return;
        SkCanvas* canvas = surface->getCanvas();
        canvas->clear(SK_ColorWHITE);

        // Draw instructions
        SkPaint paint;
        paint.setColor(SK_ColorBLACK);

        // create a surface for CPU rasterization
        sk_sp<SkSurface> cpuSurface(SkSurfaces::Raster(canvas->imageInfo()));

        SkCanvas* offscreen = cpuSurface->getCanvas();
        offscreen->save();
        offscreen->translate(50.0f, 50.0f);
        offscreen->drawPath(create_star(), paint);
        offscreen->restore();

        sk_sp<SkImage> image = cpuSurface->makeImageSnapshot();

        sk_sp<SkFontConfigInterface> fc(SkFontConfigInterface::RefGlobal());
        sk_sp<SkTypeface> typeface(SkFontMgr_New_FCI(std::move(fc))->legacyMakeTypeface("",SkFontStyle()));
        SkFont font(typeface);

        canvas->drawString("Click and drag to create rects. Press esc to quit.", 100.0f, 100.0f, font, paint);

        // Draw user rectangles
        SkRandom rand;
        for (const SkRect& rect : rects) {
            paint.setColor(rand.nextU() | 0x44808080);
            canvas->drawRect(rect, paint);
        }

        // Draw the rotating star
        canvas->save();
        canvas->translate(width() / 2.0f, height() / 2.0f);
        canvas->rotate(rotation);
        paint.setColor(SK_ColorBLACK);
#ifndef QT6
        canvas->drawImage(image, -50.0f, -50.0f);
#else
        canvas->drawPath(create_star(), paint);
#endif
        canvas->restore();

        // Flush Skia drawing
        if (auto dContext = GrAsDirectContext(canvas->recordingContext())) {
            dContext->flushAndSubmit();
        }
    }

    void mousePressEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton) {
            SkRect rect = SkRect::MakeLTRB(event->x(), event->y(), event->x(), event->y());
            rects.push_back(rect);
        }
    }

    void mouseMoveEvent(QMouseEvent* event) override {
        if (!rects.empty() && (event->buttons() & Qt::LeftButton)) {
            SkRect& rect = rects.back();
            rect.fRight = event->x();
            rect.fBottom = event->y();
            update();
        }
    }

    void keyPressEvent(QKeyEvent* event) override {
        if (event->key() == Qt::Key_Escape) {
            qApp->quit();
        }
    }

    void leaveEvent(QEvent*) override {
        // Optionally, do something on mouse leave.
    }

private:
    sk_sp<GrDirectContext> grContext;
    sk_sp<SkSurface> surface;
    std::vector<SkRect> rects;
    int rotation;
};

#include <QVBoxLayout>
#include <QWidget>

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QWidget window;
    window.setWindowTitle("Skia + Qt Example");
    window.resize(1024, 768);

    QVBoxLayout* layout = new QVBoxLayout(&window);
    SkiaQtExampleWidget* skiaWidget = new SkiaQtExampleWidget();
    layout->addWidget(skiaWidget);
    window.setLayout(layout);

    window.show();
    return app.exec();
}

#include "SkiaQTExample.moc"