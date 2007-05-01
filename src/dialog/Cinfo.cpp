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
#include "Cinfo.h"
#include "TvideoCodec.h"
#include "TaudioCodec.h"
#include "ffdshow_mediaguids.h"
#include "Tconfig.h"
#include "TffdshowPageDec.h"
#include "Tinfo.h"
#include "Ttranslate.h"
#include "TcompatibilityList.h"

//=================================== TinfoPageDec =======================================
void TinfoPageDec::init(void)
{
 setCheck(IDC_CHB_MMX     ,Tconfig::cpu_flags&FF_CPU_MMX     );
 setCheck(IDC_CHB_MMXEXT  ,Tconfig::cpu_flags&FF_CPU_MMXEXT  );
 setCheck(IDC_CHB_SSE     ,Tconfig::cpu_flags&FF_CPU_SSE     );
 setCheck(IDC_CHB_SSE2    ,Tconfig::cpu_flags&FF_CPU_SSE2    );
 setCheck(IDC_CHB_SSE3    ,Tconfig::cpu_flags&FF_CPU_SSE3    );
 setCheck(IDC_CHB_SSSE3   ,Tconfig::cpu_flags&FF_CPU_SSSE3   );
 setCheck(IDC_CHB_3DNOW   ,Tconfig::cpu_flags&FF_CPU_3DNOW   );
 setCheck(IDC_CHB_3DNOWEXT,Tconfig::cpu_flags&FF_CPU_3DNOWEXT);
 addHint(IDC_CHB_ADDTOROT,_l("Use with care - can cause ffdshow to not unload after closing the movie."));

 static const int insts[]={IDC_LBL_MULTIPLE_INSTANCES,IDC_CBX_MULTIPLE_INSTANCES,0};
 enable((filterMode&IDFF_FILTERMODE_VFW)==0,insts);

 merits.clear();
 static const int idmerits[]={IDC_LBL_MERIT,IDC_TBR_MERIT,0};
 isMerit=(filterMode&(IDFF_FILTERMODE_VFW|IDFF_FILTERMODE_ENC|IDFF_FILTERMODE_VIDEORAW|IDFF_FILTERMODE_AUDIORAW))==0;
 if (isMerit)
  {
   //merits.push_back(std::make_pair(MERIT_SW_COMPRESSOR /*0x100000*/,_("SW compressor")));
   //merits.push_back(std::make_pair(MERIT_HW_COMPRESSOR /*0x100050*/,_("HW compressor")));
   merits.push_back(std::make_pair(MERIT_DO_NOT_USE    /*0x200000*/,(const char_t*)_l("do not use")     ));
   merits.push_back(std::make_pair(MERIT_UNLIKELY      /*0x400000*/,(const char_t*)_l("unlikely")       ));
   merits.push_back(std::make_pair(MERIT_NORMAL        /*0x600000*/,(const char_t*)_l("normal")         ));
   merits.push_back(std::make_pair(MERIT_PREFERRED     /*0x800000*/,(const char_t*)_l("preferred")      ));
   merits.push_back(std::make_pair(cfgGet(IDFF_defaultMerit)       ,(const char_t*)_l("ffdshow default")));
   merits.push_back(std::make_pair(0xffffff00                      ,(const char_t*)_l("very high")      ));
   std::sort(merits.begin(),merits.end(),sortMerits);
   tbrSetRange(IDC_TBR_MERIT,0,(int)merits.size()-1,1);
   enable(1,idmerits);
  }
 else
  enable(0,idmerits);
 
 edLimitText(IDC_ED_BLACKLIST,MAX_COMPATIBILITYLIST_LENGTH);
 edLimitText(IDC_ED_COMPATIBILITYLIST,MAX_COMPATIBILITYLIST_LENGTH);
 addHint(IDC_ED_BLACKLIST,_l("List of applications for which ffdshow should not load, separated by semicolons"));

 hlv=GetDlgItem(m_hwnd,IDC_LV_INFO); 
 CRect r=getChildRect(IDC_LV_INFO);
 int ncol=0;
 ListView_AddCol(hlv,ncol,r.Width(),_l("Property"),false);
 ListView_SetExtendedListViewStyleEx(hlv,LVS_EX_FULLROWSELECT|LVS_EX_INFOTIP,LVS_EX_FULLROWSELECT|LVS_EX_INFOTIP);
 infoitems.clear(); 
 const int *infos=getInfos();
 for (int i=0;;i++)
  {
   Titem it;
   if (!info->getInfo(i,&it.id,&it.name))
    break;
   for (int j=0;infos[j];j++) 
    if (infos[j]==it.id)
     {
      it.index=j;
      infoitems.push_back(it);
     } 
  }
 std::sort(infoitems.begin(),infoitems.end(),TsortItem(infos)); 
 ListView_SetItemCount(hlv,infoitems.size()); 
 SendMessage(hlv,LVM_SETBKCOLOR,0,GetSysColor(COLOR_BTNFACE));
 SendMessage(hlv,LVM_SETTEXTBKCOLOR,0,GetSysColor(COLOR_BTNFACE));
 blacklist2dlg();
}

