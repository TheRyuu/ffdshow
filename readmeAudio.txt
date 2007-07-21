1. About ffdshow audio decoder

    ffdshow is an open source directShow filter and VFW codec mainly used
    for the fast and high-quality decoding of video in the MPEG-4 ASP
    (e.g. encoded with DivX, Xvid or FFmpeg MPEG-4) and AVC (H.264)
    formats, but supporting numerous other video and audio formats as well.

    It has the ability to manipulate audio with effects like an equalizer,
    volume control, a Dolby decoder, reverb, Winamp DSP plugins, and more.

2. Features

   - support for most used codecs: AC3, AAC, DTS, MP1/2/3, and Vorbis
   - additional support for LPCM, TTA, QDM2, ADPCM, IMC or ATRAC3 and more...
   - Dolby decoder
   - audio filters: equalizer, volume, reverb, mixer, equalizer and convolver
   - multichannel audio processing by Winamp plugins has been added
   - audio decoder AC3 output: the new check box
     "Encode only multichannel streams" allows sending multichannel audio
     (which otherwise cannot be digitally sent) to AV-amp in AC3-SPDIF
   - support for SPDIF on Windows Vista has been added
     ("Audio decoder configuration" -> "Output" -> "Connect to:")

3. ffdshow-tryouts

   ffdshow was originally developed by Milan Cutka. Since Milan Cutka stopped
   updating in 2006, we launched a new project 'ffdshow-tryouts'.

4. Configuration

   - the installer can set speaker configuration
     It loads the setting of the OS (control panel) as default.

5. Web links

   ffdshow-tryouts:
   http://ffdshow-tryout.sourceforge.net/
   http://sourceforge.net/project/showfiles.php?group_id=173941

   ffdshow:
   http://ffdshow.sourceforge.net/tikiwiki/
   http://sourceforge.net/projects/ffdshow/

   FFmpeg:
   http://ffmpeg.mplayerhq.hu/

   MPlayer:
   http://www.mplayerhq.hu/

   Doom9:
   http://forum.doom9.org/showthread.php?t=120465

   or Wikipedia, the free encyclopedia:
   http://en.wikipedia.org/wiki/Ffdshow

6. Copying

   All used sources (except of cpu utilization detection routine) and ffdshow
   itself are distributed under GPL. See copying.txt


   Milan Cutka <milan_cutka@yahoo.com>
   ffdshow-tryouts
