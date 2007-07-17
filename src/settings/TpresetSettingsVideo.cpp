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
#include "TpresetSettingsVideo.h"
#include "TavisynthSettings.h"
#include "TblurSettings.h"
#include "TcropSettings.h"
#include "TdctSettings.h"
#include "TdeinterlaceSettings.h"
#include "TdscalerFilterSettings.h"
#include "TexpandSettings.h"
#include "TlevelsSettings.h"
#include "TnoiseSettings.h"
#include "ToffsetSettings.h"
#include "TperspectiveSettings.h"
#include "TpictPropSettings.h"
#include "TpostprocSettings.h"
#include "TresizeAspectSettings.h"
#include "TsharpenSettings.h"
#include "TsubtitlesSettings.h"
#include "TwarpsharpSettings.h"
#include "TlogoawaySettings.h"
#include "TbitmapSettings.h"
#include "TvisSettings.h"
#include "TgradFunSettings.h"
#include "TgrabSettings.h"
#include "ThwOverlaySettings.h"
#include "ToutputVideoSettings.h"
#include "ffdshow_constants.h"
#include "Tlibavcodec.h"
#include "IffdshowBase.h"
#include "IffdshowDec.h"
#include "IffdshowDecVideo.h"
#include "Teval.h"
#include "Tconfig.h"

//========================= TvideoAutoPresetProps =========================
const char_t TvideoAutoPresetProps::aspectSAR=_l('S'),TvideoAutoPresetProps::aspectDAR=_l('D');

TvideoAutoPresetProps::TvideoAutoPresetProps(IffdshowBase *Ideci):
 TautoPresetProps(Ideci),
 wasResolution(false),
 deciV(Ideci),
 SAR(-1),DAR(-1),
 fps(0)
{
 fourcc[0]='\0';
}
void TvideoAutoPresetProps::getSourceResolution(unsigned int *dxPtr,unsigned int *dyPtr)
{
 if (!wasResolution)
  {
   wasResolution=true;
   deciV->getAVIdimensions(&dx,&dy);
  }
 *dxPtr=dx;*dyPtr=dy;
}
const char_t* TvideoAutoPresetProps::getFOURCC(void)
{
 if (fourcc[0]=='\0')
  {
   FOURCC fcc=deciV->getMovieFOURCC();
#ifdef UNICODE
   char fourcc0[5];
   memcpy(fourcc0,&fcc,4);
   fourcc0[4]='\0';
   MultiByteToWideChar(CP_ACP,0,fourcc0,4,fourcc,4);
#else
   memcpy(fourcc,&fcc,4);
#endif
   fourcc[4]='\0';
  }
 return fourcc;
}
const char_t* TvideoAutoPresetProps::getFOURCCitem(IffdshowDec *deciD,unsigned int index)
{
 const char_t **list=deciD->getSupportedFOURCCs();;
 while (index-- && *(list++));
 return *list;
}

const char_t* TvideoAutoPresetProps::getSAR(void)
{
 if (SAR==-1)
  {
   unsigned int a1,a2;
   if (deciV->getInputSAR(&a1,&a2)==S_OK)
    SAR=double(a1)/a2;
  }
 return &aspectSAR;
}
const char_t* TvideoAutoPresetProps::getDAR(void)
{
 if (DAR==-1)
  {
   unsigned int a1,a2;
   if (deciV->getInputDAR(&a1,&a2)==S_OK)
    DAR=double(a1)/a2;
  }
 return &aspectDAR;
}
bool TvideoAutoPresetProps::aspectMatch(const char_t *expr,const char_t *aspectStr)
{
 const double &aspect=aspectStr==&aspectSAR?SAR:DAR;
 Teval::Tvariable vars[]={{_l("aspect"),&aspect},NULL};
 const char_t *err;
 double res=Teval(expr,vars)(&err);
 return !err && !!res;
}

const char_t* TvideoAutoPresetProps::getFps(void)
{
 if (fps==0)
  {
   unsigned int fps1000;
   if (deciV->getAVIfps(&fps1000)==S_OK)
    fps=fps1000/1000.0;
  }
 return _l("fps");
}
bool TvideoAutoPresetProps::fpsMatch(const char_t *expr,const char_t *)
{
 Teval::Tvariable vars[]={{_l("fps"),&fps},NULL};
 const char_t *err;
 double res=Teval(expr,vars)(&err);
 return !err && !!res;
}

