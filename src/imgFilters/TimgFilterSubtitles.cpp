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
#include "ffdebug.h"

TimgFilterSubtitles::TsubPrintPrefs::TsubPrintPrefs(
        unsigned int Idx[4],
        unsigned int Idy[4],
        IffdshowBase *Ideci,
        const TsubtitlesSettings *cfg,
        const TffPict &pict,
        int Iclipdy,
        const Tconfig *Iconfig,
        bool Idvd,
        const TfontSettings *fontSettings):
    TprintPrefs(Ideci,fontSettings)
{
    csp=pict.csp;
    dx=pict.rectFull.dx;//Idx[0];
    dy=pict.rectFull.dy;//Idy[0];
    clipdy=Iclipdy;
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
    int i;
    deci->getParam(IDFF_fontShadowMode, &shadowMode);
    deci->getParam(IDFF_fontShadowSize, &i);
    shadowSize=i;
    deci->getParam(IDFF_fontShadowAlpha, &shadowAlpha);
    sar=pict.rectFull.sar;
}

TimgFilterSubtitles::TimgFilterSubtitles(IffdshowBase *Ideci,Tfilters *Iparent):
    TimgFilter(Ideci,Iparent),
    font(Ideci),fontCC(Ideci),
    subs(Ideci),
    cc(NULL),wasCCchange(true),everRGB(false),
    adhocMode(ADHOC_NORMAL),
    prevAdhocMode(ADHOC_NORMAL),
    glyphThread(this,Ideci)
{
    oldstereo=oldsplitborder=-1;
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
    glyphThread.done();
    boost::unique_lock<boost::recursive_mutex> lock(csEmbedded);
    if (expand)
        delete expand;
    for (Tembedded::iterator e=embedded.begin();e!=embedded.end();e++)
        if (e->second)
            delete e->second;
    if (cc)
        delete cc;
}

void TimgFilterSubtitles::onSizeChange()
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
    if (super::getOutputFmt(pict,cfg0)) {
        const TsubtitlesSettings *cfg=(const TsubtitlesSettings*)cfg0;
        isdvdproc=deci->getParam2(IDFF_dvdproc);
        if (cfg->isExpand && cfg->expandCode && !isdvdproc) {
            const char_t *subflnm=cfg->autoFlnm?findAutoSubFlnm(cfg):cfg->flnm;
            if ((subflnm[0]=='\0' || !fileexists(subflnm)) && !deci->getParam2(IDFF_subTextpin)) return true;
            if (!expand) expand=new TimgFilterSubtitleExpand(deci,parent);
            int a1,a2;
            cfg->getExpand(&a1,&a2);
            Trect::calcNewSizeAspect(/*cfg->full ? pict.rectFull : */pict.rectClip,a1,a2,expandSettings.newrect);
            expand->getOutputFmt(pict,&expandSettings);
        }
        return true;
 } else
    return false;
}

bool TimgFilterSubtitles::initSubtitles(int id,int type,const unsigned char *extradata,unsigned int extradatalen)
{
    boost::unique_lock<boost::recursive_mutex> lock(csEmbedded);
    Tembedded::iterator e=embedded.find(id);
    if (e!=embedded.end()) {
        delete e->second;
        e->second=TsubtitlesTextpin::create(type,extradata,extradatalen,deci);
    } else {
        e=embedded.insert(std::make_pair(id,TsubtitlesTextpin::create(type,extradata,extradatalen,deci))).first;
    }

    sequenceEnded=true;

    if(!e->second)
         return false;
    return *e->second;
}

void TimgFilterSubtitles::addSubtitle(int id,REFERENCE_TIME start,REFERENCE_TIME stop,const unsigned char *data,unsigned int datalen,const TsubtitlesSettings *cfg,bool utf8)
{
    Tembedded::iterator e=embedded.find(id);
    if (e==embedded.end()) return;
    boost::unique_lock<boost::recursive_mutex> lock(csEmbedded);
    e->second->setModified();
    e->second->addSubtitle(start,stop,data,datalen,cfg,utf8);
}

