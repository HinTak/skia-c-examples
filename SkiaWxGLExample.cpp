/*
  c++ \
  `wx-config --cxxflags` \
  -DGR_GL_CHECK_ERROR=0 -DGR_GL_LOG_CALLS=0 -Wall \
  -I./skia -I./skia/include/core/ -I./skia/include/gpu/ganesh/ \
  -DSK_FONTMGR_FONTCONFIG_AVAILABLE \
  SkiaWxGLExample.cpp \
  -L./skia/out/Release/ -lskparagraph -lsvg -lskshaper -lskunicode_icu -lskunicode_core -lskia \
  -lfreetype -lwebp -ljpeg -lwebpdemux -lpng -lz \
  -pthread -lwx_gtk3u_gl-3.2 -lwx_gtk3u_core-3.2 -lwx_baseu-3.2 \
  -lGL -lGLEW \
  -o SkiaWxGLExample
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
#include <GrDirectContext.h>
#include <GL/gl.h>
#include <include/gpu/ganesh/SkSurfaceGanesh.h> // SkSurfaces::WrapBackendRenderTarget
#include <include/gpu/ganesh/GrBackendSurface.h> // GrBackendRenderTarget
#include <include/gpu/ganesh/gl/GrGLInterface.h>
#include <include/gpu/ganesh/gl/GrGLDirectContext.h> // GrDirectContexts
#include <include/gpu/ganesh/gl/GrGLBackendSurface.h> // GrBackendRenderTargets

// Custom wxGLCanvas for Skia drawing
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
        // Recreate Skia surface on resize
        m_surface.reset();
        Refresh();
        event.Skip();
    }

    void OnPaint(wxPaintEvent& event)
    {
        wxPaintDC dc(this);

        SetCurrent(*m_context);

        if (glewInit() != GLEW_OK) {
            wxLogError("GLEW initialization failed!");
            return;
        }

        // Get size
        int width, height;
        GetClientSize(&width, &height);

        // Set up Skia GPU context if not done already
        if (!m_skContext) {
            auto interface = GrGLMakeNativeInterface();
            m_skContext = GrDirectContexts::MakeGL(interface);
        }

        // Recreate Skia surface if needed
        if (!m_surface || m_lastWidth != width || m_lastHeight != height) {
            GrGLFramebufferInfo fbInfo;
            fbInfo.fFBOID = 0; // 0 is default framebuffer
            fbInfo.fFormat = GL_RGBA8;

            GrBackendRenderTarget backendRenderTarget = GrBackendRenderTargets::MakeGL(width, height, 0, 0, fbInfo);

            SkColorType colorType = kRGBA_8888_SkColorType;

            m_surface = SkSurfaces::WrapBackendRenderTarget(
                m_skContext.get(),
                backendRenderTarget,
                kBottomLeft_GrSurfaceOrigin, // GL default
                colorType,
                nullptr,
                nullptr
            );
            m_lastWidth = width;
            m_lastHeight = height;
        }

        SkCanvas* canvas = m_surface->getCanvas();

        // Draw using Skia
        canvas->clear(SK_ColorWHITE);

        SkPaint paint;
        paint.setColor(SK_ColorBLUE);
        paint.setStrokeWidth(5);
        paint.setStyle(SkPaint::kStroke_Style);

        SkPath path;
        path.moveTo(100, 100);
        path.lineTo(300, 100);
        path.lineTo(300, 300);
        path.lineTo(100, 300);
        path.close();

        canvas->drawPath(path, paint);

        // Present
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