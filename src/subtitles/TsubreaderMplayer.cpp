/*
 * Subtitle reader with format autodetection
 *
 * Written by laaz
 * Some code cleanup & realloc() by A'rpi/ESP-team
 * dunnowhat sub format by szabi
 */

#include "stdafx.h"
#include "TsubreaderMplayer.h"
#include "TsubtitlesSettings.h"
#include "Tstream.h"
#include "Tconfig.h"
#include "ffdebug.h"

//========================================= TsubtitleParser =========================================
TsubtitleParser::TsubtitleParser(int Iformat,double Ifps,const TsubtitlesSettings *Icfg,const Tconfig *Iffcfg,Tsubreader *Isubreader):
 TsubtitleParserBase(Iformat,Ifps),
 ffcfg(Iffcfg),
 textfix(Icfg,Iffcfg),
 subreader(Isubreader),
 textformat(Iffcfg->getHtmlColors()),
 lineID(0)
{
 cfg = *Icfg;
}

int TsubtitleParser::eol(wchar_t p) {
    return (p=='\r' || p=='\n' || p=='\0');
}

/* Remove leading and trailing space */
void TsubtitleParser::trail_space(wchar_t *s) {
        int i = 0;
        while (iswspace((unsigned short)s[i])) ++i;
        if (i) strcpy(s, s + i);
        i = (int)strlen(s) - 1;
        while (i > 0 && iswspace((unsigned short)s[i])) s[i--] = '\0';
}

Tsubtitle* TsubtitleParser::store(TsubtitleText &sub)
{
 sub.defProps.extendedTags=cfg.extendedTags;
 sub.format(textformat);
 sub.prepareKaraoke();
 sub.fix(textfix);
 subreader->push_back(new TsubtitleText(sub));
 return subreader->back();
}

const wchar_t* TsubtitleParser::sub_readtext(const wchar_t *source, TsubtitleText &sub) {
    int len=0;
    const wchar_t *p=source;

//    printf("src=%p  dest=%p  \n",source,dest);

    while ( !eol(*p) && *p!= '|' ) {
        p++,len++;
    }

    sub.add(source,len);
    while (*p=='\r' || *p=='\n' || *p=='|') p++;

    if (*p) return p;  // not-last text field
    else return NULL;  // last text field
}

Tsubtitle* TsubtitleParserSami::parse(Tstream &fd,int flags, REFERENCE_TIME start, REFERENCE_TIME stop) {
    wchar_t text[this->LINE_LEN+1], *p=NULL,*q;
    int state;


    /* read the first line */
    if (!s)
     if ((s = fd.fgets(line, this->LINE_LEN))==NULL) return NULL;

    TsubtitleText current(this->format);
    current.start = current.stop = 0;
    state = 0;

    do {
        switch (state) {

        case 0: /* find "START=" or "Slacktime:" */
            slacktime_s = stristr (s, L"Slacktime:");
        if (slacktime_s)
                sub_slacktime = strtol (slacktime_s+10, NULL, 0) / 10;

            s = (wchar_t*)stristr (s, L"Start=");
            if (s) {
                int sec1000=strtol (s + 6, &s, 0);
                current.start = this->hmsToTime(0,0,sec1000/1000,(sec1000%1000)/10);
                /* eat '>' */
                for (; *s != '>' && *s != '\0'; s++);
                s++;
                state = 1; continue;
            }
            break;

    case 1: /* find (optionnal) "<P", skip other TAGs */
        for  (; *s == ' ' || *s == '\t'; s++); /* strip blanks, if any */
        if (*s == '\0') break;
        if (*s != '<') { state = 3; p = text; continue; } /* not a TAG */
        s++;
        if (*s == 'P' || *s == 'p') { s++; state = 2; continue; } /* found '<P' */
        for (; *s != '>' && *s != '\0'; s++); /* skip remains of non-<P> TAG */
        if (s == '\0')
          break;
        s++;
            continue;

        case 2: /* find ">" */
            if ((s = strchr (s, '>'))!=NULL) { s++; state = 3; p = text; continue; }
            break;

        case 3: /* get all text until '<' appears */
            if (*s == '\0') break;
            else if (!_strnicmp (s, L"<br>", 4)) {
                *p = '\0'; p = text; trail_space (text);
                if (text[0] != '\0')
                    current.add(text);
                s += 4;
            }
        else if ((*s == '{') && !sub_no_text_pp) { state = 5; ++s; continue; }
            else if (*s == '<') { state = 4; }
            else if (!_strnicmp (s, L"&nbsp;", 6)) { *p++ = ' '; s += 6; }
            else if (*s == '\t') { *p++ = ' '; s++; }
            else if (*s == '\r' || *s == '\n') { s++; }
            else *p++ = *s++;

            /* skip duplicated space */
            if (p > text + 2) if (*(p-1) == ' ' && *(p-2) == ' ') p--;

            continue;

    case 4: /* get current->end or skip <TAG> */
        q = (wchar_t*)stristr (s, L"Start=");
            if (q) {
                int sec1000=strtol (q + 6, &q, 0);
                current.stop = this->hmsToTime(0,0, sec1000/1000,(sec1000%1000)/10-1);
                *p = '\0'; trail_space (text);
                if (text[0] != '\0')
                    current.add(text);
                if (current.size() > 0) { state = 99; break; }
                state = 0; continue;
            }
            s = strchr (s, '>');
            if (s) { s++; state = 3; continue; }
            break;
       case 5: /* get rid of {...} text, but read the alignment code */
        if ((*s == '\\') && (*(s + 1) == 'a') && !sub_no_text_pp) {
               if (stristr(s, L"\\a1") != NULL) {
                   //current->alignment = SUB_ALIGNMENT_BOTTOMLEFT;
                   s = s + 3;
               }
               if (stristr(s, L"\\a2") != NULL) {
                   //current->alignment = SUB_ALIGNMENT_BOTTOMCENTER;
                   s = s + 3;
               } else if (stristr(s, L"\\a3") != NULL) {
                   //current->alignment = SUB_ALIGNMENT_BOTTOMRIGHT;
                   s = s + 3;
               } else if ((stristr(s, L"\\a4") != NULL) || (stristr(s, L"\\a5") != NULL) || (stristr(s, L"\\a8") != NULL)) {
                   //current->alignment = SUB_ALIGNMENT_TOPLEFT;
                   s = s + 3;
               } else if (stristr(s, L"\\a6") != NULL) {
                   //current->alignment = SUB_ALIGNMENT_TOPCENTER;
                   s = s + 3;
               } else if (stristr(s, L"\\a7") != NULL) {
                   //current->alignment = SUB_ALIGNMENT_TOPRIGHT;
                   s = s + 3;
               } else if (stristr(s, L"\\a9") != NULL) {
                   //current->alignment = SUB_ALIGNMENT_MIDDLELEFT;
                   s = s + 3;
               } else if (stristr(s, L"\\a10") != NULL) {
                   //current->alignment = SUB_ALIGNMENT_MIDDLECENTER;
                   s = s + 4;
               } else if (stristr(s, L"\\a11") != NULL) {
                   //current->alignment = SUB_ALIGNMENT_MIDDLERIGHT;
                   s = s + 4;
               }
        }
        if (*s == '}') state = 3;
        ++s;
        continue;
        }

        /* read next line */
        if (state != 99 && (s = fd.fgets (line, this->LINE_LEN))==NULL) {
            if (current.start > 0) {
                break; // if it is the last subtitle
            } else {
                return NULL;
            }
        }

    } while (state != 99);

    // For the last subtitle
    if (current.stop <= 0) {
        current.stop = current.start + this->hmsToTime(0,0,sub_slacktime/1000,(sub_slacktime%1000)/10);
        *p = '\0'; trail_space (text);
        if (text[0] != '\0')
            current.add(text);
    }
    return store(current);
}

