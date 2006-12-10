/*
 * Copyright (c) 2003-2006 Milan Cutka
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
#include "TpostprocSettings.h"
#include "postproc/postprocFilters.h"
#include "TimgFilterPostproc.h"
#include "CpostProc.h"
#include "TffdshowPageDec.h"

const int TpostprocSettings::deblockStrengthDef=256;

const TfilterIDFF TpostprocSettings::idffs=
{
 /*name*/      _l("Postprocessing"),
 /*id*/        IDFF_filterPostproc,
 /*is*/        IDFF_isPostproc,
 /*order*/     IDFF_orderPostproc,
 /*show*/      IDFF_showPostproc,
 /*full*/      IDFF_fullPostproc,
 /*half*/      IDFF_halfPostproc,
 /*dlgId*/     IDD_POSTPROC,
};

const char_t* TpostprocSettings::h264modes[]=
{
 _l("off"),
 _l("always"),
 _l("when decoding H.264 video"),
 _l("when decoding H.264 video and decoder deblocking is off"),
 NULL
};

TpostprocSettings::TpostprocSettings(TintStrColl *Icoll,TfilterIDFFs *filters):TfilterSettingsVideo(sizeof(*this),Icoll,filters,&idffs)
{
 //levelFixChrom=0;
 static const TintOptionT<TpostprocSettings> iopts[]=
  {
   IDFF_isPostproc            ,&TpostprocSettings::is                    ,0,0,_l(""),1, 
     _l("isPostproc"),0,
   IDFF_showPostproc          ,&TpostprocSettings::show                  ,0,0,_l(""),1,
     _l("showPostproc"),1,
   IDFF_orderPostproc         ,&TpostprocSettings::order                 ,1,1,_l(""),1,
     _l("orderPostproc"),0,
   IDFF_fullPostproc          ,&TpostprocSettings::full                  ,0,0,_l(""),1,
     _l("fullPostproc"),0,
   IDFF_halfPostproc          ,&TpostprocSettings::half                  ,0,0,_l(""),1,
     _l("halfPostproc"),0,
   IDFF_postprocMethod        ,&TpostprocSettings::method                ,0,5,_l(""),1,
     _l("postprocMethod"),0,
   IDFF_postprocMethodNicFirst,&TpostprocSettings::nicFirst              ,0,0,_l(""),1,
     _l("postprocMethodNicFirst"),0,
   IDFF_ppqual                ,&TpostprocSettings::qual                  ,0,6,_l(""),1,
     _l("ppqual"),6,
   IDFF_autoq                 ,&TpostprocSettings::autoq                 ,0,0,_l(""),1,
     _l("autoq"),0,
   IDFF_ppIsCustom            ,&TpostprocSettings::isCustom              ,0,0,_l(""),1,
     _l("ppIsCustom"),0,
   IDFF_ppcustom              ,&TpostprocSettings::custom                ,1,1,_l(""),1,
     _l("ppcustom"),0,
   IDFF_deblockMplayerAccurate,&TpostprocSettings::deblockMplayerAccurate,0,0,_l(""),1,
     _l("deblockMplayerAccurate"),0,
   IDFF_deblockStrength       ,&TpostprocSettings::deblockStrength       ,0,512,_l(""),1,
     _l("deblockStrength"),deblockStrengthDef,
   IDFF_levelFixLum           ,&TpostprocSettings::levelFixLum           ,0,0,_l(""),1,
     _l("levelFixLum"),0,
/*
   IDFF_levelFixChrom         ,&TpostprocSettings::levelFixChrom         ,0,0,_l(""),1,
     _l("levelFixChrom"),0,
*/
   IDFF_fullYrange            ,&TpostprocSettings::fullYrange            ,0,0,_l(""),1,
     _l("fullYrange"),0,
   IDFF_postprocNicXthresh    ,&TpostprocSettings::nicXthresh            ,0,255,_l(""),1,
     _l("postprocNicXthresh"),20,
   IDFF_postprocNicYthresh    ,&TpostprocSettings::nicYthresh            ,0,255,_l(""),1,
     _l("postprocNicYthresh"),40,
   IDFF_postprocSPPmode       ,&TpostprocSettings::sppMode               ,0,1,_l(""),1,
     _l("postprocSPPmode"),0,
#ifdef H264PP
   IDFF_postprocH264mode      ,&TpostprocSettings::h264mode              ,0,3,_l(""),1,
     _l("postprocH264mode"),3,  
#endif
   0
  };
 addOptions(iopts);
 //static const TcreateParamList1 listH264modes(TpostprocSettings::h264modes);setParamList(IDFF_postprocH264mode,&listH264modes);
}

