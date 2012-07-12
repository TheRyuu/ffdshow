/*
 *  $Id: RTS.cpp 3650 2011-08-11 12:16:46Z XhmikosR $
 *
 *  (C) 2003-2006 Gabest
 *  (C) 2006-2010 see AUTHORS
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

// This file was copied from a part of RTS.cpp in guliverkli / Media Player Classic - Home Cinema.
// For the convenience of ffdshow project,
//     CStringW was replaced by ffstring.
//     CAtlArray was replaced by std::vector.
// Imported by h.yamagata.
#include "stdafx.h"
#include "CPolygon.h"
#include "TsubtitleMixedProps.h"
#include <math.h>

CPolygon::CPolygon(const TSubtitleMixedProps& mprops, const wchar_t *str)
    : m_str(str)
    , TrenderedTextSubtitleWord(mprops)
{
    init();
}

void CPolygon::init()
{
    double drawingScale = 1.0 / pow(2.0, mprops.polygon - 1);
    m_shadowSize = getShadowSize(0);
    overhang = computeOverhang();
    drawingScaleX = mprops.scaleX * drawingScale * mprops.dx / mprops.refResX;
    drawingScaleY = mprops.scaleY * drawingScale * mprops.dy / mprops.refResY;
    ParseStr();

    // Scaling is applied in ParseStr, so don't scale it again.
    mprops.scaleX = 1;
    mprops.scaleY = 1;

    CreatePath();
}

CPolygon::CPolygon(CPolygon& src)
    : m_str(src.m_str)
    , TrenderedTextSubtitleWord(*this)
{
    m_baseline = src.m_baseline;
    dxChar = src.dxChar;
    dyChar = src.dyChar;
    m_ascent = src.m_ascent;
    m_descent = src.m_descent;

    m_pathTypesOrg.insert(m_pathTypesOrg.begin(), src.m_pathTypesOrg.begin(), src.m_pathTypesOrg.end());
    m_pathPointsOrg.insert(m_pathPointsOrg.begin(), src.m_pathPointsOrg.begin(), src.m_pathPointsOrg.end());
}

CPolygon::~CPolygon()
{
}

TrenderedTextSubtitleWord* CPolygon::Copy()
{
    return(new CPolygon(*this));
}

bool CPolygon::Append(TrenderedTextSubtitleWord* w)
{
    CPolygon* p = dynamic_cast<CPolygon*>(w);
    if (!p) {
        return false;
    }

    // TODO
    return false;

    //return true;
}

bool CPolygon::GetLONG(ffstring& str, LONG& ret)
{
    const wchar_t *s = str.c_str();
    wchar_t *e;
    ret = wcstol(s, &e, 10);
    str.erase(0, e - s);
    return(e > s);
}

bool CPolygon::GetPOINT(ffstring& str, POINT& ret)
{
    return(GetLONG(str, ret.x) && GetLONG(str, ret.y));
}

bool CPolygon::ParseStr()
{
    if (m_pathTypesOrg.size() > 0) {
        return true;
    }

    CPoint p;
    int i, j, lastsplinestart = -1, firstmoveto = -1, lastmoveto = -1;

    ffstring str = m_str;
    size_t not_of = str.find_first_not_of(L"mnlbspc 0123456789-");
    if (not_of != ffstring::npos) {
        str.erase(not_of, str.size());
    }
    str = stringreplace(str, L"m", L"*m", rfReplaceAll);
    str = stringreplace(str, L"n", L"*n", rfReplaceAll);
    str = stringreplace(str, L"l", L"*l", rfReplaceAll);
    str = stringreplace(str, L"b", L"*b", rfReplaceAll);
    str = stringreplace(str, L"s", L"*s", rfReplaceAll);
    str = stringreplace(str, L"p", L"*p", rfReplaceAll);
    str = stringreplace(str, L"c", L"*c", rfReplaceAll);

    strings strs;
    strtok(str.c_str(), L"*", strs);

    int k = 0;
    foreach(ffstring & s, strs) {
        WCHAR c = s[0];
        size_t trimCount = s.find_first_not_of(L"mnlbspc ");
        s.erase(0, trimCount);
        switch (c) {
            case 'm':
                lastmoveto = (int)m_pathTypesOrg.size();
                if (firstmoveto == -1) {
                    firstmoveto = lastmoveto;
                }
                while (GetPOINT(s, p)) {
                    m_pathTypesOrg.push_back(PT_MOVETO);
                    m_pathPointsOrg.push_back(p);
                }
                break;
            case 'n':
                while (GetPOINT(s, p)) {
                    m_pathTypesOrg.push_back(PT_MOVETONC);
                    m_pathPointsOrg.push_back(p);
                }
                break;
            case 'l':
                if (m_pathPointsOrg.size() < 1) {
                    break;
                }
                while (GetPOINT(s, p)) {
                    m_pathTypesOrg.push_back(PT_LINETO);
                    m_pathPointsOrg.push_back(p);
                }
                break;
            case 'b':
                j = (int)m_pathTypesOrg.size();
                if (j < 1) {
                    break;
                }
                while (GetPOINT(s, p)) {
                    m_pathTypesOrg.push_back(PT_BEZIERTO);
                    m_pathPointsOrg.push_back(p);
                    j++;
                }
                j = (int)(m_pathTypesOrg.size() - ((m_pathTypesOrg.size() - j) % 3));
                m_pathTypesOrg.reserve(j);
                m_pathPointsOrg.reserve(j);
                break;
            case 's':
                if (m_pathPointsOrg.size() < 1) {
                    break;
                }
                j = lastsplinestart = (int)m_pathTypesOrg.size();
                i = 3;
                while (i-- && GetPOINT(s, p)) {
                    m_pathTypesOrg.push_back(PT_BSPLINETO);
                    m_pathPointsOrg.push_back(p);
                    j++;
                }
                if (m_pathTypesOrg.size() - lastsplinestart < 3) {
                    m_pathTypesOrg.reserve(lastsplinestart);
                    m_pathPointsOrg.reserve(lastsplinestart);
                    lastsplinestart = -1;
                }
                // no break here
            case 'p':
                if (m_pathPointsOrg.size() < 3) {
                    break;
                }
                while (GetPOINT(s, p)) {
                    m_pathTypesOrg.push_back(PT_BSPLINEPATCHTO);
                    m_pathPointsOrg.push_back(p);
                }
                break;
            case 'c':
                if (lastsplinestart > 0) {
                    m_pathTypesOrg.push_back(PT_BSPLINEPATCHTO);
                    m_pathTypesOrg.push_back(PT_BSPLINEPATCHTO);
                    m_pathTypesOrg.push_back(PT_BSPLINEPATCHTO);
                    p = m_pathPointsOrg[lastsplinestart - 1]; // we need p for temp storage, because operator [] will return a reference to CPoint and Add() may reallocate its internal buffer (this is true for MFC 7.0 but not for 6.0, hehe) : note the original code is written using CAtlArray.
                    m_pathPointsOrg.push_back(p);
                    p = m_pathPointsOrg[lastsplinestart];
                    m_pathPointsOrg.push_back(p);
                    p = m_pathPointsOrg[lastsplinestart + 1];
                    m_pathPointsOrg.push_back(p);
                    lastsplinestart = -1;
                }
                break;
            default:
                break;
        }
    }

    if (lastmoveto == -1 || firstmoveto > 0) {
        m_pathTypesOrg.clear();
        m_pathPointsOrg.clear();
        return false;
    }

    int minx = INT_MAX, miny = INT_MAX, maxx = -INT_MAX, maxy = -INT_MAX;

    for (size_t k = 0; k < m_pathTypesOrg.size(); k++) {
        m_pathPointsOrg[k].x = (int)(64 * drawingScaleX * m_pathPointsOrg[k].x);
        m_pathPointsOrg[k].y = (int)(64 * drawingScaleY * m_pathPointsOrg[k].y);
        if (minx > m_pathPointsOrg[k].x) {
            minx = m_pathPointsOrg[k].x;
        }
        if (miny > m_pathPointsOrg[k].y) {
            miny = m_pathPointsOrg[k].y;
        }
        if (maxx < m_pathPointsOrg[k].x) {
            maxx = m_pathPointsOrg[k].x;
        }
        if (maxy < m_pathPointsOrg[k].y) {
            maxy = m_pathPointsOrg[k].y;
        }
    }

    dxChar = std::max(maxx - minx, 0);

    dxChar = ((int)(mprops.scaleX * dxChar) + 32) >> 6;

    m_ascent = double(std::max(maxy - miny, 0)) / 64.0;
    m_descent = 0;
    m_baseline = m_ascent;
    dyChar = m_ascent * mprops.scaleY;

    return true;
}

bool CPolygon::CreatePath()
{
    size_t len = m_pathTypesOrg.size();
    if (len == 0) {
        return false;
    }

    if (mPathPoints != len) {
        mpPathTypes = (BYTE*)realloc(mpPathTypes, len * sizeof(BYTE));
        mpPathPoints = (POINT*)realloc(mpPathPoints, len * sizeof(POINT));
        if (!mpPathTypes || !mpPathPoints) {
            return false;
        }
        mPathPoints = (int)len;
    }

    memcpy(mpPathTypes, &m_pathTypesOrg[0], len * sizeof(BYTE));
    memcpy(mpPathPoints, &m_pathPointsOrg[0], len * sizeof(POINT));

    return true;
}
