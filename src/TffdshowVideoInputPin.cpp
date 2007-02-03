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
#include "TffdshowVideoInputPin.h"
#include "ffdebug.h"
#include "dsutil.h"
#include "TvideoCodec.h"
#include "theora/theora.h"
#include "TffDecoderVideo.h"
#include "Tconvert.h"
#include "TffdshowVideo.h"
#include "TsubtitlesFile.h"
#include "libavcodec/bitstream.h"
#include "ffdshow_mediaguids.h"

//================================== TffdshowVideoInputPin =================================
TffdshowVideoInputPin::TffdshowVideoInputPin(TCHAR *objectName,TffdshowVideo *Ifv,HRESULT *phr)
 :TinputPin(objectName,Ifv->filter,phr,L"In"),
  allocator(Ifv->filter,phr),
  fv(Ifv),
  video(NULL)
{
 usingOwnAllocator=false;
 supdvddec=fv->deci->getParam2(IDFF_supDVDdec) && fv->deci->getParam2(IDFF_mpg2);
 done();
}
TffdshowVideoInputPin::~TffdshowVideoInputPin()
{
 done();
}

HRESULT TffdshowVideoInputPin::CheckMediaType(const CMediaType* mt)
{
 if (mt->majortype!=MEDIATYPE_Video && !(mt->majortype==MEDIATYPE_DVD_ENCRYPTED_PACK && supdvddec)) return VFW_E_TYPE_NOT_ACCEPTED;
 if (mt->subtype==MEDIASUBTYPE_DVD_SUBPICTURE) return VFW_E_TYPE_NOT_ACCEPTED;
 BITMAPINFOHEADER *hdr=NULL,hdr0;

 if (mt->formattype==FORMAT_VideoInfo)
  {
   VIDEOINFOHEADER *vih=(VIDEOINFOHEADER*)mt->pbFormat;
   hdr=&vih->bmiHeader;
   fixMPEGinAVI(hdr->biCompression);
  }
 else if (mt->formattype==FORMAT_VideoInfo2)
  {
   VIDEOINFOHEADER2 *vih2=(VIDEOINFOHEADER2*)mt->pbFormat;
   hdr=&vih2->bmiHeader;
   fixMPEGinAVI(hdr->biCompression);
  }
 else if (mt->formattype==FORMAT_MPEGVideo)
  {
   MPEG1VIDEOINFO *mpeg1info=(MPEG1VIDEOINFO*)mt->pbFormat;
   hdr=&(hdr0=mpeg1info->hdr.bmiHeader);
   hdr->biCompression=FOURCC_MPG1;
  }
 else if (mt->formattype==FORMAT_MPEG2Video)
  {
   MPEG2VIDEOINFO *mpeg2info=(MPEG2VIDEOINFO*)mt->pbFormat;
   hdr=&(hdr0=mpeg2info->hdr.bmiHeader);
   if (hdr->biCompression==0) hdr->biCompression=FOURCC_MPG2;
  }
 else if (mt->formattype==FORMAT_TheoraIll)
  {
   sTheoraFormatBlock *oggFormat=(sTheoraFormatBlock*)mt->pbFormat;
   hdr=&hdr0;
   hdr->biWidth=oggFormat->width;hdr->biHeight=oggFormat->height;
   hdr->biCompression=FOURCC_THEO;
  } 
 else if (mt->formattype==FORMAT_RLTheora)
  {
   hdr=&hdr0;
   hdr->biCompression=FOURCC_THEO;
  } 
 else
  return VFW_E_TYPE_NOT_ACCEPTED;

 char_t pomS[60];
 DPRINTF(_l("TffdshowVideoInputPin::CheckMediaType: %s, %i, %i"),fourcc2str(hdr2fourcc(hdr,&mt->subtype),pomS,60),hdr->biWidth,hdr->biHeight);
 
 return fv->getVideoCodecId(hdr,&mt->subtype,NULL)==CODEC_ID_NONE?VFW_E_TYPE_NOT_ACCEPTED:S_OK;
}

