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
#include "CresizeAspect.h"
#include "TresizeAspectSettings.h"
#include "TffdshowPageDec.h"
#include "TflatButtons.h"
#include "Ttranslate.h"

void TresizeAspectPage::init(void)
{
 tbrSetRange(IDC_TBR_ASPECT_USER,int(0.1*256),5*256,int(.6*256));
 tbrSetRange(IDC_TBR_HWOVERLAY_ASPECT,0,5*256,int(.6*256));

 setFont(IDC_BT_RESIZE_SIZE_MENU  ,parent->arrowsFont);
 setFont(IDC_BT_RESIZE_MOD_16     ,parent->arrowsFont);
 setFont(IDC_BT_RESIZE_ASPECT_MENU,parent->arrowsFont);
 setFont(IDC_BT_RESIZE_PIX_MENU   ,parent->arrowsFont);
}

void TresizeAspectPage::cfg2dlg(void)
{
 resize2dlg();
 resizeIf2dlg();
 aspect2dlg();
}

void TresizeAspectPage::resize2dlg(void)
{
 setCheck(IDC_CHB_RESIZE,cfgGet(IDFF_isResize));
 int rm=cfgGet(IDFF_resizeMode);
 setCheck(IDC_RBT_RESIZE_MODE_SIZE  ,rm==0);
 setCheck(IDC_RBT_RESIZE_MODE_ASPECT,rm==1);
 setCheck(IDC_RBT_RESIZE_MOD_16     ,rm==2);
 setCheck(IDC_RBT_RESIZE_MODE_MULT  ,rm==3);
 SetDlgItemInt(m_hwnd,IDC_ED_RESIZEDX,cfgGet(IDFF_resizeDx),FALSE);
 int y=cfgGet(IDFF_resizeDy);
 int isy0=cfgGet(IDFF_is_resizeDy_0);
 if(isy0) y=0;
 SetDlgItemInt(m_hwnd,IDC_ED_RESIZEDY, y ,FALSE);
 SetDlgItemInt(m_hwnd,IDC_ED_RESIZE_MOD_16,cfgGet(IDFF_resizeMultOf),FALSE);
 SetDlgItemInt(m_hwnd,IDC_ED_RESIZEA1,cfgGet(IDFF_resizeA1),FALSE);
 SetDlgItemInt(m_hwnd,IDC_ED_RESIZEA2,cfgGet(IDFF_resizeA2),FALSE);
 setText(IDC_ED_RESIZE_MULT,_l("%.3f"),float(cfgGet(IDFF_resizeMult1000)/1000.0));
 static const int idSai[]={IDC_RBT_RESIZE_MODE_SIZE,IDC_LBL_NEW_SIZE,IDC_ED_RESIZEDX,IDC_ED_RESIZEDY,IDC_BT_RESIZE_SIZE_MENU,
                           IDC_RBT_RESIZE_MODE_ASPECT,IDC_LBL_NEW_ASPECT,IDC_ED_RESIZEA1,IDC_LBL_NEW_ASPECT_SEP,IDC_ED_RESIZEA2,IDC_BT_RESIZE_ASPECT_MENU,
                           IDC_RBT_RESIZE_MOD_16,IDC_ED_RESIZE_MOD_16,IDC_ED_RESIZE_MOD_16,IDC_BT_RESIZE_MOD_16,
                           IDC_RBT_RESIZE_MODE_MULT,IDC_ED_RESIZE_MULT,
                           0};
 enable(TresizeAspectSettings::methodsProps[cfgGet(IDFF_resizeMethodLuma)].library!=TresizeAspectSettings::LIB_SAI,idSai);
 if(tr)
  {
   addHint(IDC_ED_RESIZEDY,tr->translate(IDH_ED_RESIZEDY));
   addHint(IDC_RBT_RESIZE_MODE_SIZE,tr->translate(IDH_RBT_RESIZE_MODE_SIZE));
  }
}
void TresizeAspectPage::resizeIf2dlg(void)
{
 int ri=cfgGet(IDFF_resizeIf);
 setCheck(IDC_RBT_RESIZE_ALWAYS          ,ri==0);
 setCheck(IDC_RBT_RESIZE_IFSIZE          ,ri==1);
 setCheck(IDC_RBT_RESIZE_IFNUMBEROFPIXELS,ri==2);
 SetDlgItemInt(m_hwnd,IDC_ED_RESIZE_IFX  ,cfgGet(IDFF_resizeIfXval  ),FALSE);
 SetDlgItemInt(m_hwnd,IDC_ED_RESIZE_IFY  ,cfgGet(IDFF_resizeIfYval  ),FALSE);
 SetDlgItemInt(m_hwnd,IDC_ED_RESIZE_IFPIX,cfgGet(IDFF_resizeIfPixVal),FALSE);
 setDlgItemText(m_hwnd,IDC_BT_RESIZE_XCOND  ,TresizeAspectSettings::getXcondStr(cfgGet(IDFF_resizeIfXcond  )));
 setDlgItemText(m_hwnd,IDC_BT_RESIZE_YCOND  ,TresizeAspectSettings::getXcondStr(cfgGet(IDFF_resizeIfYcond  )));
 setDlgItemText(m_hwnd,IDC_BT_RESIZE_XYCOND ,_(IDC_BT_RESIZE_XYCOND ,TresizeAspectSettings::getXYcondStr (cfgGet(IDFF_resizeIfXYcond ))));
 setDlgItemText(m_hwnd,IDC_BT_RESIZE_PIXCOND,_(IDC_BT_RESIZE_PIXCOND,TresizeAspectSettings::getPixCondStr(cfgGet(IDFF_resizeIfPixCond))));
}
void TresizeAspectPage::aspect2dlg(void)
{
 int ra=cfgGet(IDFF_isAspect);
 setCheck(IDC_RBT_ASPECT_NO  ,ra==0);
 setCheck(IDC_RBT_ASPECT_KEEP,ra==1);
 setCheck(IDC_RBT_ASPECT_USER,ra==2);
 char_t pomS[256];
 unsigned int dx,dy;
 deciV->getAVIdimensions(&dx,&dy);
 if (dx!=0 && dy!=0)
  setText(IDC_RBT_ASPECT_KEEP,_l("%s %3.2f:1"),_(IDC_RBT_ASPECT_KEEP),float(dx)/dy);
 int aspectI=cfgGet(IDFF_aspectRatio);
 setText(IDC_RBT_ASPECT_USER,_l("%s %3.2f:1"),_(IDC_RBT_ASPECT_USER),float(aspectI/65536.0));
 tbrSet(IDC_TBR_ASPECT_USER,aspectI/256);
 aspectI=cfgGet(IDFF_hwOverlayAspect);
 tbrSet(IDC_TBR_HWOVERLAY_ASPECT,aspectI/256);
 tsprintf(pomS,_l("%s "),_(IDC_LBL_HWOVERLAY_ASPECT));
 if (aspectI==0) strcat(pomS,_(IDC_LBL_HWOVERLAY_ASPECT,_l("default")));else strcatf(pomS,_l("%3.2f:1"),float(aspectI/65536.0));
 setDlgItemText(m_hwnd,IDC_LBL_HWOVERLAY_ASPECT,pomS);
}            
void TresizeAspectPage::applyResizeXY(void)
{
 BOOL ok;
 int x=GetDlgItemInt(m_hwnd,IDC_ED_RESIZEDX,&ok,FALSE);
 if (!ok || x<64 || x>16384 || (x&1)) return;
 int y=GetDlgItemInt(m_hwnd,IDC_ED_RESIZEDY,&ok,FALSE);
 if(!ok)
  {
   DWORD lasterr=GetLastError();
   if(lasterr==0)
    {
     ok=true;
     y=0;
    }
  }
 if (!ok || (y<8 && y!=0) || y>16384 || (y&1)) return;
 cfgSet(IDFF_resizeDx,x);
 if(y)
  {
   cfgSet(IDFF_resizeDy,y);
   cfgSet(IDFF_is_resizeDy_0,0);
  }
 else
  {
   cfgSet(IDFF_is_resizeDy_0,1);
  }
 cfgSet(IDFF_resizeDy_real,y);
 parent->setChange(); 
}
bool TresizeAspectPage::sizeXok(HWND hed)
{
 char_t pomS[256];
 GetWindowText(hed,pomS,255);
 char_t *stop=NULL;
 int x=strtoul(pomS,&stop,10);
 if (*stop || x<64 || x>16384 || (x&1)) return false;
 // x must be larger than sizeFactor*srcW/MAX_FILTER_SIZE
 // assume sizeFactor= 8, srcW= 1980, MAX_FILTER_SIZE=256, then x>61.8
 // FIXME dynamic calc to allow 32, or to support 4K2K
 // See initFilter / swscale.c
 // if initFilter fails(retrun -1), swscaler crashes.
 return true;
}
bool TresizeAspectPage::sizeYok(HWND hed)
{
 DWORD lasterr;
 int y=0;
 char_t *stop=NULL;
 char_t pomS[256];
 int err= GetWindowText(hed,pomS,255);
 if(err==0)
  {
   lasterr= GetLastError();
   if(lasterr==0)
    return true;
  }
 else
  {
   y=strtoul(pomS,&stop,10);
  }
 if (*stop || (y<8 && y!=0) || y>16384 || (y&1)) return false;
 return true;
}
INT_PTR TresizeAspectPage::msgProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
 switch (uMsg)
  {
   case WM_HSCROLL:
    switch (GetWindowLong(HWND(lParam),GWL_ID))
     {
      case IDC_TBR_ASPECT_USER:
       {
        int a=tbrGet(IDC_TBR_ASPECT_USER);
        cfgSet(IDFF_aspectRatio,a*256);
        aspect2dlg();
        return TRUE;
       }
      case IDC_TBR_HWOVERLAY_ASPECT:
       {
        int a=tbrGet(IDC_TBR_HWOVERLAY_ASPECT);
        cfgSet(IDFF_hwOverlayAspect,a*256);
        aspect2dlg();
        return TRUE;
       }
     } 
    break;
   case WM_COMMAND:
    switch (LOWORD(wParam))  
     {
      case IDC_ED_RESIZEDX:
      case IDC_ED_RESIZEDY:
       if (HIWORD(wParam)==EN_CHANGE && !isSetWindowText) 
        {
         HWND hed=GetDlgItem(m_hwnd,LOWORD(wParam));
         if (hed!=GetFocus()) return FALSE;
         repaint(hed);
         applyResizeXY();
         return TRUE;  
        }
       break;
      case IDC_BT_RESIZE_XCOND:
       if (HIWORD(wParam)==BN_CLICKED)
        {
         cfgSet(IDFF_resizeIfXcond,-1*cfgGet(IDFF_resizeIfXcond));
         resizeIf2dlg();
         return TRUE;
        }
       break;  
      case IDC_BT_RESIZE_YCOND:
       if (HIWORD(wParam)==BN_CLICKED)
        {
         cfgSet(IDFF_resizeIfYcond,-1*cfgGet(IDFF_resizeIfYcond));
         resizeIf2dlg();
         return TRUE;
        }
       break;  
      case IDC_BT_RESIZE_XYCOND:
       if (HIWORD(wParam)==BN_CLICKED)
        {
         cfgSet(IDFF_resizeIfXYcond,1-cfgGet(IDFF_resizeIfXYcond));
         resizeIf2dlg();
         return TRUE;
        }
       break;  
      case IDC_BT_RESIZE_PIXCOND:
       if (HIWORD(wParam)==BN_CLICKED)
        {
         cfgSet(IDFF_resizeIfPixCond,-1*cfgGet(IDFF_resizeIfPixCond));
         resizeIf2dlg();
         return TRUE;
        }
       break;  
     }   
    break;
   case WM_CTLCOLOREDIT:
    {
     HWND hwnd=HWND(lParam);
     bool ok;
     switch (getId(hwnd))
      {
       case IDC_ED_RESIZEDX:
        ok=sizeXok(hwnd);break;
       case IDC_ED_RESIZEDY:
        ok=sizeYok(hwnd);break;
       default:goto colorEnd;
      }
     if (!ok)
      { 
       HDC dc=HDC(wParam);
       SetBkColor(dc,RGB(255,0,0));
       return INT_PTR(getRed()); 
      }
     else return FALSE;  
     colorEnd:;
     break;
    }
   case WM_DRAWITEM:
    switch (wParam)
     {
      case IDC_BT_RESIZE_PIXCOND:
      case IDC_BT_RESIZE_XCOND:
      case IDC_BT_RESIZE_YCOND:
      case IDC_BT_RESIZE_XYCOND:
       {
        DRAWITEMSTRUCT *dis=(LPDRAWITEMSTRUCT)lParam;
        ((TflatButton*)Twidget::getDlgItem(dis->hwndItem))->paint(dis);
        return TRUE;
       } 
     }
    break;   
  }
 return TconfPageDecVideo::msgProc(uMsg,wParam,lParam);
}
Twidget* TresizeAspectPage::createDlgItem(int id,HWND h)
{
 if (id==IDC_BT_RESIZE_PIXCOND || id==IDC_BT_RESIZE_XCOND || id==IDC_BT_RESIZE_YCOND || id==IDC_BT_RESIZE_XYCOND)
  return new TflatButton(h,this);
 else
  return TconfPageDecVideo::createDlgItem(id,h); 
}

