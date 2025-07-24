#pragma once

#include <cstdint>

#include <xcb/xcb.h>

class x11_conn_manager
{
    typedef struct _XDisplay Display;

    Display*            m_Display;
    xcb_connection_t*   m_XCBConn;
    const xcb_screen_t* m_Screen;
    int                 m_ScreenNumber;

public:
    x11_conn_manager();
    ~x11_conn_manager();

    x11_conn_manager(const x11_conn_manager&)            = delete;
    x11_conn_manager& operator=(const x11_conn_manager&) = delete;

    operator bool() const { return m_Display != nullptr; }
    operator Display*() const { return m_Display; }
    operator xcb_connection_t*() const { return m_XCBConn; }

    const xcb_screen_t* GetScreen() const { return m_Screen; }
    int                 GetScreenNumber() const { return m_ScreenNumber; }
};

class glx_context_manager
{
    typedef struct __GLXcontextRec* GLXContext;

    const x11_conn_manager& m_Conn;
    GLXContext              m_GLXContext;
    int                     m_VisualId;

public:
    glx_context_manager(const x11_conn_manager& X11Session);
    ~glx_context_manager();

    operator bool() const { return m_GLXContext != nullptr; }

private:
};

class glx_window_manager
{
    const x11_conn_manager&    m_Conn;
    const glx_context_manager& m_GLXContextManager;
    const xcb_window_t         m_Window;
    uint32_t                   m_GLXWindow;

public:
    glx_window_manager(const x11_conn_manager& X11ConnectionManager, const glx_context_manager& GLXContextManager);
    ~glx_window_manager();

private:
};
