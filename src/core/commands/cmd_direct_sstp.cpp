#include "cmd_direct_sstp.h"
#include "../engine/engine.h"
#include "../functions/function.h" // For type tables and ToString operators
#include "../../platform/windows/windowstool.h" // For loadString
#include "../../ui/mayurc.h" // For IDS_mayu
#include <map>

// Direct SSTP Server
class DirectSSTPServer
{
public:
    tstring m_path;
    yamy::platform::WindowHandle m_hwnd;
    tstring m_name;
    tstring m_keroname;

public:
    DirectSSTPServer()
            : m_hwnd(nullptr) {
    }
};


class ParseDirectSSTPData
{
    typedef std::match_results<const char*> MR;

public:
    typedef std::map<tstring, DirectSSTPServer> DirectSSTPServers;

private:
    DirectSSTPServers *m_directSSTPServers;

public:
    // constructor
    ParseDirectSSTPData(DirectSSTPServers *i_directSSTPServers)
            : m_directSSTPServers(i_directSSTPServers) {
    }

    bool operator()(const MR& i_what) {
#ifdef _UNICODE
        tstring id(to_wstring(std::string(i_what[1].first, i_what[1].second)));
        tstring member(to_wstring(std::string(i_what[2].first, i_what[2].second)));
        tstring value(to_wstring(std::string(i_what[3].first, i_what[3].second)));
#else
        tstring id(i_what[1].first, i_what[1].second);
        tstring member(i_what[2].first, i_what[2].second);
        tstring value(i_what[3].first, i_what[3].second);
#endif

        if (member == _T("path"))
            (*m_directSSTPServers)[id].m_path = value;
        else if (member == _T("hwnd"))
            (*m_directSSTPServers)[id].m_hwnd =
                reinterpret_cast<yamy::platform::WindowHandle>((intptr_t)_ttoi64(value.c_str()));
        else if (member == _T("name"))
            (*m_directSSTPServers)[id].m_name = value;
        else if (member == _T("keroname"))
            (*m_directSSTPServers)[id].m_keroname = value;
        return true;
    }
};

void Command_DirectSSTP::load(SettingLoader *i_sl)
{
    std::string sName = getName();
    const char* cName = sName.c_str();

    i_sl->getOpenParen(true, cName); // throw ...
    i_sl->load_ARGUMENT(&m_name);
    i_sl->getComma(false, cName); // throw ...
    i_sl->load_ARGUMENT(&m_protocol);
    i_sl->getComma(false, cName); // throw ...
    i_sl->load_ARGUMENT(&m_headers);
    i_sl->getCloseParen(true, cName); // throw ...
}

void Command_DirectSSTP::exec(Engine *i_engine, FunctionParam *i_param) const
{
    if (!i_param->m_isPressed)
        return;

    // check Direct SSTP server exist ?
    if (void* hm = i_engine->getWindowSystem()->openMutex("sakura"))
        i_engine->getWindowSystem()->closeHandle(hm);
    else {
        Acquire a(&i_engine->m_log, 0);
        i_engine->m_log << _T(" Error(1): Direct SSTP server does not exist.");
        return;
    }

    void* hfm = i_engine->getWindowSystem()->openFileMapping("Sakura");
    if (!hfm) {
        Acquire a(&i_engine->m_log, 0);
        i_engine->m_log << _T(" Error(2): Direct SSTP server does not provide data.");
        return;
    }

    char *data =
        reinterpret_cast<char *>(i_engine->getWindowSystem()->mapViewOfFile(hfm));
    if (!data) {
        i_engine->getWindowSystem()->closeHandle(hfm);
        Acquire a(&i_engine->m_log, 0);
        i_engine->m_log << _T(" Error(3): Direct SSTP server does not provide data.");
        return;
    }

    long length = *(long *)data;
    const char *begin = data + 4;
    const char *end = data + length;
    std::regex getSakura("([0-9a-fA-F]{32})\\.([^\x01]+)\x01(.*?)\r\n");

    ParseDirectSSTPData::DirectSSTPServers servers;
    std::regex_iterator<const char*>
    it(begin, end, getSakura), last;
    for (; it != last; ++it)
        ((ParseDirectSSTPData)(&servers))(*it);

    // make request
    tstring request;
    if (!m_protocol.eval().size())
        request += _T("NOTIFY SSTP/1.1");
    else
        request += to_tstring(m_protocol.eval());
    request += _T("\r\n");

    bool hasSender = false;
    for (std::list<std::string>::const_iterator
            i = m_headers.begin(); i != m_headers.end(); ++ i) {
        tstringq header = to_tstring(*i);
        if (_tcsnicmp(_T("Charset"), header.c_str(), 7) == 0 ||
                _tcsnicmp(_T("Hwnd"),    header.c_str(), 4) == 0)
            continue;
        if (_tcsnicmp(_T("Sender"), header.c_str(), 6) == 0)
            hasSender = true;
        request += header;
        request += _T("\r\n");
    }

    if (!hasSender) {
        request += _T("Sender: ");
        request += to_tstring(loadString(IDS_mayu));
        request += _T("\r\n");
    }

    _TCHAR buf[100];
    _sntprintf(buf, NUMBER_OF(buf), _T("HWnd: %Iu\r\n"),
               reinterpret_cast<uintptr_t>(i_engine->m_hwndAssocWindow));
    request += buf;

#ifdef _UNICODE
    request += _T("Charset: UTF-8\r\n");
#else
    request += _T("Charset: Shift_JIS\r\n");
#endif
    request += _T("\r\n");

#ifdef _UNICODE
    std::string request_UTF_8 = to_UTF_8(request);
#endif

    // send request to Direct SSTP Server which matches m_name;
    for (ParseDirectSSTPData::DirectSSTPServers::iterator
            i = servers.begin(); i != servers.end(); ++ i) {
        std::smatch what;
        std::string name_utf8 = to_UTF_8(i->second.m_name);
        if (std::regex_match(name_utf8, what, m_name)) {
            // Use local definition or rely on Win32 include but abstract it.
            // Since we are eliminating Win32 types, we use generic types.
            // But we must construct COPYDATASTRUCT for Windows message.
            // We'll rely on COPYDATASTRUCT from windows.h since this is a platform specific message.
            // But we cast it to intptr_t for the call.
            COPYDATASTRUCT cd;
            cd.dwData = 9801;
#ifdef _UNICODE
            cd.cbData = (uint32_t)request_UTF_8.size();
            cd.lpData = (void *)request_UTF_8.c_str();
#else
            cd.cbData = (uint32_t)request.size();
            cd.lpData = (void *)request.c_str();
#endif
            uintptr_t result;
            // 0x004A is WM_COPYDATA
            i_engine->getWindowSystem()->sendMessageTimeout(i->second.m_hwnd, 0x004A,
                               reinterpret_cast<uintptr_t>(i_engine->m_hwndAssocWindow),
                               reinterpret_cast<intptr_t>(&cd),
                               0x0002 | 0x0001, // SMTO_ABORTIFHUNG | SMTO_BLOCK
                               5000, &result);
        }
    }

    i_engine->getWindowSystem()->unmapViewOfFile(data);
    i_engine->getWindowSystem()->closeHandle(hfm);
}

tostream &Command_DirectSSTP::outputArgs(tostream &i_ost) const
{
    i_ost << m_name << _T(", ");
    i_ost << m_protocol << _T(", ");
    i_ost << m_headers;
    return i_ost;
}
