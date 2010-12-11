/*
 * Copyright (c) 2004-2006 Milan Cutka
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
#include "TtextInputPin.h"
#include "TffDecoderVideo.h"
#include "ffdshow_constants.h"
#include "ffdshow_mediaguids.h"
#include "matroskaSubs.h"
#include "Tsubreader.h"
#include "TsubtitlesSettings.h"
#include "dsutil.h"

TtextInputPin::TtextInputPin(TffdshowDecVideo* pFilter,HRESULT* phr,const wchar_t *pinname,int Iid):
    CDeCSSInputPin(NAME("TtextInputPin"),pFilter,phr,pinname),
    id(Iid),
    filter(pFilter),
    segmentStart(0),
    extradata(NULL),extradatasize(0),
    firsttime(true),
    needSegment(false),
    found(false),
    utf8(false)
{
    name[0]=langName[0]=trackName[0]='\0';
    langId=0;
}
TtextInputPin::~TtextInputPin()
{
    if (extradata) {
        free(extradata);
    }
}

HRESULT TtextInputPin::CheckMediaType(const CMediaType *mtIn)
{
    /* Return S_OK (accept the subtitles connection) if embedded subtitles are enabled (IDFF_subTextpin)
       and if the given subtitles format is enabled
       (IDFF_subText for text, IDFF_subVobsub for VobSub subs, IDFF_subSSA for SSA subs, IDFF_subPGS for Blu-ray subs)
    */
    if (filter->getParam2(IDFF_subTextpin) == 0 && mtIn->majortype!=MEDIATYPE_DVD_ENCRYPTED_PACK) {
        return VFW_E_TYPE_NOT_ACCEPTED;
    } else {
        if (mtIn->majortype==MEDIATYPE_Subtitle) {
            if ((mtIn->subtype==MEDIASUBTYPE_SSA || mtIn->subtype==MEDIASUBTYPE_ASS || mtIn->subtype==MEDIASUBTYPE_ASS2) && (filter->getParam2(IDFF_subSSA) == 1)) {
                return S_OK;    // SSA/ASS/ASS2 subtitles
            } else if (mtIn->subtype==MEDIASUBTYPE_UTF8 && (filter->getParam2(IDFF_subText) == 1)) {
                return S_OK;    // UTF-8 plain text subtitles
            } else if (mtIn->subtype==MEDIASUBTYPE_NULL && mtIn->formattype == FORMAT_SubtitleInfo && (filter->getParam2(IDFF_subPGS) == 1)) {
                return S_OK;    // Blu-ray subtitles
            } else if (mtIn->subtype==MEDIASUBTYPE_HDMV_PGS && (filter->getParam2(IDFF_subPGS) == 1)) {
                return S_OK;    // Blu-ray subtitles
            } else if (mtIn->subtype==MEDIASUBTYPE_VOBSUB && (filter->getParam2(IDFF_subVobsub) == 1)) {
                return S_OK;    // VobSub subtitles
            } else if (mtIn->subtype==MEDIASUBTYPE_USF) {
                return S_OK;    // USF subtitles
            } else {
                return VFW_E_TYPE_NOT_ACCEPTED;
            }
        } else if (mtIn->majortype==MEDIATYPE_Text && (filter->getParam2(IDFF_subText) == 1)) {
            return S_OK;    // Plain text subtitles
        } else if ((mtIn->majortype==MEDIATYPE_DVD_ENCRYPTED_PACK || mtIn->majortype==MEDIATYPE_MPEG2_PACK || mtIn->majortype==MEDIATYPE_MPEG2_PES || mtIn->majortype==MEDIATYPE_Video || mtIn->majortype==MEDIATYPE_Stream)
                   && (mtIn->subtype==MEDIASUBTYPE_DVD_SUBPICTURE || mtIn->subtype==MEDIASUBTYPE_CVD_SUBPICTURE || mtIn->subtype==MEDIASUBTYPE_SVCD_SUBPICTURE)) {
            return S_OK;    // DVD, CVD, SVCD subtitles
        } else {
            return VFW_E_TYPE_NOT_ACCEPTED;
        }
    }
}