//============================== TpresetVideo =============================
TpresetVideo::TpresetVideo(const char_t *Ireg_child,const char_t *IpresetName):
 Tpreset(Ireg_child,IpresetName,-1)
{
 static const TintOptionT<TpresetVideo> iopts[]=
  {
   IDFF_presetAutoloadSize    ,&TpresetVideo::autoloadSize     ,0,0,_l(""),0,
     _l("autoloadSize"),0,
   IDFF_presetAutoloadSizeXmin,&TpresetVideo::autoloadSizeXmin ,16,16384,_l(""),0,
     _l("autoloadSizeXmin"),16,
   IDFF_presetAutoloadSizeXmax,&TpresetVideo::autoloadSizeXmax ,16,16384,_l(""),0,
     _l("autoloadSizeXmax"),2048,
   IDFF_presetAutoloadSizeCond,&TpresetVideo::autoloadSizeCond ,0,1,_l(""),0,
     _l("autoloadSizeCond"),1,
   IDFF_presetAutoloadSizeYmin,&TpresetVideo::autoloadSizeYmin ,16,16384,_l(""),0,
     _l("autoloadSizeYmin"),16,
   IDFF_presetAutoloadSizeYmax,&TpresetVideo::autoloadSizeYmax ,16,16384,_l(""),0,
     _l("autoloadSizeYmax"),2048,

   IDFF_idct               ,&TpresetVideo::idct               ,0,6,_l(""),1,
     _l("idct"),0,
   IDFF_videoDelay         ,&TpresetVideo::videoDelay         ,1,1,_l(""),1,
     _l("videoDelay"),0,
   IDFF_isVideoDelayEnd    ,&TpresetVideo::isVideoDelayEnd    ,0,0,_l(""),1,
     _l("isVideoDelayEnd"),0,
   IDFF_videoDelayEnd      ,&TpresetVideo::videoDelayEnd      ,1,1,_l(""),1,
     _l("videoDelayEnd"),0,
   IDFF_workaroundBugs     ,&TpresetVideo::workaroundBugs     ,1,1,_l(""),1,
     _l("workaroundBugs2"),FF_BUG_AUTODETECT,
   IDFF_errorConcealment   ,&TpresetVideo::errorConcealment   ,0,3,_l(""),1,
     _l("errorConcealment"),FF_EC_GUESS_MVS|FF_EC_DEBLOCK,
   IDFF_errorResilience    ,&TpresetVideo::errorResilience    ,0,4,_l(""),1,
     _l("errorResilience"),FF_ER_CAREFUL,
   IDFF_numLAVCdecThreads ,&TpresetVideo::lavcDecThreads      ,1,8,_l(""),1,
     _l("threadsnum"),1,
   IDFF_grayscale          ,&TpresetVideo::grayscale          ,0,0,_l(""),1,
     _l("grayscale"),0,
   IDFF_multiThreadDec     ,&TpresetVideo::multiThread        ,0,0,_l(""),1,
     _l("multiThread"),0,
   IDFF_dontQueueInWMP     ,&TpresetVideo::dontQueueInWMP     ,0,0,_l(""),1,
     _l("dontQueueInWMP"),1,
   IDFF_useQueueOnlyIn     ,&TpresetVideo::useQueueOnlyIn     ,0,0,_l(""),1,
     _l("useQueueOnlyIn"),1,
   IDFF_QueueCount         ,&TpresetVideo::queueCount      ,1,200,_l(""),1,
     _l("queueCount"),10,

   IDFF_dropOnDelay        ,&TpresetVideo::dropOnDelay        ,0,0,_l(""),1,
     _l("dropOnDelay"),1,
   IDFF_dropOnDelayTime    ,&TpresetVideo::dropDelayTime    ,0,20000,_l(""),1,
     _l("dropDelayTime"),1500,
   IDFF_h264skipOnDelay    ,&TpresetVideo::h264skipOnDelay    ,0,0,_l(""),1,
     _l("h264skipOnDelay"),1,
   IDFF_h264skipOnDelayTime,&TpresetVideo::h264skipDelayTime  ,0,20000,_l(""),1,
     _l("h264skipDelayTime"),350,

   IDFF_isDyInterlaced     ,&TpresetVideo::isDyInterlaced     ,0,0,_l(""),0,
     _l("isDyInterlaced"),0,
   IDFF_dyInterlaced       ,&TpresetVideo::dyInterlaced       ,0,4096,_l(""),0,
     _l("dyInterlaced"),288,
   0
  };
 addOptions(iopts);

 static const TstrOption sopts[]=
  {
   IDFF_useQueueOnlyInList  ,(TstrVal)&TpresetVideo::useQueueOnlyInList ,128,_l(""),0,
     _l("useQueueOnlyInList"),_l("mplayerc.exe;"),
   0
  };
 addOptions(sopts);

 static const TcreateParamList1 listIDCT(Tlibavcodec::idctNames);setParamList(IDFF_idct,&listIDCT);
 static const TcreateParamList1 listErrorConcealment(Tlibavcodec::errorConcealments);setParamList(IDFF_errorConcealment,&listErrorConcealment);
 static const TcreateParamList1 listErrorResilience(Tlibavcodec::errorResiliences);setParamList(IDFF_errorResilience,&listErrorResilience);

 static const char_t *aspectHelp=_l("Enter logical expression with 'aspect' variable and comparison and arithmetic operators,\nfor example \"16/9<aspect AND aspect<2.35\" or \"aspect=1\".");
 static const char_t *fpsHelp=_l("Enter logical expression with 'fps' variable and comparison and arithmetic operators,\nfor example \"fps>30\".");
 static const TautoPresetItemDef autoPresetItems[]=
  {
   {
    _l("on FOURCC match"),NULL,
    _l("autoloadFOURCC"),0,
    _l("autoloadFOURCCs"),_l(""),
    &TautoPresetProps::stricoll,
    (TautoPresetItemDef::TgetValFc)&TvideoAutoPresetProps::getFOURCC,
    &TvideoAutoPresetProps::getFOURCCitem,
   },
   {
    _l("on pixel aspect ratio match"),NULL,
    _l("autoloadSAR"),0,
    _l("autoloadSARs"),_l(""),
    (TautoPresetItemDef::TcompareFc)&TvideoAutoPresetProps::aspectMatch,
    (TautoPresetItemDef::TgetValFc)&TvideoAutoPresetProps::getSAR,
    NULL,aspectHelp
   },
   {
    _l("on picture aspect ratio match"),NULL,
    _l("autoloadDAR"),0,
    _l("autoloadDARs"),_l(""),
    (TautoPresetItemDef::TcompareFc)&TvideoAutoPresetProps::aspectMatch,
    (TautoPresetItemDef::TgetValFc)&TvideoAutoPresetProps::getDAR,
    NULL,aspectHelp
   },
   {
    _l("on frame rate match"),NULL,
    _l("autoloadFrameRate"),0,
    _l("autoloadFrameRatess"),_l(""),
    (TautoPresetItemDef::TcompareFc)&TvideoAutoPresetProps::fpsMatch,
    (TautoPresetItemDef::TgetValFc)&TvideoAutoPresetProps::getFps,
    NULL,fpsHelp
   },
   0
  };
 addAutoPresetItems(autoPresetItems);

 new TcropSettings(options,filters);
 new TdeinterlaceSettings(options,filters);
 new TlogoawaySettings(options,filters);
 postproc=new TpostprocSettings(options,filters);
 new TpictPropSettings(options,filters);
 new TgradFunSettings(options,filters);
 levels=new TlevelsSettings(options,filters);
 new ToffsetSettings(options,filters);
 new TblurSettings(options,filters);
 new TsharpenSettings(options,filters);
 new TwarpsharpSettings(options,filters);
 new TDScalerFilterSettings(options,filters);
 new TnoiseSettings(options,filters);
 resize=new TresizeAspectSettings(options,filters);
 new TperspectiveSettings(options,filters);
 new TavisynthSettings(options,filters);
 vis=new TvisSettings(options,filters);
 new TdctSettings(options,filters);
 new TbitmapSettings(options,filters);
 subtitles=new TsubtitlesSettings(options,filters);
 grab=new TgrabSettings(options,filters);
 output=new ToutputVideoSettings(options,filters);
}

