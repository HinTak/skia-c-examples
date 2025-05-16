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
        wxPaintDC dc(this);  // Required in wxWidgets on some platforms.
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
            m_grContext = GrDirectContext::MakeGL(interface);
        }

        if (!m_surface || w != m_lastW || h != m_lastH) {
            GrGLFramebufferInfo fbInfo;
            fbInfo.fFBOID = 0;
            fbInfo.fFormat = GL_RGBA8;

            GrBackendRenderTarget backendRT(w, h, 0, 0, fbInfo);
            m_surface = SkSurface::MakeFromBackendRenderTarget(
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

        // Clear the canvas to white (SDL_RenderClear in original)
        canvas->clear(SK_ColorWHITE);

        // 1. Blue rounded rectangle
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
        canvas->drawArc(arcRect, 30, 300, true, arcPaint);

        // 3. Green cubic path (closed)
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

        // 4. Black "Skia" text
        SkPaint textPaint;
        textPaint.setColor(SK_ColorBLACK);
        textPaint.setAntiAlias(true);

        SkFont font(SkTypeface::MakeDefault(), 64);
        canvas->drawString("Skia", 500, 390, font, textPaint);

        // Flush rendering
        m_surface->flushAndSubmit();
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