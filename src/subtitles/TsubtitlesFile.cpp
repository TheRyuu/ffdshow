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
#include <shlwapi.h>
#include "TsubtitlesFile.h"
#include "TsubreaderMplayer.h"
#include "TsubreaderVobsub.h"
#include "TsubreaderUSF.h"
#include "TsubtitlesSettings.h"
#include "IffdshowBase.h"
#include "distance.h"
#include "Tstream.h"

const char_t* TsubtitlesFile::exts[]=
{
 _l("utf"),
 _l("idx"),
 _l("sub"),
 _l("srt"),
 _l("smi"),
 _l("rt") ,
 _l("txt"),
 _l("ssa"),
 _l("aqt"),
 _l("mpl"),
 _l("usf"),
 NULL
};
const char_t* TsubtitlesFile::mask=_l("Subtitles (*.utf;*.sub;*.srt;*.smi;*.rt;*.txt;*.ssa;*.aqt;*.mpl;*.idx)\0*.utf;*.sub;*.srt;*.smi;*.rt;*.txt;*.ssa;*.aqt;*.mpl;*.usf;*.idx\0All files (*.*)\0*.*\0");

bool TsubtitlesFile::extMatch(const char_t *flnm)
{
 char_t ext[MAX_PATH];extractfileext(flnm,ext);strlwr(ext);
 for (int i=0;exts[i];i++)
  if (strcmp(ext,exts[i])==0)
   return true;
 return false; 
}

static void fixRelativePaths(strings &dirs,const char_t *basepath)
{
 for (strings::iterator d=dirs.begin();d!=dirs.end();d++)
  if (PathIsRelative(d->c_str()))
   {
    char_t absdir[MAX_PATH];_makepath(absdir,NULL,basepath,d->c_str(),NULL);
    char_t absdirC[MAX_PATH];
    PathCanonicalize(absdirC,absdir);
    *d=absdirC;
   }
}

void TsubtitlesFile::findSubtitlesFile(const char_t *aviFlnm,const char_t *sdir,char_t *subFlnm,size_t buflen,int heuristic,IcheckSubtitle *checkSubtitle)
{
 if (!subFlnm) return;
 if (heuristic)
  {
   char_t aviDsk[MAX_PATH],aviDir[MAX_PATH],aviName[MAX_PATH];
   _splitpath(aviFlnm,aviDsk,aviDir,aviName,NULL);
   char_t aviDskDirName[MAX_PATH];
   _makepath(aviDskDirName,aviDsk,aviDir,aviName,NULL);
   strings subfiles;
   findPossibleSubtitles(aviFlnm,sdir,subfiles);
   int mindist=INT_MAX;
   EditDistance dist;
   strings::const_iterator s=subfiles.end();
   typedef std::vector< std::pair<int,ffstring> > TsubsDists;
   TsubsDists subdists;
   for (strings::const_iterator sf=subfiles.begin();sf!=subfiles.end();sf++)
    {
     char_t sfDsk[MAX_PATH],sfDir[MAX_PATH],sfName[MAX_PATH];
     _splitpath(sf->c_str(),sfDsk,sfDir,sfName,NULL);
     char_t sfDskDirName[MAX_PATH];
     _makepath(sfDskDirName,sfDsk,sfDir,sfName,NULL);
     int newdist=dist.CalEditDistance(aviDskDirName,sfDskDirName,2*MAX_PATH);
     if (!checkSubtitle)
      {
       if (newdist<mindist)
        {
         s=sf;
         mindist=newdist;
         if (mindist==0)
          break;
        }
      }  
     else 
      subdists.push_back(std::make_pair(newdist,*sf)); 
    }
   if (!checkSubtitle) 
    {  
     if (s!=subfiles.end())
      {
       strncpy(subFlnm,s->c_str(),buflen);subFlnm[buflen-1]='\0';
       return;
      }
    }
   else
    {
     std::stable_sort(subdists.begin(),subdists.end());
     for (TsubsDists::const_iterator sd=subdists.begin();sd!=subdists.end();sd++)
      if (checkSubtitle->checkSubtitle(sd->second.c_str()))
       {
        strncpy(subFlnm,sd->second.c_str(),buflen);subFlnm[buflen-1]='\0';
        return;
       }
    }   
  }
 else
  { 
   char_t dsk[MAX_PATH],dir[MAX_PATH],name[MAX_PATH],ext[MAX_PATH];
   _splitpath(aviFlnm,dsk,dir,name,ext);
   char_t fname[MAX_PATH];_makepath(fname,NULL,NULL,name,ext); //fname - movie file name with extension (without path)
   if (name[0])
    {
     char_t path[MAX_PATH];_makepath(path,dsk,dir,NULL,NULL);    //path - directory where movie is stored
     strings dirs;strtok(sdir,_l(";"),dirs);
     fixRelativePaths(dirs,path);
     //exact match (only extension differs)
     for (strings::const_iterator d=dirs.begin();d!=dirs.end();d++)
      {
       if (!d) continue;
       char_t subFlnm0[MAX_PATH];
       //sub_extsfind(d->c_str(),name,subFlnm0);
       for (const char_t* *e=exts;*e;e++)
        {
         _makepath(subFlnm0,NULL,d->c_str(),name,*e);
         if (fileexists(subFlnm0) && (!checkSubtitle || checkSubtitle->checkSubtitle(subFlnm0)))
          {
           strncpy(subFlnm,subFlnm0,buflen);subFlnm[buflen-1]='\0';
           return;
          }
        }  
      }
     //movie.avi -> movie en.txt, movie.en.sub,....
     for (strings::const_iterator d=dirs.begin();d!=dirs.end();d++)
      {
       _makepath(fname,NULL,d->c_str(),name,NULL);strcat(fname,_l("*")); //fname - c:\movies\movie*.*
       strings files;findFiles(fname,files);
       for (strings::const_iterator f=files.begin();f!=files.end();f++)
        {
         char_t ext[MAX_PATH];extractfileext(f->c_str(),ext);strlwr(ext);
         for (const char_t* *e=exts;*e;e++)
          if (strcmp(ext,*e)==0 && (!checkSubtitle || checkSubtitle->checkSubtitle(f->c_str())))
           {
            strncpy(subFlnm,f->c_str(),buflen);subFlnm[buflen-1]='\0';
            return;
           } 
        }   
      }
    }  
  } 
 subFlnm[0]='\0';  
}

