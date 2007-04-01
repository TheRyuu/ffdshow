/*
 * Copyright (c) 2002-2006 Milan Cutka
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
#include "Cfont.h"
#include "TfontSettings.h"
#include "IffdshowParamsEnum.h"
#include "reg.h"
#include <tchar.h>
#include <string.h>

int CALLBACK TfontPage::EnumFamCallBackCharsets(CONST LOGFONT *lpelf,CONST TEXTMETRIC *lpntm,DWORD FontType,LPARAM lParam)
{
 if (FontType&(TRUETYPE_FONTTYPE|DEVICE_FONTTYPE))
  {
   ints *sl=(ints*)lParam;
   ints::iterator i=std::find(sl->begin(),sl->end(),int(lpelf->lfCharSet));
   if (i==sl->end())
    sl->push_back(lpelf->lfCharSet);
  }
 return 1;
}

int CALLBACK TfontPage::EnumFamCallBackFonts(CONST LOGFONT *lpelf,CONST TEXTMETRIC *lpntm,DWORD FontType,LPARAM lParam)
{
 if (FontType&(TRUETYPE_FONTTYPE|DEVICE_FONTTYPE)/* && lpelf->lfCharSet==0*/)
  {
   strings *sl=(strings*)lParam;
   strings::iterator i=std::find(sl->begin(),sl->end(),ffstring(lpelf->lfFaceName));
   if (i==sl->end())
    sl->push_back(lpelf->lfFaceName);
  }
 return 1;
}

void TfontPage::init(void)
{
 tbrSetRange(IDC_TBR_FONT_SPACING,-10,10,2);
 tbrSetRange(IDC_TBR_FONT_OUTLINE_STRENGTH,0,100,10);
 tbrSetRange(IDC_TBR_FONT_OUTLINE_RADIUS,1,100,10);
 tbrSetRange(IDC_TBR_FONT_XSCALE,30,300);
 tbrSetRange(IDC_TBR_FONT_SUBSHADOW_SIZE,0,20, 1);
 tbrSetRange(IDC_TBR_FONT_SUBSHADOW_ALPHA,0,255, 10);

 strings sl;
 LOGFONT lf;lf.lfCharSet=DEFAULT_CHARSET;lf.lfPitchAndFamily=0;lf.lfFaceName[0]='\0';
 HDC dc=GetDC(m_hwnd);
 EnumFontFamiliesEx(dc,&lf,EnumFamCallBackFonts,LPARAM(&sl),0);
 ReleaseDC(m_hwnd,dc);
 for (strings::const_iterator il=sl.begin();il!=sl.end();il++)
  cbxAdd(IDC_CBX_FONT_NAME,il->c_str());

 cbxCharset=GetDlgItem(m_hwnd,IDC_CBX_FONT_CHARSET);
 boldFont=NULL;
}

void TfontPage::selectCharset(int ii)
{
 int cnt=(int)SendMessage(cbxCharset,CB_GETCOUNT,0,0);
 for (int i=0;i<cnt;i++)
  {
   int iii=(int)cbxGetItemData(IDC_CBX_FONT_CHARSET,i);
   if (ii==iii)
    {
     cbxSetCurSel(IDC_CBX_FONT_CHARSET,i);
     return;
    }
  }
 cbxSetCurSel(IDC_CBX_FONT_CHARSET,0);
 cfgSet(idff_fontcharset,(int)cbxGetItemData(IDC_CBX_FONT_CHARSET,0));
}

