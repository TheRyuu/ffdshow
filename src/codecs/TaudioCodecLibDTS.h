#ifndef _TAUDIOCODECLIBDTS_H_
#define _TAUDIOCODECLIBDTS_H_

#include "TaudioCodec.h"
namespace libdts
{
 #include "libdts/dts.h"
} 

class Tdll;
class TaudioCodecLibDTS :public TaudioCodec
{
private:
 Tdll *dll;
 bool inited;
 libdts::dts_state_t* (*dts_init)(uint32_t mm_accel);
 void (*dts_free)(libdts::dts_state_t * state);
 int (*dts_syncinfo)(libdts::dts_state_t *state, uint8_t * buf, int * flags, int * sample_rate, int * bit_rate, int *frame_length);
 int (*dts_frame)(libdts::dts_state_t * state, uint8_t * buf, int * flags, libdts::level_t * level, libdts::sample_t bias);
 void (*dts_dynrng)(libdts::dts_state_t * state, libdts::level_t (* call) (libdts::level_t, void *), void * data);
 int (*dts_blocks_num)(libdts::dts_state_t * state);
 int (*dts_block)(libdts::dts_state_t * state);
 libdts::sample_t* (*dts_samples)(libdts::dts_state_t * state);

 libdts::dts_state_t *state;
 static struct Tscmap
  {
   int nchannels;
   char ch[6];
   int channelMask;
  } const scmaps[];
 int drc;
protected:
 virtual bool init(const CMediaType &mt);
 virtual void getInputDescr1(char_t *buf,size_t buflen) const;
public:
 TaudioCodecLibDTS(IffdshowBase *deci,IdecAudioSink *Isink);
 virtual ~TaudioCodecLibDTS();
 virtual int getType(void) const {return IDFF_MOVIE_LIBDTS;}
 static const char_t *dllname;
 virtual HRESULT decode(TbyteBuffer &src);
 virtual bool onSeek(REFERENCE_TIME segmentStart);
};

#endif
