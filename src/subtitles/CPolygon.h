#pragma once

#include "TrenderedTextSubtitleWord.h"
struct TprintPrefs;

class CPolygon : public TrenderedTextSubtitleWord
{
    bool GetLONG(ffstring& str, LONG& ret);
    bool GetPOINT(ffstring& str, POINT& ret);
    bool ParseStr();
    double drawingScaleX, drawingScaleY;
    void init();

protected:
    ffstring m_str;

    std::vector<BYTE> m_pathTypesOrg;
    std::vector<CPoint> m_pathPointsOrg;

    virtual bool CreatePath();

public:
    CPolygon(const TSubtitleMixedProps& mprops, const wchar_t *str);
    CPolygon(CPolygon&); // can't use a const reference because we need to use CAtlArray::Copy which expects a non-const reference
    virtual ~CPolygon();

    virtual TrenderedTextSubtitleWord* Copy();
    virtual bool Append(TrenderedTextSubtitleWord* w);
};
