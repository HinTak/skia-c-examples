#include "include/core/SkCanvas.h"
#include "include/core/SkFont.h"
#include "include/core/SkPaint.h"
#include "include/core/SkPath.h"
#include "include/core/SkTypeface.h"
#include "include/utils/SkTextUtils.h"
#include "include/core/SkContourMeasure.h"
#include <vector>
#include <cstring>
#include <cmath>

// Helper: returns a normal vector (perpendicular to the tangent)
static SkVector getNormal(const SkVector& tangent) {
    return SkVector::Make(-tangent.fY, tangent.fX);
}

// For Skia 2024: You can use SkPath::Iter for path decomposition.
static void morphPathOnCurve(const SkPath& src,
                             SkPath* dst,
                             SkContourMeasure* measure,
                             SkScalar baseDistance,
                             SkScalar pathLength,
                             SkScalar vOffset) {
    SkPath::Iter iter(src, false);
    SkPoint pts[4];
    SkPoint lastPt = {0, 0};
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
            lastPt = pts[0];
            break;
        }
        case SkPath::kLine_Verb: {
            SkScalar arcDist = std::min(baseDistance + pts[1].x(), pathLength);
            SkPoint onPath, tangent;
            measure->getPosTan(arcDist, &onPath, &tangent);
            SkVector normal = getNormal(tangent);
            SkPoint morphed = onPath + normal * pts[1].y() + SkVector::Make(0, vOffset);
            dst->lineTo(morphed);
            lastPt = pts[1];
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
            lastPt = pts[2];
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
            lastPt = pts[3];
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

// Main function: Draw morphing text on path (Skia 2024+)
void drawTextOnPathMorphing(
    SkCanvas* canvas,
    const char* utf8Text,
    const SkFont& font,
    const SkPath& path,
    SkScalar hOffset = 0,
    SkScalar vOffset = 0,
    const SkPaint& paint = SkPaint())
{
    // Convert to glyph IDs and advances
    int glyphCount = font.countText(utf8Text, strlen(utf8Text), SkTextEncoding::kUTF8);
    if (glyphCount <= 0) return;
    std::vector<SkGlyphID> glyphs(glyphCount);
    font.textToGlyphs(utf8Text, strlen(utf8Text), SkTextEncoding::kUTF8, glyphs.data(), glyphCount);
    std::vector<SkScalar> advances(glyphCount);
    font.getWidths(glyphs.data(), glyphCount, advances.data());

    // Prepare path measuring
    SkContourMeasureIter iter(path, false, 1.0f);
    sk_sp<SkContourMeasure> contour(iter.next());
    if (!contour) return;
    SkScalar pathLength = contour->length();

    SkScalar distance = hOffset;
    for (int i = 0; i < glyphCount; ++i) {
        if (distance > pathLength)
            break;

        // Get glyph outline as SkPath
        SkPath glyphPath;
        if (!font.getPath(glyphs[i], &glyphPath)) {
            distance += advances[i];
            continue;
        }

        // Morph the glyph outline
        SkPath morphedPath;
        morphPathOnCurve(glyphPath, &morphedPath, contour.get(), distance, pathLength, vOffset);

        // Draw the morphed path
        canvas->drawPath(morphedPath, paint);

        // Advance along the path
        distance += advances[i];
    }
}