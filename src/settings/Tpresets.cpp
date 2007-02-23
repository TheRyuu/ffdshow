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
#include "Tpresets.h"
#include "reg.h"
#include "TpresetSettings.h"
#include "TpresetSettingsVideo.h"
#include "TpresetSettingsAudio.h"
#include "IffdshowBase.h"
#include "IffdshowDec.h"
#include "IffdshowDecVideo.h"
#include "IffdshowDecAudio.h"

//========================================== Tpresets =========================================
Tpresets::~Tpresets()
{
 done();
}
void Tpresets::listRegKeys(strings &l)
{
 l.clear();

 HKEY hKey;
 char_t rkey[MAX_PATH];tsprintf(rkey,FFDSHOW_REG_PARENT _l("\\%s"),reg_child);
 RegOpenKeyEx(HKEY_CURRENT_USER,rkey,0,KEY_READ,&hKey);
 for (int i=0,retCode=ERROR_SUCCESS;retCode==ERROR_SUCCESS;i++)
  {
   char_t keyName[256];DWORD keyNameSize=255;
   FILETIME ftLastWriteTime;
   retCode = RegEnumKeyEx(hKey,
                          i,
                          keyName,
                          &keyNameSize,
                          NULL,
                          NULL,
                          NULL,
                          &ftLastWriteTime
                         );
   if (retCode==ERROR_SUCCESS)
    l.push_back(ffstring(keyName));
   else
    break;
  }
 RegCloseKey(hKey);
}
void Tpresets::init(void)
{
 Tpreset *def=newPreset();
 def->loadDefault();
 push_back(def);

 strings keys;
 listRegKeys(keys);
 for (strings::const_iterator i=keys.begin();i!=keys.end();i++)
  if (findPreset(i->c_str())==end())
   {
    Tpreset *preset=newPreset(i->c_str());
    preset->loadReg();
    push_back(preset);
   }
 if (keys.size()==0)
  (*begin())->saveReg();
}
void Tpresets::done(void)
{
 for (iterator i=begin();i!=end();i++)
  delete *i;
 clear();
}
Tpresets::iterator Tpresets::findPreset(const char_t *presetName)
{
 for (iterator i=begin();i!=end();i++)
  if (_stricoll(presetName,(*i)->presetName)==0)
   return i;
 return end();
}

void Tpresets::storePreset(Tpreset *preset)
{
 iterator i=findPreset(preset->presetName);
 if (i!=end())
  *i=preset;
 else
  push_back(preset);
}

Tpreset* Tpresets::getPreset(const char_t *presetName,bool create)
{
 iterator i=findPreset(presetName);
 if (i!=end()) return *i;
 else
  if (create)
   {
    Tpreset *newpreset=newPreset(presetName);
    newpreset->loadReg();
    storePreset(newpreset);
    return newpreset;
   }
  else
   return NULL;
}

void Tpresets::savePreset(Tpreset *preset,const char_t *presetName)
{
 if (presetName)
  strcpy(preset->presetName,presetName);
 preset->saveReg();
 storePreset(preset);
}
bool  Tpresets::savePresetFile(Tpreset *preset,const char_t *flnm)
{
 bool res=preset->saveFile(flnm);
 if (res)
  storePreset(preset);
 return res;
}

bool Tpresets::removePreset(const char_t *presetName)
{
 iterator i=findPreset(presetName);
 if (i!=begin() && i!=end())
  {
   delete *i;
   erase(i);
   return true;
  }
 else return false;
}

