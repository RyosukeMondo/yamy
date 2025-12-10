// setting.cpp


#include "misc.h"
#include "mayu.h"
#include "stringtool.h"
#include "windowstool.h"
#include "setting.h"

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
    snprintf(buf, NUMBER_OF(buf), ".mayu%d", index);

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

    const char *home = getenv("HOME");
    if (home)
        o_pathes->push_back(home);

    const char *homedrive = getenv("HOMEDRIVE");
    const char *homepath = getenv("HOMEPATH");
    if (homedrive && homepath)
        o_pathes->push_back(std::string(homedrive) + homepath);

    const char *userprofile = getenv("USERPROFILE");
    if (userprofile)
        o_pathes->push_back(userprofile);

    char buf[GANA_MAX_PATH];
    DWORD len = GetCurrentDirectory(NUMBER_OF(buf), buf);
    if (0 < len && len < NUMBER_OF(buf))
        o_pathes->push_back(buf);
#else //USE_INI
    char buf[GANA_MAX_PATH];
#endif //USE_INI

    if (GetModuleFileName(GetModuleHandle(nullptr), buf, NUMBER_OF(buf)))
        o_pathes->push_back(pathRemoveFileSpec(buf));
}