HRESULT TtextInputPin::SetMediaType(const CMediaType* mtIn)
{
    DPRINTF(_l("TtextInputPin::SetMediaType"));
    name[0]='\0';
    HRESULT hr=CDeCSSInputPin::SetMediaType(mtIn);
    if (hr!=S_OK) {
        return hr;
    }
    type=Tsubreader::SUB_INVALID;
    needSegment=false;
    if (mtIn->majortype==MEDIATYPE_Subtitle) {
        const SUBTITLEINFO *psi=(const SUBTITLEINFO*)mtIn->Format();
        ffstring isolang = (psi->IsoLang[0])? text<char_t>(psi->IsoLang) : _l("Und");

        Ttranslate *tr = NULL;
        filter->getTranslator(&tr);
        const char_t *isoname=tr->translate(TsubtitlesSettings::getLangDescrIso(isolang.c_str()));
        langId=TsubtitlesSettings::getLangId(isolang.c_str());

        if (isoname == NULL) {
            isoname = isolang.c_str();
        }

        if (isoname) {
            text<char_t>(isoname, strlen(isoname), langName, countof(langName));
        }

        text<char_t>(psi->TrackName, (int)countof(psi->TrackName), trackName, countof(trackName));
        tsnprintf_s(name, 256, _TRUNCATE, _l("%s%s%s%s"),trackName,trackName[0]?_l(" ["):_l(""),isoname,trackName[0]?_l("]"):_l(""));
        if (extradata) {
            free(extradata);
            extradata=NULL;
        }
        extradatasize=mtIn->cbFormat-psi->dwOffset;
        if (extradatasize) {
            unsigned char*extradataSrc=mtIn->pbFormat+psi->dwOffset;
            if (extradatasize>=3 && extradataSrc[0]==0xef && extradataSrc[1]==0xbb && extradataSrc[2]==0xbf) { // BOM UTF-8
                extradataSrc+=3;
                extradatasize-=3;
                utf8=true;
            }
            extradata=(unsigned char*)malloc(extradatasize);
            memcpy(extradata,extradataSrc,extradatasize);
        }
        if (mtIn->subtype==MEDIASUBTYPE_UTF8) {
            type=Tsubreader::SUB_SUBVIEWER|Tsubreader::SUB_ENC_UTF8;
        } else if (mtIn->subtype==MEDIASUBTYPE_VOBSUB) {
            type=Tsubreader::SUB_VOBSUB;
        } else if (mtIn->subtype==MEDIASUBTYPE_HDMV_PGS || (mtIn->subtype == MEDIASUBTYPE_NULL && mtIn->formattype == FORMAT_SubtitleInfo)) {
            type=Tsubreader::SUB_PGS;
        } else if (mtIn->subtype==MEDIASUBTYPE_SSA || mtIn->subtype==MEDIASUBTYPE_ASS || mtIn->subtype==MEDIASUBTYPE_ASS2) {
            type=Tsubreader::SUB_SSA;
            if (ismatroska) {
                utf8=true;
            }
        } else if (mtIn->subtype==MEDIASUBTYPE_USF) {
            type=Tsubreader::SUB_USF;
        }
    } else if (mtIn->majortype==MEDIATYPE_Text) {
        type=Tsubreader::SUB_SUBVIEWER/*|Tsubreader::SUB_ENC_UTF8*/;    //TODO: enable for mp4 subtitles?
    } else if (mtIn->subtype==MEDIASUBTYPE_DVD_SUBPICTURE) {
        if (mtIn->majortype==MEDIATYPE_Stream) {
            needSegment=true;
        }
        type=Tsubreader::SUB_DVD;
    } else if (mtIn->subtype==MEDIASUBTYPE_CVD_SUBPICTURE) {
        type=Tsubreader::SUB_CVD;
    } else if (mtIn->subtype==MEDIASUBTYPE_SVCD_SUBPICTURE) {
        type=Tsubreader::SUB_SVCD;
    }
    firsttime=true;
    if (filter->getParam2(IDFF_subShowEmbedded)==0) {
        filter->putParam(IDFF_subShowEmbedded,id);
    }
    return S_OK;
}
STDMETHODIMP TtextInputPin::ReceiveConnection(IPin* pConnector, const AM_MEDIA_TYPE* pmt)
{
    DPRINTF(_l("TtextInputPin::ReceiveConnection"));
    utf8=false;
    const CLSID &ref=GetCLSID(pConnector);
    ismatroska=false;
    if ( searchPrevNextFilter(PINDIR_INPUT,pConnector,CLSID_HaaliMediaSplitter)
            || searchPrevNextFilter(PINDIR_INPUT,pConnector,CLSID_MPC_MatroskaSplitter)
            || searchPrevNextFilter(PINDIR_INPUT,pConnector,CLSID_GabestMatroskaSplitter)
            || searchPrevNextFilter(PINDIR_INPUT,pConnector,CLSID_LAVFSplitter)) {
        ismatroska=true;
    }
#if 0
    PIN_INFO pininfo;
    FILTER_INFO filterinfo;
    pConnector->QueryPinInfo(&pininfo);
    if (pininfo.pFilter) {
        pininfo.pFilter->QueryFilterInfo(&filterinfo);
        DPRINTF (_l("TtextInputPin::CompleteConnect filter=%s pin=%s"),filterinfo.achName,pininfo.achName);
        if (filterinfo.pGraph) {
            filterinfo.pGraph->Release();
        }
        pininfo.pFilter->Release();
    }
    DPRINTF(_l("CLSID 0x%x,0x%x,0x%x"),ref.Data1,ref.Data2,ref.Data3);
    for(int i=0; i<8; i++) {
        DPRINTF(_l(",0x%2x"),ref.Data4[i]);
    }
#endif

    return CDeCSSInputPin::ReceiveConnection(pConnector,pmt);
}
HRESULT TtextInputPin::CompleteConnect(IPin *pReceivePin)
{
    if (name[0]=='\0') {
        PIN_INFO pi;
        pReceivePin->QueryPinInfo(&pi);
        text<char_t>(pi.achName, -1, name, countof(name));
        if (pi.pFilter) {
            pi.pFilter->Release();
        }
    }
    return CDeCSSInputPin::CompleteConnect(pReceivePin);
}

