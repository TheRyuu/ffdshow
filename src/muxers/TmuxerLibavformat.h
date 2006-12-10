#ifndef _TMUXERLIBAVFORMAT_H_
#define _TMUXERLIBAVFORMAT_H_

#include "Tmuxer.h"
#include "libavformat/avformat.h"

struct Tlibavcodec;
class TmuxerLibavformat :public Tmuxer
{
private:
 Tlibavcodec *lavc;
 AVFormatContext fctx;
 AVStream strm;
 AVFormatParameters ap;
 bool writing;
public:
 TmuxerLibavformat(IffdshowBase *Ideci,int id);
 virtual ~TmuxerLibavformat();
 virtual size_t writeFrame(const void *data,size_t len,const TencFrameParams &frameParams);
};

#endif
