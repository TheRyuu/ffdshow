/*
 * Copyright (c) 2004-2010 Damien Bain-Thouverez
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
#include "IffdshowBase.h"
#include "IffdshowDecVideo.h"
#include "TsubtitlesTextpinPGS.h"
#include "TsubtitlePGS.h"
#include "Tsubreader.h"

TsubtitlesTextpinPGS::TsubtitlesTextpinPGS(int Itype, IffdshowBase *Ideci):
    TsubtitlesTextpin(Itype, Ideci)
{
    sub_format = Tsubreader::SUB_PGS;
    subs = new Tsubreader;
    pSubtitlePGSParser = new TsubtitlePGSParser(Ideci);
}

TsubtitlesTextpinPGS::~TsubtitlesTextpinPGS(void)
{
    if (pSubtitlePGSParser) {
        delete pSubtitlePGSParser;
    }
    /*for (TcompositionObjects::iterator c=m_compositionObjects.begin();c!=m_compositionObjects.end();)
    {
     delete *c;
     c=m_compositionObjects.erase(c);
    }*/
}


void TsubtitlesTextpinPGS::resetSubtitles(void)
{
    pSubtitlePGSParser->reset();
    subs->clear();
    TsubtitlesTextpin::resetSubtitles();
}

bool TsubtitlesTextpinPGS::ctlSubtitles(unsigned int id, const void *data, unsigned int datalen)
{
    return true;
}

void TsubtitlesTextpinPGS::addSubtitle(REFERENCE_TIME start, REFERENCE_TIME stop, const unsigned char *data, unsigned int datalen, const TsubtitlesSettings *cfg, bool utf8)
{
    /* Code to remove overlapping subtitles : PGS can have simultaneous subs so do not uncomment
    for (Tsubreader::reverse_iterator s=subs->rbegin();s!=subs->rend();s++)
     if ((*s)->stop==_I64_MAX && (*s)->start<start)
     {
      (*s)->stop=start;
      break;
     }*/
    pSubtitlePGSParser->parse(start, stop, data, datalen);
}




// Retrieve the list of subtitles to display for the giving start=>stop sequence
Tsubtitle* TsubtitlesTextpinPGS::getSubtitle(const TsubtitlesSettings *cfg, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop, bool *forceChange)
{
    m_compositionObjects.clear();
    pSubtitlePGSParser->getObjects(rtStart, rtStop, &m_compositionObjects);
    foreach(TcompositionObject * pCompositionObject, m_compositionObjects) {
        if (pCompositionObject->m_pSubtitlePGS == NULL) {
            for (int i = 0; i < pCompositionObject->m_nWindows; i++) {
                DPRINTF(_l("TsubtitlesTextpinPGS::addSubtitle Subtitles added"));
                TsubtitlePGS *sub = new TsubtitlePGS(deci, rtStart, rtStop, pCompositionObject, &pCompositionObject->m_Windows[i], this);
                subs->push_back((Tsubtitle*)sub);
            }
        }
    }
    // Clear passed subs
    for (Tsubreader::iterator s0 = subs->begin() ; s0 != subs->end() ;) {
        if ((*s0)->stop != INVALID_TIME && (*s0)->stop < rtStart) {
            delete *s0;
            s0 = subs->erase(s0);
        } else {
            s0++;
        }
    }

    // Rebuild the list
    subtitles.clear();
    REFERENCE_TIME start = _I64_MAX, stop = _I64_MIN;
    foreach(Tsubtitle * sub, *subs) {
        ((TsubtitlePGS*)sub)->updateTimestamps();
        if ((sub->stop == INVALID_TIME || rtStart <= sub->stop)
                && rtStop  >= sub->start) {
            subtitles.push_back(sub);
            if (sub->start < start) {
                start = sub->start;
            }
            if (sub->stop != INVALID_TIME && sub->stop  > stop) {
                stop = sub->stop ;
            }
        }
    }
    if (subtitles.empty()) {
        return NULL;
    }
    subtitles.start = start;
    subtitles.stop = stop;
    return &subtitles;
}

void TsubtitlesTextpinPGS::Tsubtitles::print(
    REFERENCE_TIME time,
    bool wasseek,
    Tfont &f,
    bool forceChange,
    TprintPrefs &prefs,
    unsigned char **dst,
    const stride_t *stride)
{
    for (const_iterator s = begin(); s != end(); s++) {
        if ((*s)->start <= time && (*s)->stop > time) {
            (*s)->print(time, wasseek, f, forceChange, prefs, dst, stride);
        }
    }
}

Tsubtitle* TsubtitlesTextpinPGS::Tsubtitles::copy(void)
{
    return new Tsubtitles(*this);
}