void TresizeAspectPage::onResizeSizeMenu(void)
{
 static const char_t *pixels[]=
  {
   _l("320x240"),
   _l("640x480"),
   _l("720x480"),
   _l("720x576"),
   NULL
  };
 int cmd=selectFromMenu(pixels,IDC_BT_RESIZE_SIZE_MENU,false);
 int x,y;
 switch (cmd)
  {
   case 0:x=320;y=240;break; 
   case 1:x=640;y=480;break;
   case 2:x=720;y=480;break;
   case 3:x=720;y=576;break;
   default:return;
  }
 cfgSet(IDFF_resizeDx,x);cfgSet(IDFF_resizeDy,y);
 cfgSet(IDFF_resizeDy_real,y);
 resize2dlg(); 
}
void TresizeAspectPage::onResizeMulfOfMenu(void)
{
 static const char_t *mults[]=
  {
   _l("2"),_l("4"),_l("8"),_l("16"),_l("32"),_l("64"),_l("128"),NULL
  };
 int cmd=selectFromMenu(mults,IDC_BT_RESIZE_MOD_16,false);
 if (cmd>=0)
  {
   cfgSet(IDFF_resizeMultOf,atoi(mults[cmd]));
   resize2dlg();
  } 
}
void TresizeAspectPage::onResizeAspectMenu(void)
{
 static const char_t *aspects[]=
  {
   _l("4:3"),
   _l("5:4"),
   _l("16:9"),
   _l("2.35:1"),
   NULL
  };
 int cmd=selectFromMenu(aspects,IDC_BT_RESIZE_ASPECT_MENU,false);
 int a1,a2;
 switch (cmd)
  {
   case 0:a1=  4;a2=  3;break; 
   case 1:a1=  5;a2=  4;break; 
   case 2:a1= 16;a2=  9;break;
   case 3:a1=235;a2=100;break;
   default:return;
  }
 cfgSet(IDFF_resizeA1,a1);cfgSet(IDFF_resizeA2,a2);
 resize2dlg(); 
}
void TresizeAspectPage::onResizePixMenu(void)
{
 static const char_t *pixels[]=
  {
   _l("320x240"),
   _l("640x480"),
   NULL
  };
 int cmd=selectFromMenu(pixels,IDC_BT_RESIZE_PIX_MENU,false);
 int pix;
 switch (cmd)
  {
   case 0:pix=320*240;break; 
   case 1:pix=640*480;break;
   default:return;
  }
 cfgSet(IDFF_resizeIfPixVal,pix);
 resizeIf2dlg(); 
}