void TinfoPageDec::cfg2dlg(void)
{
 setCheck(IDC_CHB_ADDTOROT,cfgGet(IDFF_addToROT));
 int allow=cfgGet(IDFF_allowedCpuFlags);
 setCheck(IDC_CHB_ALLOW_MMX     ,allow&FF_CPU_MMX     );
 setCheck(IDC_CHB_ALLOW_MMXEXT  ,allow&FF_CPU_MMXEXT  );
 setCheck(IDC_CHB_ALLOW_SSE     ,allow&FF_CPU_SSE     );
 setCheck(IDC_CHB_ALLOW_SSE2    ,allow&FF_CPU_SSE2    );
 setCheck(IDC_CHB_ALLOW_SSE3    ,allow&FF_CPU_SSE3    );
 setCheck(IDC_CHB_ALLOW_SSSE3   ,allow&FF_CPU_SSSE3   );
 setCheck(IDC_CHB_ALLOW_3DNOW   ,allow&FF_CPU_3DNOW   );
 setCheck(IDC_CHB_ALLOW_3DNOWEXT,allow&FF_CPU_3DNOWEXT);
 cbxSetDataCurSel(IDC_CBX_MULTIPLE_INSTANCES,cfgGet(IDFF_multipleInstances));
 if (isMerit) merit2dlg();
 blacklist2dlg();
}

bool TinfoPageDec::sortMerits(const Tmerit &m1,const Tmerit &m2)
{
 return m1.first<m2.first;
}

void TinfoPageDec::merit2dlg(void)
{
 DWORD merit=0;deci->getMerit(&merit);
 meritset=true;
 for (int ii=0;ii<2;ii++,merit=cfgGet(IDFF_defaultMerit))
  {
   int i=0;
   for (Tmerits::const_iterator m=merits.begin();m!=merits.end();m++,i++)
    if (m->first==merit)
     {
      tbrSet(IDC_TBR_MERIT,i);
      setText(IDC_LBL_MERIT,_l("%s %s"),_(IDC_LBL_MERIT),_(IDC_LBL_MERIT,m->second));
      return;
     }
  }
}

void TinfoPageDec::blacklist2dlg(void)
{
 int is=cfgGet(IDFF_isBlacklist);
 setCheck(IDC_CHB_BLACKLIST,is);
 const char_t *blacklist0=cfgGetStr(IDFF_blacklist);
 ffstring blacklistResult;
 convertDelimit(blacklist0,_l(";"),blacklistResult,_l(";"));
 SetDlgItemText(m_hwnd,IDC_ED_BLACKLIST,blacklistResult.c_str());
 enable(is,IDC_ED_BLACKLIST);
 enable(is,IDC_BT_BLACKLIST);

 is=cfgGet(IDFF_isCompatibilityList);
 setCheck(IDC_CHB_COMPATIBILITYLIST,is);
 const char_t *complist0=cfgGetStr(IDFF_compatibilityList);
 ffstring complistResult;
 convertDelimit(complist0,_l("\r\n"),complistResult,_l(";"));
 SetDlgItemText(m_hwnd,IDC_ED_COMPATIBILITYLIST,complistResult.c_str());
 enable(is,IDC_ED_COMPATIBILITYLIST);
 enable(is,IDC_BT_COMPATIBILITYLIST);
}

void TinfoPageDec::applySettings(void)
{
 if (isMerit && meritset)
  deci->setMerit(merits[tbrGet(IDC_TBR_MERIT)].first);
}