STDMETHODIMP TffdshowVideoInputPin::ReceiveConnection(IPin* pConnector, const AM_MEDIA_TYPE* pmt)
{
 CAutoLock cObjectLock(m_pLock);

 if (m_Connected)
  {
   CMediaType mt(*pmt);

   BITMAPINFOHEADER bih, bihCur;
   ExtractBIH(mt, &bih);
   ExtractBIH(m_mt, &bihCur);

   // HACK: for the intervideo filter, when it tries to change the pitch from 720 to 704...
   //if(bihCur.biWidth != bih.biWidth  && bihCur.biHeight == bih.biHeight)
   // return S_OK;
 
   return (CheckMediaType(&mt) != S_OK || SetMediaType(&mt) != S_OK/* || !initVideo(mt)*/)
           ? VFW_E_TYPE_NOT_ACCEPTED
           : S_OK;

   // TODO: send ReceiveConnection downstream
  }
 else
  {
   HRESULT hr=fv->deci->checkInputConnect(pConnector);
   if (hr!=S_OK) return hr;
  }
 return TinputPin::ReceiveConnection(pConnector, pmt);
}

bool TffdshowVideoInputPin::init(const CMediaType &mt)
{
 bool neroavc=false,truncated=false;
 if (mt.formattype==FORMAT_VideoInfo)
  {
   VIDEOINFOHEADER *vih=(VIDEOINFOHEADER*)mt.pbFormat;
   biIn.bmiHeader=vih->bmiHeader;
   pictIn.setSize(vih->bmiHeader.biWidth,abs(vih->bmiHeader.biHeight));
   fixMPEGinAVI(biIn.bmiHeader.biCompression);
  }
 else if (mt.formattype==FORMAT_VideoInfo2)
  {
   VIDEOINFOHEADER2 *vih2=(VIDEOINFOHEADER2*)mt.pbFormat;
   biIn.bmiHeader=vih2->bmiHeader;
   pictIn.setSize(vih2->bmiHeader.biWidth,abs(vih2->bmiHeader.biHeight));
   pictIn.setDar(Rational(vih2->dwPictAspectRatioX,vih2->dwPictAspectRatioY));
   DPRINTF(_l("TffdshowVideoInputPin::initVideo: darX:%i, darY:%i"),vih2->dwPictAspectRatioX,vih2->dwPictAspectRatioY);
   fixMPEGinAVI(biIn.bmiHeader.biCompression);
  }
 else if (mt.formattype==FORMAT_MPEGVideo)
  {
   MPEG1VIDEOINFO *mpeg1info=(MPEG1VIDEOINFO*)mt.pbFormat;
   biIn.bmiHeader=mpeg1info->hdr.bmiHeader;biIn.bmiHeader.biCompression=FOURCC_MPG1;
   pictIn.setSize(std::max(mpeg1info->hdr.rcSource.right,mpeg1info->hdr.bmiHeader.biWidth),std::max(mpeg1info->hdr.rcSource.bottom,mpeg1info->hdr.bmiHeader.biHeight));
  }
 else if (mt.formattype==FORMAT_MPEG2Video)
  {
   MPEG2VIDEOINFO *mpeg2info=(MPEG2VIDEOINFO*)mt.pbFormat;
   biIn.bmiHeader=mpeg2info->hdr.bmiHeader;
   pictIn.setSize(std::max(mpeg2info->hdr.rcSource.right,mpeg2info->hdr.bmiHeader.biWidth),std::max(mpeg2info->hdr.rcSource.bottom,mpeg2info->hdr.bmiHeader.biHeight));
   pictIn.setDar(Rational(mpeg2info->hdr.dwPictAspectRatioX,mpeg2info->hdr.dwPictAspectRatioY));
   if (biIn.bmiHeader.biCompression==0)
    biIn.bmiHeader.biCompression=FOURCC_MPG2;
   else
    {
     biIn.bmiHeader.biCompression=FCCupper(biIn.bmiHeader.biCompression);
     neroavc=true;
    } 
  }
 else if (mt.formattype==FORMAT_TheoraIll)
  {
   memset(&biIn,0,sizeof(biIn));
   sTheoraFormatBlock *oggFormat=(sTheoraFormatBlock*)mt.pbFormat;
   biIn.bmiHeader.biCompression=FOURCC_THEO;
   pictIn.setSize(biIn.bmiHeader.biWidth=oggFormat->width,biIn.bmiHeader.biHeight=oggFormat->height);
   pictIn.setDar(Rational(oggFormat->aspectNumerator,oggFormat->aspectDenominator));
   biIn.bmiHeader.biBitCount=12;
  } 
 else if (mt.formattype==FORMAT_RLTheora)
  {
   struct RLTheora 
    {
     VIDEOINFOHEADER hdr;
     DWORD headerSize[3];	// 0: Header, 1: Comment, 2: Codebook
    };
   const RLTheora *rl=(const RLTheora*)mt.pbFormat; 
   GetBitContext gb;
   init_get_bits(&gb,(const uint8_t*)(rl+1),rl->headerSize[0]);
   int ptype = get_bits(&gb, 8);
   if (!(ptype&0x80))
    return false;
   biIn.bmiHeader.biCompression=FOURCC_THEO; 
   skip_bits(&gb, 6*8); /* "theora" */
   int major=get_bits(&gb,8); /* version major */
   int minor=get_bits(&gb,8); /* version minor */
   int micro=get_bits(&gb,8); /* version micro */
   int theora = (major << 16) | (minor << 8) | micro;

   if (theora < 0x030200)
    {
     ;//flipped_image = 1;
    }
    
    biIn.bmiHeader.biWidth = get_bits(&gb, 16) << 4;
    biIn.bmiHeader.biHeight = get_bits(&gb, 16) << 4;
    pictIn.setSize(biIn.bmiHeader.biWidth,biIn.bmiHeader.biHeight);
    
    skip_bits(&gb, 24); /* frame width */
    skip_bits(&gb, 24); /* frame height */

    skip_bits(&gb, 8); /* offset x */
    skip_bits(&gb, 8); /* offset y */

    skip_bits(&gb, 32); /* fps numerator */
    skip_bits(&gb, 32); /* fps denumerator */

    Rational sample_aspect_ratio;
    sample_aspect_ratio.num = get_bits(&gb, 24); /* aspect numerator */
    sample_aspect_ratio.den = get_bits(&gb, 24); /* aspect denumerator */
    pictIn.setSar(sample_aspect_ratio);
  } 
 else
  return false;

 REFERENCE_TIME avgTimePerFrame0=getAvgTimePerFrame(mt);
 avgTimePerFrame=avgTimePerFrame0?avgTimePerFrame0:400000;

 char_t pomS[60];
 DPRINTF(_l("TffdshowVideoInputPin::initVideo: %s, width:%i, height:%i, aspectX:%i, aspectY:%i"),fourcc2str(hdr2fourcc(&biIn.bmiHeader,&mt.subtype),pomS,60) ,pictIn.rectFull.dx,pictIn.rectFull.dy,pictIn.rectFull.dar().num,pictIn.rectFull.dar().den);
again:
 if (video) {delete video;codec=video=NULL;}
 codecId=(CodecID)fv->getVideoCodecId(&biIn.bmiHeader,&mt.subtype,&biIn.bmiHeader.biCompression);
 if (codecId==CODEC_ID_NONE) return false;
 
 if (codecId==CODEC_ID_H264)
  {
   Textradata extradata(mt,16);
   if (extradata.size)
    decodeH264SPS(extradata.data,extradata.size,pictIn);
  }
 if (mpeg4_codec(codecId))
  {
   Textradata extradata(mt,16);
   if (extradata.size)
    decodeMPEG4pictureHeader(extradata.data,extradata.size,pictIn);
  } 
 else if (mpeg12_codec(codecId))
  {
   Textradata extradata(mt,16);
   if (extradata.size)
    {
     bool isH264;
     if (decodeMPEGsequenceHeader(biIn.bmiHeader.biCompression==FOURCC_MPG2,extradata.data,extradata.size,pictIn,&isH264) && isH264)
      {
       biIn.bmiHeader.biCompression=FOURCC_H264;
       truncated=true;
       goto again;
      }
    } 
  }
  
 if (!fv->sink)
  rawDecode=true;
 else
  {
   fv->initCodecSettings();
   codec=video=TvideoCodecDec::initDec(fv->deci,fv->sink,codecId,biIn.bmiHeader.biCompression,mt);
   if (!video) 
    return false;
   else 
    {
     static const GUID CLSID_NeroDigitalParser={0xE206E4DE,0xA7EE,0x4A62,0xB3,0xE9,0x4F,0xBC,0x8F,0xE8,0x4C,0x73};
     static const GUID CLSID_HalliMatroskaFile={0x55DA30FC,0xF16B,0x49FC,0xBA,0xA5,0xAE,0x59,0xFC,0x65,0xF8,0x2D};
     //neroavc=biIn.bmiHeader.biCompression==FOURCC_AVC1 && (searchPreviousFilter(this,CLSID_NeroDigitalParser) || searchPreviousFilter(this,CLSID_HalliMatroskaFile));
     if (!video->beginDecompress(pictIn,biIn.bmiHeader.biCompression,mt,(neroavc?TvideoCodecDec::SOURCE_NEROAVC:0)|(truncated?TvideoCodecDec::SOURCE_TRUNCATED:0)))
      {
       delete video;codec=video=NULL;
       return false;
      }
    }  
   rawDecode=raw_codec(codecId);
  }
 allocator.NotifyMediaType(mt);
 strippacket=!!(mt.majortype==MEDIATYPE_DVD_ENCRYPTED_PACK);
 return true;
}

