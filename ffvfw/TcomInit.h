/*
 * Copyright (c) 2009 h.yamagata
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */


#pragma once
// BOOST
#pragma warning (push)
#pragma warning(disable: 4244 4819)
#include "boost/thread.hpp"
#pragma warning (pop)

class TcomInit
{
    std::map<DWORD, HRESULT> CoInitializedThreads;
    boost::mutex mutex;

public:
    void init() {
        DPRINTF(_l("TcomInit::init"));
        boost::unique_lock<boost::mutex> lock(mutex);
        DWORD currentthread = GetCurrentThreadId();
        std::map<DWORD, HRESULT>::iterator i = CoInitializedThreads.find(currentthread);
        if (i == CoInitializedThreads.end()
                || FAILED(CoInitializedThreads[currentthread])) {
            CoInitializedThreads[currentthread] = CoInitialize(NULL);
        }
    }

    void uninit() {
        DPRINTF(_l("TcomInit::init"));
        boost::unique_lock<boost::mutex> lock(mutex);
        DWORD currentthread = GetCurrentThreadId();
        std::map<DWORD, HRESULT>::iterator i = CoInitializedThreads.find(currentthread);
        if (i == CoInitializedThreads.end()) {
            return;
        }
        if (SUCCEEDED(CoInitializedThreads[currentthread])) {
            CoUninitialize();
        }
        CoInitializedThreads.erase(i);
    }
};
