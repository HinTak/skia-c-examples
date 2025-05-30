#include "include/core/SkCanvas.h"
#include "include/core/SkFont.h"
#include "include/core/SkImageInfo.h"
#include "include/core/SkPaint.h"
#include "include/core/SkPath.h"
#include "include/core/SkSurface.h"
#include "include/core/SkTypeface.h"
#include "include/core/SkStream.h"
#include "include/core/SkData.h"
#include "include/codec/SkEncodedImageFormat.h"
#include "include/encode/SkPngEncoder.h"
#include "include/core/SkContourMeasure.h"
#include <vector>
#include <cstring>
#include <cmath>
#include <memory>
#include "include/core/SkFontMgr.h"
#include "include/ports/SkFontConfigInterface.h"
#include "include/ports/SkFontMgr_FontConfigInterface.h"

#include "SkTextOnPath.h"

void SkCanvas_drawTextOnPath(SkCanvas& self, const std::string& text, const SkPath& path,
            const SkMatrix* matrix, const SkFont& font, const SkPaint& paint) {
            SkDrawTextOnPath(text.c_str(), text.size(), paint, font, path, matrix, &self);
        }

// --- Demo main ---

int main(int argc, char** argv) {
    // Create a raster surface (800x400)
    SkImageInfo info = SkImageInfo::MakeN32Premul(800, 400);
    auto surface = SkSurfaces::Raster(info);
    SkCanvas* canvas = surface->getCanvas();
    canvas->clear(SK_ColorWHITE);

    // Load typeface (use system default if nullptr)
    sk_sp<SkFontConfigInterface> fc(SkFontConfigInterface::RefGlobal());
    sk_sp<SkTypeface> typeface(SkFontMgr_New_FCI(std::move(fc))->legacyMakeTypeface("",SkFontStyle()));
    SkFont font(typeface, 72);

    // Create a path: a simple sine wave
    SkPath curve;
    curve.moveTo(60, 200);
    for (int x = 60; x <= 740; x += 10) {
        float y = 200 + 60 * std::sin(2 * 3.14159f * (x-60) / 350);
        curve.lineTo(x, y);
    }

    // Draw the curve for reference
    SkPaint pathPaint;
    pathPaint.setAntiAlias(true);
    pathPaint.setStyle(SkPaint::kStroke_Style);
    pathPaint.setColor(SK_ColorBLUE);
    pathPaint.setStrokeWidth(2);
    canvas->drawPath(curve, pathPaint);

    // Draw morphing text
    SkPaint textPaint;
    textPaint.setAntiAlias(true);
    textPaint.setColor(SK_ColorRED);
    SkCanvas_drawTextOnPath(*canvas, "Morphing Text On Path!", curve, nullptr, font, textPaint);

    // Save to PNG file
    auto image = surface->makeImageSnapshot();
    if (image) {
        auto png = SkPngEncoder::Encode(nullptr, image.get(), {});
        if (png) {
            SkFILEWStream out("morphing_text_on_path.png");
            out.write(png->data(), png->size());
        }
    }

    return 0;
}
