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

//========================== TimgFilterAvisynth::Tffdshow_source ===============================
AVS_Value AVSC_CC TimgFilterAvisynth::Tffdshow_source::Create(AVS_ScriptEnvironment *env, AVS_Value args, void *user_data)
{
 Tc_createStruct *cs=(Tc_createStruct*)user_data;
 AVS_FilterInfo *fi;
 AVS_Clip *new_clip=cs->first->avs_new_c_filter(env,&fi,args,0);
 Tffdshow_source *filter=new Tffdshow_source(cs->second,(VideoInfo&)fi->vi,cs->first);
 fi->user_data=filter;
 fi->get_frame=get_frame;
 fi->get_parity=get_parity;
 fi->set_cache_hints=set_cache_hints;
 fi->free_filter=free_filter;
 AVS_Value v;cs->first->avs_set_to_clip(&v, new_clip);
 cs->first->avs_release_clip(new_clip);
 return v; 
}

void AVSC_CC TimgFilterAvisynth::Tffdshow_source::free_filter(AVS_FilterInfo *fi)
{
 if (fi && fi->user_data)
  delete (Tffdshow_source*)fi->user_data;
}

TimgFilterAvisynth::Tffdshow_source::Tffdshow_source(const Tinput *Iinput,VideoInfo &Ivi,IScriptEnvironment *Ienv):
 input(Iinput),
 vi(Ivi),
 env(Ienv)
{
 memset(&vi,0,sizeof(VideoInfo));
 vi.width=input->dx;
 vi.height=input->dy;
 vi.fps_numerator=input->fpsnum;
 vi.fps_denominator=input->fpsden;
 vi.num_frames=NUM_FRAMES;
 switch (input->csp&FF_CSPS_MASK)
  {
   case FF_CSP_420P :vi.pixel_type=AVS_CS_YV12 ;break;
   case FF_CSP_YUY2 :vi.pixel_type=AVS_CS_YUY2 ;break;
   case FF_CSP_RGB32:vi.pixel_type=AVS_CS_BGR32;break;
   case FF_CSP_RGB24:vi.pixel_type=AVS_CS_BGR24;break;
  }
}

AVS_VideoFrame* AVSC_CC TimgFilterAvisynth::Tffdshow_source::get_frame(AVS_FilterInfo *fi, int n)
{
 Tffdshow_source *filter=(Tffdshow_source*)fi->user_data;
 PVideoFrame frame(filter->env,filter->env->avs_new_video_frame_a(*filter->env,&filter->vi,16));
 if (filter->input->src[0])
  switch (filter->input->csp&FF_CSPS_MASK)
   {
    case FF_CSP_420P:
     TffPict::copy(frame->GetWritePtr(PLANAR_Y),frame->GetPitch(PLANAR_Y),filter->input->src[0],filter->input->stride1[0],filter->input->dx  ,filter->input->dy  );
     TffPict::copy(frame->GetWritePtr(PLANAR_U),frame->GetPitch(PLANAR_U),filter->input->src[1],filter->input->stride1[1],filter->input->dx/2,filter->input->dy/2);
     TffPict::copy(frame->GetWritePtr(PLANAR_V),frame->GetPitch(PLANAR_V),filter->input->src[2],filter->input->stride1[2],filter->input->dx/2,filter->input->dy/2);
     break;
    case FF_CSP_YUY2: 
     TffPict::copy(frame->GetWritePtr(),frame->GetPitch(),filter->input->src[0],filter->input->stride1[0],filter->input->dx*filter->input->cspBpp,filter->input->dy);
     break;
    case FF_CSP_RGB24:
    case FF_CSP_RGB32:
     TffPict::copy(frame->GetWritePtr(),frame->GetPitch(),filter->input->src[0]+filter->input->stride1[0]*(filter->input->dy-1),-filter->input->stride1[0],filter->input->dx*filter->input->cspBpp,filter->input->dy);
     break;
  }  
 return frame;
}

//================================ TimgFilterAvisynth::Tavisynth ===============================
TimgFilterAvisynth::Tavisynth::PClip* TimgFilterAvisynth::Tavisynth::createClip(const TavisynthSettings *cfg,const Tffdshow_source::Tinput *input)
{
 if (!env) env=CreateScriptEnvironment(AVISYNTH_INTERFACE_VERSION);
 if (env)
  {
   Tffdshow_source::Tc_createStruct cs(env,input);
   env->AddFunction("ffdshow_source","",Tffdshow_source::Create,(void*)&cs);
   char script[2048];sprintf(script,"%s%s",cfg->ffdshowSource?"ffdshow_source()\n":"",(const char*)text<char>(cfg->script));
   AVSValue eval_args[]={script,"ffdshow_filter_avisynth_script"};
   try 
    {
     AVSValue val=env->Invoke("Eval",AVSValue(eval_args,2));
     if (val.IsClip())
      return new PClip(val,env);
    }
   catch (AvisynthError &err)
    {
     throw err;
    } 
  }
 return NULL; 
}