void TffdshowVideoInputPin::done(void)
{
 if (video) delete video;codec=video=NULL;
 memset(&biIn,0,sizeof(biIn));
 avgTimePerFrame=0;
 codecId=CODEC_ID_NONE;rawDecode=false;
 autosubflnm[0]=oldSubSearchDir[0]='\0';oldSubHeuristic=false;
}

STDMETHODIMP TffdshowVideoInputPin::GetAllocator(IMemAllocator** ppAllocator)
{
 if (!raw_codec(codecId)) 
  return TinputPin::GetAllocator(ppAllocator);
 else
  { 
   CheckPointer(ppAllocator, E_POINTER);
   if (m_pAllocator==NULL)
    {
     m_pAllocator=&allocator;
     m_pAllocator->AddRef();
    }
   m_pAllocator->AddRef();
   *ppAllocator=m_pAllocator;
   return NOERROR;
  } 
}
STDMETHODIMP TffdshowVideoInputPin::NotifyAllocator(IMemAllocator *pAllocator,BOOL bReadOnly)
{
 HRESULT hr=TinputPin::NotifyAllocator(pAllocator,bReadOnly);
 if (FAILED(hr)) return hr;
 usingOwnAllocator=(pAllocator==(IMemAllocator*)&allocator);
 return S_OK;
}

