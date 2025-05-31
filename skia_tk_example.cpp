/*
  c++ -std=c++17 -O2 \
  -I./skia \
  -I/usr/include/tcl8.6 -I/usr/include/tk8.6 \
  skia_tk_example.cpp \
  -L./skia/out/Release \
  -lskia -ltcl8.6 -ltk8.6 -lfreetype -lwebp -lwebpdemux -lpng -ljpeg -lX11 -lpthread -ldl -lz -lm \
  -o skia_tk_example
*/
// Build with: g++ -std=c++17 -O2 -I/path/to/skia/include -I/usr/include/tcl8.6 -I/usr/include/tk8.6 skia_tk_example.cpp -L/path/to/skia/out/static -lskia -ltcl8.6 -ltk8.6 -lpthread -ldl -lz -lm -o skia_tk_example
// Make sure Skia and Tk libraries are available in your build environment.

#include <tk.h>
#include <tcl.h>
#include <vector>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <memory>
#include <random>
#include <string>
#include <sstream>
#include <iostream>
#include <X11/Xutil.h> // XEvent + XK_Escape

#include "include/core/SkSurface.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkPath.h"
#include "include/core/SkFont.h"
#include "include/core/SkPaint.h"
#include "include/core/SkImage.h"
#include "include/core/SkData.h"
#include "src/base/SkRandom.h"

// Window and drawing state
struct Rect {
    float l, t, r, b;
};

struct ApplicationState {
    bool fQuit = false;
    int window_width = 800;
    int window_height = 600;
    std::vector<Rect> fRects;
    bool dragging = false;
    Rect drag_rect;
    float rotation = 0;
    Tk_PhotoHandle tk_photo = nullptr;
    Tk_PhotoImageBlock tk_block;
    sk_sp<SkSurface> surface;
    SkFont font;
    ApplicationState() : font(SkFont(nullptr, 18.0f)) {}
};

ApplicationState gState;
Tk_Window gWin;
Tk_Window gMainWin;
Tcl_Interp *gInterp;
Tk_Canvas gCanvas;
Tk_PhotoHandle gPhotoHandle;
Tk_PhotoImageBlock gLastBlock;
Tcl_TimerToken gAnimToken = nullptr;

const char *HELP_MESSAGE = "Click and drag to create rects.  Press esc to quit.";

// Forward declarations
void draw();
void schedule_animation(int ms);

SkPath create_star() {
    const int kNumPoints = 5;
    SkPath path;
    float angle = 2 * M_PI / kNumPoints;
    std::vector<SkPoint> points;
    for (int i = 0; i < kNumPoints; ++i) {
        float theta = i * angle - M_PI_2;
        float x = cos(theta) * 50;
        float y = sin(theta) * 50;
        points.push_back(SkPoint::Make(x, y));
    }
    path.moveTo(points[0]);
    for (int i = 0; i < kNumPoints; ++i) {
        int idx = (2 * i) % kNumPoints;
        path.lineTo(points[idx]);
    }
    path.setFillType(SkPathFillType::kEvenOdd);
    path.close();
    return path;
}

// Tk event handlers
void OnMouseDown(ClientData, XEvent *eventPtr) {
    if (eventPtr->type == ButtonPress) {
        int x = eventPtr->xbutton.x;
        int y = eventPtr->xbutton.y;
        gState.dragging = true;
        gState.drag_rect = {float(x), float(y), float(x), float(y)};
        gState.fRects.push_back(gState.drag_rect);
    }
}

void OnMouseDrag(ClientData, XEvent *eventPtr) {
    if (eventPtr->type == MotionNotify && gState.dragging && !gState.fRects.empty()) {
        int x = eventPtr->xmotion.x;
        int y = eventPtr->xmotion.y;
        gState.fRects.back().r = float(x);
        gState.fRects.back().b = float(y);
        draw();
    }
}

void OnMouseUp(ClientData, XEvent *eventPtr) {
    if (eventPtr->type == ButtonRelease) {
        gState.dragging = false;
        draw();
    }
}

void OnConfigure(ClientData, XEvent *eventPtr) {
    if (eventPtr->type == ConfigureNotify) {
        int w = eventPtr->xconfigure.width;
        int h = eventPtr->xconfigure.height;
        if (w != gState.window_width || h != gState.window_height) {
            gState.window_width = w;
            gState.window_height = h;
            gState.surface = SkSurfaces::Raster(SkImageInfo::MakeN32Premul(gState.window_width, gState.window_height));
            draw();
        }
    }
}

void OnKeyDown(ClientData, XEvent *eventPtr) {
    if (eventPtr->type == KeyPress) {
        KeySym ks = XLookupKeysym(&eventPtr->xkey, 0);
        if (ks == XK_Escape) {
            gState.fQuit = true;
            if (gAnimToken) {
                Tcl_DeleteTimerHandler(gAnimToken);
                gAnimToken = nullptr;
            }
            Tk_DestroyWindow(gMainWin);
        }
    }
}

void OnExpose(ClientData, XEvent *eventPtr) {
    if (eventPtr->type == Expose) {
        draw();
    }
}

// Animation callback
void AnimationTimer(ClientData) {
    if (!gState.fQuit) {
        gState.rotation = fmod(gState.rotation + 1, 360);
        draw();
        schedule_animation(16);
    }
}

