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
#include "TimgFilterSubtitles.h"
#include "TsubtitlesSettings.h"
#include "IffdshowBase.h"
#include "IffdshowDecVideo.h"
#include "TimgFilterExpand.h"
#include "TglobalSettings.h"
#include "TimgFilters.h"
#include "Tsubtitle.h"
#include "TsubtitleText.h"
#include "Tsubreader.h"
#include "TsubtitlesTextpin.h"

TimgFilterSubtitles::TsubPrintPrefs::TsubPrintPrefs(unsigned char *Idst[4],stride_t Istride[4],unsigned int Idx[4],unsigned int Idy[4],IffdshowBase *Ideci,const TsubtitlesSettings *cfg,const TffPict &pict,const Tconfig *Iconfig,bool Idvd):TrenderedSubtitleLines::TprintPrefs(Ideci)
{
 dst=Idst;
 stride=Istride;
 shiftX=pict.cspInfo.shiftX;shiftY=pict.cspInfo.shiftY;
 dx=Idx[0];
 dy=Idy[0];
 xpos=cfg->posX;
 ypos=cfg->posY;
 align=cfg->align;
 linespacing=cfg->linespacing;
 vobchangeposition=!!cfg->vobsubChangePosition;
 vobscale=cfg->vobsubScale;
 vobaamode=cfg->vobsubAA;
 vobaagauss=cfg->vobsubAAswgauss;
 textBorderLR=2*cfg->splitBorder;
 deci=Ideci;
 config=Iconfig;
 dvd=Idvd;
 // Copy subtitles shadow vars
 shadowSize = cfg->shadowSize;
 shadowAlpha = cfg->shadowAlpha;
 shadowMode = cfg->shadowMode;
}

TimgFilterSubtitles::TimgFilterSubtitles(IffdshowBase *Ideci,Tfilters *Iparent):
 TimgFilter(Ideci,Iparent),
 font(Ideci),fontCC(Ideci),
 subs(Ideci),
 oldFontCfg((TfontSettingsSub*)malloc(sizeof(TfontSettingsSub))),
 oldFontCCcfg((TfontSettingsSub*)malloc(sizeof(TfontSettingsSub))),
 cc(NULL),wasCCchange(true)
{
 oldFontCfg->weight=oldFontCCcfg->weight=-1;oldstereo=oldsplitborder=-1;
 AVIfps=-1;
 expand=NULL;
 expandSizeChanged=fontSizeChanged=true;oldExpandCode=-1;
 oldSizeDx=oldSizeDy=0;
 isdvdproc=false;
 wasDiscontinuity=true;
 again=false;
 prevCfg=NULL;
 subFlnmChanged=1;
}
TimgFilterSubtitles::~TimgFilterSubtitles()
{
 if (expand) delete expand;
 for (Tembedded::iterator e=embedded.begin();e!=embedded.end();e++)
  if (e->second)
   delete e->second;
 ::free(oldFontCfg);::free(oldFontCCcfg);
 if (cc) delete cc;
}

void TimgFilterSubtitles::onSizeChange(void)
{
 expandSizeChanged=fontSizeChanged=true;
}
void TimgFilterSubtitles::onSubFlnmChange(int id,int)
{
 subFlnmChanged=id?id:-1;
}
void TimgFilterSubtitles::onSubFlnmChangeStr(int id,const char_t*)
{
 subFlnmChanged=id?id:-1;
}

bool TimgFilterSubtitles::is(const TffPictBase &pict,const TfilterSettingsVideo *cfg)
{
 isdvdproc=deci->getParam2(IDFF_dvdproc);
 return isdvdproc || super::is(pict,cfg);
}

bool TimgFilterSubtitles::getOutputFmt(TffPictBase &pict,const TfilterSettingsVideo *cfg0)
{
 if (super::getOutputFmt(pict,cfg0))
  {
   const TsubtitlesSettings *cfg=(const TsubtitlesSettings*)cfg0;
   isdvdproc=deci->getParam2(IDFF_dvdproc);
   if (cfg->isExpand && cfg->expandCode && !isdvdproc)
    {
     const char_t *subflnm=cfg->autoFlnm?findAutoSubFlnm(cfg):cfg->flnm;
     if ((subflnm[0]=='\0' || !fileexists(subflnm)) && !deci->getParam2(IDFF_subTextpin)) return true;
     if (!expand) expand=new TimgFilterExpand(deci,parent);
     int a1,a2;
     cfg->getExpand(&a1,&a2);
     Trect::calcNewSizeAspect(pict.rectFull,a1,a2,expandSettings.newrect);
     expand->getOutputFmt(pict,&expandSettings);
    }
   return true;
  }
 else
  return false;
}

