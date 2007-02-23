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
#include "TavisynthSettings.h"
#include "avisynth/TimgFilterAvisynth.h"
#include "Cavisynth.h"
#include "TffdshowPageDec.h"

const TfilterIDFF TavisynthSettings::idffs=
{
 /*name*/      _l("Avisynth"),
 /*id*/        IDFF_filterAvisynth,
 /*is*/        IDFF_isAvisynth,
 /*order*/     IDFF_orderAvisynth,
 /*show*/      IDFF_showAvisynth,
 /*full*/      IDFF_fullAvisynth,
 /*half*/      0,
 /*dlgId*/     IDD_AVISYNTH,
};

TavisynthSettings::TavisynthSettings(TintStrColl *Icoll,TfilterIDFFs *filters):TfilterSettingsVideo(sizeof(*this),Icoll,filters,&idffs)
{
 half=0;
 memset(script,0,sizeof(script));
 static const TintOptionT<TavisynthSettings> iopts[]=
  {
   IDFF_isAvisynth               ,&TavisynthSettings::is           ,0,0,_l(""),1,
     _l("isAvisynth"),0,
   IDFF_showAvisynth             ,&TavisynthSettings::show         ,0,0,_l(""),1,
     _l("showAvisynth"),1,
   IDFF_orderAvisynth            ,&TavisynthSettings::order        ,1,1,_l(""),1,
     _l("orderAvisynth"),0,
   IDFF_fullAvisynth             ,&TavisynthSettings::full         ,0,0,_l(""),1,
     _l("fullAvisynth"),0,
   IDFF_avisynthInYV12           ,&TavisynthSettings::inYV12       ,0,0,_l(""),1,
     _l("avisynthInYV12"),1,
   IDFF_avisynthInYUY2           ,&TavisynthSettings::inYUY2       ,0,0,_l(""),1,
     _l("avisynthInYUY2"),1,
   IDFF_avisynthInRGB24          ,&TavisynthSettings::inRGB24      ,0,0,_l(""),1,
     _l("avisynthInRGB24"),1,
   IDFF_avisynthInRGB32          ,&TavisynthSettings::inRGB32      ,0,0,_l(""),1,
     _l("avisynthInRGB32"),1,
   IDFF_avisynthFfdshowSource    ,&TavisynthSettings::ffdshowSource,0,0,_l(""),1,
     _l("avisynthFfdshowSource"),1,
   0
  };
 addOptions(iopts);
 static const TstrOption sopts[]=
  {
   IDFF_avisynthScript,(TstrVal)&TavisynthSettings::script,2048,_l(""),1,
     _l("avisynthScript"),_l(""),
   0
  };
 addOptions(sopts);
}

void TavisynthSettings::createFilters(size_t filtersorder,Tfilters *filters,TfilterQueue &queue) const
{
 idffOnChange(idffs,filters,queue.temporary);
 if (is && show)
  queueFilter<TimgFilterAvisynth>(filtersorder,filters,queue);
}
void TavisynthSettings::createPages(TffdshowPageDec *parent) const
{
 parent->addFilterPage<TavisynthPage>(&idffs);
}

const int* TavisynthSettings::getResets(unsigned int pageId)
{
 static const int idResets[]={IDFF_avisynthInYV12,IDFF_avisynthInYUY2,IDFF_avisynthInRGB24,IDFF_avisynthInRGB32,IDFF_avisynthFfdshowSource,0};
 return idResets;
}
