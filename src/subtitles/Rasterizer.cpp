/*
 *    Copyright (C) 2003-2006 Gabest
 *    http://www.gabest.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include <string.h>
#include <math.h>
#include <vector>
#include <algorithm>
#include "Rasterizer.h"
//#include "SeparableFilter.h"

#ifndef _MAX    /* avoid collision with common (nonconforming) macros */
#define _MAX    (max)
#define _MIN    (min)
#define _IMPL_MAX max
#define _IMPL_MIN min
#else
#define _IMPL_MAX _MAX
#define _IMPL_MIN _MIN
#endif

Rasterizer::Rasterizer() : mpPathTypes(NULL), mpPathPoints(NULL), mPathPoints(0), TrenderedSubtitleWordBase(true)
{
    mGlyphBmpWidth = mGlyphBmpHeight = 0;
    mPathOffsetX = mPathOffsetY = 0;
    mOffsetX = mOffsetY = 0;
}

Rasterizer::~Rasterizer()
{
    _TrashPath();
    _TrashOverlay();
}

void Rasterizer::_TrashPath()
{
    delete [] mpPathTypes;
    delete [] mpPathPoints;
    mpPathTypes = NULL;
    mpPathPoints = NULL;
    mPathPoints = 0;
}

void Rasterizer::_TrashOverlay()
{
}

void Rasterizer::_ReallocEdgeBuffer(int edges)
{
    mEdgeHeapSize = edges;
    mpEdgeBuffer = (Edge*)realloc(mpEdgeBuffer, sizeof(Edge) * edges);
}

void Rasterizer::_EvaluateBezier(int ptbase, bool fBSpline)
{
    const POINT* pt0 = mpPathPoints + ptbase;
    const POINT* pt1 = mpPathPoints + ptbase + 1;
    const POINT* pt2 = mpPathPoints + ptbase + 2;
    const POINT* pt3 = mpPathPoints + ptbase + 3;

    double x0 = pt0->x;
    double x1 = pt1->x;
    double x2 = pt2->x;
    double x3 = pt3->x;
    double y0 = pt0->y;
    double y1 = pt1->y;
    double y2 = pt2->y;
    double y3 = pt3->y;

    double cx3, cx2, cx1, cx0, cy3, cy2, cy1, cy0;

    if (fBSpline) {
        // 1   [-1 +3 -3 +1]
        // - * [+3 -6 +3  0]
        // 6   [-3  0 +3  0]
        //       [+1 +4 +1  0]

        double _1div6 = 1.0 / 6.0;

        cx3 = _1div6 * (-  x0 + 3 * x1 - 3 * x2 + x3);
        cx2 = _1div6 * (3 * x0 - 6 * x1 + 3 * x2);
        cx1 = _1div6 * (-3 * x0       + 3 * x2);
        cx0 = _1div6 * (x0 + 4 * x1 + 1 * x2);

        cy3 = _1div6 * (-  y0 + 3 * y1 - 3 * y2 + y3);
        cy2 = _1div6 * (3 * y0 - 6 * y1 + 3 * y2);
        cy1 = _1div6 * (-3 * y0     + 3 * y2);
        cy0 = _1div6 * (y0 + 4 * y1 + 1 * y2);
    } else { // bezier
        // [-1 +3 -3 +1]
        // [+3 -6 +3  0]
        // [-3 +3  0  0]
        // [+1  0  0  0]

        cx3 = -  x0 + 3 * x1 - 3 * x2 + x3;
        cx2 =  3 * x0 - 6 * x1 + 3 * x2;
        cx1 = -3 * x0 + 3 * x1;
        cx0 =    x0;

        cy3 = -  y0 + 3 * y1 - 3 * y2 + y3;
        cy2 =  3 * y0 - 6 * y1 + 3 * y2;
        cy1 = -3 * y0 + 3 * y1;
        cy0 =    y0;
    }

    //
    // This equation is from Graphics Gems I.
    //
    // The idea is that since we're approximating a cubic curve with lines,
    // any error we incur is due to the curvature of the line, which we can
    // estimate by calculating the maximum acceleration of the curve.  For
    // a cubic, the acceleration (second derivative) is a line, meaning that
    // the absolute maximum acceleration must occur at either the beginning
    // (|c2|) or the end (|c2+c3|).  Our bounds here are a little more
    // conservative than that, but that's okay.
    //
    // If the acceleration of the parametric formula is zero (c2 = c3 = 0),
    // that component of the curve is linear and does not incur any error.
    // If a=0 for both X and Y, the curve is a line segment and we can
    // use a step size of 1.

    double maxaccel1 = fabs(2 * cy2) + fabs(6 * cy3);
    double maxaccel2 = fabs(2 * cx2) + fabs(6 * cx3);

    double maxaccel = maxaccel1 > maxaccel2 ? maxaccel1 : maxaccel2;
    double h = 1.0;

    if (maxaccel > 8.0) {
        h = sqrt(8.0 / maxaccel);
    }

    if (!fFirstSet) {
        firstp.x = (LONG)cx0;
        firstp.y = (LONG)cy0;
        lastp = firstp;
        fFirstSet = true;
    }

    for (double t = 0; t < 1.0; t += h) {
        double x = cx0 + t * (cx1 + t * (cx2 + t * cx3));
        double y = cy0 + t * (cy1 + t * (cy2 + t * cy3));
        _EvaluateLine(lastp.x, lastp.y, (int)x, (int)y);
    }

    double x = cx0 + cx1 + cx2 + cx3;
    double y = cy0 + cy1 + cy2 + cy3;
    _EvaluateLine(lastp.x, lastp.y, (int)x, (int)y);
}