bool TimgFilterSubtitles::initSubtitles(int id,int type,const unsigned char *extradata,unsigned int extradatalen)
{
 csEmbedded.Lock();
 Tembedded::iterator e=embedded.find(id);
 if (e!=embedded.end())
  {
   delete e->second;
   e->second=TsubtitlesTextpin::create(type,extradata,extradatalen,deci);
  }
 else
  {
   e=embedded.insert(std::make_pair(id,TsubtitlesTextpin::create(type,extradata,extradatalen,deci))).first;
  }
 csEmbedded.Unlock();
 if(!e->second)
  return false;
 return *e->second;
}
void TimgFilterSubtitles::addSubtitle(int id,REFERENCE_TIME start,REFERENCE_TIME stop,const unsigned char *data,unsigned int datalen,const TsubtitlesSettings *cfg,bool utf8)
{
 Tembedded::iterator e=embedded.find(id);
 if (e==embedded.end()) return;
 csEmbedded.Lock();
 e->second->setModified();
 e->second->addSubtitle(start,stop,data,datalen,cfg,utf8);
 csEmbedded.Unlock();
}
void TimgFilterSubtitles::resetSubtitles(int id)
{
 Tembedded::iterator e=embedded.find(id);
 if (e==embedded.end()) return;
 csEmbedded.Lock();
 e->second->resetSubtitles();
 csEmbedded.Unlock();
}
bool TimgFilterSubtitles::ctlSubtitles(int id,int type,unsigned int ctl_id,const void *ctl_data,unsigned int ctl_datalen)
{
 Tembedded::iterator e=embedded.find(id);
 if (e==embedded.end())
  e=embedded.insert(std::make_pair(id,TsubtitlesTextpin::create(type,NULL,0,deci))).first;
 csEmbedded.Lock();
 bool res=e->second->ctlSubtitles(ctl_id,ctl_data,ctl_datalen);
 csEmbedded.Unlock();
 if (res && prevCfg)
  {
   again=true;
   TffPict pict=prevPict;
   process(prevIt,pict,prevCfg);
   again=false;
  }
 return res;
}

const char_t* TimgFilterSubtitles::findAutoSubFlnm(const TsubtitlesSettings *cfg)
{
 struct TcheckSubtitle : IcheckSubtitle
  {
  private:
   TsubtitlesFile subs;
   const TsubtitlesSettings *cfg;
   double fps;
  public:
   TcheckSubtitle(IffdshowBase *deci,const TsubtitlesSettings *Icfg,double Ifps):subs(deci),cfg(Icfg),fps(Ifps) {}
   STDMETHODIMP checkSubtitle(const char_t *subFlnm)
    {
     return subs.init(cfg,subFlnm,fps,false,2);
    }
  } checkSubtitle(deci,cfg,25);
 return deciV->findAutoSubflnms(&checkSubtitle);
}

