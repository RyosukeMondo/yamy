#pragma once
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// layoutmanager.h


#ifndef _LAYOUTMANAGER_H
#  define _LAYOUTMANAGER_H

#  include "misc.h"
#  include "../platform/types.h"
#  include <list>


///
class LayoutManager
{
public:
    ///
    enum Origin {
        ORIGIN_LEFT_EDGE,                ///
        ORIGIN_TOP_EDGE = ORIGIN_LEFT_EDGE,        ///
        ORIGIN_CENTER,                ///
        ORIGIN_RIGHT_EDGE,                ///
        ORIGIN_BOTTOM_EDGE = ORIGIN_RIGHT_EDGE,    ///
    };

    ///
    enum Restrict {
        RESTRICT_NONE = 0,                ///
        RESTRICT_HORIZONTALLY = 1,            ///
        RESTRICT_VERTICALLY = 2,            ///
        RESTRICT_BOTH = RESTRICT_HORIZONTALLY | RESTRICT_VERTICALLY, ///
    };

private:
    ///
    class Item
    {
    public:
        yamy::platform::WindowHandle m_hwnd;                ///
        yamy::platform::WindowHandle m_hwndParent;                ///
        yamy::platform::Rect m_rc;                    ///
        yamy::platform::Rect m_rcParent;                ///
        Origin m_origin[4];                ///
    };

    ///
    class SmallestSize
    {
    public:
        yamy::platform::WindowHandle m_hwnd;                ///
        yamy::platform::Size m_size;                ///

    public:
        ///
        SmallestSize() : m_hwnd(nullptr) { }
    };

    typedef std::list<Item> Items;        ///

protected:
    yamy::platform::WindowHandle m_hwnd;                    ///

private:
    Items m_items;                ///
    Restrict m_smallestRestriction;        ///
    yamy::platform::Size m_smallestSize;                ///
    Restrict m_largestRestriction;        ///
    yamy::platform::Size m_largestSize;                ///

public:
    ///
    LayoutManager(yamy::platform::WindowHandle i_hwnd);

    /** restrict the smallest size of the window to the current size of it or
        specified by i_size */
    void restrictSmallestSize(Restrict i_restrict = RESTRICT_BOTH,
                              yamy::platform::Size *i_size = nullptr);

    /** restrict the largest size of the window to the current size of it or
        specified by i_size */
    void restrictLargestSize(Restrict i_restrict = RESTRICT_BOTH,
                             yamy::platform::Size *i_size = nullptr);

    /** Calculate new rectangle for a child window based on origins and parent resizing.
        Exposed for unit testing.
     */
    static yamy::platform::Rect calculateRect(const yamy::platform::Rect& originalParentRect,
                              const yamy::platform::Rect& originalChildRect,
                              const yamy::platform::Rect& currentParentRect,
                              const Origin origins[4]);

    ///
    bool addItem(yamy::platform::WindowHandle i_hwnd,
                 Origin i_originLeft = ORIGIN_LEFT_EDGE,
                 Origin i_originTop = ORIGIN_TOP_EDGE,
                 Origin i_originRight = ORIGIN_LEFT_EDGE,
                 Origin i_originBottom = ORIGIN_TOP_EDGE);
    ///
    void adjust() const;

    /// draw size box
    virtual bool wmPaint();

    /// size restriction
    virtual bool wmSizing(int i_edge, yamy::platform::Rect *io_rc);

    /// hittest for size box
    virtual bool wmNcHitTest(int i_x, int i_y);

    /// WM_SIZE
    virtual bool wmSize(uint32_t /* i_fwSizeType */, short /* i_nWidth */,
                        short /* i_nHeight */);

    /// forward message
    virtual bool defaultWMHandler(uint32_t i_message, WPARAM i_wParam,
                                  LPARAM i_lParam);
};


#endif // !_LAYOUTMANAGER_H
