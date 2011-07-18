/*
 * Copyright (c) 2002-2006 Milan Cutka
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "stdafx.h"
#include "TimgFilterAvisynth.h"
#include "IffdshowBase.h"
#include "IffdshowDecVideo.h"
#include "Tconvert.h"

bool debugPrint=false;
int maxBufferAhead=0;
int maxBufferBack=0;

//========================== TimgFilterAvisynth::TframeBuffer ===============================
void TimgFilterAvisynth::TframeBuffer::CreateFrame(TimgFilterAvisynth::Tinput *input)
{
    ReleaseFrame();

    AVS_VideoInfo vi;
    input->InitVideoInfo(vi);

    this->input=input;
    frame=input->env->avs_new_video_frame_a(*input->env,&vi,16);

    switch (input->csp&FF_CSPS_MASK) {
        case FF_CSP_420P:
            TffPict::copy(avs_get_write_ptr_p(frame,PLANAR_Y),avs_get_pitch_p(frame,PLANAR_Y),input->src[0],input->stride1[0],input->dx  ,input->dy);
            TffPict::copy(avs_get_write_ptr_p(frame,PLANAR_U),avs_get_pitch_p(frame,PLANAR_U),input->src[1],input->stride1[1],input->dx/2,input->dy/2);
            TffPict::copy(avs_get_write_ptr_p(frame,PLANAR_V),avs_get_pitch_p(frame,PLANAR_V),input->src[2],input->stride1[2],input->dx/2,input->dy/2);
            break;

        case FF_CSP_YUY2:
            TffPict::copy(avs_get_write_ptr(frame),avs_get_pitch(frame),input->src[0],input->stride1[0],input->dx*bytesPerPixel,input->dy);
            break;

        case FF_CSP_RGB24:
        case FF_CSP_RGB32:
            TffPict::copy(avs_get_write_ptr(frame),avs_get_pitch(frame),input->src[0]+input->stride1[0]*(input->dy-1),-input->stride1[0],input->dx*bytesPerPixel,input->dy);
            break;
    }
}

void TimgFilterAvisynth::TframeBuffer::CreateField(TimgFilterAvisynth::Tinput *input, bool topField)
{
    ReleaseFrame();

    AVS_VideoInfo vi;
    input->InitVideoInfo(vi);
    vi.height/=2;

    this->input=input;
    frame=input->env->avs_new_video_frame_a(*input->env,&vi,16);

    switch (input->csp&FF_CSPS_MASK) {
        case FF_CSP_420P:
            TffPict::copy(avs_get_write_ptr_p(frame,PLANAR_Y),avs_get_pitch_p(frame,PLANAR_Y),input->src[0]+(topField ? 0 : input->stride1[0]),input->stride1[0]*2,input->dx  ,input->dy/2);
            TffPict::copy(avs_get_write_ptr_p(frame,PLANAR_U),avs_get_pitch_p(frame,PLANAR_U),input->src[1]+(topField ? 0 : input->stride1[1]),input->stride1[1]*2,input->dx/2,input->dy/4);
            TffPict::copy(avs_get_write_ptr_p(frame,PLANAR_V),avs_get_pitch_p(frame,PLANAR_V),input->src[2]+(topField ? 0 : input->stride1[2]),input->stride1[2]*2,input->dx/2,input->dy/4);
            break;

        case FF_CSP_YUY2:
            TffPict::copy(avs_get_write_ptr(frame),avs_get_pitch(frame),input->src[0]+(topField ? 0 : input->stride1[0]),input->stride1[0]*2,input->dx*bytesPerPixel,input->dy/2);
            break;

        case FF_CSP_RGB24:
        case FF_CSP_RGB32:
            TffPict::copy(avs_get_write_ptr(frame),avs_get_pitch(frame),input->src[0]+input->stride1[0]*(input->dy-(topField ? 1 : 2)),-input->stride1[0]*2,input->dx*bytesPerPixel,input->dy/2);
            break;
    }
}

void TimgFilterAvisynth::TframeBuffer::CombineFrame(TimgFilterAvisynth::Tinput *input, bool inTopField, bool outTopField, AVS_VideoFrame *otherField)
{
    ReleaseFrame();

    AVS_VideoInfo vi;
    input->InitVideoInfo(vi);

    this->input=input;
    frame=input->env->avs_new_video_frame_a(*input->env,&vi,16);

    switch (input->csp&FF_CSPS_MASK) {
        case FF_CSP_420P:
            TffPict::copy(avs_get_write_ptr_p(frame,PLANAR_Y)+(outTopField ? 0 : avs_get_pitch_p(frame,PLANAR_Y)),avs_get_pitch_p(frame,PLANAR_Y)*2,input->src[0]+(inTopField ? 0 : input->stride1[0]),input->stride1[0]*2,input->dx  ,input->dy/2);
            TffPict::copy(avs_get_write_ptr_p(frame,PLANAR_U)+(outTopField ? 0 : avs_get_pitch_p(frame,PLANAR_U)),avs_get_pitch_p(frame,PLANAR_U)*2,input->src[1]+(inTopField ? 0 : input->stride1[1]),input->stride1[1]*2,input->dx/2,input->dy/4);
            TffPict::copy(avs_get_write_ptr_p(frame,PLANAR_V)+(outTopField ? 0 : avs_get_pitch_p(frame,PLANAR_V)),avs_get_pitch_p(frame,PLANAR_V)*2,input->src[2]+(inTopField ? 0 : input->stride1[2]),input->stride1[2]*2,input->dx/2,input->dy/4);

            TffPict::copy(avs_get_write_ptr_p(frame,PLANAR_Y)+(outTopField ? avs_get_pitch_p(frame,PLANAR_Y) : 0),avs_get_pitch_p(frame,PLANAR_Y)*2,avs_get_read_ptr_p(otherField,PLANAR_Y),avs_get_pitch_p(otherField,PLANAR_Y),input->dx  ,input->dy/2);
            TffPict::copy(avs_get_write_ptr_p(frame,PLANAR_U)+(outTopField ? avs_get_pitch_p(frame,PLANAR_U) : 0),avs_get_pitch_p(frame,PLANAR_U)*2,avs_get_read_ptr_p(otherField,PLANAR_U),avs_get_pitch_p(otherField,PLANAR_U),input->dx/2,input->dy/4);
            TffPict::copy(avs_get_write_ptr_p(frame,PLANAR_V)+(outTopField ? avs_get_pitch_p(frame,PLANAR_V) : 0),avs_get_pitch_p(frame,PLANAR_V)*2,avs_get_read_ptr_p(otherField,PLANAR_V),avs_get_pitch_p(otherField,PLANAR_V),input->dx/2,input->dy/4);

            break;

        case FF_CSP_YUY2:
            TffPict::copy(avs_get_write_ptr(frame)+(outTopField ? 0 : avs_get_pitch(frame)),avs_get_pitch(frame)*2,input->src[0]+(inTopField ? 0 : input->stride1[0]),input->stride1[0]*2,input->dx*bytesPerPixel,input->dy/2);
            TffPict::copy(avs_get_write_ptr(frame)+(outTopField ? avs_get_pitch(frame) : 0),avs_get_pitch(frame)*2,avs_get_read_ptr(otherField),avs_get_pitch(otherField),input->dx*bytesPerPixel,input->dy/2);

            break;

        case FF_CSP_RGB24:
        case FF_CSP_RGB32:
            TffPict::copy(avs_get_write_ptr(frame)+(outTopField ? 0 : avs_get_pitch(frame)),avs_get_pitch(frame)*2,input->src[0]+input->stride1[0]*(input->dy-(inTopField ? 1 : 2)),-input->stride1[0]*2,input->dx*bytesPerPixel,input->dy/2);
            TffPict::copy(avs_get_write_ptr(frame)+(outTopField ? avs_get_pitch(frame) : 0),avs_get_pitch(frame)*2,avs_get_read_ptr(otherField),avs_get_pitch(otherField),input->dx*bytesPerPixel,input->dy/2);
            break;
    }
}

void TimgFilterAvisynth::TframeBuffer::ReleaseFrame()
{
    if (frame && input) {
        input->env->avs_release_video_frame(frame);
    }

    input=NULL;
    frame=NULL;
}

TimgFilterAvisynth::TframeBuffer::~TframeBuffer()
{
    ReleaseFrame();
}

//========================== TimgFilterAvisynth::Tffdshow_source ===============================
AVS_Value AVSC_CC TimgFilterAvisynth::Tffdshow_source::Create(AVS_ScriptEnvironment *env, AVS_Value args, void *user_data)
{
    Tinput* input=(Tinput*)user_data;
    AVS_Value v;
    AVS_FilterInfo *fi;
    AVS_Clip *new_clip=input->env->avs_new_c_filter(*input->env,&fi,args,0);
    Tffdshow_source *filter=new Tffdshow_source(input,(VideoInfo&)fi->vi);

    fi->user_data=filter;
    fi->get_frame=get_frame;
    fi->get_parity=get_parity;
    fi->set_cache_hints=set_cache_hints;
    fi->free_filter=free_filter;

    input->env->avs_set_to_clip(&v, new_clip);
    input->env->avs_release_clip(new_clip);

    return v;
}

void AVSC_CC TimgFilterAvisynth::Tffdshow_source::free_filter(AVS_FilterInfo *fi)
{
    if (fi && fi->user_data) {
        delete (Tffdshow_source*)fi->user_data;
    }
}

TimgFilterAvisynth::Tffdshow_source::Tffdshow_source(Tinput *Iinput,VideoInfo &Ivi):
    input(Iinput),
    vi(Ivi)
{
    input->InitVideoInfo(*&vi);
}

int TimgFilterAvisynth::findBuffer(TframeBuffer* buffers, int numBuffers, int n)
{
    // Find the index of the buffer that's framenumber is closest (or equal to) n

    int minDistance=MAX_INT;
    int bestBufferNo=0;

    if (buffers && numBuffers > 1)
        for (int bufferNo=0; bufferNo < numBuffers; bufferNo++) {
            TframeBuffer& buffer=buffers[bufferNo];

            if (buffer.frameNo < 0) {
                continue;
            } else if (buffer.frameNo == n) {
                bestBufferNo=bufferNo;

                break;
            }

            int distance=abs(buffer.frameNo-n);

            if (distance < minDistance) {
                minDistance=distance;
                bestBufferNo=bufferNo;
            }
        }

    return bestBufferNo;
}

AVS_VideoFrame* AVSC_CC TimgFilterAvisynth::Tffdshow_source::get_frame(AVS_FilterInfo *fi, int n)
{
    Tffdshow_source *filter=(Tffdshow_source*)fi->user_data;
    Tinput* input=filter->input;

    // Calculate request statistics for currently produced frame

    int curFrameDistance=(n >= input->backLimit ? n : input->backLimit)-input->curFrame;

    if (input->numAccessedFrames < 100) {
        input->accessedFrames[input->numAccessedFrames]=curFrameDistance;
    }

    if (input->minAccessedFrame > curFrameDistance) {
        input->minAccessedFrame=curFrameDistance;
    }

    if (input->maxAccessedFrame < curFrameDistance) {
        input->maxAccessedFrame=curFrameDistance;
    }

    input->numAccessedFrames++;

    // Find the buffered frame that's closest to n and return it

    if (input->numBuffers > 0) {
        TframeBuffer& buffer=input->buffers[findBuffer(input->buffers,input->numBuffers,n)];

        if (debugPrint) {
            DPRINTF(_l("TimgFilterAvisynth: Looked up frame %i, using frame %i"),n,buffer.frameNo);
        }

        if (buffer.frame) {
            return input->env->avs_copy_video_frame(buffer.frame);
        }
    }

    int count;
    unsigned long* dest;
    PVideoFrame frame(input->env,input->env->avs_new_video_frame_a(*input->env,&filter->vi,16));

    if (debugPrint) {
        DPRINTF(_l("TimgFilterAvisynth: Looked up frame %i, but no frames were buffered"),n);
    }

    switch (filter->input->csp&FF_CSPS_MASK) {
        case FF_CSP_420P:
            memset(frame->GetWritePtr(PLANAR_Y),0,frame->GetPitch(PLANAR_Y)*frame->GetHeight(PLANAR_Y));
            memset(frame->GetWritePtr(PLANAR_U),128,frame->GetPitch(PLANAR_U)*frame->GetHeight(PLANAR_U));
            memset(frame->GetWritePtr(PLANAR_V),128,frame->GetPitch(PLANAR_V)*frame->GetHeight(PLANAR_V));

            break;

        case FF_CSP_YUY2:
            for (dest=(unsigned long*)frame->GetWritePtr(),
                    count=frame->GetPitch()*frame->GetHeight();
                    count > 0;
                    dest++,
                    count-=sizeof(unsigned long)) {
                *dest=0x80008000;
            }

            break;

        case FF_CSP_RGB24:
        case FF_CSP_RGB32:
            memset(frame->GetWritePtr(),0,frame->GetPitch()*frame->GetHeight());

            break;
    }

    return frame;
}

//============================= TimgFilterAvisynth::Tffdshow_setAR =============================

AVS_Value AVSC_CC TimgFilterAvisynth::Tffdshow_setAR::Create_SetSAR(AVS_ScriptEnvironment *env, AVS_Value args, void * user_data)
{
    return Create(env,args,user_data,false);
}

AVS_Value AVSC_CC TimgFilterAvisynth::Tffdshow_setAR::Create_SetDAR(AVS_ScriptEnvironment *env, AVS_Value args, void * user_data)
{
    return Create(env,args,user_data,true);
}

AVS_Value AVSC_CC TimgFilterAvisynth::Tffdshow_setAR::Create(AVS_ScriptEnvironment *env, AVS_Value args, void * user_data, bool setDAR)
{
    Tinput* input=(Tinput*)user_data;
    int x=avs_as_int(avs_array_elt(args,0));
    int y=avs_as_int(avs_array_elt(args,1));

    input->outputSar=( setDAR ? Rational(0,0) : Rational(x,y));
    input->outputDar=(!setDAR ? Rational(0,0) : Rational(x,y));

    return avs_void;
}

//================================ TimgFilterAvisynth::Tavisynth ===============================
bool TimgFilterAvisynth::Tavisynth::createClip(const TavisynthSettings *cfg,Tinput *input,TffPictBase& pict)
{
    if (!input->env) {
        input->env=CreateScriptEnvironment(AVISYNTH_INTERFACE_VERSION);

        if (!input->env) {
            return false;
        }

        input->env->AddFunction(
            "ffdshow_source",
            "",
            TimgFilterAvisynth::Tffdshow_source::Create,
            (void*)input);

        input->env->AddFunction(
            "ffdshow_setSAR",
            "[x]i[y]i",
            TimgFilterAvisynth::Tffdshow_setAR::Create_SetSAR,
            (void*)input);

        input->env->AddFunction(
            "ffdshow_setDAR",
            "[x]i[y]i",
            TimgFilterAvisynth::Tffdshow_setAR::Create_SetDAR,
            (void*)input);
    }

    IScriptEnvironment* env=input->env;

    Rational sar=pict.rectClip.sar;
    Rational dar=pict.rectClip.dar();

    env->SetGlobalVar("ffdshow_sar_x",AVSValue(sar.num));
    env->SetGlobalVar("ffdshow_sar_y",AVSValue(sar.den));
    env->SetGlobalVar("ffdshow_dar_x",AVSValue(dar.num));
    env->SetGlobalVar("ffdshow_dar_y",AVSValue(dar.den));

    char script[MAX_AVISYNTH_SCRIPT_LENGTH];

    // Convert script to ASCII; add ffdshow_source if the option for it is checked
    _snprintf(
        script,
        sizeof(script)-1,
        "%s%s%s",
        cfg->ffdshowSource ? "ffdshow_source()\n" : "",
        (const char*)text<char>(cfg->script),
        cfg->ffdshowSource ? "\nreturn last" : "");

    script[sizeof(script)-1]=0;

    try {
        // Try using the script

        AVSValue eval_args[]= {script,"ffdshow_filter_avisynth_script"};
        AVSValue val=env->Invoke("Eval",AVSValue(eval_args,2));

        if (val.IsClip()) {
            input->clip=new PClip(val,env);

            return true;
        } else {
            throw AvisynthError("Invalid script!");
        }
    } catch (AvisynthError &err) {
        // Create another script that contains only the source with the error subtitled onto it
        char errMsg[1024];

        _snprintf(errMsg,1023,"%s",(const char*)text<char>(err.msg));
        errMsg[1023]=0;

        for (char* errPtr=errMsg; *errPtr; errPtr++)
            if (*errPtr == '\"') {
                *errPtr='\'';
            } else if (*errPtr == '\n') {
                *errPtr=' ';
            }

        _snprintf(script,2047,"return ffdshow_source().Subtitle(\"%s\")",errMsg);

        try {
            // Try using that script

            AVSValue err_eval_args[]= {script,"ffdshow_filter_avisynth_error_script"};
            AVSValue val=env->Invoke("Eval",AVSValue(err_eval_args,2));

            if (val.IsClip()) {
                input->clip=new PClip(val,env);

                return true;
            } else {
                throw err;
            }
        } catch (AvisynthError) {
            // Oh well...
            throw err;
        }
    }
}

void TimgFilterAvisynth::Tavisynth::setOutFmt(const TavisynthSettings *cfg,Tinput *input,TffPictBase &pict)
{
    if ((pict.rectClip == inputRect) && !(pict.rectClip.dar() != inputDar)) { // No operator== in Rational... o_O;
        if (outputRect != inputRect) {
            pict.rectFull=pict.rectClip=outputRect;
        }
    } else if (createClip(cfg,input,pict)) {
        IScriptEnvironment* env=input->env;

        inputRect=pict.rectClip;
        inputSar=pict.rectClip.sar;
        inputDar=pict.rectClip.dar();

        const VideoInfo &vi=(*input->clip)->GetVideoInfo();

        if (input->outputDar)
            pict.rectFull=pict.rectClip=outputRect=
                                            Trect(
                                                0,0,vi.width,vi.height,
                                                Rational((vi.height*input->outputDar.num)/double(vi.width*input->outputDar.den),32768));
        else if (input->outputSar) {
            pict.rectFull=pict.rectClip=outputRect=Trect(0,0,vi.width,vi.height,input->outputSar);
        } else if ((unsigned int)vi.width != inputRect.dx || (unsigned int)vi.height != inputRect.dy) {
            pict.rectFull=pict.rectClip=outputRect=Trect(0,0,vi.width,vi.height,inputSar);
        } else {
            outputRect=inputRect;
        }

        delete input->clip;
        input->clip=0;
    }
}

void TimgFilterAvisynth::Tavisynth::skipAhead(bool passFirstThrough, bool clearLastOutStopTime)
{
    // Skip ahead at least 1000 frames to make sure AviSynth doesn't use buffered frames,
    // then round the new frame numbers up to make sure the relative alignment between the
    // different framenumbers stays the same
    // (Don't skip if buffering is turned off, though...)

    int skippedFrames=(numBuffers > (applyPulldown == 1 ? 2 : 1) ? 1000 : 0);
    REFERENCE_TIME roundToLong=frameScaleNum*frameScaleNum*frameScaleDen;
    int roundTo=(int)(roundToLong > NUM_FRAMES ? NUM_FRAMES : roundToLong);

    curInFrameNo=curInFrameNo+skippedFrames+(roundTo-1);
    curInFrameNo-=curInFrameNo%roundTo;

    if (curInFrameNo > TimgFilterAvisynth::NUM_FRAMES/2)
        // Make sure we don't actually hit the end of the AviSynth clip
    {
        curInFrameNo=0;
    }

    backLimit=curInFrameNo;

    curOutFrameNo=(int)(((REFERENCE_TIME)curInFrameNo)*frameScaleDen/frameScaleNum);
    curOutScaledFrameNo=curInFrameNo;

    if (clearLastOutStopTime) {
        lastOutStopTime=0;
    }

    resetBuffers=true;
    this->passFirstThrough=passFirstThrough;
}

void TimgFilterAvisynth::Tavisynth::done(void)
{
    if (buffers) {
        delete[] buffers;
    }
    buffers=0;
}

void TimgFilterAvisynth::Tavisynth::init(const TavisynthSettings &oldcfg, Tinput* input,uint64_t *outcsp, TffPictBase& pict)
{
    infoBuf[0]=0;

    if (createClip(&oldcfg,input,pict)) {
        const VideoInfo &vi=(*input->clip)->GetVideoInfo();
        if      (vi.IsRGB24()) {
            *outcsp=FF_CSP_RGB24|FF_CSP_FLAGS_VFLIP;
        } else if (vi.IsRGB32()) {
            *outcsp=FF_CSP_RGB32|FF_CSP_FLAGS_VFLIP;
        } else if (vi.IsYUY2()) {
            *outcsp=FF_CSP_YUY2;
        } else if (vi.IsYV12()) {
            *outcsp=FF_CSP_420P;
        } else {
            *outcsp=FF_CSP_NULL;
        }

        applyPulldown=oldcfg.applyPulldown;

        enableBuffering=!!oldcfg.enableBuffering;
        bufferAhead=(enableBuffering ? oldcfg.bufferAhead : 0);
        bufferBack=(enableBuffering ? oldcfg.bufferBack : 0);

        if (bufferAhead == 0 && bufferBack == 0) {
            enableBuffering=false;
        }

        buffersNeeded=bufferAhead+1;
        input->numBuffers=numBuffers=
                              buffersNeeded+
                              bufferBack+
                              (enableBuffering && vi.num_frames != NUM_FRAMES ? 1 : 0)+
                              (applyPulldown == 1 ? 1 : 0);

        curInFrameNo=0;
        curOutFrameNo=0;
        curOutScaledFrameNo=0;
        backLimit=0;

        lastOutStopTime=-1;

        frameScaleNum=REFERENCE_TIME(NUM_FRAMES);
        frameScaleDen=REFERENCE_TIME(vi.num_frames);

        REFERENCE_TIME frameScaleGCD=lavc_gcd(frameScaleNum,frameScaleDen);

        frameScaleNum/=frameScaleGCD;
        frameScaleDen/=frameScaleGCD;

        if (frameScaleNum > frameScaleDen*60 ||
                frameScaleDen > frameScaleNum*60) {
            frameScaleNum=frameScaleDen=1;
        }

        inputRect=pict.rectClip;
        inputSar=pict.rectClip.sar;
        inputDar=pict.rectClip.dar();

        if (input->outputDar)
            pict.rectFull=pict.rectClip=outputRect=
                                            Trect(
                                                0,0,vi.width,vi.height,
                                                Rational((vi.height*input->outputDar.num)/double(vi.width*input->outputDar.den),32768));
        else if (input->outputSar) {
            pict.rectFull=pict.rectClip=outputRect=Trect(0,0,vi.width,vi.height,input->outputSar);
        } else if ((unsigned int)vi.width != inputRect.dx || (unsigned int)vi.height != inputRect.dy) {
            pict.rectFull=pict.rectClip=outputRect=Trect(0,0,vi.width,vi.height,inputSar);
        } else {
            outputRect=inputRect;
        }

        restart=true;
        deleteBuffers=true;
    }
}

HRESULT TimgFilterAvisynth::Tavisynth::process(TimgFilterAvisynth *self,TfilterQueue::iterator& it,TffPict &pict,const TavisynthSettings *cfg)
{
    HRESULT hr = S_FALSE;
    bool sequenceStart=(pict.fieldtype & FIELD_TYPE::MASK_SEQ) == FIELD_TYPE::SEQ_START;
    bool sequenceEnd=(pict.fieldtype & FIELD_TYPE::SEQ_END) == FIELD_TYPE::SEQ_END; // If the sequence starts & stops in the same frame, give stop precedence (again)
    bool isYV12=((pict.csp&FF_CSPS_MASK) == FF_CSP_420P);
    Tinput* input=self->input;

    if (sequenceStart) {
        passFirstThrough=true;
    }

    if (sequenceEnd) {
        passLastThrough=true;
    }

    if (passFirstThrough) {
        resetBuffers=true;
    }

    if (restart) {
        if (debugPrint) {
            DPRINTF(_l("TimgFilterAvisynth: (Re-)Starting playback"));
            DPRINTF(_l("TimgFilterAvisynth: FrameScale: %lli/%lli"),frameScaleNum,frameScaleDen);
        }

        skipAhead(passFirstThrough,true);

        restart=false;
    }

    if (deleteBuffers) {
        if (debugPrint && buffers) {
            DPRINTF(_l("TimgFilterAvisynth: Freeing buffers"));
        }

        if (buffers) {
            delete[] buffers;
        }
        buffers=NULL;

        deleteBuffers=false;
        resetBuffers=true;
    }

    if (resetBuffers) {
        if (debugPrint) {
            DPRINTF(_l("TimgFilterAvisynth: (Re-)Initializing buffers"));
        }

        hasPulldown=false;
        buffersFilled=0;
        curBufferNo=0;
        minAccessedFrame=INT_MAX;
        maxAccessedFrame=INT_MIN;
        ignoreAheadValue=true;

        if (!buffers) {
            input->buffers=buffers=new TframeBuffer[numBuffers+(applyPulldown != 0 ? 1 : 0)];
        }

        for (int bufNo=0; bufNo < numBuffers+(applyPulldown > 0 ? 1 : 0); bufNo++) {
            buffers[bufNo].frameNo=-1;
            buffers[bufNo].bytesPerPixel=input->cspBpp;
            buffers[bufNo].start=0;
            buffers[bufNo].stop=0;
            buffers[bufNo].fieldType=0;
            buffers[bufNo].ReleaseFrame();
        }

        resetBuffers=false;
    }

    TframeBuffer& curBuffer=buffers[curBufferNo];

    curBuffer.frameNo=curInFrameNo;

    if (applyPulldown != 1 || !(pict.fieldtype & FIELD_TYPE::MASK_INT)) {
        curBuffer.CreateFrame(input);
        curBuffer.start=pict.rtStart;
        curBuffer.stop=pict.rtStop;
        curBuffer.fieldType=pict.fieldtype;

        if (debugPrint) {
            DPRINTF(_l("TimgFilterAvisynth: Buffering frame %i, %10.6f - %10.6f = %10.6f %s"),curInFrameNo,curBuffer.start/10000.0,curBuffer.stop/10000.0,(curBuffer.stop-curBuffer.start)/10000.0,_l("--"));
        }
    } else {
        // Buffer frames with pulldown applied according to fieldtype interlace flags to get
        // constant frame rate TFF video
        //
        // Example:
        // +------------------- Number of previous frame (stored in pulldownBuffer)
        // | +----------------- Number of current frame
        // | | +--------------- TFF/BFF flag on previous frame
        // | | | +------------- TFF/BFF flag on current frame
        // | | | |       +----- Frame(s) produced by combining these top & bottom fields
        // | | | |       |
        // - - - - ------------
        // - 1 T T 1T 1B|
        // 1 2 T B 2T 2B|
        // 2 3 B B 2T 3B|
        // 3 4 B T 3T 4B|4T 4B|
        // 4 5 T T 5T 5B|
        // 5 6 T B 6T 6B|
        // 6 7 B B 6T 7B|
        // 7 8 B T 7T 8B|8T 8B|
        // 8 9 T T 9T 9B|
        // ...

        // Buffer to store the previous frame
        TframeBuffer& pulldownBuffer=buffers[numBuffers];

        // pulldownBuffer has a frameNo of -1 if it was just initialized
        bool hasPulldownBuffer=(pulldownBuffer.frameNo >= 0);

        bool lastTFF=(pulldownBuffer.fieldType & FIELD_TYPE::INT_TFF) != 0;
        bool curTFF=(pict.fieldtype & FIELD_TYPE::INT_TFF) != 0;

        REFERENCE_TIME curDuration=pict.rtStop-pict.rtStart;
        REFERENCE_TIME lastDuration=(hasPulldownBuffer ? pulldownBuffer.stop-pulldownBuffer.start : curDuration);

        // 3:2 pulldown has alternating long and short frame durations, figure out which is which
        REFERENCE_TIME shortDuration;
        REFERENCE_TIME longDuration;

        if (curDuration < lastDuration) {
            shortDuration=curDuration;
            longDuration=lastDuration;
        } else {
            shortDuration=lastDuration;
            longDuration=curDuration;
        }

        if (!hasPulldownBuffer) {
            // Just copy the first frame, whatever it is

            curBuffer.CreateFrame(input);
            curBuffer.start=pict.rtStart;
            curBuffer.stop=pict.rtStop;

            if (debugPrint) {
                DPRINTF(_l("TimgFilterAvisynth: Buffering frame %i, %10.6f - %10.6f = %10.6f %s"),curInFrameNo,curBuffer.start/10000.0,curBuffer.stop/10000.0,(curBuffer.stop-curBuffer.start)/10000.0,_l("TT"));
            }
        } else {
            // Combine fields into new frames and adjust the timestamps accordingly

            if (lastTFF) {
                // T -> x

                curBuffer.CreateFrame(input);

                char_t* fields;

                if (curTFF) {
                    // T -> T
                    curBuffer.start=pulldownBuffer.start+shortDuration;
                    curBuffer.stop=pulldownBuffer.stop+shortDuration;

                    fields=_l("TT");
                } else {
                    hasPulldown=true;

                    // T -> B
                    curBuffer.start=pulldownBuffer.start+shortDuration;
                    curBuffer.stop=pulldownBuffer.stop+shortDuration/2;

                    fields=_l("TB");
                }

                if (debugPrint) {
                    DPRINTF(_l("TimgFilterAvisynth: Buffering frame %i, %10.6f - %10.6f = %10.6f %s"),curInFrameNo,curBuffer.start/10000.0,curBuffer.stop/10000.0,(curBuffer.stop-curBuffer.start)/10000.0,fields);
                }
            } else {
                hasPulldown=true;

                // B -> x

                curBuffer.CombineFrame(input,false,false,pulldownBuffer.frame);

                if (!curTFF) {
                    // B -> B
                    curBuffer.start=pulldownBuffer.start+shortDuration/2;
                    curBuffer.stop=pulldownBuffer.stop+shortDuration/2;

                    if (debugPrint) {
                        DPRINTF(_l("TimgFilterAvisynth: Buffering frame %i, %10.6f - %10.6f = %10.6f %s"),curInFrameNo,curBuffer.start/10000.0,curBuffer.stop/10000.0,(curBuffer.stop-curBuffer.start)/10000.0,_l("BB"));
                    }
                } else {
                    // B -> T
                    curBuffer.start=pulldownBuffer.start+shortDuration/2;
                    curBuffer.stop=pulldownBuffer.stop;

                    if (debugPrint) {
                        DPRINTF(_l("TimgFilterAvisynth: Buffering frame %i, %10.6f - %10.6f = %10.6f %s"),curInFrameNo,curBuffer.start/10000.0,curBuffer.stop/10000.0,(curBuffer.stop-curBuffer.start)/10000.0,_l("BT"));
                    }

                    curBufferNo=(curBufferNo+1)%numBuffers;
                    buffersFilled++;
                    curInFrameNo++;

                    TframeBuffer& curBuffer2=buffers[curBufferNo];

                    curBuffer2.CreateFrame(input);
                    curBuffer2.frameNo=curInFrameNo;
                    curBuffer2.start=pulldownBuffer.start+shortDuration/2+shortDuration;
                    curBuffer2.stop=pulldownBuffer.stop+shortDuration;
                    curBuffer2.fieldType=FIELD_TYPE::INT_TFF;

                    if (debugPrint) {
                        DPRINTF(_l("TimgFilterAvisynth: Buffering frame %i, %10.6f - %10.6f = %10.6f %s"),curInFrameNo,curBuffer2.start/10000.0,curBuffer2.stop/10000.0,(curBuffer2.stop-curBuffer2.start)/10000.0,_l("BT"));
                    }
                }
            }
        }

        // Frames that had pulldown applied are always TFF
        curBuffer.fieldType=FIELD_TYPE::INT_TFF;

        pulldownBuffer.CreateField(input,true);
        pulldownBuffer.start=pict.rtStart;
        pulldownBuffer.stop=pict.rtStop;
        pulldownBuffer.fieldType=pict.fieldtype;
        pulldownBuffer.frameNo=0;
    }

    if (applyPulldown == 2) {
        // Twiddle timestamps on current frame

        // Buffer to store the previous frame properties
        TframeBuffer& lastBuffer=buffers[numBuffers];

        // lastBuffer has a frameNo of -1 if it was just initialized
        bool hasLastBuffer=(lastBuffer.frameNo >= 0);

        if (hasLastBuffer) {
            REFERENCE_TIME lastDuration=lastBuffer.stop-lastBuffer.start;
            REFERENCE_TIME curDuration=curBuffer.stop-curBuffer.start;
            REFERENCE_TIME avgDuration=(curDuration+lastDuration)/2;

            lastBuffer.start=curBuffer.start;
            lastBuffer.stop=curBuffer.stop;

            if (curDuration > lastDuration) {
                curBuffer.start+=(curDuration-avgDuration);
            } else {
                curBuffer.stop+=(avgDuration-curDuration);
            }
        } else {
            lastBuffer.start=curBuffer.start;
            lastBuffer.stop=curBuffer.stop;
        }

        lastBuffer.frameNo=0;
    }

    curBufferNo=(curBufferNo+1)%numBuffers;
    buffersFilled++;
    curInFrameNo++;

    if ((buffersFilled < buffersNeeded) && !passFirstThrough && !passLastThrough)
        // Not enough has been buffered, so don't return frames yet
    {
        return hr;
    }

    if (debugPrint) {
        if (passFirstThrough) {
            DPRINTF(_l("TimgFilterAvisynth: Passing through first frame"));
        }

        if (sequenceStart) {
            DPRINTF(_l("TimgFilterAvisynth: Start of MPEG2 sequence"));
        }

        if (sequenceEnd) {
            DPRINTF(_l("TimgFilterAvisynth: End of MPEG2 sequence, flushing buffers"));
        }
    }

    int minBuffers=(passFirstThrough || passLastThrough ? 0 : bufferAhead);

    while (buffers && buffersFilled > minBuffers) {
        int lastOutScaledFrameNo=(int)(REFERENCE_TIME(curOutFrameNo)*frameScaleNum/frameScaleDen);
        int targetOutScaledFrameNo=(int)(REFERENCE_TIME(curOutFrameNo+1)*frameScaleNum/frameScaleDen);
        int lastOutScaledFrameRem=(int)(REFERENCE_TIME(curOutFrameNo)*frameScaleNum%frameScaleDen);

        if (lastOutScaledFrameNo < targetOutScaledFrameNo) {
            buffersFilled--;
        }

        bool finalBuffer=(passLastThrough && buffersFilled == 0 && numBuffers > 1);

        if (curOutScaledFrameNo < targetOutScaledFrameNo) {
            curOutScaledFrameNo++;
        }

        if (curOutScaledFrameNo == targetOutScaledFrameNo || finalBuffer) {
            if (finalBuffer) {
                skipAhead(false,false);

                passFirstThrough=false;
                passLastThrough=false;
                ignoreAheadValue=true;
            }

            int requestedFrame=curOutFrameNo;

            // Set up request statistics
            input->numAccessedFrames=0;
            input->minAccessedFrame=INT_MAX;
            input->maxAccessedFrame=INT_MIN;
            input->curFrame=lastOutScaledFrameNo;
            input->backLimit=backLimit;

            if (debugPrint) {
                DPRINTF(_l("TimgFilterAvisynth: Requesting frame %i from Avisynth"),requestedFrame);
            }

            // Request frame from AviSynth script
            PVideoFrame frame=(*input->clip)->GetFrame(requestedFrame);

            isYV12=((self->outcsp&FF_CSPS_MASK) == FF_CSP_420P);

            // Evaluate request statistics
            if (input->numAccessedFrames > 0 && !sequenceStart && !passFirstThrough) {
                if (minAccessedFrame > input->minAccessedFrame) {
                    minAccessedFrame=input->minAccessedFrame;
                }

                if (ignoreAheadValue) {
                    ignoreAheadValue=false;
                } else if (maxAccessedFrame < input->maxAccessedFrame) {
                    maxAccessedFrame=input->maxAccessedFrame;
                }
            }

            char* bufPos=infoBuf;

            maxBufferBack=(minAccessedFrame < 0 ? -minAccessedFrame : 0);
            maxBufferAhead=(maxAccessedFrame > 0 ? maxAccessedFrame : 0);

            bufPos+=sprintf(
                        bufPos,
                        "Frame %6i: %3i back, %3i ahead; ",
                        lastOutScaledFrameNo,
                        maxBufferBack,
                        maxBufferAhead);

            if (hasPulldown) {
                bufPos+=sprintf(bufPos,"applying pulldown; ");
            }

            bufPos+=sprintf(bufPos,"requested ");

            int frameCount=input->numAccessedFrames;

            if (frameCount > 100) {
                frameCount=100;
            }

            if (frameCount == 0) {
                bufPos+=sprintf(bufPos,"none");
            } else
                for (int framePos=0; framePos < frameCount; framePos++) {
                    bufPos+=sprintf(bufPos,"%i ",lastOutScaledFrameNo+input->accessedFrames[framePos]);
                }

            *bufPos=0;

            if (debugPrint) {
                DPRINTFA("TimgFilterAvisynth: %s", infoBuf);
            }

            if (pict.diff[0] || outputRect!=self->pictRect) {
                // Copy resulting frame into current TffPict

                unsigned char *data[4];

                if (outputRect==self->pictRect) {
                    self->getNext(self->outcsp,pict,cfg->full,data);
                } else {
                    self->getNext(self->outcsp,pict,cfg->full,data,&outputRect);
                }

                if (outputRect != pict.rectFull) {
                    self->parent->dirtyBorder=1;
                }

                if (isYV12) {
                    TffPict::copy(data[0],self->stride2[0],frame->GetReadPtr(PLANAR_Y),frame->GetPitch(PLANAR_Y),frame->GetRowSize(PLANAR_Y),frame->GetHeight(PLANAR_Y));
                    TffPict::copy(data[1],self->stride2[1],frame->GetReadPtr(PLANAR_U),frame->GetPitch(PLANAR_U),frame->GetRowSize(PLANAR_U),frame->GetHeight(PLANAR_U));
                    TffPict::copy(data[2],self->stride2[2],frame->GetReadPtr(PLANAR_V),frame->GetPitch(PLANAR_V),frame->GetRowSize(PLANAR_V),frame->GetHeight(PLANAR_V));
                } else {
                    TffPict::copy(data[0],self->stride2[0],frame->GetReadPtr(),frame->GetPitch(),frame->GetRowSize(),frame->GetHeight());
                }
            } else {
                // Reference the resulting frame's data in current TffPict

                pict.csp=self->outcsp;

                if (isYV12) {
                    pict.data[0]=(unsigned char*)frame->GetReadPtr(PLANAR_Y);
                    pict.ro[0]=true;
                    pict.stride[0]=frame->GetPitch(PLANAR_Y);
                    pict.data[1]=(unsigned char*)frame->GetReadPtr(PLANAR_U);
                    pict.ro[1]=true;
                    pict.stride[1]=frame->GetPitch(PLANAR_U);
                    pict.data[2]=(unsigned char*)frame->GetReadPtr(PLANAR_V);
                    pict.ro[2]=true;
                    pict.stride[2]=frame->GetPitch(PLANAR_V);
                    pict.data[3]=NULL;
                    pict.stride[3]=0;
                } else {
                    pict.data[0]=(unsigned char*)frame->GetReadPtr();
                    pict.ro[0]=true;
                    pict.stride[0]=frame->GetPitch();
                    pict.data[1]=pict.data[2]=pict.data[3]=NULL;
                }

                pict.calcDiff();
            }

            // Calculate the new frame's timestamps

            TframeBuffer& lastOutFrameBuffer=buffers[findBuffer(buffers,numBuffers,lastOutScaledFrameNo)];
            REFERENCE_TIME lastDuration=lastOutFrameBuffer.stop-lastOutFrameBuffer.start;

            pict.fieldtype=lastOutFrameBuffer.fieldType;

            if (frameScaleNum == frameScaleDen) {
                // Use original timestamps if no frames are added or removed
                pict.rtStart=lastOutFrameBuffer.start;
                pict.rtStop=lastOutFrameBuffer.stop;
            } else {
                // Interpolate new timestamps
                pict.rtStart=lastOutFrameBuffer.start+lastDuration*(lastOutScaledFrameRem)/frameScaleDen;
                pict.rtStop=pict.rtStart+lastDuration*frameScaleNum/frameScaleDen;

                REFERENCE_TIME difference=pict.rtStart-lastOutStopTime;

                if (difference < 0) {
                    difference=-difference;
                }

                if (difference < lastDuration/4)
                    // Fill small gaps between consecutive frames
                {
                    pict.rtStart=lastOutStopTime;
                }

                if ((passFirstThrough || finalBuffer) && pict.rtStart < lastOutStopTime) {
                    // Make sure that a frame gets delivered when passFirstThrough or finalBuffer is true
                    pict.rtStart=lastOutStopTime;

                    if (pict.rtStop < pict.rtStart) {
                        pict.rtStop=pict.rtStart+lastDuration*frameScaleNum/(frameScaleDen*2);
                    }
                }
            }

            if ((frameScaleNum == frameScaleDen) ||
                    pict.rtStart >= lastOutStopTime && pict.rtStop >= pict.rtStart) {
                // Only deliver frames with strictly increasing timestamps

                if (debugPrint) {
                    DPRINTF(_l("TimgFilterAvisynth: Delivering frame %i; %10.6f - %10.6f = %10.6f"),curOutFrameNo,pict.rtStart/10000.0,pict.rtStop/10000.0,(pict.rtStop-pict.rtStart)/10000.0);
                }

                if (input->outputDar) {
                    pict.setDar(input->outputDar);
                } else if (input->outputSar) {
                    pict.setSar(input->outputSar);
                }

                if (csp_isRGB_RGB(pict.csp)) {
                    pict.csp|=FF_CSP_FLAGS_VFLIP;
                }

                memcpy(&pict.cspInfo,csp_getInfo(pict.csp),sizeof(pict.cspInfo));
                if (buffersFilled > minBuffers) {
                    TffPict tempPict=pict;

                    ++it;
                    self->parent->processAndDeliverSample(it,tempPict); // we have to deliver the additional frame that has been created (pict will be taken care of by the caller method)
                    --it;
                } else {
                    ++it;
                    hr = self->parent->processSample(it,pict);
                    --it;
                }
            } else if (debugPrint) {
                DPRINTF(_l("TimgFilterAvisynth: Not delivering frame %i; %10.6f - %10.6f = %10.6f"),curOutFrameNo,pict.rtStart/10000.0,pict.rtStop/10000.0,(pict.rtStop-pict.rtStart)/10000.0);
            }

            curOutFrameNo++;

            lastOutStopTime=pict.rtStop;
        }

        if (passFirstThrough) {
            skipAhead(false,false);
        }
    }

    if (sequenceEnd) {
        skipAhead(false,false);
    }

    return hr;
}

//===================================== TimgFilterAvisynth =====================================
TimgFilterAvisynth::TimgFilterAvisynth(IffdshowBase *Ideci,Tfilters *Iparent):TimgFilter(Ideci,Iparent)
{
    avisynth=0;
    input=0;
    outFmtInput=0;
    outcsp=FF_CSP_NULL;
    oldcfg.script[0]='\0';
}

TimgFilterAvisynth::~TimgFilterAvisynth()
{
    if (avisynth) {
        delete avisynth;
    }
    if (input) {
        delete input;
    }
    if (outFmtInput) {
        delete outFmtInput;
    }
}

void TimgFilterAvisynth::done(void)
{
    if (avisynth) {
        delete avisynth;
    }
    avisynth=0;
    if (input) {
        delete input;
    }
    input=0;
    if (outFmtInput) {
        delete outFmtInput;
    }
    outFmtInput=0;
}

void TimgFilterAvisynth::onSizeChange(void)
{
    if (debugPrint) {
        DPRINTF(_l("TimgFilterAvisynth: onSizeChange"));
    }

    oldcfg.script[0]='\0';
}

void TimgFilterAvisynth::onSeek(void)
{
    if (debugPrint) {
        DPRINTF(_l("TimgFilterAvisynth: onSeek"));
    }

    if (avisynth) {
        avisynth->skipAhead(false,true);
    }
}

void TimgFilterAvisynth::onFlush(void)
{
    if (debugPrint) {
        DPRINTF(_l("TimgFilterAvisynth: onFlush"));
    }

    if (avisynth) {
        avisynth->skipAhead(false,true);
    }
}

void TimgFilterAvisynth::onStop(void)
{
    if (debugPrint) {
        DPRINTF(_l("TimgFilterAvisynth: onStop"));
    }

    if (avisynth) {
        avisynth->skipAhead(false,true);
    }
}

void TimgFilterAvisynth::reset(void)
{
    if (debugPrint) {
        DPRINTF(_l("TimgFilterAvisynth: reset"));
    }

    oldcfg.script[0]='\0';
}

/*
If this method returns false when the script is empty there won't be no video at all...
-- Leak

bool TimgFilterAvisynth::is(const TffPictBase &pict,const TfilterSettingsVideo *cfg0)
{
 const TavisynthSettings *cfg=(const TavisynthSettings*)cfg0;
 return super::is(pict,cfg) && cfg->script[0];
}
*/