HRESULT TimgFilterSubtitles::process(TfilterQueue::iterator it,TffPict &pict,const TfilterSettingsVideo *cfg0)
{
 const TsubtitlesSettings *cfg=(const TsubtitlesSettings*)cfg0;

 if (cfg->isExpand && cfg->expandCode && !isdvdproc)
  {
   Trect newExpandRect=cfg->full?pict.rectFull:pict.rectClip;
   if (expandSizeChanged || oldExpandCode!=cfg->expandCode || oldExpandRect!=newExpandRect)
    {
     oldExpandCode=cfg->expandCode;oldExpandRect=newExpandRect;
     if (expand) delete expand;expand=NULL;
     TffPict newpict;newpict.rectFull=pict.rectFull;newpict.rectClip=pict.rectClip;
     getOutputFmt(newpict,cfg);
     parent->dirtyBorder=1;
     expandSizeChanged=false;
    }
   if (expand)
    expand->process(NULL,pict,&expandSettings);
  }

 if (AVIfps==-1) AVIfps=deciV->getAVIfps1000_2()/1000.0;
 if (subFlnmChanged)
  {
   const char_t *subflnm=cfg->autoFlnm?findAutoSubFlnm(cfg):cfg->flnm;
   if (subFlnmChanged!=-1 || stricmp(subflnm,subs.subFlnm)!=0)
    subs.init(cfg,subflnm,AVIfps,!!deci->getParam2(IDFF_subWatch),false);
   subFlnmChanged=0;
  }
 if (subs || !embedded.empty())
  {
   REFERENCE_TIME frameStart=cfg->speed2*((pict.rtStart-parent->subtitleResetTime)-cfg->delay*(REF_SECOND_MULT/1000))/cfg->speed;
   bool forceChange=false;
   const Tsubtitle *sub=NULL;
   csEmbedded.Lock();
   int shownEmbedded=deci->getParam2(IDFF_subShowEmbedded);
   bool useembedded=!embedded.empty() && shownEmbedded;
   if (useembedded)
    {
     Tembedded::iterator e=embedded.find(shownEmbedded);
     if (e!=embedded.end() && e->second)
      {
       if (!e->second->IsProcessOverlapDone())
        e->second->processOverlap();
       sub=e->second->getSubtitle(cfg,frameStart,&forceChange);
      }
    }
   csEmbedded.Unlock();
   if (!useembedded)
    sub=subs.getSubtitle(cfg,frameStart,&forceChange);
   if (sub)
    {
     if (!again)
      init(pict,cfg->full,cfg->half);
     if (memcmp(oldFontCfg->name,cfg->font.name,sizeof(TfontSettingsSub)-sizeof(Toptions))!=0 || fontSizeChanged || oldstereo!=cfg->stereoscopic || oldsplitborder!=cfg->splitBorder)
      {
       memcpy(oldFontCfg,&cfg->font,sizeof(cfg->font));oldstereo=cfg->stereoscopic;oldsplitborder=cfg->splitBorder;
       font.init(oldFontCfg);
      }
     fontSizeChanged=false;

     /*
     if (isdvdproc)
      {
       if (!again || !prevCfg)
        {
         prevIt=it;
         prevCfg=cfg;
         prevPict=pict;
         pict.setRO(true);
         prevPict.copyFrom(pict,prevbuf);
        }
       else
        {
         pict=prevPict;
         pict.setRO(true);
        }
      }*/
     unsigned char *dst[4];
     getCurNext(FF_CSP_420P,pict,cfg->full,COPYMODE_DEF,dst);
     unsigned int sizeDx,sizeDy;
     if (cfg->font.autosizeVideoWindow)
      {
       CRect r;
       deciV->getVideoDestRect(&r);
       sizeDx=r.Width();sizeDy=r.Height();
       forceChange|=oldSizeDx!=sizeDx || oldSizeDy!=sizeDy;
       oldSizeDx=sizeDx;oldSizeDy=oldSizeDy;
      }
     else
      {
       sizeDx=pict.rectFull.dx;
       sizeDy=pict.rectFull.dy;
      }

     TsubPrintPrefs printprefs(dst,stride2,dx1,dy1,deci,cfg,pict,parent->config,!!isdvdproc);
     if (!cfg->stereoscopic || isdvdproc)
      {
       printprefs.sizeDx=sizeDx;
       printprefs.sizeDy=sizeDy;
       sub->print(frameStart,wasDiscontinuity,font,forceChange,printprefs);
      }
     else
      {
       printprefs.sizeDx=sizeDx;
       printprefs.sizeDy=sizeDy/2;
       printprefs.posXpix=cfg->stereoscopic?cfg->stereoscopicPar*int(sizeDx)/2000:0;
       sub->print(frameStart,wasDiscontinuity,font,forceChange,printprefs);
       for (unsigned int i=0;i<pict.cspInfo.numPlanes;i++)
       printprefs.dst[i]+=stride2[i]*(dy1[0]/2)>>pict.cspInfo.shiftY[i];
       printprefs.posXpix=-printprefs.posXpix;
       sub->print(frameStart,false,font,false,printprefs);
      }
     wasDiscontinuity=false;
    }
  }

 if (cfg->cc)
  {
   csCC.Lock();
   if (cc && cc->numlines())
    {
     if (!again)
      init(pict,cfg->full,cfg->half);
     unsigned char *dst[4];
     getCurNext(FF_CSP_420P,pict,cfg->full,COPYMODE_DEF,dst);
    #if 0
     TsubtitlesSettings cfg(*cfg);
     cfg.posX=50;
     cfg.posY=100;
     cfg.align=ALIGN_CENTER;
    #else
     const TsubtitlesSettings &cfg2(*cfg);
    #endif
     TsubPrintPrefs printprefs(dst,stride2,dx1,dy1,deci,&cfg2,pict,parent->config,!!isdvdproc);
     printprefs.sizeDx=pict.rectFull.dx;
     printprefs.sizeDy=pict.rectFull.dy;

     if (memcmp(oldFontCCcfg->name,cfg2.font.name,sizeof(TfontSettingsSub)-sizeof(Toptions))!=0 || oldsplitborder!=cfg2.splitBorder)
      {
       memcpy(oldFontCCcfg,&cfg2.font,sizeof(cfg2.font));oldsplitborder=cfg2.splitBorder;
       fontCC.init(oldFontCCcfg);
      }

     cc->print(0,false,fontCC,wasCCchange,printprefs);
     wasCCchange=false;
    }
   csCC.Unlock();
  }
 return parent->deliverSample(++it,pict);
}
void TimgFilterSubtitles::onSeek(void)
{
 wasDiscontinuity=true;
 again=false;
 CAutoLock l(&csCC);
 hideClosedCaptions();
}

const char_t* TimgFilterSubtitles::getCurrentFlnm(void) const
{
 return subs.subFlnm;
}

void TimgFilterSubtitles::addClosedCaption(const char *line)
{
 CAutoLock l(&csCC);
 if (!cc)
  cc=new TsubtitleTextBase<char>(Tsubreader::SUB_SUBRIP);
 cc->add(line);
 TsubtitleFormat format(NULL);
 cc->format(format);
 wasCCchange=true;
}
void TimgFilterSubtitles::hideClosedCaptions(void)
{
 CAutoLock l(&csCC);
 if (cc)
  {
   cc->clear();
   wasCCchange=true;
  }
}
