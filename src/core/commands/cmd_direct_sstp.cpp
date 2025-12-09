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
    HWND m_hwnd;
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
                reinterpret_cast<HWND>((LONG_PTR)_ttoi64(value.c_str()));
        else if (member == _T("name"))
            (*m_directSSTPServers)[id].m_name = value;
        else if (member == _T("keroname"))
            (*m_directSSTPServers)[id].m_keroname = value;
        return true;
    }
};

void Command_DirectSSTP::load(SettingLoader *i_sl)
{
    tstring tsName = to_tstring(Name);
    const _TCHAR* tName = tsName.c_str();

    i_sl->getOpenParen(true, tName); // throw ...
    i_sl->load_ARGUMENT(&m_name);
    i_sl->getComma(false, tName); // throw ...
    i_sl->load_ARGUMENT(&m_protocol);
    i_sl->getComma(false, tName); // throw ...
    i_sl->load_ARGUMENT(&m_headers);
    i_sl->getCloseParen(true, tName); // throw ...
}

void Command_DirectSSTP::exec(Engine *i_engine, FunctionParam *i_param) const
{
    if (!i_param->m_isPressed)
        return;

    // check Direct SSTP server exist ?
    if (void* hm = i_engine->m_windowSystem->openMutex(_T("sakura")))
        i_engine->m_windowSystem->closeHandle(hm);
    else {
        Acquire a(&i_engine->m_log, 0);
        i_engine->m_log << _T(" Error(1): Direct SSTP server does not exist.");
        return;
    }

    void* hfm = i_engine->m_windowSystem->openFileMapping(_T("Sakura"));
    if (!hfm) {
        Acquire a(&i_engine->m_log, 0);
        i_engine->m_log << _T(" Error(2): Direct SSTP server does not provide data.");
        return;
    }

    char *data =
        reinterpret_cast<char *>(i_engine->m_windowSystem->mapViewOfFile(hfm));
    if (!data) {
        i_engine->m_windowSystem->closeHandle(hfm);
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
        request += m_protocol.eval();
    request += _T("\r\n");

    bool hasSender = false;
    for (std::list<tstringq>::const_iterator
            i = m_headers.begin(); i != m_headers.end(); ++ i) {
        if (_tcsnicmp(_T("Charset"), i->c_str(), 7) == 0 ||
                _tcsnicmp(_T("Hwnd"),    i->c_str(), 4) == 0)
            continue;
        if (_tcsnicmp(_T("Sender"), i->c_str(), 6) == 0)
            hasSender = true;
        request += i->c_str();
        request += _T("\r\n");
    }

    if (!hasSender) {
        request += _T("Sender: ");
        request += to_tstring(loadString(IDS_mayu));
        request += _T("\r\n");
    }

    _TCHAR buf[100];
    _sntprintf(buf, NUMBER_OF(buf), _T("HWnd: %Iu\r\n"),
               reinterpret_cast<ULONG_PTR>(i_engine->m_hwndAssocWindow));
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
        tsmatch what;
        if (std::regex_match(i->second.m_name, what, m_name)) {
            COPYDATASTRUCT cd;
            cd.dwData = 9801;
#ifdef _UNICODE
            cd.cbData = (DWORD)request_UTF_8.size();
            cd.lpData = (void *)request_UTF_8.c_str();
#else
            cd.cbData = (DWORD)request.size();
            cd.lpData = (void *)request.c_str();
#endif
            uintptr_t result;
            i_engine->m_windowSystem->sendMessageTimeout((WindowSystem::WindowHandle)i->second.m_hwnd, WM_COPYDATA,
                               reinterpret_cast<uintptr_t>(i_engine->m_hwndAssocWindow),
                               reinterpret_cast<intptr_t>(&cd),
                               SMTO_ABORTIFHUNG | SMTO_BLOCK, 5000, &result);
        }
    }

    i_engine->m_windowSystem->unmapViewOfFile(data);
    i_engine->m_windowSystem->closeHandle(hfm);
}

tostream &Command_DirectSSTP::outputArgs(tostream &i_ost) const
{
    i_ost << m_name << _T(", ");
    i_ost << m_protocol << _T(", ");
    i_ost << m_headers;
    return i_ost;
}
