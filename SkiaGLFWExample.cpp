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
    ApplicationState() : fQuit(false), isDragging(false), window_width(0), window_height(0) {}
    skia_private::TArray<SkRect> fRects;
    bool fQuit;
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
    ApplicationState* state = (ApplicationState*)glfwGetWindowUserPointer(window);
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        state->fQuit = true;
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
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "GLFW Window", nullptr, nullptr);
    if (!window) {
        handle_error("Failed to create GLFW window");
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);  // Enable vsync

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    glViewport(0, 0, width, height);
    glClearColor(1, 1, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    auto interface = GrGLMakeNativeInterface();
    sk_sp<GrDirectContext> grContext(GrDirectContexts::MakeGL(interface));
    SkASSERT(grContext);

    GrGLFramebufferInfo info;
    info.fFBOID = 0;  // Default framebuffer
    info.fFormat = GR_GL_RGBA8;

    GrBackendRenderTarget target = GrBackendRenderTargets::MakeGL(width, height, 0, 8, info);
    SkSurfaceProps props;

    sk_sp<SkSurface> surface(SkSurfaces::WrapBackendRenderTarget(grContext.get(), target,
                                                                 kBottomLeft_GrSurfaceOrigin,
                                                                 kRGBA_8888_SkColorType, nullptr, &props));

    SkCanvas* canvas = surface->getCanvas();
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

    sk_sp<SkSurface> cpuSurface(SkSurfaces::Raster(canvas->imageInfo()));
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

    while (!glfwWindowShouldClose(window) && !state.fQuit) {
        glfwPollEvents();

        canvas->clear(SK_ColorWHITE);

        // Draw help message
        paint.setColor(SK_ColorBLACK);
        canvas->drawString(helpMessage, 100.0f, 100.0f, font, paint);

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
        canvas->translate(width / 2.0, height / 2.0);
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