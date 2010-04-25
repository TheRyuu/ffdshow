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

TsubreaderPGS::TsubreaderPGS(IffdshowBase *Ideci, Tstream &Ifd, double Ifps,const TsubtitlesSettings *Icfg,const Tconfig *Iffcfg):
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
 pStream->rewind();
 if (pSubtitlePGSParser!=NULL) pSubtitlePGSParser->reset();
}

void TsubreaderPGS::getSubtitle(const TsubtitlesSettings *cfg, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop, bool *forceChange)
{
 // Parse 10 seconds before subtitles occur
 //rtStart+=10000*1000*10;
 rtStop+=10000*1000*10; 
 parse(0, rtStart, rtStop);
}

// Blu-Ray subtitles files parser
void TsubreaderPGS::parse(int flags, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop) {

 REFERENCE_TIME segStart=INVALID_TIME, segStop=INVALID_TIME;
 do {
  if (!pStream->read(data,1,2)) break;
  if (data[0] != 80 || data[1] != 0x47) { DPRINTF(_l("TsubreaderPGS::parse wrong format"));break;}
  if (!pStream->read(data,1,4)) break;
  segStart=(data[3] + ((int64_t)data[2] << 8) + ((int64_t)data[1] << 0x10) + ((int64_t)data[0] << 0x18))/90;
  if (!pStream->read(data,1,4)) break;
  segStop=(data[3] + ((int64_t)data[2] << 8) + ((int64_t)data[1] << 0x10) + ((int64_t)data[0] << 0x18))/90;

  // Times are in ms, convert them to REFENCE_TIME unit (100ns)
  segStart *= 10000;
  segStop *= 10000;

  if (segStop == 0) segStop = segStart;

  pStream->read(data,1,3); // Segment type (1 byte) and segment length (2 bytes)
  size_t datalen = data[2] + (((uint32_t)data[1]) << 8);

  // Passed subtitles, jump to next segment
  if (rtStart != INVALID_TIME && rtStart > segStop) 
  {
   if (!pStream->seek(datalen, SEEK_CUR)) break;
   continue;
  }

  // Subtitles after given range, stop here and return the current list
  if (!((rtStart == INVALID_TIME || rtStart < segStop) 
   && (rtStop == INVALID_TIME || rtStop >= segStart)))
  {
   pStream->seek(-13, SEEK_CUR);
   break;
  }
#if DEBUG_PGS_TIMESTAMPS
  char_t rtString[25];
  rt2Str(segStart, rtString);
  DPRINTF(_l("TsubreaderPGS::parse %s Segment type %X"),rtString, data[0]);
#endif

  pStream->read(&data[3],1,datalen); // Read the segment

  if (pSubtitlePGSParser->parse(segStart, segStop, data, datalen+3) != S_OK)
  {
   DPRINTF(_l("TsubreaderPGS::parse error during parsing"));
   break;
  }
 } while(1);

 TcompositionObjects compositionObjects;
 pSubtitlePGSParser->getObjects(rtStart, rtStop, &compositionObjects); 
 foreach (TcompositionObject *pCompositionObject, compositionObjects) {
  if (pCompositionObject->m_pSubtitlePGS == NULL) {
   for (int i=0;i<pCompositionObject->m_nWindows; i++) {
#if DEBUG_PGS_TIMESTAMPS
    DPRINTF(_l("TsubreaderPGS::parse Subtitles added"));
#endif
    TsubtitlePGS *sub=new TsubtitlePGS(deci, rtStart, rtStop, pCompositionObject, &pCompositionObject->m_Windows[i], this);
    push_back((Tsubtitle*)sub);
   }
  }
 }
}
