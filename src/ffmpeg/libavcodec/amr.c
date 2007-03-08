/*
 * AMR Audio decoder stub
 * Copyright (c) 2003 the ffmpeg project
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

 /** @file
 * Adaptive Multi-Rate (AMR) Audio decoder stub.
 *
 * This code implements both an AMR-NarrowBand (AMR-NB) and an AMR-WideBand
 * (AMR-WB) audio encoder/decoder through external reference code from
 * http://www.3gpp.org/. The license of the code from 3gpp is unclear so you
 * have to download the code separately. Two versions exists: One fixed-point
 * and one with floats. For some reason the float-encoder is significant faster
 * at least on a P4 1.5GHz (0.9s instead of 9.9s on a 30s audio clip at MR102).
 * Both float and fixed point are supported for AMR-NB, but only float for
 * AMR-WB.
 *
 * \section AMR-NB
 *
 * \subsection Float
 * The float version (default) can be downloaded from:
 * http://www.3gpp.org/ftp/Specs/archive/26_series/26.104/26104-510.zip
 * Extract the source into \c "ffmpeg/libavcodec/amr_float".
 *
 * \subsection Fixed-point
 * The fixed-point (TS26.073) can be downloaded from:
 * http://www.3gpp.org/ftp/Specs/archive/26_series/26.073/26073-510.zip.
 * Extract the source into \c "ffmpeg/libavcodec/amr".
 * To use the fixed version run \c "./configure" with \c "--enable-amr_nb-fixed".
 *
 * \subsection Specification
 * The specification for AMR-NB can be found in TS 26.071
 * (http://www.3gpp.org/ftp/Specs/html-info/26071.htm) and some other
 * info at http://www.3gpp.org/ftp/Specs/html-info/26-series.htm.
 *
 * \section AMR-WB
 * \subsection Float
 * The reference code can be downloaded from:
 * http://www.3gpp.org/ftp/Specs/archive/26_series/26.204/26204-510.zip
 * It should be extracted to \c "ffmpeg/libavcodec/amrwb_float". Enable it with
 * \c "--enable-amr_wb".
 *
 * \subsection Fixed-point
 * If someone wants to use the fixed point version it can be downloaded from:
 * http://www.3gpp.org/ftp/Specs/archive/26_series/26.173/26173-571.zip.
 *
 * \subsection Specification
 * The specification for AMR-WB can be downloaded from:
 * http://www.3gpp.org/ftp/Specs/archive/26_series/26.171/26171-500.zip.
 *
 */

#include "avcodec.h"

#ifdef AMR_NB_FIXED

#define MMS_IO

#include "amr/sp_dec.h"
#include "amr/d_homing.h"
#include "amr/typedef.h"
#include "amr/sp_enc.h"
#include "amr/sid_sync.h"
#include "amr/e_homing.h"

#else
#include "amr_float/interf_dec.h"
#include "amr_float/interf_enc.h"
#endif

/* Common code for fixed and float version*/
typedef struct AMR_bitrates
{
    int startrate;
    int stoprate;
    enum Mode mode;
} AMR_bitrates;

/* Match desired bitrate with closest one*/
static enum Mode getBitrateMode(int bitrate)
{
    /* Adjusted so that all bitrates can be used from commandline where
       only a multiple of 1000 can be specified*/
    AMR_bitrates rates[]={ {0,4999,MR475}, //4
                           {5000,5899,MR515},//5
                           {5900,6699,MR59},//6
                           {6700,7000,MR67},//7
                           {7001,7949,MR74},//8
                           {7950,9999,MR795},//9
                           {10000,11999,MR102},//10
                           {12000,64000,MR122},//12
                         };
    int i;

    for(i=0;i<8;i++)
    {
        if(rates[i].startrate<=bitrate && rates[i].stoprate>=bitrate)
        {
            return(rates[i].mode);
        }
    }
    /*Return highest possible*/
    return(MR122);
}

static void amr_decode_fix_avctx(AVCodecContext * avctx)
{
    //const int is_amr_wb = 1 + (avctx->codec_id == CODEC_ID_AMR_WB);
    const int is_amr_wb = 1;

    if(avctx->sample_rate == 0)
    {
        avctx->sample_rate = 8000 * is_amr_wb;
    }

    if(avctx->channels == 0)
    {
        avctx->channels = 1;
    }

    avctx->frame_size = 160 * is_amr_wb;
}

#ifdef AMR_NB_FIXED
/* fixed point version*/
/* frame size in serial bitstream file (frame type + serial stream + flags) */
#define SERIAL_FRAMESIZE (1+MAX_SERIAL_SIZE+5)

typedef struct AMRContext {
    int frameCount;
    Speech_Decode_FrameState *speech_decoder_state;
    enum RXFrameType rx_type;
    enum Mode mode;
    Word16 reset_flag;
    Word16 reset_flag_old;

    enum Mode enc_bitrate;
    Speech_Encode_FrameState *enstate;
    sid_syncState *sidstate;
    enum TXFrameType tx_frametype;
} AMRContext;

static int amr_nb_decode_init(AVCodecContext * avctx)
{
    AMRContext *s = avctx->priv_data;

    s->frameCount=0;
    s->speech_decoder_state=NULL;
    s->rx_type = (enum RXFrameType)0;
    s->mode= (enum Mode)0;
    s->reset_flag=0;
    s->reset_flag_old=1;

    if(Speech_Decode_Frame_init(&s->speech_decoder_state, "Decoder"))
    {
        av_log(avctx, AV_LOG_ERROR, "Speech_Decode_Frame_init error\n");
        return -1;
    }

    amr_decode_fix_avctx(avctx);

    if(avctx->channels > 1)
    {
        av_log(avctx, AV_LOG_ERROR, "amr_nb: multichannel decoding not supported\n");
        return -1;
    }

    return 0;
}

