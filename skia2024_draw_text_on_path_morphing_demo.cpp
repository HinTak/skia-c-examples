// skia2024_draw_text_on_path_morphing_demo.cpp
// A minimal, runnable Skia 2024+ demo: true glyph morphing text-on-path!
// To build: link with Skia and its dependencies

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

// --- Morphing "draw text on path" implementation ---

static SkVector getNormal(const SkVector& tangent) {
    return SkVector::Make(-tangent.fY, tangent.fX);
}

static void morphPathOnCurve(const SkPath& src,
                             SkPath* dst,
                             SkContourMeasure* measure,
                             SkScalar baseDistance,
                             SkScalar pathLength,
                             SkScalar vOffset) {
    SkPath::Iter iter(src, false);
    SkPoint pts[4];
    SkPath::Verb verb;
    while ((verb = iter.next(pts)) != SkPath::kDone_Verb) {
        switch (verb) {
        case SkPath::kMove_Verb: {
            SkScalar arcDist = std::min(baseDistance + pts[0].x(), pathLength);
            SkPoint onPath, tangent;
            measure->getPosTan(arcDist, &onPath, &tangent);
            SkVector normal = getNormal(tangent);
            SkPoint morphed = onPath + normal * pts[0].y() + SkVector::Make(0, vOffset);
            dst->moveTo(morphed);
            break;
        }
        case SkPath::kLine_Verb: {
            SkScalar arcDist = std::min(baseDistance + pts[1].x(), pathLength);
            SkPoint onPath, tangent;
            measure->getPosTan(arcDist, &onPath, &tangent);
            SkVector normal = getNormal(tangent);
            SkPoint morphed = onPath + normal * pts[1].y() + SkVector::Make(0, vOffset);
            dst->lineTo(morphed);
            break;
        }
        case SkPath::kQuad_Verb: {
            SkScalar arcDist1 = std::min(baseDistance + pts[1].x(), pathLength);
            SkScalar arcDist2 = std::min(baseDistance + pts[2].x(), pathLength);
            SkPoint onPath1, tangent1, onPath2, tangent2;
            measure->getPosTan(arcDist1, &onPath1, &tangent1);
            measure->getPosTan(arcDist2, &onPath2, &tangent2);
            SkVector normal1 = getNormal(tangent1);
            SkVector normal2 = getNormal(tangent2);
            SkPoint morphed1 = onPath1 + normal1 * pts[1].y() + SkVector::Make(0, vOffset);
            SkPoint morphed2 = onPath2 + normal2 * pts[2].y() + SkVector::Make(0, vOffset);
            dst->quadTo(morphed1, morphed2);
            break;
        }
        case SkPath::kCubic_Verb: {
            SkScalar arcDist1 = std::min(baseDistance + pts[1].x(), pathLength);
            SkScalar arcDist2 = std::min(baseDistance + pts[2].x(), pathLength);
            SkScalar arcDist3 = std::min(baseDistance + pts[3].x(), pathLength);
            SkPoint onPath1, tangent1, onPath2, tangent2, onPath3, tangent3;
            measure->getPosTan(arcDist1, &onPath1, &tangent1);
            measure->getPosTan(arcDist2, &onPath2, &tangent2);
            measure->getPosTan(arcDist3, &onPath3, &tangent3);
            SkVector normal1 = getNormal(tangent1);
            SkVector normal2 = getNormal(tangent2);
            SkVector normal3 = getNormal(tangent3);
            SkPoint morphed1 = onPath1 + normal1 * pts[1].y() + SkVector::Make(0, vOffset);
            SkPoint morphed2 = onPath2 + normal2 * pts[2].y() + SkVector::Make(0, vOffset);
            SkPoint morphed3 = onPath3 + normal3 * pts[3].y() + SkVector::Make(0, vOffset);
            dst->cubicTo(morphed1, morphed2, morphed3);
            break;
        }
        case SkPath::kClose_Verb:
            dst->close();
            break;
        default:
            break;
        }
    }
}

void drawTextOnPathMorphing(
    SkCanvas* canvas,
    const char* utf8Text,
    const SkFont& font,
    const SkPath& path,
    SkScalar hOffset = 0,
    SkScalar vOffset = 0,
    const SkPaint& paint = SkPaint())
{
    int glyphCount = font.countText(utf8Text, strlen(utf8Text), SkTextEncoding::kUTF8);
    if (glyphCount <= 0) return;
    std::vector<SkGlyphID> glyphs(glyphCount);
    font.textToGlyphs(utf8Text, strlen(utf8Text), SkTextEncoding::kUTF8, glyphs.data(), glyphCount);
    std::vector<SkScalar> advances(glyphCount);
    font.getWidths(glyphs.data(), glyphCount, advances.data());

    SkContourMeasureIter iter(path, false, 1.0f);
    sk_sp<SkContourMeasure> contour(iter.next());
    if (!contour) return;
    SkScalar pathLength = contour->length();

    SkScalar distance = hOffset;
    for (int i = 0; i < glyphCount; ++i) {
        if (distance > pathLength)
            break;
        SkPath glyphPath;
        if (!font.getPath(glyphs[i], &glyphPath)) {
            distance += advances[i];
            continue;
        }
        SkPath morphedPath;
        morphPathOnCurve(glyphPath, &morphedPath, contour.get(), distance, pathLength, vOffset);
        canvas->drawPath(morphedPath, paint);
        distance += advances[i];
    }
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
    pathPaint.setStyle(SkPaint::kStroke_Style);
    pathPaint.setColor(SK_ColorBLUE);
    pathPaint.setStrokeWidth(2);
    canvas->drawPath(curve, pathPaint);

    // Draw morphing text
    SkPaint textPaint;
    textPaint.setColor(SK_ColorBLACK);
    drawTextOnPathMorphing(canvas, "Morphing Text On Path!", font, curve, 0, 0, textPaint);

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