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
#include <GrDirectContext.h>
#include <SkFont.h>
#include <SkTypeface.h>
#include <GL/gl.h>
#include <include/gpu/ganesh/SkSurfaceGanesh.h> // SkSurfaces::WrapBackendRenderTarget
#include <include/gpu/ganesh/GrBackendSurface.h> // GrBackendRenderTarget
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

class SkiaGLPanel : public wxGLCanvas
{
public:
    SkiaGLPanel(wxWindow* parent)
        : wxGLCanvas(parent, wxID_ANY, nullptr, wxDefaultPosition, wxSize(640, 480), wxFULL_REPAINT_ON_RESIZE)
        , m_context(nullptr)
        , m_skContext(nullptr)
        , m_surface(nullptr)
    {
        m_context = new wxGLContext(this);
        SetBackgroundStyle(wxBG_STYLE_CUSTOM);
        Bind(wxEVT_PAINT, &SkiaGLPanel::OnPaint, this);
        Bind(wxEVT_SIZE, &SkiaGLPanel::OnResize, this);
    }

    ~SkiaGLPanel() override
    {
        if (m_surface) m_surface.reset();
        if (m_skContext) m_skContext.reset();
        delete m_context;
    }

    void OnResize(wxSizeEvent& event)
    {
        m_surface.reset();
        Refresh();
        event.Skip();
    }

    void OnPaint(wxPaintEvent& event)
    {
        wxPaintDC dc(this);

        SetCurrent(*m_context);

        static bool glew_initialized = false;
        if (!glew_initialized) {
            glew_initialized = true;
            glewExperimental = GL_TRUE;
            if (glewInit() != GLEW_OK) {
                wxLogError("GLEW initialization failed!");
                return;
            }
        }

        int width, height;
        GetClientSize(&width, &height);

        if (!m_skContext) {
            auto interface = GrGLMakeNativeInterface();
            m_skContext = GrDirectContexts::MakeGL(interface);
        }

        if (!m_surface || m_lastWidth != width || m_lastHeight != height) {
            GrGLFramebufferInfo fbInfo;
            fbInfo.fFBOID = 0;
            fbInfo.fFormat = GL_RGBA8;

            GrBackendRenderTarget backendRenderTarget = GrBackendRenderTargets::MakeGL(width, height, 0, 0, fbInfo);

            SkColorType colorType = kRGBA_8888_SkColorType;

            m_surface = SkSurfaces::WrapBackendRenderTarget(
                m_skContext.get(),
                backendRenderTarget,
                kBottomLeft_GrSurfaceOrigin,
                colorType,
                nullptr,
                nullptr
            );
            m_lastWidth = width;
            m_lastHeight = height;
        }

        SkCanvas* canvas = m_surface->getCanvas();
        canvas->clear(SK_ColorWHITE);

        // 1. Draw a blue rounded rectangle
        SkPaint bluePaint;
        bluePaint.setColor(SK_ColorBLUE);
        bluePaint.setAntiAlias(true);
        bluePaint.setStrokeWidth(4);
        bluePaint.setStyle(SkPaint::kStroke_Style);

        SkRect roundRect = SkRect::MakeXYWH(50, 50, 300, 200);
        canvas->drawRoundRect(roundRect, 30, 30, bluePaint);

        // 2. Draw a filled red arc
        SkPaint redPaint;
        redPaint.setColor(SK_ColorRED);
        redPaint.setAntiAlias(true);
        redPaint.setStyle(SkPaint::kFill_Style);

        SkRect arcRect = SkRect::MakeXYWH(400, 50, 150, 150);
        // Sweep from 30 to 270 degrees (sweep 240deg)
        canvas->drawArc(arcRect, 30, 240, true, redPaint);

        // 3. Draw a closed green cubic path
        SkPaint greenPaint;
        greenPaint.setColor(SK_ColorGREEN);
        greenPaint.setAntiAlias(true);
        greenPaint.setStrokeWidth(3);
        greenPaint.setStyle(SkPaint::kStroke_Style);

        SkPath cubicPath;
        cubicPath.moveTo(100, 300);
        cubicPath.cubicTo(150, 400, 250, 200, 300, 350);
        cubicPath.lineTo(100, 300);
        canvas->drawPath(cubicPath, greenPaint);

        // 4. Draw text "Skia"
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
        canvas->drawString("Skia", 400, 250, font, textPaint);

        auto direct = GrAsDirectContext(m_surface->recordingContext());
        if (direct) {
          direct->flush(m_surface.get(), SkSurfaces::BackendSurfaceAccess::kNoAccess, GrFlushInfo());
          direct->submit();
        }
        glFlush();
        SwapBuffers();
    }

private:
    wxGLContext* m_context;
    sk_sp<GrDirectContext> m_skContext;
    sk_sp<SkSurface> m_surface;
    int m_lastWidth = 0;
    int m_lastHeight = 0;
};

// wxWidgets main frame
class SkiaFrame : public wxFrame
{
public:
    SkiaFrame()
        : wxFrame(NULL, wxID_ANY, "Skia wxWidgets OpenGL Example", wxDefaultPosition, wxSize(640, 480))
    {
        new SkiaGLPanel(this);
    }
};

// wxWidgets application class
class SkiaApp : public wxApp
{
public:
    virtual bool OnInit()
    {
        SkGraphics::Init();
        SkiaFrame* frame = new SkiaFrame();
        frame->Show(true);
        return true;
    }
};

wxIMPLEMENT_APP(SkiaApp);