bool TinfoPageDec::strCaseCmp(const ffstring &s1,const ffstring &s2)
{
 return DwStrcasecmp(s1,s2)==-1;
}

INT_PTR TinfoPageDec::msgProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
 switch (uMsg)
  {
   case WM_HSCROLL:
    switch (getId(HWND(lParam)))
     {
      case IDC_TBR_MERIT:
       setText(IDC_LBL_MERIT,_l("%s %s"),_(IDC_LBL_MERIT),_(IDC_LBL_MERIT,merits[tbrGet(IDC_TBR_MERIT)].second));
       parent->setChange();
       return TRUE;
     }
    break;
   case WM_COMMAND:
    switch (LOWORD(wParam))  
     {
      case IDC_CHB_ALLOW_MMX:
      case IDC_CHB_ALLOW_MMXEXT:
      case IDC_CHB_ALLOW_SSE:
      case IDC_CHB_ALLOW_SSE2:
      case IDC_CHB_ALLOW_SSE3:
      case IDC_CHB_ALLOW_SSSE3:
      case IDC_CHB_ALLOW_3DNOW:
      case IDC_CHB_ALLOW_3DNOWEXT:
       {
        int allow=0;
        if (getCheck(IDC_CHB_ALLOW_MMX     )) allow|=FF_CPU_MMX;
        if (getCheck(IDC_CHB_ALLOW_MMXEXT  )) allow|=FF_CPU_MMXEXT;
        if (getCheck(IDC_CHB_ALLOW_SSE     )) allow|=FF_CPU_SSE;
        if (getCheck(IDC_CHB_ALLOW_SSE2    )) allow|=FF_CPU_SSE2;
        if (getCheck(IDC_CHB_ALLOW_SSE3    )) allow|=FF_CPU_SSE3;
        if (getCheck(IDC_CHB_ALLOW_SSSE3   )) allow|=FF_CPU_SSSE3;
        if (getCheck(IDC_CHB_ALLOW_3DNOW   )) allow|=FF_CPU_3DNOW;
        if (getCheck(IDC_CHB_ALLOW_3DNOWEXT)) allow|=FF_CPU_3DNOWEXT;
        cfgSet(IDFF_allowedCpuFlags,allow);
        return TRUE;
       }
      case IDC_ED_BLACKLIST:
       if (HIWORD(wParam)==EN_CHANGE && !isSetWindowText)
        comp_dlg2cfg(IDC_ED_BLACKLIST,IDFF_blacklist,_l(";"));
       return TRUE;
      case IDC_ED_COMPATIBILITYLIST:
       if (HIWORD(wParam)==EN_CHANGE && !isSetWindowText)
        comp_dlg2cfg(IDC_ED_COMPATIBILITYLIST,IDFF_compatibilityList,_l("\r\n"));
       return TRUE;
     }    
    break;
   case WM_NOTIFY:
    {
     NMHDR *nmhdr=LPNMHDR(lParam);
     if (nmhdr->hwndFrom==hlv && nmhdr->idFrom==IDC_LV_INFO)
      switch (nmhdr->code)
       {
        case LVN_GETDISPINFO:
         {
          NMLVDISPINFO *nmdi=(NMLVDISPINFO*)lParam;
          int i=nmdi->item.iItem;
          if (i==-1) break;
          if (nmdi->item.mask&LVIF_TEXT)
           switch (nmdi->item.iSubItem)
            {
             case 0:
              {
               const char_t *descr;
               deciD->getKeyParamDescr(i,&descr);
               tsprintf(nmdi->item.pszText,_l("%s: %s"),infoitems[i].translatedName,infoitems[i].val?infoitems[i].val:_l(""));
               break;
              }
            }
          return TRUE;
         }
       }  
     break;  
    }  
  }
 return TconfPageDec::msgProc(uMsg,wParam,lParam);
}
void TinfoPageDec::comp_dlg2cfg(int IDC_ED_target,int IDFF_target,const char_t *delimitResult)
{
 char_t complist0[MAX_COMPATIBILITYLIST_LENGTH];
 GetDlgItemText(m_hwnd,IDC_ED_target,complist0,MAX_COMPATIBILITYLIST_LENGTH);
 ffstring complistResult;
 convertDelimit(complist0,_l(";"),complistResult,delimitResult);
 cfgSet(IDFF_target,complistResult.c_str());
}

