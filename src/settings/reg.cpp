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
#include "reg.h"
#include "Tstream.h"

//================================== TregOpStreamRead =================================
TregOpStreamRead::TregOpStreamRead(const void *buf,size_t len,char_t sep,bool Iloaddef):loaddef(Iloaddef)
{
 for (const char_t *cur=(const char_t*)buf,*end=(const char_t*)buf+len;cur<end;)
  {
   while (*cur==' ' && cur<=end)
    cur++;
   const char_t *s=(const char_t*)memchr(cur,sep,end-cur);
   if (!s)
    s=end;
   char_t line[256];
   strncpy(line,cur,s-cur);line[s-cur]='\0';
   char_t *ir=strchr(line,'=');
   if (ir)
    {
     *ir='\0';
     strs.insert(std::make_pair(ffstring(line),ffstring(ir+1)));
    }
   cur=s+1;
  }
}

//=============================== TregOpIDstreamWrite ===============================
bool TregOpIDstreamWrite::_REG_OP_N(short int id,const char_t *X,int &Y,const int)
{
 if (id)
  {
   append(id);
   append(Y);
  }
 return true;
}
void TregOpIDstreamWrite::_REG_OP_S(short int id,const char_t *X,char_t *Y,size_t buflen,const char_t *Z)
{
 if (id)
  {
   append((short int)-id);
   if (sizeof(char_t)==sizeof(wchar_t))
    {
     static const uint8_t marker[]={0xff,0xfe};
     append(marker,2);
    }
   append(Y,(strlen(Y)+1)*sizeof(char_t));
  }
}

//=============================== TregOpIDstreamRead ================================
TregOpIDstreamRead::TregOpIDstreamRead(const void *buf,size_t len,const void* *last)
{
 const char *p,*pEnd;
 for (p=(const char*)buf,pEnd=p+len;p<pEnd;)
  {
   short int id=*(short int*)p;
   p+=2;
   if (id>0)
    {
     vals.insert(std::make_pair(id,Tval(*(int*)p)));
     p+=4;
    }
   else if (id<0)
    {
     if ((uint8_t)(p[0])==0x0ff && (uint8_t)(p[1])==0xfe)
      {
       const wchar_t *pw=text<wchar_t>((const wchar_t*)(p+2));
       vals.insert(std::make_pair(-id,Tval(text<char_t>(pw))));
       p=(const char*)(strchr(pw,'\0')+1);
      }
     else
      {
       vals.insert(std::make_pair(-id,Tval(text<char_t>(p))));
       p=strchr(p,'\0')+1;
      }
    }
   else
    {
     p-=2;
     break;
    }
  }
 if (last) *last=p;
}
bool TregOpIDstreamRead::_REG_OP_N(short int id,const char_t *X,int &Y,const int Z)
{
 Tvals::const_iterator i=vals.find(id);
 if (i==vals.end())
  {
   Y=Z;
   return false;
  }
 else
  {
   Y=i->second.i;
   return true;
  }
}
void TregOpIDstreamRead::_REG_OP_S(short int id,const char_t *X,char_t *Y,size_t buflen,const char_t *Z)
{
 Tvals::const_iterator i=vals.find(id);
 strncpy(Y,i==vals.end()?Z:i->second.s.c_str(),buflen);Y[buflen-1]='\0';
}

//===================================================================================
static const char_t* hivename(HKEY hive)
{
 if      (hive==HKEY_LOCAL_MACHINE      ) return _l("HKEY_LOCAL_MACHINE");
 else if (hive==HKEY_CURRENT_USER       ) return _l("HKEY_CURRENT_USER");
 else if (hive==HKEY_CLASSES_ROOT       ) return _l("HKEY_CLASSES_ROOT");
 else if (hive==HKEY_USERS              ) return _l("HKEY_USERS");
 else if (hive==HKEY_PERFORMANCE_DATA   ) return _l("HKEY_PERFORMANCE_DATA");
 else if (hive==HKEY_PERFORMANCE_TEXT   ) return _l("HKEY_PERFORMANCE_TEXT");
 else if (hive==HKEY_PERFORMANCE_NLSTEXT) return _l("HKEY_PERFORMANCE_NLSTEXT");
 else if (hive==HKEY_CURRENT_CONFIG     ) return _l("HKEY_CURRENT_CONFIG");
 else if (hive==HKEY_DYN_DATA           ) return _l("HKEY_DYN_DATA");
 else return _l("");
}

bool regExport(Tstream &f,HKEY hive,const char_t *key,bool unicode)
{
 HKEY hkey=NULL;
 RegOpenKey(hive,key,&hkey);
 if (!hkey) return false;
 char_t subkey[256];
 int i;
 for (i=0;RegEnumKey(hkey,i,subkey,256)==ERROR_SUCCESS;i++)
  {
   char_t key2[256];tsprintf(key2,_l("%s\\%s"),key,subkey);
   regExport(f,hive,key2,unicode);
  }
 char_t valuename[256];DWORD valuenamelen=256;
 DWORD type,datalen;
 for (i=0;(valuenamelen=256,RegEnumValue(hkey,i,valuename,&valuenamelen,NULL,&type,NULL,&datalen))==ERROR_SUCCESS;i++)
  {
   BYTE *data=(BYTE*)_alloca(datalen);
   valuenamelen++;
   RegEnumValue(hkey,i,valuename,&valuenamelen,NULL,&type,data,&datalen);
   if (i==0) f.printf(_l("[%s\\%s]\n"),hivename(hive),key);
   switch (type)
    {
     case REG_DWORD:
      f.printf(_l("\"%s\"=dword:%08x\n"),valuename,*((DWORD*)data));
      break;
     case REG_SZ:
      {
       ffstring dataS=(const char_t*)data;
       f.printf(_l("\"%s\"=\"%s\"\n"),valuename,stringreplace(dataS,_l("\\"),_l("\\\\"),rfReplaceAll).c_str());
       break;
      }
    }
  }
 if (i>0) f.printf(_l("\n"));
 RegCloseKey(hkey);
 return true;
}
