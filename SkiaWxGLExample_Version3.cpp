/*
  c++ \
  `wx-config --cxxflags` \
  -DGR_GL_CHECK_ERROR=0 -DGR_GL_LOG_CALLS=0 -Wall \
  -I./skia -I./skia/include/core/ -I./skia/include/gpu/ganesh/ \
  -DSK_FONTMGR_FONTCONFIG_AVAILABLE \
  SkiaWxGLExample_Version3.cpp \
  -L./skia/out/Release/ -lskparagraph -lsvg -lskshaper -lskunicode_icu -lskunicode_core -lskia \
  -lfreetype -lwebp -ljpeg -lwebpdemux -lpng -lz \
  -pthread -lwx_gtk3u_gl-3.2 -lwx_gtk3u_core-3.2 -lwx_baseu-3.2 \
  -lGL -lGLEW \
  -lfontconfig \
  -o SkiaWxGLExample_Version3
*/
#include <wx/wx.h>
#include <GL/glew.h>
#include <wx/glcanvas.h>
#include <SkSurface.h>
#include <SkCanvas.h>
#include <SkPaint.h>
#include <SkPath.h>
#include <SkGraphics.h>
#include <SkImageInfo.h>
#include <SkColorSpace.h>
#include <SkFont.h>
#include <SkTypeface.h>
#include <include/gpu/ganesh/SkSurfaceGanesh.h> // SkSurfaces::WrapBackendRenderTarget
#include <include/gpu/ganesh/GrBackendSurface.h> // GrBackendRenderTarget
#include <include/gpu/ganesh/GrDirectContext.h>
#include <include/gpu/ganesh/gl/GrGLInterface.h>
#include <include/gpu/ganesh/gl/GrGLDirectContext.h> // GrDirectContexts
#include <include/gpu/ganesh/gl/GrGLBackendSurface.h> // GrBackendRenderTargets
#include "include/core/SkFontMgr.h"
#ifdef __linux__
#include "include/ports/SkFontConfigInterface.h"
#include "include/ports/SkFontMgr_FontConfigInterface.h"
#endif
#ifdef __APPLE__
#include "include/ports/SkFontMgr_mac_ct.h"
#endif

#include <GL/gl.h>

class SkiaGLCanvas : public wxGLCanvas
{
public:
    SkiaGLCanvas(wxWindow* parent)
        : wxGLCanvas(parent, wxID_ANY, nullptr, wxDefaultPosition, wxSize(640, 480), wxFULL_REPAINT_ON_RESIZE)
    {
        m_glContext = new wxGLContext(this);
        Bind(wxEVT_PAINT, &SkiaGLCanvas::OnPaint, this);
        Bind(wxEVT_SIZE,  &SkiaGLCanvas::OnResize, this);
    }

    ~SkiaGLCanvas()
    {
        if (m_surface) m_surface.reset();
        if (m_grContext) m_grContext.reset();
        delete m_glContext;
    }

private:
    void OnResize(wxSizeEvent& event)
    {
        m_surface.reset();
        Refresh();
        event.Skip();
    }

