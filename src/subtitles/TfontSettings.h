#ifndef _TFONTSETTINGS_H_
#define _TFONTSETTINGS_H_

#include "Toptions.h"

struct TfontSettings :Toptions {
public:
    TfontSettings(TintStrColl *Icoll=NULL);
    TfontSettings& operator =(const TfontSettings &src) {
        memcpy(((uint8_t*)this)+sizeof(Toptions),((uint8_t*)&src)+sizeof(Toptions),sizeof(*this)-sizeof(Toptions));
        return *this;
    }
    virtual void reg_op(TregOp &t);
    unsigned int getSize(unsigned int dx,unsigned int dy) const {
        if (autosize && dx && dy) {
            return limit(sizeA*ff_sqrt(dx*dx+dy*dy)/1000,3U,255U);
        } else {
            return sizeP;
        }
    }
    bool getTip(char_t *buf,size_t len) {
        tsnprintf_s(buf, len, _TRUNCATE, _l("Font: %s, %s charset, %ssize:%i, %s, spacing:%i\noutline width:%i"),name,getCharset(charset),autosize?_l("auto"):_l(""),autosize?sizeA:sizeP,weights[weight/100-1].name,spacing,outlineWidth);
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
    int autosize,autosizeVideoWindow;
    int sizeP,sizeA,sizeOverride;
    int xscale,yscale;
    int spacing,weight;
    int opaqueBox;
    int italic;
    int underline;
    int color,outlineColor,shadowColor,colorOverride;
    int bodyAlpha,outlineAlpha,shadowAlpha;
    int split;
    int aspectAuto;
    int outlineWidth, outlineWidthOverride;
    int shadowSize, shadowMode, shadowOverride; // Subtitles shadow
    int blur, blurMode;
    int scaleBorderAndShadowOverride;
    /**
     * gdi_font_scale: 4: for OSD. rendering_window is 4x5.
     *                 8-16: for subtitles. 16:very sharp (slow), 12:soft & sharp, (moderately slow) 8:blurry (fast)
     */
    int gdi_font_scale;
    bool operator == (const TfontSettings &rt) const;
    bool operator != (const TfontSettings &rt) const;
protected:
    virtual void getDefaultStr(int id,char_t *buf,size_t buflen);
};

struct TfontSettingsOSD :TfontSettings {
    TfontSettingsOSD(TintStrColl *Icoll=NULL);
};

struct TfontSettingsSub :TfontSettings {
    TfontSettingsSub(TintStrColl *Icoll=NULL);
    virtual void reg_op(TregOp &t);
};

#endif
