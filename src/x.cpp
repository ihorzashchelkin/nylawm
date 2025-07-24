#include <iostream>
#include <print>

#include <X11/Xlib-xcb.h>
#include <X11/Xlib.h>

#include <GL/glx.h>
#include <xcb/xcb.h>

#include "x.hpp"

x11_conn_manager::x11_conn_manager()
    : m_Display(XOpenDisplay(nullptr))
{
    if (!m_Display)
        std::println(std::cerr, "could not open display");

    m_XCBConn = XGetXCBConnection(m_Display);
    if (!m_XCBConn)
    {
        std::println(std::cerr, "could not get XCB connection from display");
        XCloseDisplay(m_Display);
        m_Display = nullptr;
    }

    XSetEventQueueOwner(m_Display, XCBOwnsEventQueue);

    m_ScreenNumber = DefaultScreen(m_Display);

    xcb_screen_iterator_t ScreenIter = xcb_setup_roots_iterator(xcb_get_setup(m_XCBConn));
    for (auto n = m_ScreenNumber; ScreenIter.rem && n > 0; --n, xcb_screen_next(&ScreenIter))
        ;

    m_Screen = ScreenIter.data;
}

x11_conn_manager::~x11_conn_manager()
{
    if (m_Display)
        XCloseDisplay(m_Display);
}

glx_context_manager::glx_context_manager(const x11_conn_manager& X11Session)
    : m_Conn { X11Session }
    , m_GLXContext {}
{
    static constexpr int VisualAttribs[] = {
        GLX_X_RENDERABLE, True,
        GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
        GLX_RENDER_TYPE, GLX_RGBA_BIT,
        GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR,
        GLX_RED_SIZE, 8,
        GLX_GREEN_SIZE, 8,
        GLX_BLUE_SIZE, 8,
        GLX_ALPHA_SIZE, 8,
        GLX_DEPTH_SIZE, 24,
        GLX_STENCIL_SIZE, 8,
        GLX_DOUBLEBUFFER, True,
        // GLX_SAMPLE_BUFFERS  , 1,
        // GLX_SAMPLES         , 4,
        None
    };

    int          NumFBConfigs;
    GLXFBConfig* FBConfigs = glXChooseFBConfig(X11Session, X11Session.GetScreenNumber(), VisualAttribs, &NumFBConfigs);
    if (!FBConfigs || !NumFBConfigs)
    {
        std::println("glXGetFBConfigs failed");
        return;
    }

    GLXFBConfig FBConfig = FBConfigs[0];
    glXGetFBConfigAttrib(X11Session, FBConfig, GLX_VISUAL_ID, &m_VisualId);

    if (!(m_GLXContext = glXCreateNewContext(X11Session, FBConfig, GLX_RGBA_TYPE, 0, True)))
    {
        std::println("glXCreateNewContext failed");
        return;
    }
}

glx_context_manager::~glx_context_manager()
{
    glXDestroyContext(m_Conn, m_GLXContext);
}

glx_window_manager::glx_window_manager(const x11_conn_manager& Conn, const glx_context_manager& GLXContextManager)
    : m_Conn { Conn }
    , m_GLXContextManager { GLXContextManager }
    , m_Window { xcb_generate_id(Conn) }
{
    xcb_colormap_t ColorMap = xcb_generate_id(m_Conn);
    xcb_create_colormap(
        m_Conn,
        XCB_COLORMAP_ALLOC_NONE,
        ColorMap,
        X11Session.GetScreen()->root,
        m_VisualId);

    xcb_create_window(
        m_X11Session,
        XCB_COPY_FROM_PARENT,
        Window,
        m_X11Session.GetScreen()->root,
        0, 0,
        150, 150,
        0,
        XCB_WINDOW_CLASS_INPUT_OUTPUT,
        m_VisualId,
        XCB_CW_EVENT_MASK | XCB_CW_COLORMAP,
        (uint32_t[]) { XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_KEY_PRESS, ColorMap, 0 });

    xcb_map_window(m_X11Session, Window);

    m_GLXWindow = glXCreateWindow(m_X11Session, FBConfig, Window, 0);
    glXMakeContextCurrent(m_X11Session, m_GLXWindow, m_GLXWindow, Context);

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glXSwapBuffers(Display, glxWindow);
}

#if 0
#endif
