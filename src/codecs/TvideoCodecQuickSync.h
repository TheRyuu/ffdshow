#ifndef _TVIDEOCODECQUICKSYNC_H_
#define _TVIDEOCODECQUICKSYNC_H_

#include "TvideoCodec.h"

// Forward declarations
class Tdll;
struct IQuickSyncDecoder;
struct QsFrameData;

class TvideoCodecQuickSync : public TvideoCodecDec
{
public:
    TvideoCodecQuickSync(IffdshowBase *Ideci,IdecVideoSink *IsinkD, int codecID);
    virtual ~TvideoCodecQuickSync();

    virtual int getType() const { return IDFF_MOVIE_QUICK_SYNC; }
    virtual const char_t* getName() const;
    static bool check(Tconfig* config);
    static const char_t *dllname;

protected:
    virtual bool beginDecompress(TffPictBase &pict,FOURCC infcc,const CMediaType &mt,int sourceFlags);
    virtual HRESULT decompress(const unsigned char *src,size_t srcLen,IMediaSample *pIn);
    virtual HRESULT BeginFlush();
    virtual HRESULT EndFlush();
    virtual HRESULT onEndOfStream();
    virtual bool onDiscontinuity();
    virtual bool testMediaType(FOURCC fcc,const CMediaType &mt);
    virtual void setOutputPin(IPin *pPin);
    virtual bool onSeek(REFERENCE_TIME segmentStart);
    HRESULT DeliverSurface(QsFrameData* frameData);
    static HRESULT DeliverSurfaceCallback(void* obj, QsFrameData* frameData);

    // C function pointers (factory functions)
    IQuickSyncDecoder* (__stdcall *createQuickSync)();
    void (__stdcall *destroyQuickSync)(IQuickSyncDecoder*);

    CCritSec            m_csLock;
    Tdll*               m_Dll;
    IQuickSyncDecoder*  m_QuickSync;
    CMemAllocator       m_FakeAllocator;
    CMediaSample        m_MediaSample;
};

#endif