void Rasterizer::_EvaluateLine(int pt1idx, int pt2idx)
{
    const POINT* pt1 = mpPathPoints + pt1idx;
    const POINT* pt2 = mpPathPoints + pt2idx;

    _EvaluateLine(pt1->x, pt1->y, pt2->x, pt2->y);
}

void Rasterizer::_EvaluateLine(int x0, int y0, int x1, int y1)
{
    if (lastp.x != x0 || lastp.y != y0) {
        _EvaluateLine(lastp.x, lastp.y, x0, y0);
    }

    if (!fFirstSet) {
        firstp.x = x0;
        firstp.y = y0;
        fFirstSet = true;
    }
    lastp.x = x1;
    lastp.y = y1;

    if (y1 > y0) { // down
        __int64 xacc = (__int64)x0 << 13;

        // prestep y0 down

        int dy = y1 - y0;
        int y = ((y0 + 3)&~7) + 4;
        int iy = y >> 3;

        y1 = (y1 - 5) >> 3;

        if (iy <= y1) {
            __int64 invslope = (__int64(x1 - x0) << 16) / dy;

            while (mEdgeNext + y1 + 1 - iy > mEdgeHeapSize) {
                _ReallocEdgeBuffer(mEdgeHeapSize * 2);
            }

            xacc += (invslope * (y - y0)) >> 3;

            while (iy <= y1) {
                int ix = (int)((xacc + 32768) >> 16);

                mpEdgeBuffer[mEdgeNext].next = mpScanBuffer[iy];
                mpEdgeBuffer[mEdgeNext].posandflag = ix * 2 + 1;

                mpScanBuffer[iy] = mEdgeNext++;

                ++iy;
                xacc += invslope;
            }
        }
    } else if (y1 < y0) { // up
        __int64 xacc = (__int64)x1 << 13;

        // prestep y1 down

        int dy = y0 - y1;
        int y = ((y1 + 3)&~7) + 4;
        int iy = y >> 3;

        y0 = (y0 - 5) >> 3;

        if (iy <= y0) {
            __int64 invslope = (__int64(x0 - x1) << 16) / dy;

            while (mEdgeNext + y0 + 1 - iy > mEdgeHeapSize) {
                _ReallocEdgeBuffer(mEdgeHeapSize * 2);
            }

            xacc += (invslope * (y - y1)) >> 3;

            while (iy <= y0) {
                int ix = (int)((xacc + 32768) >> 16);

                mpEdgeBuffer[mEdgeNext].next = mpScanBuffer[iy];
                mpEdgeBuffer[mEdgeNext].posandflag = ix * 2;

                mpScanBuffer[iy] = mEdgeNext++;

                ++iy;
                xacc += invslope;
            }
        }
    }
}

bool Rasterizer::BeginPath(HDC hdc)
{
    _TrashPath();

    return !!::BeginPath(hdc);
}

