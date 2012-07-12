#pragma once

#include "Tsubtitle.h"
#include "TsubtitleProps.h"
#include "TsubtitlesSettings.h"
#include "TSSAstyles.h"
#include "TsubtitleFormat.h"

struct TsubtitleWord {
private:
    ffstring text, fixed;
    bool useFixed;
    const ffstring& getText(void) const {
        return useFixed ? fixed : text;
    }
    ffstring& getText(void) {
        return useFixed ? fixed : text;
    }
public:
    TSubtitleProps props;

    TsubtitleWord(const ffstring &Itext): text(Itext), useFixed(false) {}
    TsubtitleWord(const ffstring &Itext, const TSubtitleProps &defProps): text(Itext), useFixed(false), props(defProps) {}

    TsubtitleWord(const wchar_t *Itext): text(Itext), useFixed(false) {}
    TsubtitleWord(const wchar_t *Itext, const TSubtitleProps &defProps): text(Itext), useFixed(false), props(defProps) {}

    TsubtitleWord(const wchar_t *s, size_t len): text(s, len), useFixed(false) {}
    TsubtitleWord(const wchar_t *s, size_t len, const TSubtitleProps &defProps): text(s, len), useFixed(false), props(defProps) {}

    void set(const ffstring &s) {
        getText() = s;
    }

    // New code should use getString, as the cast makes readability poor.
    operator const wchar_t *(void) const {
        return getString();
    }
    const wchar_t* getString() const {
        return getText().c_str();
    }

    size_t size() const {
        return getText().size();
    }
    void eraseLeft(size_t num) {
        getText().erase(0, num);
    }
    void addTailSpace(void) {
        text += L" ";
    }
};

struct TsubtitleLine :
        std::vector< TsubtitleWord > {
private:
    typedef std::vector<TsubtitleWord> Tbase;
    void applyWords(const TsubtitleFormat::Twords &words, int subFormat);
public:
    TSubtitleProps props;
    int lineBreakReason; // 0: none, 1: \n, 2: \N
    TsubtitleLine(void) {}
    TsubtitleLine(const ffstring &Itext) {
        push_back(Itext);
    }
    TsubtitleLine(const ffstring &Itext, const TSubtitleProps &defProps) {
        push_back(TsubtitleWord(Itext, defProps));
    }

    TsubtitleLine(const wchar_t *Itext) {
        push_back(Itext);
    }
    TsubtitleLine(const wchar_t *Itext, const TSubtitleProps &defProps) {
        push_back(TsubtitleWord(Itext, defProps));
    }
    TsubtitleLine(const wchar_t *Itext, const TSubtitleProps &defProps, int IlineBreakReason): lineBreakReason(IlineBreakReason) {
        push_back(TsubtitleWord(Itext, defProps));
    }

    TsubtitleLine(const wchar_t *s, size_t len) {
        push_back(TsubtitleWord(s, len));
    }
    TsubtitleLine(const wchar_t *s, size_t len, const TSubtitleProps &defProps) {
        push_back(TsubtitleWord(s, len, defProps));
    }
    TsubtitleLine(const wchar_t *s, size_t len, const TSubtitleProps &defProps, int IlineBreakReason): lineBreakReason(IlineBreakReason) {
        push_back(TsubtitleWord(s, len, defProps));
    }
    size_t strlen(void) const;
    void format(TsubtitleFormat &format, int sfmt, TsubtitleText &parent);
    bool checkTrailingSpaceRight(const_iterator w) const;
};

struct TsubtitleText : public Tsubtitle, public std::vector< TsubtitleLine > {
private:
    typedef std::vector<TsubtitleLine> Tbase;

    TrenderedSubtitleLines lines;
    bool rendering_ready;
    TSSAstyles styles;

    TprintPrefs old_prefs;
    boost::mutex mutex_lines;
    typedef std::list<TrenderedTextSubtitleWord *> TrenderedPolygons;

    // call newWord or get from renderedPolygons
    TrenderedTextSubtitleWord* getRenderedWord(
        const wchar_t *s,
        size_t slen,
        const TprintPrefs &prefs,
        const TsubtitleWord *w,
        const LOGFONT &lf,
        const Tfont &font,
        bool trimLeftSpaces,
        bool trimRightSpaces,
        TrenderedPolygons &renderedPolygons);

public:
    friend class Tfont;
    int subformat;
    TSubtitleProps defProps;
    TsubtitleText(const TsubtitleText &src);
    TsubtitleText(int Isubformat): subformat(Isubformat), rendering_ready(false) {}
    TsubtitleText(int Isubformat, const TSubtitleProps &IdefProps):
        subformat(Isubformat)
        , defProps(IdefProps)
        , rendering_ready(false) {}
    TsubtitleText(int Isubformat, const TSubtitleProps &IdefProps, const TSSAstyles &Istyles):
        subformat(Isubformat)
        , defProps(IdefProps)
        , rendering_ready(false)
        , styles(Istyles) {}
    virtual ~TsubtitleText() {
        dropRenderedLines();
    }