void TfontPage::fillCharsets(void)
{
 int oldi=cbxGetCurSel(IDC_CBX_FONT_CHARSET);
 int oldii=(oldi!=CB_ERR)?(int)cbxGetItemData(IDC_CBX_FONT_CHARSET,oldi):ANSI_CHARSET;

 cbxClear(IDC_CBX_FONT_CHARSET);memset(validCharsets,0,sizeof(validCharsets));
 ints sl;
 LOGFONT lf;
 cfgGet(idff_fontname,lf.lfFaceName,LF_FACESIZE);
 lf.lfCharSet=DEFAULT_CHARSET;lf.lfPitchAndFamily=0;
 HDC dc=GetDC(m_hwnd);
 EnumFontFamiliesEx(dc,&lf,EnumFamCallBackCharsets,LPARAM(&sl),0);
 ReleaseDC(m_hwnd,dc);
 for (int i=0;TfontSettings::charsets[i]!=-1;i++)
  {
   int data=TfontSettings::charsets[i];
   if (isIn(sl,data))
    validCharsets[data]=true;
   cbxAdd(IDC_CBX_FONT_CHARSET,_(IDC_CBX_FONT_CHARSET,TfontSettings::getCharset(data)),data);
  }
 selectCharset(oldii);
}

void TfontPage::cfg2dlg(void)
{
 font2dlg();
}

void TfontPage::font2dlg(void)
{
 size2dlg();
 shadow2dlg();
 spacingxscale2dlg();
 shadowSize2dlg();
 shadowAlpha2dlg();
 
 cbxSetDataCurSel(IDC_CBX_FONT_WEIGHT,cfgGet(idff_fontweight));
 SendDlgItemMessage(m_hwnd,IDC_CBX_FONT_NAME,CB_SELECTSTRING,WPARAM(-1),LPARAM(cfgGetStr(idff_fontname)));
 fillCharsets();
#ifdef UNICODE
 selectCharset(cfgGet(idff_fontcharset));
#else
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

 if ((lang[0]=='J' || lang[0]=='j') && (lang[1]=='A' || lang[1]=='P' || lang[1]=='a'|| lang[1]=='p')) /* Japanese ANSI or Unicode */
  selectCharset(SHIFTJIS_CHARSET);
 else
  selectCharset(cfgGet(idff_fontcharset));
#endif
 setCheck(IDC_CHB_FONT_FAST,cfgGet(idff_fontfast));
 repaint(GetDlgItem(m_hwnd,IDC_IMG_FONT_COLOR));
}
void TfontPage::size2dlg(void)
{
 int aut;
 if (idff_fontautosize)
  {
   aut=cfgGet(idff_fontautosize);
   setCheck(IDC_CHB_FONT_AUTOSIZE,aut);
   enable(1,IDC_CHB_FONT_AUTOSIZE);
  }
 else
  {
   aut=0;
   enable(0,IDC_CHB_FONT_AUTOSIZE);
  }
 if (aut)
  {
   tbrSetRange(IDC_TBR_FONT_SIZE,0,100,5);
   tbrSet(IDC_TBR_FONT_SIZE,cfgGet(idff_fontsizea),IDC_LBL_FONT_SIZE);
   enable(1,IDC_CHB_FONT_AUTOSIZE_VIDEOWINDOW);
  }
 else
  {
   tbrSetRange(IDC_TBR_FONT_SIZE,3,127,6);
   tbrSet(IDC_TBR_FONT_SIZE,cfgGet(idff_fontsizep),IDC_LBL_FONT_SIZE);
   enable(0,IDC_CHB_FONT_AUTOSIZE_VIDEOWINDOW);
  }
 setCheck(IDC_CHB_FONT_AUTOSIZE_VIDEOWINDOW,cfgGet(IDFF_fontAutosizeVideoWindow));
}
void TfontPage::spacingxscale2dlg(void)
{
 if (idff_fontxscale)
  {
   int xscale=cfgGet(idff_fontxscale);
   tbrSet(IDC_TBR_FONT_XSCALE,cfgGet(idff_fontxscale));
   setText(IDC_LBL_FONT_XSCALE,_l("%s %i%%"),_(IDC_LBL_FONT_XSCALE),xscale);
  }
 tbrSet(IDC_TBR_FONT_SPACING,cfgGet(idff_fontspacing),IDC_LBL_FONT_SPACING);
}