void TimgFilterAvisynth::Tavisynth::setOutFmt(const TavisynthSettings *cfg,const Tffdshow_source::Tinput *input,TffPictBase &pict)
{
 if (PClip *clip=createClip(cfg,input))
  {
   const VideoInfo &vi=(*clip)->GetVideoInfo();
   pict.rectFull=pict.rectClip=Trect(0,0,vi.width,vi.height,pict.rectFull.sar);
   delete clip;
  }
}

void TimgFilterAvisynth::Tavisynth::done(void)
{
 if (clip) delete clip;clip=NULL;
 if (env) delete env;env=NULL;
}

void TimgFilterAvisynth::Tavisynth::init(const TavisynthSettings &oldcfg,const Tffdshow_source::Tinput &input,int *outcsp)
{
 clip=createClip(&oldcfg,&input);
 if (clip)
  {
   const VideoInfo &vi=(*clip)->GetVideoInfo();
   if      (vi.IsRGB24()) *outcsp=FF_CSP_RGB24|FF_CSP_FLAGS_VFLIP;
   else if (vi.IsRGB32()) *outcsp=FF_CSP_RGB32|FF_CSP_FLAGS_VFLIP;
   else if (vi.IsYUY2())  *outcsp=FF_CSP_YUY2;
   else if (vi.IsYV12())  *outcsp=FF_CSP_420P;
   else                   *outcsp=FF_CSP_NULL;
   outrect=Trect(0,0,vi.width,vi.height);
   if (vi.num_frames==NUM_FRAMES && REFERENCE_TIME(vi.fps_numerator)*input.fpsden!=REFERENCE_TIME(vi.fps_denominator)*input.fpsnum)
    {
     fpsscaleNum=REFERENCE_TIME(vi.fps_denominator)*input.fpsnum;
     fpsscaleDen=REFERENCE_TIME(vi.fps_numerator)*input.fpsden;
    } 
   else 
    fpsscaleNum=fpsscaleDen=1;
  }
}

void TimgFilterAvisynth::Tavisynth::process(TimgFilterAvisynth *self,TffPict &pict,const TavisynthSettings *cfg)
{
 int currentFrame=self->deci->getParam2(IDFF_currentFrame);
 PVideoFrame frame=(*clip)->GetFrame(currentFrame);
 if (pict.diff[0] || outrect!=self->pictRect)
  {
   unsigned char *data[4];
   if (outrect==self->pictRect)
    self->getNext(self->outcsp,pict,cfg->full,data);
   else
    self->getNext(self->outcsp,pict,cfg->full,data,&outrect);
   if ((pict.csp&FF_CSPS_MASK)==FF_CSP_420P)
    {
     TffPict::copy(data[0],self->stride2[0],frame->GetReadPtr(PLANAR_Y),frame->GetPitch(PLANAR_Y),frame->GetRowSize(PLANAR_Y),frame->GetHeight(PLANAR_Y));
     TffPict::copy(data[1],self->stride2[1],frame->GetReadPtr(PLANAR_U),frame->GetPitch(PLANAR_U),frame->GetRowSize(PLANAR_U),frame->GetHeight(PLANAR_U));
     TffPict::copy(data[2],self->stride2[2],frame->GetReadPtr(PLANAR_V),frame->GetPitch(PLANAR_V),frame->GetRowSize(PLANAR_V),frame->GetHeight(PLANAR_V));
    }
   else
    TffPict::copy(data[0],self->stride2[0],frame->GetReadPtr(),frame->GetPitch(),frame->GetRowSize(),frame->GetHeight());
  } 
 else
  {
   pict.csp=self->outcsp;
   if ((pict.csp&FF_CSPS_MASK)==FF_CSP_420P)
    {
     pict.data[0]=(unsigned char*)frame->GetReadPtr(PLANAR_Y);pict.ro[0]=true;pict.stride[0]=frame->GetPitch(PLANAR_Y);
     pict.data[1]=(unsigned char*)frame->GetReadPtr(PLANAR_U);pict.ro[1]=true;pict.stride[1]=frame->GetPitch(PLANAR_U);
     pict.data[2]=(unsigned char*)frame->GetReadPtr(PLANAR_V);pict.ro[2]=true;pict.stride[2]=frame->GetPitch(PLANAR_V);
     pict.data[3]=NULL;pict.stride[3]=0;
    }
   else
    {
     pict.data[0]=(unsigned char*)frame->GetReadPtr();pict.ro[0]=true;pict.stride[0]=frame->GetPitch();
     pict.data[1]=pict.data[2]=pict.data[3]=NULL;
    }  
   pict.calcDiff(); 
  }
 pict.rtStart=fpsscaleNum*pict.rtStart/fpsscaleDen; 
 pict.rtStop=fpsscaleNum*pict.rtStop/fpsscaleDen; 
}

