// clang++ main.cpp -std=c++17 -I./skia -L./skia/out/Release -lskia -lfreetype -lwebp -ljpeg -lwebpdemux -lpng -lz -lfontconfig  -o morphtext

// Minimal Skia 2025+ sample: True glyph morphing on path
// Build with: clang++ main.cpp -std=c++17 -I/path/to/skia/include -L/path/to/skia/out/Release -lskia -o morphtext
// (You must link Skia and its dependencies. Adjust include/library paths as needed.)

#include "include/core/SkCanvas.h"
#include "include/core/SkFont.h"
#include "include/core/SkPaint.h"
#include "include/core/SkPath.h"
#include "include/core/SkSurface.h"
#include "include/codec/SkEncodedImageFormat.h"
#include "include/encode/SkPngEncoder.h"
#include "include/core/SkStream.h"
#include "include/core/SkContourMeasure.h"
#include "include/core/SkPathBuilder.h"
#include "include/core/SkTypeface.h"
#include <iostream>
#include <vector>
#include <cstring>
#include "include/core/SkFontMgr.h"
#include "include/ports/SkFontConfigInterface.h"
#include "include/ports/SkFontMgr_FontConfigInterface.h"

static SkVector getNormal(const SkVector& tangent) {
    return SkVector::Make(-tangent.fY, tangent.fX);
}

void DrawTextOnPath_Morphing2025(
    SkCanvas* canvas,
    const char* utf8Text,
    const SkFont& font,
    const SkPath& path,
    SkScalar hOffset,
    SkScalar vOffset,
    const SkPaint& paint)
{
    int glyphCount = font.countText(utf8Text, strlen(utf8Text), SkTextEncoding::kUTF8);
    std::vector<SkGlyphID> glyphs(glyphCount);
    font.textToGlyphs(utf8Text, strlen(utf8Text), SkTextEncoding::kUTF8, glyphs.data(), glyphCount);
    std::vector<SkScalar> advances(glyphCount);
    font.getWidths(glyphs.data(), glyphCount, advances.data());

    SkContourMeasureIter iter(path, false, 1.0f);
    sk_sp<SkContourMeasure> contour = iter.next();
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

        SkPathBuilder builder;
        SkPath::Iter it(glyphPath, false);
        SkPoint pts[4];
        for (SkPath::Verb verb = it.next(pts); verb != SkPath::kDone_Verb; verb = it.next(pts)) {
            switch (verb) {
                case SkPath::kMove_Verb: {
                    SkPoint p = pts[0];
                    SkScalar arcDist = distance + p.x();
                    if (arcDist > pathLength) arcDist = pathLength;
                    SkPoint onPath, tangent;
                    contour->getPosTan(arcDist, &onPath, &tangent);
                    SkVector normal = getNormal(tangent);
                    SkPoint morphedPt = onPath + normal * p.y() + SkVector::Make(0, vOffset);
                    builder.moveTo(morphedPt);
                    break;
                }
                case SkPath::kLine_Verb: {
                    SkPoint p = pts[1];
                    SkScalar arcDist = distance + p.x();
                    if (arcDist > pathLength) arcDist = pathLength;
                    SkPoint onPath, tangent;
                    contour->getPosTan(arcDist, &onPath, &tangent);
                    SkVector normal = getNormal(tangent);
                    SkPoint morphedPt = onPath + normal * p.y() + SkVector::Make(0, vOffset);
                    builder.lineTo(morphedPt);
                    break;
                }
                case SkPath::kQuad_Verb: {
                    SkPoint cp = pts[1], ep = pts[2];
                    SkScalar arcDist1 = distance + cp.x();
                    SkScalar arcDist2 = distance + ep.x();
                    if (arcDist1 > pathLength) arcDist1 = pathLength;
                    if (arcDist2 > pathLength) arcDist2 = pathLength;

                    SkPoint onPath1, tangent1, onPath2, tangent2;
                    contour->getPosTan(arcDist1, &onPath1, &tangent1);
                    contour->getPosTan(arcDist2, &onPath2, &tangent2);

                    SkVector normal1 = getNormal(tangent1);
                    SkVector normal2 = getNormal(tangent2);

                    SkPoint morphedCp = onPath1 + normal1 * cp.y() + SkVector::Make(0, vOffset);
                    SkPoint morphedEp = onPath2 + normal2 * ep.y() + SkVector::Make(0, vOffset);
                    builder.quadTo(morphedCp, morphedEp);
                    break;
                }
                case SkPath::kCubic_Verb: {
                    SkPoint cp1 = pts[1], cp2 = pts[2], ep = pts[3];
                    SkScalar arcDist1 = distance + cp1.x();
                    SkScalar arcDist2 = distance + cp2.x();
                    SkScalar arcDist3 = distance + ep.x();
                    if (arcDist1 > pathLength) arcDist1 = pathLength;
                    if (arcDist2 > pathLength) arcDist2 = pathLength;
                    if (arcDist3 > pathLength) arcDist3 = pathLength;

                    SkPoint onPath1, tangent1, onPath2, tangent2, onPath3, tangent3;
                    contour->getPosTan(arcDist1, &onPath1, &tangent1);
                    contour->getPosTan(arcDist2, &onPath2, &tangent2);
                    contour->getPosTan(arcDist3, &onPath3, &tangent3);

                    SkVector normal1 = getNormal(tangent1);
                    SkVector normal2 = getNormal(tangent2);
                    SkVector normal3 = getNormal(tangent3);

                    SkPoint morphedCp1 = onPath1 + normal1 * cp1.y() + SkVector::Make(0, vOffset);
                    SkPoint morphedCp2 = onPath2 + normal2 * cp2.y() + SkVector::Make(0, vOffset);
                    SkPoint morphedEp = onPath3 + normal3 * ep.y() + SkVector::Make(0, vOffset);
                    builder.cubicTo(morphedCp1, morphedCp2, morphedEp);
                    break;
                }
                case SkPath::kClose_Verb:
                    builder.close();
                    break;
                default:
                    break;
            }
        }

        SkPath morphedPath = builder.detach();
        canvas->drawPath(morphedPath, paint);

        distance += advances[i];
    }
}

