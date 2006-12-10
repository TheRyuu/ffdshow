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
#include "TmixerSettings.h"
#include "TaudioFilterMixer.h"
#include "TaudioFilterHeadphone.h"
#include "Cmixer.h"
#include "TffdshowPageDec.h"

const TmixerSettings::TchConfig TmixerSettings::chConfigs[]=
{
  0,_l("1/0 - mono"),            0, 1, 0                                                                                                , TsampleFormat::DOLBY_NO,
  1,_l("2/0 - stereo"),          1, 2, 0                                                                                                , TsampleFormat::DOLBY_NO,
  2,_l("3/0 - 3 front"),         2, 3, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER                                      , TsampleFormat::DOLBY_NO,
  3,_l("2/1 - surround"),        3, 3, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_BACK_CENTER                                       , TsampleFormat::DOLBY_NO,
  4,_l("3/1 - surround"),        4, 4, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER|SPEAKER_BACK_CENTER                  , TsampleFormat::DOLBY_NO,
  5,_l("2/2 - quadro"),          5, 4, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_BACK_LEFT|SPEAKER_BACK_RIGHT                      , TsampleFormat::DOLBY_NO,
  6,_l("3/2 - 5 channels"),      6, 5, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER|SPEAKER_BACK_LEFT|SPEAKER_BACK_RIGHT , TsampleFormat::DOLBY_NO,
                                
  7,_l("1/0+LFE 1.1 mono"),      0, 2, SPEAKER_FRONT_CENTER                                                                            |SPEAKER_LOW_FREQUENCY , TsampleFormat::DOLBY_NO,
  8,_l("2/0+LFE 2.1 stereo"),    1, 3, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT                                                          |SPEAKER_LOW_FREQUENCY , TsampleFormat::DOLBY_NO,
  9,_l("3/0+LFE 3.1 front"),     2, 4, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER                                     |SPEAKER_LOW_FREQUENCY , TsampleFormat::DOLBY_NO,
 10,_l("2/1+LFE 3.1 surround"),  3, 4, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_BACK_CENTER                                      |SPEAKER_LOW_FREQUENCY , TsampleFormat::DOLBY_NO,
 11,_l("3/1+LFE 4.1 surround"),  4, 5, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER|SPEAKER_BACK_CENTER                 |SPEAKER_LOW_FREQUENCY , TsampleFormat::DOLBY_NO,
 12,_l("2/2+LFE 4.1 quadro"),    5, 5, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_BACK_LEFT|SPEAKER_BACK_RIGHT                     |SPEAKER_LOW_FREQUENCY , TsampleFormat::DOLBY_NO,
 13,_l("3/2+LFE 5.1 channels"),  6, 6, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER|SPEAKER_BACK_LEFT|SPEAKER_BACK_RIGHT|SPEAKER_LOW_FREQUENCY , TsampleFormat::DOLBY_NO,
                                
 14,_l("Dolby Surround/ProLogic"), 14, 2, 0,TsampleFormat::DOLBY_SURROUND  ,
 15,_l("Dolby ProLogic II"),       15, 2, 0,TsampleFormat::DOLBY_PROLOGICII,

 16,_l("same as input"),           -1, 0, 0, TsampleFormat::DOLBY_NO,
 
 17,_l("headphone virtual spatialization"),     -1, 2, 0, TsampleFormat::DOLBY_NO,
 18,_l("Head-related transfer function (HRTF)"),-1, 2, 0, TsampleFormat::DOLBY_NO,

 19,_l("Dolby Surround/ProLogic+LFE"), 14, 3, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_LOW_FREQUENCY,TsampleFormat::DOLBY_SURROUND  ,
 20,_l("Dolby ProLogic II+LFE"),       15, 3, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_LOW_FREQUENCY,TsampleFormat::DOLBY_PROLOGICII,
 
 NULL
};

const TfilterIDFF TmixerSettings::idffs=
{
 /*name*/      _l("Mixer"),
 /*id*/        IDFF_filterMixer,
 /*is*/        IDFF_isMixer,
 /*order*/     IDFF_orderMixer,
 /*show*/      IDFF_showMixer,
 /*full*/      0,
 /*half*/      0,
 /*dlgId*/     IDD_MIXER,
};

