#ifndef _TAUDIOCODEC_H_
#define _TAUDIOCODEC_H_

#include "Tcodec.h"
#include "ffdshow_constants.h"
#include "TsampleFormat.h"

class TaudioParser;

DECLARE_INTERFACE_(IdecAudioSink, IdecSink)
{
    STDMETHOD(deliverDecodedSample)(void * buf, size_t numsamples, const TsampleFormat & fmt) PURE;
    STDMETHOD(setCodecId)(AVCodecID codecId) PURE;
    STDMETHOD(getCodecId)(AVCodecID * pCodecId) PURE;
    STDMETHOD(getAudioParser)(TaudioParser **ppAudioParser) PURE;
    STDMETHOD(deliverProcessedSample)(const void * buf, size_t numsamples, const TsampleFormat & outsf0) PURE;
};

class TaudioCodec : public TcodecDec
{
private:
    static TaudioCodec* getDecLib(AVCodecID codecId, IffdshowBase *deci, IdecAudioSink *sink);
    Tbuffer buf;
protected:
    comptrQ<IffdshowDecAudio> deciA;
    IdecAudioSink *sinkA;
    TsampleFormat fmt;
    virtual bool init(const CMediaType &mt) = 0;
    void* getDst(size_t needed);
    virtual void getInputDescr1(char_t *buf, size_t buflen) const = 0;
    unsigned int bpssum, numframes, lastbps;
    Tbuffer srcBuf;
    size_t buflen;
public:
    TaudioCodec(IffdshowBase *Ideci, IdecAudioSink *Isink);
    static TaudioCodec* initSource(IffdshowBase *Ideci, IdecAudioSink *sink, AVCodecID codecId, const TsampleFormat &fmt, const CMediaType &mt);
    const TsampleFormat& getInputSF(void) const {
        return fmt;
    }
    unsigned int getLastbps(void) const {
        return lastbps;
    }

    void getInputDescr(char_t *buf, size_t buflen) const;
    virtual HRESULT decode(TbyteBuffer &src) = 0;
};

#endif
