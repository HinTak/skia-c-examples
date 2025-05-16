#include <wx/wx.h>
#include <wx/glcanvas.h>
#include <SkSurface.h>
#include <SkCanvas.h>
#include <SkPaint.h>
#include <SkPath.h>
#include <SkGraphics.h>
#include <SkImageInfo.h>
#include <SkFont.h>
#include <SkTypeface.h>
#include <include/gpu/GrDirectContext.h>
#include <include/gpu/gl/GrGLInterface.h>
#include <GL/glew.h>
#include <GL/gl.h>

// Custom wxGLCanvas for Skia rendering
class SkiaGLCanvas : public wxGLCanvas
{
public:
    SkiaGLCanvas(wxWindow* parent)
        : wxGLCanvas(parent, wxID_ANY, nullptr, wxDefaultPosition, wxSize(640, 480), wxFULL_REPAINT_ON_RESIZE)
    {
        m_glContext = new wxGLContext(this);
        Bind(wxEVT_PAINT, &SkiaGLCanvas::OnPaint, this);
        Bind(wxEVT_SIZE, &SkiaGLCanvas::OnResize, this);
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
        m_surface.reset(); // Reset the Skia surface on resize
        Refresh();
        event.Skip();
    }

    void OnPaint(wxPaintEvent&)
    {
        wxPaintDC dc(this);
        SetCurrent(*m_glContext);

        static bool glewInitialized = false;
        if (!glewInitialized) {
            glewExperimental = GL_TRUE;
            glewInit();
            glewInitialized = true;
        }

        int width, height;
        GetClientSize(&width, &height);
        if (width <= 0 || height <= 0) return;

        if (!m_grContext) {
            auto interface = GrGLMakeNativeInterface();
            m_grContext = GrDirectContext::MakeGL(interface);
        }

        if (!m_surface || width != m_lastWidth || height != m_lastHeight) {
            GrGLFramebufferInfo fbInfo;
            fbInfo.fFBOID = 0; // Default framebuffer
            fbInfo.fFormat = GL_RGBA8;

            GrBackendRenderTarget backendRT(width, height, 0, 0, fbInfo);
            m_surface = SkSurface::MakeFromBackendRenderTarget(
                m_grContext.get(),
                backendRT,
                kBottomLeft_GrSurfaceOrigin,
                kRGBA_8888_SkColorType,
                nullptr,
                nullptr
            );
            m_lastWidth = width;
            m_lastHeight = height;
        }

        SkCanvas* canvas = m_surface->getCanvas();

        // Clear to white, matching SDL_RenderClear in the original
        canvas->clear(SK_ColorWHITE);

        // --- Faithful Drawing Commands from SkiaSDLExample.cpp ---

        // 1. Blue rounded rectangle
        SkPaint rrectPaint;
        rrectPaint.setColor(0xFF4285F4); // Google's blue color
        rrectPaint.setAntiAlias(true);
        rrectPaint.setStyle(SkPaint::kStroke_Style);
        rrectPaint.setStrokeWidth(4);
        SkRect rrect = SkRect::MakeXYWH(60, 40, 360, 240);
        canvas->drawRoundRect(rrect, 40, 40, rrectPaint);

        // 2. Red filled arc
        SkPaint arcPaint;
        arcPaint.setColor(0xFFEA4335); // Google's red color
        arcPaint.setAntiAlias(true);
        arcPaint.setStyle(SkPaint::kFill_Style);
        SkRect arcRect = SkRect::MakeXYWH(420, 60, 160, 160);
        canvas->drawArc(arcRect, 30, 300, true, arcPaint);

        // 3. Green cubic path
        SkPaint cubicPaint;
        cubicPaint.setColor(0xFF34A853); // Google's green color
        cubicPaint.setAntiAlias(true);
        cubicPaint.setStyle(SkPaint::kStroke_Style);
        cubicPaint.setStrokeWidth(3);
        SkPath cubicPath;
        cubicPath.moveTo(100, 320);
        cubicPath.cubicTo(180, 450, 330, 180, 380, 340);
        cubicPath.close();
        canvas->drawPath(cubicPath, cubicPaint);

        // 4. Black text "Skia"
        SkPaint textPaint;
        textPaint.setColor(SK_ColorBLACK);
        textPaint.setAntiAlias(true);
        SkFont font(SkTypeface::MakeDefault(), 64);
        canvas->drawString("Skia", 500, 390, font, textPaint);

        // --- End of Faithful Drawing Commands ---

        m_surface->flushAndSubmit();
        glFlush();
        SwapBuffers();
    }

    wxGLContext* m_glContext = nullptr;
    sk_sp<GrDirectContext> m_grContext;
    sk_sp<SkSurface> m_surface;
    int m_lastWidth = 0, m_lastHeight = 0;
};

// wxWidgets frame containing the SkiaGLCanvas
class SkiaFrame : public wxFrame
{
public:
    SkiaFrame()
        : wxFrame(nullptr, wxID_ANY, "Skia wxWidgets OpenGL (Faithful Port)", wxDefaultPosition, wxSize(640, 480))
    {
        new SkiaGLCanvas(this);
    }
};

// wxWidgets application class
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