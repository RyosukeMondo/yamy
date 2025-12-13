#include "cmd_direct_sstp.h"
#include "../engine/engine.h"
#include "../functions/function.h" // For type tables and ToString operators
#include "windowstool.h" // For loadString (platform-specific)
#include "../../ui/mayurc.h" // For IDS_mayu
#include "../../utils/stringtool.h" // For strcasecmp_utf8
#include "../platform/message_constants.h"
#include "../platform/ipc.h"
#include <map>
#include <cstring>
#include <cstdio>

// Direct SSTP Server
class DirectSSTPServer
{
public:
    std::string m_path;
    yamy::platform::WindowHandle m_hwnd;
    std::string m_name;
    std::string m_keroname;

public:
    DirectSSTPServer()
            : m_hwnd(nullptr) {
    }
};


class ParseDirectSSTPData
{
    typedef std::match_results<const char*> MR;

public:
    typedef std::map<std::string, DirectSSTPServer> DirectSSTPServers;

private:
    DirectSSTPServers *m_directSSTPServers;

public:
    // constructor
    ParseDirectSSTPData(DirectSSTPServers *i_directSSTPServers)
            : m_directSSTPServers(i_directSSTPServers) {
    }

    bool operator()(const MR& i_what) {
        std::string id(i_what[1].first, i_what[1].second);
        std::string member(i_what[2].first, i_what[2].second);
        std::string value(i_what[3].first, i_what[3].second);

        if (member == "path")
            (*m_directSSTPServers)[id].m_path = value;
        else if (member == "hwnd")
            (*m_directSSTPServers)[id].m_hwnd =
                reinterpret_cast<yamy::platform::WindowHandle>((intptr_t)std::stoll(value));
        else if (member == "name")
            (*m_directSSTPServers)[id].m_name = value;
        else if (member == "keroname")
            (*m_directSSTPServers)[id].m_keroname = value;
        return true;
    }
};

void Command_DirectSSTP::load(SettingLoader *i_sl)
{
    const char* tName = Name;

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
    if (void* hm = i_engine->getWindowSystem()->openMutex("sakura"))
        i_engine->getWindowSystem()->closeHandle(hm);
    else {
        Acquire a(&i_engine->m_log, 0);
        i_engine->m_log << " Error(1): Direct SSTP server does not exist.";
        return;
    }

    void* hfm = i_engine->getWindowSystem()->openFileMapping("Sakura");
    if (!hfm) {
        Acquire a(&i_engine->m_log, 0);
        i_engine->m_log << " Error(2): Direct SSTP server does not provide data.";
        return;
    }

    char *data =
        reinterpret_cast<char *>(i_engine->getWindowSystem()->mapViewOfFile(hfm));
    if (!data) {
        i_engine->getWindowSystem()->closeHandle(hfm);
        Acquire a(&i_engine->m_log, 0);
        i_engine->m_log << " Error(3): Direct SSTP server does not provide data.";
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
    std::string request;
    if (!m_protocol.eval().size())
        request += "NOTIFY SSTP/1.1";
    else
        request += m_protocol.eval();
    request += "\r\n";

    bool hasSender = false;
    for (std::list<std::string>::const_iterator
            i = m_headers.begin(); i != m_headers.end(); ++ i) {
        const std::string& header = *i;
        // Case-insensitive comparison using strcasecmp_utf8
        if (strcasecmp_utf8(header.c_str(), "Charset") == 0 ||
                strcasecmp_utf8(header.c_str(), "Hwnd") == 0)
            continue;
        if (strcasecmp_utf8(header.c_str(), "Sender") == 0)
            hasSender = true;
        request += header;
        request += "\r\n";
    }

    if (!hasSender) {
        request += "Sender: ";
        request += loadString(IDS_mayu);
        request += "\r\n";
    }

    char buf[100];
    snprintf(buf, sizeof(buf), "HWnd: %zu\r\n",
               reinterpret_cast<size_t>(i_engine->m_hwndAssocWindow));
    request += buf;

    request += "Charset: UTF-8\r\n";
    request += "\r\n";

    std::string request_UTF_8 = request;

    // send request to Direct SSTP Server which matches m_name;
    for (ParseDirectSSTPData::DirectSSTPServers::iterator
            i = servers.begin(); i != servers.end(); ++ i) {
        std::smatch what;
        if (std::regex_match(i->second.m_name, what, m_name)) {
            yamy::platform::CopyData cd{
                9801,
                static_cast<uint32_t>(request_UTF_8.size()),
                request_UTF_8.c_str()
            };
            uintptr_t result;
            i_engine->getWindowSystem()->sendCopyData(
                i_engine->m_hwndAssocWindow,
                i->second.m_hwnd,
                cd,
                yamy::platform::SendMessageFlags::NORMAL,
                5000,
                &result);
        }
    }

    i_engine->getWindowSystem()->unmapViewOfFile(data);
    i_engine->getWindowSystem()->closeHandle(hfm);
}

std::ostream &Command_DirectSSTP::outputArgs(std::ostream &i_ost) const
{
    i_ost << m_name << ", ";
    i_ost << m_protocol << ", ";
    i_ost << m_headers;
    return i_ost;
}