void TimgFilterSubtitles::resetSubtitles(int id)
{
     Tembedded::iterator e=embedded.find(id);
     if (e==embedded.end()) return;
     {
         boost::unique_lock<boost::recursive_mutex> lock(csEmbedded);
         e->second->resetSubtitles();
     }
     if(isdvdproc) {
         deciV->lockCSReceive();
         parent->onSeek();
         deciV->unlockCSReceive();
     }
}

bool TimgFilterSubtitles::ctlSubtitles(int id,int type,unsigned int ctl_id,const void *ctl_data,unsigned int ctl_datalen)
{
    Tembedded::iterator e=embedded.find(id);
    if (e==embedded.end())
     e=embedded.insert(std::make_pair(id,TsubtitlesTextpin::create(type,NULL,0,deci))).first;
    bool res;
    {
        boost::unique_lock<boost::recursive_mutex> lock(csEmbedded);
        res=e->second->ctlSubtitles(ctl_id,ctl_data,ctl_datalen);
    }

    if (res && prevCfg  && deci->getState2() == State_Running && (sequenceEnded || raw_codec(deci->getCurrentCodecId2()))) {
        // Send last pict with changed subtitles upstream in the filter chain only if the graph is running -
        // doing it while it's paused will hang everything

        deciV->lockCSReceive();

        // Pull image out of yadif's next picture buffer.
        parent->pullImageFromSubtitlesFilter(prevIt);

        TffPict pict=prevPict;
        pict.fieldtype|=FIELD_TYPE::SEQ_START|FIELD_TYPE::SEQ_END;

        again=true;

        if (prevAdhocMode == ADHOC_ADHOC_DRAW_DVD_SUB_ONLY)
         adhocMode = ADHOC_ADHOC_DRAW_DVD_SUB_ONLY;

        process(prevIt,pict,prevCfg);

        again=false;

        deciV->unlockCSReceive();
    }
    return res;
}

const char_t* TimgFilterSubtitles::findAutoSubFlnm(const TsubtitlesSettings *cfg)
{
    struct TcheckSubtitle : IcheckSubtitle {
    private:
        TsubtitlesFile subs;
        const TsubtitlesSettings *cfg;
        double fps;
    public:
        TcheckSubtitle(IffdshowBase *deci,const TsubtitlesSettings *Icfg,double Ifps):subs(deci),cfg(Icfg),fps(Ifps) {}
        STDMETHODIMP checkSubtitle(const char_t *subFlnm) {
            return subs.init(cfg,subFlnm,fps,false,2);
        }
    } checkSubtitle(deci,cfg,25);
    return deciV->findAutoSubflnms(&checkSubtitle);
}