STDMETHODIMP TtextInputPin::NewSegment(REFERENCE_TIME tStart,REFERENCE_TIME tStop,double dRate)
{
    DPRINTF(_l("TtextInputPin::NewSegment"));
    segmentStart=tStart;
    filter->resetSubtitles(id);
    return CBaseInputPin::NewSegment(tStart,tStop,dRate);
}
STDMETHODIMP TtextInputPin::EndOfStream(void)
{
    return S_OK;
}
STDMETHODIMP TtextInputPin::BeginFlush(void)
{
    return S_OK;
}
STDMETHODIMP TtextInputPin::EndFlush(void)
{
    if (Tsubreader::isBitmapsub(type)) {
        filter->resetSubtitles(id);
    }
    return S_OK;
}

STDMETHODIMP TtextInputPin::QuerySupported(REFGUID PropSet,ULONG Id,ULONG *pTypeSupport)
{
    if (PropSet!=AM_KSPROPSETID_DvdSubPic) {
        return CDeCSSInputPin::QuerySupported(PropSet,Id,pTypeSupport);
    }
    switch (Id) {
        case AM_PROPERTY_DVDSUBPIC_PALETTE:
            *pTypeSupport=KSPROPERTY_SUPPORT_SET;
            return S_OK;
        case AM_PROPERTY_DVDSUBPIC_HLI:
            *pTypeSupport=KSPROPERTY_SUPPORT_SET;
            return S_OK;
        case AM_PROPERTY_DVDSUBPIC_COMPOSIT_ON:
            *pTypeSupport=KSPROPERTY_SUPPORT_SET;
            return S_OK;
        default:
            return E_PROP_ID_UNSUPPORTED;
    }
}
STDMETHODIMP TtextInputPin::Set(REFGUID PropSet,ULONG ctl_Id,LPVOID pInstanceData,ULONG InstanceLength,LPVOID pPropertyData,ULONG DataLength)
{
    if (PropSet!=AM_KSPROPSETID_DvdSubPic) {
        return CDeCSSInputPin::Set(PropSet,ctl_Id,pInstanceData,InstanceLength,pPropertyData,DataLength);
    }
    filter->ctlSubtitles(id,type,ctl_Id,pPropertyData,DataLength);
    firsttime=false;
    found=true;
    return S_OK;
}

