#pragma once

#include "interfaces.h"
#include "Crect.h"

struct TfontSettings;
struct Rational;
class TfontManager;
struct TprintPrefs;
struct Ttransform {
    // Create different t1 and t2 for every effect,
    // as there can be multiple /t tags with different
    // times and style overriders
    bool isTransform;
    bool isAlpha;
    int alpha;
    REFERENCE_TIME alphaT1, alphaT2;
    double accel;
};

#define DEFAULT_SECONDARY_COLOR 0x50FFFF // Yellow color for default

struct TSubtitleProps {
    typedef enum {
        Not_specified = -1,
        Outline = 1,
        Opaquebox = 3
    } TBorderStyle;

    TSubtitleProps() {
        reset();
    }
    TSubtitleProps(int IrefResX, int IrefResY, int IwrapStyle, int IscaleBorderAndShadow) {
        reset();
        refResX = IrefResX;
        refResY = IrefResY;
        wrapStyle = IwrapStyle;
        scaleBorderAndShadow = IscaleBorderAndShadow;
    }
    TSubtitleProps(bool Iitalic, bool Iunderline) {
        reset();
        italic = Iitalic;
        underline = Iunderline;
    }
    int bold;
    bool italic, underline, strikeout;

    int blur_be;    // \be
    double gauss; // \blur

    bool isColor;
    COLORREF color, SecondaryColour, TertiaryColour, OutlineColour, ShadowColour;
    int colorA, SecondaryColourA, TertiaryColourA, OutlineColourA, ShadowColourA;
    int refResX, refResY;
    bool isMove, isOrg, isClip;
    Ttransform transform;
    unsigned int transformT1, transformT2;
    CRect clip;
    CPoint pos, pos2; // move from pos to pos2
    CPoint org;
    unsigned int moveT1, moveT2;
    int wrapStyle; // -1 = default
    int scaleBorderAndShadow;
    double size;
    double scaleX, scaleY; //1.0 means no scaling, -1 = default
    char_t fontname[LF_FACESIZE];
    int polygon;  // {\p0/1/2/...} 0 disable, scale 2^(polygon-1)
    int encoding; // -1 = default
    int version;  // -1 = default
    int extendedTags; // 1 = default
    double spacing;  //INT_MIN = default
    double x; // Calculated x position
    double y; // Calculated y position
    int lineID;
    void reset(void);

    // Alignment. This sets how text is "justified" within the Left/Right onscreen margins,
    // and also the vertical placing. Values may be 1=Left, 2=Centered, 3=Right.
    // Add 4 to the value for a "Toptitle". Add 8 to the value for a "Midtitle".
    // eg. 5 = left-justified toptitle]
    // -1 = default(center)
    int alignment;
    bool isAlignment;

    int marginR, marginL, marginV, marginTop, marginBottom; // -1 = default
    double angleX; // 0 = default
    double angleY; // 0 = default
    double angleZ; // 0 = default
    TBorderStyle borderStyle;
    double outlineWidth, shadowDepth; // -1 = default
    int layer; // 0 = default

    bool isSSA() const; // is this SSA/ASS/ASS2?
    int get_spacing(const TprintPrefs &prefs) const;
    double get_xscale(double Ixscale, const Rational& sar, int fontSettingsOverride) const;
    double get_yscale(double Iyscale, const Rational& sar, int fontSettingsOverride) const;
    double get_maxWidth(unsigned int screenWidth, int textMarginLR, int subFormat, IffdshowBase *deci) const;
    REFERENCE_TIME get_moveStart(void) const;
    REFERENCE_TIME get_moveStop(void) const;
    static int alignASS2SSA(int align);
    int tmpFadT1, tmpFadT2;
    int isFad;
    int fadeA1, fadeA2, fadeA3;
    bool karaokeNewWord; // true if the word is top of karaoke sequence.
    REFERENCE_TIME tStart, tStop;
    REFERENCE_TIME fadeT1, fadeT2, fadeT3, fadeT4;
    REFERENCE_TIME karaokeDuration, karaokeStart;
    REFERENCE_TIME karaokeFillStart, karaokeFillEnd;
    bool isScroll() const {
        return scroll.isScroll();
    }
    enum {
        KARAOKE_NONE,
        KARAOKE_k,
        KARAOKE_kf,
        KARAOKE_ko,
        KARAOKE_ko_opaquebox  // Special mode for opaque box with ko.
    } karaokeMode;
    struct Tscroll {
        int x1, x2, y1, y2;
        int delay;
        int directionV; // Up -1, Down 1, Left,right 0
        int directionH; // Banner right to left (default) -1, left to right 1, other 0
        int fadeaway;
        Tscroll() {
            x1 = x2 = y1 = y2 = delay = directionV = directionH = fadeaway = 0;
        }
        bool isScroll() const {
            return directionV || directionH;
        }
    } scroll;
};