    void OnPaint(wxPaintEvent&)
    {
        wxPaintDC dc(this);
        SetCurrent(*m_glContext);

        static bool glewInitDone = false;
        if (!glewInitDone) {
            glewExperimental = GL_TRUE;
            glewInit();
            glewInitDone = true;
        }

        int w, h;
        GetClientSize(&w, &h);

        if (w <= 0 || h <= 0) return;

        if (!m_grContext) {
            auto interface = GrGLMakeNativeInterface();
            m_grContext = GrDirectContexts::MakeGL(interface);
        }

        if (!m_surface || w != m_lastW || h != m_lastH) {
            GrGLFramebufferInfo fbInfo;
            fbInfo.fFBOID = 0;
            fbInfo.fFormat = GL_RGBA8;

            GrBackendRenderTarget backendRT = GrBackendRenderTargets::MakeGL(w, h, 0, 0, fbInfo);
            m_surface = SkSurfaces::WrapBackendRenderTarget(
                m_grContext.get(),
                backendRT,
                kBottomLeft_GrSurfaceOrigin,
                kRGBA_8888_SkColorType,
                nullptr,
                nullptr
            );
            m_lastW = w;
            m_lastH = h;
        }

        SkCanvas* canvas = m_surface->getCanvas();

        // Clear to white (original uses SDL_RenderClear, which is white)
        canvas->clear(SK_ColorWHITE);

        // --- Faithful reproduction of SkiaSDLExample.cpp drawing commands ---

        // 1. Blue rounded rectangle (stroked)
        SkPaint rrectPaint;
        rrectPaint.setColor(0xFF4285F4); // Google's blue
        rrectPaint.setAntiAlias(true);
        rrectPaint.setStyle(SkPaint::kStroke_Style);
        rrectPaint.setStrokeWidth(4);
        SkRect rrectRect = SkRect::MakeXYWH(60, 40, 360, 240);
        canvas->drawRoundRect(rrectRect, 40, 40, rrectPaint);

        // 2. Red filled arc
        SkPaint arcPaint;
        arcPaint.setColor(0xFFEA4335); // Google's red
        arcPaint.setAntiAlias(true);
        arcPaint.setStyle(SkPaint::kFill_Style);
        SkRect arcRect = SkRect::MakeXYWH(420, 60, 160, 160);
        // Skia's angles are degrees, sweep, and start at 3 o'clock, CW.
        canvas->drawArc(arcRect, 30, 300, true, arcPaint);

        // 3. Green cubic path (stroked, closed)
        SkPaint cubicPaint;
        cubicPaint.setColor(0xFF34A853); // Google's green
        cubicPaint.setAntiAlias(true);
        cubicPaint.setStyle(SkPaint::kStroke_Style);
        cubicPaint.setStrokeWidth(3);
        SkPath cubicPath;
        cubicPath.moveTo(100, 320);
        cubicPath.cubicTo(180, 450, 330, 180, 380, 340);
        cubicPath.close();
        canvas->drawPath(cubicPath, cubicPaint);

        // 4. Black text "Skia" (centered horizontally, same baseline as original)
        SkPaint textPaint;
        textPaint.setColor(SK_ColorBLACK);
        textPaint.setAntiAlias(true);

 #ifdef __linux__
    sk_sp<SkFontConfigInterface> fc(SkFontConfigInterface::RefGlobal());
    sk_sp<SkTypeface> typeface(SkFontMgr_New_FCI(std::move(fc))->legacyMakeTypeface("",SkFontStyle()));
#endif
#ifdef __APPLE__
    sk_sp<SkTypeface> typeface(SkFontMgr_New_CoreText(nullptr)->legacyMakeTypeface("",SkFontStyle()));
#endif
        SkFont font(typeface, 64);
        // Centered horizontally at 500,390 as in the original, baseline at y=390
        canvas->drawString("Skia", 500, 390, font, textPaint);

        // --- End of faithful reproduction ---

        auto direct = GrAsDirectContext(m_surface->recordingContext());
        if (direct) {
          direct->flush(m_surface.get(), SkSurfaces::BackendSurfaceAccess::kNoAccess, GrFlushInfo());
          direct->submit();
        }
        glFlush();
        SwapBuffers();
    }

    wxGLContext* m_glContext = nullptr;
    sk_sp<GrDirectContext> m_grContext;
    sk_sp<SkSurface> m_surface;
    int m_lastW = 0, m_lastH = 0;
};

class SkiaFrame : public wxFrame
{
public:
    SkiaFrame()
        : wxFrame(nullptr, wxID_ANY, "Skia wxWidgets OpenGL (Faithful Port)", wxDefaultPosition, wxSize(640, 480))
    {
        new SkiaGLCanvas(this);
    }
};

class SkiaApp : public wxApp
{
public:
    bool OnInit() override
    {
        SkGraphics::Init();
        auto* frame = new SkiaFrame();
        frame->Show(true);
        return true;
    }
};

wxIMPLEMENT_APP(SkiaApp);