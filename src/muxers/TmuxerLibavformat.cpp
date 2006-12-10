/*
 * Copyright (c) 2002-2006 Milan Cutka
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "stdafx.h"
#include "ffImgfmt.h"
#include "ffcodecs.h"
#include "TmuxerLibavformat.h"
#include "Tlibavcodec.h"
#include "ffdshow_constants.h"
#include "IffdshowBase.h"
#include "TencStats.h"
#include "TffPict.h"

TmuxerLibavformat::TmuxerLibavformat(IffdshowBase *Ideci,int id):Tmuxer(Ideci)
{
 deci->getLibavcodec(&lavc);
 writing=false;
 if (!lavc->ok) return;
 AVOutputFormat *of=lavc->guess_format(id==MUXER_MPEG?"mpeg":"flv",NULL,NULL);
 if (!of) return;
 memset(&fctx,0,sizeof(fctx));
 fctx.oformat=of;
 text<char>(deci->getParamStr2(IDFF_enc_storeExtFlnm),fctx.filename);
 lavc->url_fopen(&fctx.pb,fctx.filename,URL_WRONLY);
 memset(&strm,0,sizeof(strm));
 strm.codec=lavc->avcodec_alloc_context();
 strm.codec->codec_type=CODEC_TYPE_VIDEO;
 strm.codec->bit_rate=deci->getParam2(IDFF_enc_bitrate1000)*1000;
 strm.codec->time_base.den=deci->getParam2(IDFF_enc_fpsRate);strm.codec->time_base.num=deci->getParam2(IDFF_enc_fpsScale);
 strm.time_base=strm.codec->time_base;
 fctx.streams[0]=&strm;
 fctx.nb_streams=1;
 lavc->av_set_parameters(&fctx,&ap);
 lavc->av_write_header(&fctx);
 writing=true;
}
TmuxerLibavformat::~TmuxerLibavformat()
{
 if (writing)
  {
   lavc->av_write_trailer(&fctx);
   lavc->avcodec_close(strm.codec);
  } 
 if (lavc) lavc->Release();
}
size_t TmuxerLibavformat::writeFrame(const void *data,size_t len,const TencFrameParams &frameParams)
{
 if (writing)
  {
   AVPacket pkt;
   lavc->av_new_packet(&pkt,(int)len);
   memcpy(pkt.data,data,len);
   pkt.flags=frameParams.frametype==FRAME_TYPE::I?PKT_FLAG_KEY:0;
   lavc->av_write_frame(&fctx,&pkt);
    av_free_packet(&pkt);
  } 
 return 0;
}