uint64_t TimgFilterAvisynth::getWantedCsp(const TavisynthSettings *cfg) const
{
    return (cfg->inYV12?FF_CSP_420P:0)|(cfg->inYUY2?FF_CSP_YUY2:0)|(cfg->inRGB24?FF_CSP_RGB24:0)|(cfg->inRGB32?FF_CSP_RGB32:0);
}

uint64_t TimgFilterAvisynth::getSupportedInputColorspaces(const TfilterSettingsVideo *cfg0) const
{
    const TavisynthSettings *cfg=(const TavisynthSettings*)cfg0;
    return getWantedCsp(cfg);
}

uint64_t TimgFilterAvisynth::getSupportedOutputColorspaces(const TfilterSettingsVideo *cfg) const
{
    return outcsp?outcsp:FF_CSP_420P|FF_CSP_YUY2|FF_CSP_RGB24|FF_CSP_RGB32;
}

bool TimgFilterAvisynth::getOutputFmt(TffPictBase &pict,const TfilterSettingsVideo *cfg0)
{
    if (is(pict,cfg0)) {
        const TavisynthSettings *cfg=(const TavisynthSettings*)cfg0;

        try {
            Trect r=pict.getRect(cfg->full,cfg->half);

            if (!outFmtInput) {
                outFmtInput=new Tinput();
            }

            outFmtInput->numBuffers=0;
            outFmtInput->buffers=0;
            outFmtInput->numAccessedFrames=0;
            outFmtInput->minAccessedFrame=INT_MAX;
            outFmtInput->maxAccessedFrame=INT_MIN;
            outFmtInput->curFrame=0;
            outFmtInput->backLimit=0;
            outFmtInput->dx=r.dx;
            outFmtInput->dy=r.dy;
            outFmtInput->csp=getWantedCsp(cfg);
            outFmtInput->cspBpp=(outFmtInput->csp ? csp_getInfo(outFmtInput->csp)->Bpp : 0);
            outFmtInput->src[0]=NULL;

            lavc_reduce(&outFmtInput->fpsnum,&outFmtInput->fpsden,deciV->getAVIfps1000_2(),1000,65000);

            if (!avisynth) {
                avisynth=new Tavisynth;
            }

            avisynth->setOutFmt(cfg,outFmtInput,pict);
        } catch (Tavisynth::AvisynthError &err) {
            DPRINTFA("%s",err.msg);
        }

        return true;
    } else {
        return false;
    }
}