INT_PTR TfontPage::msgProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
 switch (uMsg)
  {
   case WM_DESTROY:
    if (boldFont) DeleteObject(boldFont);boldFont=NULL;
    break;
   case WM_HSCROLL:
    switch (getId(HWND(lParam)))
     {
      case IDC_TBR_FONT_SIZE:
       if (cfgGet(idff_fontautosize))
        cfgSet(idff_fontsizea,tbrGet(IDC_TBR_FONT_SIZE));
       else
        cfgSet(idff_fontsizep,tbrGet(IDC_TBR_FONT_SIZE));
       size2dlg();
       return TRUE;
     }
    break;
   case WM_COMMAND:
    switch (LOWORD(wParam))
     {
      case IDC_CHB_FONT_AUTOSIZE:
       if (idff_fontautosize)
        {
         cfgSet(idff_fontautosize,getCheck(IDC_CHB_FONT_AUTOSIZE));
         size2dlg();
        }
       return TRUE;
      case IDC_CHB_FONT_AUTOSIZE_VIDEOWINDOW:
       if (idff_fontautosizevideowindow)
        cfgSet(idff_fontautosizevideowindow,getCheck(IDC_CHB_FONT_AUTOSIZE_VIDEOWINDOW));
       return TRUE;
      case IDC_IMG_FONT_COLOR:
       if (HIWORD(wParam)==STN_CLICKED)
        {
         if (chooseColor(idff_fontcolor))
          {
           font2dlg();
           repaint(GetDlgItem(m_hwnd,IDC_IMG_FONT_COLOR));
          }
         return TRUE;
        }
       break;
     }
    break;
   case WM_DRAWITEM:
    if (wParam==IDC_IMG_FONT_COLOR)
     {
      LPDRAWITEMSTRUCT dis=LPDRAWITEMSTRUCT(lParam);
      LOGBRUSH lb;
      lb.lbColor=cfgGet(idff_fontcolor);
      lb.lbStyle=BS_SOLID;
      HBRUSH br=CreateBrushIndirect(&lb);
      FillRect(dis->hDC,&dis->rcItem,br);
      DeleteObject(br);
      return TRUE;
     }
    else if (wParam==IDC_CBX_FONT_CHARSET)
     {
      DRAWITEMSTRUCT *dis=(DRAWITEMSTRUCT*)lParam;
      COLORREF crOldTextColor=GetTextColor(dis->hDC);
      COLORREF crOldBkColor=GetBkColor(dis->hDC);
      HBRUSH br;
      if ((dis->itemAction|ODA_SELECT) && (dis->itemState&ODS_SELECTED))
       {
        SetTextColor(dis->hDC,GetSysColor(COLOR_HIGHLIGHTTEXT));
        SetBkColor(dis->hDC,GetSysColor(COLOR_HIGHLIGHT));
        br=CreateSolidBrush(GetSysColor(COLOR_HIGHLIGHT));
       }
      else
       br=CreateSolidBrush(crOldBkColor);
      FillRect(dis->hDC,&dis->rcItem,br);
      if (int(dis->itemData)!=CB_ERR)
       {
        RECT r=dis->rcItem;r.left+=2;
        char_t text[70];SendMessage(cbxCharset,CB_GETLBTEXT,dis->itemID,LPARAM(text));
        if (!boldFont)
         {
          LOGFONT oldFont;
          HFONT hf=(HFONT)GetCurrentObject(dis->hDC,OBJ_FONT);
          GetObject(hf,sizeof(LOGFONT),&oldFont);
          oldFont.lfWeight=FW_BLACK;
          boldFont=CreateFontIndirect(&oldFont);
         }
        HGDIOBJ oldfont=NULL;
        if (validCharsets[dis->itemData])
         oldfont=SelectObject(dis->hDC,boldFont);
        DrawText(dis->hDC,text,(int)strlen(text),&r,DT_LEFT|DT_SINGLELINE|DT_VCENTER);
        if (oldfont)
         SelectObject(dis->hDC,oldfont);
       }
      SetTextColor(dis->hDC,crOldTextColor);
      SetBkColor(dis->hDC,crOldBkColor);
      DeleteObject(br);
      return TRUE;
     }
    else
     break;
  }
 return TconfPageDecVideo::msgProc(uMsg,wParam,lParam);
}

