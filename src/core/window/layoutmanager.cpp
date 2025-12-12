//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// layoutmanager.cpp

#ifdef _WIN32

#include "layoutmanager.h"
#include "windowstool.h"

#include <windowsx.h>


//
LayoutManager::LayoutManager(yamy::platform::WindowHandle i_hwnd)
        : m_hwnd(i_hwnd),
        m_smallestRestriction(RESTRICT_NONE),
        m_largestRestriction(RESTRICT_NONE)
{
}

// restrict the smallest size of the window to the current size of it or
// specified by i_size
void LayoutManager::restrictSmallestSize(Restrict i_restrict, yamy::platform::Size *i_size)
{
    m_smallestRestriction = i_restrict;
    if (i_size)
        m_smallestSize = *i_size;
    else {
        yamy::platform::Rect rc;
        GetWindowRect(static_cast<HWND>(m_hwnd), reinterpret_cast<RECT*>(&rc));
        m_smallestSize.cx = rc.right - rc.left;
        m_smallestSize.cy = rc.bottom - rc.top;
    }
}


// restrict the largest size of the window to the current size of it or
// specified by i_size
void LayoutManager::restrictLargestSize(Restrict i_restrict, yamy::platform::Size *i_size)
{
    m_largestRestriction = i_restrict;
    if (i_size)
        m_largestSize = *i_size;
    else {
        yamy::platform::Rect rc;
        GetWindowRect(static_cast<HWND>(m_hwnd), reinterpret_cast<RECT*>(&rc));
        m_largestSize.cx = rc.right - rc.left;
        m_largestSize.cy = rc.bottom - rc.top;
    }
}

//
bool LayoutManager::addItem(yamy::platform::WindowHandle i_hwnd, Origin i_originLeft,
                            Origin i_originTop,
                            Origin i_originRight, Origin i_originBottom)
{
    Item item;
    if (!i_hwnd)
        return false;
    item.m_hwnd = i_hwnd;
#ifdef MAYU64
    if (!(GetWindowLongPtr(static_cast<HWND>(i_hwnd), GWL_STYLE) & WS_CHILD))
#else
    if (!(GetWindowLong(static_cast<HWND>(i_hwnd), GWL_STYLE) & WS_CHILD))
#endif
        return false;
    item.m_hwndParent = (yamy::platform::WindowHandle)GetParent(static_cast<HWND>(i_hwnd));
    if (!item.m_hwndParent)
        return false;

    RECT rc;
    getChildWindowRect(static_cast<HWND>(i_hwnd), &rc);
    item.m_rc.left = rc.left; item.m_rc.top = rc.top; item.m_rc.right = rc.right; item.m_rc.bottom = rc.bottom;

    RECT rcParent;
    GetWindowRect(static_cast<HWND>(item.m_hwndParent), &rcParent);
    item.m_rcParent.left = rcParent.left; item.m_rcParent.top = rcParent.top; item.m_rcParent.right = rcParent.right; item.m_rcParent.bottom = rcParent.bottom;

    item.m_origin[0] = i_originLeft;
    item.m_origin[1] = i_originTop;
    item.m_origin[2] = i_originRight;
    item.m_origin[3] = i_originBottom;

    m_items.push_back(item);
    return true;
}

yamy::platform::Rect LayoutManager::calculateRect(const yamy::platform::Rect& originalParentRect,
                                  const yamy::platform::Rect& originalChildRect,
                                  const yamy::platform::Rect& currentParentRect,
                                  const Origin origins[4])
{
    yamy::platform::Rect outRect; // Default constructor initializes to (0,0,0,0)
    int32_t* outPtrs[4] = { &outRect.left, &outRect.top, &outRect.right, &outRect.bottom };
    int32_t originalChildPos[4] = { originalChildRect.left, originalChildRect.top, originalChildRect.right, originalChildRect.bottom };
    
    // Helper lambdas or just calculation
    int originalParentW = originalParentRect.right - originalParentRect.left;
    int originalParentH = originalParentRect.bottom - originalParentRect.top;
    int currentParentW = currentParentRect.right - currentParentRect.left;
    int currentParentH = currentParentRect.bottom - currentParentRect.top;
    
    int originalParentDim[4] = { originalParentW, originalParentH, originalParentW, originalParentH };
    int currentParentDim[4] = { currentParentW, currentParentH, currentParentW, currentParentH };
    
    for(int j=0; j<4; ++j) {
        switch(origins[j]) {
        case ORIGIN_LEFT_EDGE:
            *outPtrs[j] = originalChildPos[j];
            break;
        case ORIGIN_CENTER:
            *outPtrs[j] = currentParentDim[j] / 2 - (originalParentDim[j] / 2 - originalChildPos[j]);
            break;
        case ORIGIN_RIGHT_EDGE:
            *outPtrs[j] = currentParentDim[j] - (originalParentDim[j] - originalChildPos[j]);
            break;
        }
    }
    return outRect;
}

//
void LayoutManager::adjust() const
{
    for (Items::const_iterator i = m_items.begin(); i != m_items.end(); ++ i) {
        yamy::platform::Rect parentRect;
        GetWindowRect(static_cast<HWND>(i->m_hwndParent), reinterpret_cast<RECT*>(&parentRect));

        yamy::platform::Rect newChildRect = calculateRect(i->m_rcParent, i->m_rc, parentRect, i->m_origin);
        
        MoveWindow(static_cast<HWND>(i->m_hwnd), newChildRect.left, newChildRect.top,
                   newChildRect.right - newChildRect.left, 
                   newChildRect.bottom - newChildRect.top, FALSE);
    }
}