Tsubtitle* TsubtitleParserMicrodvd::parse(Tstream &fd,int flags, REFERENCE_TIME, REFERENCE_TIME) {
    wchar_t line[this->LINE_LEN+1];
    wchar_t line2[this->LINE_LEN+1];
    const wchar_t *p, *next;

    int start=0,stop=0;
    bool skip;
    do {
      do {
          if (!fd.fgets (line, this->LINE_LEN)) return NULL;
      } while ((swscanf (line,
                        L"{%ld}{}%[^\r\n]",
                        &start, line2) < 2) &&
               (swscanf (line,
                        L"{%ld}{%ld}%[^\r\n]",
                        &start, &stop, line2) < 3));
      skip=false;
      if (start==1 && stop==1)
       {
        wchar_t *e;double newfps;
        if ((newfps=strtod(line2,&e))>0 && !*e)
         {
          this->fps=newfps;
          skip=true;
         }
       }
    } while (skip);

    TsubtitleText current(this->format);
    current.start=this->frameToTime(start);
    current.stop =this->frameToTime(stop );

    p=line2;

    next=p;
    while ((next =sub_readtext (next, current))!=NULL)
     ;
    return store(current);
}

Tsubtitle* TsubtitleParserSubrip::parse(Tstream &fd,int flags, REFERENCE_TIME start, REFERENCE_TIME stop) {
    wchar_t line[this->LINE_LEN+1];
    int a1,a2,a3,a4,b1,b2,b3,b4;
    wchar_t *p=NULL, *q=NULL;
    int len;
    TsubtitleText current(this->format);
    while (1) {
        if (!fd.fgets (line, this->LINE_LEN)) return NULL;
        if (flags&this->PARSETIME)
         {
          if (swscanf (line, L"%d:%d:%d.%d,%d:%d:%d.%d",&a1,&a2,&a3,&a4,&b1,&b2,&b3,&b4) < 8) continue;
          current.start = this->hmsToTime(a1,a2,a3,a4);
          current.stop  = this->hmsToTime(b1,b2,b3,b4);
          if (!fd.fgets (line, this->LINE_LEN)) return NULL;
         }

        p=q=line;
        for (;;) {
            for (q=p,len=0; *p && *p!='\r' && *p!='\n' && *p!='|' && strncmp(p,L"[br]",4); p++,len++);
            current.add(q,len);
            if (!*p || *p=='\r' || *p=='\n') break;
            if (*p=='|') p++;
            else while (*p++!=']');
        }
        break;
    }

    return store(current);
}

Tsubtitle* TsubtitleParserSubviewer::parse(Tstream &fd,int flags, REFERENCE_TIME start, REFERENCE_TIME stop) {
    wchar_t line[this->LINE_LEN+1];
    int a1,a2,a3,a4,b1,b2,b3,b4;
    wchar_t *p=NULL;
    int len;
    TsubtitleText current(this->format);
    TsubtitleParser::textformat.resetProps();
    while (!current.size()) {
        if (flags&this->PARSETIME)
         {
          if (!fd.fgets(line, this->LINE_LEN)) return NULL;
          int li;
      if ((len=swscanf(line, L"%d:%d:%d%[,.:]%d --> %d:%d:%d%[,.:]%d",&a1,&a2,&a3,&li,&a4,&b1,&b2,&b3,&li,&b4)) < 10)
           continue;
          current.start = this->hmsToTime(a1,a2,a3,a4/10);
          current.stop  = this->hmsToTime(b1,b2,b3,b4/10);
         }
        for (;;) {
            if (!fd.fgets (line, this->LINE_LEN)) goto end;//break;
            len=0;
            for (p=line; *p!='\n' && *p!='\r' && *p; p++,len++);
            if (len) {
                int j=0,skip=0;
        wchar_t *curptr0,*curptr=curptr0=(wchar_t*)_alloca((len+1)*sizeof(wchar_t));
                for(; j<len; j++) {
            /* let's filter html tags ::atmos */
                    /*
            if(line[j]=='>') {
            skip=0;
            continue;
            }
            if(line[j]=='<') {
            skip=1;
            continue;
            }*/
            if(skip) {
            continue;
            }
            *curptr=line[j];
            curptr++;
        }
        *curptr='\0';
                current.add(curptr0);
            } else {
                break;
            }
        }
    }
end:
    return current.empty()?NULL:store(current);
}

