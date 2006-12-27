/*
 * Copyright (c) 2006 h.yamagata
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

/*
 * ListEmptyIMediaSamples is a helper class of queue and does prefetch.
 * GetBuffer and Receive is now called from the same thread.
 * If Receive and GetBuffer is called from other threads,
 * GetBuffer often returns error.
 * By cooperating with TffOutputQ,
 * this class buffers the IMediaSample as soon as it is released in TffOutputQ::ThreadProc.
 */

#ifndef _TLISTEMPTYIMEDIASAMPLES_H_
#define _TLISTEMPTYIMEDIASAMPLES_H_

class TffdshowDecVideoOutputPin;
class ListEmptyIMediaSamples : public CCritSec
{
protected:
 std::list<IMediaSample *> *buf;
 int maxSamples;
 TffdshowDecVideoOutputPin *m_pOutputDecVideo;
 HANDLE hPopEvent;
 bool stopping,flashing;
 bool empty,firstRun;
public:
 ListEmptyIMediaSamples(TffdshowDecVideoOutputPin *pOut, int count);
 ~ListEmptyIMediaSamples();
 void addAll(void);
 void addOne(void);
 IMediaSample* GetBuffer(void);
 void freeAll(void);
 void BeginStop(void);
 void EndStop(void);
 void BeginFlush(void);
 void EndFlush(void);
 void NewSegment(void);
};

#endif