void TpresetVideo::reg_op(TregOp &t)
{
 t._REG_OP_N(0,_l("needOutcspsFix"),needOutcspsFix,1);
 t._REG_OP_N(0,_l("needGlobalFix"),needGlobalFix,1);
 Tpreset::reg_op(t);
}

int TpresetVideo::getDefault(int id)
{
 if (id==IDFF_multiThreadDec)
  return Tconfig::getCPUcount()>1?1:0;
 else
  return Tpreset::getDefault(id);
}


void TpresetVideo::loadReg(void)
{
 Tpreset::loadReg();
 //if (idct==6) idct=0;
 if (needOutcspsFix)
  {
   needOutcspsFix=0;
   char_t rkey[MAX_PATH];tsprintf(rkey,FFDSHOW_REG_PARENT _l("\\%s"),reg_child);
   TregOpRegRead t(HKEY_LOCAL_MACHINE,rkey);
   output->reg_op_outcsps(t);
  }
 if (needGlobalFix)
  {
   needGlobalFix=0;
   char_t rkey[MAX_PATH];tsprintf(rkey,FFDSHOW_REG_PARENT _l("\\%s"),reg_child);
   TregOpRegRead t(HKEY_CURRENT_USER,rkey);
   vis->reg_op2(t);
   grab->reg_op2(t);
  }
}

bool TpresetVideo::autoloadSizeMatch(int AVIdx,int AVIdy)
{
 bool Xok=(autoloadSizeXmin<=AVIdx && AVIdx<=autoloadSizeXmax);
 bool Yok=(autoloadSizeYmin<=AVIdy && AVIdy<=autoloadSizeYmax);
 return (autoloadSizeCond==0)?(Xok && Yok):(Xok || Yok);
}

//=========================== TpresetVideoPlayer ==========================
TpresetVideoPlayer::TpresetVideoPlayer(const char_t *Ireg_child, const char_t *IpresetName):TpresetVideo(Ireg_child,IpresetName)
{
 new ThwOverlaySettings(options,filters);
}
