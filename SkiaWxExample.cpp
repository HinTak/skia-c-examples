/*
  c++ \
  `wx-config --cxxflags` \
  -DGR_GL_CHECK_ERROR=0 -DGR_GL_LOG_CALLS=0 -Wall \
  -I./skia -I./skia/include/core/ -I./skia/include/gpu/ganesh/ \
  -DSK_FONTMGR_FONTCONFIG_AVAILABLE \
  SkiaWxExample.cpp \
  -L./skia/out/Release/ -lskparagraph -lsvg -lskshaper -lskunicode_icu -lskunicode_core -lskia \
  -lfreetype -lwebp -ljpeg -lwebpdemux -lpng -lz \
  -pthread -lwx_gtk3u_core-3.2 -lwx_baseu-3.2 \
  -o SkiaWxExample
 */
#include <wx/wx.h>
#include <wx/dcbuffer.h>
#include <SkSurface.h>
#include <SkCanvas.h>
#include <SkPaint.h>
#include <SkPath.h>
#include <SkGraphics.h>
#include <SkImageInfo.h>
#include <SkData.h>
#include <SkStream.h>

// Custom wxPanel for Skia drawing
class SkiaPanel : public wxPanel
{
public:
    SkiaPanel(wxWindow* parent)
        : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(640, 480))
    {
        SetBackgroundStyle(wxBG_STYLE_PAINT);  // Double-buffered drawing
        Bind(wxEVT_PAINT, &SkiaPanel::OnPaint, this);
    }

    void OnPaint(wxPaintEvent& event)
    {
        wxAutoBufferedPaintDC dc(this);
        wxSize size = GetClientSize();

        // Create Skia surface using BGRA32 pixels on a wxMemoryDC
        SkImageInfo info = SkImageInfo::MakeN32Premul(size.x, size.y);
        std::vector<uint32_t> pixels(size.x * size.y);

        sk_sp<SkSurface> surface = SkSurfaces::WrapPixels(
            info, pixels.data(), size.x * 4);

        SkCanvas* canvas = surface->getCanvas();

        // Clear background
        canvas->clear(SK_ColorWHITE);

        // Draw something with Skia (same as original SDL2 example)
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

        // Transfer Skia's output to wxDC
        wxImage img(size.x, size.y, false);
        unsigned char* dst = img.GetData();
        uint32_t* src = pixels.data();
        for (int y = 0; y < size.y; ++y)
        {
            for (int x = 0; x < size.x; ++x)
            {
                uint32_t pixel = src[y * size.x + x];
                // Skia pixel is BGRA; wxImage wants RGB
                dst[0] = (pixel >> 16) & 0xFF; // R
                dst[1] = (pixel >> 8) & 0xFF;  // G
                dst[2] = (pixel) & 0xFF;       // B
                dst += 3;
            }
        }
        wxBitmap bmp(img);
        dc.DrawBitmap(bmp, 0, 0, false);
    }
};

// wxWidgets main frame
class SkiaFrame : public wxFrame
{
public:
    SkiaFrame()
        : wxFrame(NULL, wxID_ANY, "Skia wxWidgets Example", wxDefaultPosition, wxSize(640, 480))
    {
        new SkiaPanel(this);
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