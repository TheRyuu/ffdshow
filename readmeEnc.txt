1. About ffdshow video encoder

Video for Windows and DirectShow codec based on VFW code from XviD 
project and using libavcodec library from FFmpeg project for 
compression. Few other compression libraries are supported too.

2. Features:

   - various compression methods:
       H264
       MPEG 4 compatible (Xvid, DivX 4, DivX 5) using libavcodec 
       or Xvid
       Divx 3 compatible, MSMPEG4v2, MSMPEG4v1
       WMV1/7, WMV2/8
       H263, H263+
       HuffYUV in YV12 colorspace
       MJPEG
       MPEG 1/2
       Theora (not for regular use, format isn't stabilized yet)
       all Windows Media Video 9 supported encoders with output 
       to asf/wmv file or AVI
   - all common encoding modes: constant bitrate, fixed quantizer, 
     fixed quality, two pass encoding (depends on compressor features)
   - for libavcodec and Xvid detailed selection of motion estimation method
   - minimum and maximum I frames interval
   - minimum and maximum quantizer selection, quantizer type and trellis 
     quantization selection for libavcodec
   - adaptive quantization (aka. masking) for libavcodec and Xvid, 
     single coefficient elimination
   - selectable compression of starting and ending credits
   - two curve compression algorithms for second pass of two pass encoding 
     thanks to Xvid developers
   - second pass simulation: although not very precise, still helpful 
     for tweaking advanced curve compression parameters
   - ability to use libavcodec internal two pass code
   - image preprocessing with ffdshow image filters 
    (latest ffdshow version must be installed)
   - graph during encoding: if your encoding program doesn't provide one
   - B frames support: from one to eight consecutive B frames
   - support for MPEG 4 quarterpel and GMC
   - selectable interlaced encoding
   - libavcodec's MPEG4/MPEG2/MPEG1 and x264 encoders are multithreaded
   - decompression

3. Web links

   ffdshow:
   http://ffdshow.sourceforge.net/tikiwiki/
   http://sourceforge.net/projects/ffdshow/
   http://www.ffdshow.info/

   FFmpeg:
   http://ffmpeg.mplayerhq.hu/

   Xvid:
   http://www.xvid.org/

   MPlayer:
   http://www.mplayerhq.hu/

   MJPEGtools:
   http://mjpeg.sourceforge.net/

   or at Doom9:
   http://forum.doom9.org/showthread.php?t=120465

4. Copying

   ffdshow is distributed under GPL. See copying.txt


   Milan Cutka <milan_cutka@yahoo.com>