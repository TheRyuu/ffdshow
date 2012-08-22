#pragma once

#include "interfaces.h"
#include "ffImgfmt.h"
#include "TSubtitleMixedProps.h"
#include "rational.h"
#include "TfontSettings.h"
#include "CRect.h"

#define size_of_rgb32 4

enum {
    ALIGN_FFDSHOW = 0,
    ALIGN_LEFT = 1,
    ALIGN_CENTER = 2,
    ALIGN_RIGHT = 3
};

class TrenderedSubtitleLine;
class TfontManager;
struct Tconfig;

struct TrotateParam {
    CPoint axis;
    CPoint before;
};

struct TprintPrefs {
    TprintPrefs(IffdshowBase *Ideci, const TfontSettings *IfontSettings);

    TprintPrefs() {
        memset(this, 0, sizeof(*this));
        memset(&fontSettings, 0, sizeof(fontSettings));
    }

    bool operator != (const TprintPrefs &rt) const;
    bool operator == (const TprintPrefs &rt) const;

    void operator = (const TprintPrefs &rt) {
        memcpy(this, &rt, sizeof(*this));
    }

    unsigned int dx, dy, clipdy;
    bool isOSD;
    int xpos, ypos;
    int align;
    int linespacing;
    unsigned int sizeDx, sizeDy;
    int stereoScopicParallax;
    bool vobchangeposition;
    int subimgscale, vobaamode, vobaagauss;
    bool OSDitemSplit;
    int textMarginLR;
    int tabsize;
    bool dvd;
    TfontSettings::TshadowMode shadowMode;
    int shadowAlpha; // Subtitles shadow
    double shadowSize;
    bool outlineBlur;
    TfontSettings::TblurStrength blurStrength;
    uint64_t csp;
    double outlineWidth;
    Rational sar;
    bool opaqueBox;
    int italic;
    int underline;
    int subformat;

    // xinput,yinput
    // decoded resolution or 384,288 depending on the settings of
    // "Use movie demensions instead of ASS script information".
    // Note that this works for SRT too.
    unsigned int xinput, yinput;

    TfontSettings fontSettings;
    YUVcolorA yuvcolor;  // body
    YUVcolorA outlineYUV, shadowYUV;

    // members that are not compared by operator == and !=
    REFERENCE_TIME rtStart;
    IffdshowBase *deci;
    const Tconfig *config;
};

class TrenderedSubtitleLines: public std::vector<TrenderedSubtitleLine*>
{
public:
    TrenderedSubtitleLines() {}
    TrenderedSubtitleLines(TrenderedSubtitleLine *ln) {
        push_back(ln);
    }

    /**
     * reset
     *  just clear pointers, do not delete objects.
     */
    void reset() {
        erase(begin(), end());
    }

    /**
     * clear
     *  clear pointers and delete objects.
     */
    void clear();

    using std::vector<value_type>::empty;

    void print(
        const TprintPrefs &prefs,
        unsigned char **dst,
        const stride_t *stride);

    size_t getMemorySize() const;

private:
    void printASS(
        const TprintPrefs &prefs,
        unsigned char **dst,
        const stride_t *stride);

    class ParagraphKey
    {
    public:
        int alignment;
        double marginTop, marginBottom;
        double marginL, marginR;
        int isMove, isScroll;
        CPoint pos;
        int layer;
        bool hasPrintedRect;
        CRectDouble printedRect;
        int lineID;

        ParagraphKey(TrenderedSubtitleLine *line);
        bool operator != (const ParagraphKey &rt) const;
        bool operator == (const ParagraphKey &rt) const;
        bool operator < (const ParagraphKey &rt) const;
    };

    class ParagraphValue
    {
        double maxr, maxl;
    public:
        double width, height, y;
        double xmin, xmax, y0, xoffset, yoffset;
        CRectDouble overhang;
        bool firstuse;
        CRectDouble myrect;

        ParagraphValue():
            width(0),
            height(0),
            y(0),
            xmin(-1),
            xmax(-1),
            y0(0),
            xoffset(0),
            yoffset(0),
            maxr(0),
            maxl(0),
            firstuse(true)
        {};
        void processLine(TrenderedSubtitleLine *line, int alignment);
    };
    class TlayerSort
    {
    public:
        bool operator()(TrenderedSubtitleLine *lt, TrenderedSubtitleLine *rt) const;
    };

    void handleCollision(TrenderedSubtitleLine *line, int x, ParagraphValue &pval, unsigned int prefsdy, int alignment);
};

