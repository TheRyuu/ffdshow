#pragma once

#include "Tsubtitle.h"
#include "TsubtitleProps.h"
#include "TsubtitlesSettings.h"
#include "TSSAstyles.h"

struct TsubtitlesSettings;
struct Tconfig;
struct TsubtitleLine;
class ThtmlColors;
class TsubtitleFormat
{
public:
    struct Tword {
        size_t i1, i2;
        TSubtitleProps props;
    };
    struct Twords : std::vector<Tword> {
        void add(const wchar_t *l, const wchar_t* &l1, const wchar_t* &l2, TSubtitleProps &props, size_t step) {
            Tword word;
            word.i1 = l1 - l;
            word.i2 = l2 - l;
            word.props = props;
            push_back(word);
            l1 = (l2 += step);
            props.karaokeNewWord = false;
        }
    };
private:
    TSubtitleProps props, lineProps;
    template<int c> struct Tncasecmp {
        bool operator()(wchar_t c1) {
            return c == tolower(c1);
        }
    };
    static ffstring getAttribute(const wchar_t *start, const wchar_t *end, const wchar_t *attrname);
    struct Tssa {
    private:
        TSubtitleProps &props, &lineProps;
        const TSubtitleProps &defprops;
        const TSSAstyles &styles;
        Twords &words;
        int sfmt;
    public:

        struct TparenthesesContent {
            ffstring str;
            double doubleval;
            bool ok; // is number?
            TparenthesesContent(ffstring Istr) {
                wchar_t *bufend;
                str = Istr;
                doubleval = strtod(Istr.c_str(), &bufend);
                ok = (*bufend == 0 && bufend != Istr.c_str());
            }
        };
        typedef std::vector<TparenthesesContent> TparenthesesContents;

        struct TstoreParam {
            size_t offset;
            double min;
            double max;
            double default_value;
            size_t size; // size of actual intx_t or double
            bool isInteger; // Integer or double value
            TstoreParam(size_t Ioffset, double Imin, double Imax, double Idefault_value, size_t Isize, bool IisInteger): offset(Ioffset), min(Imin), max(Imax), default_value(Idefault_value), size(Isize), isInteger(IisInteger) {}
        };
        struct TstoreParams: public std::vector<TstoreParam> {
            // returns number of contents that have the value within the range (min...max) and have been written to.
            int writeProps(const TparenthesesContents &contents, TSubtitleProps *props);
        };

        int parse_parentheses(TparenthesesContents &contents, ffstring arg);
        Tssa(TSubtitleProps &Iprops,
             TSubtitleProps &IlineProps,
             const TSubtitleProps &Idefprops,
             const TSSAstyles &Istyles,
             Twords &Iwords,
             int Isfmt):

            props(Iprops),
            lineProps(IlineProps),
            defprops(Idefprops),
            styles(Istyles),
            words(Iwords),
            sfmt(Isfmt) {}
        typedef void (Tssa::*TssaAction)(ffstring &arg);
        typedef int (*Tstr_cmp_func)(const wchar_t *a, const wchar_t *b, size_t c);

        bool arg2int(const ffstring &arg, int min, int max, int &enc);
        bool color2int(ffstring arg, int &intval);

        // fuctions that parse tokens
        void fontName(ffstring &arg);
        //void fontSize(ffstring &arg);
        template<int TSubtitleProps::*offset, int min, int max> void intProp(ffstring &arg);
        template<int min, int max> void intPropAn(ffstring &arg);
        template<int min, int max> void intPropA(ffstring &arg);
        template<int min, int max> void intPropQ(ffstring &arg);
        template<double TSubtitleProps::*offset, int min, int max> void doubleProp(ffstring &arg);
        template<double TSubtitleProps::*offset, int min, int max> void doublePropDiv100(ffstring &arg);
        template<int TSubtitleProps::*offset1, int TSubtitleProps::*offset2, int min, int max> bool intProp2(ffstring &arg);
        void clip(ffstring &arg);
        void pos(ffstring &arg);
        void move(ffstring &arg);
        void org(ffstring &arg);
        void transform(ffstring &arg);
        void fad(ffstring &arg);
        void fade(ffstring &arg);
        void karaoke_kf(ffstring &arg);
        void karaoke_ko(ffstring &arg);
        void karaoke_k(ffstring &arg);
        void karaoke_fixProperties();
        template<bool TSubtitleProps::*offset> void boolProp(ffstring &arg);
        template<COLORREF TSubtitleProps::*offset> void color(ffstring &arg);
        template<int TSubtitleProps::*offset> void alpha(ffstring &arg);
        void alphaAll(ffstring &arg);
        void reset(ffstring &arg);

        bool processToken(const wchar_t* &l2, const wchar_t *tok, TssaAction action);
        bool processTokenC(const wchar_t* &l2, const wchar_t *tok, TssaAction action);
        bool processTokenI(const wchar_t* &l2, const wchar_t *tok, TssaAction action, Tstr_cmp_func str_cmp_func);
        void processTokens(const wchar_t *l, const wchar_t* &l1, const wchar_t* &l2, const wchar_t *end);
    };
    const ThtmlColors *htmlcolors;
public:
    TsubtitleFormat(const ThtmlColors *Ihtmlcolors): htmlcolors(Ihtmlcolors) {}
    void processHTMLTags(Twords &words, const wchar_t* &l, const wchar_t* &l1, const wchar_t* &l2);
    const TSubtitleProps& processHTML(Twords &words, const TsubtitleLine &line);
    const TSubtitleProps& processSSA(Twords &words, const TsubtitleLine &line, int sfmt, TsubtitleText &parent);
    void processMicroDVD(TsubtitleText &parent, std::vector< TsubtitleLine >::iterator it);
    void processMPL2(TsubtitleLine &line);
    void resetProps(void) {
        props.reset();
    }
    void setProps(const TSubtitleProps &p) {
        props = p;
        lineProps = p;
    }
    const TSubtitleProps& get_lineProps() const {
        return lineProps;
    }
};