//===================================== TimgFilterAvisynth =====================================
TimgFilterAvisynth::TimgFilterAvisynth(IffdshowBase *Ideci,Tfilters *Iparent):TimgFilter(Ideci,Iparent)
{
 avisynth=NULL;
 oldcfg.script[0]='\0';outcsp=FF_CSP_NULL;
}
TimgFilterAvisynth::~TimgFilterAvisynth()
{
 if (avisynth) delete avisynth;avisynth=NULL;
}
void TimgFilterAvisynth::done(void)
{
 if (avisynth) avisynth->done();
}
void TimgFilterAvisynth::onSizeChange(void)
{
 done();oldcfg.script[0]='\0';
}

bool TimgFilterAvisynth::is(const TffPictBase &pict,const TfilterSettingsVideo *cfg0)
{
 const TavisynthSettings *cfg=(const TavisynthSettings*)cfg0;
 return super::is(pict,cfg) && cfg->script[0];
}

int TimgFilterAvisynth::getWantedCsp(const TavisynthSettings *cfg) const
{
 return (cfg->inYV12?FF_CSP_420P:0)|(cfg->inYUY2?FF_CSP_YUY2:0)|(cfg->inRGB24?FF_CSP_RGB24:0)|(cfg->inRGB32?FF_CSP_RGB32:0);
} 

int TimgFilterAvisynth::getSupportedInputColorspaces(const TfilterSettingsVideo *cfg0) const
{
 const TavisynthSettings *cfg=(const TavisynthSettings*)cfg0;
 return getWantedCsp(cfg);
}
int TimgFilterAvisynth::getSupportedOutputColorspaces(const TfilterSettingsVideo *cfg) const
{
 return outcsp?outcsp:FF_CSP_420P|FF_CSP_YUY2|FF_CSP_RGB24|FF_CSP_RGB32;
}

bool TimgFilterAvisynth::getOutputFmt(TffPictBase &pict,const TfilterSettingsVideo *cfg0)
{
 if (is(pict,cfg0))
  {
   const TavisynthSettings *cfg=(const TavisynthSettings*)cfg0;
   try
    {
     Trect r=pict.getRect(cfg->full,cfg->half);
     Tffdshow_source::Tinput input;
     input.dx=r.dx;
     input.dy=r.dy;
     lavc_reduce(&input.fpsnum,&input.fpsden,deciV->getAVIfps1000_2(),1000,65000);
     input.csp=getWantedCsp(cfg);
     input.src[0]=NULL;
     if (!avisynth) avisynth=new Tavisynth;
     avisynth->setOutFmt(cfg,&input,pict);
    }
   catch (Tavisynth::AvisynthError &err)
    {
    }
   return true;
  }
 else
  return false;   
} 
HRESULT TimgFilterAvisynth::process(TfilterQueue::iterator it,TffPict &pict,const TfilterSettingsVideo *cfg0)
{
 if (is(pict,cfg0))
  {
   const TavisynthSettings *cfg=(const TavisynthSettings*)cfg0;
   init(pict,cfg->full,cfg->half);
   int wantedcsp=getWantedCsp(cfg);
   if (wantedcsp!=0)
    {
     getCur(wantedcsp,pict,cfg->full,input.src);

     input.stride1=stride1;
     if (!cfg->equal(oldcfg))
      {
       done();
       oldcfg=*cfg;
       try 
        {
         input.dx=dx1[0];input.dy=dy1[0];
         input.csp=csp1;input.cspBpp=csp_getInfo(csp1)->Bpp;
         lavc_reduce(&input.fpsnum,&input.fpsden,deciV->getAVIfps1000_2(),1000,65000);
         if (!avisynth) avisynth=new Tavisynth;
         avisynth->init(oldcfg,input,&outcsp);
         deciV->drawOSD(0,50,_l("")); 
        }
       catch (Tavisynth::AvisynthError &err)
        {
         deciV->drawOSD(0,50,text<char_t>(err.msg));
        } 
      }
     if (avisynth && avisynth->clip)
      avisynth->process(this,pict,cfg);
    }  
  }  
 return parent->deliverSample(++it,pict);
}
