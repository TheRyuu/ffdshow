#ifndef _TSUBTITLETEXT_H_
#define _TSUBTITLETEXT_H_

#include "Tsubtitle.h"
#include "TsubtitleProps.h"
#include "TsubtitlesSettings.h"

struct TsubtitlesSettings;
struct Tconfig;
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
template<class tchar> class TtextFix : public TtextFixBase
{
private:
 typedef typename tchar_traits<tchar>::ffstring ffstring;
 typedef typename tchar_traits<tchar>::strings strings;
 TsubtitlesSettings cfg;
 bool EndOfPrevSentence,inHearing;
 strings odict;
 static inline bool in(tchar x,const tchar *s) {return strchr(s,x)!=NULL;}
public:
 TtextFix(const TsubtitlesSettings *Icfg,const Tconfig *ffcfg);
 bool process(ffstring &text,ffstring &fixed);
};

template<class tchar> struct TsubtitleLine;
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
   template<class tchar> void add(const tchar *l,const tchar* &l1,const tchar* &l2, TSubtitleProps &props,size_t step)
    {
     Tword word;word.i1=l1-l;word.i2=l2-l;word.props=props;
     push_back(word);
     l1=(l2+=step);
     props.karaokeNewWord = false;
    }
  };
private:
 TSubtitleProps props;
 template<class tchar,int c> struct Tncasecmp
  {
   bool operator ()(tchar c1)
    {
     return c==tolower(c1);
    }
  };
 template<class tchar> static DwString<tchar> getAttribute(const tchar *start,const tchar *end,const tchar *attrname);
 template<class tchar> struct Tssa
  {
  private:
   TSubtitleProps &props;
   const TSubtitleProps &defprops;
   Twords &words;
  public:
   typedef typename tchar_traits<tchar>::ffstring ffstring;
   typedef typename tchar_traits<tchar>::strings strings;

   union intx_t {
       int32_t  i32;
       uint32_t u32;
       int64_t  i64;
       uint64_t u64;
   };

   struct TparenthesesContent {
       ffstring str;
       int64_t intval;
       bool ok; // is number?
       TparenthesesContent(ffstring Istr) {
           tchar *bufend;
           str = Istr;
           intval = _strtoi64(Istr.c_str(), &bufend, 10);
           ok = (*bufend == 0 && bufend != Istr.c_str());
       }
   };
   typedef std::vector<TparenthesesContent> TparenthesesContents;

   struct TstoreParam {
       intx_t TSubtitleProps::* offset;
       int64_t min;
       int64_t max;
       int64_t default_value;
       size_t size; // sizeof actual intx_t
       TstoreParam(intx_t TSubtitleProps::* Ioffset, int64_t Imin, int64_t Imax, int64_t Idefault_value, size_t Isize):offset(Ioffset),min(Imin),max(Imax),default_value(Idefault_value),size(Isize) {}
   };
   struct TstoreParams: public std::vector<TstoreParam> {
       // returns number of contents that have the value within the range (min...max) and have been written to. 
       int writeProps(const TparenthesesContents &contents, TSubtitleProps &props);
   };

   int parse_parentheses(TparenthesesContents &contents, ffstring arg);
   Tssa(TSubtitleProps &Iprops,const TSubtitleProps &Idefprops,Twords &Iwords):props(Iprops),defprops(Idefprops),words(Iwords) {}
   typedef void (Tssa::*TssaAction)(ffstring &arg);
   typedef int (*Tstr_cmp_func)(const tchar *a, const tchar *b, size_t c);

   // fuctions that parse tokens
   void fontName(ffstring &arg);
   //void fontSize(ffstring &arg);
   template<int TSubtitleProps::*offset,int min,int max> void intProp(ffstring &arg);
   template<int TSubtitleProps::*offset,int min,int max> void intPropAn(ffstring &arg);
   template<double TSubtitleProps::*offset,int min,int max> void doubleProp(ffstring &arg);
   template<int TSubtitleProps::*offset1,int TSubtitleProps::*offset2,int min,int max> bool intProp2(ffstring &arg);
   void pos(ffstring &arg);
   void move(ffstring &arg);
   void fad(ffstring &arg);
   void fade(ffstring &arg);
   void karaoke_kf(ffstring &arg);
   void karaoke_ko(ffstring &arg);
   void karaoke_k(ffstring &arg);
   template<bool TSubtitleProps::*offset> void boolProp(ffstring &arg);
   bool arg2int(const ffstring &arg, int min, int max, int &enc);
   bool color2int(ffstring arg, int &intval);
   template<COLORREF TSubtitleProps::*offset> void color(ffstring &arg);
   template<int TSubtitleProps::*offset> void alpha(ffstring &arg);
   void alphaAll(ffstring &arg);
   void reset(ffstring &arg);

   bool processToken(const tchar* &l2,const tchar *tok,TssaAction action);
   bool processTokenC(const tchar* &l2,const tchar *tok,TssaAction action);
   bool processTokenI(const tchar* &l2,const tchar *tok,TssaAction action,Tstr_cmp_func str_cmp_func);
   void processTokens(const tchar *l,const tchar* &l1,const tchar* &l2,const tchar *end);
  };
 const ThtmlColors *htmlcolors;
