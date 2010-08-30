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
#include "TffdshowDec.h"
#include "TdialogSettings.h"
#include "TglobalSettings.h"
#include "Tpresets.h"
#include "TpresetSettings.h"
#include "Ttranslate.h"
#include "TkeyboardDirect.h"
#include "ffdshowRemoteAPIimpl.h"
#include "Tfilters.h"
#include "IffdshowDecVideo.h"
#include "dsutil.h"
#include "TsubtitlesFile.h"

STDMETHODIMP_(int) TffdshowDec::getVersion2(void)
{
 return VERSION;
}

template<> interfaces<char_t>::IffdshowDec* TffdshowDec::getDecInterface(void)
{
 return this;
}
template<> interfaces<tchar_traits<char_t>::other_char_t>::IffdshowDec* TffdshowDec::getDecInterface(void)
{
 return &dec_char;
}

STDMETHODIMP TffdshowDec::NonDelegatingQueryInterface(REFIID riid,void **ppv)
{
 if (globalSettings->isBlacklist && (riid==IID_IBaseFilter || riid==IID_IMediaFilter) && globalSettings->inBlacklist(getExeflnm()))
  return E_NOINTERFACE;
 if (globalSettings->isWhitelist && (riid==IID_IBaseFilter || riid==IID_IMediaFilter) && !globalSettings->inWhitelist(getExeflnm(),this))
  return E_NOINTERFACE;
 if (riid==IID_IffdshowBaseA)
  return GetInterface<IffdshowBaseA>(getBaseInterface<IffdshowBaseA>(),ppv);
 if (riid==IID_IffdshowBaseW)
  return GetInterface<IffdshowBaseW>(getBaseInterface<IffdshowBaseW>(),ppv);
 else if (riid==IID_IffdshowDecA)
  return GetInterface<IffdshowDecA>(getDecInterface<IffdshowDecA>(),ppv);
 else if (riid==IID_IffdshowDecW)
  return GetInterface<IffdshowDecW>(getDecInterface<IffdshowDecW>(),ppv);
 else if (riid==IID_ISpecifyPropertyPages)
  return GetInterface<ISpecifyPropertyPages>(this,ppv);
 else if (riid==IID_IAMStreamSelect)
  return GetInterface<IAMStreamSelect>(this,ppv);
 else if (riid==IID_IffdshowParamsEnum)
  return GetInterface<IffdshowParamsEnum>(this,ppv);
 else
  return CTransformFilter::NonDelegatingQueryInterface(riid,ppv);
}

TffdshowDec::TffdshowDec(TintStrColl *Ioptions,const TCHAR *name,LPUNKNOWN punk,REFCLSID clsid,TglobalSettingsDec *IglobalSettings,TdialogSettingsDec *IdialogSettings,Tpresets *Ipresets,Tpreset* &IpresetSettings,CmyTransformFilter *Imfilter,TinputPin* &Iminput,CTransformOutputPin* &Imoutput,IFilterGraph* &Igraph,Tfilters* &Ifilters,const CLSID &Iproppageid,int IcfgDlgCaptionId,int IiconId,DWORD IdefaultMerit):
 presets(Ipresets),
 presetSettings(IpresetSettings),
 filters(Ifilters),
 CmyTransformFilter(name,punk,clsid),
 TffdshowBase(
  punk,
  Ioptions,
  globalSettings=IglobalSettings,
  dialogSettings=IdialogSettings,
  Imfilter,
  Iminput,
  Imoutput,
  Igraph,
  IcfgDlgCaptionId,IiconId,
  IdefaultMerit),
 proppageid(Iproppageid),
 dec_char(punk,this),
 m_dirtyStop(false)
{
 static const TintOptionT<TffdshowDec> iopts[]=
  {
   IDFF_movieDuration ,&TffdshowDec::moviesecs,-1,-1,_l(""),0,NULL,0,
   0
  };
 addOptions(iopts);

 keys=NULL;mouse=NULL;
 remote=NULL;
 hereSeek=false;
 presetSettings=NULL;
 firsttransform=true;discontinuity=true;
 filters=NULL;
 onNewFiltersWnd=NULL;onNewFiltersMsg=0;
}
TffdshowDec::~TffdshowDec()
{
 if (presets) delete presets;
 if (keys) delete keys;
 if (mouse) delete mouse;
 if (remote) delete remote;
}

STDMETHODIMP TffdshowDec::getGraph(IFilterGraph* *graphPtr)
{
 return TffdshowBase::getGraph(graphPtr);
}
STDMETHODIMP_(int) TffdshowDec::getState2(void)
{
 return TffdshowBase::getState2();
}

HRESULT TffdshowDec::onInitDialog(void)
{
 initPresets();
 if (!presetSettings) setActivePreset(globalSettings->defaultPreset,false);
 return TffdshowBase::onInitDialog();
}

//tell and seek
STDMETHODIMP TffdshowDec::tell(int *seconds)
{
 return TffdshowBase::tell(seconds);
}
STDMETHODIMP TffdshowDec::seek(int seconds)
{
 return TffdshowBase::seek(seconds);
}
STDMETHODIMP TffdshowDec::stop(void)
{
 return TffdshowBase::stop();
}
STDMETHODIMP_(int) TffdshowDec::getCurTime2(void)
{
 return TffdshowBase::getCurTime2();
}
STDMETHODIMP TffdshowDec::run(void)
{
 return TffdshowBase::run();
}
STDMETHODIMP TffdshowDec::getDuration(int *duration)
{
 return E_NOTIMPL;
}

STDMETHODIMP TffdshowDec::initKeys(void)
{
 if (!keys) keys=new Tkeyboard(options,this);
 return S_OK;
}
STDMETHODIMP TffdshowDec::saveKeysSettings(void)
{
 if (!keys) return S_FALSE;
 keys->save();
 return S_OK;
}
STDMETHODIMP TffdshowDec::loadKeysSettings(void)
{
 if (!keys) return S_FALSE;
 keys->load();
 return S_OK;
}
STDMETHODIMP_(int) TffdshowDec::getKeyParamCount2(void)
{
 return keys?(int)keys->keysParams.size():0;
}
STDMETHODIMP TffdshowDec::getKeyParamDescr(unsigned int i,const char_t **descr)
{
 if (!keys || i>=keys->keysParams.size())
  {
   *descr=_l("");
   return S_FALSE;
  };
 *descr=keys->keysParams[i].descr;
 return S_OK;
}
STDMETHODIMP_(int) TffdshowDec::getKeyParamKey2(unsigned int i)
{
 if (!keys || i>=keys->keysParams.size()) return 0;
 return keys->keysParams[i].key;
}
STDMETHODIMP TffdshowDec::setKeyParamKey(unsigned int i,int key)
{
 if (!keys || i>=keys->keysParams.size()) return S_FALSE;
 keys->keysParams[i].key=key;
 sendOnChange(IDFF_keysSeek1/*just not to send 0*/,key);
 return S_OK;
}
STDMETHODIMP TffdshowDec::setKeyParamKeyCheck(unsigned int i,int key,int *prev,const char_t* *prevDescr)
{
 if (key!=0 && (prev || prevDescr))
  for (unsigned int previ=0;previ<keys->keysParams.size();previ++)
   if (previ!=i && keys->keysParams[previ].key==key)
    {
     if (prev) *prev=previ;
     if (prevDescr) *prevDescr=keys->keysParams[previ].descr;
     return E_INVALIDARG;
    }
 return setKeyParamKey(i,key);
}