bool Rasterizer::EndPath(HDC hdc)
{
    ::CloseFigure(hdc);

    if (::EndPath(hdc)) {
        mPathPoints = GetPath(hdc, NULL, NULL, 0);

        if (!mPathPoints) {
            return true;
        }

        mpPathTypes = (BYTE*)malloc(sizeof(BYTE) * mPathPoints);
        mpPathPoints = (CPoint*)malloc(sizeof(CPoint) * mPathPoints);

        if (mPathPoints == GetPath(hdc, mpPathPoints, mpPathTypes, mPathPoints)) {
            return true;
        }
    }

    ::AbortPath(hdc);

    return false;
}

bool Rasterizer::PartialBeginPath(HDC hdc, bool bClearPath)
{
    if (bClearPath) {
        _TrashPath();
    }

    return !!::BeginPath(hdc);
}

bool Rasterizer::PartialEndPath(HDC hdc, long dx, long dy)
{
    ::CloseFigure(hdc);

    if (::EndPath(hdc)) {
        int nPoints;
        BYTE* pNewTypes;
        CPoint* pNewPoints;

        nPoints = GetPath(hdc, NULL, NULL, 0);

        if (!nPoints) {
            return true;
        }

        pNewTypes = (BYTE*)realloc(mpPathTypes, (mPathPoints + nPoints) * sizeof(BYTE));
        pNewPoints = (CPoint*)realloc(mpPathPoints, (mPathPoints + nPoints) * sizeof(POINT));

        if (pNewTypes) {
            mpPathTypes = pNewTypes;
        }

        if (pNewPoints) {
            mpPathPoints = pNewPoints;
        }

        BYTE* pTypes = new BYTE[nPoints];
        POINT* pPoints = new POINT[nPoints];

        if (pNewTypes && pNewPoints && nPoints == GetPath(hdc, pPoints, pTypes, nPoints)) {
            for (int i = 0; i < nPoints; ++i) {
                mpPathPoints[mPathPoints + i].x = pPoints[i].x + dx;
                mpPathPoints[mPathPoints + i].y = pPoints[i].y + dy;
                mpPathTypes[mPathPoints + i] = pTypes[i];
            }

            mPathPoints += nPoints;

            delete[] pTypes;
            delete[] pPoints;
            return true;
        } else {
            DebugBreak();
        }

        delete[] pTypes;
        delete[] pPoints;
    }

    ::AbortPath(hdc);

    return false;
}

