#ifndef _TAUDIOCODECLIBAVCODEC_H_
#define _TAUDIOCODECLIBAVCODEC_H_

#include "TaudioCodec.h"
#include "Tlibavcodec.h"
#include "TaudioParser.h"
#include "TrealAudioInfo.h"

class TaudioCodecLibavcodec :public TaudioCodec
{
private:
    Tlibavcodec *ffmpeg;
    AVCodec *avcodec;
    mutable char codecName[100];
    AVCodecParserContext *parser;
    bool codecInited;
    bool contextInited;
    int src_ch_layout;
    TrealAudioInfo m_realAudioInfo;
protected:
    virtual bool init(const CMediaType &mt);
    virtual void getInputDescr1(char_t *buf,size_t buflen) const;
    virtual void updateChannelMapping(void);
public:
    AVCodecContext *avctx;
    TaudioCodecLibavcodec(IffdshowBase *deci, IdecAudioSink *Isink);
    virtual ~TaudioCodecLibavcodec();
    virtual int getType(void) const {
        return IDFF_MOVIE_LAVC;
    }
    virtual const char_t* getName(void) const;
    virtual HRESULT decode(TbyteBuffer &src);
    virtual bool onSeek(REFERENCE_TIME segmentStart);
};

#endif
