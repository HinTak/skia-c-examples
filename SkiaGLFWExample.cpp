/*
 * Copyright 2015 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 */
/*
 * Ported to GLFW by Copilot, 2025, from SkiaSDLExample.cpp, Hin-Tak Leung
 *
 * HTL (compared to a "faithful" manual port):
 *   - The first version simply deleted all the mouse/key/resizing
 *     interaction code. Quite alarming.
 *   - It simply was not able to do initial full-desktop window size,
 *     and decided arbitrarily to use 800,600.
 *   - Arbitrarily GLFW_CONTEXT_VERSION_MINOR to 3.
 *   - 2nd prompt, it added back mouse_button_callback/cursor_position_callback
 *     but decided to add more, the isDragging/currentRect constructs (below).
 *     Stilll missing key_callback / framebuffer_size_callback.
 *   - Copilot insisted on revert to (outdated/wrong) Skia m87 API from current,
 *     on GrBackendRenderTarget initializaton.
 *   - Copilot added the isDragging/currentRect constructs all by itself!
 *     (and it comes with bugs...)
 *   - Some of the missing code is filled-in by referencing the Copilot python
 *     port, which did a much better job.
 *
 * Compile with (skia m137):
     c++ -DGR_GL_CHECK_ERROR=0 -DGR_GL_LOG_CALLS=0 -Wall \
     -I./skia \
     -DSK_FONTMGR_FONTCONFIG_AVAILABLE \
     SkiaGLFWExample.cpp \
     -L./skia/out/Release/ \
     -lskparagraph -lsvg -lskshaper -lskunicode_icu -lskunicode_core -lskia \
     -lfreetype -lwebp -ljpeg -lwebpdemux -lpng -lz  -lglfw -lGL -lfontconfig \
     -o SkiaGLFWExample
 */
#include "include/gpu/ganesh/GrBackendSurface.h"
#include "include/gpu/ganesh/GrDirectContext.h"
#include <GLFW/glfw3.h>
#include "include/core/SkCanvas.h"
#include "include/core/SkFont.h"
#include "include/core/SkSurface.h"
#include "src/base/SkRandom.h"
#include "include/private/base/SkTArray.h"

#include "include/gpu/ganesh/gl/GrGLInterface.h"
#include "src/gpu/ganesh/gl/GrGLUtil.h"
#include "include/gpu/ganesh/SkSurfaceGanesh.h"
#include "include/gpu/ganesh/gl/GrGLDirectContext.h"
#include "include/gpu/ganesh/gl/GrGLBackendSurface.h"
#include "include/core/SkColorSpace.h"

#include "include/core/SkFontMgr.h"
#ifdef __linux__
#include "include/ports/SkFontConfigInterface.h"
#include "include/ports/SkFontMgr_FontConfigInterface.h"
#endif
#ifdef __APPLE__
#include "include/ports/SkFontMgr_mac_ct.h"
#endif

struct ApplicationState {
    ApplicationState() : isDragging(false), window_width(0), window_height(0) {}
    skia_private::TArray<SkRect> fRects;
    bool isDragging;
    int window_width;
    int window_height;
    SkRect currentRect;
};

static void handle_error(const char* error_message) {
    fprintf(stderr, "Error: %s\n", error_message);
}

static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    ApplicationState* state = (ApplicationState*)glfwGetWindowUserPointer(window);
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);
            state->currentRect = SkRect::MakeLTRB(SkIntToScalar(xpos), SkIntToScalar(ypos), SkIntToScalar(xpos), SkIntToScalar(ypos));
            state->isDragging = true;
        } else if (action == GLFW_RELEASE) {
            state->fRects.push_back() = state->currentRect;
            state->isDragging = false;
        }
    }
}

static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    ApplicationState* state = (ApplicationState*)glfwGetWindowUserPointer(window);
    if (state->isDragging) {
        state->currentRect.fRight = SkIntToScalar(xpos);
        state->currentRect.fBottom = SkIntToScalar(ypos);
    }
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

static void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    ApplicationState* state = (ApplicationState*)glfwGetWindowUserPointer(window);
    state->window_width = width;
    state->window_height = height;
    glViewport(0, 0, width, height);
}