bool Rasterizer::ScanConvert()
{
    int lastmoveto = -1;
    int i;

    // Drop any outlines we may have.

    mOutline.clear();
    mWideOutline.clear();
    mWideBorder = 0;

    // Determine bounding box

    if (!mPathPoints) {
        mPathOffsetX = mPathOffsetY = 0;
        mWidth = mHeight = 0;
        return 0;
    }

    int minx = INT_MAX;
    int miny = INT_MAX;
    int maxx = INT_MIN;
    int maxy = INT_MIN;

    for (i = 0; i < mPathPoints; ++i) {
        int ix = mpPathPoints[i].x;
        int iy = mpPathPoints[i].y;

        if (ix < minx) {
            minx = ix;
        }
        if (ix > maxx) {
            maxx = ix;
        }
        if (iy < miny) {
            miny = iy;
        }
        if (iy > maxy) {
            maxy = iy;
        }
    }

    minx = (minx >> 3) & ~7;
    miny = (miny >> 3) & ~7;
    maxx = (maxx + 7) >> 3;
    maxy = (maxy + 7) >> 3;

    for (i = 0; i < mPathPoints; ++i) {
        mpPathPoints[i].x -= minx * 8;
        mpPathPoints[i].y -= miny * 8;
    }

    if (minx > maxx || miny > maxy) {
        mWidth = mHeight = 0;
        mPathOffsetX = mPathOffsetY = 0;
        _TrashPath();
        return true;
    }

    mWidth = maxx + 1 - minx;
    mHeight = maxy + 1 - miny;

    mPathOffsetX = minx;
    mPathOffsetY = miny;

    // Initialize edge buffer.  We use edge 0 as a sentinel.

    mEdgeNext = 1;
    mEdgeHeapSize = 2048;
    mpEdgeBuffer = (Edge*)malloc(sizeof(Edge) * mEdgeHeapSize);

    // Initialize scanline list.

    mpScanBuffer = new unsigned int[mHeight];
    memset(mpScanBuffer, 0, mHeight * sizeof(unsigned int));

    // Scan convert the outline.  Yuck, Bezier curves....

    // Unfortunately, Windows 95/98 GDI has a bad habit of giving us text
    // paths with all but the first figure left open, so we can't rely
    // on the PT_CLOSEFIGURE flag being used appropriately.

    fFirstSet = false;
    firstp.x = firstp.y = 0;
    lastp.x = lastp.y = 0;

    for (i = 0; i < mPathPoints; ++i) {
        BYTE t = mpPathTypes[i] & ~PT_CLOSEFIGURE;

        switch (t) {
            case PT_MOVETO:
                if (lastmoveto >= 0 && firstp != lastp) {
                    _EvaluateLine(lastp.x, lastp.y, firstp.x, firstp.y);
                }
                lastmoveto = i;
                fFirstSet = false;
                lastp = mpPathPoints[i];
                break;
            case PT_MOVETONC:
                break;
            case PT_LINETO:
                if (mPathPoints - (i - 1) >= 2) {
                    _EvaluateLine(i - 1, i);
                }
                break;
            case PT_BEZIERTO:
                if (mPathPoints - (i - 1) >= 4) {
                    _EvaluateBezier(i - 1, false);
                }
                i += 2;
                break;
            case PT_BSPLINETO:
                if (mPathPoints - (i - 1) >= 4) {
                    _EvaluateBezier(i - 1, true);
                }
                i += 2;
                break;
            case PT_BSPLINEPATCHTO:
                if (mPathPoints - (i - 3) >= 4) {
                    _EvaluateBezier(i - 3, true);
                }
                break;
        }
    }

    if (lastmoveto >= 0 && firstp != lastp) {
        _EvaluateLine(lastp.x, lastp.y, firstp.x, firstp.y);
    }

    // Free the path since we don't need it anymore.

    _TrashPath();

    // Convert the edges to spans.  We couldn't do this before because some of
    // the regions may have winding numbers >+1 and it would have been a pain
    // to try to adjust the spans on the fly.  We use one heap to detangle
    // a scanline's worth of edges from the singly-linked lists, and another
    // to collect the actual scans.

    std::vector<int> heap;

    mOutline.reserve(mEdgeNext / 2);

    __int64 y = 0;

    for (y = 0; y < mHeight; ++y) {
        int count = 0;

        // Detangle scanline into edge heap.

        for (unsigned ptr = (unsigned)(mpScanBuffer[y] & 0xffffffff); ptr; ptr = mpEdgeBuffer[ptr].next) {
            heap.push_back(mpEdgeBuffer[ptr].posandflag);
        }

        // Sort edge heap.  Note that we conveniently made the opening edges
        // one more than closing edges at the same spot, so we won't have any
        // problems with abutting spans.

        std::sort(heap.begin(), heap.end()/*begin() + heap.size()*/);

        // Process edges and add spans.  Since we only check for a non-zero
        // winding number, it doesn't matter which way the outlines go!

        std::vector<int>::iterator itX1 = heap.begin();
        std::vector<int>::iterator itX2 = heap.end(); // begin() + heap.size();

        int x1 = 0, x2 = 0;

        for (; itX1 != itX2; ++itX1) {
            int x = *itX1;

            if (!count) {
                x1 = (x >> 1);
            }

            if (x & 1) {
                ++count;
            } else {
                --count;
            }

            if (!count) {
                x2 = (x >> 1);

                if (x2 > x1) {
                    mOutline.push_back(std::pair<__int64, __int64>((y << 32) + x1 + 0x4000000040000000i64, (y << 32) + x2 + 0x4000000040000000i64)); // G: damn Avery, this is evil! :)
                }
            }
        }

        heap.clear();
    }

    // Dump the edge and scan buffers, since we no longer need them.

    free(mpEdgeBuffer);
    delete [] mpScanBuffer;

    // All done!

    return true;
}

using namespace std;