const char* TimgFilterAvisynth::getInfoBuffer(void)
{
    if (avisynth && oldcfg.is && oldcfg.show) {
        return avisynth->infoBuf;
    } else if (deciV->getMovieFOURCC() == 0x53495641 /* "AVIS" */) {
        return "disabled (input is an AviSynth script)";
    } else {
        return "unavailable";
    }
}

HRESULT TimgFilterAvisynth::process(TfilterQueue::iterator it,TffPict &pict,const TfilterSettingsVideo *cfg0)
{
    HRESULT hr = S_OK;
    if (is(pict,cfg0)) {
        const TavisynthSettings *cfg=(const TavisynthSettings*)cfg0;

        init(pict,cfg->full,cfg->half);

        uint64_t wantedcsp=getWantedCsp(cfg);

        if (wantedcsp!=0) {
            bool reset=!cfg->equal(oldcfg);

            if (reset) {
                done();
            }

            if (!input) {
                input=new Tinput();
            }

            getCur(wantedcsp,pict,cfg->full,input->src);

            input->stride1=stride1;

            if (reset) {
                oldcfg=*cfg;

                try {
                    debugPrint=(strnicmp(cfg->script,_l("#DEBUG"),6) == 0);
                    input->numBuffers=0;
                    input->buffers=0;
                    input->numAccessedFrames=0;
                    input->minAccessedFrame=INT_MAX;
                    input->maxAccessedFrame=INT_MIN;
                    input->curFrame=0;
                    input->backLimit=0;
                    input->dx=dx1[0];
                    input->dy=dy1[0];
                    input->csp=csp1;
                    input->cspBpp=csp_getInfo(csp1)->Bpp;

                    lavc_reduce(&input->fpsnum,&input->fpsden,deciV->getAVIfps1000_2(),1000,65000);

                    if (!avisynth) {
                        avisynth=new Tavisynth;
                    }

                    avisynth->init(oldcfg,input,&outcsp,pict);
                } catch (Tavisynth::AvisynthError &err) {
                    deciV->shortOSDmessage(text<char_t>(err.msg),100);
                    DPRINTFA("%s",err.msg);
                }
            }

            if (avisynth && input && input->clip) {
                hr = avisynth->process(this,it,pict,cfg);
            }
        }
    }

    return hr;
}

int TimgFilterAvisynth::getMaxBufferAhead()
{
    return maxBufferAhead;
}
int TimgFilterAvisynth::getMaxBufferBack()
{
    return maxBufferBack;
}
