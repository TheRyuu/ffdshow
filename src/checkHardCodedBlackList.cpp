/*
 * Copyright (c) 2003-2006 Milan Cutka
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
#include <windows.h>
#include "Tconfig.h"
#include "TglobalSettings.h"
#include "checkHardCodedBlackList.h"
#include "TcompatibilityManager.h"

// Explorer.exe loads ffdshow.ax and never releases.
// That causes annoying error on re-install that one have to log off.
// With this patch, ffdshow.ax avoids to be loaded by returning false on DllMain
// if the caller is included in BlackList("Don't use ffdshow in:").
bool checkHardCodedBlackList(HINSTANCE hInstance)
{
    TcompatibilityManager::s_mode = 0;
    bool result = false;
    strings blacklistList2;
    HKEY hKey = NULL;
    LONG regErr;

    // read from registry directly because it is difficult to initialize Tconfig in DllMain.(Because it loads module)
    regErr = RegOpenKeyEx(HKEY_CURRENT_USER, FFDSHOW_REG_PARENT _l("\\") FFDSHOW, 0, KEY_READ, &hKey);
    if (regErr != ERROR_SUCCESS) {
        return false;
    }

    TregOpRegRead tHKCU_global(HKEY_CURRENT_USER, FFDSHOW_REG_PARENT _l("\\") FFDSHOW);
    tHKCU_global._REG_OP_N(IDFF_allowDPRINTF, _l("allowDPRINTF"), allowDPRINTF, 0);

    DWORD type;
    char_t blacklist[MAX_COMPATIBILITYLIST_LENGTH];
    ffstring fileName;
    /* get filename of calling application */
    ffstring cmd(GetCommandLine());
    //DPRINTF(_l("cmd %s"),cmd.c_str());

    // skip heading spaces
    while (cmd.at(0) == ' ') {
        cmd.erase(0, 1);
    }

    if (cmd.at(0) == '"') {
        cmd.erase(0, 1);
        size_t pos = cmd.find(_l("\""));
        if (pos != ffstring::npos) {
            cmd.erase(pos, cmd.length());
        }
    } else {
        size_t pos = cmd.find(_l(" "));
        if (pos != ffstring::npos) {
            cmd.erase(pos, cmd.length());
        }
    }
    extractfilename(cmd.c_str(), fileName);
    // DPRINTF(_l("fileName %s"),fileName.c_str());

    // FIXME: use TregOpRegRead
    DWORD isBlacklist;
    DWORD cbData = sizeof(isBlacklist);
    regErr = RegQueryValueEx(hKey, _l("isBlacklist"), NULL, &type, (LPBYTE)&isBlacklist, &cbData);
    if (regErr == ERROR_SUCCESS && isBlacklist) {
        cbData = sizeof(blacklist);
        regErr = RegQueryValueEx(hKey, _l("blacklist"), NULL, &type, (LPBYTE)blacklist, &cbData);
        if (regErr == ERROR_SUCCESS) {
            strings blacklistList;
            strtok(blacklist, _l(";"), blacklistList);

            for (strings::const_iterator b = blacklistList.begin(); b != blacklistList.end(); b++) {
                if (DwStrcasecmp(*b, _l("explorer.exe")) == 0) {
                    blacklistList2.push_back(_l("explorer.exe"));
                    break;
                }
            }
        }
    }

    /* some applications/games that are always blacklisted */
    blacklistList2.push_back(_l("oblivion.exe"));
    blacklistList2.push_back(_l("morrowind.exe"));
    blacklistList2.push_back(_l("YSO_WIN.exe"));
    blacklistList2.push_back(_l("WORMS 4 MAYHEM.EXE"));
    blacklistList2.push_back(_l("sh3.exe"));
    blacklistList2.push_back(_l("fallout3.exe"));
    blacklistList2.push_back(_l("hl2.exe"));
    blacklistList2.push_back(_l("gta_sa.exe"));
    blacklistList2.push_back(_l("age3.exe"));
    blacklistList2.push_back(_l("pes2008.exe"));
    blacklistList2.push_back(_l("pes2009.exe"));

    for (strings::const_iterator b = blacklistList2.begin(); b != blacklistList2.end(); b++) {
        if (DwStrcasecmp(*b, fileName) == 0) {
            result = true;
            break;
        }
    }

#if 0
    // explorer.exe may be present in both blacklist and whitelist. In this case, whitelist is prioritized (if you enable this block).
    if (!result && DwStrcasecmp(fileName, _l("explorer.exe")) == 0) {
        DWORD isWhitelist;
        regErr = RegQueryValueEx(hKey, _l("isWhitelist"), NULL, &type, (LPBYTE)&isWhitelist, &cbData);
        if (regErr == ERROR_SUCCESS && isWhitelist) {
            result = true;
            cbData = sizeof(blacklist);
            regErr = RegQueryValueEx(hKey, _l("whitelist"), NULL, &type, (LPBYTE)blacklist, &cbData);
            if (regErr == ERROR_SUCCESS) {
                strings whitelistList;
                strtok(blacklist, _l(";"), whitelistList);

                for (strings::const_iterator b = whitelistList.begin(); b != whitelistList.end(); b++) {
                    if (DwStrcasecmp(*b, _l("explorer.exe")) == 0) {
                        result = false;
                        break;
                    }
                }
            }
        }
    }
#endif

    if (hKey) {
        RegCloseKey(hKey);
    }
    return result;
}
