#ifndef _TSUBTITLETEXT_H_
#define _TSUBTITLETEXT_H_

#include "Tsubtitle.h"
#include "TsubtitleProps.h"
#include "TsubtitlesSettings.h"

struct TsubtitlesSettings;
struct Tconfig;
struct TsubtitleText;
class TtextFixBase
{
public:
 static strings getDicts(const Tconfig *cfg);
 enum
  {
   fixAP=1,
   fixPunctuation=2,
   fixNumbers=4,
   fixCapital=8,
   fixCapital2=16,
   fixIl=32,
   fixOrtography=64,
   fixHearingImpaired=128
  };
};

class TtextFix : public TtextFixBase
{
private:
 TsubtitlesSettings cfg;
 bool EndOfPrevSentence,inHearing;
 strings odict;
 static inline bool in(wchar_t x,const wchar_t *s) {return strchr(s,x)!=NULL;}
public:
 TtextFix(const TsubtitlesSettings *Icfg,const Tconfig *ffcfg);
 bool process(ffstring &text,ffstring &fixed);
};

struct TsubtitleLine;
class ThtmlColors;
class TsubtitleFormat
{
public:
 struct Tword
  {
   size_t i1,i2;
   TSubtitleProps props;
  };
 struct Twords : std::vector<Tword>
  {
   void add(const wchar_t *l,const wchar_t* &l1,const wchar_t* &l2, TSubtitleProps &props,size_t step)
    {
     Tword word;word.i1=l1-l;word.i2=l2-l;word.props=props;
     push_back(word);
     l1=(l2+=step);
     props.karaokeNewWord = false;
    }
  };
private:
 TSubtitleProps props;
 template<int c> struct Tncasecmp
  {
   bool operator ()(wchar_t c1)
    {
     return c==tolower(c1);
    }
  };
 static ffstring getAttribute(const wchar_t *start,const wchar_t *end,const wchar_t *attrname);
 struct Tssa
  {
  private:
   TSubtitleProps &props;
   const TSubtitleProps &defprops;
   Twords &words;
  public:

   struct TparenthesesContent {
       ffstring str;
       int64_t intval;
       bool ok; // is number?
       TparenthesesContent(ffstring Istr) {
           wchar_t *bufend;
           str = Istr;
           intval = _strtoi64(Istr.c_str(), &bufend, 10);
           ok = (*bufend == 0 && bufend != Istr.c_str());
       }
   };
   typedef std::vector<TparenthesesContent> TparenthesesContents;

   struct TstoreParam {
       size_t offset;
       int64_t min;
       int64_t max;
       int64_t default_value;
       size_t size; // sizeof actual intx_t
       TstoreParam(size_t Ioffset, int64_t Imin, int64_t Imax, int64_t Idefault_value, size_t Isize):offset(Ioffset),min(Imin),max(Imax),default_value(Idefault_value),size(Isize) {}
   };
   struct TstoreParams: public std::vector<TstoreParam> {
       // returns number of contents that have the value within the range (min...max) and have been written to. 
       int writeProps(const TparenthesesContents &contents, TSubtitleProps *props);
   };

   int parse_parentheses(TparenthesesContents &contents, ffstring arg);
   Tssa(TSubtitleProps &Iprops,const TSubtitleProps &Idefprops,Twords &Iwords):props(Iprops),defprops(Idefprops),words(Iwords) {}
   typedef void (Tssa::*TssaAction)(ffstring &arg);
   typedef int (*Tstr_cmp_func)(const wchar_t *a, const wchar_t *b, size_t c);

   bool arg2int(const ffstring &arg, int min, int max, int &enc);
   bool color2int(ffstring arg, int &intval);

