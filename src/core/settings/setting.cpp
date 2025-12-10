// setting.cpp


#include "misc.h"
#include "mayu.h"
#include "stringtool.h"
#include "windowstool.h"
#include "setting.h"
#include <regex>
#include <string>
#include <cstdio>
#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace Event
{
Key prefixed("prefixed");
Key before_key_down("before-key-down");
Key after_key_up("after-key-up");
Key *events[] = {
    &prefixed,
    &before_key_down,
    &after_key_up,
    nullptr,
};
}


// get mayu filename
bool getFilenameFromConfig(const ConfigStore &i_config,
    std::string *o_name, std::string *o_filename, Setting::Symbols *o_symbols)
{
    int index;
    i_config.read(".mayuIndex", &index, 0);
    char buf[100];
    snprintf(buf, sizeof(buf), ".mayu%d", index);

    std::string entry;
    if (!i_config.read(buf, &entry))
        return false;

    Regex getFilename("^([^;]*);([^;]*);(.*)$");
    std::smatch getFilenameResult;
    if (!std::regex_match(entry, getFilenameResult, getFilename))
        return false;

    if (o_name)
        *o_name = getFilenameResult.str(1);
    if (o_filename)
        *o_filename = getFilenameResult.str(2);
    if (o_symbols) {
        std::string symbols = getFilenameResult.str(3);
        Regex symbol("-D([^;]*)(.*)$");
        std::smatch symbolResult;
        while (std::regex_search(symbols, symbolResult, symbol)) {
            o_symbols->insert(symbolResult.str(1));
            symbols = symbolResult.str(2);
        }
    }
    return true;
}


// get home directory path
void getHomeDirectories(const ConfigStore *i_config, HomeDirectories *o_pathes)
{
    std::string filename;
#ifndef USE_INI
    if (i_config && getFilenameFromConfig(*i_config, nullptr, &filename, nullptr) &&
            !filename.empty()) {
        Regex getPath("^(.*[/\\\\])[^/\\\\]*$");
        std::smatch getPathResult;
        if (std::regex_match(filename, getPathResult, getPath))
            o_pathes->push_back(getPathResult.str(1));
    }

#ifdef _WIN32
    auto getenv_u = [](const wchar_t* name) -> std::string {
        wchar_t* val = _wgetenv(name);
        return val ? yamy::platform::wstring_to_utf8(val) : std::string();
    };

    std::string home = getenv_u(L"HOME");
    if (!home.empty())
        o_pathes->push_back(home);

    std::string homedrive = getenv_u(L"HOMEDRIVE");
    std::string homepath = getenv_u(L"HOMEPATH");
    if (!homedrive.empty() && !homepath.empty())
        o_pathes->push_back(homedrive + homepath);

    std::string userprofile = getenv_u(L"USERPROFILE");
    if (!userprofile.empty())
        o_pathes->push_back(userprofile);

    wchar_t wbuf[GANA_MAX_PATH];
    DWORD len = GetCurrentDirectoryW(NUMBER_OF(wbuf), wbuf);
    if (0 < len && len < NUMBER_OF(wbuf))
        o_pathes->push_back(yamy::platform::wstring_to_utf8(wbuf));

#else // Linux/Unix
    const char *home = getenv("HOME");
    if (home)
        o_pathes->push_back(home);

    char buf[GANA_MAX_PATH];
    if (getcwd(buf, sizeof(buf)))
        o_pathes->push_back(buf);
#endif

#endif // !USE_INI

#ifdef _WIN32
    wchar_t wbuf2[GANA_MAX_PATH];
    if (GetModuleFileNameW(GetModuleHandle(nullptr), wbuf2, NUMBER_OF(wbuf2)))
        o_pathes->push_back(pathRemoveFileSpec(yamy::platform::wstring_to_utf8(wbuf2)));
#else
    char buf2[GANA_MAX_PATH];
    ssize_t len = readlink("/proc/self/exe", buf2, sizeof(buf2)-1);
    if (len != -1) {
        buf2[len] = '\0';
        o_pathes->push_back(pathRemoveFileSpec(buf2));
    }
#endif
}