Tsubtitle* TsubtitleParserSubviewer2::parse(Tstream &fd,int flags, REFERENCE_TIME start, REFERENCE_TIME stop) {
    wchar_t line[this->LINE_LEN+1];
    int a1,a2,a3,a4;
    wchar_t *p=NULL;
    int len;
    TsubtitleText current(this->format);
    while (!current.size()) {
        if (!fd.fgets (line, this->LINE_LEN)) return NULL;
        if (line[0]!='{')
            continue;
        if ((len=swscanf (line, L"{T %d:%d:%d:%d",&a1,&a2,&a3,&a4)) < 4)
            continue;
        current.start = this->hmsToTime(a1,a2,a3,a4);
        for (/*i=0*/;;) {
            if (!fd.fgets (line, this->LINE_LEN)) goto end;//break;
            if (line[0]=='}') break;
            len=0;
            for (p=line; *p!='\n' && *p!='\r' && *p; ++p,++len);
            if (len) {
                current.add(line,len);
            } else {
                break;
            }
        }
        //current->lines=i;
    }
  end:
    return current.empty()?NULL:store(current);
}


Tsubtitle* TsubtitleParserVplayer::parse(Tstream &fd,int flags, REFERENCE_TIME start, REFERENCE_TIME stop) {
        wchar_t line[this->LINE_LEN+1];
        int a1,a2,a3,a4;
        const wchar_t *p=NULL, *next;wchar_t separator1,separator2;
        int plen;
        TsubtitleText current(this->format);
        while (current.empty()) {
                if (!fd.fgets (line, this->LINE_LEN)) return NULL;
                int ret=swscanf(line, L"%d:%d:%d%c%d%c%n",L"%d:%d:%d%c%d%lc%n",&a1,&a2,&a3,&separator1,&a4,&separator2,&plen);
                if (ret!=6)
                 {
                  a4=0;
                  ret=swscanf(line, L"%d:%d:%d%c%n",L"%d:%d:%d%lc%n",&a1,&a2,&a3,&separator1,&plen);
                  if (ret!=4)
                   continue;
                 }

                if ((current.start = this->hmsToTime(a1,a2,a3,a4*10))==NULL)
                        continue;
                /* removed by wodzu
                p=line;
                // finds the body of the subtitle
                for (i=0; i<3; i++){
                   p=strchr(p,':');
                   if (p==NULL) break;
                   ++p;
                }
                if (p==NULL) {
                    printf("SUB: Skipping incorrect subtitle line!\n");
                    continue;
                }
                */
                // by wodzu: hey! this time we know what length it has! what is
                // that magic for? it can't deal with space instead of third
                // colon! look, what simple it can be:
                p = &line[ plen ];

                if (*p!='|') {
                        //
                        next = p;
                        while ((next =sub_readtext (next, current))!=NULL)
                         ;
                }
        }

        return store(current);
}

Tsubtitle* TsubtitleParserRt::parse(Tstream &fd,int flags, REFERENCE_TIME start, REFERENCE_TIME stop) {
        //TODO: This format uses quite rich (sub/super)set of xhtml
        // I couldn't check it since DTD is not included.
        // WARNING: full XML parses can be required for proper parsing
    wchar_t line[this->LINE_LEN+1];
    int a1,a2,a3,a4,b1,b2,b3,b4;
    const wchar_t *p=NULL,*next=NULL;
    int plen;
    TsubtitleText current(this->format);
    while (current.empty()) {
        if (!fd.fgets (line, this->LINE_LEN)) return NULL;
        //TODO: it seems that format of time is not easily determined, it may be 1:12, 1:12.0 or 0:1:12.0
        //to describe the same moment in time. Maybe there are even more formats in use.
        //if ((len=wsscanf (line, L"<Time Begin=\"%d:%d:%d.%d\" End=\"%d:%d:%d.%d\"",&a1,&a2,&a3,&a4,&b1,&b2,&b3,&b4)) < 8)
        plen=a1=a2=a3=a4=b1=b2=b3=b4=0;
        if (
    (swscanf(line, L"<%*[tT]ime %*[bB]egin=\"%d.%d\" %*[Ee]nd=\"%d.%d\"%*[^<]<clear/>%n",&a3,&a4,&b3,&b4,&plen) < 4) &&
    (swscanf(line, L"<%*[tT]ime %*[bB]egin=\"%d.%d\" %*[Ee]nd=\"%d:%d.%d\"%*[^<]<clear/>%n",&a3,&a4,&b2,&b3,&b4,&plen) < 5) &&
        (swscanf(line, L"<%*[tT]ime %*[bB]egin=\"%d:%d\" %*[Ee]nd=\"%d:%d\"%*[^<]<clear/>%n",&a2,&a3,&b2,&b3,&plen) < 4) &&
        (swscanf(line, L"<%*[tT]ime %*[bB]egin=\"%d:%d\" %*[Ee]nd=\"%d:%d.%d\"%*[^<]<clear/>%n",&a2,&a3,&b2,&b3,&b4,&plen) < 5) &&
//      (swscanf(line, L"<%*[tT]ime %*[bB]egin=\"%d:%d.%d\" %*[Ee]nd=\"%d:%d\"%*[^<]<clear/>%n",&a2,&a3,&a4,&b2,&b3,&plen) < 5) &&
        (swscanf(line, L"<%*[tT]ime %*[bB]egin=\"%d:%d.%d\" %*[Ee]nd=\"%d:%d.%d\"%*[^<]<clear/>%n",&a2,&a3,&a4,&b2,&b3,&b4,&plen) < 6) &&
    (swscanf(line, L"<%*[tT]ime %*[bB]egin=\"%d:%d:%d.%d\" %*[Ee]nd=\"%d:%d:%d.%d\"%*[^<]<clear/>%n",&a1,&a2,&a3,&a4,&b1,&b2,&b3,&b4,&plen) < 8) &&
    //now try it without end time
    (swscanf(line, L"<%*[tT]ime %*[bB]egin=\"%d.%d\"%*[^<]<clear/>%n",&a3,&a4,&plen) < 2) &&
    (swscanf(line, L"<%*[tT]ime %*[bB]egin=\"%d:%d\"%*[^<]<clear/>%n",&a2,&a3,&plen) < 2) &&
    (swscanf(line, L"<%*[tT]ime %*[bB]egin=\"%d:%d.%d\"%*[^<]<clear/>%n",&a2,&a3,&a4,&plen) < 3) &&
    (swscanf(line, L"<%*[tT]ime %*[bB]egin=\"%d:%d:%d.%d\"%*[^<]<clear/>%n",&a1,&a2,&a3,&a4,&plen) < 4)
        )
            continue;
        current.start = this->hmsToTime(a1,a2,a3,a4);
        current.stop  = this->hmsToTime(a1,a2,a3,a4);
    if (b1 == 0 && b2 == 0 && b3 == 0 && b4 == 0)
      current.stop = current.start+this->frameToTime(200);
        p=line; p+=plen;
        // TODO: I don't know what kind of convention is here for marking multiline subs, maybe <br/> like in xml?
        next = strstr(line,L"<clear/>");
        if(next && strlen(next)>8){
          next+=8;
          while ((next =sub_readtext (next, current))!=NULL)
           ;
        }
    }

    return store(current);
}