Twidget* TfontPage::createDlgItem(int id,HWND h)
{
 if (id==IDC_TBR_FONT_SIZE)
  {
   static const TbindTrackbar<TfontPageSubtitles> htbr={IDC_TBR_FONT_SIZE,IDFF_fontSizeA,NULL};
   return new TwidgetSubclassTbr(h,this,TbindTrackbars(&htbr));
  }
 else
  return TconfPageDecVideo::createDlgItem(id,h);
}
int TfontPage::getTbrIdff(int id,const TbindTrackbars bind)
{
 return getCheck(IDC_CHB_FONT_AUTOSIZE)?idff_fontsizea:idff_fontsizep;
}

bool TfontPage::reset(bool testonly)
{
 if (!testonly)
  {
   deci->resetParam(idff_fontname);
   deci->resetParam(idff_fontcharset);
   deci->resetParam(idff_fontautosize);
   deci->resetParam(idff_fontsizep);
   deci->resetParam(idff_fontsizea);
   deci->resetParam(idff_fontweight);
   deci->resetParam(idff_fontcolor);
   deci->resetParam(idff_fontoutlinestrength);
   deci->resetParam(idff_fontoutlineradius);
   deci->resetParam(idff_fontspacing);
   deci->resetParam(idff_fontxscale);
   deci->resetParam(idff_fontfast);
   deci->resetParam(idff_subshadowmode);
   deci->resetParam(idff_subshadowsize);
   deci->resetParam(idff_subshadowalpha);
  }
 return true;
}

void TfontPage::translate(void)
{
 TconfPageBase::translate();

 int sel=cbxGetCurSel(IDC_CBX_FONT_WEIGHT);
 cbxClear(IDC_CBX_FONT_WEIGHT);
 for (int i=0;TfontSettings::weights[i].name;i++)
  cbxAdd(IDC_CBX_FONT_WEIGHT,_(IDC_CBX_FONT_WEIGHT,TfontSettings::weights[i].name),TfontSettings::weights[i].id);
 cbxSetCurSel(IDC_CBX_FONT_WEIGHT,sel);

 cbxTranslate(IDC_CBX_FONT_SUBSHADOW_MODE,TfontSettings::shadowModes);
}

TfontPage::TfontPage(TffdshowPageDec *Iparent,const TfilterIDFF *idff,int IfilterPageId):TconfPageDecVideo(Iparent,idff,IfilterPageId)
{
 dialogId=IDD_FONT;
}