// draw size box
bool LayoutManager::wmPaint()
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(static_cast<HWND>(m_hwnd), &ps);
    RECT rc;
    GetClientRect(static_cast<HWND>(m_hwnd), &rc);
    rc.left = rc.right - GetSystemMetrics(SM_CXHTHUMB);
    rc.top = rc.bottom - GetSystemMetrics(SM_CYVTHUMB);
    DrawFrameControl(hdc, &rc, DFC_SCROLL, DFCS_SCROLLSIZEGRIP);
    EndPaint(static_cast<HWND>(m_hwnd), &ps);
    return true;
}


// size restriction
bool LayoutManager::wmSizing(int i_edge, yamy::platform::Rect *io_rc)
{
    switch (i_edge) {
    case WMSZ_TOPLEFT:
    case WMSZ_LEFT:
    case WMSZ_BOTTOMLEFT:
        if (m_smallestRestriction & RESTRICT_HORIZONTALLY)
            if (io_rc->right - io_rc->left < m_smallestSize.cx)
                io_rc->left = io_rc->right - m_smallestSize.cx;
        if (m_largestRestriction & RESTRICT_HORIZONTALLY)
            if (m_largestSize.cx < io_rc->right - io_rc->left)
                io_rc->left = io_rc->right - m_largestSize.cx;
        break;
    }
    switch (i_edge) {
    case WMSZ_TOPRIGHT:
    case WMSZ_RIGHT:
    case WMSZ_BOTTOMRIGHT:
        if (m_smallestRestriction & RESTRICT_HORIZONTALLY)
            if (io_rc->right - io_rc->left < m_smallestSize.cx)
                io_rc->right = io_rc->left + m_smallestSize.cx;
        if (m_largestRestriction & RESTRICT_HORIZONTALLY)
            if (m_largestSize.cx < io_rc->right - io_rc->left)
                io_rc->right = io_rc->left + m_largestSize.cx;
        break;
    }
    switch (i_edge) {
    case WMSZ_TOP:
    case WMSZ_TOPLEFT:
    case WMSZ_TOPRIGHT:
        if (m_smallestRestriction & RESTRICT_VERTICALLY)
            if (io_rc->bottom - io_rc->top < m_smallestSize.cy)
                io_rc->top = io_rc->bottom - m_smallestSize.cy;
        if (m_largestRestriction & RESTRICT_VERTICALLY)
            if (m_largestSize.cy < io_rc->bottom - io_rc->top)
                io_rc->top = io_rc->bottom - m_largestSize.cy;
        break;
    }
    switch (i_edge) {
    case WMSZ_BOTTOM:
    case WMSZ_BOTTOMLEFT:
    case WMSZ_BOTTOMRIGHT:
        if (m_smallestRestriction & RESTRICT_VERTICALLY)
            if (io_rc->bottom - io_rc->top < m_smallestSize.cy)
                io_rc->bottom = io_rc->top + m_smallestSize.cy;
        if (m_largestRestriction & RESTRICT_VERTICALLY)
            if (m_largestSize.cy < io_rc->bottom - io_rc->top)
                io_rc->bottom = io_rc->top + m_largestSize.cy;
        break;
    }
    return true;
}


// hittest for size box
bool LayoutManager::wmNcHitTest(int i_x, int i_y)
{
    POINT p = { i_x, i_y };
    ScreenToClient(static_cast<HWND>(m_hwnd), &p);
    RECT rc;
    GetClientRect(static_cast<HWND>(m_hwnd), &rc);
    if (rc.right - GetSystemMetrics(SM_CXHTHUMB) <= p.x &&
            rc.bottom - GetSystemMetrics(SM_CYVTHUMB) <= p.y) {
        SetWindowLongPtr(static_cast<HWND>(m_hwnd), DWLP_MSGRESULT, HTBOTTOMRIGHT);
        return true;
    }
    return false;
}


// WM_SIZE
bool LayoutManager::wmSize(uint32_t /* i_fwSizeType */, short /* i_nWidth */,
                           short /* i_nHeight */)
{
    adjust();
    RedrawWindow(static_cast<HWND>(m_hwnd), nullptr, nullptr,
                 RDW_ERASE | RDW_INVALIDATE | RDW_FRAME | RDW_ALLCHILDREN);
    return true;
}


// forward message
bool LayoutManager::defaultWMHandler(uint32_t i_message,
                                     uintptr_t i_wParam, intptr_t i_lParam)
{
    switch (i_message) {
    case WM_SIZE:
        return wmSize((uint32_t)i_wParam, LOWORD(i_lParam), HIWORD(i_lParam));
    case WM_PAINT:
        return wmPaint();
    case WM_SIZING:
        return wmSizing((int)i_wParam, reinterpret_cast<yamy::platform::Rect *>(i_lParam));
    case WM_NCHITTEST:
        return wmNcHitTest(static_cast<int>(static_cast<int16_t>(i_lParam & 0xFFFF)),
                          static_cast<int>(static_cast<int16_t>((i_lParam >> 16) & 0xFFFF)));
    }
    return false;
}

#endif // _WIN32