void TsubtitlesFile::findPossibleSubtitles(const char_t *dir,strings &files)
{
 char_t autosubmask[MAX_PATH];
 _makepath(autosubmask,NULL,dir,/*name*/_l("*"),_l("*"));
 strings files0;
 findFiles(autosubmask,files0);
 for (strings::iterator f=files0.begin();f!=files0.end();f++)
  if (extMatch(f->c_str()))
   files.push_back(*f);
 std::sort(files.begin(),files.end(),ffstring_iless());
}
void TsubtitlesFile::findPossibleSubtitles(const char_t *aviFlnm,const char_t *sdir,strings &files)
{
 if (!aviFlnm || !sdir) return;
 char_t avidir[MAX_PATH];
 if (aviFlnm[0])
  {
   char_t dsk[MAX_PATH],dir[MAX_PATH],name[MAX_PATH];
   _splitpath(aviFlnm,dsk,dir,name,NULL);
   _makepath(avidir,dsk,dir,NULL,NULL);
  } 
 else
  strcpy(avidir,_l(".")); 
 strings dirs;strtok(sdir,_l(";"),dirs);
 fixRelativePaths(dirs,avidir);
 for (strings::const_iterator sd=dirs.begin();sd!=dirs.end();sd++)
  findPossibleSubtitles(sd->c_str(),files);
}

TsubtitlesFile::TsubtitlesFile(IffdshowBase *Ideci):Tsubtitles(Ideci)
{
 hwatch=NULL;
 subFlnm[0]='\0';
}
TsubtitlesFile::~TsubtitlesFile()
{
 if (hwatch) FindCloseChangeNotification(hwatch);
}

void TsubtitlesFile::done(void)
{
 Tsubtitles::done();
 if (hwatch) FindCloseChangeNotification(hwatch);hwatch=NULL;
}

bool TsubtitlesFile::init(const TsubtitlesSettings *cfg,const char_t *IsubFlnm,double Ifps,bool watch,int checkOnly)
{
 if (!cfg->is)
  return false;

 if (!IsubFlnm || (checkOnly==2 || stricmp(subFlnm,IsubFlnm)!=0))
  strcpy(subFlnm,IsubFlnm?IsubFlnm:_l(""));
 if (checkOnly<2)
  { 
   fps=Ifps;
   done();
  } 
 if (subFlnm[0]=='\0') return false;
 FILE *f=fopen(subFlnm,_l("r"));if (!f) return false;
 TstreamFile fs(f);
 sub_format=Tsubreader::sub_autodetect(fs,ffcfg);
 if (sub_format!=Tsubreader::SUB_INVALID && checkOnly!=2)
  {
   fs.rewind();
   if ((sub_format&Tsubreader::SUB_FORMATMASK)==Tsubreader::SUB_VOBSUB)
    {
     if (cfg->vobsub)
      {
       subs=new TsubreaderVobsub(fs,subFlnm,deci);
       int deflang=subs->findlang(cfg->deflang);
       if (deflang==0)
        deflang=subs->findlang(cfg->deflang2);
       subs->setLang(deflang);
      }
    }
   else if ((sub_format&Tsubreader::SUB_FORMATMASK)==Tsubreader::SUB_USF)
    subs=new TsubreaderUSF2(fs,deci,false);
   else 
    subs=new TsubreaderMplayer(fs,sub_format,fps,cfg,ffcfg,false);
  }  
 fclose(f);
 if (checkOnly==2)
  return sub_format!=Tsubreader::SUB_INVALID;
 if (!subs || subs->empty()) 
  {
   if (subs) delete subs;subs=NULL;
   subFlnm[0]='\0';
   return false;
  }
 if (!checkOnly)
  { 
   subs->adjust_subs_time(6.0);
   Tsubtitles::init();
   char_t subPath[MAX_PATH];
   extractfilepath(subFlnm,subPath);
   if (watch)
    {
     hwatch=FindFirstChangeNotification(subPath,FALSE,FILE_NOTIFY_CHANGE_LAST_WRITE|FILE_NOTIFY_CHANGE_SIZE);
     lastwritetime=fileLastWriteTime(subFlnm);
    }
  }  
 return true; 
}

void TsubtitlesFile::checkChange(const TsubtitlesSettings *cfg,bool *forceChange)
{
 if (hwatch)
  {
   DWORD res=WaitForSingleObject(hwatch,0);
   if (res==WAIT_OBJECT_0 && lastwritetime!=fileLastWriteTime(subFlnm)) 
    {
     init(cfg,subFlnm,fps,true,0);
     if (forceChange) *forceChange=true;
    }
  }
}