void Tpresets::nextUniqueName(Tpreset *preset)
{
 nextUniqueName(preset->presetName);
}
void Tpresets::nextUniqueName(char_t *presetName)
{
 iterator i=findPreset(presetName);
 if (i==end()) return;
 for (int ii=1;;ii++)
  {
   char_t pomS[260];
   tsprintf(pomS,_l("%s %i"),presetName,ii);
   if (findPreset(pomS)==end())
    {
     strcpy(presetName,pomS);
     return;
    }
  }
}
void Tpresets::saveRegAll(void)
{
 for (iterator ii=begin();ii!=end();ii++)
  if (!(*ii)->autoLoadedFromFile)
   (*ii)->saveReg();

 strings keys;
 listRegKeys(keys);
 for (strings::const_iterator i=keys.begin();i!=keys.end();i++)
  if (i->c_str()[0]!='\0' && findPreset(i->c_str())==end())
   {
    char_t presetRegStr[256];
    tsprintf(presetRegStr,FFDSHOW_REG_PARENT _l("\\%s\\%s"),reg_child,i->c_str());
    RegDeleteKey(HKEY_CURRENT_USER,presetRegStr);
   }
}

Tpreset* Tpresets::getAutoPreset(IffdshowBase *deci,bool filefirst)
{
 TautoPresetProps aprops(deci);
 return getAutoPreset0(aprops,filefirst);
}
Tpreset* Tpresets::getAutoPreset0(TautoPresetProps &aprops,bool filefirst)
{
 if (filefirst)
  {
   const char_t *AVIname=aprops.getSourceFullFlnm();
   char_t drive[MAX_PATH],path[MAX_PATH],name[MAX_PATH];
   _splitpath(AVIname,drive,path,name,NULL);
   char_t presetFlnm[1024];
   _makepath(presetFlnm,drive,path,name,presetext);
   if (fileexists(presetFlnm))
    {
     Tpreset *preset=newPreset(AVIname);
     preset->loadFile(presetFlnm);
     Tpreset::normalizePresetName(preset->presetName,AVIname);
     preset->autoLoadedFromFile=true;
     iterator i=findPreset(preset->presetName);
     if (i!=end() && (*i)->autoLoadedFromFile) removePreset(preset->presetName);
     nextUniqueName(preset);
     push_back(preset);
     return preset;
    }
  }
 for (iterator i=begin();i!=end();i++)
  if ((*i)->isAutoPreset(aprops))
   return *i;
 return NULL;
}

//======================================= TpresetsVideo =======================================
Tpreset* TpresetsVideo::getAutoPreset(IffdshowBase *deci,bool filefirst)
{
 TvideoAutoPresetProps aprops(deci);
 return getAutoPreset0(aprops,filefirst);
}
Tpreset* TpresetsVideo::getAutoPreset0(TautoPresetProps &aprops,bool filefirst)
{
 if (Tpreset *preset=Tpresets::getAutoPreset0(aprops,filefirst))
  return preset;
 else
  {
   unsigned int dx,dy;
   ((TvideoAutoPresetProps&)aprops).getSourceResolution(&dx,&dy);
   for (iterator i=begin();i!=end();i++)
    {
     TpresetVideo *presetV=(TpresetVideo*)*i;
     if (presetV->autoloadSize && presetV->autoloadSizeMatch(dx,dy))
      return presetV;
    }
  }
 return NULL;
}
Tpreset* TpresetsVideo::newPreset(const char_t *presetName)
{
 return new TpresetVideo(reg_child,presetName?presetName:FFPRESET_DEFAULT);
}

//======================================= TpresetsVideo =======================================
Tpreset* TpresetsVideoPlayer::newPreset(const char_t *presetName)
{
 return new TpresetVideoPlayer(reg_child,presetName?presetName:FFPRESET_DEFAULT);
}

//======================================= TpresetsAudio =======================================
Tpreset* TpresetsAudio::newPreset(const char_t *presetName)
{
 return new TpresetAudio(presetName?presetName:FFPRESET_DEFAULT,reg_child);
}
Tpreset* TpresetsAudio::getAutoPreset(IffdshowBase *deci,bool filefirst)
{
 TaudioAutoPresetProps aprops(deci);
 return getAutoPreset0(aprops,filefirst);
}
