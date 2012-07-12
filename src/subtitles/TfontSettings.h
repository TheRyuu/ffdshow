#pragma once

#include "Toptions.h"

struct TfontSettings : Toptions {
public:
    typedef enum {
        Softest = 0,
        Softer = 1,
        Soft = 2,
        Normal = 3,
        Strong = 4,
        Stronger = 5, // ASS \be
        Extreme = 6
    } TblurStrength;

    typedef enum {
        ShadowOSD = -1,
        GlowingShadow = 0,
        GradientShadow = 1,
        ClassicShadow = 2, // SSA/ASS always use this.
        ShadowDisabled = 3
    } TshadowMode;

    TfontSettings(TintStrColl *Icoll = NULL);
    TfontSettings& operator =(const TfontSettings &src) {
        memcpy(((uint8_t*)this) + sizeof(Toptions), ((uint8_t*)&src) + sizeof(Toptions), sizeof(*this) - sizeof(Toptions));
        return *this;
    }
    virtual void reg_op(TregOp &t);
    unsigned int getSize(int dx, int dy) const {
        if (autosize && dx && dy) {
            return limit(sizeA * ff_sqrt(dx * dx + dy * dy) / 1000, 3U, 255U);
        } else {
            return sizeP;
        }
    }
    bool getTip(char_t *buf, size_t len) {
        tsnprintf_s(buf, len, _TRUNCATE, _l("Font: %s, %s charset, %ssize:%i, %s, spacing:%i\noutline width:%i"), name, getCharset(charset), autosize ? _l("auto") : _l(""), autosize ? sizeA : sizeP, weights[weight / 100 - 1].name, spacing, outlineWidth);
        return true;
    }
    struct Tweigth {
        const char_t *name;
        int id;
    };
    static const Tweigth weights[];
    static const int charsets[];
    static const char_t *getCharset(int i);
    static int getCharset(const char_t *name);
    static int GDI_charset_to_code_page(int charset);
    static const char_t *shadowModes[];
    static const char_t *blurModes[];

    char_t name[260];
    int fontSettingsOverride;
    int charset;
    int autosize, autosizeVideoWindow;
    int sizeP, sizeA, sizeOverride;
    int xscale, yscale;
    int spacing, weight;
    int opaqueBox;
    int italic;
    int underline;
    int color, outlineColor, shadowColor, colorOverride;
    int bodyAlpha, outlineAlpha, shadowAlpha;
    int split; // Check box of "Split long subtitle lines"
    int outlineWidth, outlineWidthOverride;
    int shadowSize, shadowOverride; // Subtitles shadow
    TshadowMode shadowMode;
    int blur;
    TblurStrength blurStrength;
    int scaleBorderAndShadowOverride;
    int hqBorder;
    int memory; // in mega bytes
    /**
     * gdi_font_scale: 4: for OSD.
     *                 64: for subtitles.
     */
    int gdi_font_scale;
    bool operator == (const TfontSettings &rt) const;
    bool operator != (const TfontSettings &rt) const;
protected:
    virtual void getDefaultStr(int id, char_t *buf, size_t buflen);
};

struct TfontSettingsOSD : TfontSettings {
    TfontSettingsOSD(TintStrColl *Icoll = NULL);
};

struct TfontSettingsSub : TfontSettings {
    TfontSettingsSub(TintStrColl *Icoll = NULL);
    virtual void reg_op(TregOp &t);
};