void schedule_animation(int ms) {
    gAnimToken = Tcl_CreateTimerHandler(ms, AnimationTimer, nullptr);
}

// Convert Skia surface to Tk PhotoImage
void skia_to_photoimage(SkSurface *surface, Tk_PhotoHandle photo, int w, int h) {
    SkImageInfo info = SkImageInfo::MakeN32Premul(w, h);
    std::vector<uint32_t> pixels(w * h);
    if (!surface->readPixels(info, pixels.data(), w*4, 0, 0)) return;

    // Tk expects RGBA, Skia gives BGRA. Swap B and R.
    for (uint32_t &px : pixels) {
        uint8_t b = (px >> 16) & 0xFF;
        uint8_t r = (px >> 0) & 0xFF;
        px = (px & 0xFF00FF00) | (r << 16) | (b << 0);
    }

    Tk_PhotoImageBlock block;
    block.width = w;
    block.height = h;
    block.pixelSize = 4;
    block.pitch = w * 4;
    block.offset[0] = 0; // Red
    block.offset[1] = 1; // Green
    block.offset[2] = 2; // Blue
    block.offset[3] = 3; // Alpha
    block.pixelPtr = reinterpret_cast<unsigned char *>(pixels.data());

    Tk_PhotoSetSize(gInterp, photo, w, h);
    Tk_PhotoPutBlock(gInterp, photo, &block, 0, 0, w, h, TK_PHOTO_COMPOSITE_SET);
    // pixels will be destroyed after this function, but Tk copies the data.
}

// Drawing logic
void draw() {
    if (!gState.surface)
        gState.surface = SkSurfaces::Raster(SkImageInfo::MakeN32Premul(gState.window_width, gState.window_height));
    SkCanvas *canvas = gState.surface->getCanvas();
    canvas->clear(SK_ColorWHITE);

    // Help message
    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setColor(SK_ColorBLACK);
    canvas->drawString(HELP_MESSAGE, 10, gState.font.getSize() + 10, gState.font, paint);

    // Rectangles
    SkRandom rand(0);
    for (const auto &rect : gState.fRects) {
        paint.setColor(rand.nextU() | 0x44808080);
        SkRect skrect = SkRect::MakeLTRB(rect.l, rect.t, rect.r, rect.b);
        canvas->drawRect(skrect, paint);
    }

    // Rotating star in center
    canvas->save();
    float cx = gState.window_width / 2.0f;
    float cy = gState.window_height / 2.0f;
    canvas->translate(cx, cy);
    canvas->rotate(gState.rotation);
    paint.setColor(SK_ColorBLUE);
    SkPath star = create_star();
    canvas->drawPath(star, paint);
    canvas->restore();

    // Copy to Tk PhotoImage
    if (!gState.tk_photo) return;
    skia_to_photoimage(gState.surface.get(), gState.tk_photo, gState.window_width, gState.window_height);

    // Draw PhotoImage on canvas
    std::ostringstream oss;
    oss << "if {[winfo exists .c]} {.c create image 0 0 -anchor nw -image skimg}";
    Tcl_Eval(gInterp, oss.str().c_str());
}

// Main
int main(int argc, char **argv) {
    // Init Tcl/Tk
    gInterp = Tcl_CreateInterp();
    if (Tcl_Init(gInterp) == TCL_ERROR) return 1;
    if (Tk_Init(gInterp) == TCL_ERROR) return 1;

    // Create root window
    Tk_MainWindow(gInterp);
    Tcl_Eval(gInterp, "wm title . {Skia + Tk Example}");
    Tcl_Eval(gInterp, "canvas .c -width 800 -height 600 -bg white");
    Tcl_Eval(gInterp, "pack .c -fill both -expand 1");
    Tcl_DoOneEvent(0);  // Force Tk to process widget creation
    gMainWin = Tk_NameToWindow(gInterp, ".", nullptr);
    if (!gMainWin) {
      std::cerr << "Failed to get Tk main window" << std::endl;
      return 1;
    }
    gWin = Tk_NameToWindow(gInterp, ".c", gMainWin);
    if (!gWin) {
      std::cerr << "Failed to get Tk window for .c" << std::endl;
      return 1;
    }

    // Create Tk PhotoImage
    Tcl_Eval(gInterp, "image create photo skimg");
    gState.tk_photo = Tk_FindPhoto(gInterp, "skimg");
    if (!gState.tk_photo) {
      std::cerr << "Failed to create/find Tk PhotoImage 'skimg'" << std::endl;
      return 1;
    }

    // Bind events
    Tk_CreateEventHandler(gWin, ButtonPressMask, OnMouseDown, nullptr);
    Tk_CreateEventHandler(gWin, ButtonReleaseMask, OnMouseUp, nullptr);
    Tk_CreateEventHandler(gWin, ButtonMotionMask, OnMouseDrag, nullptr);
    Tk_CreateEventHandler(gWin, StructureNotifyMask, OnConfigure, nullptr);
    Tk_CreateEventHandler(gWin, ExposureMask, OnExpose, nullptr);
    Tk_CreateEventHandler(gMainWin, KeyPressMask, OnKeyDown, nullptr);

    Tk_GeometryRequest(gWin, 800, 600);

    // Start animation
    draw();
    schedule_animation(16);

    // Main loop
    Tk_MainLoop();

    // Cleanup
    Tcl_DeleteInterp(gInterp);
    return 0;
}
