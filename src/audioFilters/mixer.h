/*
  Mixer

  Mixer and gain control class. Smooth gain control based on ac3 time window,
  so delayed samples are also required for operation.

  Usage.
    Create instance of a mixer, set input and output modes, set matrix
    directly or call calc_matrix() function, then call mix() function.

    'level' - desired output level. It is guaranteed that Samples will not
      exceed it.
    'clev', 'slev', 'lfelev' are params for matrix calculation calc_matrix().
    'master', 'gain', 'dynrng' are used in gain control and matrix-independent.
    'normalize' flag controls gain control behavior. True means one-pass
      normalization. So at at the beginning mixing use 'gain' = 'master'.
      When overflow occur gain is decreased and so on. When 'normalize' = false
      then after overflow gain begins to increase bit by bit until it
      reaches 'master' again or other overflow occur.
    'auto_gain' - automatic gain control. It will automatically lower gain
      level on overload and restore it back then.
    'voice_control' - (only when stereo input) enables voice control when
      stereo input. Amplifies in-phase signal and route it to center
      speaker if present. Only when auto_matrix = true.
    'expand_stereo' - (only when stereo input) enables surround control when
      stereo input. Amplifies out-of-phase signal and route it to surround
      speakers if present. Only when auto_matrix = true.



    calc_matrix() - calc mixing matrix complied with ac3 standart (not normalized)
    normalize() - normalizes matrix so no overflow at output if no overflow at input.
    reset() - reset time window, reset to 'master' to 'gain' and 'dynrng' to 1.0.
*/

#ifndef MIXER_H
#define MIXER_H

#include "TsampleFormat.h"
#include "TmixerSettings.h"
#include "TaudioFilter.h"
#include "IffdshowDecAudio.h"

class TmixerMatrix
{
private:
 static const double LEVEL_PLUS6DB,LEVEL_PLUS3DB,LEVEL_3DB,LEVEL_45DB,LEVEL_6DB;
 // channel numbers
 // used as index in arrays
 enum
  {
   CH_L        =0, // Left channel
   CH_R        =1, // Right channel
   CH_C        =2, // Center channel
   CH_LFE      =3, // LFE channel
   CH_SL       =4, // Surround left channel
   CH_SR       =5, // Surround right channel
   CH_NONE     =6, // indicates that channel is not used in channel order

   CH_M        =2, // Mono channel = center channel
   CH_S        =4  // Surround channel for x/1 modes
  };
protected:
 static const int NCHANNELS=6;
 // mixing matrix 6-to-6 channels
 TsampleFormat calc_matrix(const TsampleFormat &insf,const TmixerSettings *cfg);
public:
 typedef double mixer_matrix_t[NCHANNELS][NCHANNELS];
 mixer_matrix_t matrix;

 TmixerMatrix(void);
};

template<class sample_t,class div_t,div_t div,div_t div2> class Mixer :public TmixerMatrix
{
private:
 typedef typename TsampleFormatInfo<sample_t>::helper_t helper_t;
 typedef helper_t mixer_matrix_t[NCHANNELS][NCHANNELS];
 mixer_matrix_t matrix;
 unsigned int rows,cols;

 template<unsigned int colsT,unsigned int rowsT> static void mix(size_t nsamples,const sample_t *insamples,sample_t *outsamples,const mixer_matrix_t &matrix)
  {
   for (unsigned int nsample=0;nsample<nsamples;insamples+=rowsT,outsamples+=colsT,nsample++)
    for (unsigned int i=0;i<colsT;i++)
     {
      helper_t sum=0;
      for (unsigned int j=0;j<rowsT;j++)
       sum+=(insamples[j]*matrix[j][i])/helper_t(div/div2);
      outsamples[i]=TsampleFormatInfo<sample_t>::limit(sum);
     }
  }
 typedef void (*TmixFn)(size_t nsamples,const sample_t *insamples,sample_t *outsamples,const mixer_matrix_t &matrix);
 TmixFn mixFn;

 TsampleFormat calc_matrix(const TsampleFormat &insf,const TmixerSettings *cfg,const TmixerMatrix::mixer_matrix_t* *matrixPtr)
  {
   TsampleFormat outsf=TmixerMatrix::calc_matrix(insf,cfg);
   *matrixPtr=&this->TmixerMatrix::matrix;
   static const int mspeakers[]={SPEAKER_FRONT_LEFT,SPEAKER_FRONT_RIGHT,SPEAKER_FRONT_CENTER,SPEAKER_LOW_FREQUENCY,SPEAKER_BACK_LEFT|SPEAKER_BACK_CENTER,SPEAKER_BACK_RIGHT};
   rows=0;
   int inmask=insf.makeChannelMask(),outmask=outsf.makeChannelMask();
   for (int i=0;i<NCHANNELS;i++)
    if (inmask&mspeakers[i])
     {
      cols=0;
      for (int j=0;j<NCHANNELS;j++)
       if (outmask&mspeakers[j])
        {
         matrix[rows][cols]=helper_t(TmixerMatrix::matrix[i][j]*div/div2);
         cols++;
        }
      rows++;
     }
   static const TmixFn mixFns[6][6]=
    {
     mix<1,1>,mix<1,2>,mix<1,3>,mix<1,4>,mix<1,5>,mix<1,6>,
     mix<2,1>,mix<2,2>,mix<2,3>,mix<2,4>,mix<2,5>,mix<2,6>,
     mix<3,1>,mix<3,2>,mix<3,3>,mix<3,4>,mix<3,5>,mix<3,6>,
     mix<4,1>,mix<4,2>,mix<4,3>,mix<4,4>,mix<4,5>,mix<4,6>,
     mix<5,1>,mix<5,2>,mix<5,3>,mix<5,4>,mix<5,5>,mix<5,6>,
     mix<6,1>,mix<6,2>,mix<6,3>,mix<6,4>,mix<6,5>,mix<6,6>,
    };
   mixFn=mixFns[cols-1][rows-1];
   return outsf;
  }
 unsigned int oldnchannels,oldchannelmask;int oldsf;
 TmixerSettings oldcfg;
 Tbuffer buf;
 TsampleFormat outfmt;
public:
 IffdshowDecAudio *deciA;

 Mixer(void)
  {
   oldnchannels=0;oldchannelmask=0xffffffff;oldsf=TsampleFormat::SF_NULL;
   oldcfg.out=-1;
  }
 void process(TsampleFormat &fmt,sample_t* &samples1,size_t &numsamples,const TmixerSettings *cfg,const TmixerMatrix::mixer_matrix_t* *matrixPtr)
  {
   if (oldnchannels!=fmt.nchannels || oldchannelmask!=fmt.channelmask || oldsf!=fmt.sf || !cfg->equal(oldcfg))
    {
     oldnchannels=fmt.nchannels;oldchannelmask=fmt.channelmask;oldsf=fmt.sf;oldcfg=*cfg;
     outfmt=calc_matrix(fmt,cfg,matrixPtr);
    }

   TsampleFormat fmt2=fmt;
   fmt2.setChannels(outfmt.nchannels,outfmt.channelmask);
   sample_t *samples2=(sample_t*)TaudioFilter::alloc_buffer(fmt2,numsamples,buf);

   mixFn(numsamples,samples1,samples2,matrix);

   fmt=fmt2;
   samples1=samples2;
  }
};

#endif