TsubtitleParserSSA::TsubtitleParserSSA(int Iformat,double Ifps,const TsubtitlesSettings *Icfg,const Tconfig *Iffcfg,Tsubreader *Isubreader,bool isEmbedded0):
 TsubtitleParser(Iformat,Ifps,Icfg,Iffcfg,Isubreader),
 inV4styles(0),inEvents(0),inInfo(0),
 playResX(0),playResY(0),wrapStyle(0),
 timer(1,1),
 isEmbedded(isEmbedded0)
{
 defprops.alignment = 2;
 defprops.outlineWidth = defprops.shadowDepth = 2;
 defprops.size = 26;
 strcpy(defprops.fontname, _l("Arial"));
 defprops.encoding = 0;
 defprops.isColor = true;
 defprops.marginR = defprops.marginL = defprops.marginV = 0;
 defprops.version = TsubtitleParserSSA::SSA;
}

void TsubtitleParserSSA::strToInt(const ffstring &str,int *i)
{
 if (!str.empty())
  {
   wchar_t *end;
   int val=strtol(str.c_str(),&end,10);
   if (*end=='\0' && val>=0) *i=val;
  }
}

void TsubtitleParserSSA::strToIntMargin(const ffstring &str,int *i)
{
 if (!str.empty() /*str.size()==4 && str.compare(_L("0000"))!=0*/)
  {
   wchar_t *end;
   int val=strtol(str.c_str(),&end,10);
   if (*end=='\0' && val>0) *i=val;
  }
}

void TsubtitleParserSSA::strToDouble(const ffstring &str,double *d)
{
 if (!str.empty())
  {
   wchar_t *end;
   double val=strtod(str.c_str(),&end);
   if (*end=='\0') *d=val;
  }
}

bool TsubtitleParserSSA::Tstyle::toCOLORREF(const ffstring& colourStr,COLORREF &colour,int &alpha)
{
 if (colourStr.empty()) return false;
 int radix;
 ffstring s1,s2;
 s1=colourStr;
 s1.ConvertToUpperCase();
 if (s1.compare(0,2,L"&H",2)==0)
  {
   s1.erase(0,2);
   radix=16;
  }
 else
  radix=10;
 s2=s1;
 if (s1.size()>6)
  {
   s1.erase(s1.size()-6,6);
   s2.erase(0,s2.size()-6);
  }
 else
  s1.clear();

 int msb=0;
 if (!s1.empty())
  {
   const wchar_t *alphaS=s1.c_str();
   wchar_t *endalpha;
   long a=strtol(alphaS,&endalpha,radix);
   if (*endalpha=='\0')
    msb=a;
  }
 if (s2.empty()) return false;
 const wchar_t *colorS=s2.c_str();
 wchar_t *endcolor;
 COLORREF c=strtol(colorS,&endcolor,radix);
 if (*endcolor=='\0')
  {
   DWORD result=msb * (radix==16 ? 0x1000000 : 1000000) + c;
   colour=result & 0xffffff;
   alpha=256-(result>>24);
   return true;
  }
 return false;
}

void TsubtitleParserSSA::Tstyle::toProps(void)
{
 if (fontname)
  text<char_t>(fontname.c_str(), -1, props.fontname, countof(props.fontname));
 if (int size=atoi(fontsize.c_str()))
  props.size=size;
 bool isColor=toCOLORREF(primaryColour,props.color,props.colorA);
 isColor|=toCOLORREF(secondaryColour,props.SecondaryColour,props.SecondaryColourA);
 isColor|=toCOLORREF(tertiaryColour,props.TertiaryColour,props.TertiaryColourA);
 isColor|=toCOLORREF(outlineColour,props.OutlineColour,props.OutlineColourA);
 if (version==TsubtitleParserSSA::SSA)
  {
   isColor|=toCOLORREF(backgroundColour,props.OutlineColour,props.OutlineColourA);
   props.ShadowColour=props.OutlineColour;
   props.ShadowColourA=128;
  }
 else
  isColor|=toCOLORREF(backgroundColour,props.ShadowColour,props.ShadowColourA);
 props.isColor=isColor;
 if (bold==L"-1")
  props.bold=1;
 else
  props.bold=0;
 if (italic==L"-1") props.italic=true;
 if (underline==L"-1") props.underline=true;
 if (strikeout==L"-1") props.strikeout=true;
 strToInt(encoding,&props.encoding);
 strToDouble(spacing,&props.spacing);
 strToInt(fontScaleX,&props.scaleX);
 strToInt(fontScaleY,&props.scaleY);
 strToInt(alignment,&props.alignment);
 strToInt(marginLeft,&props.marginL);
 strToInt(marginRight,&props.marginR);
 strToInt(marginV,&props.marginV);
 strToInt(marginTop,&props.marginTop);
 strToInt(marginBottom,&props.marginBottom);
 strToInt(borderStyle,&props.borderStyle);
 strToDouble(outlineWidth,&props.outlineWidth);
 strToDouble(shadowDepth,&props.shadowDepth);
 if (alignment && this->version != SSA)
  props.alignment=TSubtitleProps::alignASS2SSA(props.alignment);
}
void TsubtitleParserSSA::Tstyles::add(Tstyle &s)
{
 s.toProps();
 insert(std::make_pair(s.name,s));
}
const TSubtitleProps* TsubtitleParserSSA::Tstyles::getProps(const ffstring &style)
{
 std::map<ffstring,Tstyle,ffstring_iless>::const_iterator si=this->find(style);
 if (si!=this->end())
  return &si->second.props;

 std::map<ffstring,Tstyle,ffstring_iless>::const_iterator iDefault=this->find(ffstring(L"Default"));
 if (iDefault!=this->end())
  return &iDefault->second.props;

 iDefault=this->find(ffstring(L"*Default"));

 if (iDefault!=this->end())
  return &iDefault->second.props;
 else
  return NULL;
}