HRESULT TimgFilterSubtitles::process(TfilterQueue::iterator it,TffPict &pict,const TfilterSettingsVideo *cfg0)
{
    // Don't produce extra frames if there is going to be a new frame shortly anyway
    if (pict.fieldtype & FIELD_TYPE::SEQ_END)
        sequenceEnded=true;
    else if (pict.fieldtype & FIELD_TYPE::SEQ_START)
        sequenceEnded=false;

    const TsubtitlesSettings *cfg=(const TsubtitlesSettings*)cfg0;

    // Leter box. Expand rectFull and assign it to rectClip. So "Process whole image" is ignored.
    int clipdy=pict.rectClip.dy; // save clipdy
    if (cfg->isExpand 
       && cfg->expandCode 
       && !isdvdproc 
       && adhocMode != ADHOC_ADHOC_DRAW_DVD_SUB_ONLY) {
        Trect newExpandRect=cfg->full?pict.rectFull:pict.rectClip;
        if (expandSizeChanged || oldExpandCode!=cfg->expandCode || oldExpandRect!=newExpandRect || pict.rectClip!=oldRectClip) {
            oldExpandCode=cfg->expandCode;oldExpandRect=newExpandRect;oldRectClip=pict.rectClip;
            if (expand) delete expand;expand=NULL;
            TffPict newpict;
            newpict.rectFull=pict.rectFull;
            newpict.rectClip=pict.rectClip;
            getOutputFmt(newpict,cfg);
            parent->dirtyBorder=1;
            expandSizeChanged=false;
        }
        if (expand) {
            expand->process(pict,&expandSettings);
            checkBorder(pict);
            pict.rectClip=pict.rectFull;
            pict.calcDiff();
        }
    }

    if (AVIfps==-1) AVIfps=deciV->getAVIfps1000_2()/1000.0;
    if (subFlnmChanged) {
        const char_t *subflnm=cfg->autoFlnm?findAutoSubFlnm(cfg):cfg->flnm;
        if (subFlnmChanged!=-1 || stricmp(subflnm,subs.subFlnm)!=0)
            subs.init(cfg,subflnm,AVIfps,!!deci->getParam2(IDFF_subWatch),false);
        subFlnmChanged=0;
    }

    if (isdvdproc && (sequenceEnded || raw_codec(deci->getCurrentCodecId2())) && adhocMode != ADHOC_SECOND_DONT_DRAW_DVD_SUB) {
        if (!again || !prevCfg) {
            prevIt=it;
            prevCfg=cfg;
            prevPict=pict;
            prevAdhocMode = adhocMode;

            pict.setRO(true);
            prevPict.copyFrom(pict,prevbuf);
        } else {
            REFERENCE_TIME difference=(prevPict.rtStop-prevPict.rtStart)/10;

            if (difference < 10)
             difference=10;

            prevPict.rtStart+=difference;
            prevPict.rtStop+=difference;
            pict=prevPict;

            pict.setRO(true);
        }
    }

    char_t outputfourcc[20];
    deciV->getOutputFourcc(outputfourcc,20);
    bool rgb32_if_possible = (strncmp(outputfourcc,_l("RGB"),3)==0 && !parent->isAnyActiveDownstreamFilter(it)) || pict.csp==FF_CSP_RGB32;

    {
        boost::unique_lock<boost::recursive_mutex> lock(csEmbedded);
        if (subs || !embedded.empty()) {
            REFERENCE_TIME frameStart = cfg->speed2 * ((pict.rtStart - parent->subtitleResetTime) - cfg->delay * (REF_SECOND_MULT / 1000)) / cfg->speed;
            REFERENCE_TIME frameStop  = cfg->speed2 * ((pict.rtStop  - parent->subtitleResetTime) - cfg->delay * (REF_SECOND_MULT / 1000)) / cfg->speed;
            bool forceChange=false;
            Tsubtitle *sub=NULL;
            TsubtitlesTextpin* pin = getTextpin();
            Tsubtitles* subtitles = NULL;
            bool isText = false;
            bool isDVDsub = false;
            int subformat = -1;
            if (pin 
               && (adhocMode == ADHOC_NORMAL 
               || (adhocMode == ADHOC_ADHOC_DRAW_DVD_SUB_ONLY && isdvdproc) 
               || (adhocMode == ADHOC_SECOND_DONT_DRAW_DVD_SUB && !isdvdproc))) {
                 subformat = pin->sub_format;
                 sub = pin->getSubtitle(cfg, frameStart, frameStop, &forceChange);
                 subtitles = pin;
            }
            if (!pin && adhocMode != ADHOC_ADHOC_DRAW_DVD_SUB_ONLY &&  cfg->is) {
                sub           = subs.getSubtitle(cfg, frameStart, frameStop, &forceChange);
                subformat = subs.sub_format;
                subtitles = &subs;
            }
            isText = Tsubreader::isText(subformat);
            isDVDsub = Tsubreader::isDVDsub(subformat);

            bool stereoScopic = cfg->stereoscopic && !isdvdproc && subformat != Tsubreader::SUB_SSA;

            int outcsp = FF_CSP_420P;
            if (rgb32_if_possible && (isText || isDVDsub))
                outcsp = FF_CSP_RGB32;

            unsigned int sizeDx,sizeDy;
            if (cfg->font.autosizeVideoWindow) {
                CRect r;
                deciV->getVideoDestRect(&r);
                sizeDx=r.Width();sizeDy=r.Height();
            } else {
                sizeDx=cfg->full ? pict.rectFull.dx : pict.rectClip.dx;
                sizeDy=cfg->full ? pict.rectFull.dy : pict.rectClip.dy;
            }
            forceChange|=oldSizeDx!=sizeDx || oldSizeDy!=sizeDy;
            oldSizeDx=sizeDx;oldSizeDy=sizeDy;

            TsubPrintPrefs printprefs(dx1,dy1,deci,cfg,pict,clipdy,parent->config,!!isdvdproc,&cfg->font);
            printprefs.csp = outcsp;
            printprefs.subformat = subformat;
            printprefs.rtStart=frameStart;
            printprefs.fontSettings.gdi_font_scale = 64;
            const Trect *decodedPict = deciV->getDecodedPictdimensions();
            if (decodedPict!=NULL) {
                printprefs.xinput = decodedPict->dx;
                printprefs.yinput = decodedPict->dy;
            }

            if (!stereoScopic) {
                printprefs.sizeDx=sizeDx;
                printprefs.sizeDy=sizeDy;
            } else {
                printprefs.sizeDx=sizeDx/2;
                printprefs.sizeDy=sizeDy;
                printprefs.stereoScopicParallax = cfg->stereoscopicPar * int(sizeDx) / 2000;
            }

            if (isText) {
                {
                   boost::unique_lock<boost::mutex> lock(glyphThread.mutex_prefs);
                   glyphThread.shared_prefs = printprefs;
                   glyphThread.threadCmd = 1;
                }
                glyphThread.condv_prefs.notify_one();
            }

            if (printprefs != oldprefs && subtitles && subtitles->subs) {
                subtitles->subs->dropRendered();
                oldprefs = printprefs;
            }

            if (sub && (isdvdproc || cfg->is)) {
                init(pict,cfg->full,cfg->half);

                fontSizeChanged=false;

                unsigned char *dst[4];
                getCurNext(outcsp, pict, cfg->full, COPYMODE_DEF, dst);
                if (outcsp == FF_CSP_RGB32)
                    everRGB = true;

                if (!stereoScopic) {
                    sub->print(frameStart,wasDiscontinuity,font,forceChange,printprefs,dst,stride2);
                } else {
                    sub->print(frameStart,wasDiscontinuity,font,forceChange,printprefs,dst,stride2);
                    unsigned char *dst_right[4] = {dst[0],dst[1],dst[2],dst[3]};
                    if (printprefs.csp == FF_CSP_420P) {
                        int half = (pict.rectClip.dx / 2) & ~1;
                        dst_right[0] += half;
                        dst_right[1] += half / 2;
                        dst_right[2] += half / 2;
                    } else {
                        // RG32
                        dst_right[0] += pict.rectClip.dx * 2;
                    }
                    printprefs.stereoScopicParallax = -printprefs.stereoScopicParallax;
                    sub->print(frameStart,false,font,false,printprefs,dst_right,stride2);
                }
                wasDiscontinuity=false;
            }
        }
    }

    if (cfg->cc && cfg->is && adhocMode != ADHOC_ADHOC_DRAW_DVD_SUB_ONLY) {
        boost::unique_lock<boost::recursive_mutex> lock(csCC);
        if (cc && cc->numlines()) {
            if (!again)
                init(pict,cfg->full,cfg->half);
            unsigned char *dst[4];
            int outcsp;
            if (rgb32_if_possible) {
                outcsp = FF_CSP_RGB32;
                everRGB = true;
            } else {
                outcsp = FF_CSP_420P;
            }
            getCurNext3(outcsp, pict, cfg->full, COPYMODE_DEF, dst);
            const TsubtitlesSettings &cfg2(*cfg);
            TsubPrintPrefs printprefs(dx1,dy1,deci,&cfg2,pict,clipdy,parent->config,!!isdvdproc,&cfg->font);
            printprefs.csp=pict.csp & FF_CSPS_MASK;
            printprefs.sizeDx=pict.rectFull.dx;
            printprefs.sizeDy=pict.rectFull.dy;

            const Trect *decodedPict = deciV->getDecodedPictdimensions();
            if (decodedPict!=NULL) {
                printprefs.xinput = decodedPict->dx;
                printprefs.yinput = decodedPict->dy;
            }

            // Because closed captions are delivered at the same time of the video frame, like OSD, threading does not help.
            printprefs.fontSettings.gdi_font_scale = 4;
            fontCC.print(cc,false,printprefs,dst,stride2);
            wasCCchange=false;
        }
    }

    if (adhocMode != ADHOC_ADHOC_DRAW_DVD_SUB_ONLY 
       && everRGB
       && (pict.csp & FF_CSPS_MASK) != FF_CSP_RGB32 
       && rgb32_if_possible) {
        unsigned char *dst[4];
        getCurNext3(FF_CSP_RGB32, pict, cfg->full, COPYMODE_DEF, dst);
    }

    if (adhocMode == ADHOC_ADHOC_DRAW_DVD_SUB_ONLY) {
      adhocMode = ADHOC_SECOND_DONT_DRAW_DVD_SUB;
      if (!again)
          return S_OK;
      else
          return parent->deliverSample(it,pict);
    } else {
        adhocMode = ADHOC_NORMAL;
    }

    if (parent->getStopAtSubtitles()) {
        // We just wanted to update prevPict.
        parent->setStopAtSubtitles(false);
        return S_OK;
    }

    return parent->deliverSample(++it,pict);
}

