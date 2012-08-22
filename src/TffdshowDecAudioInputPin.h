#ifndef _TFFDSHOWDECAUDIOINPUTPIN_H_
#define _TFFDSHOWDECAUDIOINPUTPIN_H_

#include "TinputPin.h"
#include "TaudioCodec.h"
#include "TaudioParser.h"

class TffdshowDecAudio;
class TffdshowDecAudioInputPin : public TinputPin, public IdecAudioSink, public IPinConnection
{
private:
    TffdshowDecAudio *filter;
    bool searchdts;
    CCritSec m_csReceive;
    TbyteBuffer buf, newSrcBuffer;
    int jitter;
    TaudioParser *audioParser;
    CMediaType outmt;
    HANDLE m_hNotifyEvent;
    CAMEvent m_evBlock;
    bool m_useBlock;
protected:
    virtual bool init(const CMediaType &mt);
    virtual void done(void);
public:

    DECLARE_IUNKNOWN

    REFERENCE_TIME insample_rtStart, insample_rtStop;
    TffdshowDecAudioInputPin(const char_t* pObjectName, TffdshowDecAudio* pFilter, HRESULT* phr, LPWSTR pName, int Inumber);
    virtual ~TffdshowDecAudioInputPin();
    TaudioCodec *audio;
    void block(bool is);
    int number;

    bool is_spdif_codec(void) const {
        return  audio && spdif_codec(audio->codecId);
    }
    bool isActive();

    // IPin
    STDMETHODIMP Receive(IMediaSample* pSample);
    STDMETHODIMP EndFlush(void);
    STDMETHODIMP BeginFlush();
    STDMETHODIMP EndOfStream();
    STDMETHODIMP NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);
    HRESULT CompleteConnect(IPin* pReceivePin);

    HRESULT Active(void);
    HRESULT Inactive(void);




    // IPinConnection
    STDMETHODIMP DynamicQueryAccept(const AM_MEDIA_TYPE* pmt);
    STDMETHODIMP NotifyEndOfStream(HANDLE hNotifyEvent);
    STDMETHODIMP IsEndPin();
    STDMETHODIMP DynamicDisconnect();

    // IdecAudioSink
    STDMETHODIMP deliverDecodedSample(void *buf, size_t numsamples, const TsampleFormat &fmt);
    STDMETHODIMP flushDecodedSamples(void);
    STDMETHODIMP setCodecId(AVCodecID codecId);
    STDMETHODIMP getCodecId(AVCodecID *pCodecId);
    STDMETHODIMP getAudioParser(TaudioParser **ppAudioParser);
    STDMETHODIMP deliverProcessedSample(const void *buf, size_t numsamples, const TsampleFormat &fmt);
    STDMETHODIMP_(bool)getsf(TsampleFormat &sf); //true if S/PDIF or bitstream HDMI

    HRESULT getMovieSource(const TaudioCodec* *moviePtr);
    virtual HRESULT getInCodecString(char_t *buf, size_t buflen);
    int getInputBitrate(void) const;
    int getJitter(void) const;
};

#endif