class TrenderedSubtitleWordBase
{
private:
    bool own;
public:
    TrenderedSubtitleWordBase(bool Iown):
        own(Iown),
        dxChar(0),
        dyChar(0) {
        for (int i = 0; i < 3; i++) {
            bmp[i] = NULL;
            msk[i] = NULL;
            outline[i] = NULL;
            shadow[i] = NULL;
            dx[i] = 0;
            dy[i] = 0;
        }
    }
    virtual ~TrenderedSubtitleWordBase();
    unsigned int dx[3], dy[3];
    double dxChar, dyChar;
    unsigned char *bmp[3], *msk[3];
    stride_t bmpmskstride[3];
    unsigned char *outline[3], *shadow[3];
    virtual void print(int startx, int starty, unsigned int dx[3], int dy[3], unsigned char *dstLn[3], const stride_t stride[3], const unsigned char *bmp[3], const unsigned char *msk[3], REFERENCE_TIME rtStart = REFTIME_INVALID) const = 0;
    uint64_t csp;
    virtual double get_ascent() const {
        return dy[0];
    }
    virtual double get_descent() const {
        return 0;
    }
    virtual double get_baseline() const {
        return dy[0];
    }
    virtual size_t getMemorySize() const {
        return 0;
    }
};

class TrenderedVobsubWord : public TrenderedSubtitleWordBase
{
private:
    bool shiftChroma;
public:
    TrenderedVobsubWord(bool IshiftChroma=true):shiftChroma(IshiftChroma),TrenderedSubtitleWordBase(false) {}
    virtual void print(int startx, int starty, unsigned int dx[3],int dy[3],unsigned char *dstLn[3],const stride_t stride[3],const unsigned char *bmp[3],const unsigned char *msk[3],REFERENCE_TIME rtStart=REFTIME_INVALID) const;
};

class TrenderedTextSubtitleWord;

class TrenderedSubtitleLine : public std::vector<TrenderedSubtitleWordBase*>
{
    bool firstrun;
    double emptyHeight; // This is used as charHeight if empty.
    bool hasPrintedRect, hasParagraphRect;
    CRectDouble printedRect, paragraphRect;
public:
    TSubtitleMixedProps mprops;

    TrenderedSubtitleLine():
        firstrun(true),
        hasPrintedRect(false),
        hasParagraphRect(false) {
        mprops.reset();
    }
    TrenderedSubtitleLine(const TSubtitleProps &p, const TprintPrefs &prefs):
        firstrun(true),
        hasPrintedRect(false),
        hasParagraphRect(false),
        mprops(p, prefs) {
    }

    TrenderedSubtitleLine(const TSubtitleProps &p, const TprintPrefs &prefs, double IemptyHeight):
        firstrun(true),
        hasPrintedRect(false),
        hasParagraphRect(false),
        mprops(p, prefs),
        emptyHeight(IemptyHeight) {
    }

    TrenderedSubtitleLine(TrenderedSubtitleWordBase *w):
        firstrun(true),
        hasPrintedRect(false),
        hasParagraphRect(false) {
        push_back(w);
        mprops.reset();
    }

    const TSubtitleMixedProps& getProps() const;

    const CRectDouble& getPrintedRect() const {
        return printedRect;
    }
    bool getHasPrintedRect() const {
        return hasPrintedRect;
    }
    bool checkCollision(const CRectDouble &query, CRectDouble &ans);

    double width() const;
    unsigned int height() const;
    double lineHeight() const;
    double getTopOverhang() const;
    double getBottomOverhang() const;
    double getLeftOverhang() const;
    double getRightOverhang() const;
    double baselineHeight() const;
    using std::vector<value_type>::push_back;
    using std::vector<value_type>::empty;
    void clear();
    void print(
        double startx,
        double starty,
        const TprintPrefs &prefs,
        unsigned int prefsdx,
        unsigned int prefsdy,
        unsigned char **dst,
        const stride_t *stride);

    void setParagraphRect(CRectDouble &IparagraphRect);

    size_t getMemorySize() const;
};

struct TfontSettings;
struct Tsubtitle;
struct TfontSettings;
struct TsubtitleText;
class Tfont
{
private:
    IffdshowBase *deci;
    TfontManager *fontManager;
    HDC hdc;
    HGDIOBJ oldFont;
    TrenderedSubtitleLines lines;
    unsigned int height;
    uint64_t oldCsp;
    void prepareC(TsubtitleText *sub, const TprintPrefs &prefs, bool forceChange);
public:
    friend struct TsubtitleText;
    //TfontSettings *fontSettings;
    Tfont(IffdshowBase *Ideci);
    ~Tfont();
    void init();
    /**
     * print (for OSD)
     * @return height
     */
    int print(
        TsubtitleText *sub,
        bool forceChange,
        const TprintPrefs &prefs,
        unsigned char **dst,
        const stride_t *stride);
    /**
     * printf(for subtitles)
     * lines must be filled before called
     */
    void print(
        const TprintPrefs &prefs,
        unsigned char **dst,
        const stride_t *stride);
    void reset() {
        lines.reset();
    }
    void done();
};
