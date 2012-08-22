#ifndef _TVIDEOCODEC_H_
#define _TVIDEOCODEC_H_

#include "Tcodec.h"
#include "TffPict.h"
#include "ffdshow_constants.h"
#include "ffImgfmt.h"
#include "TencStats.h"
#include "interfaces.h"

DECLARE_INTERFACE_(IdecVideoSink, IdecSink)
{
    STDMETHOD(deliverDecodedSample)(TffPict & pict) PURE;
    STDMETHOD(deliverPreroll)(int frametype) PURE;
    STDMETHOD(acceptsManyFrames)(void) PURE;
};

class TvideoCodec : virtual public Tcodec
{
public:
    TvideoCodec(IffdshowBase *Ideci);
    virtual ~TvideoCodec();
    bool ok;
    int connectedSplitter;
    bool isInterlacedRawVideo;
    Rational containerSar;

    struct CAPS {
        enum {
            NONE = 0,
            VIS_MV = 1,
            VIS_QUANTS = 2
        };
    };

    virtual void end(void) {}
};

class TvideoCodecDec : virtual public TvideoCodec, virtual public TcodecDec
{
protected:
    bool isdvdproc;
    comptrQ<IffdshowDecVideo> deciV;
    IdecVideoSink *sinkD;
    TvideoCodecDec(IffdshowBase *Ideci, IdecVideoSink *Isink);

    /* guessMPEG2sar
     * Regarding MPEG-2, currently there are two ways of encoding SAR.
     * Of course one is wrong. However, considerable number of videos are encoded in a wrong way.
     * Here we try to guess which is crrect.
     *
     * Input:
     *  r: rect and SAR decoded using the spec
     *  sar2: SAR decoded using the wrong spec
     *  containerSar: container sar
     *
     * Algorithm
     *  DVD
     *   containerSAR seems to give the best result.
     *  non DVD:
     *   1. r.sar is tested for 4:3, 16:9, containerSar. If it matches with any of them, it's OK.
     *   2. sar2 is tested for 4:3, 16:9, containerSar. If it matches with any of them, sar2 will be used.
     *   3. containerSar is tested for 4:3, 16:9. If it matches with either of the two, containerSar will be used.
     *   4. Still no match? r.sar is used.
     */
    Rational guessMPEG2sar(const Trect &r, const Rational &sar2, const Rational &containerSar);

    class TtelecineManager
    {
    private:
        TvideoCodecDec* parent;
        int segment_count;
        int pos_in_group;
        struct {
            int fieldtype;
            int repeat_pict;
            REFERENCE_TIME rtStart;
        } group[2]; // store information about 2 recent frames.
        REFERENCE_TIME group_rtStart;
        bool film;
        int cfg_softTelecine;
    public:
        TtelecineManager(TvideoCodecDec* Iparent);
        void get_timestamps(TffPict &pict);
        void get_fieldtype(TffPict &pict);
        void new_frame(int top_field_first, int repeat_pict, const REFERENCE_TIME &rtStart, const REFERENCE_TIME &rtStop);
        void onSeek(void);
    } telecineManager;

public:
    static TvideoCodecDec* initDec(IffdshowBase *deci, IdecVideoSink *Isink, AVCodecID codecId, FOURCC fcc, const CMediaType &mt);

    virtual ~TvideoCodecDec();

    virtual int caps(void) const {
        return CAPS::NONE;
    }
    virtual bool testMediaType(FOURCC fcc, const CMediaType &mt) {
        return true;
    }
    virtual void forceOutputColorspace(const BITMAPINFOHEADER *hdr, int *ilace, TcspInfos &forcedCsps) {
        *ilace = 0; //cspInfos of forced output colorspace, empty when entering function
    }
    enum {SOURCE_REORDER = 1};
    virtual bool beginDecompress(TffPictBase &pict, FOURCC infcc, const CMediaType &mt, int sourceFlags) = 0;
    virtual HRESULT decompress(const unsigned char *src, size_t srcLen, IMediaSample *pIn) = 0;
    virtual bool onDiscontinuity(void) {
        return false;
    }
    virtual HRESULT onEndOfStream(void) {
        return S_OK;
    }

