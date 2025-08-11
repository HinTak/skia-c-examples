#include <gtk/gtk.h>
#include <epoxy/gl.h>

#define SK_GANESH
#define SK_GL
#include "include/gpu/ganesh/GrBackendSurface.h"
#include "include/gpu/ganesh/GrDirectContext.h"
#include "include/gpu/ganesh/gl/GrGLInterface.h"
#include "include/gpu/ganesh/gl/egl/GrGLMakeEGLInterface.h"
#include "include/gpu/ganesh/SkSurfaceGanesh.h"
#include "include/gpu/ganesh/gl/GrGLBackendSurface.h"
#include "include/gpu/ganesh/gl/GrGLDirectContext.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkColorSpace.h"
#include "include/core/SkSurface.h"
#include "src/gpu/ganesh/gl/GrGLDefines.h"
#include "src/gpu/ganesh/gl/GrGLUtil.h"

#include <cstdio>
#include <iostream>
#include <chrono>
#include <thread>

#include "logging.h"

#define LOG_TAG "Log"

constexpr auto windowWidth = 800;
constexpr auto windowHeight = 600;

GrDirectContext *sContext = nullptr;
sk_sp<SkSurface> sSurface;
GtkWidget *gl_area = nullptr;

void init_skia(int width, int height) {
    const auto interface = GrGLInterfaces::MakeEGL();
    if (!interface || !interface->validate()) {
        LOG_ERROR(LOG_TAG, "Failed to create GrGLInterface");
        std::abort();
    }

    sContext = GrDirectContexts::MakeGL(interface).release();
    if (!sContext) {
        LOG_ERROR(LOG_TAG, "Failed to create GrDirectContext");
        std::abort();
    }

    GrGLint buffer;
    GR_GL_CALL(interface, GetIntegerv(GR_GL_FRAMEBUFFER_BINDING, &buffer));

    GrGLFramebufferInfo framebufferInfo;
    framebufferInfo.fFBOID = buffer;
    framebufferInfo.fFormat = GL_RGBA8;

    constexpr auto colorType = kRGBA_8888_SkColorType;
    const auto backendRenderTarget = GrBackendRenderTargets::MakeGL(width, height, 0, 8, framebufferInfo);
    if (!backendRenderTarget.isValid()) {
        LOG_ERROR(LOG_TAG, "Failed to create GrBackendRenderTarget");
        std::abort();
    }

    sSurface = SkSurfaces::WrapBackendRenderTarget(sContext, backendRenderTarget, kBottomLeft_GrSurfaceOrigin,
                                                   colorType, nullptr, nullptr);
    if (sSurface == nullptr) {
        LOG_ERROR(LOG_TAG, "Failed to create Skia surface");
        if (sContext->abandoned()) {
            LOG_ERROR(LOG_TAG, "GrDirectContext is abandoned");
        }
        std::abort();
    }

    LOG_INFO(LOG_TAG, "Skia initialized");
}

void cleanup_skia() {
    sSurface.reset();
    delete sContext;
}

static void realize(GtkGLArea *area) {
    gtk_gl_area_make_current(area);
    if (gtk_gl_area_get_error(area) != nullptr)
        return;

    const GLubyte *renderer = glGetString(GL_RENDERER);
    const GLubyte *version = glGetString(GL_VERSION);
    std::cout << "Renderer: " << renderer << std::endl;
    std::cout << "OpenGL version supported: " << version << std::endl;

    init_skia(windowWidth, windowHeight);
}

static void unrealize(GtkGLArea *area) {
    gtk_gl_area_make_current(area);
    if (gtk_gl_area_get_error(area) != nullptr)
        return;

    cleanup_skia();
}

static gboolean render(GtkGLArea *area, GdkGLContext *context) {
    const auto frameStart = std::chrono::high_resolution_clock::now();

    glClearColor(1.0, 1.0, 1.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    auto canvas = sSurface->getCanvas();
    canvas->clear(SK_ColorWHITE);

    SkPaint paint;
    paint.setColor(SK_ColorRED);
    paint.setAntiAlias(true);
    canvas->drawCircle(windowWidth / 2, windowHeight / 2, 100, paint);

    sContext->flush();

    const auto frameEnd = std::chrono::high_resolution_clock::now();
    const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(frameEnd - frameStart);

    std::this_thread::sleep_for(std::chrono::milliseconds(std::min(1LL, (16LL - duration.count())))); // ~60 FPS

    return TRUE;
}

static void on_minus_clicked(GtkButton *button, gpointer user_data) {
    LOG_DEBUG(LOG_TAG, "Minus button clicked");
}

static void on_plus_clicked(GtkButton *button, gpointer user_data) {
    LOG_DEBUG(LOG_TAG, "Plus button clicked");
}

static void activate(GtkApplication *app, gpointer user_data) {
    auto window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Sample");
    gtk_window_set_default_size(GTK_WINDOW(window), windowWidth, windowHeight);

    auto cursor = gdk_cursor_new_from_name("default", nullptr);
    gtk_widget_set_cursor(window, cursor);
    g_object_unref(cursor);

    auto box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_window_set_child(GTK_WINDOW(window), box);

    gl_area = gtk_gl_area_new();
    gtk_gl_area_set_use_es(GTK_GL_AREA(gl_area), TRUE);
    gtk_widget_set_hexpand(gl_area, TRUE);
    gtk_widget_set_vexpand(gl_area, TRUE);

    g_signal_connect(gl_area, "realize", G_CALLBACK(realize), nullptr);
    g_signal_connect(gl_area, "unrealize", G_CALLBACK(unrealize), nullptr);
    g_signal_connect(gl_area, "render", G_CALLBACK(render), nullptr);

    gtk_box_append(GTK_BOX(box), gl_area);

    auto button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_widget_set_halign(button_box, GTK_ALIGN_CENTER);

    auto minus_button = gtk_button_new_with_label("-");
    auto plus_button = gtk_button_new_with_label("+");

    g_signal_connect(minus_button, "clicked", G_CALLBACK(on_minus_clicked), nullptr);
    g_signal_connect(plus_button, "clicked", G_CALLBACK(on_plus_clicked), nullptr);

    gtk_box_append(GTK_BOX(button_box), minus_button);
    gtk_box_append(GTK_BOX(button_box), plus_button);

    gtk_box_append(GTK_BOX(box), button_box);

    gtk_widget_show(window);
}

int main(int argc, char *argv[]) {
    std::cout << "Starting application..." << std::endl;
    gdk_set_allowed_backends("wayland");
    auto app = gtk_application_new("org.gtk.example", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), nullptr);
    std::cout << "Running application..." << std::endl;
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}
