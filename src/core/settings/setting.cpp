//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// setting.cpp


#include "misc.h"
#include "mayu.h"
#include "stringtool.h"
#include "windowstool.h"
#include "setting.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace Event
{
Key prefixed(_T("prefixed"));
Key before_key_down(_T("before-key-down"));
Key after_key_up(_T("after-key-up"));
Key *events[] = {
	&prefixed,
	&before_key_down,
	&after_key_up,
	NULL,
};
}


// get mayu filename
bool getFilenameFromConfig(const ConfigStore &i_config,
	tstringi *o_name, tstringi *o_filename, Setting::Symbols *o_symbols)
{
	int index;
	i_config.read(_T(".mayuIndex"), &index, 0);
	_TCHAR buf[100];
	_sntprintf(buf, NUMBER_OF(buf), _T(".mayu%d"), index);

	tstringi entry;
	if (!i_config.read(buf, &entry))
		return false;

	tregex getFilename(_T("^([^;]*);([^;]*);(.*)$"));
	tsmatch getFilenameResult;
	if (!std::regex_match(entry, getFilenameResult, getFilename))
		return false;

	if (o_name)
		*o_name = getFilenameResult.str(1);
	if (o_filename)
		*o_filename = getFilenameResult.str(2);
	if (o_symbols) {
		tstringi symbols = getFilenameResult.str(3);
		tregex symbol(_T("-D([^;]*)(.*)$"));
		tsmatch symbolResult;
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
	tstringi filename;
#ifndef USE_INI
	if (i_config && getFilenameFromConfig(*i_config, NULL, &filename, NULL) &&
			!filename.empty()) {
		tregex getPath(_T("^(.*[/\\\\])[^/\\\\]*$"));
		tsmatch getPathResult;
		if (std::regex_match(filename, getPathResult, getPath))
			o_pathes->push_back(getPathResult.str(1));
	}

	const _TCHAR *home = _tgetenv(_T("HOME"));
	if (home)
		o_pathes->push_back(home);

	const _TCHAR *homedrive = _tgetenv(_T("HOMEDRIVE"));
	const _TCHAR *homepath = _tgetenv(_T("HOMEPATH"));
	if (homedrive && homepath)
		o_pathes->push_back(tstringi(homedrive) + homepath);

	const _TCHAR *userprofile = _tgetenv(_T("USERPROFILE"));
	if (userprofile)
		o_pathes->push_back(userprofile);

	_TCHAR buf[GANA_MAX_PATH];
	DWORD len = GetCurrentDirectory(NUMBER_OF(buf), buf);
	if (0 < len && len < NUMBER_OF(buf))
		o_pathes->push_back(buf);
#else //USE_INI
	_TCHAR buf[GANA_MAX_PATH];
#endif //USE_INI

	if (GetModuleFileName(GetModuleHandle(NULL), buf, NUMBER_OF(buf)))
		o_pathes->push_back(pathRemoveFileSpec(buf));
}