STDMETHODIMP TffdshowVideoInputPin::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
 fv->lockReceive();
 HRESULT hr=TinputPin::NewSegment(tStart,tStop,dRate);
 fv->unlockReceive(); 
 return hr;
}

STDMETHODIMP TffdshowVideoInputPin::Receive(IMediaSample* pSample)
{
 AM_MEDIA_TYPE *pmt=NULL;
 if (SUCCEEDED(pSample->GetMediaType(&pmt)) && pmt)
  {
   CMediaType mt(*pmt);
   SetMediaType(&mt);
   allocator.mtChanged=false;
   DeleteMediaType(pmt);
  }
 return TinputPin::Receive(pSample);
}

HRESULT TffdshowVideoInputPin::decompress(IMediaSample *pSample,long *srcLen)
{
 BYTE *bitstream;
 if (pSample->GetPointer(&bitstream)!=S_OK) 
  { 
   *srcLen=-1;
   return S_FALSE;
  } 
 *srcLen=pSample->GetActualDataLength();
 if (bitstream && strippacket)
  StripPacket(bitstream,*srcLen);
 return video->decompress(bitstream,*srcLen,pSample);
}

HRESULT TffdshowVideoInputPin::getAVIfps(unsigned int *fps1000)
{
 if (!fps1000 || avgTimePerFrame==0) return S_FALSE;
 
 *fps1000=(unsigned int)(REF_SECOND_MULT*1000/avgTimePerFrame);
 return S_OK;
}
HRESULT TffdshowVideoInputPin::getAverageTimePerFrame(int64_t *avg)
{
 if (IsConnected() && m_mt.cbFormat)
  *avg=getAvgTimePerFrame(m_mt);
 else
  *avg=avgTimePerFrame;
 return S_OK;
}

HRESULT TffdshowVideoInputPin::getAVIdimensions(unsigned int *x,unsigned int *y)
{
 if (!x || !y) return E_POINTER;
 *x=pictIn.rectFull.dx;*y=pictIn.rectFull.dy;
 return (pictIn.rectFull.dx==0 || pictIn.rectFull.dy==0)?S_FALSE:S_OK;
}
HRESULT TffdshowVideoInputPin::getInputSAR(unsigned int *a1,unsigned int *a2)
{
 if (!a1 || !a2) return E_POINTER;
 *a1=pictIn.rectFull.sar.num;*a2=pictIn.rectFull.sar.den;
 return *a1 && *a2?S_OK:S_FALSE;
}
HRESULT TffdshowVideoInputPin::getInputDAR(unsigned int *a1,unsigned int *a2)
{
 if (!a1 || !a2) return E_POINTER;
 *a1=pictIn.rectFull.dar().num;*a2=pictIn.rectFull.dar().den;
 return *a1 && *a2?S_OK:S_FALSE;
}

