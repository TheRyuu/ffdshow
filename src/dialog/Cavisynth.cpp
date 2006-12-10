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
#include "Cavisynth.h"
#include "TffdshowPageDec.h"

const char_t* TavisynthPage::avs_mask=_l("Avisynth scripts (*.avs)\0*.avs\0\0");

void TavisynthPage::init(void)
{
 static const TanchorInfo ainfo[]=
  {
   IDC_GRP_AVISYNTH,TanchorInfo::LEFT|TanchorInfo::RIGHT|TanchorInfo::BOTTOM|TanchorInfo::TOP,
   IDC_BT_AVS_LOAD,TanchorInfo::RIGHT|TanchorInfo::TOP,
   IDC_BT_AVS_SAVE,TanchorInfo::RIGHT|TanchorInfo::TOP,
   IDC_ED_AVISYNTH,TanchorInfo::LEFT|TanchorInfo::RIGHT|TanchorInfo::BOTTOM|TanchorInfo::TOP,
   0,0
  };
 anchors.init(ainfo,*this); 
 edLimitText(IDC_ED_AVISYNTH,2048);
}

void TavisynthPage::cfg2dlg(void)
{
 setCheck(IDC_CHB_AVISYNTH_YV12 ,cfgGet(IDFF_avisynthInYV12 ));
 setCheck(IDC_CHB_AVISYNTH_YUY2 ,cfgGet(IDFF_avisynthInYUY2 ));
 setCheck(IDC_CHB_AVISYNTH_RGB32,cfgGet(IDFF_avisynthInRGB32));
 setCheck(IDC_CHB_AVISYNTH_RGB24,cfgGet(IDFF_avisynthInRGB24));
 setCheck(IDC_CHB_AVISYNTH_FFDSHOW,cfgGet(IDFF_avisynthFfdshowSource));
 setDlgItemText(m_hwnd,IDC_ED_AVISYNTH,cfgGetStr(IDFF_avisynthScript));
}

INT_PTR TavisynthPage::msgProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
 int c;
 switch (uMsg)
  {
   case WM_COMMAND:
    switch (LOWORD(wParam))  
     {
      case IDC_CHB_AVISYNTH:
       c=getCheck(IDC_CHB_AVISYNTH);
       cfgSet(IDFF_isAvisynth,c);
       if(!c)
        cfgSet(IDFF_OSDuser,_l(""));
       break;
      case IDC_ED_AVISYNTH:
       if (HIWORD(wParam)==EN_CHANGE && !isSetWindowText) 
        {
         parent->setChange();
         return TRUE;
        }
       break;   
     }
    break;
  }
 return TconfPageDecVideo::msgProc(uMsg,wParam,lParam);
}

void TavisynthPage::onLoad(void)
{
 char_t scriptflnm[MAX_PATH]=_l("");
 if (dlgGetFile(false,m_hwnd,_(-IDD_AVISYNTH,_l("Load Avisynth script")),avs_mask,_l("avs"),scriptflnm,_l("."),0))
  {
   FILE *f=fopen(scriptflnm,_l("rb"));
   if (f)
    {
     char script[2048];
     size_t len=fread(script,1,2048,f);
     fclose(f);
     script[len]='\0';
     setDlgItemText(m_hwnd,IDC_ED_AVISYNTH,text<char_t>(script));
    } 
  }
}
void TavisynthPage::onSave(void)
{
 char_t scriptflnm[MAX_PATH]=_l("");
 if (dlgGetFile(true,m_hwnd,_(-IDD_AVISYNTH,_l("Save Avisynth script")),avs_mask,_l("avs"),scriptflnm,_l("."),0))
  {
   FILE *f=fopen(scriptflnm,_l("wt"));
   if (f)
    {
     int linescnt=(int)SendDlgItemMessage(m_hwnd,IDC_ED_AVISYNTH,EM_GETLINECOUNT,0,0);
     for (int i=0;i<linescnt;i++)
      {
       char_t line[2048];line[0]=LOBYTE(2048);line[1]=HIBYTE(2048);
       LRESULT len=SendDlgItemMessage(m_hwnd,IDC_ED_AVISYNTH,EM_GETLINE,i,LPARAM(line));
       line[len]='\0';
       if (i>0) fputs("\n",f);
       fputs(text<char>(line),f);
      } 
     fclose(f);
    }
  }   
}

void TavisynthPage::applySettings(void)
{
 char_t script[2048];
 GetDlgItemText(m_hwnd,IDC_ED_AVISYNTH,script,2048);
 cfgSet(IDFF_avisynthScript,script);
}

TavisynthPage::TavisynthPage(TffdshowPageDec *Iparent,const TfilterIDFF *idff):TconfPageDecVideo(Iparent,idff)
{
 resInter=IDC_CHB_AVISYNTH;
 static const TbindCheckbox<TavisynthPage> chb[]=
  {
   IDC_CHB_AVISYNTH_YV12,IDFF_avisynthInYV12,NULL,
   IDC_CHB_AVISYNTH_YUY2,IDFF_avisynthInYUY2,NULL,
   IDC_CHB_AVISYNTH_RGB32,IDFF_avisynthInRGB32,NULL,
   IDC_CHB_AVISYNTH_RGB24,IDFF_avisynthInRGB24,NULL,
   IDC_CHB_AVISYNTH_FFDSHOW,IDFF_avisynthFfdshowSource,NULL,
   0,NULL,NULL
  };
 bindCheckboxes(chb);
 static const TbindButton<TavisynthPage> bt[]=
  {
   IDC_BT_AVS_LOAD,&TavisynthPage::onLoad,
   IDC_BT_AVS_SAVE,&TavisynthPage::onSave,
   0,NULL
  };
 bindButtons(bt);
}