Tsubtitle* TsubtitleParserSSA::parse(Tstream &fd, int flags, REFERENCE_TIME start, REFERENCE_TIME stop) {
/*
 * Sub Station Alpha v4 (and v2?) scripts have 9 commas before subtitle
 * other Sub Station Alpha scripts have only 8 commas before subtitle
 * Reading the "ScriptType:" field is not reliable since many scripts appear
 * w/o it
 *
 * http://www.scriptclub.org is a good place to find more examples
 * http://www.eswat.demon.co.uk is where the SSA specs can be found
 */
 wchar_t line0[this->LINE_LEN+1];
 wchar_t *line=line0;
 while (fd.fgets(line, this->LINE_LEN))
  {
#if 0
   text<char_t> lineD0(line);
   const char_t* lineD1=(const char_t*)lineD0;
   DPRINTF(_l("%s"),lineD1);
#endif
   lineID++;
   if (line[0]==';')
    continue;
   wchar_t *cr=strrchr(line,'\n');if (cr) *cr='\0';
   cr=strrchr(line,'\r');if (cr) *cr='\0';
   if (strnicmp(line,L"[Script Info]",13)==0)
    {
     inV4styles=0;
     inEvents=0;
     inInfo=1;
    }
   else if (inInfo && strnicmp(line,L"PlayResX:",8)==0)
    strToInt(line+9,&playResX);
   else if (inInfo && strnicmp(line,L"PlayResY:",8)==0)
    strToInt(line+9,&playResY);
   else if (inInfo && strnicmp(line,L"Timer:",6)==0)
    {
     wchar_t *end;
     double t=strtod(line+7,&end);
     if (*end=='\0' && t!=0)
      timer=Rational(t/100.0,INT32_MAX);
    }
   else if (inInfo && strnicmp(line,L"WrapStyle:",9)==0)
    {
     strToInt(line+10,&wrapStyle);
    }
   else if (strnicmp(line,L"[V4 Styles]",11)==0)
    {
     version=SSA;
     inV4styles=2;
     inEvents=0;
     inInfo=0;
    }
   else if (strnicmp(line,L"[V4+ Styles]",11)==0)
    {
     version=ASS;
     inV4styles=2;
     inEvents=0;
     inInfo=0;
    }
   else if (strnicmp(line,L"[V4++ Styles]",11)==0)
    {
     version=ASS2;
     inV4styles=2;
     inEvents=0;
     inInfo=0;
    }
   else if (strnicmp(line,L"[Events]",8)==0)
    {
     inV4styles=0;
     inEvents=2;
     inInfo=0;
    }
   else if (inV4styles==2 && strnicmp(line,L"Format:",7)==0)
    {
     strlwr(line);strrmchar(line,L' ');
     typedef std::vector<Tstrpart > Tparts;
     Tparts fields;
     const wchar_t *l=line+7;
     strtok(l,L",",fields);
     styleFormat.clear();
     for (Tparts::const_iterator f=fields.begin();f!=fields.end();f++)
      {
       if (strnicmp(f->first,L"name",4)==0)
        styleFormat.push_back(&Tstyle::name);
       else if (strnicmp(f->first,L"layer",5)==0)
        styleFormat.push_back(&Tstyle::layer);
       else if (strnicmp(f->first,L"fontname",8)==0)
        styleFormat.push_back(&Tstyle::fontname);
       else if (strnicmp(f->first,L"fontsize",8)==0)
        styleFormat.push_back(&Tstyle::fontsize);
       else if (strnicmp(f->first,L"primaryColour",13)==0)
        styleFormat.push_back(&Tstyle::primaryColour);
       else if (strnicmp(f->first,L"SecondaryColour",15)==0)
        styleFormat.push_back(&Tstyle::secondaryColour);
       else if (strnicmp(f->first,L"TertiaryColour",14)==0)
        styleFormat.push_back(&Tstyle::tertiaryColour);
       else if (strnicmp(f->first,L"OutlineColour",13)==0)
        styleFormat.push_back(&Tstyle::outlineColour);
       else if (strnicmp(f->first,L"BackColour",10)==0)
        styleFormat.push_back(&Tstyle::backgroundColour);
       else if (strnicmp(f->first,L"bold",4)==0)
        styleFormat.push_back(&Tstyle::bold);
       else if (strnicmp(f->first,L"italic",6)==0)
        styleFormat.push_back(&Tstyle::italic);
       else if (strnicmp(f->first,L"Underline",9)==0)
        styleFormat.push_back(&Tstyle::underline);
       else if (strnicmp(f->first,L"Strikeout",9)==0)
        styleFormat.push_back(&Tstyle::strikeout);
       else if (strnicmp(f->first,L"ScaleX",6)==0)
        styleFormat.push_back(&Tstyle::fontScaleX);
       else if (strnicmp(f->first,L"ScaleY",6)==0)
        styleFormat.push_back(&Tstyle::fontScaleY);
       else if (strnicmp(f->first,L"Spacing",7)==0)
        styleFormat.push_back(&Tstyle::spacing);
       else if (strnicmp(f->first,L"outline",7)==0)
        styleFormat.push_back(&Tstyle::outlineWidth);
       else if (strnicmp(f->first,L"shadow",6)==0)
        styleFormat.push_back(&Tstyle::shadowDepth);
       else if (strnicmp(f->first,L"alignment",9)==0)
        styleFormat.push_back(&Tstyle::alignment);
       else if (strnicmp(f->first,L"encoding",8)==0)
        styleFormat.push_back(&Tstyle::encoding);
       else if (strnicmp(f->first,L"marginl",7)==0)
        styleFormat.push_back(&Tstyle::marginLeft);
       else if (strnicmp(f->first,L"marginr",7)==0)
        styleFormat.push_back(&Tstyle::marginRight);
       else if (strnicmp(f->first,L"marginv",7)==0)
        styleFormat.push_back(&Tstyle::marginV);
       else if (strnicmp(f->first,L"borderstyle",11)==0)
        styleFormat.push_back(&Tstyle::borderStyle);
       else
        styleFormat.push_back(NULL);
      }
     inV4styles=1;
    }
   else if (inV4styles && strnicmp(line,L"Style:",6)==0)
    {
     if (inV4styles==2)
      {
       styleFormat.clear();
       if (version==ASS2)
        {
         styleFormat.push_back(&Tstyle::name);
         styleFormat.push_back(&Tstyle::fontname);
         styleFormat.push_back(&Tstyle::fontsize);
         styleFormat.push_back(&Tstyle::primaryColour);
         styleFormat.push_back(&Tstyle::secondaryColour);
         styleFormat.push_back(&Tstyle::tertiaryColour);
         styleFormat.push_back(&Tstyle::backgroundColour);
         styleFormat.push_back(&Tstyle::bold);
         styleFormat.push_back(&Tstyle::italic);
         if (version>=ASS) styleFormat.push_back(&Tstyle::underline);
         if (version>=ASS) styleFormat.push_back(&Tstyle::strikeout);
         if (version>=ASS) styleFormat.push_back(&Tstyle::fontScaleX);
         if (version>=ASS) styleFormat.push_back(&Tstyle::fontScaleY);
         if (version>=ASS) styleFormat.push_back(&Tstyle::spacing);
         if (version>=ASS) styleFormat.push_back(&Tstyle::angleZ);
         styleFormat.push_back(&Tstyle::borderStyle);
         styleFormat.push_back(&Tstyle::outlineWidth);
         styleFormat.push_back(&Tstyle::shadowDepth);
         styleFormat.push_back(&Tstyle::alignment);
         styleFormat.push_back(&Tstyle::marginLeft);
         styleFormat.push_back(&Tstyle::marginRight);
         styleFormat.push_back(&Tstyle::marginTop);
         if (version>=ASS2) styleFormat.push_back(&Tstyle::marginBottom);
         styleFormat.push_back(&Tstyle::encoding);
         if (version<=SSA) styleFormat.push_back(&Tstyle::alpha);
         styleFormat.push_back(&Tstyle::relativeTo);
        }
       inV4styles=1;
      }
     strings fields;
     strtok(line+7,L",",fields);
     Tstyle style(playResX,playResY,version,wrapStyle);
     for (size_t i=0;i<fields.size() && i<styleFormat.size();i++)
       if (styleFormat[i])
        style.*(styleFormat[i])=fields[i];
     styles.add(style);
    }
   else if (inEvents==2 && strnicmp(line,L"Format:",7)==0)
    {
     strlwr(line);strrmchar(line,L' ');
     typedef std::vector<Tstrpart > Tparts;
     Tparts fields;
     const wchar_t *l=line+7;
     strtok(l,L",",fields);
     eventFormat.clear();

     // On embedded streams, read order is added as first column
     if (isEmbedded)
        eventFormat.push_back(&Tevent::readorder);

     for (Tparts::const_iterator f=fields.begin();f!=fields.end();f++)
      {
       if (strnicmp(f->first,L"marked",6)==0)
        eventFormat.push_back(&Tevent::marked);
       else if (strnicmp(f->first,L"layer",5)==0)
        eventFormat.push_back(&Tevent::layer);
       else if (strnicmp(f->first,L"start",5)==0)
        {
         // On embedded subtitles time is removed
         if (!isEmbedded) eventFormat.push_back(&Tevent::start);
        }
       else if (strnicmp(f->first,L"end",3)==0)
       {
         // On embedded subtitles time is removed
         if (!isEmbedded) eventFormat.push_back(&Tevent::end);
       }
       else if (strnicmp(f->first,L"style",5)==0)
        eventFormat.push_back(&Tevent::style);
       else if (strnicmp(f->first,L"name",4)==0)
        eventFormat.push_back(&Tevent::name);
       else if (strnicmp(f->first,L"marginL",7)==0)
        eventFormat.push_back(&Tevent::marginL);
       else if (strnicmp(f->first,L"marginR",7)==0)
        eventFormat.push_back(&Tevent::marginR);
       else if (strnicmp(f->first,L"marginV",7)==0)
        eventFormat.push_back(&Tevent::marginV);
       else if (strnicmp(f->first,L"marginR",7)==0)
        eventFormat.push_back(&Tevent::marginR);
       else if (strnicmp(f->first,L"effect",6)==0)
        eventFormat.push_back(&Tevent::effect);
       else if (strnicmp(f->first,L"text",4)==0)
        eventFormat.push_back(&Tevent::text);
       else
        eventFormat.push_back(NULL);
      }
     inEvents=1;
    }
   else if ((flags&this->SSA_NODIALOGUE) || (inEvents==1 && strnicmp(line,L"Dialogue:",8)==0))
    {
     if (eventFormat.empty())
      {
       if (!(flags&this->SSA_NODIALOGUE))
        {
         if (version<=SSA) eventFormat.push_back(&Tevent::marked);
         if (version>=ASS) eventFormat.push_back(&Tevent::layer);
         eventFormat.push_back(&Tevent::start);
         eventFormat.push_back(&Tevent::end);
        }
       else
        {
         eventFormat.push_back(&Tevent::readorder);
         eventFormat.push_back(&Tevent::layer);
        }
       eventFormat.push_back(&Tevent::style);
       eventFormat.push_back(&Tevent::actor);
       eventFormat.push_back(&Tevent::marginL);
       eventFormat.push_back(&Tevent::marginR);
       eventFormat.push_back(&Tevent::marginT);
       if (version>=ASS2) eventFormat.push_back(&Tevent::marginB);
       eventFormat.push_back(&Tevent::effect);
       eventFormat.push_back(&Tevent::text);
      }
     strings fields;
     strtok(line+(flags&this->SSA_NODIALOGUE?0:9),L"," ,fields,true,eventFormat.size());
     Tevent event;
     event.dummy=""; // avoid being optimized.
     for (size_t i=0;i<fields.size() && i<eventFormat.size();i++/*,it++*/)
      {
       const wchar_t *str=fields[i].data();
       if (eventFormat[i])
        event.*(eventFormat[i]/* *it*/)=fields[i];
      }
     if (event.text)
      {
#if 0
       text<char_t> lineD2(event.text.c_str());
       const char_t* lineD3=(const char_t*)lineD2;
       DPRINTF(_l("%s"),lineD3);
#endif
       int hour1=0,min1=0,sec1=0,hunsec1=0;
       int hour2=0,min2=0,sec2=0,hunsec2=0;
       if (!(flags&this->PARSETIME) ||
           (swscanf(event.start.c_str(),L"%d:%d:%d.%d",&hour1, &min1, &sec1, &hunsec1)==4 &&
            swscanf(event.end.c_str()  ,L"%d:%d:%d.%d",&hour2, &min2, &sec2, &hunsec2)==4))
        {
         const TSubtitleProps *props=styles.getProps(event.style);
         TsubtitleText current(this->format,props?*props:defprops);
         current.defProps.lineID = lineID;
         strToIntMargin(event.marginL,&current.defProps.marginL);
         strToIntMargin(event.marginR,&current.defProps.marginR);
         strToIntMargin(event.marginV,&current.defProps.marginV);
         strToInt(event.layer,&current.defProps.layer);
         if (flags&this->PARSETIME)
          {
           current.start=timer.den*this->hmsToTime(hour1,min1,sec1,hunsec1)/timer.num;
           current.stop =timer.den*this->hmsToTime(hour2,min2,sec2,hunsec2)/timer.num;
          }
         else if (start != REFTIME_INVALID && stop != REFTIME_INVALID)
          {
           current.start = start;
           current.stop=stop;
          }
         current.defProps.tStart = current.defProps.karaokeStart = current.start;
         current.defProps.tStop = current.stop;

         // FIXME
         // \h removal : \h is hard space, so it should be replaced HARD sapce, soft space for band-aid.
         for (size_t i=0 ; i<event.text.size() ; i++)
         {
          if (event.text[i]=='\\' && event.text[i+1]=='h')
           {
            event.text[i]=0x20; // ' '
            event.text.erase(i+1,1);
           }
         }

         const wchar_t *line2=event.text.c_str();
         do
         {
          const wchar_t *tmp,*tmp1;
          int lineBreakReason = 0;
          do
           {
            tmp=strstr(line2, L"\\n");
            tmp1=strstr(line2, L"\\N");
            if (tmp == NULL && tmp1 == NULL)
             break;
            if (tmp && tmp1)
             tmp = std::min(tmp,tmp1);
            if (tmp == NULL)
             tmp = tmp1;
            current.addSSA(line2, tmp-line2, lineBreakReason);
            lineBreakReason = tmp[1] == 'n' ? 1 : 2;
            line2=tmp+2;
           } while (1);
          current.addSSA(line2, lineBreakReason);
         } while (flags&this->SSA_NODIALOGUE && fd.fgets((wchar_t*)(line2=line), this->LINE_LEN));
         return store(current);
        }
      }
    }
  }
 return NULL;
}