HRESULT TtextInputPin::Receive(IMediaSample *pSample)
{
    HRESULT hr;
    ASSERT(pSample);

    hr=CDeCSSInputPin::Receive(pSample);
    if (FAILED(hr)) {
        return hr;
    }

    AM_MEDIA_TYPE* pmt=NULL;
    if (SUCCEEDED(pSample->GetMediaType(&pmt)) && pmt) {
        CMediaType mt(*pmt);
        bool oldfirsttime=firsttime;
        SetMediaType(&mt);
        DeleteMediaType(pmt);
        firsttime=oldfirsttime;
    }

    if (firsttime) {
        DPRINTF(_l("TtextInputPin::Receive initSubtitles"));
        firsttime=false;
        found=filter->initSubtitles(id,type,extradata,extradatasize);
    }

    REFERENCE_TIME t1=-1,t2=-1;
    pSample->GetTime(&t1,&t2);

    BYTE *data;
    pSample->GetPointer(&data);
    long datalen=pSample->GetActualDataLength();
    if (Tsubreader::isDVDsub(type)) {
        StripPacket(data,datalen);
    }
    //int sStart=float(t1+segmentStart)/REF_SECOND_MULT,sStop=float(t2+segmentStart)/REF_SECOND_MULT;
    //data[datalen]='\0';
    //DPRINTF(_l("%02i:%02i:%02i-%02i:%02i:%02i %s"),sStart/3600,(sStart%3600)/60,sStart%60,sStop/3600,(sStop%3600)/60,sStop%60,(const char_t*)text<char_t>((const char*)data));
    if (data && datalen>0) {
        filter->addSubtitle(id,t1+segmentStart,t2+segmentStart,data,datalen,utf8);
    }
    return S_OK;
}

HRESULT TtextInputPin::Transform(IMediaSample* pSample)
{
    return S_FALSE;
}

STDMETHODIMP TtextInputPin::Disconnect(void)
{
    filter->putParam(IDFF_subShowEmbedded,0);
    name[0]='\0';
    found=false;
    return CDeCSSInputPin::Disconnect();
}

HRESULT TtextInputPin::getInfo(const char_t* *namePtr,int *idPtr,int *foundPtr)
{
    if (namePtr) {
        *namePtr=name;
    }
    if (idPtr) {
        *idPtr=id;
    }
    if (foundPtr) {
        *foundPtr=found|IsConnected();
    }
    return S_OK;
}

HRESULT TtextInputPin::getInfo(const char_t* *trackNamePtr, const char_t* *langNamePtr,LCID *langIdPtr,int *idPtr,int *foundPtr)
{
    if (trackName) {
        *trackNamePtr=trackName;
    }
    if (langName) {
        *langNamePtr=langName;
    }
    if (idPtr) {
        *idPtr=id;
    }
    if (langIdPtr) {
        *langIdPtr = langId;
    }
    if (foundPtr) {
        *foundPtr=found|IsConnected();
    }
    return S_OK;
}