STDMETHODIMP TffdshowDec::resetKeys(void)
{
 if (!keys) return S_FALSE;
 keys->reset();
 return S_OK;
}
STDMETHODIMP TffdshowDec::exportKeysToGML(const char_t *flnm)
{
 return keys->exportToGML(flnm)?S_OK:E_FAIL;
}

STDMETHODIMP TffdshowDec::initMouse(void)
{
 if (!mouse) mouse=new Tmouse(options,this);
 return S_OK;
}

STDMETHODIMP TffdshowDec::initRemote(void)
{
 if (!remote) remote=new Tremote(options,this);
 return S_OK;
}
STDMETHODIMP TffdshowDec::saveRemoteSettings(void)
{
 if (!remote) return S_FALSE;
 remote->save();
 return S_OK;
}
STDMETHODIMP TffdshowDec::loadRemoteSettings(void)
{
 if (!remote) return S_FALSE;
 remote->load();
 return S_OK;
}

void TffdshowDec::onTrayIconChange(int id,int newval)
{
 if (!firsttransform && (globalSettings->filtermode&IDFF_FILTERMODE_PROC)==0)
  TffdshowBase::onTrayIconChange(id,newval);
}

STDMETHODIMP TffdshowDec::getNumPresets(unsigned int *value)
{
 if (!value) return E_POINTER;
 *value=(unsigned int)presets->size();
 return S_OK;
}
STDMETHODIMP TffdshowDec::getPresetName(unsigned int i,char_t *buf,size_t len)
{
 if (!buf) return E_POINTER;
 if (i>=presets->size()) return S_FALSE;
 if (len<strlen((*presets)[i]->presetName)+1) return E_OUTOFMEMORY;
 strcpy(buf,(*presets)[i]->presetName);
 return S_OK;
}
STDMETHODIMP TffdshowDec::getActivePresetName(char_t *buf,size_t len)
{
 if (!buf) return E_POINTER;
 if (!presetSettings) return S_FALSE;
 if (len<strlen(presetSettings->presetName)+1) return E_OUTOFMEMORY;
 strcpy(buf,presetSettings->presetName);
 return S_OK;
}
STDMETHODIMP_(const char_t*) TffdshowDec::getActivePresetName2(void)
{
 return presetSettings?presetSettings->presetName:NULL;
}

STDMETHODIMP TffdshowDec::setActivePreset(const char_t *name,int create)
{
 if (!name) return S_FALSE;
 Tpreset *preset=presets->getPreset(name,!!create);
 if (preset)
  {
   setPresetPtr(preset);
   return S_OK;
  }
 else
  return S_FALSE;
}
STDMETHODIMP TffdshowDec::saveActivePreset(const char_t *name)
{
 if (!presetSettings) return E_INVALIDARG;
 presets->savePreset(presetSettings,name);
 return S_OK;
}
STDMETHODIMP TffdshowDec::saveActivePresetToFile(const char_t *flnm)
{
 if (!flnm || !presetSettings) return E_INVALIDARG;
 return presets->savePresetFile(presetSettings,flnm)?S_OK:E_FAIL;
}
STDMETHODIMP TffdshowDec::loadActivePresetFromFile(const char_t *flnm)
{
 if (!flnm) return E_INVALIDARG;
 if (!presetSettings)
  presetSettings=presets->newPreset();
 if (presetSettings->loadFile(flnm))
  {
   presets->storePreset(presetSettings);
   notifyParamsChanged();
   return S_OK;
  }
 else return E_FAIL;
}
STDMETHODIMP TffdshowDec::savePresetMem(void *buf,size_t len)
{
 if (!presetSettings) return E_UNEXPECTED;
 if (!buf) return E_POINTER;
 TregOpIDstreamWrite t;
 presetSettings->reg_op(t);

 if (len==0)
  *(int*)buf=(int)t.size();
 else
  memcpy(buf,&*t.begin(),std::min(size_t(len),t.size()));
 return S_OK;
}
STDMETHODIMP TffdshowDec::loadPresetMem(const void *buf,size_t len)
{
 if (!presetSettings) return E_UNEXPECTED;
 TregOpIDstreamRead t(buf,len);
 presetSettings->reg_op(t);
 return 0;
}

STDMETHODIMP TffdshowDec::removePreset(const char_t *name)
{
 if (stricmp(presetSettings->presetName,globalSettings->defaultPreset)==0)
  putParamStr(IDFF_defaultPreset,(*presets)[0]->presetName);
 return presets->removePreset(name)?S_OK:S_FALSE;
}
STDMETHODIMP TffdshowDec::getPresets(Tpresets *presets2)
{
 if (!presets2) return E_INVALIDARG;
 presets2->done();
 for (Tpresets::iterator i=presets->begin();i!=presets->end();i++)
  presets2->push_back((*i)->copy());
 return S_OK;
}

