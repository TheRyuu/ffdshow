#ifndef _TVIDEOCODECLIBAVCODECDXVA_H_
#define _TVIDEOCODECLIBAVCODECDXVA_H_

#include <atlbase.h>
#include <dxva.h>
#include <dxva2api.h>
#include <streams.h>
#include <dvdmedia.h>
#include <d3dx9.h>
#include <evr.h>
#include <mfapi.h>
#include <videoacc.h>
#include "TDXVADecoder.h"

enum PCI_Vendors {
    PCIV_ATI         = 0x1002,
    PCIV_nVidia      = 0x10DE,
    PCIV_Intel       = 0x8086,
    PCIV_S3_Graphics = 0x5333
};

// Bitmasks for DXVA compatibility check
#define DXVA_UNSUPPORTED_LEVEL   1
#define DXVA_TOO_MUCH_REF_FRAMES 2
#define DXVA_INCOMPATIBLE_SAR    4

typedef enum {
    MODE_SOFTWARE,
    MODE_DXVA1,
    MODE_DXVA2
} DXVA_MODE;

#define MAX_SUPPORTED_MODE 5
#define ROUND_FRAMERATE(var,FrameRate) if (labs ((long)(var - FrameRate)) < FrameRate*1/100) var = FrameRate;
typedef struct TDXVA_PARAMS {
    const int   PicEntryNumber;
    const UINT  PreferedConfigBitstream;
    const GUID* Decoder[MAX_SUPPORTED_MODE];
    const WORD  RestrictedMode[MAX_SUPPORTED_MODE];
} DXVA_PARAMS;

struct Textradata;
class TccDecoder;
struct TlibavcodecExt;

class TvideoCodecLibavcodec;
class TvideoCodecLibavcodecDxva : public TvideoCodecLibavcodec
{
    friend class TffdshowDecVideoAllocatorDXVA;
    friend class TDXVADecoder;
    friend class TDXVADecoderH264;
    friend class TDXVADecoderVC1;
private:
    int nPCIVendor;
    int nPCIDevice;
    ffstring strDeviceDescription;
    LARGE_INTEGER videoDriverVersion;
    bool isDXVACompatible;
    DXVA_PARAMS *dxvaParamsp;
    virtual void detectVideoCard(HWND hwnd);
    GUID dxvaDecoderGUID;
    DXVA_MODE nDXVAMode;
    TDXVADecoder* pDXVADecoder;
    CodecID dxvaCodecId;
    bool initDXVAMode;

    // DXVA settings
    int nARMode; // aspect ratio mode
    AVRational sar; // aspect ratio

    // === DXVA1 variables
    DDPIXELFORMAT                           pixelFormat;

    // === DXVA2 variables
    CComPtr<IDirect3DDeviceManager9>        m_pDeviceManager;
    CComPtr<IDirectXVideoDecoderService>    m_pDecoderService;
    CComPtr<IDirect3DSurface9>              pDecoderRenderTarget;
    DXVA2_ConfigPictureDecode               dxva2Config;
    HANDLE hDevice;
    DXVA2_VideoDesc videoDesc;
    virtual void cleanup();

protected:

    // TvideoCodecLibavcodec overloaded methods
    virtual void create(void);
    virtual bool beginDecompress(TffPictBase &pict, FOURCC fcc, const CMediaType &mt, int sourceFlags);
    virtual HRESULT decompress(const unsigned char *src, size_t srcLen0, IMediaSample *pIn);

    // TvideoCodecLibavcodecDXVA methods
    virtual bool isVista(void);
    virtual REFERENCE_TIME getAvrTimePerFrame(void);
    virtual void PostProcessUSWCFrame(void * PostProcessUSWCBuffer, UINT pitch);
    virtual void GetProcessedFrame(Tbuffer &processedFrame, UINT width, UINT height);
    virtual void OverlayYV12OnUSWCFrame(unsigned char * pSrc, unsigned char * pDest, UINT width, UINT height, UINT pitch);

public:
    TvideoCodecLibavcodecDxva(IffdshowBase *Ideci, IdecVideoSink *IsinkD, CodecID IcodecId);
    virtual ~TvideoCodecLibavcodecDxva();

    //TvideoCodecDec overloaded methods
    virtual bool onSeek(REFERENCE_TIME segmentStart);
    virtual int useDXVA(void);
    virtual int getType(void) const {
        return IDFF_MOVIE_FFMPEG_DXVA;
    }
    const char_t* getName(void) const;

    //TvideoCodecLibavcodecDxva methods
    virtual void getDXVAOutputFormats(TcspInfos &ocsps);
    virtual bool checkDXVAMode(IPin *pReceivePin);
    virtual STDMETHODIMP_(GUID*) getDXVADecoderGUID(void);
    virtual int getPicEntryNumber(void);
    virtual BOOL isSupportedDecoderConfig(const D3DFORMAT nD3DFormat, const DXVA2_ConfigPictureDecode& config, bool& bIsPrefered);
    virtual HRESULT configureDXVA2(IPin *pPin);
    virtual HRESULT setEVRForDXVA2(IPin *pPin);
    virtual void fillInVideoDescription(DXVA2_VideoDesc *pDesc);
    virtual HRESULT findDXVA2DecoderConfiguration(IDirectXVideoDecoderService *pDecoderService, const GUID& guidDecoder, DXVA2_ConfigPictureDecode *pSelectedConfig, BOOL *pbFoundDXVA2Configuration);
    virtual HRESULT createDXVA2Decoder(UINT nNumRenderTargets, IDirect3DSurface9** pDecoderRenderTargets);
    virtual HRESULT findDXVA1DecoderConfiguration(IAMVideoAccelerator* pAMVideoAccelerator, const GUID* guidDecoder, DDPIXELFORMAT* pPixelFormat);
    virtual HRESULT checkDXVA1Decoder(const GUID *pGuid);
    virtual void setDXVA1Params(const GUID* pGuid, DDPIXELFORMAT* pPixelFormat);
    virtual WORD getDXVA1RestrictedMode();
    virtual HRESULT createDXVA1Decoder(IAMVideoAccelerator*  pAMVideoAccelerator, const GUID* pDecoderGuid, DWORD dwSurfaceCount);
    virtual UINT getAdapter(IDirect3D9* pD3D, HWND hWnd);
    void flushDXVADecoder(void) {
        if (pDXVADecoder) {
            // fixme: a crash occurs here when removing ffdshow from the filter graph in GraphStudio
            // only happens when the graph was constructed automatically and EVR is used (due to manually set high merit)
            pDXVADecoder->Flush();
        }
    }

    virtual bool isDXVASupported();
    virtual int pictWidthRounded();
    virtual int pictHeightRounded();
    DDPIXELFORMAT* getPixelFormat() {
        return &pixelFormat;
    };
    int getPCIVendor() {
        return nPCIVendor;
    };
    virtual void updateAspectRatio(void);
    DXVA2_ConfigPictureDecode* getDXVA2Config(void) {
        return &dxva2Config;
    };
    virtual bool isPostProcessingEnabled(void);

};

#endif
