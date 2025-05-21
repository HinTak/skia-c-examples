#include <QApplication>
#include <QWidget>
#include <QPainter>
#include <QImage>
#include <QTimer>
#include <QResizeEvent>
#include "include/core/SkCanvas.h"
#include "include/core/SkSurface.h"
#include "include/core/SkPaint.h"
#include "include/core/SkImageInfo.h"
#include "include/core/SkColor.h"

class SkiaWidget : public QWidget {
    Q_OBJECT
public:
    SkiaWidget(QWidget *parent = nullptr)
        : QWidget(parent), skSurface(nullptr) {
        setMinimumSize(640, 480);
        setAttribute(Qt::WA_OpaquePaintEvent);
        setAttribute(Qt::WA_NoSystemBackground);
        setAttribute(Qt::WA_PaintOnScreen, false);
        // Optional: For continuous animation, use a timer
        // QTimer* timer = new QTimer(this);
        // connect(timer, &QTimer::timeout, this, QOverload<>::of(&SkiaWidget::update));
        // timer->start(16); // ~60 FPS
    }

    ~SkiaWidget() override {
        // SkSurface is ref-counted, will release automatically
    }

protected:
    void paintEvent(QPaintEvent *) override {
        ensureSurface();

        SkCanvas* canvas = skSurface->getCanvas();
        // Clear background
        canvas->clear(SK_ColorWHITE);

        // Example Skia drawing (similar to SkiaSDLExample.cpp)
        SkPaint paint;
        paint.setColor(SK_ColorRED);
        paint.setAntiAlias(true);
        canvas->drawCircle(200, 200, 64, paint);

        paint.setColor(SK_ColorBLUE);
        paint.setStyle(SkPaint::kStroke_Style);
        paint.setStrokeWidth(6);
        canvas->drawRect(SkRect::MakeXYWH(100, 100, 200, 200), paint);

        // Flush Skia drawing
        skSurface->flushAndSubmit();

        // Get Skia's pixel buffer
        SkPixmap pixmap;
        if (!skSurface->peekPixels(&pixmap)) return;

        // Wrap Skia pixels as QImage (do NOT deep copy)
        QImage img((const uchar*)pixmap.addr(),
                   pixmap.width(), pixmap.height(),
                   pixmap.rowBytes(),
                   QImage::Format_RGBA8888_Premultiplied);

        // Draw to Qt widget
        QPainter painter(this);
        painter.drawImage(0, 0, img);
    }

    void resizeEvent(QResizeEvent *event) override {
        QSize sz = event->size();
        createSurface(sz.width(), sz.height());
        QWidget::resizeEvent(event);
    }

private:
    void ensureSurface() {
        if (!skSurface || skSurface->width() != width() || skSurface->height() != height()) {
            createSurface(width(), height());
        }
    }

    void createSurface(int w, int h) {
        if (w <= 0 || h <= 0) return;
        SkImageInfo info = SkImageInfo::Make(w, h, kRGBA_8888_SkColorType, kPremul_SkAlphaType);
        skSurface = SkSurfaces::Raster(info);
    }

    sk_sp<SkSurface> skSurface;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    SkiaWidget widget;
    widget.setWindowTitle("Skia + Qt6 Example");
    widget.resize(640, 480);
    widget.show();
    return app.exec();
}

#include "SkiaQtExample.moc"