void TpostprocSettings::createFilters(size_t filtersorder,Tfilters *filters,TfilterQueue &queue) const
{
 idffOnChange(idffs,filters,queue.temporary);
 if (is && show)
  {
   if (!queue.temporary)
    {
     setOnChange(IDFF_postprocMethod,filters,&Tfilters::onQueueChange);
     setOnChange(IDFF_postprocMethodNicFirst,filters,&Tfilters::onQueueChange);
     setOnChange(IDFF_postprocH264mode,filters,&Tfilters::onQueueChange);
    } 
   if (h264mode!=1)
    switch (method)
     {
      case 0:
       queueFilter<TimgFilterPostprocMplayer>(filtersorder,filters,queue); break;
      case 1: 
       queueFilter<TimgFilterPostprocNic>(filtersorder,filters,queue); break;
      case 2:
       if (nicFirst)
        {
         queueFilter<TimgFilterPostprocNic>(filtersorder,filters,queue); 
         queueFilter<TimgFilterPostprocMplayer>(filtersorder,filters,queue); 
        }
       else
        {
         queueFilter<TimgFilterPostprocMplayer>(filtersorder,filters,queue); 
         queueFilter<TimgFilterPostprocNic>(filtersorder,filters,queue); 
        }
       break;
      case 4:
       queueFilter<TimgFilterPostprocSpp>(filtersorder,filters,queue); break;
      case 5:
       queueFilter<TimgFilterPostprocFspp>(filtersorder,filters,queue); break;
     }
#ifdef H264PP
   if (h264mode!=0)
    queueFilter<TimgFilterPostprocH264>(filtersorder,filters,queue); 
#endif
  }  
}
void TpostprocSettings::createPages(TffdshowPageDec *parent) const
{
 parent->addFilterPage<TpostProcPage>(&idffs);
}

const int* TpostprocSettings::getResets(unsigned int pageId)
{
 static const int idResets[]={
  IDFF_postprocMethod,IDFF_postprocMethodNicFirst,
  IDFF_ppIsCustom,IDFF_ppcustom,
  IDFF_ppqual,IDFF_autoq,
  IDFF_deblockMplayerAccurate,
  IDFF_deblockStrength,
  IDFF_levelFixLum,/*IDFF_levelFixChrom,*/IDFF_fullYrange,
  IDFF_postprocNicXthresh,IDFF_postprocNicYthresh,
  IDFF_postprocSPPmode,IDFF_postprocH264mode,
  0};
 return idResets;
}

bool TpostprocSettings::getTip(unsigned int pageId,char_t *tipS,size_t len)
{
 if (isCustom)
  {
   tsprintf(tipS,_l("custom: %s%s%s%s%s%s"),custom&LUM_V_DEBLOCK?_l("luma deblock (V),"):_l(""),custom&LUM_H_DEBLOCK?_l("luma deblock (H),"):_l(""),custom&CHROM_V_DEBLOCK?_l("chroma deblock (V),"):_l(""),custom&CHROM_H_DEBLOCK?_l("chroma deblock (H),"):_l(""),custom&LUM_DERING?_l("luma dering,"):_l(""),custom&CHROM_DERING?_l("chroma dering,"):_l(""));
   char_t *c=strrchr(tipS,',');if (c) *c='\n'; else strcat(tipS,_l("\n"));
  } 
 else
  {
   strcpy(tipS,_l("presets: "));
   strcatf(tipS,_l("strength %i"),qual);
   if (autoq) strcat(tipS,_l(" automatic"));
   strcat(tipS,_l("\n"));
  }
 //if (levelFixChrom || levelFixLum) strcat(tipS,_l("level fix\n"));
 strcatf(tipS,_l("processing strength: %i\n"),deblockStrength);
 switch (method)
  {
   case 0:strcat(tipS,_l("mplayer"));break;
   case 1:strcat(tipS,_l("Nic's"));break;
   case 2:strcat(tipS,nicFirst?_l("Nic's + mplayer"):_l("mplayer + Nic's"));break;
   case 4:strcat(tipS,_l("SPP"));break;
   case 5:strcat(tipS,_l("fast SPP"));break;
  }
 return true;
}
