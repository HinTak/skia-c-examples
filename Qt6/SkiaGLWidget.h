#pragma once

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include "include/core/SkSurface.h"
#include "include/gpu/GrDirectContext.h"

class SkiaGLWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT
public:
    explicit SkiaGLWidget(QWidget* parent = nullptr);
    ~SkiaGLWidget();

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

private:
    std::unique_ptr<GrDirectContext> fContext;
    sk_sp<SkSurface> fSurface;
};