TresizeAspectPage::TresizeAspectPage(TffdshowPageDec *Iparent,const TfilterIDFF *idff):TconfPageDecVideo(Iparent,idff,1)
{
 resInter=IDC_CHB_RESIZE;
// helpURL="resize_aspect.html";
 static const TbindTrackbar<TresizeAspectPage> htbr[]=
  {
   IDC_TBR_ASPECT_USER,IDFF_aspectRatio,&TresizeAspectPage::aspect2dlg,
   IDC_TBR_HWOVERLAY_ASPECT,IDFF_hwOverlayAspect,&TresizeAspectPage::aspect2dlg,
   0,0,NULL
  };
 bindHtracks(htbr);
 static const TbindRadiobutton<TresizeAspectPage> rbt[]=
  {
   IDC_RBT_RESIZE_MODE_SIZE,IDFF_resizeMode,0,NULL,
   IDC_RBT_RESIZE_MODE_ASPECT,IDFF_resizeMode,1,NULL,
   IDC_RBT_RESIZE_MOD_16,IDFF_resizeMode,2,NULL,
   IDC_RBT_RESIZE_MODE_MULT,IDFF_resizeMode,3,NULL,
   IDC_RBT_ASPECT_NO,IDFF_isAspect,0,NULL,
   IDC_RBT_ASPECT_KEEP,IDFF_isAspect,1,NULL,
   IDC_RBT_ASPECT_USER,IDFF_isAspect,2,NULL,
   IDC_RBT_RESIZE_ALWAYS,IDFF_resizeIf,0,NULL,
   IDC_RBT_RESIZE_IFSIZE,IDFF_resizeIf,1,NULL,
   IDC_RBT_RESIZE_IFNUMBEROFPIXELS,IDFF_resizeIf,2,NULL,
   0,0,0,NULL
  };
 bindRadioButtons(rbt);
 static const TbindEditInt<TresizeAspectPage> edInt[]=
  {
   IDC_ED_RESIZEA1,0,10000,IDFF_resizeA1,NULL,
   IDC_ED_RESIZEA2,0,10000,IDFF_resizeA2,NULL,
   IDC_ED_RESIZE_IFX,0,16384,IDFF_resizeIfXval,NULL,
   IDC_ED_RESIZE_IFY,0,16384,IDFF_resizeIfYval,NULL,
   IDC_ED_RESIZE_IFPIX,0,16384*16384,IDFF_resizeIfPixVal,NULL,
   IDC_ED_RESIZE_MOD_16,1,128,IDFF_resizeMultOf,NULL,
   0
  };
 bindEditInts(edInt); 
 static const TbindEditReal<TresizeAspectPage> edReal[]=
  {
   IDC_ED_RESIZE_MULT,0.001,1000.0,IDFF_resizeMult1000,1000.0,NULL,
   0
  };
 bindEditReals(edReal); 
 static const TbindButton<TresizeAspectPage> bt[]=
  {
   IDC_BT_RESIZE_SIZE_MENU,&TresizeAspectPage::onResizeSizeMenu,
   IDC_BT_RESIZE_MOD_16,&TresizeAspectPage::onResizeMulfOfMenu,
   IDC_BT_RESIZE_ASPECT_MENU,&TresizeAspectPage::onResizeAspectMenu,
   IDC_BT_RESIZE_PIX_MENU,&TresizeAspectPage::onResizePixMenu,
   0,NULL
  };
 bindButtons(bt); 
}