HRESULT TffdshowVideoInputPin::getMovieSource(const TvideoCodecDec* *moviePtr)
{
 if (!moviePtr) return S_FALSE;
 *moviePtr=video;
 return S_OK;
}
FOURCC TffdshowVideoInputPin::getMovieFOURCC(void)
{
 return codecId!=CODEC_ID_NONE?biIn.bmiHeader.biCompression:0;
}

HRESULT TffdshowVideoInputPin::getFrameTime(unsigned int framenum,unsigned int *sec)
{
 if (!sec) return E_POINTER;
 if (avgTimePerFrame==0) return E_FAIL;
 *sec=(unsigned int)(avgTimePerFrame*framenum/REF_SECOND_MULT);
 return S_OK;
}
HRESULT TffdshowVideoInputPin::getFrameTimeMS(unsigned int framenum,unsigned int *msec)
{
 if (!msec) return E_POINTER;
 if (avgTimePerFrame==0) return E_FAIL;
 *msec=(unsigned int)(avgTimePerFrame*framenum/10000);
 return S_OK;
}
HRESULT TffdshowVideoInputPin::calcMeanQuant(float *quant)
{
 if (!quant) return E_POINTER;
 if (!video) return S_FALSE;
 *quant=video->calcMeanQuant();
 return S_OK;
}
HRESULT TffdshowVideoInputPin::quantsAvailable(void)
{
 if (!video) return E_FAIL;
 return video->quants?S_OK:S_FALSE;
}
HRESULT TffdshowVideoInputPin::getQuantMatrices(uint8_t intra8[64],uint8_t inter8[64],uint8_t intra4luma[16],uint8_t intra4chroma[16],uint8_t inter4luma[16],uint8_t inter4chroma[16])
{
 if (!intra8 || !inter8) return E_POINTER;
 if (!video) return E_FAIL;
 if ((!video->inter_matrix && !video->intra_matrix) || (video->intra_matrix[0]==0 && video->inter_matrix[0]==0)) return E_UNEXPECTED;
 if (video->inter_matrix)
  for (int i=0;i<64;i++)
   inter8[i]=(uint8_t)video->inter_matrix[i];
 else
   memset(inter8,0,64); 
 if (video->intra_matrix)
  for (int i=0;i<64;i++)
   intra8[i]=(uint8_t)video->intra_matrix[i];
 else
   memset(intra8,0,64); 
 if (inter4luma)  
  if (video->inter_matrix_luma)
   for (int i=0;i<16;i++)  
    inter4luma[i]=(uint8_t)video->inter_matrix_luma[i];
  else 
   memset(inter4luma,0,16);
 if (inter4chroma)  
  if (video->inter_matrix_chroma)
   for (int i=0;i<16;i++)  
    inter4chroma[i]=(uint8_t)video->inter_matrix_chroma[i];
  else 
   memset(inter4chroma,0,16);
 if (intra4luma)  
  if (video->intra_matrix_luma)
   for (int i=0;i<16;i++)  
    intra4luma[i]=(uint8_t)video->intra_matrix_luma[i];
  else 
   memset(intra4luma,0,16);
 if (intra4chroma)  
  if (video->intra_matrix_chroma)
   for (int i=0;i<16;i++)  
    intra4chroma[i]=(uint8_t)video->intra_matrix_chroma[i];
  else 
   memset(intra4chroma,0,16);
 return S_OK;  
}

HRESULT TffdshowVideoInputPin::getInCodecString(char_t *buf,size_t buflen)
{
 if (!buf) return E_POINTER;
 if (video)
  {
   char_t name[60];
   tsnprintf(buf,buflen,_l("%s (%s)"),fourcc2str(biIn.bmiHeader.biCompression,name,60),video->getName());
   buf[buflen-1]='\0';
  } 
 else
  buf[0]='\0';
 return S_OK;
}
bool TffdshowVideoInputPin::waitForKeyframes(void)
{
 return !rawDecode && !(video && mpeg12_codec(codecId) && biIn.bmiHeader.biCompression!=FOURCC_MPEG);
}
void TffdshowVideoInputPin::setSampleSkipped(void)
{
 if (video)
  video->onDiscontinuity();
}