TsubtitlesTextpin* TimgFilterSubtitles::getTextpin()
{
    // make sure csEmbedded is locked
    int shownEmbedded=deci->getParam2(IDFF_subShowEmbedded);
    if (embedded.size() && shownEmbedded) {
        Tembedded::iterator e=embedded.find(shownEmbedded);
        if (e!=embedded.end() && e->second)
            return e->second;
    }
    return NULL;
}

void TimgFilterSubtitles::onSeek()
{
    wasDiscontinuity=true;
    again=false;
    {
        boost::unique_lock<boost::recursive_mutex> lock(csCC);
        hideClosedCaptions();
    }
    {
        boost::unique_lock<boost::recursive_mutex> lock(csEmbedded);
        TsubtitlesTextpin* pin = getTextpin();
        if (pin)
            pin->onSeek();
        glyphThread.onSeek();
    }
}

const char_t* TimgFilterSubtitles::getCurrentFlnm() const
{
    return subs.subFlnm;
}

void TimgFilterSubtitles::addClosedCaption(const wchar_t *line)
{
    boost::unique_lock<boost::recursive_mutex> lock(csCC);
    if (!cc)
        cc=new TsubtitleText(Tsubreader::SUB_SUBRIP);
    cc->add(line);
    TsubtitleFormat format(NULL);
    cc->format(format);
    wasCCchange=true;
}
void TimgFilterSubtitles::hideClosedCaptions()
{
    boost::unique_lock<boost::recursive_mutex> lock(csCC);
    if (cc) {
        cc->clear();
        wasCCchange=true;
    }
}

