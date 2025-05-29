/*
 * Copyright 2018 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Modified to avoid use of SkTextToPathIter.
 */

#include "SkPathMeasure.h"
#include "SkCanvas.h"
#include "SkMatrix.h"
#include "SkPaint.h"
#include "SkTypeface.h"
#include "SkPath.h"
#include <vector>
#include <functional>

static void morphpoints(SkPoint dst[], const SkPoint src[], int count,
                        SkPathMeasure& meas, const SkMatrix& matrix) {
    for (int i = 0; i < count; i++) {
        SkPoint pos;
        SkVector tangent;

        matrix.mapXY(src[i].fX, src[i].fY, &pos);
        SkScalar sx = pos.fX;
        SkScalar sy = pos.fY;

        if (!meas.getPosTan(sx, &pos, &tangent)) {
            tangent.set(0, 0);
        }
        dst[i].set(pos.fX - tangent.fY * sy, pos.fY + tangent.fX * sy);
    }
}

static void morphpath(SkPath* dst, const SkPath& src, SkPathMeasure& meas,
                      const SkMatrix& matrix) {
    SkPath::Iter    iter(src, false);
    SkPoint         srcP[4], dstP[3];
    SkPath::Verb    verb;

    while ((verb = iter.next(srcP)) != SkPath::kDone_Verb) {
        switch (verb) {
            case SkPath::kMove_Verb:
                morphpoints(dstP, srcP, 1, meas, matrix);
                dst->moveTo(dstP[0]);
                break;
            case SkPath::kLine_Verb:
                srcP[0].fX = SkScalarAve(srcP[0].fX, srcP[1].fX);
                srcP[0].fY = SkScalarAve(srcP[0].fY, srcP[1].fY);
                morphpoints(dstP, srcP, 2, meas, matrix);
                dst->quadTo(dstP[0], dstP[1]);
                break;
            case SkPath::kQuad_Verb:
                morphpoints(dstP, &srcP[1], 2, meas, matrix);
                dst->quadTo(dstP[0], dstP[1]);
                break;
            case SkPath::kConic_Verb:
                morphpoints(dstP, &srcP[1], 2, meas, matrix);
                dst->conicTo(dstP[0], dstP[1], iter.conicWeight());
                break;
            case SkPath::kCubic_Verb:
                morphpoints(dstP, &srcP[1], 3, meas, matrix);
                dst->cubicTo(dstP[0], dstP[1], dstP[2]);
                break;
            case SkPath::kClose_Verb:
                dst->close();
                break;
            default:
                break;
        }
    }
}

// Helper: convert text to glyphs using SkPaint and store advances
static void textToGlyphsAndAdvances(const void* text, size_t byteLength, const SkPaint& paint, 
                                    std::vector<SkGlyphID>& glyphs, std::vector<SkScalar>& advances) {
    int count = paint.textToGlyphs(text, byteLength, nullptr, 0);
    glyphs.resize(count);
    paint.textToGlyphs(text, byteLength, glyphs.data(), count);

    advances.resize(count);
    paint.getTextWidths(glyphs.data(), count * sizeof(SkGlyphID), advances.data(), nullptr);
}

void SkVisitTextOnPath_noIter(const void* text, size_t byteLength, const SkPaint& paint,
                              const SkPath& follow, const SkMatrix* matrix,
                              const std::function<void(const SkPath&)>& visitor) {
    if (byteLength == 0) {
        return;
    }

    SkPathMeasure meas(follow, false);

    // Convert text to glyphs and advances
    std::vector<SkGlyphID> glyphs;
    std::vector<SkScalar> advances;
    textToGlyphsAndAdvances(text, byteLength, paint, glyphs, advances);

    // Determine initial offset for alignment
    SkScalar totalAdvance = 0;
    for (SkScalar adv : advances) totalAdvance += adv;
    SkScalar pathLen = meas.getLength();
    SkScalar hOffset = 0;
    switch (paint.getTextAlign()) {
        case SkPaint::kCenter_Align: hOffset = (pathLen - totalAdvance) / 2; break;
        case SkPaint::kRight_Align:  hOffset = (pathLen - totalAdvance); break;
        default: break; // Left align = 0
    }

    // Prepare font and path scaling
    SkFont font(paint.getTypeface(), paint.getTextSize());
    SkScalar xpos = hOffset;
    for (size_t i = 0; i < glyphs.size(); ++i) {
        SkPath glyphPath;
        if (font.getTypeface()) {
            font.getTypeface()->getPath(glyphs[i], &glyphPath);
        } else {
            paint.getTextPath(&glyphs[i], sizeof(SkGlyphID), 1, 0, 0, &glyphPath);
        }

        // Scale glyph path to match paint size
        SkMatrix glyphMatrix;
        glyphMatrix.setScale(paint.getTextSize() / font.getTypeface()->getUnitsPerEm(),
                             paint.getTextSize() / font.getTypeface()->getUnitsPerEm());
        glyphMatrix.postTranslate(xpos, 0);
        if (matrix) glyphMatrix.postConcat(*matrix);

        // Morph the glyph path onto the follow path
        SkPath morphedPath;
        morphpath(&morphedPath, glyphPath, meas, glyphMatrix);
        visitor(morphedPath);

        xpos += advances[i];
    }
}

void SkDrawTextOnPath_noIter(const void* text, size_t byteLength, const SkPaint& paint,
                             const SkPath& follow, const SkMatrix* matrix, SkCanvas* canvas) {
    SkVisitTextOnPath_noIter(text, byteLength, paint, follow, matrix, [canvas, &paint](const SkPath& path) {
        canvas->drawPath(path, paint);
    });
}

void SkDrawTextOnPathHV_noIter(const void* text, size_t byteLength, const SkPaint& paint,
                               const SkPath& follow, SkScalar h, SkScalar v, SkCanvas* canvas) {
    SkMatrix matrix = SkMatrix::MakeTrans(h, v);
    SkDrawTextOnPath_noIter(text, byteLength, paint, follow, &matrix, canvas);
}