const char_t* TffdshowVideoInputPin::findAutoSubflnm(IcheckSubtitle *checkSubtitle,const char_t *searchDir,const char_t *searchExt,bool heuristic)
{
 if (IsConnected()==FALSE) return _l("");
 const char_t *AVIname=getFileSourceName();
 if (AVIname[0]=='\0') return _l("");
 if (autosubflnm[0]=='\0' || oldSubHeuristic!=heuristic || stricmp(oldSubSearchDir,searchDir)!=0)
  {
   oldSubHeuristic=heuristic;strcpy(oldSubSearchDir,searchDir);
   TsubtitlesFile::findSubtitlesFile(AVIname,searchDir,searchExt,autosubflnm,MAX_PATH,heuristic,checkSubtitle);
  } 
 return autosubflnm;
}

//================================ TffdshowVideoEncInputPin ================================
STDMETHODIMP TffdshowVideoEncInputPin::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
 if (riid==IID_IMixerPinConfig)
  {
   isOverlay=true;
   return GetInterface<IMixerPinConfig>(this,ppv); 
  } 
 else  
  return TffdshowVideoInputPin::NonDelegatingQueryInterface(riid, ppv);
}

STDMETHODIMP TffdshowVideoEncInputPin::SetRelativePosition(THIS_ IN DWORD dwLeft, IN DWORD dwTop, IN DWORD dwRight, IN DWORD dwBottom)
{
 DPRINTF(_l(" SetRelativePosition"));
 return S_OK;
}    
STDMETHODIMP TffdshowVideoEncInputPin::GetRelativePosition(THIS_ OUT DWORD *pdwLeft,OUT DWORD *pdwTop,OUT DWORD *pdwRight,OUT DWORD *pdwBottom)
{
 DPRINTF(_l(" GetRelativePosition"));
 return S_OK;
}    
STDMETHODIMP TffdshowVideoEncInputPin::SetZOrder(THIS_ IN DWORD dwZOrder)
{
 DPRINTF(_l(" SetZOrder"));
 return S_OK;
}    
STDMETHODIMP TffdshowVideoEncInputPin::GetZOrder(THIS_ OUT DWORD *pdwZOrder)
{
 DPRINTF(_l(" GetZOrder"));
 return S_OK;
}    
STDMETHODIMP TffdshowVideoEncInputPin::SetColorKey(THIS_ IN COLORKEY *pColorKey)
{
 DPRINTF(_l(" SetColorKey"));
 return S_OK;
}    
STDMETHODIMP TffdshowVideoEncInputPin::GetColorKey(THIS_ OUT COLORKEY *pColorKey,OUT DWORD *pColor)
{
 DPRINTF(_l(" GetColorKey"));
 return S_OK;
}    
STDMETHODIMP TffdshowVideoEncInputPin::SetBlendingParameter(THIS_ IN DWORD dwBlendingParameter)
{
 DPRINTF(_l(" SetBlendingParameter"));
 return S_OK;
}    
STDMETHODIMP TffdshowVideoEncInputPin::GetBlendingParameter(THIS_ OUT DWORD *pdwBlendingParameter)
{
 DPRINTF(_l(" GetBlendingParameter"));
 return S_OK;
}    
STDMETHODIMP TffdshowVideoEncInputPin::SetAspectRatioMode(THIS_ IN AM_ASPECT_RATIO_MODE amAspectRatioMode)
{
 DPRINTF(_l(" SetAspectRatioMode"));
 return S_OK;
}    
STDMETHODIMP TffdshowVideoEncInputPin::GetAspectRatioMode(THIS_ OUT AM_ASPECT_RATIO_MODE* pamAspectRatioMode)
{
 DPRINTF(_l(" GetAspectRatioMode"));
 return S_OK;
}    
STDMETHODIMP TffdshowVideoEncInputPin::SetStreamTransparent(THIS_ IN BOOL bStreamTransparent)
{
 DPRINTF(_l(" SetStreamTransparent"));
 return S_OK;
}    
STDMETHODIMP TffdshowVideoEncInputPin::GetStreamTransparent(THIS_ OUT BOOL *pbStreamTransparent)
{
 DPRINTF(_l(" GetStreamTransparent"));
 return S_OK;
}    