bool TimgFilterSubtitles::enterAdhocMode()
{
    if (adhocMode == ADHOC_NORMAL)
        adhocMode = ADHOC_ADHOC_DRAW_DVD_SUB_ONLY;
    return !again;
}

HANDLE TimgFilterSubtitles::getGlyphThreadHandle()
{
    return glyphThread.get_platform_specific_thread();
}

// ========================= TimgFilterSubtitles::TglyphThread =========================
TimgFilterSubtitles::TglyphThread::TglyphThread(TimgFilterSubtitles *Iparent, IffdshowBase *deci):
    parent(Iparent),
    threadCmd(1),
    current_pos(0),
    oldpin(NULL),
    font(deci),
    firstrun(true),
    used_memory(0),
    mutex_prefs(), // initialize before starting a thread
    condv_prefs(),
    platform_specific_thread(NULL),
    thread(glyphThreadFunc0,this)
{
    shared_prefs.csp = -1;
}

void TimgFilterSubtitles::TglyphThread::glyphThreadFunc()
{
    slow();
    SetThreadPriorityBoost(get_platform_specific_thread(), true);
    TsubtitleText *next = NULL;
    do {
        // DPRINTF(_l("glyphThreadFunc top level loop current_pos=%d used_memory=%d"),current_pos,used_memory);
        {
            // do not hog mutex too long
            TthreadPriority pr(get_platform_specific_thread(),
                THREAD_PRIORITY_ABOVE_NORMAL,
                THREAD_PRIORITY_BELOW_NORMAL);

            boost::unique_lock<boost::mutex> lock(mutex_prefs);
            if (threadCmd == 0) return;
            if (firstrun
              || (shared_prefs == copied_prefs
                  && (next == NULL || used_memory > max_memory_usage))) {
                condv_prefs.wait(lock);
                if (threadCmd == 0) return;
            }

            // compare again as shared_prefs may have chaged during condv_prefs.wait
            if (shared_prefs != copied_prefs) {
                // in this case, rendered subtitles are dropped by TimgFilterSubtitles::process. This is not beautiful, but better for performance.
                // DPRINTF(_l("prefs changed"));
                current_pos = 0;
                used_memory = 0;
            }
            firstrun = false;
            copied_prefs = shared_prefs;
        }

        {
            deferred_lock<boost::mutex> lock_next;
            {
                // do not hog mutex too long
                TthreadPriority pr(get_platform_specific_thread(),
                    THREAD_PRIORITY_ABOVE_NORMAL,
                    THREAD_PRIORITY_BELOW_NORMAL);

                boost::unique_lock<boost::recursive_mutex> lock_emb(parent->csEmbedded);
                next = getNext();
                if (used_memory > max_memory_usage)
                    clean_past();
                if (next) {
                    // lock next before unlocking csEmbedded.
                    lock_next.lock(next->get_lock_ptr());
                }
            }

            if (next && used_memory < max_memory_usage) {
                // make sure next is locked here and unlock before leaving.
                // csEmbedded is not locked here for performance.
                // DPRINTF(_l("glyphThreadFunc next %I64i"),next->start);
                used_memory += next->prepareGlyph(copied_prefs,font,false);
                current_pos++;
            }
        }
    } while(1);
}

