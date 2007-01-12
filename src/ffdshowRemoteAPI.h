#ifndef _FFDSHOWREMOTEAPI_H_
#define _FFDSHOWREMOTEAPI_H_

#include "ffdshow_constants.h"

#define FFDSHOW_REMOTE_MESSAGE "ffdshow_remote_message"
#define FFDSHOW_REMOTE_CLASS "ffdshow_remote_class"

//lParam - parameter id to be used by WPRM_PUTPARAM, WPRM_GETPARAM and COPY_PUTPARAMSTR
#define WPRM_SETPARAM_ID 0

//lParam - new value of parameter
//returns TRUE or FALSE
#define WPRM_PUTPARAM    1

//lParam - unused
//return the value of parameter
#define WPRM_GETPARAM    2

//lParam - parameter id
#define WPRM_GETPARAM2   3

#define WPRM_STOP        4
#define WPRM_RUN         5

//returns playback status
// -1 - if not available
//  0 - stopped
//  1 - paused
//  2 - running
#define WPRM_GETSTATE    6

//returns movie duration in seconds
#define WPRM_GETDURATION 7
//returns current position in seconds
#define WPRM_GETCURTIME  8

#define WPRM_PREVPRESET 11
#define WPRM_NEXTPRESET 12 

//Set current time in seconds
#define WPRM_SETCURTIME 13

#define WPRM_SETADDTOROT 14



//WM_COPYDATA 
//COPYDATASTRUCT.dwData=
#define COPY_PUTPARAMSTR        9 // lpData points to new param value
#define COPY_SETACTIVEPRESET   10 // lpData points to new preset name
#define COPY_AVAILABLESUBTITLE_FIRST 11 // lpData points to buffer where first file name will be stored  - if no subtitle file is available, lpData will contain empty string
#define COPY_AVAILABLESUBTITLE_NEXT  12 // lpData points to buffer where next file name will be stored  - if no subtitle file is available, lpData will contain empty string
#define COPY_GETPARAMSTR       13 // lpData points to buffer where param value will be stored
#define COPY_GET_PRESETLIST		14 //Get the list of presets (array of strings)
#define COPY_GET_SOURCEFILE		15 //Get the filename currently played
#define COPY_CURRENT_SUBTITLES	16 //Get the subtitle filename currently read
#define COPY_GET_SUBTITLEFILESLIST 17 //Get the list of available subtitle files
#endif