   // fuctions that parse tokens
   void fontName(ffstring &arg);
   //void fontSize(ffstring &arg);
   template<int TSubtitleProps::*offset,int min,int max> void intProp(ffstring &arg);
   template<int TSubtitleProps::*offset,int min,int max> void intPropAn(ffstring &arg);
   template<double TSubtitleProps::*offset,int min,int max> void doubleProp(ffstring &arg);
   template<int TSubtitleProps::*offset1,int TSubtitleProps::*offset2,int min,int max> bool intProp2(ffstring &arg);
   void pos(ffstring &arg);
   void move(ffstring &arg);
   void org(ffstring &arg);
   void fad(ffstring &arg);
   void fade(ffstring &arg);
   void karaoke_kf(ffstring &arg);
   void karaoke_ko(ffstring &arg);
   void karaoke_k(ffstring &arg);
   template<bool TSubtitleProps::*offset> void boolProp(ffstring &arg);
   template<COLORREF TSubtitleProps::*offset> void color(ffstring &arg);
   template<int TSubtitleProps::*offset> void alpha(ffstring &arg);
   void alphaAll(ffstring &arg);
   void reset(ffstring &arg);

   bool processToken(const wchar_t* &l2,const wchar_t *tok,TssaAction action);
   bool processTokenC(const wchar_t* &l2,const wchar_t *tok,TssaAction action);
   bool processTokenI(const wchar_t* &l2,const wchar_t *tok,TssaAction action,Tstr_cmp_func str_cmp_func);
   void processTokens(const wchar_t *l,const wchar_t* &l1,const wchar_t* &l2,const wchar_t *end);
  };
 const ThtmlColors *htmlcolors;
public:
 TsubtitleFormat(const ThtmlColors *Ihtmlcolors):htmlcolors(Ihtmlcolors) {}
 void processHTMLTags(Twords &words, const wchar_t* &l, const wchar_t* &l1, const wchar_t* &l2);
 Twords processHTML(const TsubtitleLine &line);
 Twords processSSA(const TsubtitleLine &line,int sfmt, TsubtitleText &parent);
 void processMicroDVD(TsubtitleText &parent, std::vector< TsubtitleLine >::iterator it);
 void processMPL2(TsubtitleLine &line);
 void resetProps(void){props.reset();}
};

struct TsubtitleWord
{
private:
 ffstring text,fixed;
 bool useFixed;
 const ffstring& getText(void) const {return useFixed?fixed:text;}
 ffstring& getText(void) {return useFixed?fixed:text;}
public:
 TSubtitleProps props;

 TsubtitleWord(const ffstring &Itext):text(Itext),useFixed(false) {}
 TsubtitleWord(const ffstring &Itext,const TSubtitleProps &defProps):text(Itext),useFixed(false),props(defProps) {}

 TsubtitleWord(const wchar_t *Itext):text(Itext),useFixed(false) {}
 TsubtitleWord(const wchar_t *Itext,const TSubtitleProps &defProps):text(Itext),useFixed(false),props(defProps) {}

 TsubtitleWord(const wchar_t *s,size_t len):text(s,len),useFixed(false) {}
 TsubtitleWord(const wchar_t *s,size_t len,const TSubtitleProps &defProps):text(s,len),useFixed(false),props(defProps) {}

 void set(const ffstring &s)
  {
   getText()=s;
  }
 operator const wchar_t *(void) const
  {
   return getText().c_str();
  }
 void fix(TtextFix &fix)
  {
   useFixed=fix.process(text,fixed);
  }
 size_t size(void) const {return getText().size();}
 void eraseLeft(size_t num)
  {
   getText().erase(0,num);
  }
 void addTailSpace(void)
  {
   text += L" ";
  }
};

struct TsubtitleLine :
    std::vector< TsubtitleWord >
{
private:
 typedef std::vector<TsubtitleWord> Tbase;
 void applyWords(const TsubtitleFormat::Twords &words);
public:
 TSubtitleProps props;
 int lineBreakReason; // 0: none, 1: \n, 2: \N
 TsubtitleLine(void) {}
 TsubtitleLine(const ffstring &Itext) {push_back(Itext);}
 TsubtitleLine(const ffstring &Itext,const TSubtitleProps &defProps) {push_back(TsubtitleWord(Itext,defProps));}

 TsubtitleLine(const wchar_t *Itext) {push_back(Itext);}
 TsubtitleLine(const wchar_t *Itext,const TSubtitleProps &defProps) {push_back(TsubtitleWord(Itext,defProps));}
 TsubtitleLine(const wchar_t *Itext,const TSubtitleProps &defProps,int IlineBreakReason):lineBreakReason(IlineBreakReason) {push_back(TsubtitleWord(Itext,defProps));}

 TsubtitleLine(const wchar_t *s,size_t len) {push_back(TsubtitleWord(s,len));}
 TsubtitleLine(const wchar_t *s,size_t len,const TSubtitleProps &defProps) {push_back(TsubtitleWord(s,len,defProps));}
 TsubtitleLine(const wchar_t *s,size_t len,const TSubtitleProps &defProps,int IlineBreakReason):lineBreakReason(IlineBreakReason) {push_back(TsubtitleWord(s,len,defProps));}
 size_t strlen(void) const;
 void format(TsubtitleFormat &format,int sfmt,TsubtitleText &parent);
 void fix(TtextFix &fix);
};