Tsubtitle* TsubtitleParserDunnowhat::parse(Tstream &fd,int flags, REFERENCE_TIME, REFERENCE_TIME) {
    wchar_t line[this->LINE_LEN+1];
    wchar_t text[this->LINE_LEN+1];

    if (!fd.fgets (line, this->LINE_LEN))
        return NULL;
    long start,stop;
    if (swscanf(line, L"%ld,%ld,\"%[^\"]", &start,
                &stop, text) <3)
        return NULL;
    TsubtitleText current(this->format);
    current.start=this->frameToTime(start);
    current.stop =this->frameToTime(stop );
    current.add(text);

    return store(current);
}

Tsubtitle* TsubtitleParserMPsub::parse(Tstream &fd,int flags, REFERENCE_TIME start, REFERENCE_TIME stop) {
        wchar_t line[this->LINE_LEN+1];
        float a,b;
        int num=0;
        wchar_t *p, *q;

        do
        {
                if (!fd.fgets(line, this->LINE_LEN)) return NULL;
        } while (swscanf (line, L"%f %f", &a, &b) !=2);
        TsubtitleText current(this->format);
        mpsub_position += a*(sub_uses_time ? 100.0 : 1.0);
        current.start=this->frameToTime(int(sub_uses_time?mpsub_position/100.0:mpsub_position));
        mpsub_position += b*(sub_uses_time ? 100.0 : 1.0);
        current.stop =this->frameToTime(int(sub_uses_time?mpsub_position/100.0:mpsub_position));

        while (1) {
                if (!fd.fgets (line, this->LINE_LEN)) {
                        if (num == 0) return NULL;
                        else return store(current);
                }
                p=line;
                while (iswspace((unsigned short)*p)) p++;
                if (eol(*p) && num > 0) return store(current);
                if (eol(*p)) return NULL;

                for (q=p; !eol(*q); q++);
                *q='\0';
                if (strlen(p)) {
                        current.add(p);
                        ++num;
                } else {
                        if (num) return store(current);
                        else return NULL;
                }
        }
        return NULL; // we should have returned before if it's OK
}