TmixerSettings::TmixerSettings(TintStrColl *Icoll,TfilterIDFFs *filters):TfilterSettingsAudio(sizeof(*this),Icoll,filters,&idffs)
{
 static const TintOptionT<TmixerSettings> iopts[]=
  {
   IDFF_isMixer          ,&TmixerSettings::is             ,0,0,_l(""),1, 
     _l("isMixer"),0,
   IDFF_showMixer        ,&TmixerSettings::show           ,0,0,_l(""),1,
     _l("showMixer"),1,
   IDFF_orderMixer       ,&TmixerSettings::order          ,1,1,_l(""),1,
     _l("orderMixer"),0,
   IDFF_mixerOut         ,&TmixerSettings::out            ,0,20,_l(""),1,
     _l("mixerOut"),14,
   IDFF_normalizeMatrix  ,&TmixerSettings::normalizeMatrix,0,0,_l(""),1,
     _l("mixerNormalizeMatrix"),1,
   IDFF_mixerExpandStereo,&TmixerSettings::expandStereo   ,0,0,_l(""),1,
     _l("mixerExpandStereo"),1,
   IDFF_mixerVoiceControl,&TmixerSettings::voiceControl   ,0,0,_l(""),1,
     _l("mixerVoiceControl"),1,
   IDFF_customMatrix     ,&TmixerSettings::customMatrix   ,0,0,_l(""),1,
     _l("mixerCustomMatrix"),0,
   IDFF_mixerMatrix00    ,&TmixerSettings::matrix00       ,-4*100000,4*100000,_l(""),1,
     _l("mixerMatrix00"),100000,
   IDFF_mixerMatrix01    ,&TmixerSettings::matrix01       ,-4*100000,4*100000,_l(""),1,
     _l("mixerMatrix02"),0,
   IDFF_mixerMatrix02    ,&TmixerSettings::matrix02       ,-4*100000,4*100000,_l(""),1,
     _l("mixerMatrix01"),0,
   IDFF_mixerMatrix03    ,&TmixerSettings::matrix03       ,-4*100000,4*100000,_l(""),1,
     _l("mixerMatrix05"),0,
   IDFF_mixerMatrix04    ,&TmixerSettings::matrix04       ,-4*100000,4*100000,_l(""),1,
     _l("mixerMatrix03"),0,
   IDFF_mixerMatrix05    ,&TmixerSettings::matrix05       ,-4*100000,4*100000,_l(""),1,
     _l("mixerMatrix04"),0,
   IDFF_mixerMatrix10    ,&TmixerSettings::matrix10       ,-4*100000,4*100000,_l(""),1,
     _l("mixerMatrix20"),0,
   IDFF_mixerMatrix11    ,&TmixerSettings::matrix11       ,-4*100000,4*100000,_l(""),1,
     _l("mixerMatrix22"),100000,
   IDFF_mixerMatrix12    ,&TmixerSettings::matrix12       ,-4*100000,4*100000,_l(""),1,
     _l("mixerMatrix21"),0,
   IDFF_mixerMatrix13    ,&TmixerSettings::matrix13       ,-4*100000,4*100000,_l(""),1,
     _l("mixerMatrix25"),0,
   IDFF_mixerMatrix14    ,&TmixerSettings::matrix14       ,-4*100000,4*100000,_l(""),1,
     _l("mixerMatrix23"),0,
   IDFF_mixerMatrix15    ,&TmixerSettings::matrix15       ,-4*100000,4*100000,_l(""),1,
     _l("mixerMatrix24"),0,
   IDFF_mixerMatrix20    ,&TmixerSettings::matrix20       ,-4*100000,4*100000,_l(""),1,
     _l("mixerMatrix10"),0,
   IDFF_mixerMatrix21    ,&TmixerSettings::matrix21       ,-4*100000,4*100000,_l(""),1,
     _l("mixerMatrix12"),0,
   IDFF_mixerMatrix22    ,&TmixerSettings::matrix22       ,-4*100000,4*100000,_l(""),1,
     _l("mixerMatrix11"),100000,
   IDFF_mixerMatrix23    ,&TmixerSettings::matrix23       ,-4*100000,4*100000,_l(""),1,
     _l("mixerMatrix15"),0,
   IDFF_mixerMatrix24    ,&TmixerSettings::matrix24       ,-4*100000,4*100000,_l(""),1,
     _l("mixerMatrix13"),0,
   IDFF_mixerMatrix25    ,&TmixerSettings::matrix25       ,-4*100000,4*100000,_l(""),1,
     _l("mixerMatrix14"),0,
   IDFF_mixerMatrix30    ,&TmixerSettings::matrix30       ,-4*100000,4*100000,_l(""),1,
     _l("mixerMatrix50"),0,
   IDFF_mixerMatrix31    ,&TmixerSettings::matrix31       ,-4*100000,4*100000,_l(""),1,
     _l("mixerMatrix52"),0,
   IDFF_mixerMatrix32    ,&TmixerSettings::matrix32       ,-4*100000,4*100000,_l(""),1,
     _l("mixerMatrix51"),0,
   IDFF_mixerMatrix33    ,&TmixerSettings::matrix33       ,-4*100000,4*100000,_l(""),1,
     _l("mixerMatrix55"),100000,
   IDFF_mixerMatrix34    ,&TmixerSettings::matrix34       ,-4*100000,4*100000,_l(""),1,
     _l("mixerMatrix53"),0,
   IDFF_mixerMatrix35    ,&TmixerSettings::matrix35       ,-4*100000,4*100000,_l(""),1,
     _l("mixerMatrix54"),0,
   IDFF_mixerMatrix40    ,&TmixerSettings::matrix40       ,-4*100000,4*100000,_l(""),1,
     _l("mixerMatrix30"),0,
   IDFF_mixerMatrix41    ,&TmixerSettings::matrix41       ,-4*100000,4*100000,_l(""),1,
     _l("mixerMatrix32"),0,
   IDFF_mixerMatrix42    ,&TmixerSettings::matrix42       ,-4*100000,4*100000,_l(""),1,
     _l("mixerMatrix31"),0,
   IDFF_mixerMatrix43    ,&TmixerSettings::matrix43       ,-4*100000,4*100000,_l(""),1,
     _l("mixerMatrix35"),0,
   IDFF_mixerMatrix44    ,&TmixerSettings::matrix44       ,-4*100000,4*100000,_l(""),1,
     _l("mixerMatrix33"),100000,
   IDFF_mixerMatrix45    ,&TmixerSettings::matrix45       ,-4*100000,4*100000,_l(""),1,
     _l("mixerMatrix34"),0,
   IDFF_mixerMatrix50    ,&TmixerSettings::matrix50       ,-4*100000,4*100000,_l(""),1,
     _l("mixerMatrix40"),0,
   IDFF_mixerMatrix51    ,&TmixerSettings::matrix51       ,-4*100000,4*100000,_l(""),1,
     _l("mixerMatrix42"),0,
   IDFF_mixerMatrix52    ,&TmixerSettings::matrix52       ,-4*100000,4*100000,_l(""),1,
     _l("mixerMatrix41"),0,
   IDFF_mixerMatrix53    ,&TmixerSettings::matrix53       ,-4*100000,4*100000,_l(""),1,
     _l("mixerMatrix45"),0,
   IDFF_mixerMatrix54    ,&TmixerSettings::matrix54       ,-4*100000,4*100000,_l(""),1,
     _l("mixerMatrix43"),0,
   IDFF_mixerMatrix55    ,&TmixerSettings::matrix55       ,-4*100000,4*100000,_l(""),1,
     _l("mixerMatrix44"),100000,
   IDFF_mixerClev        ,&TmixerSettings::clev           ,0,3200,_l(""),1,
     _l("mixerClev"),100,
   IDFF_mixerSlev        ,&TmixerSettings::slev           ,0,3200,_l(""),1,
     _l("mixerSlev"),100,
   IDFF_mixerLFElev      ,&TmixerSettings::lfelev         ,0,3200,_l(""),1,
     _l("mixerLFElev"),100,
   IDFF_headphone_dim    ,&TmixerSettings::headphone_dim  ,0,100,_l(""),1,
     _l("headphone_dim"),10,
   0
  };
 addOptions(iopts);
 static const TcreateParamList2<TchConfig> listMixerOut(chConfigs,&TchConfig::name);setParamList(IDFF_mixerOut,&listMixerOut);
}

