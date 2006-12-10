1. About ffdshow video decoder

   ffdshow is a DirectShow decoding filter for decompressing DIVX movies.
   It uses libavcodec from FFmpeg project for video decompression and
   compression (It can use xvidcore.dll installed with xvid codec, too.),
   postprocessing code from MPlayer to enhance visual quality of low
   bitrate movies, and is based on original DirectShow filter from Xvid,
   which is GPL'ed educational implementation of MPEG4 encoder.

2. Features

   - fast video decompression using optimized MMX, SSE and 3DNow! code
   - support for different codecs: Xvid and all DIVX versions
   - support for H.264 and H.264/AVC (Advanced Video Coding)
   - additional support for MSMPEG4v1, MSMPEG4v2, MSMPEG4v3 and H.263
   - can act as generic postprocessing filter for other decoders like MPEG1 or
     MPEG2
   - image postprocessing for higher playback quality
   - automatic quality control: automatically reduces postprocessing level when
     CPU load is high
   - hue, saturation and luminance correction (MMX optimized)
   - two sharpening filters: xsharpen and unsharp mask
   - blur and temporal smoother
   - tray icon with menu and quick access to configuration dialog
   - noising with two selectable algorithms
   - resizing and aspect ratio changing
   - subtitles
   - completely free software: ffdshow is distributed under GPL

3. Configuration

   To be written.

4. Web links

   ffdshow:
   http://ffdshow.sourceforge.net/tikiwiki/
   http://sourceforge.net/projects/ffdshow/
   http://www.ffdshow.info/

   Xvid:
   http://www.xvid.org/

   FFmpeg:
   http://ffmpeg.mplayerhq.hu/

   libmpeg2:
   http://libmpeg2.sourceforge.net/

   MPlayer:
   http://www.mplayerhq.hu/

   xsharpen, unsharp mask, msharpen, hue and saturation code
   http://sauron.mordor.net/dgraft/index.html

   or at doom9:
   http://forum.doom9.org/showthread.php?t=98600

5. Copying

   All used sources (except of cpu utilization detection routine) and ffdshow
   itself are distributed under GPL. See copying.txt


   Milan Cutka <milan_cutka@yahoo.com>