public:
 TsubtitleFormat(const ThtmlColors *Ihtmlcolors):htmlcolors(Ihtmlcolors) {}
 template<class tchar> void processHTMLTags(Twords &words, const tchar* &l, const tchar* &l1, const tchar* &l2);
 template<class tchar> Twords processHTML(const TsubtitleLine<tchar> &line);
 template<class tchar> Twords processSSA(const TsubtitleLine<tchar> &line,int sfmt, TsubtitleTextBase<tchar> &parent);
 template<class tchar> void processMicroDVD(TsubtitleTextBase<tchar> &parent,typename std::vector< TsubtitleLine<tchar> >::iterator it);
 template<class tchar> void processMPL2(TsubtitleLine<tchar> &line);
 void resetProps(void){props.reset();}
};

template<class tchar> struct TsubtitleWord
{
private:
 typedef typename tchar_traits<tchar>::ffstring ffstring;
 typedef typename tchar_traits<tchar>::strings strings;
 ffstring text,fixed;
 bool useFixed;
 const ffstring& getText(void) const {return useFixed?fixed:text;}
 ffstring& getText(void) {return useFixed?fixed:text;}
public:
 TSubtitleProps props;

 TsubtitleWord(const ffstring &Itext):text(Itext),useFixed(false) {}
 TsubtitleWord(const ffstring &Itext,const TSubtitleProps &defProps):text(Itext),useFixed(false),props(defProps) {}

 TsubtitleWord(const tchar *Itext):text(Itext),useFixed(false) {}
 TsubtitleWord(const tchar *Itext,const TSubtitleProps &defProps):text(Itext),useFixed(false),props(defProps) {}

 TsubtitleWord(const tchar *s,size_t len):text(s,len),useFixed(false) {}
 TsubtitleWord(const tchar *s,size_t len,const TSubtitleProps &defProps):text(s,len),useFixed(false),props(defProps) {}

 void set(const ffstring &s)
  {
   getText()=s;
  }
 operator const tchar *(void) const
  {
   return getText().c_str();
  }
 void fix(TtextFix<tchar> &fix)
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
   text += _L(" ");
  }
};

