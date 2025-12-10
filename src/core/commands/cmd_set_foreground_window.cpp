#include "cmd_set_foreground_window.h"
#include "../engine/engine.h"
#include "../functions/function.h" // For type tables and ToString operators
#include "../../platform/windows/windowstool.h" // For setForegroundWindow

// Helper to find window
struct FindWindowData {
    yamy::platform::IWindowSystem* ws;
    std::regex classRegex;
    std::regex titleRegex;
    yamy::platform::WindowHandle found;

    FindWindowData(yamy::platform::IWindowSystem* w, const std::string& c, const std::string& t)
        : ws(w), classRegex(c, std::regex::icase), titleRegex(t, std::regex::icase), found(nullptr) {}
};

void Command_SetForegroundWindow::load(SettingLoader *i_sl)
{
    const char* tName = Name;

    i_sl->getOpenParen(true, tName); // throw ...
    i_sl->load_ARGUMENT(&m_className);
    i_sl->getComma(false, tName); // throw ...
    i_sl->load_ARGUMENT(&m_titleName);
    if (i_sl->getCloseParen(false, tName))
      return;
    i_sl->getComma(false, tName); // throw ...
    i_sl->load_ARGUMENT(&m_logicalOp);
    i_sl->getCloseParen(true, tName); // throw ...
}

void Command_SetForegroundWindow::exec(Engine *i_engine, FunctionParam *i_param) const
{
    if (!i_param->m_isPressed)
        return;

    std::string classNameUtf8 = to_UTF_8(m_className.eval());
    std::string titleNameUtf8 = to_UTF_8(m_titleName.eval());

    FindWindowData data(i_engine->getWindowSystem(), classNameUtf8, titleNameUtf8);

    i_engine->getWindowSystem()->enumerateWindows([&data, this](yamy::platform::WindowHandle hwnd) -> bool {
        // Skip invisible windows if needed, but original code used FindWindow or similar
        // Original code logic isn't fully visible here, but FindWindow usually finds top-level.
        // enumerateWindows should iterate top-level windows.

        std::string cls = data.ws->getClassName(hwnd);
        std::string title = data.ws->getTitleName(hwnd);

        bool matchClass = std::regex_match(cls, data.classRegex);
        bool matchTitle = std::regex_match(title, data.titleRegex);

        bool match = false;
        if (m_logicalOp == LogicalOperatorType_or)
            match = matchClass || matchTitle;
        else
            match = matchClass && matchTitle;

        if (match) {
            data.found = hwnd;
            return false; // Stop enumeration
        }
        return true; // Continue
    });

    if (data.found) {
        setForegroundWindow(data.found);
    } else {
        // log warning
        tstring className = to_tstring(classNameUtf8);
        tstring titleName = to_tstring(titleNameUtf8);
        Acquire a(&i_engine->m_log, 0);
        i_engine->m_log << _T("Window not found: class='") << className << _T("', title='") << titleName << _T("'") << std::endl;
    }
}

tostream &Command_SetForegroundWindow::outputArgs(tostream &i_ost) const
{
    i_ost << m_className << _T(", ");
    i_ost << m_titleName << _T(", ");
    i_ost << m_logicalOp;
    return i_ost;
}