//====================================== TfontPageSubtitles =====================================
TfontPageSubtitles::TfontPageSubtitles(TffdshowPageDec *Iparent,const TfilterIDFF *idff):TfontPage(Iparent,idff,2)
{
 idff_fontcharset=IDFF_fontCharset;
 idff_fontname=IDFF_fontName;
 idff_fontautosize=IDFF_fontAutosize;
 idff_fontautosizevideowindow=IDFF_fontAutosizeVideoWindow;
 idff_fontsizep=IDFF_fontSizeP;
 idff_fontsizea=IDFF_fontSizeA;
 idff_fontspacing=IDFF_fontSpacing;
 idff_fontoutlinestrength=IDFF_fontOutlineStrength;
 idff_fontoutlineradius=IDFF_fontOutlineRadius;
 idff_fontweight=IDFF_fontWeight;
 idff_fontcolor=IDFF_fontColor;
 idff_fontxscale=IDFF_fontXscale;
 idff_fontfast=IDFF_fontFast;
 idff_subshadowmode=IDFF_fontShadowMode;
 idff_subshadowalpha=IDFF_fontShadowAlpha;
 idff_subshadowsize=IDFF_fontShadowSize;
 static const TbindCheckbox<TfontPageSubtitles> chb[]=
  {
   IDC_CHB_FONT_FAST,IDFF_fontFast,NULL,
   0,NULL,NULL
  };
 bindCheckboxes(chb);
 static const TbindTrackbar<TfontPageSubtitles> htbr[]=
  {
   IDC_TBR_FONT_SPACING,IDFF_fontSpacing,&TfontPageSubtitles::spacingxscale2dlg,
   IDC_TBR_FONT_OUTLINE_STRENGTH,IDFF_fontOutlineStrength,&TfontPageSubtitles::shadow2dlg,
   IDC_TBR_FONT_OUTLINE_RADIUS,IDFF_fontOutlineRadius,&TfontPageSubtitles::shadow2dlg,
   IDC_TBR_FONT_XSCALE,IDFF_fontXscale,&TfontPageSubtitles::spacingxscale2dlg,
   IDC_TBR_FONT_SUBSHADOW_SIZE,IDFF_fontShadowSize,&TfontPageSubtitles::shadowSize2dlg,
   IDC_TBR_FONT_SUBSHADOW_ALPHA,IDFF_fontShadowAlpha,&TfontPageSubtitles::shadowAlpha2dlg,
   0,0,NULL
  };
 bindHtracks(htbr);
 static const TbindCombobox<TfontPageSubtitles> cbx[]=
  {
   IDC_CBX_FONT_CHARSET,IDFF_fontCharset,BINDCBX_DATA,NULL,
   IDC_CBX_FONT_WEIGHT,IDFF_fontWeight,BINDCBX_DATA,NULL,
   IDC_CBX_FONT_NAME,IDFF_fontName,BINDCBX_TEXT,&TfontPageSubtitles::fillCharsets,
   IDC_CBX_FONT_SUBSHADOW_MODE,IDFF_fontShadowMode,BINDCBX_SEL,&TfontPageSubtitles::font2dlg,
   0
  };
 bindComboboxes(cbx);
}

void TfontPageSubtitles::shadow2dlg(void)
{
 int shadowmode=cfgGet(idff_subshadowmode);
 int outlineradius=cfgGet(idff_fontoutlineradius);
 tbrSet(IDC_TBR_FONT_OUTLINE_STRENGTH,cfgGet(idff_fontoutlinestrength),IDC_LBL_FONT_OUTLINE_STRENGTH);
 tbrSet(IDC_TBR_FONT_OUTLINE_RADIUS,outlineradius,IDC_LBL_FONT_OUTLINE_RADIUS);
 cbxSetCurSel(IDC_CBX_FONT_SUBSHADOW_MODE,shadowmode);
 static const int idShadows[]={IDC_LBL_FONT_SUBSHADOW_ALPHA,IDC_TBR_FONT_SUBSHADOW_ALPHA,IDC_LBL_FONT_SUBSHADOW_SIZE,IDC_TBR_FONT_SUBSHADOW_SIZE,0};
 enable(shadowmode!=3,idShadows);
 enable(shadowmode==3 || outlineradius==0,IDC_CHB_FONT_FAST);
}

void TfontPageSubtitles::shadowSize2dlg(void)
{
 if (idff_subshadowsize)
  {
   int shadowmode=cfgGet(idff_subshadowmode);
   int subshadowsize=cfgGet(idff_subshadowsize);
   tbrSet(IDC_TBR_FONT_SUBSHADOW_SIZE,cfgGet(idff_subshadowsize));
   if (subshadowsize == 0)
	setText(IDC_LBL_FONT_SUBSHADOW_SIZE,_l("%s disabled"),_(IDC_LBL_FONT_SUBSHADOW_SIZE),subshadowsize);
   else
	setText(IDC_LBL_FONT_SUBSHADOW_SIZE,_l("%s %i"),_(IDC_LBL_FONT_SUBSHADOW_SIZE),subshadowsize);
   enable(shadowmode==3 || subshadowsize==0,IDC_CHB_FONT_FAST);
  }
}

