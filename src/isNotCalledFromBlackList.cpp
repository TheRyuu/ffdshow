#include "stdafx.h"
#include <windows.h>
#include "Tconfig.h"
#include "TglobalSettings.h"
#include "isNotCalledFromBlackList.h"
#include "TcompatibilityManager.h"

// Explorer.exe loads ffdshow.ax and never releases.
// That causes annoying error on re-install that one have to log off.
// With this patch, ffdshow.ax avoids to be loaded by returning false on DllMain
// if the caller is included in BlackList("Don't use ffdshow in:").
bool isNotCalledFromBlackList(HINSTANCE hInstance)
{
 TcompatibilityManager::s_mode=0;
 bool result= true;
 strings blacklistList2;
 HKEY hKey= NULL;
 LONG regErr;
 // read from registry directly because it is difficult to initialize Tconfig in DllMain.(Because it loads module)
 regErr= RegOpenKeyEx(HKEY_CURRENT_USER, _l("Software\\GNU\\ffdshow"), 0, KEY_READ, &hKey);
 if(regErr!=ERROR_SUCCESS)
  return true;
 DWORD type;
 DWORD isBlacklist;
 DWORD isWhitelist;
 DWORD cbData=sizeof(isBlacklist);
 char_t blacklist[MAX_COMPATIBILITYLIST_LENGTH];
 char_t fileName[MAX_PATH+2];
 char_t cmdBuf[MAX_PATH+3];
 char_t* cmdCopy=cmdBuf;
 char_t* cmd;
 char_t* endOfCmd;

 cmd= GetCommandLine();
 cmdCopy[MAX_PATH+2]='\0';
 strncpy(cmdCopy,cmd,MAX_PATH+2);
 if(cmdCopy[0]=='"')
  {
   cmdCopy++;
   if(cmdCopy[0]!=NULL)
    {
     endOfCmd= _tcschr(cmdCopy, '"');
     if(endOfCmd && endOfCmd <= &(cmdCopy[MAX_PATH+1]))
      *endOfCmd= '\0';
    }
  }
 extractfilename(cmdCopy,fileName);

 regErr= RegQueryValueEx(hKey, _l("isBlacklist"), NULL, &type, (LPBYTE)&isBlacklist, &cbData);
 if(regErr==ERROR_SUCCESS && isBlacklist)
  {
   cbData= sizeof(blacklist);
   regErr= RegQueryValueEx(hKey, _l("blacklist"), NULL, &type, (LPBYTE)blacklist, &cbData);
   if (regErr==ERROR_SUCCESS)
    {
     strings blacklistList;
     strtok(blacklist,_l(";"),blacklistList);

     for (strings::const_iterator b=blacklistList.begin();b!=blacklistList.end();b++)
      {
       if (DwStrcasecmp(*b,_l("explorer.exe"))==0)
        {
         blacklistList2.push_back(_l("explorer.exe"));
         break;
        }
      }
    }
  }
 blacklistList2.push_back(_l("oblivion.exe"));
 blacklistList2.push_back(_l("morrowind.exe"));
 blacklistList2.push_back(_l("YSO_WIN.exe"));
 for (strings::const_iterator b=blacklistList2.begin();b!=blacklistList2.end();b++)
  {
   if (DwStrcasecmp(*b,fileName)==0)
    {
     result= false;
     break;
    }
  }
 if (result && stricmp(fileName,_l("explorer.exe"))==0)
  {
   regErr= RegQueryValueEx(hKey, _l("isWhitelist"), NULL, &type, (LPBYTE)&isWhitelist, &cbData);
   if(regErr==ERROR_SUCCESS && isWhitelist)
    {
     result=false;
     cbData= sizeof(blacklist);
     regErr= RegQueryValueEx(hKey, _l("whitelist"), NULL, &type, (LPBYTE)blacklist, &cbData);
     if (regErr==ERROR_SUCCESS)
      {
       strings whitelistList;
       strtok(blacklist,_l(";"),whitelistList);

       for (strings::const_iterator b=whitelistList.begin();b!=whitelistList.end();b++)
        {
         if (DwStrcasecmp(*b,_l("explorer.exe"))==0)
          {
           result=true;
           break;
          }
        }
      }
    }
  }
 if(hKey)
  RegCloseKey(hKey);
 return result;
}