int main() {
    const int W = 800, H = 400;
    sk_sp<SkSurface> surface = SkSurfaces::Raster(SkImageInfo::MakeN32Premul(W, H));
    SkCanvas* canvas = surface->getCanvas();
    canvas->clear(SK_ColorWHITE);

    // Build a path (simple arc)
    SkPath myPath;
    myPath.moveTo(100, 200);
    myPath.arcTo(600, 200, 700, 200, 300); // large arc

    // Draw the guide path
    SkPaint pathPaint;
    pathPaint.setAntiAlias(true);
    pathPaint.setStyle(SkPaint::kStroke_Style);
    pathPaint.setColor(SK_ColorLTGRAY);
    pathPaint.setStrokeWidth(4);
    canvas->drawPath(myPath, pathPaint);

    // Prepare font
    sk_sp<SkFontConfigInterface> fc(SkFontConfigInterface::RefGlobal());
    sk_sp<SkTypeface> typeface(SkFontMgr_New_FCI(std::move(fc))->legacyMakeTypeface("",SkFontStyle()));
    SkFont font(typeface, 64); // font size 64

    // Prepare paint
    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setColor(SK_ColorBLACK);

    // Draw morphed text
    DrawTextOnPath_Morphing2025(canvas, "Morph!", font, myPath, 0, 0, paint);

    // Save result to PNG
    sk_sp<SkImage> img(surface->makeImageSnapshot());
    SkFILEWStream out("morphed_text.png");
    auto data = SkPngEncoder::Encode(nullptr, img.get(), {});
    out.write(data->data(), data->size());

    std::cout << "Wrote morphed_text.png\n";
    return 0;
}