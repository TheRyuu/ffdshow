#ifndef _TAUDIOCODECLIBAVCODEC_H_
#define _TAUDIOCODECLIBAVCODEC_H_

#include "TaudioCodec.h"
#include "Tlibavcodec.h"

class TaudioCodecLibavcodec :public TaudioCodec
{
private:
 Tlibavcodec *libavcodec;
 AVCodec *avcodec;mutable char codecName[100];
 AVCodecContext *avctx;
 bool codecinited;
 int isGain;
protected:
 virtual bool init(const CMediaType &mt);
 virtual void getInputDescr1(char_t *buf,size_t buflen) const;
public:
 TaudioCodecLibavcodec(IffdshowBase *deci,IdecAudioSink *Isink);
 virtual ~TaudioCodecLibavcodec();
 virtual int getType(void) const {return IDFF_MOVIE_LAVC;}
 virtual const char_t* getName(void) const;
 virtual HRESULT decode(TbyteBuffer &src);
 virtual bool onSeek(REFERENCE_TIME segmentStart);
};

#endif
