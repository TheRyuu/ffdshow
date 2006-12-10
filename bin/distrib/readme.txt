1. About ffdshow video decoder

ffdshow is a DirectShow decoding filter for decompressing DIVX movies.
It uses libavcodec from ffmpeg project or for video decompression (it can use
xvid.dll installed with xvid codec too), postprocessing code from mplayer to 
enhance visual quality of low bitrate movies, and is based on original
DirectShow filter from XviD, which is GPL'ed educational implementation
of MPEG4 encoder.

2. Features

- fast video decompression using optimized MMX, SSE and 3DNow! code
- support for different codecs: XviD and all DIVX versions
- additional support for MSMPEG4v1, MSMPEG4v2, MSMPEG4v3 and H263
- can act as generic postprocessing filter for other decoders like MPEG1 or MPEG2
- image postprocessing for higher playback quality
- automatic quality control: automatically reduces postprocessing level when CPU load is high
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

ffdshow: http://cutka.szm.sk/ffdshow/ or http://sourceforge.net/projects/ffdshow/
XviD:    http://www.xvid.org/
ffmpeg:  http://ffmpeg.org/
libmpeg2:http://libmpeg2.sourceforge.net/
mplayer: http://www.mplayerhq.hu/
DVD2AVI: http://arbor.ee.ntu.edu.tw/~jackei/dvd2avi/
xsharpen, unsharp mask, msharpen, hue and saturation code
         http://sauron.mordor.net/dgraft/index.html

and 
doom9:   http://www.doom9.org/

5. Copying

All used sources (except of cpu utilization detection routine) and ffdshow itself
are distributed under GPL. See copying.txt


Milan Cutka (cutka@szm.sk)