template<class tchar> struct TsubtitleLine : std::vector< TsubtitleWord<tchar> >
{
private:
 typedef TsubtitleWord<tchar> TsubtitleWord;
 typedef std::vector<TsubtitleWord> Tbase;
 typedef typename tchar_traits<tchar>::ffstring ffstring;
 typedef typename tchar_traits<tchar>::strings strings;
 void applyWords(const TsubtitleFormat::Twords &words);
public:
 TSubtitleProps props;
 int lineBreakReason; // 0: none, 1: \n, 2: \N
 TsubtitleLine(void) {}
 TsubtitleLine(const ffstring &Itext) {push_back(Itext);}
 TsubtitleLine(const ffstring &Itext,const TSubtitleProps &defProps) {push_back(TsubtitleWord(Itext,defProps));}

 TsubtitleLine(const tchar *Itext) {push_back(Itext);}
 TsubtitleLine(const tchar *Itext,const TSubtitleProps &defProps) {push_back(TsubtitleWord(Itext,defProps));}
 TsubtitleLine(const tchar *Itext,const TSubtitleProps &defProps,int IlineBreakReason):lineBreakReason(IlineBreakReason) {push_back(TsubtitleWord(Itext,defProps));}

 TsubtitleLine(const tchar *s,size_t len) {push_back(TsubtitleWord(s,len));}
 TsubtitleLine(const tchar *s,size_t len,const TSubtitleProps &defProps) {push_back(TsubtitleWord(s,len,defProps));}
 TsubtitleLine(const tchar *s,size_t len,const TSubtitleProps &defProps,int IlineBreakReason):lineBreakReason(IlineBreakReason) {push_back(TsubtitleWord(s,len,defProps));}
 size_t strlen(void) const;
 void format(TsubtitleFormat &format,int sfmt,TsubtitleTextBase<tchar> &parent);
 void fix(TtextFix<tchar> &fix);
};
#include "TsubreaderMplayer.h"
template<class tchar> struct TsubtitleTextBase :public Tsubtitle,public std::vector< TsubtitleLine<tchar> >
{
private:
 typedef TsubtitleLine<tchar> TsubtitleLine;
 typedef std::vector<TsubtitleLine> Tbase;
 typedef typename tchar_traits<tchar>::ffstring ffstring;
 typedef typename tchar_traits<tchar>::strings strings;
public:
 int subformat;
 TSubtitleProps defProps;
 TsubtitleTextBase(int Isubformat):subformat(Isubformat) {}
 TsubtitleTextBase(int Isubformat,const TSubtitleProps &IdefProps):subformat(Isubformat),defProps(IdefProps) {}
 void set(const strings &strs)
  {
   this->clear();
   for (typename strings::const_iterator s=strs.begin();s!=strs.end();s++)
    this->push_back(TsubtitleLine(*s,defProps));
  }
 void set(const ffstring &str)
  {
   if (this->size()==1)
    this->at(0)=str;
   else
    {
     this->clear();
     this->push_back(TsubtitleLine(str,defProps));
    }
  }
 void add(const tchar *s)
  {
   this->push_back(TsubtitleLine(s,defProps));
  }
 void add(const tchar *s,size_t len)
  {
   this->push_back(TsubtitleLine(s,len,defProps));
  }
 void addSSA(const tchar *s, int lineBreakReason)
  {
   this->push_back(TsubtitleLine(s, defProps, lineBreakReason));
  }
 void addSSA(const tchar *s, size_t len, int lineBreakReason)
  {
   this->push_back(TsubtitleLine(s, len, defProps, lineBreakReason));
  }
 virtual void addEmpty(void)
  {
   this->push_back(TsubtitleLine(_L(" "),defProps));
  }
 void format(TsubtitleFormat &format);
 void prepareKaraoke(void);
 template<class Tval> void propagateProps(typename Tbase::iterator it,Tval TSubtitleProps::*offset,Tval val,typename Tbase::iterator itend)
  {
   for (;it!=itend;it++)
    for (typename TsubtitleLine::iterator w=it->begin();w!=it->end();w++)
     w->props.*offset=val;
  }
 template<class Tval> void propagateProps(typename Tbase::iterator it,Tval TSubtitleProps::*offset,Tval val)
  {
   propagateProps(it,offset,val,this->end());
  }
 template<class Tval> void propagateProps(Tval TSubtitleProps::*offset,Tval val)
  {
   propagateProps(this->begin(),offset,val,this->end());
  }
 void fix(TtextFix<tchar> &fix);
 virtual void print(REFERENCE_TIME time,bool wasseek,Tfont &f,bool forceChange,TrenderedSubtitleLines::TprintPrefs &prefs) const;
 virtual Tsubtitle* copy(void);
 virtual Tsubtitle* create(void) {return new TsubtitleTextBase<tchar>(subformat);}
 virtual bool copyLine(Tsubtitle *dst,size_t linenum)
  {
   ((TsubtitleTextBase<tchar>*)dst)->push_back(this->at(linenum));
   return true;
  }
 virtual bool copyProps(Tsubtitle *dst)
  {
   ((TsubtitleTextBase<tchar>*)dst)->defProps=this->defProps;
   return true;
  }

 virtual size_t numlines(void) const
  {
   return this->size();
  }
 virtual size_t numchars(void) const
  {
   size_t c=0;
   for (typename Tbase::const_iterator l=this->begin();l!=this->end();l++)
    c+=l->strlen();
   return c;
  }
 virtual bool isText(void) const {return true;}
};

#endif