STDMETHODIMP TffdshowDec::setPresets(const Tpresets *presets2)
{
    if (!presets2) return E_POINTER;

    // copy every thing preset in presets2 (src).
    foreach (const Tpreset* src ,*presets2) {
        bool copied;
        copied = false;
        foreach (Tpreset* dst, *presets) {
            if (strncmp(dst->presetName, src->presetName, countof(dst->presetName)) == 0) {
                *dst = *src;
                copied = true;
                break;
            }
        }
        if (!copied) {
            presets->push_back(src->copy());
        }
    }

    // remove from presets (dst) what does not exist in presets2 (src).
    for (Tpresets::iterator dst = presets->begin() ; dst != presets->end() ; ) {
        bool found = false;
        foreach (const Tpreset* src, *presets2) {
            if (strncmp((*dst)->presetName, src->presetName, countof((*dst)->presetName)) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            dst = presets->erase(dst);
        } else {
            dst++;
        }
    }
    return S_OK;
}

STDMETHODIMP TffdshowDec::savePresets(void)
{
 presets->saveRegAll();
 return S_OK;
}
STDMETHODIMP TffdshowDec::setPresetPtr(Tpreset *preset)
{
 if (!preset) return E_POINTER;
 if (presetSettings!=preset)
  {
   lock(IDFF_lockPresetPtr);
   presetSettings=preset;
   presetSettings->options->setSendOnChange(fastdelegate::MakeDelegate(this,&TffdshowDec::sendOnChange));
   notifyParamsChanged();
   sendOnChange(0,0);
   unlock(IDFF_lockPresetPtr);
  }
 return S_OK;
}
STDMETHODIMP TffdshowDec::getPresetPtr(Tpreset**preset)
{
 if (!preset) return E_POINTER;
 *preset=presetSettings;
 return S_OK;
}
void TffdshowDec::getColls(TintStrColls &colls)
{
 if (presetSettings)
  colls.push_back(presetSettings->options);
 TffdshowBase::getColls(colls);
}

STDMETHODIMP TffdshowDec::getParam(unsigned int paramID,int* value)
{
 if (!value) return S_FALSE;
 if (presetSettings && presetSettings->options->get(paramID,value))
  return S_OK;
 else
  return TffdshowBase::getParam(paramID,value);
}
STDMETHODIMP TffdshowDec::putParam(unsigned int paramID,int val)
{
 return presetSettings && presetSettings->options->set(paramID,val)?S_OK:TffdshowBase::putParam(paramID,val);
}
STDMETHODIMP TffdshowDec::invParam(unsigned int paramID)
{
 return presetSettings && presetSettings->options->inv(paramID)?S_OK:TffdshowBase::invParam(paramID);
}
STDMETHODIMP TffdshowDec::resetParam(unsigned int paramID)
{
 return presetSettings && presetSettings->options->reset(paramID)?S_OK:TffdshowBase::resetParam(paramID);
}

STDMETHODIMP TffdshowDec::getParamStr3(unsigned int paramID,const char_t* *value)
{
 if (!value) return S_FALSE;
 if (presetSettings && presetSettings->options->get(paramID,value))
  return S_OK;
 else
  return TffdshowBase::getParamStr3(paramID,value);
}
STDMETHODIMP TffdshowDec::putParamStr(unsigned int paramID,const char_t *buf)
{
 if (!buf) return S_FALSE;
 return presetSettings && presetSettings->options->set(paramID,buf)?S_OK:TffdshowBase::putParamStr(paramID,buf);
}
STDMETHODIMP TffdshowDec::getParamInfo(unsigned int paramID,TffdshowParamInfo *paramPtr)
{
 if (!paramPtr) return E_POINTER;
 return presetSettings && presetSettings->options->getInfo(paramID,paramPtr)?S_OK:TffdshowBase::getParamInfo(paramID,paramPtr);
}
STDMETHODIMP TffdshowDec::notifyParam(int id,int val)
{
 return presetSettings && presetSettings->options->notifyParam(id,val)?S_OK:TffdshowBase::notifyParam(id,val);
}
STDMETHODIMP TffdshowDec::notifyParamStr(int id,const char_t *val)
{
 return presetSettings && presetSettings->options->notifyParam(id,val)?S_OK:TffdshowBase::notifyParamStr(id,val);
}

STDMETHODIMP TffdshowDec::notifyParamsChanged(void)
{
 if (filters) filters->onQueueChange(0,0);
 return TffdshowBase::notifyParamsChanged();
}

STDMETHODIMP TffdshowDec::renameActivePreset(const char_t *newName)
{
 if (!newName) return E_POINTER;
 if (strlen(newName)>MAX_PATH) return E_OUTOFMEMORY;
 int res=stricmp(presetSettings->presetName,globalSettings->defaultPreset);
 strcpy(presetSettings->presetName,newName);
 if (res==0) putParamStr(IDFF_defaultPreset,newName);
 sendOnChange(0,0);
 return S_OK;
}
STDMETHODIMP TffdshowDec::isDefaultPreset(const char_t *presetName)
{
 return stricmp(globalSettings->defaultPreset,presetName)==0?1:0;
}
STDMETHODIMP TffdshowDec::initPresets(void)
{
 if (!presets->empty()) return E_UNEXPECTED;
 presets->init();
 return S_OK;
}

void TffdshowDec::initPreset(void)
{
 DPRINTF(_l("initPreset"));

 if (presets->empty()) initPresets();

 getSourceName();

 if (globalSettings->autoPreset)
  if (Tpreset *preset=presets->getAutoPreset(this,!!globalSettings->autoPresetFileFirst))
   {
    setPresetPtr(preset);
    return;
   }
 setActivePreset(globalSettings->defaultPreset,false);
}

STDMETHODIMP TffdshowDec::createTempPreset(const char_t *presetName)
{
 if (presets->empty()) return E_UNEXPECTED;
 Tpreset *preset=presets->newPreset(presetName);
 preset->autoLoadedFromFile=true;
 preset->loadReg();
 presets->push_back(preset);
 return S_OK;
}

STDMETHODIMP TffdshowDec::putStringParams(const char_t *params,char_t sep,int loaddef)
{
 if (!presetSettings) return E_UNEXPECTED;
 if (!params) return E_POINTER;
 TregOpStreamRead t(params,strlen(params),sep,!!loaddef);
 presetSettings->reg_op(t);
 notifyParamsChanged();
 return S_OK;
}

STDMETHODIMP TffdshowDec::resetFilter(unsigned int filterID)
{
 return resetFilterEx(filterID,0);
}
STDMETHODIMP TffdshowDec::resetFilterEx(unsigned int filterID,unsigned int filterPageId)
{
 if (!presetSettings) return E_UNEXPECTED;
 TfilterSettings *fs=presetSettings->getSettings(filterID);
 if (!fs) return E_INVALIDARG;
 return fs->reset(filterPageId)?S_OK:S_FALSE;
}
STDMETHODIMP TffdshowDec::filterHasReset(unsigned int filterID)
{
 return filterHasResetEx(filterID,0);
}
STDMETHODIMP TffdshowDec::filterHasResetEx(unsigned int filterID,unsigned int filterPageId)
{
 if (!presetSettings) return E_UNEXPECTED;
 TfilterSettings *fs=presetSettings->getSettings(filterID);
 if (!fs) return E_INVALIDARG;
 return fs->hasReset(filterPageId)?S_OK:S_FALSE;
}

STDMETHODIMP TffdshowDec::getFilterTip(unsigned int filterID,char_t *buf,size_t buflen)
{
 return getFilterTipEx(filterID,0,buf,buflen);
}
STDMETHODIMP TffdshowDec::getFilterTipEx(unsigned int filterID,unsigned int filterPageId,char_t *buf,size_t buflen)
{
 if (!presetSettings) return E_UNEXPECTED;
 if (!buf) return E_POINTER;
 TfilterSettings *fs=presetSettings->getSettings(filterID);
 if (!fs) return E_INVALIDARG;
 return fs->getTip(filterPageId,buf,buflen)?S_OK:S_FALSE;
}

STDMETHODIMP TffdshowDec::getPresetsPtr(Tpresets* *presetsPtr)
{
 if (!presetsPtr) return E_POINTER;
 *presetsPtr=presets;
 return S_OK;
}
STDMETHODIMP TffdshowDec::cyclePresets(int step)
{
 if (!presets || !presetSettings) return E_UNEXPECTED;
 Tpreset* preset=presets->getPreset(presetSettings->presetName,false);
 Tpresets::const_iterator pi=std::find(presets->begin(),presets->end(),preset);
 if (pi==presets->end()) return E_FAIL;
 for (int i=0;i!=step;i+=sign(step))
  {
   if (sign(step)==-1 && pi==presets->begin())
    pi=presets->end()-1;
   else if (sign(step)==1 && pi==presets->end()-1)
    pi=presets->begin();
   else
    pi+=sign(step);
  }
 setActivePreset((*pi)->presetName,false);
 return S_OK;
}

STDMETHODIMP TffdshowDec::newSample(IMediaSample* *samplePtr)
{
 return E_NOTIMPL;
}


STDMETHODIMP TffdshowDec::JoinFilterGraph(IFilterGraph *pGraph,LPCWSTR pName)
{
 return onJoinFilterGraph(pGraph,pName);
}

HRESULT TffdshowDec::BreakConnect(PIN_DIRECTION dir)
{
/*
 if (dir==PINDIR_OUTPUT && filters)
  filters->onDisconnect(dir);
*/
 return CTransformFilter::BreakConnect(dir);
}

HRESULT TffdshowDec::onGraphRemove(void)
{
 if (keys) delete keys;keys=NULL;
 if (mouse) delete mouse;mouse=NULL;
 if (remote) delete remote;remote=NULL;
 return TffdshowBase::onGraphRemove();
}

HRESULT TffdshowDec::StopStreaming(void)
{
 if (filters) filters->onStop();
 return CTransformFilter::StopStreaming();
}

STDMETHODIMP_(int) TffdshowDec::getMinOrder2(void)
{
 if (!presetSettings) return INT_MIN;
 return presetSettings->getMinOrder();
}
STDMETHODIMP_(int) TffdshowDec::getMaxOrder2(void)
{
 if (!presetSettings) return INT_MIN;
 return presetSettings->getMaxOrder();
}
STDMETHODIMP TffdshowDec::resetOrder(void)
{
 if (!presetSettings) return E_FAIL;
 if (presetSettings->resetOrder())
  sendOnChange(0,0);
 if (filters) filters->onQueueChange(0,0);
 return S_OK;
}
STDMETHODIMP TffdshowDec::setFilterOrder(unsigned int filterID,unsigned int newOrder)
{
 if (!presetSettings) return S_FALSE;
 return presetSettings->setFilterOrder(filterID,newOrder)?S_OK:S_FALSE;
}

STDMETHODIMP TffdshowDec::get_ExSeekCapabilities(long * pExCapabilities)
{
 *pExCapabilities=AM_EXSEEK_BUFFERING|AM_EXSEEK_NOSTANDARDREPAINT|AM_EXSEEK_CANSEEK|AM_EXSEEK_CANSCAN;//|AM_EXSEEK_SCANWITHOUTCLOCK;
 return S_OK;
}
STDMETHODIMP TffdshowDec::get_MarkerCount(long * pMarkerCount)
{
 *pMarkerCount=TffdshowBase::getDuration();
 return S_OK;
}
STDMETHODIMP TffdshowDec::get_CurrentMarker(long * pCurrentMarker)
{
 *pCurrentMarker=getCurTime2();
 return S_OK;
}
STDMETHODIMP TffdshowDec::GetMarkerTime(long MarkerNum, double * pMarkerTime)
{
 DPRINTF(_l("GetMarkerTime :%i"),MarkerNum);
 *pMarkerTime=MarkerNum;
 return S_OK;
}
STDMETHODIMP TffdshowDec::GetMarkerName(long MarkerNum, BSTR * pbstrMarkerName)
{
 return E_NOTIMPL;
}
STDMETHODIMP TffdshowDec::put_PlaybackSpeed(double Speed)
{
 if (comptrQ<IMediaSeeking> seek=graph)
  return seek->SetRate(Speed);
 else
  return E_NOINTERFACE;
}
STDMETHODIMP TffdshowDec::get_PlaybackSpeed(double * pSpeed)
{
 if (comptrQ<IMediaSeeking> seek=graph)
  return seek->GetRate(pSpeed);
 else
  return E_NOINTERFACE;
}

AM_MEDIA_TYPE* TffdshowDec::getInputMediaType(int lIndex)
{
 return CreateMediaType(&m_pInput->CurrentMediaType());
}

bool TffdshowDec::streamsSort(const Tstream *s1,const Tstream *s2)
{
 return s2->order>s1->order;
}

STDMETHODIMP TffdshowDec::Count(DWORD* pcStreams)
{
 if (!pcStreams) return E_POINTER;
 if (cfgDlgHwnd || !presetSettings)
 {
  *pcStreams=0;
  return S_OK;
 }
 *pcStreams=0;

 for (Tstreams::iterator s=streams.begin();s!=streams.end();s++) delete *s;streams.clear();
 Ttranslate *tr;getTranslator(&tr);

 if (isStreamsMenu())
 {
  for (unsigned int i=0;i<presets->size();i++)
  {
   streams.push_back(new TstreamPreset(this,-200+i,0,(*presets)[i]->presetName));
  }
  const char_t *activepresetname=getActivePresetName2();
  if (activepresetname)
  {
   const TfilterIDFFs *filters;getFilterIDFFs(activepresetname,&filters);
   for (TfilterIDFFs::const_iterator f=filters->begin();f!=filters->end();f++)
   {
    if (f->idff->is && (f->idff->show==0 || getParam2(f->idff->show)))
     // 10 is the group of the stream. 1 is for audio stream so should not be used
     streams.push_back(new TstreamFilter(this,getParam2(f->idff->order),10,f->idff,tr));      
   }
   std::sort(streams.begin(),streams.end(),streamsSort);
  }
  for (const TfilterIDFF *f=getNextFilterIDFF();f && f->name;f++)
  {
   if (f->show==0 || getParam2(f->show))
       // Group 1 or 2 should not be used : for audio (1) and subtitle (2) streams
       streams.push_back(new TstreamFilter(this,f->order?getParam2(f->order):1000,f->order?10:20,f,tr));
  }
  addOwnStreams();
  *pcStreams=(DWORD)streams.size();
 }
 tr->release();
 
 // Now the subtitles streams
 // Subtitle files
 char_t *pdummy = NULL;
 subtitleFiles.clear();
 if (getParam2(IDFF_subFiles) && getCurrentSubtitlesFile(&pdummy) == S_OK) // Returns E_NOTIMPL if TffdshowDec is not TffdshowDecVideo
 {
  TsubtitlesFile::findPossibleSubtitles(getSourceName(),
   getParamStr2(IDFF_subSearchDir),
   subtitleFiles, 
   (TsubtitlesFile::subtitleFilesSearchMode)getParam2(IDFF_streamsSubFilesMode));
  *pcStreams += subtitleFiles.size();
 }
 if (pdummy) CoTaskMemFree(pdummy);

 // Subtitle streams
 extractExternalStreams();
 *pcStreams += externalSubtitleStreams.size();
 *pcStreams += externalAudioStreams.size();
 *pcStreams += externalEditionStreams.size();
 return S_OK;
}
STDMETHODIMP TffdshowDec::Info(long lIndex, AM_MEDIA_TYPE** ppmt, DWORD* pdwFlags, LCID* plcid, DWORD* pdwGroup, WCHAR** ppszName, IUnknown** ppObject, IUnknown** ppUnk)
{
 // In order : audio streams, embedded subtitles, then FFDShow filters then in last external subtitles (which can vary)
 long internalStreams = isStreamsMenu() ? streams.size() : 0;
 long subFiles = getParam2(IDFF_subFiles) ? subtitleFiles.size() : 0;
 long firstFilterIndex = externalSubtitleStreams.size() + externalAudioStreams.size() + externalEditionStreams.size();
 long firstSubFileIndex = firstFilterIndex + internalStreams;
 long count = firstSubFileIndex + subFiles;
 if (lIndex<0 || lIndex>= count || !presetSettings) return E_INVALIDARG;
 if (internalStreams > 0 && lIndex >= firstFilterIndex && lIndex<firstSubFileIndex)
 {
  lIndex-=firstFilterIndex;
  if (ppmt) *ppmt=getInputMediaType(lIndex);
  if (pdwFlags)
   *pdwFlags=streams[lIndex]->getFlags();
  if (plcid) *plcid=0;
  if (pdwGroup) *pdwGroup=streams[lIndex]->group;
  if (ppszName)
   {
    ffstring name=streams[lIndex]->getName();//stringreplace(ffstring(streams[lIndex]->getName()),"&","&&",rfReplaceAll);
    size_t wlen=(name.size()+1)*sizeof(WCHAR);
    *ppszName=(WCHAR*)CoTaskMemAlloc(wlen);memset(*ppszName,0,wlen);
    nCopyAnsiToWideChar(*ppszName,name.c_str());
   }
  if (ppObject) *ppObject=NULL;
  if (ppUnk) *ppUnk=NULL;
  return S_OK;
 }

 if (ppmt) *ppmt=getInputMediaType(lIndex);
 if (plcid) *plcid=0;
 

 // Subtitles files
 if (lIndex >= firstSubFileIndex)
 {
  lIndex -= firstSubFileIndex;
  if (pdwGroup) *pdwGroup = 4; // Subtitle files (arbitrary group)
  const wchar_t *subtitleFilename = subtitleFiles[lIndex].c_str();
  if (ppszName)
  {
   size_t wlen=(subtitleFiles[lIndex].size()+1)*sizeof(WCHAR);
   *ppszName=(WCHAR*)CoTaskMemAlloc(wlen);memset(*ppszName,0,wlen);
   nCopyAnsiToWideChar(*ppszName,subtitleFilename);
  }
  if (pdwFlags)
  {
   char_t *curSubflnm = NULL;
   if (getCurrentSubtitlesFile(&curSubflnm) != S_OK) return E_NOTIMPL;

   // Current subtitle file : can be a custom subtitle file (IDFF_subTempFilename) or (if null) call getCurrentSubtitlesFile
   const char_t *curCustomSubflnm=getParamStr2(IDFF_subTempFilename);
   if (curCustomSubflnm && strcmp(curCustomSubflnm, _l("")) != 0)
   {
    if (getParam2(IDFF_subShowEmbedded)==0 && curCustomSubflnm != NULL && stricmp(subtitleFilename,curCustomSubflnm)==0)
     *pdwFlags = AMSTREAMSELECTINFO_ENABLED|AMSTREAMSELECTINFO_EXCLUSIVE;
    else
     *pdwFlags = 0;
   }
   else
   {
    if (getParam2(IDFF_subShowEmbedded)==0 && curSubflnm != NULL && stricmp(subtitleFilename,curSubflnm)==0)
     *pdwFlags = AMSTREAMSELECTINFO_ENABLED|AMSTREAMSELECTINFO_EXCLUSIVE;
    else
     *pdwFlags = 0;
   }

   if (curSubflnm) CoTaskMemFree(curSubflnm);
  }
  return S_OK;
 }

 TexternalStream stream;
 if (lIndex < (long)externalAudioStreams.size())
 {
  if (pdwGroup) *pdwGroup = 1; // Audio stream
  stream = externalAudioStreams[lIndex];
 }
 else if (lIndex < (long) (externalAudioStreams.size() + externalSubtitleStreams.size()))
 {
  lIndex -= externalAudioStreams.size();
  if (pdwGroup) *pdwGroup = 2; // Subtitles stream
  stream = externalSubtitleStreams[lIndex];
 }
 else // Editions
 {
  lIndex -= externalAudioStreams.size();
  lIndex -= externalSubtitleStreams.size();
  if (pdwGroup) *pdwGroup = 18; // Editions streams
  stream = externalEditionStreams[lIndex];
 }
 DPRINTF(_l("TffdshowDec::Info Stream #%d %s [%s] (%ld)"), lIndex, stream.streamName.c_str(), stream.streamLanguageName.c_str(), stream.langId);

 if (plcid) *plcid=stream.langId;

 if (pdwFlags)
 {
  if (stream.enabled)
   *pdwFlags = AMSTREAMSELECTINFO_ENABLED|AMSTREAMSELECTINFO_EXCLUSIVE;
  else
   *pdwFlags = 0;
 }
 if (ppszName)
 {
  ffstring streamName = stream.streamName;
  if (stream.streamName.length() == 0 && stream.streamLanguageName.length() > 0) streamName = _l("[") + stream.streamLanguageName + _l("]");
  else if (stream.langId == 0 && stream.streamLanguageName.length() > 0)  streamName += _l(" [") + stream.streamLanguageName + _l("]");
  size_t wlen=(streamName.length()+1)*sizeof(WCHAR);
  *ppszName=(WCHAR*)CoTaskMemAlloc(wlen);memset(*ppszName,0,wlen);
  nCopyAnsiToWideChar(*ppszName,streamName.c_str());
 }
 return S_OK;
}
STDMETHODIMP TffdshowDec::Enable(long lIndex, DWORD dwFlags)
{
 // In order : audio streams, embedded subtitles, then FFDShow filters then in last external subtitles (which can vary)
 long internalStreams = (isStreamsMenu() ? streams.size() : 0);
 long firstFilterIndex = externalSubtitleStreams.size() + externalAudioStreams.size() + externalEditionStreams.size();
 long firstSubFileIndex = firstFilterIndex + internalStreams;
 long count = firstSubFileIndex + subtitleFiles.size();

 if (lIndex<0 || lIndex >=count) return E_INVALIDARG;
 if (internalStreams > 0 && lIndex >= firstFilterIndex && lIndex<firstSubFileIndex)
 {
  lIndex -= firstFilterIndex;
  DPRINTF(_l("TffdshowDec::Enable postprocessing stream n°%ld"), lIndex);
  if (firsttransform) return S_OK;
  if (/*!(dwFlags&AMSTREAMSELECTENABLE_ENABLE)*/dwFlags!=AMSTREAMSELECTENABLE_ENABLE) return E_NOTIMPL;
  
  if (streams[lIndex]->action())
   {
    saveGlobalSettings();
    saveKeysSettings();
    saveRemoteSettings();
    saveActivePreset(NULL);
   }
  return S_OK;
 }


  // Subtitles files
 if (lIndex >= firstSubFileIndex)
 {
  lIndex -= firstSubFileIndex;
  DPRINTF(_l("TffdshowDec::Enable subtitle file n°%ld"), lIndex);
  if (lIndex >= (long)subtitleFiles.size()) return E_INVALIDARG;
  setSubtitlesFile(subtitleFiles[lIndex].c_str());
  return S_OK;
 }

 if (lIndex < (long)externalAudioStreams.size())
 {
  DPRINTF(_l("TffdshowDec::Enable subtitle stream #%ld Id %ld"), lIndex, externalAudioStreams[lIndex].streamNb);
  return setExternalStream(1, externalAudioStreams[lIndex].streamNb);
 }
 else if (lIndex < (long)(externalAudioStreams.size() + externalSubtitleStreams.size()))
 {
  lIndex -= (long)externalAudioStreams.size();
  DPRINTF(_l("TffdshowDec::Enable subtitle stream #%ld Id %ld"), lIndex, externalSubtitleStreams[lIndex].streamNb);
  return setExternalStream(2, externalSubtitleStreams[lIndex].streamNb);
 }
 else
 {
  lIndex -= (long)externalAudioStreams.size();
  lIndex -= (long)externalSubtitleStreams.size();
  DPRINTF(_l("TffdshowDec::Enable edition #%ld Id %ld"), lIndex, externalEditionStreams[lIndex].streamNb);
  return setExternalStream(18, externalEditionStreams[lIndex].streamNb);
 }
}

TffdshowDec::TstreamFilter::TstreamFilter(TffdshowDec *Iself,int Iorder,int Igroup,const TfilterIDFF *If,Ttranslate *Itr):Tstream(Iself,Iorder,Igroup),f(If)
{
 tr=Itr;
 tr->addref();
}
TffdshowDec::TstreamFilter::~TstreamFilter()
{
 tr->release();
}
DWORD TffdshowDec::TstreamFilter::getFlags(void)
{
 return ((f->is==IDFF_isKeys && !self->keys) || self->getParam2(f->is)==0)?0:AMSTREAMSELECTINFO_ENABLED;
}
const char_t* TffdshowDec::TstreamFilter::getName(void)
{
 const char_t *ret=tr->translate(NULL,f->dlgId,0,f->name);
 if (strcmp(ret,_l("Subtitles"))==0)
  return _l("Subtitles ");
 else
  return ret;
}
bool TffdshowDec::TstreamFilter::action(void)
{
 self->invParam(f->is);
 return true;
}

DWORD TffdshowDec::TstreamPreset::getFlags(void)
{
 return stricmp(preset,self->presetSettings->presetName)==0?AMSTREAMSELECTINFO_ENABLED|AMSTREAMSELECTINFO_EXCLUSIVE:0;
}
const char_t* TffdshowDec::TstreamPreset::getName(void)
{
 return preset;
}
bool TffdshowDec::TstreamPreset::action(void)
{
 self->setActivePreset(preset,0);
 return true;
}

STDMETHODIMP TffdshowDec::getShortDescription(char_t *buf,int buflen)
{
 if (!buf) return E_POINTER;
 const char_t *activepresetname=getActivePresetName2();
 if (!activepresetname)
  {
   buf[0]='\0';
   return E_UNEXPECTED;
  }
 int len = tsnprintf_s(buf, buflen, _TRUNCATE, _l("ffdshow %s: "),FFDSHOW_VER);
 buf+=len;buflen-=len;
 const TfilterIDFFs *filters;getFilterIDFFs(activepresetname,&filters);
 for (TfilterIDFFs::const_iterator f=filters->begin();f!=filters->end() && buflen>0;f++)
  if (f->idff->is && getParam2(f->idff->is))
   {
    len=tsnprintf_s(buf, buflen, _TRUNCATE, _l("%s "),f->idff->name);
    buf+=len;buflen-=len;
   }
 for (const TfilterIDFF *f=getNextFilterIDFF();f && f->name && buflen>0;f++)
  if (f->is && getParam2(f->is))
   {
    len=tsnprintf_s(buf, buflen, _TRUNCATE,_l("%s "),f->name);
    buf+=len;buflen-=len;
   }
 buf[-1]='\0';
 return true;
}

STDMETHODIMP TffdshowDec::createPresetPages(const char_t *presetname,TffdshowPageDec *pages)
{
 if (!presetname) return S_FALSE;
 Tpreset *preset=presets->getPreset(presetname,false);
 if (preset)
  {
   preset->createPages(pages);
   return S_OK;
  }
 else
  return S_FALSE;
}

STDMETHODIMP TffdshowDec::getFilterIDFFs(const char_t *presetname,const TfilterIDFFs* *filters)
{
 if (!filters) return E_POINTER;
 Tpreset *preset=presets->getPreset(presetname,false);
 if (preset)
  {
   *filters=preset->getFilters();
   return S_OK;
  }
 else
  return S_FALSE;
}

HRESULT TffdshowDec::NewSegment(REFERENCE_TIME tStart,REFERENCE_TIME tStop,double dRate)
{
 if ((tStop&0xfffffffffffffffLL)==0xfffffffffffffffLL)
  moviesecs=1; //tStop is most probably wrong, but don't risk getDuration
 else
  moviesecs=int(tStop/REF_SECOND_MULT);
 if (filters) filters->onSeek();
 m_dirtyStop = false;
 return CTransformFilter::NewSegment(tStart,tStop,dRate);
}

STDMETHODIMP TffdshowDec::GetPages(CAUUID *pPages)
{
 DPRINTF(_l("TffdshowDec::GetPages"));
 initDialog();
 onTrayIconChange(0,0);

 pPages->cElems=1;
 pPages->pElems=(GUID*)CoTaskMemAlloc(pPages->cElems*sizeof(GUID));
 if (pPages->pElems==NULL) return E_OUTOFMEMORY;
 pPages->pElems[0]=proppageid;
 return S_OK;
}

STDMETHODIMP_(unsigned int) TffdshowDec::getPresetAutoloadItemsCount2(void)
{
 return presetSettings?(unsigned int)presetSettings->getAutoPresetItemsCount():0;
}
STDMETHODIMP TffdshowDec::getPresetAutoloadItemInfo(unsigned int index,const char_t* *name,const char_t* *hint,int *allowWildcard,int *is,int *isVal,char_t *val,size_t vallen,int *isList,int *isHelp)
{
 if (!presetSettings) return E_UNEXPECTED;
 if (!name || !allowWildcard || !is || !val || !vallen) return E_POINTER;
 if (index>=presetSettings->getAutoPresetItemsCount()) return E_INVALIDARG;
 presetSettings->getAutoPresetItemInfo(index,name,hint,allowWildcard,is,isVal,val,vallen,isList,isHelp);
 return S_OK;
}
STDMETHODIMP TffdshowDec::setPresetAutoloadItem(unsigned int index,int is,const char_t *val)
{
 if (!presetSettings) return E_UNEXPECTED;
 if (!val) return E_POINTER;
 if (index>=presetSettings->getAutoPresetItemsCount()) return E_INVALIDARG;
 presetSettings->setAutoPresetItem(index,is,val);
 return S_OK;
}
STDMETHODIMP TffdshowDec::getPresetAutoloadItemHelp(unsigned int index,const char_t* *helpPtr)
{
 if (!presetSettings) return E_UNEXPECTED;
 if (!helpPtr) return E_POINTER;
 if (index>=presetSettings->getAutoPresetItemsCount()) return E_INVALIDARG;
 presetSettings->getAutoPresetItemHelp(index,helpPtr);
 return *helpPtr?S_OK:S_FALSE;
}

STDMETHODIMP_(const char_t*) TffdshowDec::getPresetAutoloadItemList(unsigned int paramIndex,unsigned int listIndex)
{
 return presetSettings && paramIndex<presetSettings->getAutoPresetItemsCount()?presetSettings->getAutoPresetItemList(this,paramIndex,listIndex):NULL;
}
STDMETHODIMP_(const char_t**) TffdshowDec::getSupportedFOURCCs(void)
{
 return globalSettings->getFOURCClist();
}
STDMETHODIMP_(const Tstrptrs*) TffdshowDec::getCodecsList(void)
{
 if (codecs.empty())
  globalSettings->getCodecsList(codecs);
 return &codecs;
}

STDMETHODIMP TffdshowDec::queryFilterInterface(const IID &iid,void **ptr)
{
 if (!ptr) return E_POINTER;
 if (!filters) return E_UNEXPECTED;
 lock(IDFF_lockPresetPtr);
 HRESULT res=filters->queryFilterInterface(iid,ptr);
 unlock(IDFF_lockPresetPtr);
 return res;
}
STDMETHODIMP TffdshowDec::setOnNewFiltersMsg(HWND wnd,unsigned int msg)
{
 onNewFiltersWnd=wnd;onNewFiltersMsg=msg;
 return S_OK;
}
STDMETHODIMP TffdshowDec::sendOnNewFiltersMsg(void)
{
 if (onNewFiltersMsg)
  {
   PostMessage(onNewFiltersWnd,onNewFiltersMsg,0,0);
   return S_OK;
  }
 else
  return S_FALSE;
}

STDMETHODIMP_(TinputPin*) TffdshowDec::getInputPin()
{
 return minput;
}

STDMETHODIMP_(CTransformOutputPin*) TffdshowDec::getOutputPin()
{
 return m_pOutput;
}

STDMETHODIMP TffdshowDec::getExternalStreams(void **pAudioStreams, void **pSubtitleStreams, void **pEditionStreams)
{
 *pAudioStreams=&externalAudioStreams;
 *pSubtitleStreams=&externalSubtitleStreams;
 *pEditionStreams=&externalEditionStreams;
 return S_OK;
}

STDMETHODIMP TffdshowDec::extractExternalStreams(void)
{
 externalSubtitleStreams.clear();
 externalAudioStreams.clear();
 externalEditionStreams.clear();
 comptr<IEnumFilters> eff;
 IFilterGraph    *m_pGraph = NULL;
 comptr<IffdshowDecVideo> deciV;
 this->NonDelegatingQueryInterface(getGUID<IffdshowDecVideo>(),(void**) &deciV);
 Ttranslate *tr = NULL;getTranslator(&tr);


 getGraph(&m_pGraph); // Graph we belong to
 if (m_pGraph == NULL) return E_FAIL;
 bool foundHaali = false;
 if (m_pGraph->EnumFilters(&eff)==S_OK)
 {
  eff->Reset();
  const char_t *fileName = getSourceName();
  for (comptr<IBaseFilter> bff;eff->Next(1,&bff,NULL)==S_OK;bff=NULL)
  {
   // Look for FFDShowVideo (we may be inside FFDShowAudio here)
   if (deciV == NULL)
    bff->QueryInterface(getGUID<IffdshowDecVideo>(), (void**)&deciV);
   // Stop browsing the graph if we have found the streams and FFDShow video
   if (deciV != NULL && foundHaali) break;

   // We have found the streams but not FFDShow video
   if (foundHaali) continue;

   char_t name[MAX_PATH],filtername[MAX_PATH];
   getFilterName(bff,name,filtername,countof(filtername));

   if (IS_FFDSHOW_VIDEO(filtername)) continue;

   FILTER_INFO filterinfo;
   bff->QueryFilterInfo(&filterinfo);
   if (filterinfo.pGraph)
    filterinfo.pGraph->Release();

   IAMStreamSelect *pAMStreamSelect = NULL;
   bff->QueryInterface(IID_IAMStreamSelect, (void**) &pAMStreamSelect);
   if (pAMStreamSelect == NULL)
    continue;


   // Haali splitter
   if (!strcmp(filtername, fileName))
   {
    externalSubtitleStreams.clear();
    externalAudioStreams.clear();
    foundHaali = true;
   }

   DWORD cStreams = 0;
   pAMStreamSelect->Count(&cStreams);
   TexternalStreams localAudioStreams;

   for (long streamNb=0; streamNb<(long)cStreams; streamNb++)
   {
    DWORD streamSelect = 0;
    LCID streamLanguageId = 0;
    DWORD streamGroup = 0;
    WCHAR *pstreamName = NULL;
    HRESULT hr = pAMStreamSelect->Info(streamNb, NULL, &streamSelect, 
     &streamLanguageId, &streamGroup, &pstreamName, NULL, NULL);
    if (hr != S_OK) continue;

    //DPRINTF(_l("TffdshowDec::extractExternalStreams Group %ld Stream %s"), streamGroup, pstreamName);

    // Not audio neither subtitles neither editions or only one audio stream
    if ((streamGroup != 1 && streamGroup != 2 && streamGroup != 6590033 && streamGroup != 18)
     || (streamGroup == 1 && cStreams == 1) // audio, only one
     || (streamGroup == 18 && cStreams == 1)) //editions, only one
    {
     if (pstreamName != NULL)
      CoTaskMemFree(pstreamName);
     continue;
    }

    // Get language name
    char_t languageName[256] = _l("");
    if (streamLanguageId != 0) GetLocaleInfo(streamLanguageId, LOCALE_SLANGUAGE, languageName, 255);

    char_t streamName[256];
    if (pstreamName)
    {
     if (!strncmp(pstreamName, _l("Undetermined, "), 14) && strlen(pstreamName) > 15) // MPC
      text<char_t>(&pstreamName[14], -1, streamName, 255);
     else 
      text<char_t>(pstreamName, -1, streamName, 255);
    }
    else
     tsnprintf_s(streamName, countof(streamName), _TRUNCATE, _l("%s (%ld)"), tr->translate(_l("Undetermined")), streamNb);

    TexternalStream stream;
    stream.filterName = ffstring(filtername);
    stream.streamNb = streamNb;
    stream.langId = streamLanguageId;
    if ((streamSelect & AMSTREAMSELECTINFO_ENABLED) == AMSTREAMSELECTINFO_ENABLED)
     stream.enabled = true;
    else stream.enabled = false;

    /*DPRINTF(_l("extract stream %d %s from filter %s %s"), streamNb, streamName, filtername,
     (stream.enabled ? _l("Enabled") : _l("")));*/

    stream.streamName = ffstring(streamName);
    stream.streamLanguageName = ffstring(languageName);
    if (streamGroup == 1) // Audio
    {
     localAudioStreams.push_back(stream);
    }
    else if (streamGroup == 2 || streamGroup == 6590033)// Subtitles
    {
     externalSubtitleStreams.push_back(stream);
    }
    else
    {
     externalEditionStreams.push_back(stream);
    }
    if (pstreamName != NULL)
     CoTaskMemFree(pstreamName);
   }

   // Only one filter may handle audio streams switching
   if (externalAudioStreams.size() < localAudioStreams.size())
   {
    externalAudioStreams.clear();
    for (unsigned int i=0;i<localAudioStreams.size();i++)
    {
     externalAudioStreams.push_back(localAudioStreams[i]);
    }
   }

   pAMStreamSelect->Release();
  }
 }

 // If subtitles streams already found somewhere else skip next step
 if (externalSubtitleStreams.size() > 0) return S_OK;
 if (deciV == NULL) return S_OK;  

 // Now add subtitle streams connected to FFDShow input text pin if any
 int textpinconnectedCnt=deciV->getConnectedTextPinCnt();
 if (!textpinconnectedCnt) return S_OK;
 
 int currentEmbeddedStream = getParam2(IDFF_subShowEmbedded);
 for (int i=0;i<textpinconnectedCnt;i++)
 {
  const char_t *trackName = NULL, *langName = NULL;
  int found,id;
  LCID langId = 0;
  deciV->getConnectedTextPinInfo(i,&trackName,&langName,&langId,&id,&found);
  if (found)
  {
   char_t s[256] = _l("");
   if (trackName[0])
   {
    strncatf(s, countof(s), _l("%s"), trackName);
   }
   else
   {
    ff_strncpy(s, tr->translate(_l("embedded")), countof(s));
    strncatf(s, countof(s), _l(" (%d)"), id);
   }

   TexternalStream stream;
   stream.filterName = ffstring(_l("FFDSHOW"));
   stream.streamNb = id;
   stream.langId = langId;
   if (currentEmbeddedStream == id)
    stream.enabled = true;
   else stream.enabled = false;

   /*DPRINTF(_l("extract internal stream %d %s %s"), stream.streamNb, s,
     (stream.enabled ? _l("Enabled") : _l("")));*/
   
   stream.streamName = ffstring(trackName);
   stream.streamLanguageName = ffstring(langName);
   externalSubtitleStreams.push_back(stream);
  }
 }

 // If we have at least 1 external subtitles stream but not haali, add one entry for "No subtitles"
 if (!foundHaali && externalSubtitleStreams.size() > 0)
 {
   TexternalStream stream;
   stream.filterName = ffstring(_l("FFDSHOW"));
   stream.streamNb = 0;
   stream.langId = 0;
   stream.enabled = false;
   stream.streamName = trans->translate(_l("No subtitles"));
   stream.streamLanguageName = _l("");
   externalSubtitleStreams.push_back(stream);
   
 }

 return S_OK;
}

STDMETHODIMP TffdshowDec::setExternalStream(int group, long streamNb)
{
 CAutoLock lock(&m_csSetExternalStream);
 TexternalStreams *pStreams = NULL;
 TexternalStream *pStream = NULL;
 if (group == 1) // Audio
  pStreams = &externalAudioStreams;
 else if (group == 2 || group == 6590033)// Subtitles
  pStreams = &externalSubtitleStreams;
 else
  pStreams = &externalEditionStreams;

 for (long l = 0; l<(long)pStreams->size(); l++)
 {
  if ((*pStreams)[l].streamNb == streamNb)
  {
   pStream = &(*pStreams)[l];
   break;
  }
 }
 if (pStream == NULL) return E_FAIL;


 // Embedded subtitles within FFDShow
 if (!strcmp(pStream->filterName.c_str(), _l("FFDSHOW")))
 {
  DPRINTF(_l("TffdshowDec::setExternalStream set internal ffdshow stream %ld"), streamNb);
  int oldId=getParam2(IDFF_subShowEmbedded);
  putParam(IDFF_subShowEmbedded,streamNb);
 }
 else
 {
  comptr<IEnumFilters> eff;
  IFilterGraph    *m_pGraph = NULL;
  getGraph(&m_pGraph); // Graph we belong to
  if (m_pGraph->EnumFilters(&eff)==S_OK)
  {
   eff->Reset();
   const char_t *fileName = getSourceName();
   for (comptr<IBaseFilter> bff;eff->Next(1,&bff,NULL)==S_OK;bff=NULL)
   {
    char_t name[MAX_PATH],filtername[MAX_PATH];
    getFilterName(bff,name,filtername,countof(filtername));

    if (strcmp(pStream->filterName.c_str(), filtername)) continue;

    IAMStreamSelect *pAMStreamSelect = NULL;
    bff->QueryInterface(IID_IAMStreamSelect, (void**) &pAMStreamSelect);
    if (pAMStreamSelect == NULL)
     continue;

    /*if (foundHaali && strcmp(filtername, fileName))
    {
    pAMStreamSelect->Release();
    continue;
    }*/
    DPRINTF(_l("TffdshowDec::setExternalStream set external stream %ld inside filter %s"), streamNb, filtername);
    pAMStreamSelect->Enable(streamNb, AMSTREAMSELECTENABLE_ENABLE);
    pAMStreamSelect->Release();
    break;
   }
  }
 }
 return S_OK;
}