void Rasterizer::_OverlapRegion(tSpanBuffer& dst, tSpanBuffer& src, int dx, int dy)
{
    tSpanBuffer temp;

    temp.reserve(dst.size() + src.size());

    dst.swap(temp);

    tSpanBuffer::iterator itA = temp.begin();
    tSpanBuffer::iterator itAE = temp.end();
    tSpanBuffer::iterator itB = src.begin();
    tSpanBuffer::iterator itBE = src.end();

    // Don't worry -- even if dy<0 this will still work! // G: hehe, the evil twin :)

    unsigned __int64 offset1 = (((__int64)dy) << 32) - dx;
    unsigned __int64 offset2 = (((__int64)dy) << 32) + dx;

    while (itA != itAE && itB != itBE) {
        if ((*itB).first + offset1 < (*itA).first) {
            // B span is earlier.  Use it.

            unsigned __int64 x1 = (*itB).first + offset1;
            unsigned __int64 x2 = (*itB).second + offset2;

            ++itB;

            // B spans don't overlap, so begin merge loop with A first.

            for (;;) {
                // If we run out of A spans or the A span doesn't overlap,
                // then the next B span can't either (because B spans don't
                // overlap) and we exit.

                if (itA == itAE || (*itA).first > x2) {
                    break;
                }

                do {
                    x2 = _MAX(x2, (*itA++).second);
                } while (itA != itAE && (*itA).first <= x2);

                // If we run out of B spans or the B span doesn't overlap,
                // then the next A span can't either (because A spans don't
                // overlap) and we exit.

                if (itB == itBE || (*itB).first + offset1 > x2) {
                    break;
                }

                do {
                    x2 = _MAX(x2, (*itB++).second + offset2);
                } while (itB != itBE && (*itB).first + offset1 <= x2);
            }

            // Flush span.

            dst.push_back(tSpan(x1, x2));
        } else {
            // A span is earlier.  Use it.

            unsigned __int64 x1 = (*itA).first;
            unsigned __int64 x2 = (*itA).second;

            ++itA;

            // A spans don't overlap, so begin merge loop with B first.

            for (;;) {
                // If we run out of B spans or the B span doesn't overlap,
                // then the next A span can't either (because A spans don't
                // overlap) and we exit.

                if (itB == itBE || (*itB).first + offset1 > x2) {
                    break;
                }

                do {
                    x2 = _MAX(x2, (*itB++).second + offset2);
                } while (itB != itBE && (*itB).first + offset1 <= x2);

                // If we run out of A spans or the A span doesn't overlap,
                // then the next B span can't either (because B spans don't
                // overlap) and we exit.

                if (itA == itAE || (*itA).first > x2) {
                    break;
                }

                do {
                    x2 = _MAX(x2, (*itA++).second);
                } while (itA != itAE && (*itA).first <= x2);
            }

            // Flush span.

            dst.push_back(tSpan(x1, x2));
        }
    }

    // Copy over leftover spans.

    while (itA != itAE) {
        dst.push_back(*itA++);
    }

    while (itB != itBE) {
        dst.push_back(tSpan((*itB).first + offset1, (*itB).second + offset2));
        ++itB;
    }
}

bool Rasterizer::CreateWidenedRegion(int rx, int ry)
{
    if (rx < 0) {
        rx = 0;
    }
    if (ry < 0) {
        ry = 0;
    }

    mWideBorder = max(rx, ry);

    if (ry > 0) {
        // Do a half circle.
        // _OverlapRegion mirrors this so both halves are done.
        for (int y = -ry; y <= ry; ++y) {
            int x = (int)(0.5 + sqrt(float(ry * ry - y * y)) * float(rx) / float(ry));

            _OverlapRegion(mWideOutline, mOutline, x, y);
        }
    } else if (ry == 0 && rx > 0) {
        // There are artifacts if we don't make at least two overlaps of the line, even at same Y coord
        _OverlapRegion(mWideOutline, mOutline, rx, 0);
        _OverlapRegion(mWideOutline, mOutline, rx, 0);
    }

    return true;
}

void Rasterizer::DeleteOutlines()
{
    mWideOutline.clear();
    mOutline.clear();
}

