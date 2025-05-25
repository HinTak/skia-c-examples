#include "include/core/SkCanvas.h"
#include "include/core/SkFont.h"
#include "include/core/SkPath.h"
#include "include/core/SkPaint.h"
#include "include/core/SkContourMeasure.h"
#include "include/core/SkPathBuilder.h"
#include <vector>
#include <cmath>

static SkVector getNormal(const SkVector& tangent) {
    return SkVector::Make(-tangent.fY, tangent.fX);
}

void DrawTextOnPath_Morphing2025(
    SkCanvas* canvas,
    const char* utf8Text,
    const SkFont& font,
    const SkPath& path,
    SkScalar hOffset = 0,
    SkScalar vOffset = 0,
    const SkPaint& paint = SkPaint())
{
    // Convert text to glyphs and advances
    int glyphCount = font.countText(utf8Text, strlen(utf8Text), SkTextEncoding::kUTF8);
    std::vector<SkGlyphID> glyphs(glyphCount);
    font.textToGlyphs(utf8Text, strlen(utf8Text), SkTextEncoding::kUTF8, glyphs.data(), glyphCount);
    std::vector<SkScalar> advances(glyphCount);
    font.getWidths(glyphs.data(), glyphCount, advances.data());

    // Path measure
    SkContourMeasureIter iter(path, false, 1.0f);
    std::unique_ptr<SkContourMeasure> contour = iter.next();
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

        // Morph each verb in glyph outline
        SkPathBuilder builder;
        SkPath::Iter it(glyphPath, false);
        SkPoint pts[4];
        SkPoint lastMoveTo = {0, 0};
        SkPoint lastPt = {0, 0};
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
                    lastMoveTo = morphedPt;
                    lastPt = p;
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
                    lastPt = p;
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
                    lastPt = ep;
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
                    lastPt = ep;
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