void TfontPageSubtitles::shadowAlpha2dlg(void)
{
  if (idff_subshadowalpha)
  {
   int subshadowalpha=cfgGet(idff_subshadowalpha);
   int displayValue = (int)subshadowalpha*100/255;
   tbrSet(IDC_TBR_FONT_SUBSHADOW_ALPHA,cfgGet(idff_subshadowalpha));
   if (displayValue == 0)
	   setText(IDC_LBL_FONT_SUBSHADOW_ALPHA,_l("%s transparent"),_(IDC_LBL_FONT_SUBSHADOW_ALPHA));
   else if (displayValue == 100)
	   setText(IDC_LBL_FONT_SUBSHADOW_ALPHA,_l("%s opaque"),_(IDC_LBL_FONT_SUBSHADOW_ALPHA));
   else
	   setText(IDC_LBL_FONT_SUBSHADOW_ALPHA,_l("%s %i%%"),_(IDC_LBL_FONT_SUBSHADOW_ALPHA), displayValue);

  }
}

//========================================= TfontPageOSD ========================================
TfontPageOSD::TfontPageOSD(TffdshowPageDec *Iparent):TfontPage(Iparent)
{
 idff_fontcharset=IDFF_OSDfontCharset;
 idff_fontname=IDFF_OSDfontName;
 idff_fontautosize=0;
 idff_fontautosizevideowindow=0;
 idff_fontsizep=IDFF_OSDfontSize;
 idff_fontsizea=0;
 idff_fontspacing=IDFF_OSDfontSpacing;
 idff_fontoutlinestrength=IDFF_OSDfontOutlineStrength;
 idff_fontoutlineradius=IDFF_OSDfontOutlineRadius;
 idff_fontweight=IDFF_OSDfontWeight;
 idff_fontcolor=IDFF_OSDfontColor;
 idff_fontxscale=IDFF_OSDfontXscale;
 idff_fontfast=IDFF_OSDfontFast;
 static const TbindCheckbox<TfontPageOSD> chb[]=
  {
   IDC_CHB_FONT_FAST,IDFF_OSDfontFast,NULL,
   0,NULL,NULL
  };
 bindCheckboxes(chb);
 static const TbindTrackbar<TfontPageOSD> htbr[]=
  {
   IDC_TBR_FONT_SPACING,IDFF_OSDfontSpacing,&TfontPageOSD::spacingxscale2dlg,
   IDC_TBR_FONT_OUTLINE_STRENGTH,IDFF_OSDfontOutlineStrength,&TfontPageOSD::shadow2dlg,
   IDC_TBR_FONT_OUTLINE_RADIUS,IDFF_OSDfontOutlineRadius,&TfontPageOSD::shadow2dlg,
   IDC_TBR_FONT_XSCALE,IDFF_OSDfontXscale,&TfontPageOSD::spacingxscale2dlg,
   0,0,NULL
  };
 bindHtracks(htbr);
 static const TbindCombobox<TfontPageOSD> cbx[]=
  {
   IDC_CBX_FONT_CHARSET,IDFF_OSDfontCharset,BINDCBX_DATA,NULL,
   IDC_CBX_FONT_WEIGHT,IDFF_OSDfontWeight,BINDCBX_DATA,NULL,
   IDC_CBX_FONT_NAME,IDFF_OSDfontName,BINDCBX_TEXT,&TfontPageOSD::fillCharsets,
   0
  };
 bindComboboxes(cbx);
}

void TfontPageOSD::shadow2dlg(void)
{
 int outlineradius=cfgGet(idff_fontoutlineradius);
 tbrSet(IDC_TBR_FONT_OUTLINE_STRENGTH,cfgGet(idff_fontoutlinestrength),IDC_LBL_FONT_OUTLINE_STRENGTH);
 tbrSet(IDC_TBR_FONT_OUTLINE_RADIUS,outlineradius,IDC_LBL_FONT_OUTLINE_RADIUS);
 static const int idShadows[]={IDC_LBL_FONT_SUBSHADOW_MODE,IDC_CBX_FONT_SUBSHADOW_MODE,IDC_LBL_FONT_SUBSHADOW_ALPHA,IDC_TBR_FONT_SUBSHADOW_ALPHA,IDC_LBL_FONT_SUBSHADOW_SIZE,IDC_TBR_FONT_SUBSHADOW_SIZE,0};
 enable(0,idShadows);
}