void TmixerSettings::createFilters(size_t filtersorder,Tfilters *filters,TfilterQueue &queue) const
{
 idffOnChange(idffs,filters,queue.temporary);
 if (is && show)
  {
   if (!queue.temporary) setOnChange(IDFF_mixerOut,filters,&Tfilters::onQueueChange);
   switch (out)
    {
     case 17:queueFilter<TaudioFilterHeadphone>(filtersorder,filters,queue);break;
     case 18:queueFilter<TaudioFilterHeadphone2>(filtersorder,filters,queue);break;
     default:queueFilter<TaudioFilterMixer>(filtersorder,filters,queue);break;
    } 
  }  
}
void TmixerSettings::createPages(TffdshowPageDec *parent) const
{
 parent->addFilterPage<TmixerPage>(&idffs);
}

const int* TmixerSettings::getResets(unsigned int pageId)
{
 static const int idResets[]={
  IDFF_mixerOut,
  IDFF_normalizeMatrix,
  IDFF_mixerExpandStereo,IDFF_mixerVoiceControl,
  IDFF_mixerClev,IDFF_mixerSlev,IDFF_mixerLFElev,
  IDFF_customMatrix,
  IDFF_mixerMatrix00,IDFF_mixerMatrix01,IDFF_mixerMatrix02,IDFF_mixerMatrix03,IDFF_mixerMatrix04,IDFF_mixerMatrix05,IDFF_mixerMatrix10,IDFF_mixerMatrix11,IDFF_mixerMatrix12,IDFF_mixerMatrix13,IDFF_mixerMatrix14,IDFF_mixerMatrix15,IDFF_mixerMatrix20,IDFF_mixerMatrix21,IDFF_mixerMatrix22,IDFF_mixerMatrix23,IDFF_mixerMatrix24,IDFF_mixerMatrix25,IDFF_mixerMatrix30,IDFF_mixerMatrix31,IDFF_mixerMatrix32,IDFF_mixerMatrix33,IDFF_mixerMatrix34,IDFF_mixerMatrix35,IDFF_mixerMatrix40,IDFF_mixerMatrix41,IDFF_mixerMatrix42,IDFF_mixerMatrix43,IDFF_mixerMatrix44,IDFF_mixerMatrix45,IDFF_mixerMatrix50,IDFF_mixerMatrix51,IDFF_mixerMatrix52,IDFF_mixerMatrix53,IDFF_mixerMatrix54,IDFF_mixerMatrix55,
  IDFF_headphone_dim,
  0};
 return idResets;
}