    const TSSAstyles& getStyles() {
        return styles;
    }
    void set(const strings &strs) {
        this->clear();
        for (strings::const_iterator s = strs.begin(); s != strs.end(); s++) {
            this->push_back(TsubtitleLine(*s, defProps));
        }
    }

    void set(const ffstring &str) {
        if (this->size() == 1) {
            this->at(0) = str;
        } else {
            this->clear();
            this->push_back(TsubtitleLine(str, defProps));
        }
    }

    void add(const wchar_t *s) {
        this->push_back(TsubtitleLine(s, defProps));
    }

    void add(const wchar_t *s, size_t len) {
        this->push_back(TsubtitleLine(s, len, defProps));
    }

    void addSSA(const wchar_t *s, int lineBreakReason) {
        this->push_back(TsubtitleLine(s, defProps, lineBreakReason));
    }

    void addSSA(const wchar_t *s, size_t len, int lineBreakReason) {
        this->push_back(TsubtitleLine(s, len, defProps, lineBreakReason));
    }

    virtual void addEmpty(void) {
        this->push_back(TsubtitleLine(L" ", defProps));
    }

    void format(TsubtitleFormat &format);

    void fixFade(const TSubtitleProps &lineProps);

    template<class Tval> void propagateProps(Tbase::iterator it, Tval TSubtitleProps::*offset, Tval val, Tbase::iterator itend) {
        for (; it != itend; it++)
            foreach(TsubtitleWord & word, *it)
            word.props.*offset = val;
    }

    template<class Tval> void propagateProps(Tbase::iterator it, Tval TSubtitleProps::*offset, Tval val) {
        propagateProps(it, offset, val, this->end());
    }

    template<class Tval> void propagateProps(Tval TSubtitleProps::*offset, Tval val) {
        propagateProps(this->begin(), offset, val, this->end());
    }

    virtual void print(
        REFERENCE_TIME time,
        bool wasseek,
        Tfont &f,
        bool forceChange,
        TprintPrefs &prefs,
        unsigned char **dst,
        const stride_t *stride);

    virtual size_t numlines(void) const {
        return this->size();
    }

    virtual size_t numchars(void) const {
        size_t c = 0;
        for (Tbase::const_iterator l = this->begin(); l != this->end(); l++) {
            c += l->strlen();
        }
        return c;
    }

    virtual bool isText(void) const {
        return true;
    }

    // return used memory
    size_t prepareGlyph(TprintPrefs prefs, Tfont &font, bool forceChange);

    double get_splitdx_for_new_line(const TsubtitleWord &w, int dx, const TprintPrefs &prefs, IffdshowBase *deci) const {
        // This method calculates the maximum length of the line considering the left/right margin and eventually
        // basing on the position set through a position tag
        return w.props.get_maxWidth(dx, prefs.textMarginLR, prefs.subformat, deci) * prefs.fontSettings.gdi_font_scale;
    }

    // return size of released memory
    virtual size_t getRenderedMemorySize() const {
        return lines.getMemorySize();
    }

    TrenderedTextSubtitleWord* TsubtitleText::newWord(
        const wchar_t *s,
        size_t slen,
        TprintPrefs prefs,
        const TsubtitleWord *w,
        const LOGFONT &lf,
        const Tfont &font,
        bool trimLeftSpaces,
        bool trimRightSpaces);

    size_t dropRenderedLines(void);

    virtual void clear() {
        dropRenderedLines();
        erase(begin(), end());
    }

    bool is_rendering_ready() {
        return rendering_ready;
    }

    boost::mutex* get_lock_ptr() {
        return &mutex_lines;
    }
};

struct TsubtitleTexts :
    public Tsubtitle,
    public std::vector< TsubtitleText* > {
    virtual bool isText(void) const {
        return true;
    }
    virtual void print(
        REFERENCE_TIME time,
        bool wasseek,
        Tfont &f,
        bool forceChange,
        TprintPrefs &prefs,
        unsigned char **dst,
        const stride_t *stride);
};