    unsigned int quantsDx, quantsStride, quantsDy, quantBytes, quantType;
    void *quants;
    uint16_t *intra_matrix, *inter_matrix;

    float calcMeanQuant(void);
    virtual bool drawMV(unsigned char *dst, unsigned int dx, stride_t stride, unsigned int dy) const {
        return false;
    }
    virtual const char* get_current_idct(void) {
        return NULL;
    }
    virtual int useDXVA(void) {
        return 0;
    };

    virtual void setOutputPin(IPin * /*pPin*/) {}
};

typedef vectorEx<uint64_t, array_allocator<uint64_t, FF_CSPS_NUM> > Tcsps;
typedef vectorEx<FOURCC> Tfourccs;

struct Tencoder {
    Tencoder(const char_t *Iname, AVCodecID Iid, const Tfourccs &Ifourccs): name(Iname), id(Iid), fourccs(Ifourccs) {}
    Tencoder(const char_t *Iname, AVCodecID Iid): name(Iname), id(Iid) {
        const FOURCC *fccs = getCodecFOURCCs(id);
        for (const FOURCC *fcc = fccs; *fcc; fcc++) {
            fourccs.push_back(*fcc);
        }
    }

    const char_t *name;
    AVCodecID id;
    Tfourccs fourccs;
};

struct Tencoders : public std::vector<Tencoder*> {
    ~Tencoders() {
        for (iterator e = begin(); e != end(); e++) {
            delete *e;
        }
    }
};

struct TmediaSample {
private:
    IMediaSample *sample;
public:
    TmediaSample(void): sample(NULL) {}
    TmediaSample(IMediaSample *Isample): sample(Isample) {}
    ~TmediaSample() {
        if (sample) {
            sample->Release();
        }
    }
    IMediaSample** operator &(void) {
        return &sample;
    }
    IMediaSample* operator ->(void) const {
        return sample;
    }
    template<class T> operator T* (void) {
        BYTE *ptr;
        return sample && sample->GetPointer(&ptr) == S_OK ? ptr : NULL;
    }
    template<class T> operator const T* (void) const {
        BYTE *ptr;
        return sample && sample->GetPointer(&ptr) == S_OK ? ptr : NULL;
    }
    long size(void) {
        return sample ? sample->GetSize() : 0;
    }
};

DECLARE_INTERFACE(IencVideoSink)
{
    STDMETHOD(getDstBuffer)(IMediaSample* *samplePtr, const TffPict & pict) PURE;
    STDMETHOD(deliverEncodedSample)(const TmediaSample & sample, TencFrameParams & params) PURE;
    STDMETHOD(deliverError)(void) PURE;
};

struct IffdshowEnc;
struct TcoSettings;
class TvideoCodecEnc : virtual public TvideoCodec
{
protected:
    comptrQ<IffdshowEnc> deciE;
    IencVideoSink *sinkE;
    const TcoSettings *coCfg;
public:
    TvideoCodecEnc(IffdshowBase *Ideci, IencVideoSink *Isink);
    virtual ~TvideoCodecEnc();
    Tencoders encoders;
    void setCoSettings(AVCodecID IcodecId) {
        codecId = IcodecId;
    }
    virtual void getCompressColorspaces(Tcsps &csps, unsigned int outDx, unsigned int outDy) {
        csps.add(FF_CSP_420P);
    }
    virtual bool prepareHeader(BITMAPINFOHEADER *outhdr) {
        return false;
    }
    virtual LRESULT beginCompress(int cfgcomode, uint64_t csp, const Trect &r) = 0;
    virtual bool supExtradata(void) {
        return false;
    }
    virtual bool getExtradata(const void* *ptr, size_t *len);
    virtual HRESULT compress(const TffPict &pict, TencFrameParams &params) = 0;
    virtual HRESULT flushEnc(const TffPict &pict, TencFrameParams &params) {
        return S_OK;
    }
};

struct TvideoCodecs : public std::vector<TvideoCodecEnc*> {
    void init(IffdshowBase *deci, IencVideoSink *sink);
    TvideoCodecEnc* getEncLib(int codecId);
    const Tencoder* getEncoder(int codecId) const;
};

#endif