void TinfoPageDec::convertDelimit(const char_t* instr,const char_t *inDelimit,ffstring &outstr,const char_t *outDelimit,const char_t *newFileName)
{
 strings complist;
 strtok(instr,inDelimit,complist);

 std::list<ffstring> complistList;
 complistList.insert(complistList.begin(),complist.begin(),complist.end());
 if (newFileName)
  complistList.push_back(newFileName);
 complistList.sort(strCaseCmp);

 for (std::list<ffstring>::iterator i=complistList.begin();i!=complistList.end();i++)
  outstr+=*i+outDelimit;
}

void TinfoPageDec::onFrame(void)
{
 if (!IsWindowVisible(m_hwnd)) return;
 for (Titems::iterator i=infoitems.begin();i!=infoitems.end();i++)
  {
   int wasChange;
   i->val=info->getVal(i->id,&wasChange,NULL);
   if (wasChange)
    ListView_Update(hlv,i-infoitems.begin());
  } 
}

void TinfoPageDec::translate(void)
{
 TconfPageDec::translate();

 int ii=cbxGetCurSel(IDC_CBX_MULTIPLE_INSTANCES);
 cbxClear(IDC_CBX_MULTIPLE_INSTANCES);
 for (int i=0;multipleInstances[i].name;i++)
  cbxAdd(IDC_CBX_MULTIPLE_INSTANCES,_(IDC_CBX_MULTIPLE_INSTANCES,multipleInstances[i].name),multipleInstances[i].id);
 cbxSetCurSel(IDC_CBX_MULTIPLE_INSTANCES,ii);
 
 for (Titems::iterator i=infoitems.begin();i!=infoitems.end();i++)
  i->translatedName=_(IDC_LV_INFO,i->name);
}

TinfoPageDec::TinfoPageDec(TffdshowPageDec *Iparent,TinfoBase *Iinfo):TconfPageDec(Iparent,NULL,0),info(Iinfo)
{
 dialogId=IDD_INFO;
 meritset=false;
 static const TbindCheckbox<TinfoPageDec> chb[]=
  {
   IDC_CHB_ADDTOROT,IDFF_addToROT,NULL,
   IDC_CHB_BLACKLIST,IDFF_isBlacklist,&TinfoPageDec::blacklist2dlg,
   IDC_CHB_COMPATIBILITYLIST,IDFF_isCompatibilityList,&TinfoPageDec::blacklist2dlg,
   0,NULL,NULL
  };
 bindCheckboxes(chb);
 static const TbindCombobox<TinfoPageDec> cbx[]=
  {
   IDC_CBX_MULTIPLE_INSTANCES,IDFF_multipleInstances,BINDCBX_DATA,NULL,
   0
  };
 bindComboboxes(cbx);
 static const TbindButton<TinfoPageDec> bt[]=
  {
   IDC_BT_COMPATIBILITYLIST,&TinfoPageDec::onEditCompatibilitylist,
   IDC_BT_BLACKLIST,&TinfoPageDec::onEditBlacklist,
   0,NULL
  };
 bindButtons(bt); 
}
TinfoPageDec::~TinfoPageDec()
{
 delete info;
}
void TinfoPageDec::onEditCompatibilitylist(void)
{
 onEditCompI(IDFF_compatibilityList);
}
void TinfoPageDec::onEditBlacklist(void)
{
 onEditCompI(IDFF_blacklist);
}
void TinfoPageDec::onEditCompI(int IDFF_target)
{
 TcompatibilityList *dlg=new TcompatibilityList(deci,m_hwnd,10,10,IDFF_target);
 if (dlg->show())
  {
   blacklist2dlg();
   parent->setChange();
  }
 delete dlg;
}
//================================= TinfoPageDecVideo ====================================
TinfoPageDecVideo::TinfoPageDecVideo(TffdshowPageDec *Iparent):TinfoPageDec(Iparent,new TinfoDecVideo(Iparent->deci))
{
}

//================================= TinfoPageDecAudio ====================================
TinfoPageDecAudio::TinfoPageDecAudio(TffdshowPageDec *Iparent):TinfoPageDec(Iparent,new TinfoDecAudio(Iparent->deci))
{
}