static int amr_nb_decode_close(AVCodecContext * avctx)
{
    AMRContext *s = avctx->priv_data;

    Speech_Decode_Frame_exit(&s->speech_decoder_state);
    return 0;
}

static int amr_nb_decode_frame(AVCodecContext * avctx,
            void *data, int *data_size,
            uint8_t * buf, int buf_size)
{
    AMRContext *s = avctx->priv_data;
    uint8_t*amrData=buf;
    int offset=0;
    UWord8 toc, q, ft;
    Word16 serial[SERIAL_FRAMESIZE];   /* coded bits */
    Word16 *synth;
    UWord8 *packed_bits;
    static Word16 packed_size[16] = {12, 13, 15, 17, 19, 20, 26, 31, 5, 0, 0, 0, 0, 0, 0, 0};
    int i;

    //printf("amr_decode_frame data_size=%i buf=0x%X buf_size=%d frameCount=%d!!\n",*data_size,buf,buf_size,s->frameCount);

    synth=data;

//    while(offset<buf_size)
    {
        toc=amrData[offset];
        /* read rest of the frame based on ToC byte */
        q  = (toc >> 2) & 0x01;
        ft = (toc >> 3) & 0x0F;

        //printf("offset=%d, packet_size=%d amrData= 0x%X %X %X %X\n",offset,packed_size[ft],amrData[offset],amrData[offset+1],amrData[offset+2],amrData[offset+3]);

        offset++;

        packed_bits=amrData+offset;

        offset+=packed_size[ft];

        //Unsort and unpack bits
        s->rx_type = UnpackBits(q, ft, packed_bits, &s->mode, &serial[1]);

        //We have a new frame
        s->frameCount++;

        if (s->rx_type == RX_NO_DATA)
        {
            s->mode = s->speech_decoder_state->prev_mode;
        }
        else {
            s->speech_decoder_state->prev_mode = s->mode;
        }

        /* if homed: check if this frame is another homing frame */
        if (s->reset_flag_old == 1)
        {
            /* only check until end of first subframe */
            s->reset_flag = decoder_homing_frame_test_first(&serial[1], s->mode);
        }
        /* produce encoder homing frame if homed & input=decoder homing frame */
        if ((s->reset_flag != 0) && (s->reset_flag_old != 0))
        {
            for (i = 0; i < L_FRAME; i++)
            {
                synth[i] = EHF_MASK;
            }
        }
        else
        {
            /* decode frame */
            Speech_Decode_Frame(s->speech_decoder_state, s->mode, &serial[1], s->rx_type, synth);
        }

        //Each AMR-frame results in 160 16-bit samples
        *data_size+=160*2;
        synth+=160;

        /* if not homed: check whether current frame is a homing frame */
        if (s->reset_flag_old == 0)
        {
            /* check whole frame */
            s->reset_flag = decoder_homing_frame_test(&serial[1], s->mode);
        }
        /* reset decoder if current frame is a homing frame */
        if (s->reset_flag != 0)
        {
            Speech_Decode_Frame_reset(s->speech_decoder_state);
        }
        s->reset_flag_old = s->reset_flag;

    }
    return offset;
}


#else /* Float point version*/

typedef struct AMRContext {
    int frameCount;
    void * decState;
    int *enstate;
    enum Mode enc_bitrate;
} AMRContext;

static int amr_nb_decode_init(AVCodecContext * avctx)
{
    AMRContext *s = avctx->priv_data;

    s->frameCount=0;
    s->decState=Decoder_Interface_init();
    if(!s->decState)
    {
        av_log(avctx, AV_LOG_ERROR, "Decoder_Interface_init error\r\n");
        return -1;
    }

    amr_decode_fix_avctx(avctx);

    if(avctx->channels > 1)
    {
        av_log(avctx, AV_LOG_ERROR, "amr_nb: multichannel decoding not supported\n");
        return -1;
    }

    return 0;
}

static int amr_nb_decode_close(AVCodecContext * avctx)
{
    AMRContext *s = avctx->priv_data;

    Decoder_Interface_exit(s->decState);
    return 0;
}

static int amr_nb_decode_frame(AVCodecContext * avctx,
            void *data, int *data_size,
            uint8_t * buf, int buf_size)
{
    AMRContext *s = avctx->priv_data;
    uint8_t*amrData=buf;
    static short block_size[16]={ 12, 13, 15, 17, 19, 20, 26, 31, 5, 0, 0, 0, 0, 0, 0, 0 };
    enum Mode dec_mode;
    int packet_size;

    /* av_log(NULL,AV_LOG_DEBUG,"amr_decode_frame buf=%p buf_size=%d frameCount=%d!!\n",buf,buf_size,s->frameCount); */

    dec_mode = (buf[0] >> 3) & 0x000F;
    packet_size = block_size[dec_mode]+1;

    if(packet_size > buf_size) {
        av_log(avctx, AV_LOG_ERROR, "amr frame too short (%u, should be %u)\n", buf_size, packet_size);
        return -1;
    }

    s->frameCount++;
    /* av_log(NULL,AV_LOG_DEBUG,"packet_size=%d amrData= 0x%X %X %X %X\n",packet_size,amrData[0],amrData[1],amrData[2],amrData[3]); */
    /* call decoder */
    Decoder_Interface_Decode(s->decState, amrData, data, 0);
    *data_size=160*2;

    return packet_size;
}

#endif

AVCodec amr_nb_decoder =
{
    "amr_nb",
    CODEC_TYPE_AUDIO,
    CODEC_ID_AMR_NB,
    sizeof(AMRContext),
    amr_nb_decode_init,
    NULL,
    amr_nb_decode_close,
    amr_nb_decode_frame,
};
