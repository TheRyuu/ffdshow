#ifndef _TMUXEROGM_H_
#define _TMUXEROGM_H_

#include "Tmuxer.h"
#include "ogg/ogg.h"

class TmuxerOGM :public Tmuxer
{
private:
 HANDLE out;
 int serialno;
 double fps,sample_rate;
 int max_frame_size;
 ogg_stream_state os;
 int packetno;
 int next_is_key;
 char *tempbuf;
 ogg_int64_t last_granulepos,old_granulepos;
 void produce_header_packets(const BITMAPINFOHEADER &bihdr);
 int flush_pages(int header_page=0);
 int queue_pages(int header_page=0);
 unsigned int written;
 int add_ogg_page(ogg_page *opage, int header_page, int index_serial = -1);
 ogg_page *copy_ogg_page(ogg_page *);
 void next_page_contains_keyframe(int serial);
 int process(const void *buf, size_t size, int num_frames,int key, int last_frame);
 typedef double stamp_t;
 enum
  {
   EMOREDATA  =-1,
   EMALLOC    =-2,
   EBADHEADER =-3,
   EBADEVENT  =-4,
   EOTHER     =-5
  };
stamp_t make_timestamp(ogg_int64_t granulepos);
public:
 TmuxerOGM(IffdshowBase *Ideci);
 virtual ~TmuxerOGM();
 virtual size_t writeHeader(const void *data,size_t len,bool flush,const BITMAPINFOHEADER &bihdr);
 virtual size_t writeFrame(const void *data,size_t len,const TencFrameParams &frameParams);
};

#endif
