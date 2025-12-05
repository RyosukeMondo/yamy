#ifndef _CONFIG_STORE_H
#define _CONFIG_STORE_H

#include "stringtool.h"
#include "misc.h"
#include <list>

/// Abstract interface for configuration storage (Registry, Ini, etc.)
class ConfigStore
{
public:
	typedef std::list<tstring> tstrings;

	virtual ~ConfigStore() {}

	/// remove key or value
	virtual bool remove(const tstring &i_name = _T("")) const = 0;

	/// check existence
	virtual bool doesExist() const = 0;

	/// read integer
	virtual bool read(const tstring &i_name, int *o_value, int i_defaultValue = 0) const = 0;
	
	/// write integer
	virtual bool write(const tstring &i_name, int i_value) const = 0;

	/// read string
	virtual bool read(const tstring &i_name, tstring *o_value,
					  const tstring &i_defaultValue = _T("")) const = 0;
	
	/// write string
	virtual bool write(const tstring &i_name, const tstring &i_value) const = 0;

#ifndef USE_INI
	/// read list of strings
	virtual bool read(const tstring &i_name, tstrings *o_value,
					  const tstrings &i_defaultValue = tstrings()) const = 0;
	
	/// write list of strings
	virtual bool write(const tstring &i_name, const tstrings &i_value) const = 0;
#endif //!USE_INI

	/// read binary data
	virtual bool read(const tstring &i_name, BYTE *o_value, DWORD *i_valueSize,
					  const BYTE *i_defaultValue = NULL, DWORD i_defaultValueSize = 0) const = 0;
	
	/// write binary data
	virtual bool write(const tstring &i_name, const BYTE *i_value,
					   DWORD i_valueSize) const = 0;
};

#endif // _CONFIG_STORE_H
