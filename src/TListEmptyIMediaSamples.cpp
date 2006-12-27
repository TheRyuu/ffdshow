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

#include "stdafx.h"
#include "ffdebug.h"
#include "TListEmptyIMediaSamples.h"
#include "TffdshowDecVideoOutputPin.h"

ListEmptyIMediaSamples::ListEmptyIMediaSamples(TffdshowDecVideoOutputPin *pOut, int count):stopping(false),flashing(false),empty(true),firstRun(true)
{
 maxSamples=count;
 m_pOutputDecVideo=pOut;
 buf = new std::list<IMediaSample *>;
 hPopEvent= CreateEvent(NULL, false, false, NULL);
}

ListEmptyIMediaSamples::~ListEmptyIMediaSamples()
{
  //DPRINTF(_l("ListEmptyIMediaSamples destructor"));
  if(buf)
   {
    freeAll();
    delete buf;
   }
  if(hPopEvent) CloseHandle(hPopEvent);
}

void ListEmptyIMediaSamples::addAll(void)
{
 //DPRINTF(_l("ListEmptyIMediaSamples::addAll"));
 if(!buf) return;
 if(flashing) return;
  for(int i=buf->size();i<maxSamples;i++)
  {
   addOne();
  }
}

void ListEmptyIMediaSamples::addOne(void)
{
 if(!buf) return;
 if(flashing) return;

 IMediaSample *pOutSample;
 //DPRINTF(_l("ListEmptyIMediaSamples::addOne about to call GetDeliveryBuffer"));
 HRESULT hr=m_pOutputDecVideo->GetDeliveryBuffer(&pOutSample, NULL, NULL, 0);
 //DPRINTF(_l("ListEmptyIMediaSamples::addOne GetDeliveryBuffer returned %x"),hr);
 if(SUCCEEDED(hr))
  {
   CAutoLock lck(this);
   buf->push_back(pOutSample);
   SetEvent(hPopEvent);
   firstRun=false;
   //DPRINTF(_l("added one"));
  }
}

IMediaSample* ListEmptyIMediaSamples::GetBuffer(void)
{
 if(!buf) return NULL;
 if(stopping) return NULL;
 if(flashing) return NULL;
 CAutoLock lck(this);
 if(empty)
  {
   addAll();
   empty=false;
  }
 if(buf->empty())
  {
   ResetEvent(hPopEvent);
   Unlock();
   //DPRINTF(_l("ListEmptyIMediaSamples::GetBuffer wainting for hPopEvent"));
   WaitForSingleObject(hPopEvent, INFINITE);
   //DPRINTF(_l("ListEmptyIMediaSamples::GetBuffer wainting over"));
   Lock();
  }
 if(buf->empty())
  {
   //DPRINTF(_l("ListEmptyIMediaSamples::GetBuffer return NULL"));
   return NULL;
  }
 IMediaSample *sample= buf->front();
 buf->pop_front();
 return sample;
}

void ListEmptyIMediaSamples::freeAll(void)
{
 //DPRINTF(_l("ListEmptyIMediaSamples::freeAll"));
 if(!buf) return;
 CAutoLock lck(this);
 SetEvent(hPopEvent);

 while(buf->size() != 0)
  {
   IMediaSample *sample=buf->back();
   buf->pop_back();
   sample->Release();
  }
 ResetEvent(hPopEvent);
 empty=true;
}

void ListEmptyIMediaSamples::BeginStop(void)
{
 //DPRINTF(_l("ListEmptyIMediaSamples::BeginStop"));
 CAutoLock lck(this);
 stopping=true;
 if(buf->size() == 0)
  {
   //DPRINTF(_l("ListEmptyIMediaSamples::BeginStop 1"));
   SetEvent(hPopEvent);
  }
}

void ListEmptyIMediaSamples::EndStop(void)
{
 //DPRINTF(_l("ListEmptyIMediaSamples::EndStop"));
 CAutoLock lck(this);
 ResetEvent(hPopEvent);
 stopping=false;
}

void ListEmptyIMediaSamples::BeginFlush(void)
{
 //DPRINTF(_l("ListEmptyIMediaSamples::BeginFlush"));
 CAutoLock lck(this);
 flashing=true;
 SetEvent(hPopEvent);
}

void ListEmptyIMediaSamples::EndFlush(void)
{
 //DPRINTF(_l("ListEmptyIMediaSamples::EndFlush"));
 CAutoLock lck(this);
 freeAll();
 flashing=false;
}

void ListEmptyIMediaSamples::NewSegment(void)
{
 //DPRINTF(_l("ListEmptyIMediaSamples::NewSegment"));
 CAutoLock lck(this);
 if(!firstRun)
  addAll();
}
