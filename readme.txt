1. About ffdshow video decoder

    ffdshow is an open source directShow filter and VFW codec mainly used
    for the fast and high-quality decoding of video in the MPEG-4 ASP
    (e.g. encoded with DivX, Xvid or FFmpeg MPEG-4) and AVC (H.264)
    formats, but supporting numerous other video and audio formats as well.

    ffdshow can be configured to display subtitles, to enable or disable
    various built-in codecs, to grab screenshots, to enable keyboard control,
    and to enhance movies with increased resolution, sharpness, and many other
    post-processing filters.Some of the postprocessing is borrowed from the
    MPlayer project and AviSynth filters.

2. Features

   - fast video decompression using optimized MMX, SSE and 3DNow! code
   - support for different codecs: Xvid and all DIVX versions
   - support for H.264/AVC (Advanced Video Coding)
   - support for MPEG1/2, WMV1/2/3, WVC1, VP5/6, SVQ1/3, DV and more...
   - additional support for MSMPEG4v1, MSMPEG4v2, MSMPEG4v3 and H.263
   - deinterlacing support (set interlace flag) for H.264 MBAFF.
   - hardware deinterlacing support for RAW video
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
   - multithreaded resize: faster resize on multi-core CPUs
   - aspect ratio changing
   - subtitles support
   - improved subtitle shadow. 3 shadow modes are available: classic, glowing and gradient
   - management of compatibility issues: "Use ffdshow only in:" option ensures 
     that ffdshow only runs in supported applications.
   - quality control is now configurable ("Decoder options" -> "Quality control")
   - completely free software: ffdshow is distributed under GPL

3. ffdshow-tryouts

   ffdshow was originally developed by Milan Cutka. Since Milan Cutka stopped
   updating in 2006, we launched a new project 'ffdshow-tryouts'.

4. Web links

   ffdshow-tryouts:
   http://ffdshow-tryout.sourceforge.net/
   http://sourceforge.net/project/showfiles.php?group_id=173941

   ffdshow:
   http://ffdshow.sourceforge.net/tikiwiki/
   http://sourceforge.net/projects/ffdshow/

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

   Doom9:
   http://forum.doom9.org/showthread.php?t=120465

   or Wikipedia, the free encyclopedia:
   http://en.wikipedia.org/wiki/Ffdshow

5. Copying

   All used sources (except of cpu utilization detection routine) and ffdshow
   itself are distributed under GPL. See copying.txt


   Milan Cutka <milan_cutka@yahoo.com>
   ffdshow-tryouts
