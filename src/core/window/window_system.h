#ifndef _WINDOW_SYSTEM_H
#define _WINDOW_SYSTEM_H

#include "../utils/stringtool.h"

enum class WindowShowCmd {
    Normal,
    Maximized,
    Minimized,
    Unknown
};

class WindowSystem {
public:
    virtual ~WindowSystem() = default;

    typedef void* WindowHandle;

    virtual WindowHandle getParent(WindowHandle window) = 0;
    virtual bool isMDIChild(WindowHandle window) = 0;
    virtual bool isChild(WindowHandle window) = 0;
    virtual WindowShowCmd getShowCommand(WindowHandle window) = 0;
    virtual tstring getClipboardText() = 0;
    
    virtual tstring getClassName(WindowHandle window) = 0;
    virtual tstring getTitleName(WindowHandle window) = 0;
    virtual bool isConsoleWindow(WindowHandle window) = 0;
    virtual void setForegroundWindow(WindowHandle window) = 0;
};

#endif // !_WINDOW_SYSTEM_H
