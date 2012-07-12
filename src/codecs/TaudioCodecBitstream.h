#ifndef _TAUDIOCODECBITSTREAM_H_
#define _TAUDIOCODECBITSTREAM_H_

#include "TaudioCodec.h"
#include "TaudioParser.h"

class TaudioCodecBitstream : public TaudioCodec
{
private:
    bool bit8, lpcm20, lpcm24, be, float64;
    bool highDef;
    TbyteBuffer bitstreamBuffer;
    size_t additional_blank_size;
    template<class sample_t> static void swapbe(sample_t *dst, size_t size);
    bool isDTSHDMA;
    size_t buffer_limit;
    size_t filledBytes;
    size_t fullMATFrameSize;
protected:
    virtual bool init(const CMediaType &mt);
    virtual void getInputDescr1(char_t *buf, size_t buflen) const;
    virtual void fillAdditionalBytes(void);
    virtual void fillAdditionalMiddleBytes(void);
    virtual void fillAdditionalEndBytes(void);
    virtual void fillBlankBytes(size_t bufferSize);
    virtual void appendMATBuffer(char *src, size_t length);
    virtual int fillMATBuffer(BYTE *src, size_t length, bool checkLength = false);
    virtual HRESULT decodeMAT(TbyteBuffer &src, TaudioParserData audioParserData);
public:
    TaudioCodecBitstream(IffdshowBase *deci, IdecAudioSink *Isink);
    virtual ~TaudioCodecBitstream();
    virtual int getType(void) const {
        return IDFF_MOVIE_RAW;
    }
    virtual HRESULT decode(TbyteBuffer &src);
    virtual const char_t* getName(void) const {
        return _l("bitstream");
    }
    virtual bool onSeek(REFERENCE_TIME segmentStart);
};
#endif

