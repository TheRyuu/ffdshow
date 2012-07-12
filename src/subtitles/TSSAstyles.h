#pragma once
#include "TsubtitleProps.h"

struct TSSAstyle {
private:
    int version;
public:
    TSSAstyle(int playResX, int playResY, int version, int wrapStyle, int scaleBorderAndShadow): props(playResX, playResY, wrapStyle, scaleBorderAndShadow) {
        this->version = version;
        props.version = version;
    }
    ffstring name, fontname, fontsize, primaryColour, bold, italic, underline, strikeout, encoding, spacing, fontScaleX, fontScaleY;
    ffstring secondaryColour, tertiaryColour, outlineColour, backgroundColour, alignment;
    ffstring angleZ, borderStyle, outlineWidth, shadowDepth, marginLeft, marginRight, marginV, marginTop, marginBottom, alpha, relativeTo, layer;
    TSubtitleProps props;
    void toProps(void);
    bool toCOLORREF(const ffstring &colourStr, COLORREF &colour, int &alpha);
};

struct TSSAstyles : std::map<ffstring, TSSAstyle, ffstring_iless> {
    const TSubtitleProps* getProps(const ffstring &style) const;
    void add(TSSAstyle &style);
};