Tsubtitle* TsubtitleParserAqt::parse(Tstream &fd,int flags, REFERENCE_TIME, REFERENCE_TIME) {
    wchar_t line[this->LINE_LEN+1];
    const wchar_t *next;
    //int i;

    long start;
    while (1) {
    // try to locate next subtitle
        if (!fd.fgets (line, this->LINE_LEN))
                return NULL;
        if (!(swscanf(line, L"-->> %ld", &start) <1))
                break;
    }
    TsubtitleText current(this->format);
    current.start=this->frameToTime(start);
    if (previous != NULL)
        previous->stop = current.start-1;

    //previous_aqt_sub = current;

    if (!fd.fgets (line, this->LINE_LEN))
        return NULL;

    sub_readtext(line,current);
    current.stop = current.start; // will be corrected by next subtitle

    if (!fd.fgets (line, this->LINE_LEN))
     return previous=store(current);

    next = line;//,i=1;
    while ((next =sub_readtext (next,current))!=NULL)
     ;

    if (current.at(0)[0]=='\0' && current.at(1)[0]=='\0') {
        // void subtitle -> end of previous marked and exit
        previous = NULL;
        return NULL;
        }

    return previous=store(current);
}

Tsubtitle* TsubtitleParserSubrip09::parse(Tstream &fd,int flags, REFERENCE_TIME start, REFERENCE_TIME stop) {
    wchar_t line[this->LINE_LEN+1];
    int a1,a2,a3;
    const wchar_t * next=NULL;
    int i;

    while (1) {
    // try to locate next subtitle
        if (!fd.fgets (line, this->LINE_LEN))
                return NULL;
        if (!(swscanf(line, L"[%d:%d:%d]",&a1,&a2,&a3) < 3))
                break;
    }
    TsubtitleText current(this->format);
    current.start = this->hmsToTime(a1,a2,a3);
    if (previous != NULL)
        previous->stop = current.start-1;

    if (!fd.fgets (line, this->LINE_LEN))
        return NULL;

    next = line;i=0;

    //(*current)[0]=""; // just to be sure that string is clear

    while ((next =sub_readtext (next, current))!=NULL)
     i++;

    if (current.size()==0 || (current.at(0)[0]=='\0') && (i==0)) {
        // void subtitle -> end of previous marked and exit
        previous = NULL;
        return NULL;
        }

    return previous=store(current);
}