static SkPath create_star() {
    static const int kNumPoints = 5;
    SkPath concavePath;
    SkPoint points[kNumPoints] = {{0, SkIntToScalar(-50)}};
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
    SkASSERT(!concavePath.isConvex());
    concavePath.close();
    return concavePath;
}

int main(int argc, char** argv) {
    if (!glfwInit()) {
        handle_error("Failed to initialize GLFW");
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Get monitor size for initial window size
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    GLFWwindow* window = glfwCreateWindow(mode->width, mode->height, "GLFW Window", nullptr, nullptr);
    if (!window) {
        handle_error("Failed to create GLFW window");
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);  // Enable vsync

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    auto interface = GrGLMakeNativeInterface();
    sk_sp<GrDirectContext> grContext(GrDirectContexts::MakeGL(interface));
    SkASSERT(grContext);

    GrGLFramebufferInfo info;
    info.fFBOID = 0;  // Default framebuffer
    info.fFormat = GR_GL_RGBA8;

    SkSurfaceProps props;

    ApplicationState state;
    state.window_width = width;
    state.window_height = height;

    glfwSetWindowUserPointer(window, &state);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    const char* helpMessage = "Click and drag to create rects. Press ESC to quit.";
    SkPaint paint;

    sk_sp<SkSurface> cpuSurface(SkSurfaces::Raster(SkImageInfo::Make(width, height, kRGBA_8888_SkColorType, kOpaque_SkAlphaType)));
    SkCanvas* offscreen = cpuSurface->getCanvas();
    offscreen->save();
    offscreen->translate(50.0f, 50.0f);
    offscreen->drawPath(create_star(), paint);
    offscreen->restore();
    sk_sp<SkImage> image = cpuSurface->makeImageSnapshot();

    int rotation = 0;
#ifdef __linux__
    sk_sp<SkFontConfigInterface> fc(SkFontConfigInterface::RefGlobal());
    sk_sp<SkTypeface> typeface(SkFontMgr_New_FCI(std::move(fc))->legacyMakeTypeface("", SkFontStyle()));
#endif
#ifdef __APPLE__
    sk_sp<SkTypeface> typeface(SkFontMgr_New_CoreText(nullptr)->legacyMakeTypeface("", SkFontStyle()));
#endif
    SkFont font(typeface);

    int last_fb_width, last_fb_height = 0;
    sk_sp<SkSurface> cached_surface = nullptr;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        int fb_width, fb_height;
        glfwGetFramebufferSize(window, &fb_width, &fb_height);
        glViewport(0, 0, fb_width, fb_height);
        glClearColor(1, 1, 1, 1);
        glClear(GL_COLOR_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);

        if ((fb_width != last_fb_width) || (fb_height != last_fb_height)) {
            GrBackendRenderTarget target = GrBackendRenderTargets::MakeGL(fb_width, fb_height, 0, 8, info);
            sk_sp<SkSurface> surface(SkSurfaces::WrapBackendRenderTarget(grContext.get(), target,
                                                                         kBottomLeft_GrSurfaceOrigin,
                                                                         kRGBA_8888_SkColorType, nullptr, &props));
            cached_surface = surface;
            last_fb_width = fb_width;
            last_fb_height = fb_height;
        }
        SkCanvas* canvas = cached_surface->getCanvas();

        canvas->clear(SK_ColorWHITE);

        // Draw help message
        paint.setColor(SK_ColorBLACK);
        canvas->drawString(helpMessage, 0.0f, font.getSize(), font, paint);

        // Draw rectangles
        SkRandom rand;
        for (int i = 0; i < state.fRects.size(); i++) {
            paint.setColor(rand.nextU() | 0x44808080);
            canvas->drawRect(state.fRects[i], paint);
        }
        if (state.isDragging) {
            paint.setColor(rand.nextU() | 0x44808080);  // Semi-transparent color for the current rectangle
            canvas->drawRect(state.currentRect, paint);
        }

        // Draw rotating star
        canvas->save();
        canvas->translate(state.window_width / 2.0, state.window_height / 2.0);
        canvas->rotate(rotation++);
        canvas->drawImage(image, -50.0f, -50.0f);
        canvas->restore();

        if (auto dContext = GrAsDirectContext(canvas->recordingContext())) {
            dContext->flushAndSubmit();
        }
        glfwSwapBuffers(window);
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}