bool Rasterizer::Rasterize(int xsub, int ysub, CRect &overhang, bool hqBorder, bool fCheckRange, const CPoint &bodysLeftTop, int resDx, int resDy, bool needAlignYV12)
{
    _TrashOverlay();

    if (!mWidth || !mHeight) {
        mGlyphBmpWidth = mGlyphBmpHeight = 1;
    }

    xsub &= 7;
    ysub &= 7;

    int width = mWidth + xsub + (overhang.left + overhang.right) * 8;
    int height = mHeight + ysub + (overhang.top + overhang.bottom) * 8;

    mOffsetX = mPathOffsetX - xsub;
    mOffsetY = mPathOffsetY - ysub;

    mWideBorder = ffalign(mWideBorder, 8);

    int gw          = ((width + 7) >> 3) + 1;
    mGlyphBmpHeight = ((height + 7) >> 3) + 1;

    int leftcut, topcut, rightcut, bottomcut;
    fCheckRange = fCheckRange && (gw * mGlyphBmpHeight < resDx * resDy / 4);
    if (fCheckRange) {
        // added by h.yamagata
        // After rotation, especially \fry, the bitmap may be too large to be located in memory (1GB or so).
        // We have to clip it. But it may be intentional for scroll...
        leftcut = 0;
        topcut = 0;
        rightcut = 0;
        bottomcut = 0;
        int lx = bodysLeftTop.x + mPathOffsetX / 8;
        int ty = bodysLeftTop.y + mPathOffsetY / 8;
        if (lx < 0) {
            leftcut = -lx;
        }
        if (lx + gw > resDx) {
            rightcut = lx + gw - resDx;
        }
        if (ty < 0) {
            topcut = -ty;
        }
        if (ty + mGlyphBmpHeight > resDy) {
            bottomcut = ty + mGlyphBmpHeight - resDy;
        }
        leftcut &= ~7;
        rightcut &= ~7;
        rightcut = gw - rightcut;
        if (rightcut < 0) {
            rightcut = 0;
        }
        bottomcut = mGlyphBmpHeight - bottomcut;
        if (bottomcut < 0) {
            bottomcut = 0;
        }
        gw = rightcut - leftcut;
        mGlyphBmpHeight = bottomcut - topcut;
        mPathOffsetX += leftcut << 3;
        mPathOffsetY += topcut << 3;
        mOffsetX += leftcut << 3;
        mOffsetY += topcut << 3;
        xsub -= leftcut << 3;
        ysub -= topcut << 3;
        rightcut -= leftcut;
        bottomcut -= topcut;
    }

    if (needAlignYV12) {
        if ((bodysLeftTop.x + (mPathOffsetX >> 3)) & 1) {
            gw++;
            overhang.left++;
        }
        if ((bodysLeftTop.y + (mPathOffsetY >> 3)) & 1) {
            mGlyphBmpHeight++;
            overhang.top++;
        }
    }

    mGlyphBmpWidth = ffalign(gw, 16);
    overhang.right  += mGlyphBmpWidth - gw;
    bmp[0] = aligned_calloc3<uint8_t>(mGlyphBmpWidth, mGlyphBmpHeight, 16);
    msk[0] = aligned_calloc3<uint8_t>(mGlyphBmpWidth, mGlyphBmpHeight, 16);

    xsub += overhang.left * 8;
    ysub += overhang.top * 8;

    if (fCheckRange) {
        RasterizeCore<true>(xsub, ysub, hqBorder, rightcut, bottomcut);
    } else {
        RasterizeCore<false>(xsub, ysub, hqBorder);
    }

    return true;
}

template <bool fCheckRange>
void Rasterizer::RasterizeCore(int xsub, int ysub, bool hqBorder, int rightcut, int bottomcut)
{
    tSpanBuffer* pOutline[2] = {&mOutline, &mWideOutline};
    byte* pDst[2] = {bmp[0], msk[0]};

    ptrdiff_t count = countof(pOutline) - 1;
    if (!hqBorder) {
        count--;
    }

    for (ptrdiff_t i = count; i >= 0; i--) {
        tSpanBuffer::iterator it = pOutline[i]->begin();
        tSpanBuffer::iterator itEnd = pOutline[i]->end();
        byte *dst0 = pDst[i];

        for (; it != itEnd; ++it) {
            int y = (int)(((*it).first >> 32) - 0x40000000) + ysub;
            int x1 = (int)(((*it).first & 0xffffffff) - 0x40000000) + xsub;
            int x2 = (int)(((*it).second & 0xffffffff) - 0x40000000) + xsub;

            if (x2 > x1) {
                int first = x1 >> 3;
                int last = (x2 - 1) >> 3;
                int y3 = y >> 3;

                if (fCheckRange) {
                    if (y3 < 0 || y3 >= bottomcut) {
                        continue;
                    }
                    if (last < 0) {
                        continue;
                    }
                    if (first < 0) {
                        first = 0;
                    }
                    if (first > rightcut) {
                        continue;
                    }
                    if (last > rightcut) {
                        last = rightcut;
                    }
                }

                byte* dst = dst0 + (mGlyphBmpWidth * (y3) + first);

                if (first == last) {
                    *dst += x2 - x1;
                } else {
                    *dst += ((first + 1) << 3) - x1;
                    dst ++;

                    while (++first < last) {
                        *dst += 0x08;
                        dst ++;
                    }

                    *dst += x2 - (last << 3);
                }
            }
        }
    }
}