bool TmixerSettings::getTip(unsigned int pageId,char_t *tipS,size_t len)
{
 tsnprintf(tipS,len,_l("%s%s%s%s%s"),chConfigs[out].name,(out!=17 && out!=18 && customMatrix)?_l("\ncustom matrix"):_l(""),(out!=17 && normalizeMatrix)?_l("\nnormalize matrix"):_l(""),(out!=17 && expandStereo && !customMatrix)?_l("\nexpand stereo"):_l(""),(out!=17 && voiceControl && !customMatrix)?_l("\nvoice control"):_l(""));
 return true;
}

void TmixerSettings::setFormatOut(TsampleFormat &fmt) const
{
 if (out!=16)
  fmt.setChannels(chConfigs[out].nchannels,chConfigs[out].channelmask);
} 
void TmixerSettings::setFormatOut(TsampleFormat &outfmt,const TsampleFormat &infmt) const
{
 outfmt.setChannels(out==16?infmt.nchannels:chConfigs[out].nchannels,out==16?infmt.channelmask:chConfigs[out].channelmask);
 outfmt.dolby=out==16?infmt.dolby:chConfigs[out].dolby;
}

//===============================
/*
 for (int i=0;i<6;i++)
  for (int j=0;j<6;j++)
   {
    char pomS[40];
    static const int o[6]={0,2,1,5,3,4};
    sprintf(pomS,"mixerMatrix%i%i",o[i],o[j]);
    t._REG_OP_N(pomS,matrix[i][j],i==j?100000:0);
   }
*/
