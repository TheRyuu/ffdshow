/*
 * Copyright (c) 2004-2006 Milan Cutka
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "stdafx.h"
#include "TfontSettings.h"
#include "ffdshow_constants.h"
#include "reg.h"

//=========================================== TfontSettings ==========================================
const TfontSettings::Tweigth TfontSettings::weights[]=
{
 _l("thin")      ,FW_THIN,
 _l("extralight"),FW_EXTRALIGHT,
 _l("light")     ,FW_LIGHT,
 _l("normal")    ,FW_NORMAL,
 _l("medium")    ,FW_MEDIUM,
 _l("semibold")  ,FW_SEMIBOLD,
 _l("bold")      ,FW_BOLD,
 _l("extrabold") ,FW_EXTRABOLD,
 _l("heavy")     ,FW_HEAVY,
 NULL
};

const int TfontSettings::charsets[]=
{
 ANSI_CHARSET,DEFAULT_CHARSET,SYMBOL_CHARSET,SHIFTJIS_CHARSET,HANGUL_CHARSET,GB2312_CHARSET,CHINESEBIG5_CHARSET,OEM_CHARSET,JOHAB_CHARSET,HEBREW_CHARSET,ARABIC_CHARSET,GREEK_CHARSET,TURKISH_CHARSET,VIETNAMESE_CHARSET,THAI_CHARSET,EASTEUROPE_CHARSET,RUSSIAN_CHARSET,MAC_CHARSET,BALTIC_CHARSET,-1
};
const char_t* TfontSettings::getCharset(int i)
{
 switch (i)
  {
   case ANSI_CHARSET       :return _l("Ansi");
   case DEFAULT_CHARSET    :return _l("Default");
   case SYMBOL_CHARSET     :return _l("Symbol");
   case SHIFTJIS_CHARSET   :return _l("Shiftjis");
   case HANGUL_CHARSET     :return _l("Hangul");
   case GB2312_CHARSET     :return _l("Gb2312");
   case CHINESEBIG5_CHARSET:return _l("Chinese");
   case OEM_CHARSET        :return _l("OEM");
   case JOHAB_CHARSET      :return _l("Johab");
   case HEBREW_CHARSET     :return _l("Hebrew");
   case ARABIC_CHARSET     :return _l("Arabic");
   case GREEK_CHARSET      :return _l("Greek");
   case TURKISH_CHARSET    :return _l("Turkish");
   case VIETNAMESE_CHARSET :return _l("Vietnamese");
   case THAI_CHARSET       :return _l("Thai");
   case EASTEUROPE_CHARSET :return _l("Easteurope");
   case RUSSIAN_CHARSET    :return _l("Cyrillic");
   case MAC_CHARSET        :return _l("Mac");
   case BALTIC_CHARSET     :return _l("Baltic");
   default                 :return _l("unknown");
  }
}  
int TfontSettings::getCharset(const char_t *name)
{
 if      (stricmp(name,_l("Default"))==0) return DEFAULT_CHARSET;
 else if (stricmp(name,_l("Symbol"))==0) return SYMBOL_CHARSET;
 else if (stricmp(name,_l("Shiftjis"))==0) return SHIFTJIS_CHARSET;
 else if (stricmp(name,_l("Hangul"))==0) return HANGUL_CHARSET;
 else if (stricmp(name,_l("Gb2312"))==0) return GB2312_CHARSET;
 else if (stricmp(name,_l("Chinese"))==0) return CHINESEBIG5_CHARSET;
 else if (stricmp(name,_l("OEM"))==0) return OEM_CHARSET;
 else if (stricmp(name,_l("Johab"))==0) return JOHAB_CHARSET;
 else if (stricmp(name,_l("Hebrew"))==0) return HEBREW_CHARSET;
 else if (stricmp(name,_l("Arabic"))==0) return ARABIC_CHARSET;
 else if (stricmp(name,_l("Greek"))==0) return GREEK_CHARSET;
 else if (stricmp(name,_l("Turkish"))==0) return TURKISH_CHARSET;
 else if (stricmp(name,_l("Vietnamese"))==0) return VIETNAMESE_CHARSET;
 else if (stricmp(name,_l("Thai"))==0) return THAI_CHARSET;
 else if (stricmp(name,_l("Easteurope"))==0) return EASTEUROPE_CHARSET;
 else if (stricmp(name,_l("Russian"))==0 || stricmp(name,_l("Cyrillic"))==0) return RUSSIAN_CHARSET;
 else if (stricmp(name,_l("Mac"))==0) return MAC_CHARSET;
 else if (stricmp(name,_l("Baltic"))==0) return BALTIC_CHARSET;
 else return ANSI_CHARSET;
}

void TfontSettings::reg_op(TregOp &t)
{
 Toptions::reg_op(t);

#ifndef UNICODE
 char_t lang[20];
 TregOpRegRead tl(HKEY_CURRENT_USER,FFDSHOW_REG_PARENT _l("\\") FFDSHOW);
 tl._REG_OP_S(IDFF_lang,_l("lang"),lang,20,_l(""));

 if (lang[0]=='\0')
  {
   TregOpRegRead tNSI(HKEY_LOCAL_MACHINE,FFDSHOW_REG_PARENT _l("\\") FFDSHOW);
   char_t langId[MAX_PATH];
   tNSI._REG_OP_S(0,_l("lang"),langId,MAX_PATH,_l("1033"));
   if(strncmp(langId,_l("1041"),4)==0)
    {lang[0]='J';lang[1]='P';}
  }

 if ((lang[0]=='J' || lang[0]=='j') && (lang[0]=='J'  || lang[1]=='P' || lang[1]=='a'|| lang[1]=='p')) /* Japanese ANSI or Unicode */
  charset= SHIFTJIS_CHARSET;