void TimgFilterSubtitles::TglyphThread::done()
{
    {
        boost::unique_lock<boost::mutex> lock(mutex_prefs);
        threadCmd = 0;
    }
    condv_prefs.notify_one();
    thread.join();
}

Tsubreader* TimgFilterSubtitles::TglyphThread::get_subreader()
{
     // make sure csEmbedded is locked
    Tsubtitles* pin = parent->getTextpin();
    if (!pin)
        pin = &parent->subs;
    if (!pin) {
        current_pos = 0;
        return NULL;
    }
    if (!pin->isText()) return NULL;
    if (pin != oldpin) {
        current_pos = 0;
        oldpin = pin;
    }
    return pin->subs;
}

TsubtitleText* TimgFilterSubtitles::TglyphThread::getNext()
{
    // make sure csEmbedded is locked
    Tsubreader *subs = get_subreader();
    if (!subs) return NULL;
    if (current_pos >= subs->size()) return NULL;

    for (Tsubreader::const_iterator i = subs->begin() + current_pos ; i != subs->end() ; i++)
    {
        TsubtitleText *subText = (TsubtitleText *)*i;
        if (subText->stop < copied_prefs.rtStart){
            current_pos++;
            used_memory -= subText->dropRenderedLines();
        }
        if (!subText->is_rendering_ready()){
            return subText;
        }
    }
    return NULL;
}

void TimgFilterSubtitles::TglyphThread::clean_past()
{
    // make sure csEmbedded is locked
    Tsubreader *subs = get_subreader();
    if (!subs) return;

    foreach (Tsubtitle *sub ,*subs)
    {
        if (sub->stop < copied_prefs.rtStart){
            used_memory -= sub->dropRenderedLines();
        }
    }
}

void TimgFilterSubtitles::TglyphThread::onSeek()
{
    // make sure csEmbedded is locked
    current_pos = 0;
    used_memory = 0;
}

void TimgFilterSubtitles::TglyphThread::slow()
{
    SetThreadPriority(get_platform_specific_thread(), THREAD_PRIORITY_BELOW_NORMAL);
}

void TimgFilterSubtitles::TglyphThread::hustle()
{
    SetThreadPriority(get_platform_specific_thread(), THREAD_PRIORITY_ABOVE_NORMAL);
}

HANDLE TimgFilterSubtitles::TglyphThread::get_platform_specific_thread()
{
    if (!platform_specific_thread)
        platform_specific_thread = thread.native_handle();
    return platform_specific_thread;
}
