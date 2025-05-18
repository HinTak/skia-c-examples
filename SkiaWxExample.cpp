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
#include <wx/rawbmp.h>
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

        // Create Skia surface using RGBA32 pixels / Premul to match wxBitmap
        SkImageInfo info = SkImageInfo::Make(size.x, size.y, kRGBA_8888_SkColorType, kPremul_SkAlphaType);
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
        wxBitmap bmp(size.x, size.y, 32);
        unsigned char *rgba = (unsigned char *)pixels.data();

        wxAlphaPixelData bmdata( bmp );
        /*
        wxAlphaPixelData::Iterator dst(bmdata);

        // the size of the RGBA buffer must match the dimensions of the bitmap
        const int w = bmp.GetWidth();
        const int h = bmp.GetHeight();

        for( int y = 0; y < h; y++) {
            dst.MoveTo(bmdata, 0, y);
            for(int x = 0; x < w; x++) {
                // wxBitmap contains rgb values pre-multiplied with alpha,
                // exactly what Skia was configured for.
                dst.Red() = rgba[0];
                dst.Green() = rgba[1];
                dst.Blue() = rgba[2];
                dst.Alpha() = rgba[3];
                dst++;
                rgba += 4;
            }
        }
        */
        auto pbmp = bmp.GetRawData(bmdata, 32); // Don't know how portable GetRawData is
        memcpy(pbmp, rgba, size.x * size.y * 4);
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