#endif
}

TfontSettings::TfontSettings(TintStrColl *Icoll):Toptions(Icoll)
{
 memset(name,0,sizeof(name));
}

//========================================= TfontSettingsOSD =========================================
TfontSettingsOSD::TfontSettingsOSD(TintStrColl *Icoll):TfontSettings(Icoll)
{
 autosize=0;
 sizeA=50;
 split=0;
 autosizeVideoWindow=0;

 static const TintOptionT<TfontSettings> iopts[]=
  {
   IDFF_OSDfontCharset       ,&TfontSettings::charset        ,1,1,_l(""),0,
     _l("OSDfontCharset"), DEFAULT_CHARSET,
   IDFF_OSDfontSize          ,&TfontSettings::sizeP          ,2,255,_l(""),0,
     _l("OSDfontSize"), 17,
   IDFF_OSDfontWeight        ,&TfontSettings::weight         ,0,900,_l(""),0,
     _l("OSDfontWeight"), FW_NORMAL,
   IDFF_OSDfontColor         ,&TfontSettings::color          ,1,1,_l(""),0,
     _l("OSDfontColor"), RGB(128,255,0),
   IDFF_OSDfontShadowStrength,&TfontSettings::shadowStrength ,0,100,_l(""),0,
     _l("OSDfontShadowStrength"), 18,
   IDFF_OSDfontShadowRadius  ,&TfontSettings::shadowRadius   ,1,100,_l(""),0,
     _l("OSDfontShadowRadius"), 15,
   IDFF_OSDfontSpacing       ,&TfontSettings::spacing        ,-10,10,_l(""),0,
     _l("OSDfontSpacing"), 0,
   IDFF_OSDfontXscale        ,&TfontSettings::xscale         ,10,300,_l(""),0,
     _l("OSDfontXscale"), 100,
   IDFF_OSDfontFast          ,&TfontSettings::fast           ,0,0,_l(""),0,
     _l("OSDfontFast"), 1,
   0
  };
 addOptions(iopts); 
 static const TstrOption sopts[]=
  {
   IDFF_OSDfontName,(TstrVal)&TfontSettings::name,LF_FACESIZE,_l(""),0,
     _l("OSDfontName"), _l("Arial"), 
   0
  };
 addOptions(sopts); 
 static const TcreateParamList2<TfontSettings::Tweigth> listOSDfontWeigth(weights,&Tweigth::name);setParamList(IDFF_OSDfontWeight,&listOSDfontWeigth);
}

//========================================= TfontSettingsSub =========================================
TfontSettingsSub::TfontSettingsSub(TintStrColl *Icoll):TfontSettings(Icoll)
{
 static const Toptions::TintOptionT<TfontSettings> iopts[]=
  {
   IDFF_fontCharset            ,&TfontSettings::charset            ,1,1,_l(""),1,
     _l("fontCharset"), DEFAULT_CHARSET,
   IDFF_fontAutosize           ,&TfontSettings::autosize           ,0,0,_l(""),1,
     _l("fontAutosize"), 0,
   IDFF_fontAutosizeVideoWindow,&TfontSettings::autosizeVideoWindow,0,0,_l(""),1,
     _l("fontAutosizeVideoWindow"), 0,
   IDFF_fontSizeP              ,&TfontSettings::sizeP              ,2,255,_l(""),1,
     _l("fontSize"), 26,
   IDFF_fontSizeA              ,&TfontSettings::sizeA              ,1,100,_l(""),1,
     _l("fontSizeA"), 60,
   IDFF_fontWeight             ,&TfontSettings::weight             ,0,900,_l(""),1,
     _l("fontWeight"), FW_NORMAL,
   IDFF_fontColor              ,&TfontSettings::color              ,1,1,_l(""),1,
     _l("fontColor"), RGB(255,255,255),
   IDFF_fontShadowStrength     ,&TfontSettings::shadowStrength     ,0,100,_l(""),1,
     _l("fontShadowStrength"), 90,
   IDFF_fontShadowRadius       ,&TfontSettings::shadowRadius       ,1,100,_l(""),1,
     _l("fontShadowRadius"), 50,
   IDFF_fontSpacing            ,&TfontSettings::spacing            ,-10,10,_l(""),1,
     _l("fontSpacing"), 0,
   IDFF_fontSplitting          ,&TfontSettings::split              ,0,0,_l(""),1,
     _l("fontSplitting"), 1,
   IDFF_fontXscale             ,&TfontSettings::xscale             ,10,300,_l(""),1,
     _l("fontXscale"), 100,
   IDFF_fontFast               ,&TfontSettings::fast               ,0,0,_l(""),1,
     _l("fontFast"), 0,
   0
  }; 
 addOptions(iopts); 
 static const Toptions::TstrOption sopts[]=
  {
   IDFF_fontName,(TstrVal)&TfontSettings::name,LF_FACESIZE,_l(""),1,
     _l("fontName"), _l("Arial"), 
   0
  };
 addOptions(sopts); 

 static const TcreateParamList2<TfontSettings::Tweigth> listFontWeigth(weights,&Tweigth::name);setParamList(IDFF_fontWeight,&listFontWeigth);
}

void TfontSettingsSub::reg_op(TregOp &t)
{
 TfontSettings::reg_op(t);
 if (weight>900) weight=900;
}
