#include <QApplication>
#include "SkiaGLWidget.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    SkiaGLWidget widget;
    widget.setWindowTitle("Skia + Qt6 OpenGL Example");
    widget.resize(640, 480);
    widget.show();
    return app.exec();
}