struct TsubtitleText :public Tsubtitle,public std::vector< TsubtitleLine >
{
private:
    typedef std::vector<TsubtitleLine> Tbase;

    TrenderedSubtitleLines lines;
    bool rendering_ready;

    TprintPrefs old_prefs;
    boost::mutex mutex_lines;
public:
    friend class Tfont;
    int subformat;
    TSubtitleProps defProps;
    TsubtitleText(const TsubtitleText &src);
    TsubtitleText(int Isubformat):subformat(Isubformat),rendering_ready(false) {}
    TsubtitleText(int Isubformat,const TSubtitleProps &IdefProps):subformat(Isubformat),defProps(IdefProps),rendering_ready(false) {}
    virtual ~TsubtitleText() {
        dropRenderedLines();
    }

    void set(const strings &strs) {
        this->clear();
        for (strings::const_iterator s=strs.begin();s!=strs.end();s++)
            this->push_back(TsubtitleLine(*s,defProps));
    }

    void set(const ffstring &str) {
        if (this->size()==1)
            this->at(0)=str;
        else {
            this->clear();
            this->push_back(TsubtitleLine(str,defProps));
        }
    }

    void add(const wchar_t *s) {
        this->push_back(TsubtitleLine(s,defProps));
    }

    void add(const wchar_t *s,size_t len) {
        this->push_back(TsubtitleLine(s,len,defProps));
    }

    void addSSA(const wchar_t *s, int lineBreakReason) {
        this->push_back(TsubtitleLine(s, defProps, lineBreakReason));
    }

    void addSSA(const wchar_t *s, size_t len, int lineBreakReason) {
        this->push_back(TsubtitleLine(s, len, defProps, lineBreakReason));
    }

    virtual void addEmpty(void) {
        this->push_back(TsubtitleLine(L" ",defProps));
    }

    void format(TsubtitleFormat &format);

    void prepareKaraoke(void);

    template<class Tval> void propagateProps(Tbase::iterator it,Tval TSubtitleProps::*offset,Tval val,Tbase::iterator itend) {
        for (;it!=itend;it++)
            foreach (TsubtitleWord &word, *it)
                word.props.*offset=val;
    }

    template<class Tval> void propagateProps(Tbase::iterator it,Tval TSubtitleProps::*offset,Tval val) {
        propagateProps(it,offset,val,this->end());
    }

    template<class Tval> void propagateProps(Tval TSubtitleProps::*offset,Tval val) {
        propagateProps(this->begin(),offset,val,this->end());
    }

    void fix(TtextFix &fix);

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
        size_t c=0;
        for (Tbase::const_iterator l=this->begin();l!=this->end();l++)
         c+=l->strlen();
        return c;
    }

    virtual bool isText(void) const {return true;}

    // return used memory
    size_t prepareGlyph(const TprintPrefs &prefs, Tfont &font,bool forceChange);

    int get_splitdx_for_new_line(const TsubtitleWord &w,int splitdx,int dx, const TprintPrefs &prefs, int gdi_font_scale, IffdshowBase *deci) const {
        // This method calculates the maximum length of the line considering the left/right margin and eventually
        // basing on the position set through a position tag
        return w.props.get_maxWidth(dx, prefs.subformat, deci) * gdi_font_scale;
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
    public std::vector< TsubtitleText* >
{
    virtual bool isText(void) const {return true;}
    virtual void print(
       REFERENCE_TIME time,
       bool wasseek,
       Tfont &f,
       bool forceChange,
       TprintPrefs &prefs,
       unsigned char **dst,
       const stride_t *stride);
};

#endif
