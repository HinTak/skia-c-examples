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
#include <GL/gl.h>
#include "include/core/SkFontMgr.h"
#ifdef __linux__
#include "include/ports/SkFontConfigInterface.h"
#include "include/ports/SkFontMgr_FontConfigInterface.h"
#endif
#ifdef __APPLE__
#include "include/ports/SkFontMgr_mac_ct.h"
#endif

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

    ~SkiaGLCanvas() override
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

        // Initialize GLEW
        static bool glewInitDone = false;
        if (!glewInitDone) {
            glewExperimental = GL_TRUE;
            if (glewInit() != GLEW_OK) {
                wxLogError("GLEW initialization failed!");
                return;
            }
            glewInitDone = true;
        }

        int w, h;
        GetClientSize(&w, &h);

        if (w <= 0 || h <= 0) return;

        // Create or reuse GrDirectContext
        if (!m_grContext) {
            auto interface = GrGLMakeNativeInterface();
            m_grContext = GrDirectContexts::MakeGL(interface);
        }

        // Create or reuse SkSurface
        if (!m_surface || w != m_lastW || h != m_lastH) {
            GrGLFramebufferInfo fbInfo;
            fbInfo.fFBOID = 0;  // Assuming default framebuffer
            fbInfo.fFormat = GL_RGBA8; // Or another format, depending on your setup

            GrBackendRenderTarget backendRT = GrBackendRenderTargets::MakeGL(w, h, 0, 0, fbInfo);
            m_surface = SkSurfaces::WrapBackendRenderTarget(
                m_grContext.get(),
                backendRT,
                kBottomLeft_GrSurfaceOrigin, // or kTopLeft_GrSurfaceOrigin
                kRGBA_8888_SkColorType,
                nullptr,
                nullptr
            );
            m_lastW = w;
            m_lastH = h;
        }

        SkCanvas* canvas = m_surface->getCanvas();

        // Clear to white (as in the original SDL example)
        canvas->clear(SK_ColorWHITE);

        // --- Drawing commands from the original SkiaSDLExample.cpp ---

        // 1. Blue rounded rectangle
        SkPaint rrPaint;
        rrPaint.setColor(0xFF4285F4);  // Google Blue
        rrPaint.setAntiAlias(true);
        rrPaint.setStyle(SkPaint::kStroke_Style);
        rrPaint.setStrokeWidth(4);

        SkRect rrRect = SkRect::MakeXYWH(60, 40, 360, 240);
        canvas->drawRoundRect(rrRect, 40, 40, rrPaint);

        // 2. Red arc
        SkPaint arcPaint;
        arcPaint.setColor(0xFFEA4335);  // Google Red
        arcPaint.setAntiAlias(true);
        arcPaint.setStyle(SkPaint::kFill_Style);

        SkRect arcRect = SkRect::MakeXYWH(420, 60, 160, 160);
        // Skia's sweep angle is how far to sweep *clockwise* from startAngle.
        canvas->drawArc(arcRect, 30, 300, true, arcPaint);

        // 3. Green cubic path
        SkPaint cubicPaint;
        cubicPaint.setColor(0xFF34A853);  // Google Green
        cubicPaint.setAntiAlias(true);
        cubicPaint.setStyle(SkPaint::kStroke_Style);
        cubicPaint.setStrokeWidth(3);

        SkPath cubicPath;
        cubicPath.moveTo(100, 320);
        cubicPath.cubicTo(180, 450, 330, 180, 380, 340);
        cubicPath.close(); // Ensure the path is closed
        canvas->drawPath(cubicPath, cubicPaint);

        // 4. "Skia" text
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
        SkFont textFont(typeface, 64); // Default typeface, size 64

        // Position the text (same as in original SDL example)
        canvas->drawString("Skia", 500, 390, textFont, textPaint);

        // --- End drawing commands ---

        // Flush and present
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