Tsubtitle* TsubtitleParserMPL2::parse(Tstream &fd,int flags, REFERENCE_TIME, REFERENCE_TIME) {
    wchar_t line[this->LINE_LEN+1];
    wchar_t line2[this->LINE_LEN+1];
    const wchar_t *p,*next;
    int start=0,end=0;

    do {
    if (!fd.fgets (line, this->LINE_LEN)) return NULL;
    } while (swscanf (line,
              L"[%ld][%ld]%[^\r\n]",
              &start, &end, line2) < 3);
    TsubtitleText current(this->format);
    current.start = (REFERENCE_TIME)start*1000000;
    current.stop = (REFERENCE_TIME)end*1000000;
    p=line2;
    next=p;
    while ((next =sub_readtext (next, current))!=NULL)
     ;
    return store(current);
}

TsubtitleParserBase* TsubtitleParserBase::getParser(int format,double fps,const TsubtitlesSettings *cfg,const Tconfig *ffcfg,Tsubreader *subreader,bool utf8,bool isEmbedded)
{
 switch (format&Tsubreader::SUB_FORMATMASK)
  {
   case Tsubreader::SUB_MICRODVD  :return new TsubtitleParserMicrodvd(format,fps,cfg,ffcfg,subreader);
   case Tsubreader::SUB_SUBRIP    :return new TsubtitleParserSubrip(format,fps,cfg,ffcfg,subreader);
   case Tsubreader::SUB_SUBVIEWER :return new TsubtitleParserSubviewer(format,fps,cfg,ffcfg,subreader);
   case Tsubreader::SUB_SAMI      :return new TsubtitleParserSami(format,fps,cfg,ffcfg,subreader);
   case Tsubreader::SUB_VPLAYER   :return new TsubtitleParserVplayer(format,fps,cfg,ffcfg,subreader);
   case Tsubreader::SUB_RT        :return new TsubtitleParserRt(format,fps,cfg,ffcfg,subreader);
   case Tsubreader::SUB_SSA       :return new TsubtitleParserSSA(format,fps,cfg,ffcfg,subreader,isEmbedded);
   case Tsubreader::SUB_DUNNOWHAT :return new TsubtitleParserDunnowhat(format,fps,cfg,ffcfg,subreader);
   case Tsubreader::SUB_MPSUB     :return new TsubtitleParserMPsub(format,fps,cfg,ffcfg,subreader);
   case Tsubreader::SUB_AQTITLE   :return new TsubtitleParserAqt(format,fps,cfg,ffcfg,subreader);
   case Tsubreader::SUB_SUBVIEWER2:return new TsubtitleParserSubviewer2(format,fps,cfg,ffcfg,subreader);
   case Tsubreader::SUB_SUBRIP09  :return new TsubtitleParserSubrip09(format,fps,cfg,ffcfg,subreader);
   case Tsubreader::SUB_MPL2      :return new TsubtitleParserMPL2(format,fps,cfg,ffcfg,subreader);
   default:return NULL;
  }
}

//======================================= TsubreaderMplayer =======================================
TsubreaderMplayer::TsubreaderMplayer(Tstream &fd,int sub_format,double fps,const TsubtitlesSettings *cfg,const Tconfig *ffcfg,bool isEmbedded)
{
 TsubtitleParserBase *parser=TsubtitleParserBase::getParser(sub_format,fps,cfg,ffcfg,this,false,isEmbedded);
 if (!parser) return;

 fd.rewind();

 while (parser->parse(fd))
  ;
 delete parser;

 processDuration(cfg);

 if (cfg->timeoverlap && !empty())
  {
   for (iterator s=begin();s!=end()-1;s++)
    { // without these braces, processOverlap() is executed multiple times... (MSVC8)
     if ((*s)->stop<(*s)->start)
      (*s)->stop=(*(s+1))->start-1;
    }
   processOverlap();
  }
}
