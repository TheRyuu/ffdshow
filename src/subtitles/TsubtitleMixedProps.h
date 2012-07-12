#pragma once

#include "TsubtitleProps.h"
#include "ffglobals.h"
#include "TfontSettings.h"
#include "rational.h"

// Mix stream's ASS options (TSubtitleProps) and user's options (TprintPrefs)
// for the convenience of the use by TrenderedTextSubtitleWord.
struct TSubtitleMixedProps: public TSubtitleProps {
    TSubtitleMixedProps(const TSubtitleProps &props, const TprintPrefs &prefs);
    TSubtitleMixedProps();

    YUVcolorA bodyYUV;
    YUVcolorA outlineYUV;
    YUVcolorA shadowYUV;

    uint64_t csp;
    int bodyBlurCount;
    int outlineBlurCount;
    bool opaqueBox;
    TfontSettings::TshadowMode shadowMode;
    bool autoSize;
    Rational sar;
    int gdi_font_scale;
    int dx, dy, clipdy;
    int blurStrength;
    bool hqBorder;  // use MPC's createWidenedRegion
    LONG lfWeight;  // LOGFONT::lfWeight for GDI
    int calculated_spacing;

    CPoint get_rotationAxis() const;
    double get_marginR(double lineWidth = 0) const;
    double get_marginL(double lineWidth = 0) const;
    double get_marginTop() const;
    double get_marginBottom() const;
    double getScrollStart() const;
    double getScrollSpeed() const; // pixels / ms
    double get_movedistanceV() const;
    double get_movedistanceH() const;
    double get_fader(REFERENCE_TIME rtStart) const;
    double get_scrollFader(int pos) const;
    int get_fadeawayWidth() const;
    CRect get_clip() const;
    void toLOGFONT(LOGFONT &lf) const;
    bool isASSmove() const {
        return isMove && !forcedCollisionCheck;
    }

    bool isCollisionCheckRequired() const {
        return (!isMove || forcedCollisionCheck) && !isOrg && !isScroll();
    }

private:
    bool forcedCollisionCheck;

    int getBlurCountBody(const TprintPrefs &prefs) const;
    int getBlurCountOutline(const TprintPrefs &prefs) const;
};