/*
 * Copyright (c) 20010 Damien Bain-Thouverez
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
#include "Tsubreader.h"
#include "TsubtitlesSettings.h"
#include "Tconfig.h"
#include "TsubreaderPGS.h"

TsubreaderPGS::~TsubreaderPGS()
{
    SAFE_DELETE(pSubtitlePGSParser);
}

TsubreaderPGS::TsubreaderPGS(IffdshowBase *Ideci, Tstream &Ifd, double Ifps, const TsubtitlesSettings *Icfg, const Tconfig *Iffcfg):
    deci(Ideci),
    ffcfg(Iffcfg),
    pStream(&Ifd)
{
    cfg = *Icfg;
    pSubtitlePGSParser = new TsubtitlePGSParser(deci);
    DPRINTF(_l("TsubreaderPGS constructor"));
}

void TsubreaderPGS::onSeek(void)
{
    clear();
    rtPos = INVALID_TIME;
    pStream->rewind();
    if (pSubtitlePGSParser != NULL) {
        pSubtitlePGSParser->reset();
    }
}

void TsubreaderPGS::getSubtitle(const TsubtitlesSettings *cfg, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop, bool *forceChange)
{
    parse(0, rtStart, rtStop);
}

// Blu-Ray subtitles files parser
void TsubreaderPGS::parse(int flags, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop)
{

    // Parse 10 seconds before subtitles occur
    REFERENCE_TIME parseRtStart = rtStart - 10000 * 1000 * 10;
    REFERENCE_TIME parseRtStop = rtStop + 10000 * 1000 * 10;

    REFERENCE_TIME segStart = INVALID_TIME, segStop = INVALID_TIME;
    if (rtPos == INVALID_TIME) {
        rtPos = parseRtStart;
    }

    REFERENCE_TIME oldRtPos = rtPos;

    bool isSeeking = true;
    do {
        if (!pStream->read(data, 1, 2)) {
            break;
        }
        if (data[0] != 80 || data[1] != 0x47) {
            DPRINTF(_l("TsubreaderPGS::parse wrong format"));
            break;
        }
        if (!pStream->read(data, 1, 4)) {
            break;
        }
        segStart = (data[3] + ((int64_t)data[2] << 8) + ((int64_t)data[1] << 0x10) + ((int64_t)data[0] << 0x18)) / 90;
        if (!pStream->read(data, 1, 4)) {
            break;
        }
        segStop = (data[3] + ((int64_t)data[2] << 8) + ((int64_t)data[1] << 0x10) + ((int64_t)data[0] << 0x18)) / 90;

        // Times are in ms, convert them to REFENCE_TIME unit (100ns)
        segStart *= 10000;
        segStop *= 10000;

        if (segStop == 0) {
            segStop = segStart;
        }

        pStream->read(data, 1, 3); // Segment type (1 byte) and segment length (2 bytes)
        size_t datalen = data[2] + (((uint32_t)data[1]) << 8);

        // Already parsed subtitles, jump to next segment
        if (isSeeking && (rtPos >= segStop)) {
            if (!pStream->seek((long)datalen, SEEK_CUR)) {
                break;
            }
            continue;
        }
        isSeeking = false;

        // Subtitles after given range, stop here and return the current list
        if (segStop > parseRtStop) {
            pStream->seek(-13, SEEK_CUR);
            break;
        }

        rtPos = segStop;

#if DEBUG_PGS
        char_t rtString[25];
        rt2Str(segStart, rtString);
        DPRINTF(_l("TsubreaderPGS::parse %s Segment type %X"), rtString, data[0]);
#endif

        pStream->read(&data[3], 1, datalen); // Read the segment

        if (pSubtitlePGSParser->parse(segStart, segStop, data, datalen + 3) != S_OK) {
            DPRINTF(_l("TsubreaderPGS::parse error during parsing"));
            break;
        }
    } while (1);

    TcompositionObjects compositionObjects;
    pSubtitlePGSParser->getObjects(rtStart, rtStop + 10000 * 1000 * 10, &compositionObjects);

    foreach(Tsubtitle * pSubtitle, (*this)) {
        ((TsubtitlePGS*)pSubtitle)->updateTimestamps();
    }

    foreach(TcompositionObject * pCompositionObject, compositionObjects) {
        if (pCompositionObject->m_pSubtitlePGS == NULL) {
            for (int i = 0; i < pCompositionObject->m_nWindows; i++) {
#if DEBUG_PGS
                char_t rtString[32], rtString2[32];
                rt2Str(pCompositionObject->m_Windows[i].m_rtStart, rtString);
                rt2Str(pCompositionObject->m_Windows[i].m_rtStop, rtString2);
                DPRINTF(_l("[%d] TsubreaderPGS::parse WindowId %d Subtitles added %s --> %s %s"), pCompositionObject->m_compositionNumber,
                        pCompositionObject->m_Windows[i].m_windowId, rtString, rtString2, (pCompositionObject->m_Windows[i].data[0].size() > 0) ? _l("has data") : _l("no data"));
#endif
                TsubtitlePGS *sub = new TsubtitlePGS(deci, rtStart, rtStop, pCompositionObject, &pCompositionObject->m_Windows[i], this);
                push_back((Tsubtitle*)sub);
            }
        }
    }
}
